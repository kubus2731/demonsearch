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

/* Zmienne globalne */
static int g_pid_fd = -1;  /* Deskryptor pliku PID */
static char *g_pid_file = NULL; /* Ścieżka do pliku PID */

/* Powiadamia przez potok rodzica o statusie demonizacji i zamyka deskryptor. */
static void notify_parent_and_close(int fd, char status)
{
    if (fd >= 0) {
        (void)write(fd, &status, 1);
        close(fd);
    }
}

/* Tworzy plik PID i zakłada na niego ekskluzywną blokadę zapisu.
   Zwraca deskryptor pliku PID, lub -1 w przypadku błędu (np. jeśli plik jest już zablokowany). */
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

/* Czyści zasoby związane z plikiem PID. */
static void cleanup_pidfile(void)
{
    if (g_pid_fd >= 0) {
        close(g_pid_fd);
        g_pid_fd = -1;
    }
    if (g_pid_file) {
        unlink(g_pid_file); /* Usunięcie pliku PID przy zamykaniu procesu. */
        free(g_pid_file);
        g_pid_file = NULL;
    }
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

    /* Potok do synchronizacji statusu demonizacji między dzieckiem a rodzicem.
       Rodzic będzie czekał na wiadomość od dziecka, aby wiedzieć, czy demonizacja się powiodła, zanim zakończy działanie. */
    if (pipe(status_pipe) < 0) {
        return -1;
    }

    /* Pierwszy fork.
       Rodzic kończy działanie, a dziecko kontynuuje demonizację. */
    pid = fork();
    if (pid < 0) {
        close(status_pipe[0]);
        close(status_pipe[1]);
        return -1;
    }

    if (pid > 0) {
        /* Rodzic czeka na status demonizacji od dziecka. */
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
    close(status_pipe[0]);

    /* Utworzenie nowej sesji.
       Proces staje się liderem sesji i odłącza się od fizycznego terminala sterującego. */
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

    /* Drugi fork. Właściwy proces demonizacji zapobiegający sytuacji, 
       w której proces jako lider sesji może ponownie przywiązać się do terminala sterującego. */
    pid = fork();

    if (pid < 0) {
        notify_parent_and_close(status_pipe[1], '0');
        _exit(EXIT_FAILURE);
    }
    if (pid > 0) {
        /* Pierwsze dziecko kończy działanie, a drugie dziecko kontynuuje jako demon. */
        _exit(EXIT_SUCCESS);
    }

    /* Blokada pliku PID, jeśli jest skonfigurowany.*/
    if (config->pid_file) {
        pid_fd = create_and_lock_pidfile(config->pid_file);

        /* Jeśli nie udało się utworzyć lub zablokować pliku PID, oznacza to, że inna instancja już działa. */
        if (pid_fd < 0) {
            fprintf(stderr, "Error: Daemon is already running or PID file locked.\n");
            notify_parent_and_close(status_pipe[1], '0');
            _exit(EXIT_FAILURE);
        }
    }

    /* Reset uprawnień.
       Zapewnia, że demon ma pełną kontrolę nad plikami, które tworzy, 
       bez dziedziczenia niepożądanych uprawnień z procesu nadrzędnego. */
    umask(0);

    /* Odpięcie od bieżącego katalogu roboczego, jeśli konfiguracja tego wymaga.
       Zapobiega blokowaniu katalogu przez proces demon, co może utrudniać odmontowanie systemów plików. */
    if (!config->keep_cwd) {
        if (chdir("/") < 0) {
            if (pid_fd >= 0) {
                close(pid_fd);
            }
            notify_parent_and_close(status_pipe[1], '0');
            _exit(EXIT_FAILURE);
        }
    }

    /* Zamykanie standardowych deskryptorów I/O i przekierowanie ich do /dev/null, jeśli konfiguracja tego wymaga.
       Zapewnia, że demon nie będzie przypadkowo czytał z terminala lub pisał do niego. */
    if (config->close_stdio) {
        /* Pobranie limitu liczby otwartych plików */
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
            if (fd > 2) close(fd);  /* Zamknięcie dodatkowego deskryptora, jeśli został otwarty. */
        }
    }

    /* Sygnał przez potok zwalnia blokadę pierwotnego procesu 
       rodzica, informując go o sukcesie demonizacji. */
    notify_parent_and_close(status_pipe[1], '1');

    if (pid_fd >= 0 && config->pid_file != NULL) {
        g_pid_fd = pid_fd;
        g_pid_file = strdup(config->pid_file);

        /* Rejestracja funkcji sprzątającej, która usunie 
           plik PID przy normalnym zakończeniu procesu. */
        atexit(cleanup_pidfile);
    }

    /* Dziecko kontynuuje działanie jako demon. */
    return 0;
}
