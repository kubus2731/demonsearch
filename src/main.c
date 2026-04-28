/* Punkt startowy: bootstrap modulow i glowna petla demona. */

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
    const char *raw_dir;
    ds_proc_table_t ptable;


    /* Parsowanie argumentów */
    if (ds_args_parse(argc, argv, &args, &show_usage) != 0) {
        if (show_usage) {
            ds_args_print_usage(argv[0]);
            return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
    }
    raw_dir = args.start_dir ? args.start_dir : "/";

    /* Zbudowanie absolutnej ścieżki PRZED chdir("/") z daemonize */
    if (realpath(raw_dir, resolved_dir) == NULL) {
        fprintf(stderr, "Error: Cannot resolve directory '%s'\n", raw_dir);
        ds_args_free(&args);
        return EXIT_FAILURE;
    }
    target_dir = resolved_dir;

    /* Inicjalizacja logera */
    ds_logger_init("demonsearch", args.verbose);

    /* Konfiguracja */
    daemon_cfg.close_stdio = true;
    daemon_cfg.keep_cwd = false; 
    daemon_cfg.pid_file = "/tmp/demonsearch.pid"; 

    /* Demonizacja procesu */
    if (ds_daemonize(&daemon_cfg) < 0) {
		ds_log_error("component=supervisor event=daemonize_failed reason=instance_lock_or_runtime_error");
        ds_args_free(&args);
        ds_logger_close();
        return EXIT_FAILURE;
    }
	ds_log_verbose_info("component=supervisor event=start target_dir=\"%s\" worker_count=%zu", target_dir, args.pattern_count);

    /* Inicjalizacja tabeli procesów nadzorcy */
    if (ds_ptable_init(&ptable, args.pattern_count) != 0) {
		ds_log_error("component=supervisor event=init_failed reason=proc_table_alloc_failed");
        ds_args_free(&args);
        ds_logger_close();
        return EXIT_FAILURE;
    }

    /* Rozwidlenie procesów */
    for (i = 0; i < args.pattern_count; i++) {
        pid_t pid = fork();
        
        if (pid == 0) {
            /*
             * PROCES DZIECKA (WORKER)
             * Każdy worker dostaje jeden unikalny wzorzec do przeszukania.
             * Worker loguje swoje zdarzenia, wykonuje skanowanie i kończy działanie z odpowiednim kodem wyjścia.
             * Po zakończeniu pracy, worker zwalnia zasoby i zamyka logger, 
             * ale NIE zwalnia tabeli procesów nadzorcy, ponieważ worker jej nie używa.
             */
            ds_ptable_free(&ptable);
            int exit_code = ds_worker_run(target_dir, args.patterns[i], args.interval_sec, args.verbose);
            ds_args_free(&args);
            ds_logger_close();
            exit(exit_code);
        } 
        else if (pid > 0) {
            /* 
             * PROCES RODZICA (SUPERVISOR)
             * Supervisor dodaje PID nowo utworzonego workera do swojej tabeli procesów,
             * ale NIE zwalnia zasobów związanych z argumentami,
             * ponieważ będą one potrzebne do logowania i ewentualnego ponownego uruchamiania workerów.
             * Supervisor kontynuuje tworzenie kolejnych workerów dla pozostałych wzorców.
             * Po utworzeniu wszystkich workerów, supervisor przechodzi w tryb nadzorcy,
             * czekając na sygnały i zarządzając cyklem życia workerów.
             */
            ds_ptable_add(&ptable, pid);
        } 
        else {
            /*
            * BŁĄD PRZY FORKOWANIU
            * Logujemy błąd, wysyłamy sygnał zakończenia do wszystkich już uruchomionych workerów, 
            * czekamy na ich zakończenie, sprzątamy zasoby i kończymy działanie z kodem błędu.
            */
			ds_log_error("component=supervisor event=fork_failed pattern=\"%s\"", args.patterns[i]);
            ds_ptable_signal_all(&ptable, SIGTERM);
            ds_ptable_wait_all(&ptable);
            ds_ptable_free(&ptable);
            ds_args_free(&args);
            ds_logger_close();
            return EXIT_FAILURE;
        }
    }

    /* Przejście w tryb Nadzorcy (Czekanie na sygnały i propagacja) */
    int supervisor_status = ds_supervisor_run(&ptable);

    /* Sprzątanie zasobów Supervisora */
    ds_ptable_free(&ptable);
    ds_args_free(&args);
    ds_logger_close();

    return supervisor_status;
}