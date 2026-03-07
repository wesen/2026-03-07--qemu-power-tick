#define _GNU_SOURCE

#include "wl_render.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

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

static int memfd_create_compat(const char *name, unsigned int flags) {
    return syscall(SYS_memfd_create, name, flags);
}

static uint32_t color(uint8_t r, uint8_t g, uint8_t b) {
    return (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b;
}

static void fill_rect(struct app *app, int x, int y, int w, int h, uint32_t pixel) {
    int max_x = x + w;
    int max_y = y + h;
    uint32_t *dst = app->buffer.data;

    if (x < 0) {
        x = 0;
    }
    if (y < 0) {
        y = 0;
    }
    if (max_x > app->width) {
        max_x = app->width;
    }
    if (max_y > app->height) {
        max_y = app->height;
    }
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

void create_buffer(struct app *app) {
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

void render(struct app *app) {
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
    draw_text(app, 48, 110, line, 3,
              app->conn_state == CONN_CONNECTED ? accent :
              (app->conn_state == CONN_CONNECTING ? warn : bad));

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

    snprintf(line, sizeof(line), "PACKET AGE MS: %" PRIu64,
             app->last_packet_ms ? now_ms() - app->last_packet_ms : 0);
    draw_text(app, 48, 460, line, 2, fg);

    fill_rect(app, app->pointer_x - 4, app->pointer_y - 4, 8, 8, accent);

    wl_surface_attach(app->surface, app->buffer.wl_buffer, 0, 0);
    wl_surface_damage_buffer(app->surface, 0, 0, app->width, app->height);
    wl_surface_commit(app->surface);
    wl_display_flush(app->display);
    app->frame_counter++;
    app->redraw_pending = false;
}
