/* Implementacja modulu: parsowanie CLI do konfiguracji runtime. */

#include "args.h"
#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Parsuje string do unsigned, zwracając 0 w przypadku sukcesu, lub -1 przy błędzie.
 */
static int parse_uint(const char *s, unsigned *out)
{
    char *end = NULL;
    unsigned long v;

    if (s == NULL || *s == '\0' || *s == '-') {
        return -1;
    }

    errno = 0;

    v = strtoul(s, &end, 10);

    if (errno != 0 || end == s || *end != '\0') {
        return -1;
    }
     
    if (v == 0 || v > UINT_MAX) {
        return -1;
    }

    *out = (unsigned)v;
    return 0;
}

void ds_args_free(ds_args_t *args)
{
    size_t i;

    if (args == NULL) {
        return;
    }

    if (args->patterns != NULL) {
        for (i = 0; i < args->pattern_count; i++) {
            free(args->patterns[i]);
        }
        free(args->patterns);
    }

    if (args->start_dir != NULL) {
        free(args->start_dir);
    }

    args->patterns = NULL;
    args->start_dir = NULL;
    args->pattern_count = 0;
    args->interval_sec = 0;
    args->verbose = false;
}

void ds_args_print_usage(const char *progname)
{
    const char *p = (progname && *progname) ? progname : "demonsearch";

    fprintf(stdout,
        "Usage: %s [-v|--verbose] [-t|--time seconds] [-d|--dir path] [--help] <pattern1> [pattern2 ...]\n"
        "\n"
        "Options:\n"
        "  -t, --time, -i, --interval <sec>  Set sleep interval between scans (default %d)\n"
        "  -d, --dir <path>                  Set starting directory (default: root '/')\n"
        "  -v, --verbose                     Start in verbose mode (logs daemon events)\n"
        "  -h, --help                        Show this help message and exit\n",
        p, DS_DEFAULT_INTERVAL_SEC);
}

int ds_args_parse(int argc, char **argv, ds_args_t *out, bool *out_show_usage)
{
    int opt;
    ds_args_t tmp;
    bool show_usage = false;
    unsigned interval = DS_DEFAULT_INTERVAL_SEC;
    bool verbose = false;
    char *dir = NULL;
    int i;
    
    /* Definicja długich opcji dla getopt_long */
    static const struct option long_opts[] = {
        {"help",     no_argument,       NULL, 'h'},
        {"verbose",  no_argument,       NULL, 'v'},
        {"time",     required_argument, NULL, 't'},
        {"interval", required_argument, NULL, 'i'},
        {"dir",      required_argument, NULL, 'd'},
        {0, 0, 0, 0},
    };

    if (out_show_usage) *out_show_usage = false;
    if (out == NULL || argv == NULL || argc < 1) return 2;

    memset(&tmp, 0, sizeof(tmp));
    opterr = 0;
    optind = 1; /* Reset globalnego stanu dla getopt */

    /*  Przetwarzanie flag konfiguracyjnych za pomocą getopt_long 
        Znak ':' po literze oznacza, że opcja wymaga argumentu. */
    while ((opt = getopt_long(argc, argv, "hvt:i:d:", long_opts, NULL)) != -1) {
        switch (opt) {
        case 'h':
            show_usage = true;
            break;
        case 'v':
            verbose = true;
            break;
        case 'd':
            dir = strdup(optarg);
            if (!dir) {
                perror("Error: strdup failed");
                return 2;
            }
            break;
        case 't':
        case 'i':
            if (parse_uint(optarg, &interval) != 0) {
                fprintf(stderr, "Error: invalid value for interval/time: '%s'. Must be a positive integer.\n", optarg ? optarg : "");
                fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
                free(dir);
                return 2;
            }
            break;
        case '?':
        default:
            fprintf(stderr, "Error: unrecognized option or missing argument.\n");
            fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
            free(dir);
            return 2;
        }
    }

    if (show_usage) {
        if (out_show_usage) *out_show_usage = true;
        free(dir);
        return 1;
    }

    if (optind >= argc) {
        fprintf(stderr, "Error: at least one search pattern must be provided.\n");
        fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
        free(dir);
        return 2;
    }

    tmp.interval_sec = interval;
    tmp.verbose = verbose;
    tmp.start_dir = dir;
    tmp.pattern_count = (size_t)(argc - optind); /* Liczba wzorców to pozostałe argumenty po opcjach. */
    tmp.patterns = (char **)calloc(tmp.pattern_count, sizeof(char *));
    
    if (tmp.patterns == NULL) {
        perror("Error: calloc failed");
        free(dir);
        return 2;
    }

    for (i = 0; i < (int)tmp.pattern_count; i++) {
        const char *src = argv[optind + i];
        if (src == NULL || *src == '\0') {
            fprintf(stderr, "Error: pattern at position %d cannot be empty.\n", i + 1);
            ds_args_free(&tmp);
            return 2;
        }

        tmp.patterns[i] = strdup(src);
        if (tmp.patterns[i] == NULL) {
            perror("Error: strdup failed for pattern");
            ds_args_free(&tmp);
            return 2;
        }
    }

    *out = tmp;
    return 0;
}