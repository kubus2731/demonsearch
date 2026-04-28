/* Implementacja modulu: sprawdzenie dopasowania sciezki/nazwy. */

#include "match.h"

#include <string.h>

bool ds_match_contains(const char *candidate, const char *pattern)
{
	if (candidate == NULL || pattern == NULL || *pattern == '\0') {
		return false;
	}

	return strstr(candidate, pattern) != NULL;
}
