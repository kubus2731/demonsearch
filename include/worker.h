#ifndef DEMONSEARCH_WORKER_H
#define DEMONSEARCH_WORKER_H

#include <stdbool.h>

/*
 * Uruchamia główną pętlę procesu roboczego, który zarządza maszyną stanów,
 * wykonuje skanowanie systemu plików i raportuje wyniki.
 * 
 * Zwraca EXIT_SUCCESS w przypadku normalnego zakończenia,
 * EXIT_FAILURE w przypadku błędu.
 */
int ds_worker_run(const char *start_dir, const char *pattern, unsigned interval_sec, bool verbose);

#endif /* DEMONSEARCH_WORKER_H */