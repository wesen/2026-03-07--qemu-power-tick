#define _GNU_SOURCE

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define MAX_DEVICES 16

struct probe_device {
    int fd;
    char path[128];
    char name[256];
};

static void add_device(struct probe_device *devices, int *count, const char *path) {
    int fd;
    char name[256] = {0};

    if (*count >= MAX_DEVICES) {
        return;
    }

    fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        return;
    }

    if (ioctl(fd, EVIOCGNAME(sizeof(name)), name) < 0) {
        snprintf(name, sizeof(name), "unknown");
    }

    devices[*count].fd = fd;
    snprintf(devices[*count].path, sizeof(devices[*count].path), "%s", path);
    snprintf(devices[*count].name, sizeof(devices[*count].name), "%s", name);
    printf("@@EVDEV open path=%s name=%s\n", devices[*count].path, devices[*count].name);
    fflush(stdout);
    (*count)++;
}

static void scan_devices(struct probe_device *devices, int *count) {
    DIR *dir;
    struct dirent *entry;

    dir = opendir("/dev/input");
    if (!dir) {
        perror("opendir /dev/input");
        exit(1);
    }

    while ((entry = readdir(dir)) != NULL) {
        char path[160];
        if (strncmp(entry->d_name, "event", 5) != 0) {
            continue;
        }
        snprintf(path, sizeof(path), "/dev/input/%s", entry->d_name);
        add_device(devices, count, path);
    }

    closedir(dir);
}

int main(void) {
    struct probe_device devices[MAX_DEVICES] = {0};
    struct pollfd pfds[MAX_DEVICES];
    int count = 0;

    scan_devices(devices, &count);
    if (count == 0) {
        fprintf(stderr, "no evdev devices found\n");
        return 1;
    }

    for (int i = 0; i < count; ++i) {
        pfds[i].fd = devices[i].fd;
        pfds[i].events = POLLIN;
        pfds[i].revents = 0;
    }

    while (1) {
        int rc = poll(pfds, count, 1000);
        if (rc < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("poll");
            return 1;
        }
        if (rc == 0) {
            continue;
        }

        for (int i = 0; i < count; ++i) {
            struct input_event ev;
            ssize_t n;

            if ((pfds[i].revents & POLLIN) == 0) {
                continue;
            }

            while ((n = read(devices[i].fd, &ev, sizeof(ev))) == (ssize_t)sizeof(ev)) {
                printf("@@EVDEV path=%s type=%u code=%u value=%d sec=%ld usec=%ld\n",
                       devices[i].path,
                       ev.type,
                       ev.code,
                       ev.value,
                       (long)ev.input_event_sec,
                       (long)ev.input_event_usec);
                fflush(stdout);
            }
        }
    }
}
