#define _GNU_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

enum {
    EV_SOCKET = 1,
    EV_REDRAW = 2,
    EV_IDLE = 3,
    EV_SIGNAL = 4,
};

enum conn_state {
    CONN_DISCONNECTED = 0,
    CONN_CONNECTING = 1,
    CONN_CONNECTED = 2,
};

static const char *conn_state_name(enum conn_state state) {
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

struct cycle_metrics {
    uint64_t request_mono_ns;
    uint64_t request_boot_ns;
    uint64_t resume_mono_ns;
    uint64_t resume_boot_ns;
    uint64_t sleep_ms;
    uint64_t resume_gap_ms;
    uint64_t redraw_latency_ms;
    uint64_t reconnect_latency_ms;
    bool redraw_recorded;
    bool reconnect_recorded;
};

struct app_state {
    int epoll_fd;
    int sock_fd;
    int redraw_timer_fd;
    int idle_timer_fd;
    int signal_fd;

    enum conn_state conn_state;

    char host[64];
    int port;
    int idle_seconds;
    int redraw_ms;
    int reconnect_ms;
    int runtime_seconds;
    int max_suspend_cycles;
    int wake_seconds;
    bool no_suspend;

    char pm_test[32];

    bool shutdown_requested;
    bool redraw_pending;
    bool screen_initialized;

    uint64_t bytes_received;
    uint64_t packets_received;
    uint64_t suspend_cycles;
    uint64_t connect_attempts;

    uint64_t started_mono_ns;
    uint64_t started_boot_ns;
    uint64_t last_redraw_mono_ns;
    uint64_t last_network_mono_ns;
    uint64_t last_suspend_mono_ns;
    uint64_t last_suspend_boot_ns;
    uint64_t last_resume_mono_ns;
    uint64_t last_resume_boot_ns;
    uint64_t next_reconnect_mono_ns;

    struct cycle_metrics last_cycle;
};

static uint64_t now_ns(clockid_t clock_id) {
    struct timespec ts;
    if (clock_gettime(clock_id, &ts) != 0) {
        perror("clock_gettime");
        exit(1);
    }
    return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static uint64_t mono_ns(void) {
    return now_ns(CLOCK_MONOTONIC);
}

static uint64_t boot_ns(void) {
    return now_ns(CLOCK_BOOTTIME);
}

static uint64_t ns_to_ms(uint64_t ns) {
    return ns / 1000000ull;
}

static double ns_to_seconds(uint64_t ns) {
    return (double)ns / 1000000000.0;
}

static void log_line(struct app_state *s, const char *kind, const char *message) {
    printf("@@LOG kind=%s mono_ms=%" PRIu64 " boot_ms=%" PRIu64 " %s\n",
           kind,
           ns_to_ms(mono_ns() - s->started_mono_ns),
           ns_to_ms(boot_ns() - s->started_boot_ns),
           message);
    fflush(stdout);
}

static void log_metric_pair(struct app_state *s, const char *name, uint64_t value_ms) {
    printf("@@METRIC name=%s value_ms=%" PRIu64 " cycle=%" PRIu64 "\n",
           name, value_ms, s->suspend_cycles);
    fflush(stdout);
}

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int epoll_add_fd(int epoll_fd, int fd, uint32_t id, uint32_t events) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = events;
    ev.data.u32 = id;
    return epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}

static int epoll_mod_fd(int epoll_fd, int fd, uint32_t id, uint32_t events) {
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = events;
    ev.data.u32 = id;
    return epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev);
}

static void request_redraw(struct app_state *s) {
    s->redraw_pending = true;
}

static void arm_timer_ms(int fd, int first_ms, int interval_ms) {
    struct itimerspec spec;
    memset(&spec, 0, sizeof(spec));
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
    struct itimerspec spec;
    memset(&spec, 0, sizeof(spec));
    if (timerfd_settime(fd, 0, &spec, NULL) != 0) {
        perror("timerfd_settime");
        exit(1);
    }
}

static void reset_idle_timer(struct app_state *s) {
    if (s->no_suspend || (s->max_suspend_cycles >= 0 && (int)s->suspend_cycles >= s->max_suspend_cycles)) {
        disarm_timer(s->idle_timer_fd);
        return;
    }

    arm_timer_ms(s->idle_timer_fd, s->idle_seconds * 1000, 0);
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

static void program_rtc_wakealarm(struct app_state *s) {
    char value[64];
    struct timespec ts;

    if (s->wake_seconds <= 0) {
        return;
    }

    if (write_text_file("/sys/class/rtc/rtc0/wakealarm", "0\n") != 0 && errno != ENOENT) {
        char msg[128];
        snprintf(msg, sizeof(msg), "wakealarm_clear_failed=%d(%s)", errno, strerror(errno));
        log_line(s, "error", msg);
    }

    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "clock_realtime_failed=%d(%s)", errno, strerror(errno));
        log_line(s, "error", msg);
        return;
    }

    snprintf(value, sizeof(value), "%" PRIu64 "\n", (uint64_t)ts.tv_sec + (uint64_t)s->wake_seconds);
    if (write_text_file("/sys/class/rtc/rtc0/wakealarm", value) != 0) {
        char msg[128];
        snprintf(msg, sizeof(msg), "wakealarm_program_failed=%d(%s)", errno, strerror(errno));
        log_line(s, "error", msg);
        return;
    }

    {
        char msg[128];
        snprintf(msg, sizeof(msg), "wakealarm_seconds=%d", s->wake_seconds);
        log_line(s, "state", msg);
    }
}

static void close_socket(struct app_state *s) {
    if (s->sock_fd >= 0) {
        epoll_ctl(s->epoll_fd, EPOLL_CTL_DEL, s->sock_fd, NULL);
        close(s->sock_fd);
        s->sock_fd = -1;
    }
}

static void schedule_reconnect(struct app_state *s, const char *reason) {
    char msg[256];
    close_socket(s);
    s->conn_state = CONN_DISCONNECTED;
    s->next_reconnect_mono_ns = mono_ns() + (uint64_t)s->reconnect_ms * 1000000ull;
    snprintf(msg, sizeof(msg), "state=DISCONNECTED reason=%s reconnect_in_ms=%d", reason, s->reconnect_ms);
    log_line(s, "state", msg);
    request_redraw(s);
}

static void update_socket_interest(struct app_state *s) {
    uint32_t events = EPOLLERR | EPOLLHUP | EPOLLRDHUP;
    if (s->conn_state == CONN_CONNECTING) {
        events |= EPOLLOUT | EPOLLIN;
    } else {
        events |= EPOLLIN;
    }

    if (epoll_mod_fd(s->epoll_fd, s->sock_fd, EV_SOCKET, events) != 0) {
        perror("epoll_ctl mod socket");
        exit(1);
    }
}

static void attempt_connect(struct app_state *s) {
    struct sockaddr_in addr;
    int fd;
    int rc;
    char msg[256];

    if (s->sock_fd >= 0 || mono_ns() < s->next_reconnect_mono_ns) {
        return;
    }

    fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        char reason[128];
        snprintf(reason, sizeof(reason), "socket_failed errno=%d(%s)", errno, strerror(errno));
        schedule_reconnect(s, reason);
        return;
    }
    if (set_nonblocking(fd) != 0) {
        close(fd);
        {
            char reason[128];
            snprintf(reason, sizeof(reason), "nonblocking_failed errno=%d(%s)", errno, strerror(errno));
            schedule_reconnect(s, reason);
        }
        return;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)s->port);
    if (inet_pton(AF_INET, s->host, &addr.sin_addr) != 1) {
        fprintf(stderr, "invalid host: %s\n", s->host);
        exit(1);
    }

    s->sock_fd = fd;
    s->connect_attempts++;
    s->conn_state = CONN_CONNECTING;

    if (epoll_add_fd(s->epoll_fd, fd, EV_SOCKET, EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP) != 0) {
        perror("epoll add socket");
        exit(1);
    }

    rc = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (rc == 0) {
        s->conn_state = CONN_CONNECTED;
        snprintf(msg, sizeof(msg), "state=CONNECTED host=%s port=%d connect_attempt=%" PRIu64,
                 s->host, s->port, s->connect_attempts);
        log_line(s, "state", msg);
        request_redraw(s);
        return;
    }

    if (errno == EINPROGRESS) {
        snprintf(msg, sizeof(msg), "state=CONNECTING host=%s port=%d connect_attempt=%" PRIu64,
                 s->host, s->port, s->connect_attempts);
        log_line(s, "state", msg);
        request_redraw(s);
        return;
    }

    {
        char reason[128];
        snprintf(reason, sizeof(reason), "connect_failed errno=%d(%s)", errno, strerror(errno));
        schedule_reconnect(s, reason);
    }
}

static void record_reconnect_metric_if_needed(struct app_state *s) {
    if (!s->last_cycle.reconnect_recorded && s->last_cycle.resume_boot_ns != 0 && s->conn_state == CONN_CONNECTED) {
        s->last_cycle.reconnect_latency_ms = ns_to_ms(boot_ns() - s->last_cycle.resume_boot_ns);
        s->last_cycle.reconnect_recorded = true;
        log_metric_pair(s, "resume_to_reconnect", s->last_cycle.reconnect_latency_ms);
    }
}

static void mark_connected(struct app_state *s) {
    char msg[256];
    s->conn_state = CONN_CONNECTED;
    update_socket_interest(s);
    snprintf(msg, sizeof(msg), "state=CONNECTED host=%s port=%d connect_attempt=%" PRIu64,
             s->host, s->port, s->connect_attempts);
    log_line(s, "state", msg);
    record_reconnect_metric_if_needed(s);
    request_redraw(s);
}

static void handle_connect_completion(struct app_state *s) {
    int err = 0;
    socklen_t err_len = sizeof(err);

    if (getsockopt(s->sock_fd, SOL_SOCKET, SO_ERROR, &err, &err_len) != 0) {
        schedule_reconnect(s, "getsockopt_failed");
        return;
    }
    if (err != 0) {
        errno = err;
        schedule_reconnect(s, strerror(err));
        return;
    }
    mark_connected(s);
}

static void render_screen(struct app_state *s) {
    uint64_t now_mono = mono_ns();
    double uptime = ns_to_seconds(now_mono - s->started_mono_ns);
    uint64_t since_network_ms = s->last_network_mono_ns == 0 ? 0 : ns_to_ms(now_mono - s->last_network_mono_ns);

    printf("\033[2J\033[H");
    printf("sleepdemo stage-1 lab\n");
    printf("=====================\n");
    printf("connection: %s\n", conn_state_name(s->conn_state));
    printf("host: %s:%d\n", s->host, s->port);
    printf("uptime_s: %.3f\n", uptime);
    printf("bytes_received: %" PRIu64 "\n", s->bytes_received);
    printf("packets_received: %" PRIu64 "\n", s->packets_received);
    printf("suspend_cycles: %" PRIu64 "\n", s->suspend_cycles);
    printf("connect_attempts: %" PRIu64 "\n", s->connect_attempts);
    printf("idle_seconds: %d\n", s->idle_seconds);
    printf("redraw_ms: %d\n", s->redraw_ms);
    printf("pm_test: %s\n", s->pm_test);
    printf("no_suspend: %s\n", s->no_suspend ? "true" : "false");
    printf("last_network_age_ms: %" PRIu64 "\n", since_network_ms);
    printf("last_sleep_ms: %" PRIu64 "\n", s->last_cycle.sleep_ms);
    printf("last_resume_gap_ms: %" PRIu64 "\n", s->last_cycle.resume_gap_ms);
    printf("last_redraw_latency_ms: %" PRIu64 "\n", s->last_cycle.redraw_latency_ms);
    printf("last_reconnect_latency_ms: %" PRIu64 "\n", s->last_cycle.reconnect_latency_ms);
    printf("\n");
    printf("Recent metrics are also emitted as @@METRIC lines for parsing.\n");
    fflush(stdout);

    s->screen_initialized = true;
    s->redraw_pending = false;
    s->last_redraw_mono_ns = now_mono;

    if (!s->last_cycle.redraw_recorded && s->last_cycle.resume_boot_ns != 0) {
        s->last_cycle.redraw_latency_ms = ns_to_ms(boot_ns() - s->last_cycle.resume_boot_ns);
        s->last_cycle.redraw_recorded = true;
        log_metric_pair(s, "resume_to_redraw", s->last_cycle.redraw_latency_ms);
    }
}

static void consume_timerfd(int fd) {
    uint64_t expirations = 0;
    ssize_t n = read(fd, &expirations, sizeof(expirations));
    if (n < 0 && errno != EAGAIN) {
        perror("read timerfd");
        exit(1);
    }
}

static void maybe_exit_on_runtime_limit(struct app_state *s) {
    if (s->runtime_seconds > 0 && mono_ns() - s->started_mono_ns >= (uint64_t)s->runtime_seconds * 1000000000ull) {
        log_line(s, "state", "runtime_limit_reached=true");
        s->shutdown_requested = true;
    }
}

static void inspect_socket_health_after_resume(struct app_state *s) {
    char byte;
    ssize_t n;

    if (s->sock_fd < 0) {
        s->next_reconnect_mono_ns = mono_ns();
        return;
    }

    n = recv(s->sock_fd, &byte, 1, MSG_PEEK | MSG_DONTWAIT);
    if (n == 0) {
        schedule_reconnect(s, "peer_closed_after_resume");
        return;
    }
    if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        schedule_reconnect(s, "socket_error_after_resume");
    }
}

static void enter_suspend_to_idle(struct app_state *s) {
    uint64_t before_mono;
    uint64_t before_boot;
    uint64_t after_mono;
    uint64_t after_boot;
    char msg[256];

    if (s->no_suspend) {
        log_line(s, "state", "suspend_skipped=true reason=no_suspend");
        reset_idle_timer(s);
        return;
    }
    if (s->max_suspend_cycles >= 0 && (int)s->suspend_cycles >= s->max_suspend_cycles) {
        log_line(s, "state", "suspend_skipped=true reason=max_suspend_cycles");
        reset_idle_timer(s);
        return;
    }

    if (strcmp(s->pm_test, "none") != 0) {
        if (write_text_file("/sys/power/pm_test", s->pm_test) != 0) {
            snprintf(msg, sizeof(msg), "pm_test_write_failed=%s", strerror(errno));
            log_line(s, "error", msg);
        }
    }

    before_mono = mono_ns();
    before_boot = boot_ns();
    program_rtc_wakealarm(s);
    s->last_suspend_mono_ns = before_mono;
    s->last_suspend_boot_ns = before_boot;
    snprintf(msg, sizeof(msg), "state=SUSPENDING cycle=%" PRIu64, s->suspend_cycles + 1);
    log_line(s, "state", msg);
    fflush(stdout);

    if (write_text_file("/sys/power/state", "freeze\n") != 0) {
        snprintf(msg, sizeof(msg), "suspend_write_failed=%s", strerror(errno));
        log_line(s, "error", msg);
        reset_idle_timer(s);
        return;
    }

    after_mono = mono_ns();
    after_boot = boot_ns();
    s->suspend_cycles++;
    s->last_resume_mono_ns = after_mono;
    s->last_resume_boot_ns = after_boot;

    memset(&s->last_cycle, 0, sizeof(s->last_cycle));
    s->last_cycle.request_mono_ns = before_mono;
    s->last_cycle.request_boot_ns = before_boot;
    s->last_cycle.resume_mono_ns = after_mono;
    s->last_cycle.resume_boot_ns = after_boot;
    s->last_cycle.sleep_ms = ns_to_ms(after_boot - before_boot);
    s->last_cycle.resume_gap_ms = ns_to_ms(after_mono - before_mono);

    snprintf(msg, sizeof(msg),
             "state=RESUMED cycle=%" PRIu64 " sleep_ms=%" PRIu64 " resume_gap_ms=%" PRIu64,
             s->suspend_cycles, s->last_cycle.sleep_ms, s->last_cycle.resume_gap_ms);
    log_line(s, "state", msg);
    log_metric_pair(s, "sleep_duration", s->last_cycle.sleep_ms);
    log_metric_pair(s, "suspend_resume_gap", s->last_cycle.resume_gap_ms);

    inspect_socket_health_after_resume(s);
    s->next_reconnect_mono_ns = mono_ns();
    request_redraw(s);
    reset_idle_timer(s);
}

static void handle_socket_readable(struct app_state *s) {
    char buffer[4096];
    for (;;) {
        ssize_t n = read(s->sock_fd, buffer, sizeof(buffer));
        if (n > 0) {
            s->bytes_received += (uint64_t)n;
            s->packets_received++;
            s->last_network_mono_ns = mono_ns();
            reset_idle_timer(s);
            request_redraw(s);
            continue;
        }
        if (n == 0) {
            schedule_reconnect(s, "peer_closed");
            return;
        }
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        schedule_reconnect(s, strerror(errno));
        return;
    }
}

static void handle_socket_event(struct app_state *s, uint32_t events) {
    if (s->conn_state == CONN_CONNECTING && (events & (EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP))) {
        handle_connect_completion(s);
    }

    if (s->conn_state == CONN_CONNECTED && (events & EPOLLIN)) {
        handle_socket_readable(s);
    }

    if (s->sock_fd >= 0 && (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) && s->conn_state != CONN_CONNECTING) {
        schedule_reconnect(s, "hangup_or_error");
    }
}

static void handle_signal_event(struct app_state *s) {
    struct signalfd_siginfo fdsi;
    ssize_t n = read(s->signal_fd, &fdsi, sizeof(fdsi));
    char msg[128];
    if (n != sizeof(fdsi)) {
        return;
    }
    snprintf(msg, sizeof(msg), "state=SHUTTING_DOWN signal=%u", fdsi.ssi_signo);
    log_line(s, "state", msg);
    s->shutdown_requested = true;
}

static void handle_redraw_timer(struct app_state *s) {
    consume_timerfd(s->redraw_timer_fd);
    if (s->redraw_pending || !s->screen_initialized) {
        render_screen(s);
    }
    attempt_connect(s);
    maybe_exit_on_runtime_limit(s);
}

static void handle_idle_timer(struct app_state *s) {
    consume_timerfd(s->idle_timer_fd);
    enter_suspend_to_idle(s);
}

static void setup_signals(struct app_state *s) {
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    if (sigprocmask(SIG_BLOCK, &mask, NULL) != 0) {
        perror("sigprocmask");
        exit(1);
    }
    s->signal_fd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
    if (s->signal_fd < 0) {
        perror("signalfd");
        exit(1);
    }
}

static void init_state(struct app_state *s) {
    memset(s, 0, sizeof(*s));
    s->epoll_fd = -1;
    s->sock_fd = -1;
    s->redraw_timer_fd = -1;
    s->idle_timer_fd = -1;
    s->signal_fd = -1;
    strcpy(s->host, "127.0.0.1");
    strcpy(s->pm_test, "none");
    s->port = 5555;
    s->idle_seconds = 5;
    s->redraw_ms = 1000;
    s->reconnect_ms = 1000;
    s->runtime_seconds = 0;
    s->max_suspend_cycles = 1;
    s->wake_seconds = 5;
}

static void usage(const char *prog) {
    fprintf(stderr,
            "usage: %s [--host IP] [--port PORT] [--idle-seconds N] [--redraw-ms N]\n"
            "          [--reconnect-ms N] [--runtime-seconds N] [--pm-test MODE]\n"
            "          [--max-suspend-cycles N] [--wake-seconds N] [--no-suspend]\n",
            prog);
}

static void parse_args(struct app_state *s, int argc, char **argv) {
    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--host") == 0 && i + 1 < argc) {
            strncpy(s->host, argv[++i], sizeof(s->host) - 1);
        } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            s->port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--idle-seconds") == 0 && i + 1 < argc) {
            s->idle_seconds = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--redraw-ms") == 0 && i + 1 < argc) {
            s->redraw_ms = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--reconnect-ms") == 0 && i + 1 < argc) {
            s->reconnect_ms = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--runtime-seconds") == 0 && i + 1 < argc) {
            s->runtime_seconds = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--pm-test") == 0 && i + 1 < argc) {
            strncpy(s->pm_test, argv[++i], sizeof(s->pm_test) - 1);
        } else if (strcmp(argv[i], "--max-suspend-cycles") == 0 && i + 1 < argc) {
            s->max_suspend_cycles = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--wake-seconds") == 0 && i + 1 < argc) {
            s->wake_seconds = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--no-suspend") == 0) {
            s->no_suspend = true;
        } else {
            usage(argv[0]);
            exit(1);
        }
    }
}

int main(int argc, char **argv) {
    struct app_state state;
    struct epoll_event events[8];
    int n;
    int i;

    init_state(&state);
    parse_args(&state, argc, argv);

    state.started_mono_ns = mono_ns();
    state.started_boot_ns = boot_ns();

    setup_signals(&state);

    state.epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (state.epoll_fd < 0) {
        perror("epoll_create1");
        return 1;
    }

    state.redraw_timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    state.idle_timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (state.redraw_timer_fd < 0 || state.idle_timer_fd < 0) {
        perror("timerfd_create");
        return 1;
    }

    if (epoll_add_fd(state.epoll_fd, state.redraw_timer_fd, EV_REDRAW, EPOLLIN) != 0 ||
        epoll_add_fd(state.epoll_fd, state.idle_timer_fd, EV_IDLE, EPOLLIN) != 0 ||
        epoll_add_fd(state.epoll_fd, state.signal_fd, EV_SIGNAL, EPOLLIN) != 0) {
        perror("epoll add");
        return 1;
    }

    arm_timer_ms(state.redraw_timer_fd, 1, state.redraw_ms);
    reset_idle_timer(&state);
    request_redraw(&state);
    log_line(&state, "state", "state=STARTING");
    attempt_connect(&state);

    while (!state.shutdown_requested) {
        n = epoll_wait(state.epoll_fd, events, (int)(sizeof(events) / sizeof(events[0])), -1);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("epoll_wait");
            return 1;
        }

        for (i = 0; i < n; i++) {
            switch (events[i].data.u32) {
            case EV_SOCKET:
                handle_socket_event(&state, events[i].events);
                break;
            case EV_REDRAW:
                handle_redraw_timer(&state);
                break;
            case EV_IDLE:
                handle_idle_timer(&state);
                break;
            case EV_SIGNAL:
                handle_signal_event(&state);
                break;
            default:
                break;
            }
        }
    }

    log_line(&state, "state", "state=EXITING");
    close_socket(&state);
    if (state.signal_fd >= 0) {
        close(state.signal_fd);
    }
    if (state.redraw_timer_fd >= 0) {
        close(state.redraw_timer_fd);
    }
    if (state.idle_timer_fd >= 0) {
        close(state.idle_timer_fd);
    }
    if (state.epoll_fd >= 0) {
        close(state.epoll_fd);
    }

    return 0;
}
