/* Modul: pomocnicze funkcje dopasowania fragmentu nazwy pliku. */

#ifndef DEMONSEARCH_MATCH_H
#define DEMONSEARCH_MATCH_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Porównuje ciąg znaków candidate z pattern.
 * 
 * Zwraca true, jeśli candidate zawiera pattern jako podciąg, false w przeciwnym razie.
 * Jeśli candidate lub pattern jest NULL, lub pattern jest pusty, zwraca false.
 */
bool ds_match_contains(const char *candidate, const char *pattern);

#endif /* DEMONSEARCH_MATCH_H */
