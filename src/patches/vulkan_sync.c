// SPDX-License-Identifier: GPL-1.0-or-later
#include <stdlib.h>
#include <unistd.h>
#include <wayland-server-core.h>
#include <wlr/config.h>
#include <wlr/render/drm_syncobj.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/util/log.h>
#include "vulkan_sync.h"

#if WLR_HAS_VULKAN_RENDERER
#include <wlr/render/vulkan.h>

struct completion {
    VkDevice device;
    VkSemaphore semaphore;
    int fd;
    struct wl_event_source *source;
    struct wl_listener renderer_destroy;
    struct wl_listener loop_destroy;
};

static void completion_destroy(struct completion *completion)
{
    if (completion->source) {
        wl_event_source_remove(completion->source);
    }
    wl_list_remove(&completion->renderer_destroy.link);
    wl_list_remove(&completion->loop_destroy.link);
    if (completion->fd >= 0) {
        close(completion->fd);
    }
    if (completion->semaphore != VK_NULL_HANDLE) {
        vkDestroySemaphore(completion->device, completion->semaphore, NULL);
    }
    free(completion);
}

static void completion_drain(struct completion *completion)
{
    /* Also safe during shutdown: neither VkDevice nor event loop is gone yet.
     * Device loss permits object destruction, but must not fake a client fence. */
    vkDeviceWaitIdle(completion->device);
    completion_destroy(completion);
}

static void handle_renderer_destroy(struct wl_listener *listener, void *data)
{
    struct completion *completion = wl_container_of(listener, completion, renderer_destroy);
    completion_drain(completion);
}

static void handle_loop_destroy(struct wl_listener *listener, void *data)
{
    struct completion *completion = wl_container_of(listener, completion, loop_destroy);
    completion_drain(completion);
}

static int handle_ready(int fd, uint32_t mask, void *data)
{
    struct completion *completion = data;
    if (mask & (WL_EVENT_ERROR | WL_EVENT_HANGUP)) {
        completion_drain(completion);
    } else {
        /* The exported fence has signalled: the semaphore is no longer used
         * by the queue. Exporting its payload alone did not make destruction safe. */
        completion_destroy(completion);
    }
    return 0;
}

bool ky_vulkan_signal_completion(struct wlr_renderer *renderer,
                                struct wlr_drm_syncobj_timeline *timeline,
                                struct wl_event_loop *loop)
{
    VkDevice device = wlr_vk_renderer_get_device(renderer);
    VkQueue queue;
    /* wlroots 0.20.2 creates one queue, index 0, in its public queue family. */
    vkGetDeviceQueue(device, wlr_vk_renderer_get_queue_family(renderer), 0, &queue);
    struct completion *completion = calloc(1, sizeof(*completion));
    if (!completion) {
        goto wait_idle;
    }
    completion->device = device;
    completion->fd = -1;
    wl_list_init(&completion->renderer_destroy.link);
    wl_list_init(&completion->loop_destroy.link);

    PFN_vkGetSemaphoreFdKHR get_fd =
        (PFN_vkGetSemaphoreFdKHR)vkGetDeviceProcAddr(device, "vkGetSemaphoreFdKHR");
    if (!get_fd) {
        goto fail;
    }
    VkExportSemaphoreCreateInfo export = {
        .sType = VK_STRUCTURE_TYPE_EXPORT_SEMAPHORE_CREATE_INFO,
        .handleTypes = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT,
    };
    VkSemaphoreCreateInfo create = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = &export,
    };
    if (vkCreateSemaphore(device, &create, NULL, &completion->semaphore) != VK_SUCCESS) {
        goto fail;
    }
    VkSubmitInfo submit = {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &completion->semaphore,
    };
    if (vkQueueSubmit(queue, 1, &submit, VK_NULL_HANDLE) != VK_SUCCESS) {
        goto fail;
    }
    VkSemaphoreGetFdInfoKHR get = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_GET_FD_INFO_KHR,
        .semaphore = completion->semaphore,
        .handleType = VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_SYNC_FD_BIT,
    };
    if (get_fd(device, &get, &completion->fd) != VK_SUCCESS) {
        goto fail;
    }
    if (completion->fd < 0) {
        /* SYNC_FD export may return -1 for an already signalled payload. */
        completion_destroy(completion);
        return wlr_drm_syncobj_timeline_signal(timeline, 1);
    }
    completion->source = wl_event_loop_add_fd(loop, completion->fd, WL_EVENT_READABLE,
                                             handle_ready, completion);
    if (!completion->source ||
        !wlr_drm_syncobj_timeline_import_sync_file(timeline, 1, completion->fd)) {
        goto fail;
    }
    completion->renderer_destroy.notify = handle_renderer_destroy;
    wl_signal_add(&renderer->events.destroy, &completion->renderer_destroy);
    completion->loop_destroy.notify = handle_loop_destroy;
    wl_event_loop_add_destroy_listener(loop, &completion->loop_destroy);
    return true;

fail:
    /* A signal submission may already be queued: wait before destroying it. */
    {
        VkResult idle = vkDeviceWaitIdle(device);
        completion_destroy(completion);
        if (idle != VK_SUCCESS) {
            wlr_log(WLR_ERROR, "Vulkan completion failed and device did not become idle");
            return false;
        }
    }
    wlr_log(WLR_ERROR, "Vulkan completion export failed; used synchronous device wait");
    return wlr_drm_syncobj_timeline_signal(timeline, 1);

wait_idle:
    if (vkDeviceWaitIdle(device) != VK_SUCCESS) {
        return false;
    }
    return wlr_drm_syncobj_timeline_signal(timeline, 1);
}
#else
bool ky_vulkan_signal_completion(struct wlr_renderer *renderer,
                                struct wlr_drm_syncobj_timeline *timeline,
                                struct wl_event_loop *loop)
{
    return false;
}
#endif
