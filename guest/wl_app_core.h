#ifndef WL_APP_CORE_H
#define WL_APP_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <wayland-client.h>

#include "xdg-shell-client-protocol.h"

enum conn_state {
    CONN_DISCONNECTED = 0,
    CONN_CONNECTING = 1,
    CONN_CONNECTED = 2,
};

struct buffer {
    struct wl_buffer *wl_buffer;
    void *data;
    size_t size;
    int width;
    int height;
    int stride;
};

struct app {
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_compositor *compositor;
    struct wl_shm *shm;
    struct wl_seat *seat;
    struct wl_keyboard *keyboard;
    struct wl_pointer *pointer;
    struct xdg_wm_base *wm_base;
    struct wl_surface *surface;
    struct xdg_surface *xdg_surface;
    struct xdg_toplevel *xdg_toplevel;

    struct buffer buffer;

    int epoll_fd;
    int socket_fd;
    int redraw_timer_fd;
    int signal_fd;

    bool configured;
    bool running;
    bool redraw_pending;

    int width;
    int height;

    char host[64];
    int port;
    enum conn_state conn_state;
    uint64_t packets_received;
    uint64_t connect_attempts;
    uint64_t last_packet_ms;
    uint64_t started_ms;
    uint64_t next_reconnect_ms;
    uint64_t frame_counter;

    int pointer_x;
    int pointer_y;
    uint64_t key_events;
    uint64_t pointer_events;
    char last_key[64];
    char last_pointer[64];
    char last_redraw_reason[64];

    bool keyboard_fallback_bound;
    bool pointer_fallback_bound;
};

uint64_t now_ms(void);
void log_line(const char *kind, const char *message);
void set_redraw_reason(struct app *app, const char *reason);
const char *conn_state_name(enum conn_state state);

#endif
