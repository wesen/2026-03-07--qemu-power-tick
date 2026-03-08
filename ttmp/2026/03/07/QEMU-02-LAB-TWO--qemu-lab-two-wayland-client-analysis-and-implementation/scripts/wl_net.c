#define _GNU_SOURCE

#include "wl_net.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

#include "wl_suspend.h"

enum {
    EV_SOCKET = 2,
};

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags < 0) {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void schedule_reconnect(struct app *app, const char *reason) {
    if (app->socket_fd >= 0) {
        epoll_ctl(app->epoll_fd, EPOLL_CTL_DEL, app->socket_fd, NULL);
        close(app->socket_fd);
        app->socket_fd = -1;
    }
    app->conn_state = CONN_DISCONNECTED;
    app->next_reconnect_ms = now_ms() + 1000;
    log_line("state", reason);
    set_redraw_reason(app, "disconnect");
}

static void record_reconnect_metric_if_needed(struct app *app) {
    if (!app->last_cycle.reconnect_recorded && app->last_cycle.resume_boot_ns != 0 &&
        app->conn_state == CONN_CONNECTED) {
        app->last_cycle.reconnect_latency_ms = ns_to_ms(boot_ns() - app->last_cycle.resume_boot_ns);
        app->last_cycle.reconnect_recorded = true;
        log_metric_pair(app, "resume_to_reconnect", app->last_cycle.reconnect_latency_ms);
    }
}

void attempt_connect(struct app *app) {
    struct sockaddr_in addr = {0};
    struct epoll_event ev = {0};
    int fd;
    int rc;

    if (app->socket_fd >= 0 || now_ms() < app->next_reconnect_ms) {
        return;
    }

    fd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (fd < 0) {
        return;
    }
    if (set_nonblocking(fd) != 0) {
        close(fd);
        return;
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)app->port);
    if (inet_pton(AF_INET, app->host, &addr.sin_addr) != 1) {
        fprintf(stderr, "invalid host: %s\n", app->host);
        exit(1);
    }

    app->socket_fd = fd;
    app->connect_attempts++;
    app->conn_state = CONN_CONNECTING;
    rc = connect(fd, (struct sockaddr *)&addr, sizeof(addr));
    if (rc == 0 || errno == EINPROGRESS) {
        ev.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
        ev.data.u32 = EV_SOCKET;
        if (epoll_ctl(app->epoll_fd, EPOLL_CTL_ADD, fd, &ev) != 0) {
            perror("epoll_ctl add socket");
            exit(1);
        }
        set_redraw_reason(app, "connect-attempt");
        return;
    }

    close(fd);
    app->socket_fd = -1;
    app->conn_state = CONN_DISCONNECTED;
    app->next_reconnect_ms = now_ms() + 1000;
}

void handle_socket_event(struct app *app, uint32_t events) {
    if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
        schedule_reconnect(app, "socket-hup");
        return;
    }

    if (events & EPOLLOUT && app->conn_state == CONN_CONNECTING) {
        int err = 0;
        socklen_t err_len = sizeof(err);

        if (getsockopt(app->socket_fd, SOL_SOCKET, SO_ERROR, &err, &err_len) != 0 || err != 0) {
            schedule_reconnect(app, "connect-failed");
            return;
        }
        app->conn_state = CONN_CONNECTED;
        set_redraw_reason(app, "connected");
        log_line("state", "connected");
        record_reconnect_metric_if_needed(app);
    }

    if (events & EPOLLIN) {
        char buffer[512];
        ssize_t rc = recv(app->socket_fd, buffer, sizeof(buffer), 0);

        if (rc <= 0) {
            schedule_reconnect(app, "recv-failed");
            return;
        }
        app->packets_received++;
        app->last_packet_ms = now_ms();
        reset_idle_timer(app);
        set_redraw_reason(app, "packet");
    }
}

void inspect_socket_health_after_resume(struct app *app) {
    char byte;
    ssize_t n;

    if (app->socket_fd < 0) {
        app->next_reconnect_ms = now_ms();
        return;
    }

    n = recv(app->socket_fd, &byte, 1, MSG_PEEK | MSG_DONTWAIT);
    if (n == 0) {
        schedule_reconnect(app, "peer_closed_after_resume");
        return;
    }
    if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
        schedule_reconnect(app, "socket_error_after_resume");
    }
}
