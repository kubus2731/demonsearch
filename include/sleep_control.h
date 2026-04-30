#ifndef DEMONSEARCH_SLEEP_CONTROL_H
#define DEMONSEARCH_SLEEP_CONTROL_H

#include "state.h"

/* Powód wybudzenia procesu z trybu uśpienia. */
typedef enum ds_wakeup_reason {
    DS_WAKEUP_INTERVAL = 0,
    DS_WAKEUP_SIGUSR1 = 1,
    DS_WAKEUP_SIGUSR2 = 2,
    DS_WAKEUP_OTHER = 3,
} ds_wakeup_reason_t;

/*
 * Blokuje wykonanie procesu na zdefiniowany interwał czasowy, zachowując gotowość
 * do natychmiastowego, asynchronicznego wybudzenia przez handlery sygnałów.
 * Wykorzystuje wywołanie systemowe przerywalne sygnałami (EINTR).
 */
void ds_sleep_interval(unsigned interval_sec, ds_runtime_state_t *state, ds_wakeup_reason_t *out_reason);

#endif /* DEMONSEARCH_SLEEP_CONTROL_H */
