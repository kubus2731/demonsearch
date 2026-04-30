#ifndef DEMONSEARCH_SUPERVISOR_H
#define DEMONSEARCH_SUPERVISOR_H

#include "proc_table.h"

/*
 * Uruchamia główną pętlę procesu nadzorczego (Supervisor).
 * Zarządza cyklem życia procesów potomnych (workerów) i działa
 * jako centralny punkt odbioru sygnałów, propagując je do workerów.
 * 
 * Zwraca 0 w przypadku pomyślnego zakończenia, 1 w przypadku błędu.
 */
int ds_supervisor_run(ds_proc_table_t *ptable);

#endif /* DEMONSEARCH_SUPERVISOR_H */