/* Modul: kontrola uspienia i wybudzania miedzy cyklami skanowania. */

#ifndef DEMONSEARCH_SLEEP_CONTROL_H
#define DEMONSEARCH_SLEEP_CONTROL_H

#include "logger.h"
#include "state.h"

/*
 * Usypia proces na podany interwał, ale z możliwością wcześniejszego wybudzenia przez sygnał.
 * @param interval_sec liczba sekund do uśpienia (nie może być 0)
 * @param state wskaźnik do globalnego stanu runtime, używany do sprawdzania pending_requests (nie może być NULL)
 * @param out_reason opcjonalny wskanik do ds_wakeup_reason_t, który zostanie ustawiony na powód wybudzenia
 * 
 * Funkcja usypia proces w pętli, sprawdzając co sekundę, czy nie pojawiły się żądania z sygnałów. 
 * Jeśli pojawi się żądanie, proces zostaje natychmiast wybudzony.
 */
void ds_sleep_interval(unsigned interval_sec, ds_runtime_state_t *state, ds_wakeup_reason_t *out_reason);

#endif /* DEMONSEARCH_SLEEP_CONTROL_H */
