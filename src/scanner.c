/* Implementacja modulu: rekurencyjny skan z bezpiecznym traversalem. */

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

static int should_abort(const volatile sig_atomic_t *abort_scan)
{
	return (abort_scan != NULL && *abort_scan != 0) ? 1 : 0;
}

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

static int scan_dir_recursive(const char *dir_path,
							  const char *pattern,
							  volatile sig_atomic_t *abort_scan,
							  ds_scan_stats_t *stats)
{
	DIR *dir;
	struct dirent *entry;

	if (should_abort(abort_scan)) {
		return 1;
	}

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

	while ((entry = readdir(dir)) != NULL) {
		char *full_path;
		struct stat st;
		int is_dir;

		if (should_abort(abort_scan)) {
			closedir(dir);
			return 1;
		}

		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
			continue;
		}

		full_path = join_path(dir_path, entry->d_name);
		if (full_path == NULL) {
			stats->errors++;
			continue;
		}

		if (lstat(full_path, &st) != 0) {
			if (errno == EACCES) {
				stats->skipped_perm++;
			} else {
				stats->errors++;
			}
			free(full_path);
			continue;
		}

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
		ds_log_verbose_compare(full_path, pattern, matched);

		if (matched) {
			stats->matches++;
			ds_log_match_found(full_path, pattern);
		}

		if (is_dir) {
			int rc = scan_dir_recursive(full_path, pattern, abort_scan, stats);
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

int ds_scanner_scan_tree(const char *root_path,
						 const char *pattern,
						 volatile sig_atomic_t *abort_scan,
						 ds_scan_stats_t *out_stats)
{
	ds_scan_stats_t tmp = {0, 0, 0, 0, 0};
	int rc;

	if (root_path == NULL || *root_path == '\0' || pattern == NULL || *pattern == '\0') {
		return -1;
	}

	rc = scan_dir_recursive(root_path, pattern, abort_scan, &tmp);
	if (out_stats != NULL) {
		*out_stats = tmp;
	}

	return rc;
}
