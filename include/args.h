/* Modul: parsowanie argumentow i definicje konfiguracji runtime. */

#ifndef DEMONSEARCH_ARGS_H
#define DEMONSEARCH_ARGS_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Domyślny interwał między skanami (w sekundach).
 */
#define DS_DEFAULT_INTERVAL_SEC 60

/*
 * Struktura przechowująca skonfigurowane opcje uruchomienia programu.
 * Reprezentuje prawidłowy stan konfiguracji po pomyślnym wywołaniu ds_args_parse().
 */
typedef struct {
    char **patterns;        /* Tablica szukanych wzorców. */
    size_t pattern_count;   /* Liczba wzorców w tablicy patterns. */
    unsigned interval_sec;  /* Interwał między skanami w sekundach. */
    bool verbose;           /* Jeśli true, program powinien działać w trybie verbose. */
    char *start_dir;        /* Opcjonalny katalog startowy. */
} ds_args_t;

/*
 * Zwalnia zasoby związane z ds_args_t.
 *
 * Po wywołaniu tej funkcji, struktura args jest zresetowana do stanu początkowego.
 */
void ds_args_free(ds_args_t *args);

/*
 * Drukuje na stdout informację o sposobie użycia programu.
 *
 * Przyjmuje opcjonalną nazwę programu, która będzie użyta w komunikacie (domyślnie "demonsearch").
 */
void ds_args_print_usage(const char *progname);

/*
 * Parsuje argumenty z linii poleceń i wypełnia strukturę ds_args_t.
 * @param argc, argv standardowe argumenty z main()
 * @param out wskaźnik do struktury, która zostanie wypełniona danymi z argumentów (nie może być NULL)
 * @param out_show_usage opcjonalny wskaźnik do bool ustawianego na true, jeśli zażądano wyświetlenie pomocy
 * 
 * Zwraca 0 w przypadku sukcesu, 1 jeśli zażądano wyświetlenie pomocy, 2 w przypadku błędu (nieprawidłowe argumenty).
 * 
 * W przypadku błędu, funkcja wypisuje odpowiedni komunikat na stderr i sugeruje użycie --help.
 */
int ds_args_parse(int argc, char **argv, ds_args_t *out, bool *out_show_usage);

#endif /* DEMONSEARCH_ARGS_H */