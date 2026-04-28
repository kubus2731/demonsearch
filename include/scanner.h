/* Modul: interfejs rekurencyjnego skanowania systemu plikow. */

#ifndef DEMONSEARCH_SCANNER_H
#define DEMONSEARCH_SCANNER_H

#include <signal.h>
#include <stddef.h>

/*
 * Struktura przechowująca statystyki skanowania.
*/
typedef struct ds_scan_stats {
	unsigned long visited_dirs;		/* Liczba odwiedzonych katalogów. */
	unsigned long visited_entries;	/* Liczba odwiedzonych wpisów. Uwzględnia zarówno pliki, jak i katalogi. */
	unsigned long matches;			/* Liczba znalezionych dopasowań. */
	unsigned long skipped_perm;		/* Liczba pominiętych wpisów z powodu braku uprawnień. */
	unsigned long errors;			/* Liczba błędów wejścia/odczytu. */
} ds_scan_stats_t;

/*
 * Rekurencyjnie skanuje drzewo od root_path.
 * @param root_path ścieżka katalogu, od którego zaczynamy skanowanie (nie może być NULL ani pusty)
 * @param pattern wzorzec do dopasowania nazw plików (nie może być NULL ani pusty)
 * @param abort_scan wskaźnik do zmiennej typu volatile sig_atomic_t, która może być ustawiona na true (niezerowa) z sygnału, aby przerwać skanowanie
 * @param out_stats opcjonalny wskaźnik do struktury ds_scan_stats_t, która zostanie wypełniona statystykami skanowania
 * 
 * Zwraca 0 w przypadku sukcesu, 1 jeśli skanowanie zostało przerwane (abort_scan), lub -1 przy błędzie.
 */
int ds_scanner_scan_tree(const char *root_path,
						 const char *pattern,
						 volatile sig_atomic_t *abort_scan,
						 ds_scan_stats_t *out_stats);

#endif /* DEMONSEARCH_SCANNER_H */
