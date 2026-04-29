#include "supervisor.h"
#include "logger.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int ds_supervisor_run(ds_proc_table_t *ptable)
{
    sigset_t mask;
    int sig;

    if (ptable == NULL || ptable->capacity == 0) {
        return 1;
    }

    /* Sygnały są blokowane na poziomie jądra. */
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGCHLD);
    
    if (sigprocmask(SIG_BLOCK, &mask, NULL) != 0) {
        ds_log_error("component=supervisor event=signal_mask_failed");
        return 1;
    }

    pid_t early_pid;
    int early_status;
    
    /* Sprzątanie workerów, które mogły się 
       zakończyć przed wejściem w główną pętlę. */
    while ((early_pid = waitpid(-1, &early_status, WNOHANG)) > 0) {
        ds_ptable_remove(ptable, early_pid);
        ds_log_verbose_info("component=supervisor event=early_worker_exit remaining_workers=%zu", ptable->active_count);
    }

    if (early_pid < 0 && errno != ECHILD) {
        ds_log_error("component=supervisor event=waitpid_failed reason=%s", strerror(errno));
        return 1;
    }

    ds_log_verbose_info("component=supervisor event=wait_loop_enter total_workers=%zu", ptable->capacity);

    /* Główna pętla zarządcy działająca tak długo jak 
       istnieją potomne procesy szukające (workery) */
    while (ptable->active_count > 0) {
        if (sigwait(&mask, &sig) != 0) {
            continue;
        }

        ds_log_verbose_info("component=supervisor event=signal_received signal=%d", sig);

        /* Zamykanie demona (SIGTERM/SIGINT) */
        if (sig == SIGTERM || sig == SIGINT) {
			ds_log_verbose_info("component=supervisor event=shutdown_requested signal=%d action=terminate_workers", sig);
            ds_ptable_signal_all(ptable, SIGTERM); /* Propagacja sygnału zakończenia do wszystkich workerów. */
            break; 
        }
        /* Sygnały do ponownego skanowania (SIGUSR1) lub przerwania skanu (SIGUSR2) */
        else if (sig == SIGUSR1 || sig == SIGUSR2) {
			ds_log_verbose_info("component=supervisor event=signal_forward signal=%d target=workers", sig);
            ds_ptable_signal_all(ptable, sig); /* Propagacja sygnału do wszystkich workerów. */
        }
        /* Śmierć dziecka (Worker się zakończył) */
        else if (sig == SIGCHLD) {
            pid_t pid;
            int status;
            
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                ds_ptable_remove(ptable, pid);
				ds_log_verbose_info("component=supervisor event=worker_exit remaining_workers=%zu", ptable->active_count);
            }
        }
    }

    /* Czekanie na zakończenie wszystkich workerów.
       Gwarantuje to poprawne zakończenie wszystkich procesów. */
	ds_log_verbose_info("component=supervisor event=cleanup_wait");
    ds_ptable_wait_all(ptable);

    /* Zakończenie pracy */
	ds_log_verbose_info("component=supervisor event=shutdown_complete");
    return 0;
}