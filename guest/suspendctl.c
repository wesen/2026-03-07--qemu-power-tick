#define _GNU_SOURCE

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

static uint64_t clock_ns(clockid_t clock_id) {
    struct timespec ts;

    if (clock_gettime(clock_id, &ts) != 0) {
        perror("clock_gettime");
        exit(1);
    }
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static uint64_t ns_to_ms(uint64_t ns) {
    return ns / 1000000ull;
}

static void log_line(const char *kind, const char *msg) {
    printf("@@LOG kind=%s %s\n", kind, msg);
    fflush(stdout);
}

static void log_metric_pair(const char *name, uint64_t value_ms) {
    printf("@@METRIC name=%s value_ms=%" PRIu64 " cycle=1\n", name, value_ms);
    fflush(stdout);
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

static void program_rtc_wakealarm(int wake_seconds) {
    char msg[128];
    char value[64];
    struct timespec ts;

    if (wake_seconds <= 0) {
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

    snprintf(value, sizeof(value), "%" PRIu64 "\n", (uint64_t)ts.tv_sec + (uint64_t)wake_seconds);
    if (write_text_file("/sys/class/rtc/rtc0/wakealarm", value) != 0) {
        snprintf(msg, sizeof(msg), "wakealarm_program_failed=%d(%s)", errno, strerror(errno));
        log_line("error", msg);
        return;
    }

    snprintf(msg, sizeof(msg), "wakealarm_seconds=%d", wake_seconds);
    log_line("state", msg);
}

static void usage(const char *argv0) {
    fprintf(stderr, "usage: %s [--wake-seconds N] [--pm-test NAME] [--delay-seconds N] [--no-suspend]\n", argv0);
}

int main(int argc, char **argv) {
    int wake_seconds = 5;
    int delay_seconds = 0;
    bool no_suspend = false;
    char pm_test[32];
    uint64_t before_mono;
    uint64_t before_boot;
    uint64_t after_mono;
    uint64_t after_boot;
    char msg[256];
    int i;

    snprintf(pm_test, sizeof(pm_test), "%s", "none");

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--wake-seconds") == 0 && i + 1 < argc) {
            wake_seconds = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--pm-test") == 0 && i + 1 < argc) {
            snprintf(pm_test, sizeof(pm_test), "%s", argv[++i]);
        } else if (strcmp(argv[i], "--delay-seconds") == 0 && i + 1 < argc) {
            delay_seconds = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--no-suspend") == 0) {
            no_suspend = true;
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    if (delay_seconds > 0) {
        sleep((unsigned int)delay_seconds);
    }

    if (no_suspend) {
        log_line("state", "suspend_skipped=true reason=no_suspend");
        return 0;
    }

    if (strcmp(pm_test, "none") != 0) {
        if (write_text_file("/sys/power/pm_test", pm_test) != 0) {
            snprintf(msg, sizeof(msg), "pm_test_write_failed=%s", strerror(errno));
            log_line("error", msg);
        }
    }

    before_mono = clock_ns(CLOCK_MONOTONIC);
    before_boot = clock_ns(CLOCK_BOOTTIME);
    program_rtc_wakealarm(wake_seconds);
    log_line("state", "state=SUSPENDING cycle=1");

    if (write_text_file("/sys/power/state", "freeze\n") != 0) {
        snprintf(msg, sizeof(msg), "suspend_write_failed=%s", strerror(errno));
        log_line("error", msg);
        return 1;
    }

    after_mono = clock_ns(CLOCK_MONOTONIC);
    after_boot = clock_ns(CLOCK_BOOTTIME);

    snprintf(msg, sizeof(msg),
             "state=RESUMED cycle=1 sleep_ms=%" PRIu64 " resume_gap_ms=%" PRIu64,
             ns_to_ms(after_boot - before_boot), ns_to_ms(after_mono - before_mono));
    log_line("state", msg);
    log_metric_pair("sleep_duration", ns_to_ms(after_boot - before_boot));
    log_metric_pair("suspend_resume_gap", ns_to_ms(after_mono - before_mono));
    return 0;
}
