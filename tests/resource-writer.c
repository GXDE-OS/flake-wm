// SPDX-License-Identifier: GPL-1.0-or-later
#define _POSIX_C_SOURCE 200809L
#include "util/wayland.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <wayland-server-protocol.h>

static void *payload(size_t size)
{
    unsigned char *data = malloc(size);
    assert(data);
    for (size_t i = 0; i < size; i++) {
        data[i] = i % 251;
    }
    return data;
}

static void start_write(struct wl_resource *resource, int fds[2])
{
    assert(pipe(fds) == 0);
    assert(fcntl(fds[0], F_SETFL, O_NONBLOCK) == 0);
    wl_resource_write_owned_data(resource, fds[1], payload(1024 * 1024), 1024 * 1024);
}

static void assert_closed(int fd)
{
    assert(fcntl(fd, F_GETFD) == -1 && errno == EBADF);
}

int main(void)
{
    signal(SIGPIPE, SIG_IGN);
    struct wl_display *display = wl_display_create();
    assert(display);
    struct wl_event_loop *loop = wl_display_get_event_loop(display);
    int sockets[2];
    assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
    struct wl_client *client = wl_client_create(display, sockets[0]);
    if (!client) {
        perror("wl_client_create");
    }
    assert(client);
    struct wl_resource *resource = wl_resource_create(client, &wl_surface_interface, 1, 0);
    assert(resource);

    // A full pipe must return immediately; later reads get the complete payload.
    int fds[2];
    start_write(resource, fds);
    size_t total = 0;
    for (;;) {
        unsigned char buf[4096];
        ssize_t n = read(fds[0], buf, sizeof(buf));
        if (n == 0) {
            break;
        }
        if (n > 0) {
            for (ssize_t i = 0; i < n; i++) {
                assert(buf[i] == (total + i) % 251);
            }
            total += n;
        } else {
            assert(errno == EAGAIN);
        }
        assert(wl_event_loop_dispatch(loop, 0) >= 0);
    }
    assert(total == 1024 * 1024);
    assert_closed(fds[1]);
    close(fds[0]);

    // Repeated requests cannot retain an unbounded number of pending pipes.
    start_write(resource, fds);
    for (int i = 0; i < 2000; i++) {
        int next[2];
        start_write(resource, next);
        char discarded[4096];
        ssize_t n;
        do {
            n = read(fds[0], discarded, sizeof(discarded));
        } while (n > 0);
        assert(n == 0);
        close(fds[0]);
        memcpy(fds, next, sizeof(fds));
    }
    close(fds[0]);
    assert(wl_event_loop_dispatch(loop, 0) >= 0);
    assert_closed(fds[1]);

    // A reader that never drains the pipe expires without blocking dispatch.
    start_write(resource, fds);
    for (int i = 0; i < 60 && fcntl(fds[1], F_GETFD) >= 0; i++) {
        assert(wl_event_loop_dispatch(loop, 100) >= 0);
    }
    assert_closed(fds[1]);
    close(fds[0]);

    // Regular files need no event source; preserve the complete write.
    char path[] = "/tmp/flake-icon-test-XXXXXX";
    int file = mkstemp(path);
    assert(file >= 0);
    unlink(path);
    int reader = dup(file);
    wl_resource_write_owned_data(resource, file, payload(123), 123);
    assert_closed(file);
    assert(lseek(reader, 0, SEEK_END) == 123);
    close(reader);

    // Failure before registration must still consume the payload and FD.
    wl_resource_write_owned_data(resource, -1, payload(123), 123);
    assert(pipe(fds) == 0);
    close(fds[0]);
    wl_resource_write_owned_data(resource, fds[1], payload(123), 123);
    assert_closed(fds[1]);

    // Resource destruction cancels a blocked transfer and closes its FD.
    start_write(resource, fds);
    wl_resource_destroy(resource);
    assert_closed(fds[1]);
    close(fds[0]);
    wl_client_destroy(client);
    close(sockets[1]);
    wl_display_destroy(display);
    return 0;
}
