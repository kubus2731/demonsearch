/* Implementacja modulu: zarządzanie cyklem życia i sygnałami procesów potomnych. */

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

    /* Blokowanie sygnałów, które będą obsługiwane w pętli nadzorcy za pomocą sigwait. */
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
    while ((early_pid = waitpid(-1, &early_status, WNOHANG)) > 0) {
        ds_ptable_remove(ptable, early_pid);
        ds_log_verbose_info("component=supervisor event=early_worker_exit pid=%d remaining_workers=%zu",
                    (int)early_pid, ptable->active_count);
    }
    if (early_pid < 0 && errno != ECHILD) {
        ds_log_error("component=supervisor event=waitpid_failed reason=%s", strerror(errno));
        return 1;
    }

    ds_log_verbose_info("component=supervisor event=wait_loop_enter total_workers=%zu", ptable->capacity);

    while (ptable->active_count > 0) {
        if (sigwait(&mask, &sig) != 0) {
            continue;
        }

        ds_log_verbose_signal_received(sig);

        /* Zamykanie demona (SIGTERM/SIGINT) */
        if (sig == SIGTERM || sig == SIGINT) {
			ds_log_verbose_info("component=supervisor event=shutdown_requested signal=%d action=terminate_workers", sig);
            ds_ptable_signal_all(ptable, SIGTERM);
            break; 
        }
        /* Sygnały do ponownego skanowania (SIGUSR1) lub przerwania skanu (SIGUSR2) */
        else if (sig == SIGUSR1 || sig == SIGUSR2) {
			ds_log_verbose_info("component=supervisor event=signal_forward signal=%d target=workers", sig);
            ds_ptable_signal_all(ptable, sig);
        }
        /* Śmierć dziecka (Worker się zakończył) */
        else if (sig == SIGCHLD) {
            pid_t pid;
            int status;
            
            while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
                ds_ptable_remove(ptable, pid);
				ds_log_verbose_info("component=supervisor event=worker_exit pid=%d remaining_workers=%zu", 
                            (int)pid, ptable->active_count);
            }
        }
    }

    /* Ostateczne sprzątanie i zapobieganie procesom Zombie */
	ds_log_verbose_info("component=supervisor event=cleanup_wait");
    ds_ptable_wait_all(ptable);

	ds_log_verbose_info("component=supervisor event=shutdown_complete");
    return 0;
}