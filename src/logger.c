/* Implementacja modulu: API logowania do syslog w formacie key=value. */

#include "logger.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <time.h>

static int g_logger_open = 0;
static int g_logger_verbose = 0;
static char g_logger_ident[16] = "demonsearch";

/*
 * Escapuje wartość do bezpiecznego logowania w formacie key=value.
 * @param src: oryginalny string (może być NULL)
 * @param dst: bufor docelowy, gdzie zostanie zapisany escapowany string
 * @param dst_size: rozmiar bufora docelowego
 *
 * Zasady escapowania:
 * - Jeśli src jest NULL, traktujemy to jako "-" (niezdefiniowane).
 * - Znaki specjalne \ i " są poprzedzane backslashem.
 * - Znaki kontrolne (ASCII < 32 lub 127) są reprezentowane jako \xHH.
 * - Pozostałe znaki są kopiowane bez zmian.
 *
 * Po wywołaniu tej funkcji, dst zawiera escapowany string gotowy do logowania.
 */
static void ds_escape_value(const char *src, char *dst, size_t dst_size)
{
    size_t i = 0;
    size_t j = 0;
    const char *in = (src != NULL) ? src : "-";

    if (dst_size == 0) {
        return;
    }

    while (in[i] != '\0' && j + 1 < dst_size) {
        unsigned char c = (unsigned char)in[i++];

        if (c == '\\' || c == '"') {
            if (j + 2 >= dst_size) {
                break;
            }
            dst[j++] = '\\';
            dst[j++] = (char)c;
            continue;
        }

        if (c < 32 || c == 127) {
            if (j + 4 >= dst_size) {
                break;
            }
            dst[j++] = '\\';
            dst[j++] = 'x';
            dst[j++] = "0123456789ABCDEF"[(c >> 4) & 0x0F];
            dst[j++] = "0123456789ABCDEF"[c & 0x0F];
            continue;
        }

        dst[j++] = (char)c;
    }

    dst[j] = '\0';
}

/*
 * Wewnętrzny wrapper na vsnprintf i syslog z obsługą opóźnionego openlog().
 */
static void ds_vlog_with_priority(int priority, const char *fmt, va_list ap)
{
    char buffer[4096];
    if (fmt == NULL) return;

    if (!g_logger_open) {
        openlog(g_logger_ident, LOG_PID | LOG_NDELAY, LOG_DAEMON);
        g_logger_open = 1;
    }

    vsnprintf(buffer, sizeof(buffer), fmt, ap);
    syslog(priority, "%s", buffer);
}

/*
 * Wariadyczna funkcja logująca z określonym priorytetem.
 */
static void ds_log_with_priority(int priority, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    ds_vlog_with_priority(priority, fmt, ap);
    va_end(ap);
}

static const char *wakeup_reason_to_str(ds_wakeup_reason_t reason)
{
    switch (reason) {
    case DS_WAKEUP_INTERVAL:
        return "interval";
    case DS_WAKEUP_SIGUSR1:
        return "SIGUSR1";
    case DS_WAKEUP_SIGUSR2:
        return "SIGUSR2";
    case DS_WAKEUP_OTHER:
    default:
        return "other";
    }
}

void ds_logger_init(const char *ident, bool verbose)
{
    if (ident != NULL && *ident != '\0') {
        strncpy(g_logger_ident, ident, sizeof(g_logger_ident) - 1);
        g_logger_ident[sizeof(g_logger_ident) - 1] = '\0';
    }
    if (g_logger_open) {
        closelog();
        g_logger_open = 0;
    }
    openlog(g_logger_ident, LOG_PID | LOG_NDELAY, LOG_DAEMON);
    g_logger_open = 1;
    g_logger_verbose = verbose ? 1 : 0;
}

void ds_logger_close(void)
{
    if (g_logger_open) {
        closelog();
        g_logger_open = 0;
    }
}

void ds_log_match_found(const char *full_path, const char *pattern)
{
    char dt[32];
    char escaped_path[2048];
    char escaped_pattern[1024];
    time_t now = time(NULL);
    struct tm tm_buf;
    
    if (localtime_r(&now, &tm_buf) == NULL || strftime(dt, sizeof(dt), "%Y-%m-%dT%H:%M:%S%z", &tm_buf) == 0) {
        snprintf(dt, sizeof(dt), "1970-01-01T00:00:00+0000");
    }

    ds_escape_value(full_path, escaped_path, sizeof(escaped_path));
    ds_escape_value(pattern, escaped_pattern, sizeof(escaped_pattern));

    ds_log_with_priority(LOG_INFO,
        "event=match timestamp=%s pattern=\"%s\" path=\"%s\"",
        dt,
        escaped_pattern,
        escaped_path);
}

void ds_log_verbose_sleep(unsigned interval_sec)
{
    if (g_logger_verbose) {
        ds_log_with_priority(LOG_DEBUG,
            "event=sleep interval_sec=%u",
            interval_sec);
    }
}

void ds_log_verbose_wakeup(ds_wakeup_reason_t reason)
{
    if (g_logger_verbose) {
        ds_log_with_priority(LOG_DEBUG,
            "event=wakeup reason=%s",
            wakeup_reason_to_str(reason));
    }
}

void ds_log_verbose_signal_received(int signo)
{
    if (g_logger_verbose) {
        ds_log_with_priority(LOG_DEBUG, 
            "event=signal_received signal=%d", 
            signo);
    }
}

void ds_log_verbose_compare(const char *path, const char *pattern, int matched)
{
    if (g_logger_verbose) {
        char escaped_path[2048];
        char escaped_pattern[1024];
        ds_escape_value(path, escaped_path, sizeof(escaped_path));
        ds_escape_value(pattern, escaped_pattern, sizeof(escaped_pattern));
        ds_log_with_priority(LOG_DEBUG, 
            "event=compare path=\"%s\" pattern=\"%s\" matched=%s", 
            escaped_path,
            escaped_pattern,
            matched ? "true" : "false");
    }
}

void ds_log_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    ds_vlog_with_priority(LOG_INFO, fmt, ap);
    va_end(ap);
}

void ds_log_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    ds_vlog_with_priority(LOG_ERR, fmt, ap);
    va_end(ap);
}

void ds_log_verbose_info(const char *fmt, ...)
{
    if (!g_logger_verbose) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    ds_vlog_with_priority(LOG_INFO, fmt, ap);
    va_end(ap);
}

void ds_log_verbose_error(const char *fmt, ...)
{
    if (!g_logger_verbose) {
        return;
    }

    va_list ap;
    va_start(ap, fmt);
    ds_vlog_with_priority(LOG_ERR, fmt, ap);
    va_end(ap);
}

void ds_log_process_role(pid_t pid, const char *role_name)
{
    char escaped_role[256];
    ds_escape_value(role_name, escaped_role, sizeof(escaped_role));
    ds_log_with_priority(LOG_INFO, 
        "event=process_role pid=%d role=\"%s\"", 
        (int)pid, 
        escaped_role);
}