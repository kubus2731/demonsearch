#include "worker.h"
#include "logger.h"
#include "scanner.h"
#include "signals.h"
#include "sleep_control.h"
#include "state.h"

#include <stdlib.h>
#include <unistd.h>

/* Callbacki dla skanera przypinane do loggera. */
static void worker_on_compare(const char *path, const char *pattern, int matched)
{
    ds_log_verbose_compare("worker", path, pattern, matched);
}

static void worker_on_match(const char *path, const char *pattern)
{
    ds_log_match_found("worker", path, pattern);
}

/* Mapowanie powodu wybudzenia na czytelny łańcuch znaków do logowania. */
static const char *get_wakeup_reason_str(ds_wakeup_reason_t reason)
{
    switch (reason) {
        case DS_WAKEUP_INTERVAL:
            return "interval";
        case DS_WAKEUP_SIGUSR1:
            return "sigusr1";
        case DS_WAKEUP_SIGUSR2:
            return "sigusr2";
        case DS_WAKEUP_OTHER:
            return "other";
        default:
            return "unknown";
    }
}

int ds_worker_run(const char *start_dir, const char *pattern, unsigned interval_sec, bool verbose)
{
    /* Deklaracja zmiennych */
    ds_runtime_state_t state;
    ds_scan_stats_t stats;
    int requests;
    int last_signal;
    ds_phase_t last_phase;  /* Przechowuje poprzedni stan, aby logować tylko zmiany stanu. */
    ds_scanner_callbacks_t callbacks = {
        .on_compare = worker_on_compare,
        .on_match = worker_on_match
    };

    /* Inicjalizacja stanu runtime. */
    state.role = DS_ROLE_WORKER;
    state.phase = DS_PHASE_SLEEPING;
    state.verbose = verbose;
    state.pending_requests = DS_REQ_NONE;
    last_phase = DS_PHASE_SLEEPING;

    if (ds_signals_install(&state) != 0) {
        ds_log_error("component=worker event=signal_handler_install_failed pattern=\"%s\"", pattern);
        return EXIT_FAILURE;
    }

    ds_log_verbose_info("component=worker event=start pattern=\"%s\"", pattern);

    /* Główna pętla procesu roboczego. Kontynuuje działanie aż 
       do otrzymania żądania zakończenia (DS_REQ_TERMINATE). */
    while (1) {
        ds_wakeup_reason_t wakeup_reason;

        /* Walidacja stanu po wybudzeniu ze snu. */
        requests = ds_signals_consume_requests(&state, &last_signal);

        if (last_signal != 0) {
            ds_log_verbose_info("component=worker event=signal_received pattern=\"%s\" signal=%d", pattern, last_signal);
        }

        if (requests & DS_REQ_TERMINATE) {
            break;
        }

        /* Jeśli proces ma zostać uśpiony bez natychmiastowego 
           ponownego skanowania, przejdź do snu. */
        if ((requests & DS_REQ_RESCAN) == 0 && (requests & DS_REQ_ABORT_SCAN)) {
            state.phase = DS_PHASE_SLEEPING;
            if (last_phase != DS_PHASE_SLEEPING) {
                ds_log_verbose_info("component=worker event=state_change state=sleeping pattern=\"%s\"", pattern);
                last_phase = DS_PHASE_SLEEPING;
            }
            ds_log_verbose_info("component=worker event=sleep pattern=\"%s\" interval_sec=%u", pattern, interval_sec);
            ds_sleep_interval(interval_sec, &state, &wakeup_reason);
            ds_log_verbose_info("component=worker event=wakeup pattern=\"%s\" reason=%s", pattern, get_wakeup_reason_str(wakeup_reason));
            continue;
        }

        /* Zarejestrowanie przejścia do fazy skanowania. */
        state.phase = DS_PHASE_SCANNING;
        if (last_phase != DS_PHASE_SCANNING) {
            ds_log_verbose_info("component=worker event=state_change state=scanning pattern=\"%s\"", pattern);
            last_phase = DS_PHASE_SCANNING;
        }

        /* Rozpoczęcie skanowania drzewa katalogów.
           Skanowanie może zostać przerwane asynchronicznie przez sygnał. */
        ds_log_verbose_info("component=worker event=scan_start pattern=\"%s\"", pattern);
        
        int scan_result = ds_scanner_scan_tree(start_dir, pattern, &state.pending_requests, &stats, &callbacks);
        if (scan_result != 0) {
            if (scan_result == -2) {
                /* Krytyczny błąd dostępu do katalogu startowego. */
                ds_log_error("component=worker event=root_dir_access_failed pattern=\"%s\" path=\"%s\"", pattern, start_dir);
                break;
            } else if (state.pending_requests != DS_REQ_NONE) {
                ds_log_verbose_info("component=worker event=scan_interrupted pattern=\"%s\"", pattern);
            } else {
                /* Krytyczny błąd argumentów. */
                ds_log_error("component=worker event=scan_invalid_args pattern=\"%s\"", pattern);
                break;
            }
        }

        /* Po zakończeniu skanowania, sprawdź zakolejkowane żądania. */
        requests = ds_signals_consume_requests(&state, &last_signal);
        if (last_signal != 0) {
            ds_log_verbose_info("component=worker event=signal_received pattern=\"%s\" signal=%d", pattern, last_signal);
        }
        
        if (requests & DS_REQ_TERMINATE) {
            break;
        }

        /* Obsługa żądania natychmiastowego ponownego skanowania. */
        if (requests & DS_REQ_RESCAN) {
            continue;
        }

        /* Po przerwaniu skanu lub normalnym zakończeniu, przejdź do snu. */
        state.phase = DS_PHASE_SLEEPING;
        if (last_phase != DS_PHASE_SLEEPING) {
            ds_log_verbose_info("component=worker event=state_change state=sleeping pattern=\"%s\"", pattern);
            last_phase = DS_PHASE_SLEEPING;
        }
        ds_log_verbose_info("component=worker event=sleep pattern=\"%s\" interval_sec=%u", pattern, interval_sec);
        ds_sleep_interval(interval_sec, &state, &wakeup_reason);
        ds_log_verbose_info("component=worker event=wakeup pattern=\"%s\" reason=%s", pattern, get_wakeup_reason_str(wakeup_reason));
    }

    /* Zakończenie pracy i zwolnienie zasobów przypisanych do workera. */
    ds_log_verbose_info("component=worker event=shutdown pattern=\"%s\"", pattern);
    ds_signals_uninstall();

    return EXIT_SUCCESS;
}