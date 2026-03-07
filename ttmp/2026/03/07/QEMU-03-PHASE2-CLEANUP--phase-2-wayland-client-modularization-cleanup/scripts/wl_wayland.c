#define _GNU_SOURCE

#include "wl_wayland.h"

#include "wl_render.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void handle_global(void *data, struct wl_registry *registry, uint32_t name,
                          const char *interface, uint32_t version) {
    struct app *app = data;
    char msg[256];

    snprintf(msg, sizeof(msg), "global name=%u interface=%s version=%u", name, interface, version);
    log_line("state", msg);

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

static void handle_toplevel_configure(void *data, struct xdg_toplevel *toplevel, int32_t width,
                                      int32_t height, struct wl_array *states) {
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

static void handle_keyboard_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format,
                                   int fd, uint32_t size) {
    (void)data;
    (void)keyboard;
    (void)format;
    (void)size;
    if (fd >= 0) {
        close(fd);
    }
}

static void handle_keyboard_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial,
                                  struct wl_surface *surface, struct wl_array *keys) {
    struct app *app = data;

    (void)keyboard;
    (void)serial;
    (void)surface;
    (void)keys;
    snprintf(app->last_key, sizeof(app->last_key), "FOCUS");
    set_redraw_reason(app, "keyboard-enter");
}

static void handle_keyboard_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial,
                                  struct wl_surface *surface) {
    struct app *app = data;

    (void)keyboard;
    (void)serial;
    (void)surface;
    snprintf(app->last_key, sizeof(app->last_key), "LEAVE");
    set_redraw_reason(app, "keyboard-leave");
}

static void handle_keyboard_key(void *data, struct wl_keyboard *keyboard, uint32_t serial,
                                uint32_t time_value, uint32_t key, uint32_t state) {
    struct app *app = data;

    (void)keyboard;
    (void)serial;
    (void)time_value;
    app->key_events++;
    snprintf(app->last_key, sizeof(app->last_key), "KEY=%u STATE=%u", key, state);
    set_redraw_reason(app, "keyboard");
    log_line("input", app->last_key);
}

static void handle_keyboard_modifiers(void *data, struct wl_keyboard *keyboard, uint32_t serial,
                                      uint32_t mods_depressed, uint32_t mods_latched,
                                      uint32_t mods_locked, uint32_t group) {
    (void)data;
    (void)keyboard;
    (void)serial;
    (void)mods_depressed;
    (void)mods_latched;
    (void)mods_locked;
    (void)group;
}

static void handle_keyboard_repeat(void *data, struct wl_keyboard *keyboard, int32_t rate,
                                   int32_t delay) {
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

static void handle_pointer_enter(void *data, struct wl_pointer *pointer, uint32_t serial,
                                 struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy) {
    struct app *app = data;

    (void)pointer;
    (void)serial;
    (void)surface;
    app->pointer_x = wl_fixed_to_int(sx);
    app->pointer_y = wl_fixed_to_int(sy);
    snprintf(app->last_pointer, sizeof(app->last_pointer), "ENTER %d,%d", app->pointer_x, app->pointer_y);
    set_redraw_reason(app, "pointer-enter");
}

static void handle_pointer_leave(void *data, struct wl_pointer *pointer, uint32_t serial,
                                 struct wl_surface *surface) {
    struct app *app = data;

    (void)pointer;
    (void)serial;
    (void)surface;
    snprintf(app->last_pointer, sizeof(app->last_pointer), "LEAVE");
    set_redraw_reason(app, "pointer-leave");
}

static void handle_pointer_motion(void *data, struct wl_pointer *pointer, uint32_t time_value,
                                  wl_fixed_t sx, wl_fixed_t sy) {
    struct app *app = data;

    (void)pointer;
    (void)time_value;
    app->pointer_x = wl_fixed_to_int(sx);
    app->pointer_y = wl_fixed_to_int(sy);
    app->pointer_events++;
    snprintf(app->last_pointer, sizeof(app->last_pointer), "MOVE %d,%d", app->pointer_x, app->pointer_y);
    set_redraw_reason(app, "pointer-motion");
}

static void handle_pointer_button(void *data, struct wl_pointer *pointer, uint32_t serial,
                                  uint32_t time_value, uint32_t button, uint32_t state) {
    struct app *app = data;

    (void)pointer;
    (void)serial;
    (void)time_value;
    app->pointer_events++;
    snprintf(app->last_pointer, sizeof(app->last_pointer), "BUTTON %u STATE %u", button, state);
    set_redraw_reason(app, "pointer-button");
    log_line("input", app->last_pointer);
}

static void handle_pointer_axis(void *data, struct wl_pointer *pointer, uint32_t time_value,
                                uint32_t axis, wl_fixed_t value) {
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

static void handle_pointer_axis_stop(void *data, struct wl_pointer *pointer, uint32_t time_value,
                                     uint32_t axis) {
    (void)data;
    (void)pointer;
    (void)time_value;
    (void)axis;
}

static void handle_pointer_axis_discrete(void *data, struct wl_pointer *pointer, uint32_t axis,
                                         int32_t discrete) {
    (void)data;
    (void)pointer;
    (void)axis;
    (void)discrete;
}

static void handle_pointer_axis_value120(void *data, struct wl_pointer *pointer, uint32_t axis,
                                         int32_t value120) {
    (void)data;
    (void)pointer;
    (void)axis;
    (void)value120;
}

static void handle_pointer_axis_relative_direction(void *data, struct wl_pointer *pointer,
                                                   uint32_t axis, uint32_t direction) {
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
    char msg[128];

    snprintf(msg, sizeof(msg), "seat-capabilities=%u", capabilities);
    log_line("state", msg);

    if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && !app->keyboard) {
        app->keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(app->keyboard, &keyboard_listener, app);
        log_line("state", "keyboard-bound");
    } else if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && app->keyboard_fallback_bound) {
        wl_keyboard_destroy(app->keyboard);
        app->keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(app->keyboard, &keyboard_listener, app);
        app->keyboard_fallback_bound = false;
        log_line("state", "keyboard-rebound");
    }

    if ((capabilities & WL_SEAT_CAPABILITY_POINTER) && !app->pointer) {
        app->pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(app->pointer, &pointer_listener, app);
        log_line("state", "pointer-bound");
    } else if ((capabilities & WL_SEAT_CAPABILITY_POINTER) && app->pointer_fallback_bound) {
        wl_pointer_destroy(app->pointer);
        app->pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(app->pointer, &pointer_listener, app);
        app->pointer_fallback_bound = false;
        log_line("state", "pointer-rebound");
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

void setup_wayland(struct app *app) {
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
        log_line("state", "seat-listener-added");
        wl_seat_add_listener(app->seat, &seat_listener, app);
        wl_display_roundtrip(app->display);
        if (!app->keyboard) {
            app->keyboard = wl_seat_get_keyboard(app->seat);
            wl_keyboard_add_listener(app->keyboard, &keyboard_listener, app);
            app->keyboard_fallback_bound = true;
            log_line("state", "keyboard-bound-fallback");
        }
        if (!app->pointer) {
            app->pointer = wl_seat_get_pointer(app->seat);
            wl_pointer_add_listener(app->pointer, &pointer_listener, app);
            app->pointer_fallback_bound = true;
            log_line("state", "pointer-bound-fallback");
        }
    } else {
        log_line("state", "seat-missing");
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
