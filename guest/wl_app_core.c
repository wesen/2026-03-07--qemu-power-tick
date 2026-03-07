#define _GNU_SOURCE

#include "wl_app_core.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

uint64_t now_ms(void) {
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        perror("clock_gettime");
        exit(1);
    }
    return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)ts.tv_nsec / 1000000ull;
}

void log_line(const char *kind, const char *message) {
    printf("@@LOG kind=%s %s\n", kind, message);
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
