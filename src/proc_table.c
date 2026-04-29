#include "proc_table.h"
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>

int ds_ptable_init(ds_proc_table_t *table, size_t capacity)
{
    if (!table || capacity == 0) return -1;
    
    /* Alokacja tablicy PID-ów i inicjalizacja pól struktury.
       Puste miejsca oznaczone są wartością 0. */
    table->pids = (pid_t *)calloc(capacity, sizeof(pid_t));
    if (!table->pids) return -1;
    
    table->capacity = capacity;
    table->active_count = 0;
    return 0;
}

void ds_ptable_add(ds_proc_table_t *table, pid_t pid)
{
    if (!table || !table->pids || table->active_count >= table->capacity) return;
    
    /* Liniowe przeszukiwanie tablicy w poszukiwaniu
       pierwszego wolnego miejsca (0) i dodanie PID-u. */
    for (size_t i = 0; i < table->capacity; i++) {
        if (table->pids[i] == 0) {
            table->pids[i] = pid;
            table->active_count++;
            return;
        }
    }
}

void ds_ptable_remove(ds_proc_table_t *table, pid_t pid)
{
    if (!table || !table->pids) return;
    
    for (size_t i = 0; i < table->capacity; i++) {
        if (table->pids[i] == pid) {
            table->pids[i] = 0;
            if (table->active_count > 0) table->active_count--;
            return;
        }
    }
}

void ds_ptable_signal_all(ds_proc_table_t *table, int signo)
{
    if (!table || !table->pids) return;
    
    /* Wysyłanie sygnału do wszystkich aktywnych procesów. */
    for (size_t i = 0; i < table->capacity; i++) {
        if (table->pids[i] > 0) {
            kill(table->pids[i], signo);
        }
    }
}

void ds_ptable_wait_all(ds_proc_table_t *table)
{
    if (!table || !table->pids) return;
    
    /* Zawieszenie wykonania procesu aż do zakończenia wszystkich procesów potomnych. */
    for (size_t i = 0; i < table->capacity; i++) {
        if (table->pids[i] > 0) {
            waitpid(table->pids[i], NULL, 0);
            table->pids[i] = 0;
        }
    }
    table->active_count = 0;
}

void ds_ptable_free(ds_proc_table_t *table)
{
    if (!table) return;
    if (table->pids) free(table->pids);
    table->pids = NULL;
    table->capacity = 0;
    table->active_count = 0;
}