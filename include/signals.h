#ifndef DEMONSEARCH_SIGNALS_H
#define DEMONSEARCH_SIGNALS_H

#include "state.h"

/*
 * Instaluje asynchroniczne handlery sygnałów (SIGUSR1, SIGUSR2, SIGINT, SIGTERM)
 * i mapuje je na wewnętrzne flagi maszyny stanów (pending_requests) w ds_runtime_state_t.
 * 
 * Zwraca 0 w przypadku sukcesu, -1 w przeciwnym razie.
 */
int ds_signals_install(ds_runtime_state_t *state);

/*
 * Odinstalowuje handlery sygnałów i ustawia ich obsługę na SIG_IGN, aby zignorować te sygnały.
 * Ustawia również g_pending_requests na NULL, aby zapobiec dalszemu przetwarzaniu.
 * Wywoływane podczas bezpiecznego zamykania procesu.
 */
void ds_signals_uninstall(void);

/*
 * Bezpiecznie odczytuje i resetuje zakolejkowane żądania sygnałów (pending_requests) z ds_runtime_state_t.
 
 * Zwraca maskę bitową zakolejkowanych żądań sygnałów (DS_REQ_*).
 */
int ds_signals_consume_requests(ds_runtime_state_t *state, int *out_last_signal);

#endif /* DEMONSEARCH_SIGNALS_H */
