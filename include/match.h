#ifndef DEMONSEARCH_MATCH_H
#define DEMONSEARCH_MATCH_H

#include <stdbool.h>
#include <stddef.h>

/*
 * Sprawdza, czy ciąg znaków zawiera dany jako podciąg.
 
 * Zwraca true jeśli candidate zawiera pattern jako podciąg,
 * false w przypadku braku dopasowania lub błędnych parametrów.
 */
bool ds_match_contains(const char *candidate, const char *pattern);

#endif /* DEMONSEARCH_MATCH_H */
