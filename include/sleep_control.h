#ifndef DEMONSEARCH_SLEEP_CONTROL_H
#define DEMONSEARCH_SLEEP_CONTROL_H

#include "logger.h"
#include "state.h"

/*
 * Blokuje wykonanie procesu na zdefiniowany interwał czasowy, zachowując gotowość
 * do natychmiastowego, asynchronicznego wybudzenia przez handlery sygnałów.
 * Wykorzystuje wywołanie systemowe przerywalne sygnałami (EINTR).
 */
void ds_sleep_interval(unsigned interval_sec, ds_runtime_state_t *state, ds_wakeup_reason_t *out_reason);

#endif /* DEMONSEARCH_SLEEP_CONTROL_H */
