#include "scanner.h"

#include "logger.h"
#include "match.h"

#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* Sprawdza, czy skanowanie powinno zostać przerwane na skutek sygnału. */
static int should_abort(const volatile sig_atomic_t *abort_scan)
{
	return (abort_scan != NULL && *abort_scan != 0) ? 1 : 0;
}

/* Łączy ścieżkę katalogu z nazwą pliku. */
static char *join_path(const char *dir_path, const char *name)
{
	size_t dir_len;
	size_t name_len;
	int needs_slash;
	size_t total;
	char *full_path;

	if (dir_path == NULL || name == NULL) {
		return NULL;
	}

	dir_len = strlen(dir_path);
	name_len = strlen(name);
	needs_slash = (dir_len > 0 && dir_path[dir_len - 1] == '/') ? 0 : 1;
	total = dir_len + (size_t)needs_slash + name_len + 1;

	full_path = (char *)malloc(total);
	if (full_path == NULL) {
		return NULL;
	}

	if (needs_slash) {
		snprintf(full_path, total, "%s/%s", dir_path, name);
	} else {
		snprintf(full_path, total, "%s%s", dir_path, name);
	}

	return full_path;
}

/* Rekurencyjnie skanuje katalog, aktualizując statystyki i sprawdzając dopasowania. */
static int scan_dir_recursive(const char *component,
							  const char *dir_path,
							  const char *pattern,
							  volatile sig_atomic_t *abort_scan,
							  ds_scan_stats_t *stats)
{
	DIR *dir;
	struct dirent *entry;

    /* Sprawdzenie, czy skanowanie powinno zostać przerwane na skutek sygnału, przed rozpoczęciem operacji. */
	if (should_abort(abort_scan)) {
		return 1;
	}

    /* Weryfikacja uprawnień (R_OK oraz X_OK) do katalogu przed próbą otwarcia.
	   Jeśli brak uprawnień, zliczamy to w statystykach i pomijamy katalog. */
	if (access(dir_path, R_OK | X_OK) != 0) {
		if (errno == EACCES) {
			stats->skipped_perm++;
			return 0;
		}
		stats->errors++;
		return 0;
	}

	dir = opendir(dir_path);
	if (dir == NULL) {
		if (errno == EACCES) {
			stats->skipped_perm++;
			return 0;
		}
		stats->errors++;
		return 0;
	}

	stats->visited_dirs++;

    /* Iterowanie wszystkich elementów odnalezionych w folderze. */
	while ((entry = readdir(dir)) != NULL) {
		char *full_path;
		struct stat st;
		int is_dir;

        /* Cykliczna weryfikacja, czy skanowanie powinno zostać przerwane na skutek sygnału, aby umożliwić szybkie zakończenie. */
		if (should_abort(abort_scan)) {
			closedir(dir);
			return 1;
		}

        /* Pominięcie specjalnych wpisów "." i "..", które reprezentują katalog bieżący i nadrzędny. */
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		full_path = join_path(dir_path, entry->d_name);
		if (full_path == NULL) {
			stats->errors++;
			continue;
		}

		/* Pobranie informacji o pliku/katalogu za pomocą lstat, aby uniknąć podążania za dowiązaniami symbolicznymi.
	       Jeśli wystąpi błąd, zliczamy go w statystykach i pomijamy ten wpis. */
		if (lstat(full_path, &st) != 0) {
			if (errno == EACCES) {
				stats->skipped_perm++;
			} else {
				stats->errors++;
			}
			free(full_path);
			continue;
		}

		/* Weryfikacja praw odczytu pojedynczego wpisu. Jeśli brak uprawnień, zliczamy to w statystykach i pomijamy ten wpis. */
		if (access(full_path, R_OK) != 0) {
			if (errno == EACCES) {
				stats->skipped_perm++;
			} else {
				stats->errors++;
			}
			free(full_path);
			continue;
        }

		stats->visited_entries++;
		is_dir = S_ISDIR(st.st_mode) ? 1 : 0;

		int matched = ds_match_contains(entry->d_name, pattern);

		/* Logowanie wyników porównania w trybie verbose. */
		ds_log_verbose_compare(component, full_path, pattern, matched);

		if (matched) {
			stats->matches++;
			ds_log_match_found(component, full_path, pattern);
		}

		if (is_dir) {
			int rc = scan_dir_recursive(component, full_path, pattern, abort_scan, stats);
			if (rc != 0) {
				free(full_path);
				closedir(dir);
				return rc;
			}
		}

		free(full_path);
	}

	closedir(dir);
	return 0;
}

int ds_scanner_scan_tree(const char *component,
						 const char *root_path,
						 const char *pattern,
						 volatile sig_atomic_t *abort_scan,
						 ds_scan_stats_t *out_stats)
{
	ds_scan_stats_t tmp = {0, 0, 0, 0, 0};
	int rc;

	/* Walidacja argumentów wejściowych. */
	if (root_path == NULL || *root_path == '\0' || pattern == NULL || *pattern == '\0') {
		return -1;
	}

	rc = scan_dir_recursive(component, root_path, pattern, abort_scan, &tmp);

	if (out_stats != NULL) {
		*out_stats = tmp;
	}

	return rc;
}
