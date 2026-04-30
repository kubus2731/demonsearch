#ifndef DEMONSEARCH_STATE_H
#define DEMONSEARCH_STATE_H

#include <signal.h>
#include <stdbool.h>

/* Określa architektoniczną rolę procesu w systemie. */
typedef enum ds_role {
    DS_ROLE_SUPERVISOR = 0, /* Zarządza cyklem życia workerów i propaguje sygnały. */
    DS_ROLE_WORKER = 1,     /* Wykonuje właściwe skanowanie systemu plików. */
} ds_role_t;

/* Reprezentuje bieżący stan aktywności maszyny stanów. */
typedef enum ds_phase {
    DS_PHASE_SLEEPING = 0,  /* Oczekiwanie na upływ interwału czasowego lub sygnał wybudzający. */
    DS_PHASE_SCANNING = 1,  /* Aktywne, rekurencyjne przeszukiwanie drzewa katalogów. */
} ds_phase_t;

/* Flagi bitowe żądań asynchronicznych. Ustawiane w handlerach sygnałów, 
   konsumowane przez pętlę główną procesu. */
typedef enum ds_request_flags {
    DS_REQ_NONE = 0,            /* Brak oczekujących żądań. */
    DS_REQ_RESCAN = 1 << 0,     /* Natychmiastowy start/restart skanowania (SIGUSR1). */
    DS_REQ_ABORT_SCAN = 1 << 1, /* Przerwanie obecnego skanowania i powrót do uśpienia (SIGUSR2). */
    DS_REQ_TERMINATE = 1 << 2,  /* Bezpieczne, kontrolowane zamknięcie procesu (SIGINT/SIGTERM). */
} ds_request_flags_t;

/* Globalny kontekst uruchomieniowy procesu. 
   Przekazywany przez wskaźnik do modułów w celu uniknięcia zmiennych globalnych. */
typedef struct ds_runtime_state {
    ds_role_t role;
    ds_phase_t phase;
    bool verbose;
    volatile sig_atomic_t pending_requests; /* Flagi modyfikowane współbieżnie przez sygnały. */
} ds_runtime_state_t;

#endif /* DEMONSEARCH_STATE_H */