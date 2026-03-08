#define _GNU_SOURCE

#include "wl_suspend.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

#include "wl_net.h"

static void arm_timer_ms(int fd, int first_ms, int interval_ms) {
    struct itimerspec spec = {0};

    spec.it_value.tv_sec = first_ms / 1000;
    spec.it_value.tv_nsec = (long)(first_ms % 1000) * 1000000L;
    spec.it_interval.tv_sec = interval_ms / 1000;
    spec.it_interval.tv_nsec = (long)(interval_ms % 1000) * 1000000L;
    if (timerfd_settime(fd, 0, &spec, NULL) != 0) {
        perror("timerfd_settime");
        exit(1);
    }
}

static void disarm_timer(int fd) {
    struct itimerspec spec = {0};

    if (timerfd_settime(fd, 0, &spec, NULL) != 0) {
        perror("timerfd_settime");
        exit(1);
    }
}

static int write_text_file(const char *path, const char *value) {
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    ssize_t written;

    if (fd < 0) {
        return -1;
    }
    written = write(fd, value, strlen(value));
    close(fd);
    if (written != (ssize_t)strlen(value)) {
        errno = EIO;
        return -1;
    }
    return 0;
}

static void program_rtc_wakealarm(struct app *app) {
    char msg[128];
    char value[64];
    struct timespec ts;

    if (app->wake_seconds <= 0) {
        return;
    }

    if (write_text_file("/sys/class/rtc/rtc0/wakealarm", "0\n") != 0 && errno != ENOENT) {
        snprintf(msg, sizeof(msg), "wakealarm_clear_failed=%d(%s)", errno, strerror(errno));
        log_line("error", msg);
    }

    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        snprintf(msg, sizeof(msg), "clock_realtime_failed=%d(%s)", errno, strerror(errno));
        log_line("error", msg);
        return;
    }

    snprintf(value, sizeof(value), "%" PRIu64 "\n", (uint64_t)ts.tv_sec + (uint64_t)app->wake_seconds);
    if (write_text_file("/sys/class/rtc/rtc0/wakealarm", value) != 0) {
        snprintf(msg, sizeof(msg), "wakealarm_program_failed=%d(%s)", errno, strerror(errno));
        log_line("error", msg);
        return;
    }

    snprintf(msg, sizeof(msg), "wakealarm_seconds=%d", app->wake_seconds);
    log_line("state", msg);
}

void reset_idle_timer(struct app *app) {
    if (app->no_suspend ||
        (app->max_suspend_cycles >= 0 && (int)app->suspend_cycles >= app->max_suspend_cycles)) {
        disarm_timer(app->idle_timer_fd);
        return;
    }

    arm_timer_ms(app->idle_timer_fd, app->idle_seconds * 1000, 0);
}

void maybe_exit_on_runtime_limit(struct app *app) {
    if (app->runtime_seconds > 0 &&
        mono_ns() - app->started_mono_ns >= (uint64_t)app->runtime_seconds * 1000000000ull) {
        log_line("state", "runtime_limit_reached=true");
        app->running = false;
    }
}

void enter_suspend_to_idle(struct app *app) {
    uint64_t before_mono;
    uint64_t before_boot;
    uint64_t after_mono;
    uint64_t after_boot;
    char msg[256];

    if (app->no_suspend) {
        log_line("state", "suspend_skipped=true reason=no_suspend");
        reset_idle_timer(app);
        return;
    }
    if (app->max_suspend_cycles >= 0 && (int)app->suspend_cycles >= app->max_suspend_cycles) {
        log_line("state", "suspend_skipped=true reason=max_suspend_cycles");
        reset_idle_timer(app);
        return;
    }

    if (strcmp(app->pm_test, "none") != 0) {
        if (write_text_file("/sys/power/pm_test", app->pm_test) != 0) {
            snprintf(msg, sizeof(msg), "pm_test_write_failed=%s", strerror(errno));
            log_line("error", msg);
        }
    }

    before_mono = mono_ns();
    before_boot = boot_ns();
    program_rtc_wakealarm(app);
    snprintf(msg, sizeof(msg), "state=SUSPENDING cycle=%" PRIu64, app->suspend_cycles + 1);
    log_line("state", msg);
    fflush(stdout);

    if (write_text_file("/sys/power/state", "freeze\n") != 0) {
        snprintf(msg, sizeof(msg), "suspend_write_failed=%s", strerror(errno));
        log_line("error", msg);
        reset_idle_timer(app);
        return;
    }

    after_mono = mono_ns();
    after_boot = boot_ns();
    app->suspend_cycles++;

    memset(&app->last_cycle, 0, sizeof(app->last_cycle));
    app->last_cycle.request_mono_ns = before_mono;
    app->last_cycle.request_boot_ns = before_boot;
    app->last_cycle.resume_mono_ns = after_mono;
    app->last_cycle.resume_boot_ns = after_boot;
    app->last_cycle.sleep_ms = ns_to_ms(after_boot - before_boot);
    app->last_cycle.resume_gap_ms = ns_to_ms(after_mono - before_mono);

    snprintf(msg, sizeof(msg),
             "state=RESUMED cycle=%" PRIu64 " sleep_ms=%" PRIu64 " resume_gap_ms=%" PRIu64,
             app->suspend_cycles, app->last_cycle.sleep_ms, app->last_cycle.resume_gap_ms);
    log_line("state", msg);
    log_metric_pair(app, "sleep_duration", app->last_cycle.sleep_ms);
    log_metric_pair(app, "suspend_resume_gap", app->last_cycle.resume_gap_ms);

    inspect_socket_health_after_resume(app);
    app->next_reconnect_ms = now_ms();
    set_redraw_reason(app, "resume");
    reset_idle_timer(app);
}
