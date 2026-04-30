#include "args.h"
#include "daemonize.h"
#include "logger.h"
#include "worker.h"
#include "supervisor.h"
#include "proc_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>

int main(int argc, char *argv[])
{
    ds_args_t args;
    bool show_usage = false;
    ds_daemon_config_t daemon_cfg;
    const char *target_dir;
    size_t i;
    char resolved_dir[PATH_MAX];
    ds_proc_table_t ptable;

    /* Parsowanie i walidacja argumentów wejściowych. */
    if (ds_args_parse(argc, argv, &args, &show_usage) != 0) {
        if (show_usage) {
            ds_args_print_usage(argv[0]);
            return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
    }

    /* Zbudowanie absolutnej ścieżki. Wywoływane przed demonizacją, 
       aby upewnić się, że ścieżki względne są poprawnie rozwiązywane. */
    if (realpath(args.start_dir, resolved_dir) == NULL) {
        fprintf(stderr, "Error: Cannot resolve directory '%s'\n", args.start_dir);
        ds_args_free(&args);
        return EXIT_FAILURE;
    }
    target_dir = resolved_dir;

    /* Konfiguracja demona. */
    daemon_cfg.close_stdio = true;
    daemon_cfg.keep_cwd = false; 
    daemon_cfg.pid_file = "/tmp/demonsearch.pid"; 

    /* Demonizacja procesu. Od tego momentu proces działa w tle. */
    if (ds_daemonize(&daemon_cfg) < 0) {
		fprintf(stderr, "Error: Daemonization failed.\n");
        ds_args_free(&args);
        ds_logger_close();
        return EXIT_FAILURE;
    }

    /* Inicjalizacja loggera. */
    ds_logger_init("demonsearch", args.verbose);

	ds_log_verbose_info("component=supervisor event=start target_dir=\"%s\" worker_count=%zu", target_dir, args.pattern_count);

    /* Inicjalizacja tabeli procesów potrzebnej 
       nadzorcy do zarządzania workerami. */
    if (ds_ptable_init(&ptable, args.pattern_count) != 0) {
		ds_log_error("component=supervisor event=init_failed reason=proc_table_alloc_failed");
        ds_args_free(&args);
        ds_logger_close();
        return EXIT_FAILURE;
    }

    /* Rozwidlenie procesów. */
    for (i = 0; i < args.pattern_count; i++) {
        pid_t pid = fork();
        
        if (pid == 0) {
            /* Izolacja pamięci procesu potomnego (workera) od procesu rodzica (supervisora). 
               Worker dziedziczy skopiowaną przestrzeń adresową. Zwolniono tablicę
               procesów (ptable), ponieważ worker nie będzie jej używał. */
            ds_ptable_free(&ptable);

            int exit_code = ds_worker_run(target_dir, args.patterns[i], args.interval_sec, args.verbose);

            ds_args_free(&args);
            ds_logger_close();
            exit(exit_code);
        } 
        else if (pid > 0) {
            /* Proces nadrzędny rejestruje PID uruchomionego workera. */
            ds_ptable_add(&ptable, pid);
        } 
        else {
            /* W przypadku wyczerpania zasobów lub innego błędu podczas forkowania, 
               logujemy błąd i inicjujemy zamknięcie wszystkich workerów, 
               czekamy na ich zakończenie, zwalniamy zasoby i kończymy działanie z kodem błędu. */
			ds_log_error("component=supervisor event=fork_failed pattern=\"%s\"", args.patterns[i]);
            ds_ptable_signal_all(&ptable, SIGTERM);
            ds_ptable_wait_all(&ptable);
            ds_ptable_free(&ptable);
            ds_args_free(&args);
            ds_logger_close();
            return EXIT_FAILURE;
        }
    }

    /* Przekazanie kontroli do głównej pętli nadzorcy. */
    int supervisor_status = ds_supervisor_run(&ptable);

    /* Ostateczne sprzątanie zasobów wraz 
       z zakończeniem działania nadzorcy. */
    ds_ptable_free(&ptable);
    ds_args_free(&args);
    ds_logger_close();
    
    return supervisor_status;
}