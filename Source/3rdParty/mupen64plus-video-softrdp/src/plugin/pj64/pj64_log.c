#include "pj64_log.h"

#if SOFTRDP_ENABLE_LOG

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

static FILE *g_log;
static unsigned g_log_lines;
static bool g_log_truncated;

#define PJ64_LOG_MAX_LINES 100000u

void pj64_log_open(void)
{
    if (g_log) {
        return;
    }

    g_log = fopen("softrdp.log", "w");
    g_log_lines = 0;
    g_log_truncated = false;
    if (g_log) {
        time_t now = time(NULL);
        fprintf(g_log, "SoftRDP log start: %lld\n", (long long)now);
        g_log_lines++;
        fflush(g_log);
    }
}

void pj64_log_close(void)
{
    if (g_log) {
        if (!g_log_truncated) {
            fprintf(g_log, "SoftRDP log end\n");
        }
        fclose(g_log);
        g_log = NULL;
    }
}

bool pj64_log_is_open(void)
{
    return g_log != NULL;
}

void pj64_log_printf(const char *fmt, ...)
{
    va_list args;

    if (!g_log || !fmt) {
        return;
    }

    if (g_log_lines >= PJ64_LOG_MAX_LINES) {
        if (!g_log_truncated) {
            fprintf(g_log, "SoftRDP log truncated after %u lines\n", PJ64_LOG_MAX_LINES);
            fflush(g_log);
            g_log_truncated = true;
        }
        return;
    }

    va_start(args, fmt);
    vfprintf(g_log, fmt, args);
    va_end(args);
    fputc('\n', g_log);
    g_log_lines++;
    fflush(g_log);
}

#endif
