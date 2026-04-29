#ifndef DEMONSEARCH_SCANNER_H
#define DEMONSEARCH_SCANNER_H

#include <signal.h>
#include <stddef.h>

/*
 * Struktura przechowująca statystyki skanowania.
 */
typedef struct ds_scan_stats {
	unsigned long visited_dirs;		/* Liczba odwiedzonych katalogów. */
	unsigned long visited_entries;	/* Liczba odwiedzonych wpisów (plików i katalogów). */
	unsigned long matches;			/* Liczba znalezionych dopasowań wzorca. */
	unsigned long skipped_perm;		/* Liczba pominiętych wpisów z powodu braku uprawnień. */
	unsigned long errors;			/* Liczba błędów wejścia/odczytu (I/O). */
} ds_scan_stats_t;

/*
 * Struktura callbacków dla skanera.
 * Pozwala na rejestrowanie zdarzeń porównywania i dopasowania wzorców.
 */
typedef struct {
    void (*on_compare)(const char *path, const char *pattern, int matched);
    void (*on_match)(const char *path, const char *pattern);
} ds_scanner_callbacks_t;

/*
 * Rekurencyjnie skanuje drzewo od root_path w poszukiwaniu dopasowań pattern.
 
 * Zwraca 0 w przypadku pomyślnego zakończenia skanowania,
 * 1 jeśli wymuszono przerwanie skanowania sygnałem,
 * -1 przy błędnych parametrach.
 * -2 Brak jakiegokolwiek dostępu do katalogu startowego (lub folder nie istnieje).
 */
int ds_scanner_scan_tree(const char *root_path,
						 const char *pattern,
						 volatile sig_atomic_t *abort_scan,
						 ds_scan_stats_t *out_stats,
                         const ds_scanner_callbacks_t *callbacks);

#endif /* DEMONSEARCH_SCANNER_H */
