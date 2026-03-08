#ifndef WL_APP_CORE_H
#define WL_APP_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
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
    int idle_timer_fd;
    int signal_fd;

    bool configured;
    bool running;
    bool redraw_pending;

    int width;
    int height;

    char host[64];
    int port;
    int idle_seconds;
    int runtime_seconds;
    int max_suspend_cycles;
    int wake_seconds;
    bool no_suspend;
    char pm_test[32];
    enum conn_state conn_state;
    uint64_t packets_received;
    uint64_t connect_attempts;
    uint64_t last_packet_ms;
    uint64_t started_ms;
    uint64_t next_reconnect_ms;
    uint64_t frame_counter;
    uint64_t suspend_cycles;
    uint64_t started_mono_ns;
    uint64_t started_boot_ns;

    int pointer_x;
    int pointer_y;
    uint64_t key_events;
    uint64_t pointer_events;
    char last_key[64];
    char last_pointer[64];
    char last_redraw_reason[64];

    bool keyboard_fallback_bound;
    bool pointer_fallback_bound;

    struct cycle_metrics last_cycle;
};

uint64_t now_ns(clockid_t clock_id);
uint64_t mono_ns(void);
uint64_t boot_ns(void);
uint64_t now_ms(void);
uint64_t ns_to_ms(uint64_t ns);
double ns_to_seconds(uint64_t ns);
void log_line(const char *kind, const char *message);
void log_metric_pair(struct app *app, const char *name, uint64_t value_ms);
void set_redraw_reason(struct app *app, const char *reason);
const char *conn_state_name(enum conn_state state);

#endif
