#include "logger.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <time.h>

/* Zmienne globalne do zarządzania logowaniem. */
static int g_logger_open = 0;
static int g_logger_verbose = 0;
static char g_logger_ident[16] = "demonsearch";

/* Sanitazuje łańcuchy znaków do bezpiecznego logowania w formacie key=value, 
   unikając problemów z cytowaniem i nieczytelnymi znakami. */
static void ds_escape_value(const char *src, char *dst, size_t dst_size)
{
    size_t i = 0, j = 0;
    const char *in = (src != NULL) ? src : "-";

    if (dst_size == 0) return;

    while (in[i] != '\0' && j + 1 < dst_size) {
        unsigned char c = (unsigned char)in[i++];

        /* Zwykłe znaki ASCII są kopiowane bez zmian, z wyjątkiem cudzysłowów i backslashy. */
        if (c >= 32 && c <= 126 && c != '"' && c != '\\') {
            dst[j++] = (char)c;
            continue;
        }

        /* Sprawdzanie, czy jest wystarczająco dużo miejsca na zapisanie sekwencji ucieczki. */
        if (j + 2 >= dst_size) break;

        switch (c) {
            case '"': dst[j++] = '\\'; dst[j++] = '"'; break;
            case '\\': dst[j++] = '\\'; dst[j++] = '\\'; break;
            case '\n': dst[j++] = '\\'; dst[j++] = 'n'; break;
            case '\r': dst[j++] = '\\'; dst[j++] = 'r'; break;
            case '\t': dst[j++] = '\\'; dst[j++] = 't'; break;
            default:
                /* Nieczytelne znaki są reprezentowane jako \xHH. */
                if (j + 4 >= dst_size) {
                    /* Nie ma wystarczająco dużo miejsca na pełną sekwencję ucieczki, więc kończymy tutaj. */
                    dst[j] = '\0';
                    return;
                }
                snprintf(&dst[j], dst_size - j, "\\x%02x", c);
                j += 4;
                break;
        }
    }

    dst[j] = '\0';
}

/* Wewnętrzny adapter dla funkcji vsnprintf i syslog.
   Otwiera strumień logów jeśli jeszcze nie jest otwarty, formatuje wiadomość i wysyła ją do syslog. */
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

/* Wariadyczna funkcja logująca z określonym priorytetem. */
static void ds_log_with_priority(int priority, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    ds_vlog_with_priority(priority, fmt, ap);
    va_end(ap);
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

void ds_log_match_found(const char* component, const char *full_path, const char *pattern)
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

    if (component == NULL || *component == '\0') {
        ds_log_with_priority(LOG_INFO,
            "event=match timestamp=%s pattern=\"%s\" path=\"%s\"",
            dt, escaped_pattern, escaped_path);
    } else {
        ds_log_with_priority(LOG_INFO,
            "component=\"%s\" event=match timestamp=%s pattern=\"%s\" path=\"%s\"",
            component, dt, escaped_pattern, escaped_path);
    }
}

void ds_log_verbose_compare(const char* component, const char *path, const char *pattern, int matched)
{
    if (g_logger_verbose) {
        char escaped_path[2048];
        char escaped_pattern[1024];
        ds_escape_value(path, escaped_path, sizeof(escaped_path));
        ds_escape_value(pattern, escaped_pattern, sizeof(escaped_pattern));

        if (component == NULL || *component == '\0') {
            ds_log_with_priority(LOG_DEBUG, 
                "event=compare path=\"%s\" pattern=\"%s\" matched=%s", 
                escaped_path, escaped_pattern, matched ? "true" : "false");
        } else {
            ds_log_with_priority(LOG_DEBUG, 
                "component=\"%s\" event=compare path=\"%s\" pattern=\"%s\" matched=%s", 
                component, escaped_path, escaped_pattern, matched ? "true" : "false");
        }
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