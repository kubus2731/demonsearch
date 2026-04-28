/* Modul: wspolne enumy, stany i stale runtime. */

#ifndef DEMONSEARCH_STATE_H
#define DEMONSEARCH_STATE_H

#include <signal.h> /* sig_atomic_t */
#include <stdbool.h>

/*
 * Rola procesu w architekturze supervisor-worker.
 */
typedef enum ds_role {
	DS_ROLE_SUPERVISOR = 0,	/* Proces nadzorczy, który zarządza workerami i odbiera sygnały. */
	DS_ROLE_WORKER = 1,		/* Proces roboczy, który wykonuje skanowanie i raportuje wyniki. */
} ds_role_t;

/*
 * Faza działania procesu.
 */
typedef enum ds_phase {
	DS_PHASE_SLEEPING = 0,	/* Proces jest uśpiony, czeka na interwał lub sygnał. */
	DS_PHASE_SCANNING = 1,	/* Proces wykonuje rekurencyjne skanowanie. */
} ds_phase_t;

/*
 * Zadania/zdarzenia asynchroniczne ustawiane przez sygnaly.
 */
typedef enum ds_request_flags {
	DS_REQ_NONE = 0,			/* Brak żądań. */
	DS_REQ_RESCAN = 1 << 0,		/* Żądanie natychmiastowego ponownego skanowania (np. SIGUSR1) */
	DS_REQ_ABORT_SCAN = 1 << 1,	/* Żądanie przerwania aktualnego skanowania (np. SIGUSR2) */
	DS_REQ_TERMINATE = 1 << 2,	/* Żądanie zakończenia procesu (np. SIGINT/SIGTERM) */
} ds_request_flags_t;

/*
 * Struktura przechowująca aktualny stan runtime procesu.
 */
typedef struct ds_runtime_state {
	ds_role_t role;
	ds_phase_t phase;
	bool verbose;
	volatile sig_atomic_t pending_requests;
} ds_runtime_state_t;

#endif /* DEMONSEARCH_STATE_H */
