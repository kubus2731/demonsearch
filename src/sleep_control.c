#include "sleep_control.h"

#include <errno.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>

void ds_sleep_interval(unsigned interval_sec, ds_runtime_state_t *state, ds_wakeup_reason_t *out_reason)
{
	struct timespec ts;
	ds_wakeup_reason_t reason = DS_WAKEUP_INTERVAL;
	int err;

	if (state == NULL || interval_sec == 0) {
		return;
	}

	state->phase = DS_PHASE_SLEEPING;
	ds_log_verbose_info("component=%s event=sleep interval_sec=%u",
						(state->role == DS_ROLE_WORKER) ? "worker" : "supervisor", interval_sec);

	/* Użyto zegara monotonicznego w celu chronienia procesu 
	   przed niestabilnymi zmianami zegara systemowego. */
	if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
		ts.tv_sec += interval_sec;

        /* Proces usypiany do konkretnego punktu w czasie.
		   Wybudzenie następuje tylko po upływie czasu lub przez 
		   przerwanie systemowe (EINTR) wywołane nadejściem sygnału. */
		while ((err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL)) != 0) {
			if (err == EINTR) {
				break;
			}
			if (state->pending_requests != DS_REQ_NONE) {
				break;
			}
		}
	}

    /* Mapowanie stanu globalnego na konkretny powód wybudzenia. */
	if (state->pending_requests != DS_REQ_NONE) {
		if (state->pending_requests & DS_REQ_RESCAN) {
			reason = DS_WAKEUP_SIGUSR1;
		} else if (state->pending_requests & DS_REQ_ABORT_SCAN) {
			reason = DS_WAKEUP_SIGUSR2;
		} else {
			reason = DS_WAKEUP_OTHER;
		}
	}

	if (out_reason != NULL) {
		*out_reason = reason;
	}
}