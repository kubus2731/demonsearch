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

bool ds_match_any(const char *candidate, const char **patterns, size_t pattern_count)
{
	size_t i;

	if (candidate == NULL || patterns == NULL || pattern_count == 0) {
		return false;
	}

	for (i = 0; i < pattern_count; i++) {
		if (ds_match_contains(candidate, patterns[i])) {
			return true;
		}
	}

	return false;
}
