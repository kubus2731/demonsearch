#ifndef DEMONSEARCH_ARGS_H
#define DEMONSEARCH_ARGS_H

#include <stdbool.h>
#include <stddef.h>

/* Domyślny czas uśpienia demona pomiędzy cyklami skanowania (w sekundach). */
#define DS_DEFAULT_INTERVAL_SEC 60

/*
 * Wynikowy kontekst konfiguracji po przetworzeniu argumentów CLI.
 * Reprezentuje zatwierdzone żądanie uruchomieniowe demona. 
 */
typedef struct {
    char **patterns;        /* Tablica szukanych wzorców. */
    size_t pattern_count;   /* Liczba wzorców w tablicy patterns. */
    unsigned interval_sec;  /* Interwał między skanami w sekundach. */
    bool verbose;           /* Jeśli true, program powinien działać w trybie verbose. */
    char *start_dir;        /* Opcjonalny katalog startowy. */
} ds_args_t;

/*
 * Zwalnia głęboko zaalokowane struktury (tablicę wzorców i ścieżkę) 
 * i przywraca obiekt do stanu zerowego.
 */
void ds_args_free(ds_args_t *args);

/*
 * Drukuje na standardowe wyjście sformatowany ekran pomocy programu.
 */
void ds_args_print_usage(const char *progname);

/*
 * Przetwarza argumenty z linii poleceń i wypełnia strukturę konfiguracyjną.
 
 * Zwraca 0 w przypadku sukcesu,
 * 1 jeśli zażądano wyświetlenia pomocy (-h/--help),
 * 2 w przypadku błędu walidacji (nieprawidłowe argumenty).
 */
int ds_args_parse(int argc, char **argv, ds_args_t *out, bool *out_show_usage);

#endif /* DEMONSEARCH_ARGS_H */