// SPDX-License-Identifier: GPL-1.0-or-later
/* Resource-lifetime fixture for sandboxes which deny SO_PEERCRED. The event
 * loop, pipes, timers and writer under test still use the real implementation. */
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <wayland-server-core.h>

struct fixture_resource {
    struct wl_signal destroy;
};
static struct wl_display *display;
static int client_fd;

struct wl_client *__wrap_wl_client_create(struct wl_display *owner, int fd)
{
    display = owner;
    client_fd = fd;
    return (struct wl_client *)owner;
}
void __wrap_wl_client_destroy(struct wl_client *client)
{
    close(client_fd);
}
struct wl_display *__wrap_wl_client_get_display(struct wl_client *client)
{
    return display;
}
struct wl_client *__wrap_wl_resource_get_client(struct wl_resource *resource)
{
    return (struct wl_client *)display;
}
struct wl_resource *__wrap_wl_resource_create(struct wl_client *client,
                                              const struct wl_interface *interface, int version,
                                              uint32_t id)
{
    struct fixture_resource *resource = calloc(1, sizeof(*resource));
    assert(resource);
    wl_signal_init(&resource->destroy);
    return (struct wl_resource *)resource;
}
void __wrap_wl_resource_add_destroy_listener(struct wl_resource *resource,
                                             struct wl_listener *listener)
{
    wl_signal_add(&((struct fixture_resource *)resource)->destroy, listener);
}
struct wl_listener *__wrap_wl_resource_get_destroy_listener(struct wl_resource *resource,
                                                            wl_notify_func_t notify)
{
    return wl_signal_get(&((struct fixture_resource *)resource)->destroy, notify);
}
void __wrap_wl_resource_destroy(struct wl_resource *resource)
{
    wl_signal_emit_mutable(&((struct fixture_resource *)resource)->destroy, resource);
    free(resource);
}
