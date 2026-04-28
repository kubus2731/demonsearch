/* Implementacja modulu: uspienie interwalowe i wybudzanie sygnalem. */

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
	ds_log_verbose_sleep(interval_sec);

	if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
		ts.tv_sec += interval_sec;
		while ((err = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &ts, NULL)) != 0) {
			if (err == EINTR) {
				break;
			}
			if (state->pending_requests != DS_REQ_NONE) {
				break;
			}
		}
	}

	if (state->pending_requests != DS_REQ_NONE) {
		if (state->pending_requests & DS_REQ_RESCAN) {
			reason = DS_WAKEUP_SIGUSR1;
		} else if (state->pending_requests & DS_REQ_ABORT_SCAN) {
			reason = DS_WAKEUP_SIGUSR2;
		} else {
			reason = DS_WAKEUP_OTHER;
		}
	}

	if (reason != DS_WAKEUP_SIGUSR2) {
		state->phase = DS_PHASE_SCANNING;
		ds_log_verbose_wakeup(reason);
	}

	if (out_reason != NULL) {
		*out_reason = reason;
	}
}