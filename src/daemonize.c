#include "daemonize.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stddef.h>

/* Powiadamia rodzica o statusie oraz zamyka deksryptor */
static void notify_parent_and_close(int fd, char status)
{
    if (fd >= 0) {
        (void)write(fd, &status, 1);
        close(fd);
    }
}

/* Zwraca deskryptor w przypadku sukcesu, -1 jeśli inna instancja już działa. */
static int create_and_lock_pidfile(const char *pid_file)
{
    int fd;
    char buf[32];
    struct flock fl;

    if (!pid_file) return -1;

    fd = open(pid_file, O_RDWR | O_CREAT | O_CLOEXEC, 0644);
    if (fd < 0) return -1;

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    if (fcntl(fd, F_SETLK, &fl) < 0) {
        close(fd);
        return -1;
    }

    if (ftruncate(fd, 0) < 0) {
        close(fd);
        return -1;
    }

    snprintf(buf, sizeof(buf), "%ld\n", (long)getpid());
    if (write(fd, buf, strlen(buf)) != (ssize_t)strlen(buf)) {
        close(fd);
        return -1;
    }

    return fd;
}

int ds_daemonize(const ds_daemon_config_t *config)
{
    pid_t pid;                      /* PID pierwszego forkowania */
    int fd;                         /* Tymczasowy deskryptor do /dev/null, jeśli close_stdio jest true */
    int pid_fd = -1;                /* Deskryptor pliku PID, jeśli pid_file jest ustawiony */
    int status_pipe[2] = {-1, -1};  /* Pipe do komunikacji statusu z dzieckiem do rodzica ('1' = sukces, '0' = błąd) */
    struct rlimit rl;               /* Struktura do pobrania limitu liczby otwartych plików */
    char status = '0';              /* Status demonizacji, domyślnie '0' (błąd), ustawiany na '1' przy sukcesie */
    ssize_t nread;                  /* Liczba bajtów odczytanych z pipe */

    if (pipe(status_pipe) < 0) {
        return -1;
    }

    /* Pierwszy fork */
    pid = fork();
    if (pid < 0) {
        close(status_pipe[0]);
        close(status_pipe[1]);
        return -1;
    }

    if (pid > 0) {
        /* Rodzic oczekuje na status od dziecka */
        close(status_pipe[1]);
        nread = read(status_pipe[0], &status, 1);
        close(status_pipe[0]);

        /* Oczekujemy dokładnie 1 bajt i status '1' dla sukcesu */
        if (nread == 1 && status == '1') {
            exit(EXIT_SUCCESS);
        }

        /* Demonizacja nie powiodła się */
        return -1;
    }

    /* Dziecko kontynuuje demonizację */
    close(status_pipe[0]);

    /* Utworzenie nowej sesji i odłączenie od terminala */
    if (setsid() < 0) {
        notify_parent_and_close(status_pipe[1], '0');
        _exit(EXIT_FAILURE);
    }

    /* Ignorowanie sygnałów SIGHUP i SIGPIPE, które mogą być wysyłane do procesu po odłączeniu od terminala */
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) < 0 || sigaction(SIGPIPE, &sa, NULL) < 0) {
        notify_parent_and_close(status_pipe[1], '0');
        _exit(EXIT_FAILURE);
    }

    /* Drugi fork */
    pid = fork();

    if (pid < 0) {
        notify_parent_and_close(status_pipe[1], '0');
        _exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        _exit(EXIT_SUCCESS);
    }

    /* Lock pliku PID musi nalezec do finalnego procesu demona. */
    if (config->pid_file) {
        pid_fd = create_and_lock_pidfile(config->pid_file);

        /* Jeśli nie udało się utworzyć lub zablokować pliku PID, oznacza to, że inna instancja już działa. */
        if (pid_fd < 0) {
            fprintf(stderr, "Error: Daemon is already running or PID file locked.\n");
            notify_parent_and_close(status_pipe[1], '0');
            _exit(EXIT_FAILURE);
        }
    }

    /* Ustawienie maski plików na 0, aby mieć pełne uprawnienia do tworzenia plików. */
    umask(0);

    /* Zmiana katalogu roboczego na '/' jeśli keep_cwd jest false */
    if (!config->keep_cwd) {
        if (chdir("/") < 0) {
            if (pid_fd >= 0) {
                close(pid_fd);
            }
            notify_parent_and_close(status_pipe[1], '0');
            _exit(EXIT_FAILURE);
        }
    }

    /* Zamknięcie deskryptorów */
    if (config->close_stdio) {
        /* Pobieramy limit liczby otwartych plików, aby wiedzieć, ile deskryptorów zamknąć. */
        if (getrlimit(RLIMIT_NOFILE, &rl) < 0 || rl.rlim_max == RLIM_INFINITY) {
            rl.rlim_max = 1024; /* Bezpieczna domyślna wartość, jeśli nie można pobrać limitu lub jest nieskończony. */
        } else if (rl.rlim_max > 8192) {
            rl.rlim_max = 8192; /* Dodatkowe ograniczenie, aby uniknąć zamykania zbyt wielu deskryptorów w systemach z bardzo wysokim limitem. */
        }

        for (unsigned int i = 0; i < rl.rlim_max; i++) {
            /* Zamykamy wszystko poza standardowymi I/O i plikiem PID */
            if (i > 2 && (int)i != pid_fd && (int)i != status_pipe[1]) {
                close(i);
            }
        }

        /* Przekierowanie standardowych deskryptorów do /dev/null */
        fd = open("/dev/null", O_RDWR);
        if (fd >= 0) {
            dup2(fd, STDIN_FILENO); 
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            if (fd > 2) close(fd);  /* Zamykamy dodatkowy deskryptor, jeśli został otwarty. */
        }
    }

    /* Demonizacja zakończona sukcesem, powiadamiamy rodzica. */
    notify_parent_and_close(status_pipe[1], '1');

    /* Dziecko kontynuuje działanie jako demon. */
    return 0;
}
