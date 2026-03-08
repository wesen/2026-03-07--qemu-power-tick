#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

struct options {
    const char *device_path;
    const char *pattern;
};

struct dumb_buffer {
    uint32_t width;
    uint32_t height;
    uint32_t stride;
    uint32_t handle;
    uint64_t size;
    uint32_t fb_id;
    void *map;
};

static void usage(const char *prog) {
    fprintf(stderr, "usage: %s [--device /dev/dri/card0] [--pattern pre|post]\n", prog);
}

static int parse_args(int argc, char **argv, struct options *opts) {
    opts->device_path = "/dev/dri/card0";
    opts->pattern = "pre";
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--device") == 0 && i + 1 < argc) {
            opts->device_path = argv[++i];
        } else if (strcmp(argv[i], "--pattern") == 0 && i + 1 < argc) {
            opts->pattern = argv[++i];
        } else {
            usage(argv[0]);
            return -1;
        }
    }
    return 0;
}

static drmModeConnector *find_connector(int fd, drmModeRes *resources) {
    for (int i = 0; i < resources->count_connectors; ++i) {
        drmModeConnector *connector = drmModeGetConnector(fd, resources->connectors[i]);
        if (connector == NULL) {
            continue;
        }
        if (connector->connection == DRM_MODE_CONNECTED && connector->count_modes > 0) {
            return connector;
        }
        drmModeFreeConnector(connector);
    }
    return NULL;
}

static drmModeCrtc *find_crtc_for_connector(int fd, drmModeRes *resources, drmModeConnector *connector, uint32_t *crtc_id_out) {
    if (connector->encoder_id != 0) {
        drmModeEncoder *encoder = drmModeGetEncoder(fd, connector->encoder_id);
        if (encoder != NULL) {
            drmModeCrtc *crtc = drmModeGetCrtc(fd, encoder->crtc_id);
            if (crtc != NULL) {
                *crtc_id_out = encoder->crtc_id;
                drmModeFreeEncoder(encoder);
                return crtc;
            }
            drmModeFreeEncoder(encoder);
        }
    }

    for (int i = 0; i < connector->count_encoders; ++i) {
        drmModeEncoder *encoder = drmModeGetEncoder(fd, connector->encoders[i]);
        if (encoder == NULL) {
            continue;
        }
        for (int j = 0; j < resources->count_crtcs; ++j) {
            if ((encoder->possible_crtcs & (1U << j)) == 0) {
                continue;
            }
            drmModeCrtc *crtc = drmModeGetCrtc(fd, resources->crtcs[j]);
            if (crtc != NULL) {
                *crtc_id_out = resources->crtcs[j];
                drmModeFreeEncoder(encoder);
                return crtc;
            }
        }
        drmModeFreeEncoder(encoder);
    }

    return NULL;
}

static drmModeModeInfo choose_mode(drmModeConnector *connector, drmModeCrtc *crtc) {
    if (crtc != NULL && crtc->mode_valid) {
        return crtc->mode;
    }
    return connector->modes[0];
}

static uint32_t pixel_for_pattern(const char *pattern, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
    bool left = x < width / 2;
    bool top = y < height / 2;
    if (strcmp(pattern, "post") == 0) {
        if (top && left) {
            return 0x00ffff00;
        }
        if (top && !left) {
            return 0x0000ffff;
        }
        if (!top && left) {
            return 0x00ff00ff;
        }
        return 0x00000000;
    }

    if (top && left) {
        return 0x00ff0000;
    }
    if (top && !left) {
        return 0x0000ff00;
    }
    if (!top && left) {
        return 0x000000ff;
    }
    return 0x00ffffff;
}

static int create_dumb_buffer(int fd, const drmModeModeInfo *mode, struct dumb_buffer *buffer) {
    struct drm_mode_create_dumb create = {0};
    create.width = mode->hdisplay;
    create.height = mode->vdisplay;
    create.bpp = 32;
    if (ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create) != 0) {
        perror("DRM_IOCTL_MODE_CREATE_DUMB");
        return -1;
    }

    buffer->width = create.width;
    buffer->height = create.height;
    buffer->stride = create.pitch;
    buffer->size = create.size;
    buffer->handle = create.handle;

    if (drmModeAddFB(fd, buffer->width, buffer->height, 24, 32, buffer->stride, buffer->handle, &buffer->fb_id) != 0) {
        perror("drmModeAddFB");
        return -1;
    }

    struct drm_mode_map_dumb map = {0};
    map.handle = buffer->handle;
    if (ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map) != 0) {
        perror("DRM_IOCTL_MODE_MAP_DUMB");
        return -1;
    }

    buffer->map = mmap(NULL, buffer->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, map.offset);
    if (buffer->map == MAP_FAILED) {
        buffer->map = NULL;
        perror("mmap");
        return -1;
    }
    return 0;
}

static void fill_pattern(struct dumb_buffer *buffer, const char *pattern) {
    for (uint32_t y = 0; y < buffer->height; ++y) {
        uint32_t *row = (uint32_t *)((uint8_t *)buffer->map + (size_t)y * buffer->stride);
        for (uint32_t x = 0; x < buffer->width; ++x) {
            row[x] = pixel_for_pattern(pattern, x, y, buffer->width, buffer->height);
        }
    }
}

static void destroy_dumb_buffer(int fd, struct dumb_buffer *buffer) {
    if (buffer->map != NULL) {
        munmap(buffer->map, buffer->size);
    }
    if (buffer->fb_id != 0) {
        drmModeRmFB(fd, buffer->fb_id);
    }
    if (buffer->handle != 0) {
        struct drm_mode_destroy_dumb destroy = {0};
        destroy.handle = buffer->handle;
        ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
    }
}

int main(int argc, char **argv) {
    struct options opts;
    if (parse_args(argc, argv, &opts) != 0) {
        return 2;
    }

    int fd = open(opts.device_path, O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        perror("open drm device");
        return 1;
    }

    drmModeRes *resources = drmModeGetResources(fd);
    if (resources == NULL) {
        perror("drmModeGetResources");
        close(fd);
        return 1;
    }

    drmModeConnector *connector = find_connector(fd, resources);
    if (connector == NULL) {
        fprintf(stderr, "no connected connector found\n");
        drmModeFreeResources(resources);
        close(fd);
        return 1;
    }

    uint32_t crtc_id = 0;
    drmModeCrtc *crtc = find_crtc_for_connector(fd, resources, connector, &crtc_id);
    if (crtc == NULL) {
        fprintf(stderr, "no usable CRTC found\n");
        drmModeFreeConnector(connector);
        drmModeFreeResources(resources);
        close(fd);
        return 1;
    }

    drmModeModeInfo mode = choose_mode(connector, crtc);
    struct dumb_buffer buffer = {0};
    if (create_dumb_buffer(fd, &mode, &buffer) != 0) {
        destroy_dumb_buffer(fd, &buffer);
        drmModeFreeCrtc(crtc);
        drmModeFreeConnector(connector);
        drmModeFreeResources(resources);
        close(fd);
        return 1;
    }

    fill_pattern(&buffer, opts.pattern);
    if (drmModeSetCrtc(fd, crtc_id, buffer.fb_id, 0, 0, &connector->connector_id, 1, &mode) != 0) {
        perror("drmModeSetCrtc");
        destroy_dumb_buffer(fd, &buffer);
        drmModeFreeCrtc(crtc);
        drmModeFreeConnector(connector);
        drmModeFreeResources(resources);
        close(fd);
        return 1;
    }

    printf("@@KMSPATTERN device=%s connector_id=%u crtc_id=%u fb_id=%u width=%u height=%u pattern=%s\n",
           opts.device_path,
           connector->connector_id,
           crtc_id,
           buffer.fb_id,
           buffer.width,
           buffer.height,
           opts.pattern);
    fflush(stdout);

    sleep(1);

    destroy_dumb_buffer(fd, &buffer);
    drmModeFreeCrtc(crtc);
    drmModeFreeConnector(connector);
    drmModeFreeResources(resources);
    close(fd);
    return 0;
}
