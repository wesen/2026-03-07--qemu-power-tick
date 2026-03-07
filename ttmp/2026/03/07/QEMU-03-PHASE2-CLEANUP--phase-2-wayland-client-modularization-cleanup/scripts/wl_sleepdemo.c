#define _GNU_SOURCE

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "wl_app_core.h"
#include "wl_net.h"
#include "wl_render.h"
#include "wl_wayland.h"

enum {
    EV_WAYLAND = 1,
    EV_SOCKET = 2,
    EV_REDRAW = 3,
    EV_SIGNAL = 4,
};

static void arm_redraw_timer(struct app *app, int first_ms, int interval_ms) {
    struct itimerspec spec = {0};

    spec.it_value.tv_sec = first_ms / 1000;
    spec.it_value.tv_nsec = (long)(first_ms % 1000) * 1000000L;
    spec.it_interval.tv_sec = interval_ms / 1000;
    spec.it_interval.tv_nsec = (long)(interval_ms % 1000) * 1000000L;
    if (timerfd_settime(app->redraw_timer_fd, 0, &spec, NULL) != 0) {
        perror("timerfd_settime");
        exit(1);
    }
}

static void setup_epoll(struct app *app) {
    struct epoll_event ev = {0};
    sigset_t mask;

    app->epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (app->epoll_fd < 0) {
        perror("epoll_create1");
        exit(1);
    }

    ev.events = EPOLLIN;
    ev.data.u32 = EV_WAYLAND;
    if (epoll_ctl(app->epoll_fd, EPOLL_CTL_ADD, wl_display_get_fd(app->display), &ev) != 0) {
        perror("epoll_ctl add wayland");
        exit(1);
    }

    app->redraw_timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    if (app->redraw_timer_fd < 0) {
        perror("timerfd_create");
        exit(1);
    }
    arm_redraw_timer(app, 1000, 1000);
    ev.events = EPOLLIN;
    ev.data.u32 = EV_REDRAW;
    if (epoll_ctl(app->epoll_fd, EPOLL_CTL_ADD, app->redraw_timer_fd, &ev) != 0) {
        perror("epoll_ctl add redraw");
        exit(1);
    }

    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigprocmask(SIG_BLOCK, &mask, NULL);
    app->signal_fd = signalfd(-1, &mask, SFD_CLOEXEC);
    if (app->signal_fd < 0) {
        perror("signalfd");
        exit(1);
    }
    ev.events = EPOLLIN;
    ev.data.u32 = EV_SIGNAL;
    if (epoll_ctl(app->epoll_fd, EPOLL_CTL_ADD, app->signal_fd, &ev) != 0) {
        perror("epoll_ctl add signal");
        exit(1);
    }
}

static void parse_args(struct app *app, int argc, char **argv) {
    snprintf(app->host, sizeof(app->host), "%s", "10.0.2.2");
    app->port = 5555;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--host") == 0 && i + 1 < argc) {
            snprintf(app->host, sizeof(app->host), "%s", argv[++i]);
        } else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            app->port = atoi(argv[++i]);
        } else {
            fprintf(stderr, "unknown arg: %s\n", argv[i]);
            exit(1);
        }
    }
}

int main(int argc, char **argv) {
    struct app app = {0};
    struct epoll_event events[8];

    app.socket_fd = -1;
    app.running = true;
    app.pointer_x = 640;
    app.pointer_y = 400;
    snprintf(app.last_redraw_reason, sizeof(app.last_redraw_reason), "%s", "boot");
    snprintf(app.last_key, sizeof(app.last_key), "%s", "NONE");
    snprintf(app.last_pointer, sizeof(app.last_pointer), "%s", "NONE");
    app.started_ms = now_ms();

    parse_args(&app, argc, argv);
    setup_wayland(&app);
    setup_epoll(&app);
    attempt_connect(&app);
    set_redraw_reason(&app, "startup");

    while (app.running) {
        int nfds;

        attempt_connect(&app);

        if (app.configured && app.redraw_pending) {
            render(&app);
        }

        wl_display_flush(app.display);
        nfds = epoll_wait(app.epoll_fd, events, 8, 250);
        if (nfds < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("epoll_wait");
            exit(1);
        }

        for (int i = 0; i < nfds; ++i) {
            switch (events[i].data.u32) {
            case EV_WAYLAND:
                if (wl_display_dispatch(app.display) < 0) {
                    app.running = false;
                }
                break;
            case EV_SOCKET:
                handle_socket_event(&app, events[i].events);
                break;
            case EV_REDRAW: {
                uint64_t expirations;

                if (read(app.redraw_timer_fd, &expirations, sizeof(expirations)) > 0) {
                    set_redraw_reason(&app, "timer");
                }
                break;
            }
            case EV_SIGNAL: {
                struct signalfd_siginfo fdsi;

                if (read(app.signal_fd, &fdsi, sizeof(fdsi)) == (ssize_t)sizeof(fdsi)) {
                    app.running = false;
                }
                break;
            }
            }
        }
    }

    return 0;
}
