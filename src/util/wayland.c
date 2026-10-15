// SPDX-FileCopyrightText: 2023 The wlroots contributors
// SPDX-FileCopyrightText: 2023 KylinSoft Co., Ltd.
//
// SPDX-License-Identifier: GPL-1.0-or-later

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "util/wayland.h"

/**
 * stolen from wlroots.
 */

struct destroy_global_data {
    struct wl_global *global;
    struct wl_event_source *event_source;
    struct wl_listener display_destroy;
};

static void destroy_global(struct destroy_global_data *data)
{
    wl_list_remove(&data->display_destroy.link);
    wl_global_destroy(data->global);
    wl_event_source_remove(data->event_source);
    free(data);
}

static int handle_timer_event(void *data)
{
    destroy_global(data);
    return 0;
}

static void handle_display_destroy(struct wl_listener *listener, void *_data)
{
    struct destroy_global_data *data = wl_container_of(listener, data, display_destroy);
    destroy_global(data);
}

void wl_global_destroy_safe(struct wl_global *global)
{
    // Don't destroy the global immediately. If the global has been created
    // recently, clients might try to bind to it after we've destroyed it.
    // Instead, remove the global so that clients stop seeing it and wait an
    // arbitrary amount of time before destroying the global as a workaround.
    // See: https://gitlab.freedesktop.org/wayland/wayland/issues/10

    wl_global_remove(global);
    wl_global_set_user_data(global, NULL); // safety net

    struct wl_display *display = wl_global_get_display(global);
    struct wl_event_loop *event_loop = wl_display_get_event_loop(display);
    struct destroy_global_data *data = calloc(1, sizeof(*data));
    if (data == NULL) {
        wl_global_destroy(global);
        return;
    }
    data->global = global;
    data->event_source = wl_event_loop_add_timer(event_loop, handle_timer_event, data);
    if (data->event_source == NULL) {
        free(data);
        wl_global_destroy(global);
        return;
    }
    wl_event_source_timer_update(data->event_source, 5000);

    data->display_destroy.notify = handle_display_destroy;
    wl_display_add_destroy_listener(display, &data->display_destroy);
}

void wl_signal_emit_oneshot(struct wl_signal *signal, void *data)
{
    struct wl_list *listeners = &signal->listener_list;
    if (wl_list_empty(listeners)) {
        return;
    }

    struct wl_listener *listener = wl_container_of(listeners->next, listener, link);
    listener->notify(listener, data);
}

struct resource_writer {
    int fd;
    char *data;
    size_t size, offset;
    struct wl_event_source *writable, *timer;
    struct wl_listener destroy;
};

static void resource_writer_destroy(struct resource_writer *writer)
{
    if (writer->writable) {
        wl_event_source_remove(writer->writable);
    }
    if (writer->timer) {
        wl_event_source_remove(writer->timer);
    }
    wl_list_remove(&writer->destroy.link);
    close(writer->fd);
    free(writer->data);
    free(writer);
}

static void resource_writer_handle_destroy(struct wl_listener *listener, void *data)
{
    struct resource_writer *writer = wl_container_of(listener, writer, destroy);
    resource_writer_destroy(writer);
}

static int resource_writer_timeout(void *data)
{
    resource_writer_destroy(data);
    return 0;
}

static int resource_writer_flush(int fd, uint32_t mask, void *data)
{
    struct resource_writer *writer = data;
    if (mask & (WL_EVENT_HANGUP | WL_EVENT_ERROR)) {
        resource_writer_destroy(writer);
        return 0;
    }
    while (writer->offset < writer->size) {
        ssize_t n = write(fd, writer->data + writer->offset, writer->size - writer->offset);
        if (n > 0) {
            writer->offset += n;
        } else if (n < 0 && errno == EINTR) {
            continue;
        } else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            return 1;
        } else {
            break;
        }
    }
    resource_writer_destroy(writer);
    return 0;
}

void wl_resource_write_owned_data(struct wl_resource *resource, int fd, void *data, size_t size)
{
    struct wl_listener *previous =
        wl_resource_get_destroy_listener(resource, resource_writer_handle_destroy);
    if (previous) {
        resource_writer_handle_destroy(previous, NULL);
    }

    int flags = fcntl(fd, F_GETFL);
    if (flags < 0 || fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        close(fd);
        free(data);
        return;
    }
    struct resource_writer *writer = calloc(1, sizeof(*writer));
    if (!writer) {
        close(fd);
        free(data);
        return;
    }
    writer->fd = fd;
    writer->data = data;
    writer->size = size;
    writer->destroy.notify = resource_writer_handle_destroy;
    wl_resource_add_destroy_listener(resource, &writer->destroy);

    if (!resource_writer_flush(fd, 0, writer)) {
        return;
    }

    struct wl_display *display = wl_client_get_display(wl_resource_get_client(resource));
    struct wl_event_loop *loop = wl_display_get_event_loop(display);
    writer->writable =
        wl_event_loop_add_fd(loop, fd, WL_EVENT_WRITABLE, resource_writer_flush, writer);
    writer->timer = wl_event_loop_add_timer(loop, resource_writer_timeout, writer);
    if (!writer->writable || !writer->timer ||
        wl_event_source_timer_update(writer->timer, 5000) < 0) {
        resource_writer_destroy(writer);
        return;
    }
}
