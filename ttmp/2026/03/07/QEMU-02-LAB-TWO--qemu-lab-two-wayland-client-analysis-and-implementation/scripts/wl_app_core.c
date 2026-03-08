#define _GNU_SOURCE

#include "wl_app_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

uint64_t now_ns(clockid_t clock_id) {
    struct timespec ts;

    if (clock_gettime(clock_id, &ts) != 0) {
        perror("clock_gettime");
        exit(1);
    }
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

uint64_t mono_ns(void) {
    return now_ns(CLOCK_MONOTONIC);
}

uint64_t boot_ns(void) {
    return now_ns(CLOCK_BOOTTIME);
}

uint64_t now_ms(void) {
    return ns_to_ms(mono_ns());
}

uint64_t ns_to_ms(uint64_t ns) {
    return ns / 1000000ull;
}

double ns_to_seconds(uint64_t ns) {
    return (double)ns / 1000000000.0;
}

void log_line(const char *kind, const char *message) {
    printf("@@LOG kind=%s %s\n", kind, message);
    fflush(stdout);
}

void log_metric_pair(struct app *app, const char *name, uint64_t value_ms) {
    printf("@@METRIC name=%s value_ms=%" PRIu64 " cycle=%" PRIu64 "\n",
           name, value_ms, app->suspend_cycles);
    fflush(stdout);
}

void set_redraw_reason(struct app *app, const char *reason) {
    snprintf(app->last_redraw_reason, sizeof(app->last_redraw_reason), "%s", reason);
    app->redraw_pending = true;
}

const char *conn_state_name(enum conn_state state) {
    switch (state) {
    case CONN_DISCONNECTED:
        return "DISCONNECTED";
    case CONN_CONNECTING:
        return "CONNECTING";
    case CONN_CONNECTED:
        return "CONNECTED";
    }
    return "UNKNOWN";
}
