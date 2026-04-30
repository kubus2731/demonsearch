#include "match.h"

#include <string.h>

bool ds_match_contains(const char *candidate, const char *pattern)
{
	/* Zabezpieczenie przed undefined behavior funkcji strstr oraz odrzucenie pustego wzorca. */
	if (candidate == NULL || pattern == NULL || *pattern == '\0') {
		return false;
	}

	return strstr(candidate, pattern) != NULL;
}
