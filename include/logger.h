#ifndef DEMONSEARCH_LOGGER_H
#define DEMONSEARCH_LOGGER_H

#include <stdbool.h>
#include <sys/types.h>

/* Makro do oznaczania funkcji logujących, które przyjmują format printf.
   Pozwala kompilatorowi sprawdzać poprawność formatowania. */
#if defined(__GNUC__) || defined(__clang__)
#define DS_PRINTF_FORMAT(fmt_index, first_arg) __attribute__((format(printf, fmt_index, first_arg)))
#else
#define DS_PRINTF_FORMAT(fmt_index, first_arg)
#endif

/*
 * Inicjalizuje połączenie z syslogiem.
 */
void ds_logger_init(const char *ident, bool verbose);

/*
 * Bezpiecznie zamyka deskryptory i połączenie z syslogiem.
 * Powinno być wywoływane podczas zamykania procesu.
 */
void ds_logger_close(void);

/*
 * Rejestruje znalezienie dopasowania wzorca do pliku.
 */
void ds_log_match_found(const char* component, const char *full_path, const char *pattern);

/*
 * Loguje porównanie ze wzorcem, wraz z informacją, czy dopasowano (tylko w trybie verbose).
 */
void ds_log_verbose_compare(const char* component, const char *path, const char *pattern, int matched);

/*
 * Wrapper do logowania ogólnych zdarzeń informacyjnych.
 */
void ds_log_info(const char *fmt, ...) DS_PRINTF_FORMAT(1, 2);

/*
 * Wrapper do logowania ogólnych zdarzeń błędów.
 */
void ds_log_error(const char *fmt, ...) DS_PRINTF_FORMAT(1, 2);

/*
 * Wrapper do logowania zdarzeń informacyjnych w trybie verbose.
 */
void ds_log_verbose_info(const char *fmt, ...) DS_PRINTF_FORMAT(1, 2);

/*
 * Wrapper do logowania zdarzeń błędów w trybie verbose.
 */
void ds_log_verbose_error(const char *fmt, ...) DS_PRINTF_FORMAT(1, 2);

#endif /* DEMONSEARCH_LOGGER_H */
