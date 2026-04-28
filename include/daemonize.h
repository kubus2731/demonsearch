/* Modul: konfiguracja demona i odpiecie od terminala. */

#ifndef DEMONSEARCH_DAEMONIZE_H
#define DEMONSEARCH_DAEMONIZE_H

#include <sys/types.h>
#include <stdbool.h>

/*
 * Struktura konfiguracyjna dla procesu demonizacji.
 */
typedef struct {
    bool close_stdio;       /* Jeśli true, zamyka standardowe deskryptory I/O i przekierowuje je do /dev/null. */
    bool keep_cwd;          /* Jeśli false, zmienia katalog roboczy na '/' po demonizacji. */
    const char *pid_file;   /* Opcjonalna ścieżka do pliku PID. Jeśli NULL, nie będzie używany. */
} ds_daemon_config_t;

/*
 * Transformuje bieżący proces w demona.
 *
 * Zwraca 0 w przypadku sukcesu, lub -1 przy błędzie.
 * 
 * Parent zawsze kończy się z kodem EXIT_SUCCESS.
 */
int ds_daemonize(const ds_daemon_config_t *config);

#endif /* DEMONSEARCH_DAEMONIZE_H */