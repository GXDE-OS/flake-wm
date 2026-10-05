// SPDX-License-Identifier: GPL-1.0-or-later
#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <xf86drm.h>
#include <wlr/types/wlr_linux_dmabuf_v1.h>
#include <kywc/log.h>
#include "linux_dmabuf.h"

struct vmware_dmabuf_check {
    int fd;
    struct wl_listener destroy;
};

static bool check_import(struct wlr_dmabuf_attributes *attribs, void *data)
{
    struct vmware_dmabuf_check *check = data;
    for (int i = 0; i < attribs->n_planes; i++) {
        uint32_t handle;
        if (drmPrimeFDToHandle(check->fd, attribs->fd[i], &handle) != 0) {
            return false;
        }
        /* open-kylin 9b1de2a1a: vmwgfx can import successfully but reject the
         * GEM close ioctl. Preserve that workaround without accepting failed
         * imports, and without weakening validation on unrelated drivers. */
        if (drmCloseBufferHandle(check->fd, handle) != 0) {
            kywc_log(KYWC_DEBUG, "vmwgfx: GEM handle close failed after successful DMA-BUF import");
        }
    }
    return true;
}

static void handle_destroy(struct wl_listener *listener, void *data)
{
    struct vmware_dmabuf_check *check = wl_container_of(listener, check, destroy);
    wl_list_remove(&check->destroy.link);
    close(check->fd);
    free(check);
}

bool ky_linux_dmabuf_init_vmware_check(struct wlr_linux_dmabuf_v1 *dmabuf, int renderer_fd)
{
    if (renderer_fd < 0) {
        return true;
    }
    drmVersion *version = drmGetVersion(renderer_fd);
    bool vmware = version && version->name && strcmp(version->name, "vmwgfx") == 0;
    drmFreeVersion(version);
    if (!vmware) {
        return true;
    }

    char *node = drmGetRenderDeviceNameFromFd(renderer_fd);
    if (!node) {
        return true; // wlroots skips this check when there is no render node.
    }
    /* A dup of the renderer fd shares its GEM handle namespace. Opening a
     * separate file description avoids closing a handle owned by rendering. */
    int fd = open(node, O_RDWR | O_CLOEXEC);
    free(node);
    if (fd < 0) {
        return false;
    }
    struct vmware_dmabuf_check *check = calloc(1, sizeof(*check));
    if (!check) {
        close(fd);
        return false;
    }
    check->fd = fd;
    check->destroy.notify = handle_destroy;
    wl_signal_add(&dmabuf->events.destroy, &check->destroy);
    wlr_linux_dmabuf_v1_set_check_dmabuf_callback(dmabuf, check_import, check);
    return true;
}
