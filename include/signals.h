/* Modul: konfiguracja obslugi sygnalow i wspolne flagi. */

#ifndef DEMONSEARCH_SIGNALS_H
#define DEMONSEARCH_SIGNALS_H

#include "state.h"

/*
 * Instaluje handlery dla sygnałów i ustawia odpowiednie flagi w state->pending_requests.
 * @param state wskaźnik do globalnego stanu runtime (nie może być NULL)
 * 
 * Zwraca 0 w przypadku sukcesu, lub -1 przy błędzie.
 */
int ds_signals_install(ds_runtime_state_t *state);

/*
 * Odinstalowuje handlery sygnałów, przywracając domyślne zachowanie.
 */
void ds_signals_uninstall(void);

/*
 * Sprawdza i konsumuje pending_requests z state, zwracając aktualne żądania i ostatni odebrany sygnał.
 * @param state wskaźnik do globalnego stanu runtime (nie może być NULL)
 * @param out_last_signal opcjonalny wskaźnik do int, który zostanie ustawiony na numer ostatniego odebranego sygnału, lub 0 jeśli nie było sygnału
 * 
 * Zwraca aktualne żądania (bitmaskę DS_REQ_*) i resetuje pending_requests do DS_REQ_NONE.
 * Jeśli state jest NULL, zwraca DS_REQ_NONE.
 */
int ds_signals_consume_requests(ds_runtime_state_t *state, int *out_last_signal);

#endif /* DEMONSEARCH_SIGNALS_H */
