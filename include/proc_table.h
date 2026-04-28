/* Modul: tabela procesow potomnych (PID i cykl zycia). */

#ifndef DEMONSEARCH_PROC_TABLE_H
#define DEMONSEARCH_PROC_TABLE_H

#include <sys/types.h>
#include <stddef.h>

/*
 * Struktura przechowująca informacje o aktywnych procesach potomnych.
 */
typedef struct {
    pid_t *pids;            /* Tablica PID-ów procesów potomnych. */
    size_t capacity;        /* Maksymalna liczba procesów, które można śledzić. */
    size_t active_count;    /* Liczba aktywnych procesów w tablicy. Nie musi być równa liczbie niezerowych PID-ów, ale nie może być większa. */
} ds_proc_table_t;

/*
 * Inicjalizuje tablicę procesów potomnych.
 * @param table wskaźnik do struktury, która zostanie zainicjalizowana (nie może być NULL)
 * @param capacity maksymalna liczba procesów, które można śledzić (nie może być 0)
 * 
 * Zwraca 0 w przypadku sukcesu, lub -1 przy błędzie.
 */
int ds_ptable_init(ds_proc_table_t *table, size_t capacity);

/*
 * Dodaje PID procesu potomnego do tablicy.
 */
void ds_ptable_add(ds_proc_table_t *table, pid_t pid);

/*
 * Usuwa PID procesu potomnego z tablicy (np. po zakończeniu).
 */
void ds_ptable_remove(ds_proc_table_t *table, pid_t pid);

/*
 * Wysyła sygnał do wszystkich aktywnych procesów potomnych.
 * @param signo numer sygnału do wysłania (np. SIGTERM)
 */
void ds_ptable_signal_all(ds_proc_table_t *table, int signo);

/*
 * Czeka na zakończenie wszystkich aktywnych procesów potomnych i czyści ich PID-y z tablicy.
 */
void ds_ptable_wait_all(ds_proc_table_t *table);

/*
 * Zwalnia zasoby związane z tablicą procesów potomnych.
 *
 * Po wywołaniu tej funkcji, struktura table jest zresetowana do stanu początkowego.
 */
void ds_ptable_free(ds_proc_table_t *table);

#endif /* DEMONSEARCH_PROC_TABLE_H */