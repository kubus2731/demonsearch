/* Implementacja modulu: jeden cykl skanu workera dla wzorca. */

#include "worker.h"
#include "logger.h"
#include "scanner.h"
#include "signals.h"
#include "sleep_control.h"
#include "state.h"

#include <stdlib.h>
#include <unistd.h>

int ds_worker_run(const char *start_dir, const char *pattern, unsigned interval_sec, bool verbose)
{
    /* Deklaracja zmiennych */
    ds_runtime_state_t state;
    ds_scan_stats_t stats;
    int requests;
    int last_signal;
    ds_phase_t last_phase;  /* Przechowuje poprzedni stan, aby logować tylko zmiany stanu. */

    state.role = DS_ROLE_WORKER;
    state.phase = DS_PHASE_SLEEPING;
    state.verbose = verbose;
    state.pending_requests = DS_REQ_NONE;
    last_phase = DS_PHASE_SLEEPING;

    if (ds_signals_install(&state) != 0) {
        ds_log_error("component=worker event=signal_handler_install_failed pattern=\"%s\"", pattern);
        return EXIT_FAILURE;
    }

    ds_log_verbose_info("component=worker event=start pattern=\"%s\" pid=%d", pattern, (int)getpid());

    while (1) {
        ds_wakeup_reason_t wakeup_reason;

        requests = ds_signals_consume_requests(&state, &last_signal);

        if (last_signal != 0) {
            ds_log_verbose_signal_received(last_signal);
        }

        if (requests & DS_REQ_TERMINATE) {
            break;
        }

        /* Przejdź do snu, jeśli otrzymano SIGUSR2 bez równoczesnego SIGUSR1. */
        if ((requests & DS_REQ_RESCAN) == 0 && (requests & DS_REQ_ABORT_SCAN)) {
            state.phase = DS_PHASE_SLEEPING;
            if (last_phase != DS_PHASE_SLEEPING) {
                ds_log_verbose_info("component=worker event=state_change state=SLEEPING pattern=\"%s\" pid=%d", pattern, (int)getpid());
                last_phase = DS_PHASE_SLEEPING;
            }
            ds_sleep_interval(interval_sec, &state, &wakeup_reason);
            ds_log_verbose_wakeup(wakeup_reason);
            continue;
        }

        /* Stan ustawiony na SCANNING przed rozpoczęciem skanowania, aby logować odpowiednio zdarzenia. */
        state.phase = DS_PHASE_SCANNING;
        if (last_phase != DS_PHASE_SCANNING) {
            ds_log_verbose_info("component=worker event=state_change state=SCANNING pattern=\"%s\" pid=%d", pattern, (int)getpid());
            last_phase = DS_PHASE_SCANNING;
        }
        ds_log_verbose_info("component=worker event=scan_start pattern=\"%s\" pid=%d", pattern, (int)getpid());

        if (ds_scanner_scan_tree(start_dir, pattern, &state.pending_requests, &stats) != 0) {
            if (state.pending_requests != DS_REQ_NONE) {
                ds_log_verbose_info("component=worker event=scan_interrupted pattern=\"%s\"", pattern);
            } else {
                ds_log_error("component=worker event=scan_failed pattern=\"%s\"", pattern);
            }
        }

        requests = ds_signals_consume_requests(&state, &last_signal);
        if (last_signal != 0) {
            ds_log_verbose_signal_received(last_signal);
        }
        
        if (requests & DS_REQ_TERMINATE) {
            break;
        }

        /* Natychmiastowe ponowne skanowanie, jeśli otrzymano SIGUSR1. */
        if (requests & DS_REQ_RESCAN) {
            continue;
        }

        /* Po przerwaniu skanu (SIGUSR2) lub normalnym zakończeniu, przejdź do snu. */
        state.phase = DS_PHASE_SLEEPING;
        if (last_phase != DS_PHASE_SLEEPING) {
            ds_log_verbose_info("component=worker event=state_change state=SLEEPING pattern=\"%s\" pid=%d", pattern, (int)getpid());
            last_phase = DS_PHASE_SLEEPING;
        }
        ds_sleep_interval(interval_sec, &state, &wakeup_reason);
        ds_log_verbose_wakeup(wakeup_reason);
    }

    ds_log_verbose_info("component=worker event=state_change state=SHUTDOWN pattern=\"%s\" pid=%d", pattern, (int)getpid());
    ds_log_verbose_info("component=worker event=shutdown pattern=\"%s\"", pattern);

    ds_signals_uninstall();

    return EXIT_SUCCESS;
}