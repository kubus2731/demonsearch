#ifndef DEMONSEARCH_DAEMONIZE_H
#define DEMONSEARCH_DAEMONIZE_H

#include <sys/types.h>
#include <stdbool.h>

/*
 * Struktura konfiguracyjna dla procesu demonizacji.
 * Umożliwia dostosowanie zachowania procesu podczas transformacji w demona.
 */
typedef struct {
    bool close_stdio;       /* Jeśli true, zamyka standardowe deskryptory I/O i przekierowuje je do /dev/null. */
    bool keep_cwd;          /* Jeśli false, zmienia katalog roboczy na '/' po demonizacji. */
    const char *pid_file;   /* Opcjonalna ścieżka do pliku PID. Jeśli NULL, nie będzie używany. */
} ds_daemon_config_t;

/*
 * Przekształca bieżący proces w niezależnego demona systemowego (wzorzec double-fork).
 * Zwraca 0 w przypadku sukcesu (proces jest demomen), -1 w przypadku błędu.
 * Proces rodzica kończy działanie natychmiast po demonizacji.
 */
int ds_daemonize(const ds_daemon_config_t *config);

#endif /* DEMONSEARCH_DAEMONIZE_H */