#ifndef DEMONSEARCH_PROC_TABLE_H
#define DEMONSEARCH_PROC_TABLE_H

#include <sys/types.h>
#include <stddef.h>

/*
 * Struktura przechowująca informacje o aktywnych procesach potomnych (workerach).
 * Pozwala procesowi nadzorczemu na propagację sygnałów i zarządzanie cyklem życia.
 */
typedef struct {
    pid_t *pids;            /* Tablica zaalokowana dynamicznie przechowująca PID-y (wartość 0 oznacza wolne miejsce). */
    size_t capacity;        /* Maksymalna liczba procesów (rozmiar tablicy). */
    size_t active_count;    /* Bieżąca liczba działających procesów. */
} ds_proc_table_t;

/*
 * Inicjalizuje tablicę procesów potomnych.
 * Zwraca 0 w przypadku sukcesu, -1 w przypadku błędu alokacji pamięci.
 */
int ds_ptable_init(ds_proc_table_t *table, size_t capacity);

/*
 * Dodaje PID procesu potomnego do tablicy.
 */
void ds_ptable_add(ds_proc_table_t *table, pid_t pid);

/*
 * Usuwa PID procesu potomnego z tablicy.
 */
void ds_ptable_remove(ds_proc_table_t *table, pid_t pid);

/*
 * Wysyła sygnał do wszystkich aktywnych procesów potomnych.
 */
void ds_ptable_signal_all(ds_proc_table_t *table, int signo);

/*
 * Zawiesza wykonanie procesu aż do zakończenia wszystkich procesów potomnych.
 */
void ds_ptable_wait_all(ds_proc_table_t *table);

/*
 * Zwalnia zaalokowaną pamięć i przywraca strukturę do stanu zerowego.
 */
void ds_ptable_free(ds_proc_table_t *table);

#endif /* DEMONSEARCH_PROC_TABLE_H */