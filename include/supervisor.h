/* Modul: Główna pętla procesu nadzorczego (Supervisor). */

#ifndef DEMONSEARCH_SUPERVISOR_H
#define DEMONSEARCH_SUPERVISOR_H

#include "proc_table.h"

/*
 * Uruchamia główną pętlę procesu nadzorczego, który zarządza procesami potomnymi i odbiera sygnały.
 */
int ds_supervisor_run(ds_proc_table_t *ptable);

#endif /* DEMONSEARCH_SUPERVISOR_H */