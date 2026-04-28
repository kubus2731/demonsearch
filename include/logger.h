/* Modul: API logowania do syslog (wyniki i zdarzenia verbose). */

#ifndef DEMONSEARCH_LOGGER_H
#define DEMONSEARCH_LOGGER_H

#include <stdbool.h>
#include <sys/types.h>

#if defined(__GNUC__) || defined(__clang__)
#define DS_PRINTF_FORMAT(fmt_index, first_arg) __attribute__((format(printf, fmt_index, first_arg)))
#else
#define DS_PRINTF_FORMAT(fmt_index, first_arg)
#endif

/*
 * Powód wybudzenia procesu z trybu uśpienia.
 */
typedef enum ds_wakeup_reason {
	DS_WAKEUP_INTERVAL = 0,	/* Wybudzenie po upływie interwału. */
	DS_WAKEUP_SIGUSR1 = 1,	/* Wybudzenie przez sygnał SIGUSR1. */
	DS_WAKEUP_SIGUSR2 = 2,	/* Wybudzenie przez sygnał SIGUSR2. */
	DS_WAKEUP_OTHER = 3,	/* Wybudzenie przez inny sygnał lub zdarzenie. */
} ds_wakeup_reason_t;

/*
 * Inicjalizuje logger.
 * @param ident prefiks dla syslog, jeśli NULL lub pusty, użyje "demonsearch"
 * @param verbose czy włączyć logowanie zdarzeń verbose
 */
void ds_logger_init(const char *ident, bool verbose);

/*
 * Zamyka logger.
 *
 * Po wywołaniu tej funkcji, logger jest zamknięty i nie można logować, dopóki ponownie nie zostanie zainicjalizowany.
 */
void ds_logger_close(void);

/*
 * Loguje znalezienie dopasowania.
 * @param full_path pełna ścieżka do dopasowanego pliku (nie może być NULL)
 * @param pattern wzorzec, który został dopasowany (nie może być NULL)
 */
void ds_log_match_found(const char *full_path, const char *pattern);

/*
 * Loguje usypianie procesu wraz z interwałem (tylko w trybie verbose).
 */
void ds_log_verbose_sleep(unsigned interval_sec);

/*
 * Loguje wybudzenie procesu wraz z powodem (tylko w trybie verbose).
 */
void ds_log_verbose_wakeup(ds_wakeup_reason_t reason);

/*
 * Loguje odebranie sygnału (tylko w trybie verbose).
*/
void ds_log_verbose_signal_received(int signo);

/*
 * Loguje porównanie ze wzorcem, wraz z informacją, czy dopasowano (tylko w trybie verbose).
 */
void ds_log_verbose_compare(const char *path, const char *pattern, int matched);

/*
 * Loguje informację.
 */
void ds_log_info(const char *fmt, ...) DS_PRINTF_FORMAT(1, 2);

/*
 * Loguje błąd.
 */
void ds_log_error(const char *fmt, ...) DS_PRINTF_FORMAT(1, 2);

/*
 * Loguje informację (tylko w trybie verbose).
 */
void ds_log_verbose_info(const char *fmt, ...) DS_PRINTF_FORMAT(1, 2);

/*
 * Loguje błąd (tylko w trybie verbose).
 */
void ds_log_verbose_error(const char *fmt, ...) DS_PRINTF_FORMAT(1, 2);

/* 
 * Przekazuje PID i role procesu do logu.
 */
void ds_log_process_role(pid_t pid, const char *role_name);

#endif /* DEMONSEARCH_LOGGER_H */
