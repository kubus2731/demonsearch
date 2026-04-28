/* Modul: interfejs wykonania procesu roboczego. */

#ifndef DEMONSEARCH_WORKER_H
#define DEMONSEARCH_WORKER_H

#include <stdbool.h>

/* 
 * Uruchamia główną pętlę procesu roboczego, który wykonuje skanowanie i raportuje wyniki.
 * @param start_dir katalog startowy do skanowania (nie może być NULL ani pusty)
 * @param pattern wzorzec do dopasowania (nie może być NULL ani pusty)
 * @param interval_sec interwał między skanami w sekundach (nie może być 0)
 * @param verbose czy włączyć tryb verbose (logowanie zdarzeń)
 * 
 * Zwraca 0 w przypadku sukcesu, lub -1 przy błędzie.
 */
int ds_worker_run(const char *start_dir, const char *pattern, unsigned interval_sec, bool verbose);

#endif /* DEMONSEARCH_WORKER_H */