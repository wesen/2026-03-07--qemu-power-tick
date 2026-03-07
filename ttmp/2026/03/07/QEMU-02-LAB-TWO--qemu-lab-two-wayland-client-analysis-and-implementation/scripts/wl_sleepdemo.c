#define _GNU_SOURCE

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>
#include <wayland-client.h>

#include "xdg-shell-client-protocol.h"

enum {
    EV_WAYLAND = 1,
    EV_SOCKET = 2,
    EV_REDRAW = 3,
    EV_SIGNAL = 4,
};

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
};

static const uint8_t font5x7[][5] = {
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00},
    ['-'] = {0x08, 0x08, 0x08, 0x00, 0x08},
    ['.'] = {0x00, 0x00, 0x00, 0x00, 0x08},
    [':'] = {0x00, 0x08, 0x00, 0x08, 0x00},
    ['0'] = {0x1e, 0x11, 0x11, 0x11, 0x1e},
    ['1'] = {0x04, 0x0c, 0x04, 0x04, 0x0e},
    ['2'] = {0x1e, 0x01, 0x1e, 0x10, 0x1f},
    ['3'] = {0x1e, 0x01, 0x0e, 0x01, 0x1e},
    ['4'] = {0x12, 0x12, 0x1f, 0x02, 0x02},
    ['5'] = {0x1f, 0x10, 0x1e, 0x01, 0x1e},
    ['6'] = {0x0e, 0x10, 0x1e, 0x11, 0x1e},
    ['7'] = {0x1f, 0x01, 0x02, 0x04, 0x08},
    ['8'] = {0x1e, 0x11, 0x1e, 0x11, 0x1e},
    ['9'] = {0x1e, 0x11, 0x1f, 0x01, 0x0e},
    ['A'] = {0x0e, 0x11, 0x1f, 0x11, 0x11},
    ['B'] = {0x1e, 0x11, 0x1e, 0x11, 0x1e},
    ['C'] = {0x0f, 0x10, 0x10, 0x10, 0x0f},
    ['D'] = {0x1e, 0x11, 0x11, 0x11, 0x1e},
    ['E'] = {0x1f, 0x10, 0x1e, 0x10, 0x1f},
    ['F'] = {0x1f, 0x10, 0x1e, 0x10, 0x10},
    ['G'] = {0x0f, 0x10, 0x17, 0x11, 0x0f},
    ['H'] = {0x11, 0x11, 0x1f, 0x11, 0x11},
    ['I'] = {0x0e, 0x04, 0x04, 0x04, 0x0e},
    ['J'] = {0x01, 0x01, 0x01, 0x11, 0x0e},
    ['K'] = {0x11, 0x12, 0x1c, 0x12, 0x11},
    ['L'] = {0x10, 0x10, 0x10, 0x10, 0x1f},
    ['M'] = {0x11, 0x1b, 0x15, 0x11, 0x11},
    ['N'] = {0x11, 0x19, 0x15, 0x13, 0x11},
    ['O'] = {0x0e, 0x11, 0x11, 0x11, 0x0e},
    ['P'] = {0x1e, 0x11, 0x1e, 0x10, 0x10},
    ['Q'] = {0x0e, 0x11, 0x11, 0x13, 0x0f},
    ['R'] = {0x1e, 0x11, 0x1e, 0x12, 0x11},
    ['S'] = {0x0f, 0x10, 0x0e, 0x01, 0x1e},
    ['T'] = {0x1f, 0x04, 0x04, 0x04, 0x04},
    ['U'] = {0x11, 0x11, 0x11, 0x11, 0x0e},
    ['V'] = {0x11, 0x11, 0x11, 0x0a, 0x04},
    ['W'] = {0x11, 0x11, 0x15, 0x1b, 0x11},
    ['X'] = {0x11, 0x0a, 0x04, 0x0a, 0x11},
    ['Y'] = {0x11, 0x0a, 0x04, 0x04, 0x04},
    ['Z'] = {0x1f, 0x02, 0x04, 0x08, 0x1f},
};

static uint64_t now_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        perror("clock_gettime");
        exit(1);
    }
    return (uint64_t)ts.tv_sec * 1000ull + (uint64_t)ts.tv_nsec / 1000000ull;
}

static int memfd_create_compat(const char *name, unsigned int flags) {
    return syscall(SYS_memfd_create, name, flags);
}

static void log_line(const char *kind, const char *message) {
    printf("@@LOG kind=%s %s\n", kind, message);
    fflush(stdout);
}

static void set_redraw_reason(struct app *app, const char *reason) {
    snprintf(app->last_redraw_reason, sizeof(app->last_redraw_reason), "%s", reason);
    app->redraw_pending = true;
}

static uint32_t color(uint8_t r, uint8_t g, uint8_t b) {
    return (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b;
}

static void fill_rect(struct app *app, int x, int y, int w, int h, uint32_t pixel) {
    int max_x = x + w;
    int max_y = y + h;
    uint32_t *dst = app->buffer.data;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (max_x > app->width) max_x = app->width;
    if (max_y > app->height) max_y = app->height;
    for (int yy = y; yy < max_y; ++yy) {
        for (int xx = x; xx < max_x; ++xx) {
            dst[yy * app->width + xx] = pixel;
        }
    }
}

static void draw_char(struct app *app, int x, int y, char c, int scale, uint32_t pixel) {
    const uint8_t *glyph = font5x7[(unsigned char)c];
    for (int col = 0; col < 5; ++col) {
        for (int row = 0; row < 7; ++row) {
            if (glyph[col] & (1u << (6 - row))) {
                fill_rect(app, x + col * scale, y + row * scale, scale, scale, pixel);
            }
        }
    }
}

static void draw_text(struct app *app, int x, int y, const char *text, int scale, uint32_t pixel) {
    for (size_t i = 0; text[i] != '\0'; ++i) {
        char c = text[i];
        if (c >= 'a' && c <= 'z') {
            c = (char)(c - 'a' + 'A');
        }
        draw_char(app, x + (int)i * (6 * scale), y, c, scale, pixel);
    }
}

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

static void render(struct app *app) {
    char line[128];
    uint32_t bg = color(0x12, 0x18, 0x1d);
    uint32_t fg = color(0xe9, 0xee, 0xf2);
    uint32_t accent = color(0x4a, 0xd3, 0x95);
    uint32_t warn = color(0xf0, 0xa2, 0x02);
    uint32_t bad = color(0xe0, 0x50, 0x50);

    if (app->conn_state == CONN_CONNECTED) {
        bg = color(0x14, 0x20, 0x17);
    } else if (app->conn_state == CONN_CONNECTING) {
        bg = color(0x21, 0x1b, 0x12);
    } else {
        bg = color(0x24, 0x14, 0x16);
    }

    fill_rect(app, 0, 0, app->width, app->height, bg);
    fill_rect(app, 24, 24, app->width - 48, app->height - 48, color(0x0d, 0x11, 0x15));
    fill_rect(app, 24, 24, app->width - 48, 56, color(0x20, 0x2a, 0x33));
    draw_text(app, 42, 42, "QEMU PHASE 2 WAYLAND DEMO", 3, fg);

    snprintf(line, sizeof(line), "STATE: %s", conn_state_name(app->conn_state));
    draw_text(app, 48, 110, line, 3, app->conn_state == CONN_CONNECTED ? accent : (app->conn_state == CONN_CONNECTING ? warn : bad));

    snprintf(line, sizeof(line), "PACKETS: %" PRIu64, app->packets_received);
    draw_text(app, 48, 160, line, 3, fg);

    snprintf(line, sizeof(line), "KEY EVENTS: %" PRIu64, app->key_events);
    draw_text(app, 48, 210, line, 3, fg);

    snprintf(line, sizeof(line), "POINTER EVENTS: %" PRIu64, app->pointer_events);
    draw_text(app, 48, 260, line, 3, fg);

    snprintf(line, sizeof(line), "LAST KEY: %s", app->last_key[0] ? app->last_key : "NONE");
    draw_text(app, 48, 310, line, 2, fg);

    snprintf(line, sizeof(line), "LAST POINTER: %s", app->last_pointer[0] ? app->last_pointer : "NONE");
    draw_text(app, 48, 350, line, 2, fg);

    snprintf(line, sizeof(line), "REDRAW: %s", app->last_redraw_reason);
    draw_text(app, 48, 390, line, 2, fg);

    snprintf(line, sizeof(line), "UPTIME MS: %" PRIu64, now_ms() - app->started_ms);
    draw_text(app, 48, 430, line, 2, fg);

    snprintf(line, sizeof(line), "PACKET AGE MS: %" PRIu64, app->last_packet_ms ? now_ms() - app->last_packet_ms : 0);
    draw_text(app, 48, 460, line, 2, fg);

    fill_rect(app, app->pointer_x - 4, app->pointer_y - 4, 8, 8, accent);

    wl_surface_attach(app->surface, app->buffer.wl_buffer, 0, 0);
    wl_surface_damage_buffer(app->surface, 0, 0, app->width, app->height);
    wl_surface_commit(app->surface);
    wl_display_flush(app->display);
    app->frame_counter++;
    app->redraw_pending = false;
}

static int create_shm_file(size_t size) {
    int fd = memfd_create_compat("wl-sleepdemo-buffer", MFD_CLOEXEC);
    if (fd < 0) {
        perror("memfd_create");
        exit(1);
    }
    if (ftruncate(fd, (off_t)size) != 0) {
        perror("ftruncate");
        exit(1);
    }
    return fd;
}

static void create_buffer(struct app *app) {
    int fd;
    struct wl_shm_pool *pool;

    app->width = 1280;
    app->height = 800;
    app->buffer.width = app->width;
    app->buffer.height = app->height;
    app->buffer.stride = app->width * 4;
    app->buffer.size = (size_t)app->buffer.stride * (size_t)app->height;

    fd = create_shm_file(app->buffer.size);
    app->buffer.data = mmap(NULL, app->buffer.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (app->buffer.data == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    pool = wl_shm_create_pool(app->shm, fd, (int)app->buffer.size);
    app->buffer.wl_buffer = wl_shm_pool_create_buffer(
        pool, 0, app->width, app->height, app->buffer.stride, WL_SHM_FORMAT_XRGB8888);
    wl_shm_pool_destroy(pool);
    close(fd);
}

static void handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
    struct app *app = data;
    (void)version;

    if (strcmp(interface, wl_compositor_interface.name) == 0) {
        app->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 4);
    } else if (strcmp(interface, wl_shm_interface.name) == 0) {
        app->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        app->wm_base = wl_registry_bind(registry, name, &xdg_wm_base_interface, 1);
    } else if (strcmp(interface, wl_seat_interface.name) == 0) {
        app->seat = wl_registry_bind(registry, name, &wl_seat_interface, 5);
    }
}

static void handle_global_remove(void *data, struct wl_registry *registry, uint32_t name) {
    (void)data;
    (void)registry;
    (void)name;
}

static const struct wl_registry_listener registry_listener = {
    .global = handle_global,
    .global_remove = handle_global_remove,
};

static void handle_wm_base_ping(void *data, struct xdg_wm_base *base, uint32_t serial) {
    (void)data;
    xdg_wm_base_pong(base, serial);
}

static const struct xdg_wm_base_listener wm_base_listener = {
    .ping = handle_wm_base_ping,
};

static void handle_xdg_surface_configure(void *data, struct xdg_surface *surface, uint32_t serial) {
    struct app *app = data;
    xdg_surface_ack_configure(surface, serial);
    app->configured = true;
    set_redraw_reason(app, "configure");
}

static const struct xdg_surface_listener xdg_surface_listener = {
    .configure = handle_xdg_surface_configure,
};

static void handle_toplevel_configure(void *data, struct xdg_toplevel *toplevel, int32_t width, int32_t height, struct wl_array *states) {
    (void)data;
    (void)toplevel;
    (void)width;
    (void)height;
    (void)states;
}

static void handle_toplevel_close(void *data, struct xdg_toplevel *toplevel) {
    struct app *app = data;
    (void)toplevel;
    app->running = false;
}

static const struct xdg_toplevel_listener toplevel_listener = {
    .configure = handle_toplevel_configure,
    .close = handle_toplevel_close,
    .configure_bounds = NULL,
    .wm_capabilities = NULL,
};

static void handle_keyboard_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int fd, uint32_t size) {
    (void)data;
    (void)keyboard;
    (void)format;
    (void)size;
    if (fd >= 0) {
        close(fd);
    }
}

static void handle_keyboard_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys) {
    (void)keyboard;
    (void)serial;
    (void)surface;
    (void)keys;
    struct app *app = data;
    snprintf(app->last_key, sizeof(app->last_key), "FOCUS");
    set_redraw_reason(app, "keyboard-enter");
}

static void handle_keyboard_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface) {
    (void)keyboard;
    (void)serial;
    (void)surface;
    struct app *app = data;
    snprintf(app->last_key, sizeof(app->last_key), "LEAVE");
    set_redraw_reason(app, "keyboard-leave");
}

static void handle_keyboard_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time_value, uint32_t key, uint32_t state) {
    (void)keyboard;
    (void)serial;
    (void)time_value;
    struct app *app = data;
    app->key_events++;
    snprintf(app->last_key, sizeof(app->last_key), "KEY=%u STATE=%u", key, state);
    set_redraw_reason(app, "keyboard");
    log_line("input", app->last_key);
}

static void handle_keyboard_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {
    (void)data;
    (void)keyboard;
    (void)serial;
    (void)mods_depressed;
    (void)mods_latched;
    (void)mods_locked;
    (void)group;
}

static void handle_keyboard_repeat(void *data, struct wl_keyboard *keyboard, int32_t rate, int32_t delay) {
    (void)data;
    (void)keyboard;
    (void)rate;
    (void)delay;
}

static const struct wl_keyboard_listener keyboard_listener = {
    .keymap = handle_keyboard_keymap,
    .enter = handle_keyboard_enter,
    .leave = handle_keyboard_leave,
    .key = handle_keyboard_key,
    .modifiers = handle_keyboard_modifiers,
    .repeat_info = handle_keyboard_repeat,
};

static void handle_pointer_enter(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy) {
    (void)pointer;
    (void)serial;
    (void)surface;
    struct app *app = data;
    app->pointer_x = wl_fixed_to_int(sx);
    app->pointer_y = wl_fixed_to_int(sy);
    snprintf(app->last_pointer, sizeof(app->last_pointer), "ENTER %d,%d", app->pointer_x, app->pointer_y);
    set_redraw_reason(app, "pointer-enter");
}

static void handle_pointer_leave(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface) {
    (void)pointer;
    (void)serial;
    (void)surface;
    struct app *app = data;
    snprintf(app->last_pointer, sizeof(app->last_pointer), "LEAVE");
    set_redraw_reason(app, "pointer-leave");
}

static void handle_pointer_motion(void *data, struct wl_pointer *pointer, uint32_t time_value, wl_fixed_t sx, wl_fixed_t sy) {
    (void)pointer;
    (void)time_value;
    struct app *app = data;
    app->pointer_x = wl_fixed_to_int(sx);
    app->pointer_y = wl_fixed_to_int(sy);
    app->pointer_events++;
    snprintf(app->last_pointer, sizeof(app->last_pointer), "MOVE %d,%d", app->pointer_x, app->pointer_y);
    set_redraw_reason(app, "pointer-motion");
}

static void handle_pointer_button(void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time_value, uint32_t button, uint32_t state) {
    (void)pointer;
    (void)serial;
    (void)time_value;
    struct app *app = data;
    app->pointer_events++;
    snprintf(app->last_pointer, sizeof(app->last_pointer), "BUTTON %u STATE %u", button, state);
    set_redraw_reason(app, "pointer-button");
    log_line("input", app->last_pointer);
}

static void handle_pointer_axis(void *data, struct wl_pointer *pointer, uint32_t time_value, uint32_t axis, wl_fixed_t value) {
    (void)data;
    (void)pointer;
    (void)time_value;
    (void)axis;
    (void)value;
}

static void handle_pointer_frame(void *data, struct wl_pointer *pointer) {
    (void)data;
    (void)pointer;
}

static void handle_pointer_axis_source(void *data, struct wl_pointer *pointer, uint32_t source) {
    (void)data;
    (void)pointer;
    (void)source;
}

static void handle_pointer_axis_stop(void *data, struct wl_pointer *pointer, uint32_t time_value, uint32_t axis) {
    (void)data;
    (void)pointer;
    (void)time_value;
    (void)axis;
}

static void handle_pointer_axis_discrete(void *data, struct wl_pointer *pointer, uint32_t axis, int32_t discrete) {
    (void)data;
    (void)pointer;
    (void)axis;
    (void)discrete;
}

static void handle_pointer_axis_value120(void *data, struct wl_pointer *pointer, uint32_t axis, int32_t value120) {
    (void)data;
    (void)pointer;
    (void)axis;
    (void)value120;
}

static void handle_pointer_axis_relative_direction(void *data, struct wl_pointer *pointer, uint32_t axis, uint32_t direction) {
    (void)data;
    (void)pointer;
    (void)axis;
    (void)direction;
}

static const struct wl_pointer_listener pointer_listener = {
    .enter = handle_pointer_enter,
    .leave = handle_pointer_leave,
    .motion = handle_pointer_motion,
    .button = handle_pointer_button,
    .axis = handle_pointer_axis,
    .frame = handle_pointer_frame,
    .axis_source = handle_pointer_axis_source,
    .axis_stop = handle_pointer_axis_stop,
    .axis_discrete = handle_pointer_axis_discrete,
    .axis_value120 = handle_pointer_axis_value120,
    .axis_relative_direction = handle_pointer_axis_relative_direction,
};

static void handle_seat_capabilities(void *data, struct wl_seat *seat, uint32_t capabilities) {
    struct app *app = data;

    if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && !app->keyboard) {
        app->keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(app->keyboard, &keyboard_listener, app);
    }

    if ((capabilities & WL_SEAT_CAPABILITY_POINTER) && !app->pointer) {
        app->pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(app->pointer, &pointer_listener, app);
    }
}

static void handle_seat_name(void *data, struct wl_seat *seat, const char *name) {
    (void)data;
    (void)seat;
    (void)name;
}

static const struct wl_seat_listener seat_listener = {
    .capabilities = handle_seat_capabilities,
    .name = handle_seat_name,
};

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        return -1;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

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

static void attempt_connect(struct app *app) {
    struct sockaddr_in addr = {0};
    int fd;
    int rc;
    struct epoll_event ev = {0};

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

static void handle_socket_event(struct app *app, uint32_t events) {
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
        set_redraw_reason(app, "packet");
    }
}

static void setup_wayland(struct app *app) {
    app->display = wl_display_connect(NULL);
    if (!app->display) {
        perror("wl_display_connect");
        exit(1);
    }

    app->registry = wl_display_get_registry(app->display);
    wl_registry_add_listener(app->registry, &registry_listener, app);
    wl_display_roundtrip(app->display);
    wl_display_roundtrip(app->display);

    if (!app->compositor || !app->shm || !app->wm_base) {
        fprintf(stderr, "missing required wayland globals\n");
        exit(1);
    }

    xdg_wm_base_add_listener(app->wm_base, &wm_base_listener, app);
    if (app->seat) {
        wl_seat_add_listener(app->seat, &seat_listener, app);
        wl_display_roundtrip(app->display);
    }

    app->surface = wl_compositor_create_surface(app->compositor);
    app->xdg_surface = xdg_wm_base_get_xdg_surface(app->wm_base, app->surface);
    xdg_surface_add_listener(app->xdg_surface, &xdg_surface_listener, app);
    app->xdg_toplevel = xdg_surface_get_toplevel(app->xdg_surface);
    xdg_toplevel_add_listener(app->xdg_toplevel, &toplevel_listener, app);
    xdg_toplevel_set_title(app->xdg_toplevel, "Phase 2 Wayland Demo");

    create_buffer(app);
    wl_surface_commit(app->surface);
    wl_display_roundtrip(app->display);
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
