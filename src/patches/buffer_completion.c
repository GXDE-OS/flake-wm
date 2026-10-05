// SPDX-License-Identifier: GPL-1.0-or-later
#include "buffer_completion.h"
#include <stdlib.h>
#include <wlr/render/drm_syncobj.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/util/log.h>
#include <xf86drm.h>

struct buffer_completion {
    struct wlr_buffer *buffer;
    struct wlr_drm_syncobj_timeline *timeline;
    uint64_t point;
    struct wl_display *display;
    struct wlr_drm_syncobj_timeline_waiter waiter;
    struct wl_listener loop_destroy;
};

static void completion_finish(struct buffer_completion *completion, bool ready)
{
    wl_list_remove(&completion->loop_destroy.link);
    wlr_drm_syncobj_timeline_waiter_finish(&completion->waiter);
    wlr_drm_syncobj_timeline_unref(completion->timeline);
    if (ready) {
        wlr_buffer_unlock(completion->buffer);
    } else {
        /* Fail closed during process shutdown. Dropping this last reference
         * would let wlroots signal the client's release before GPU completion.
         * The kernel reclaims the quarantined buffer when the process exits. */
        wlr_log(WLR_ERROR, "Unfinished GPU buffer quarantined until process exit");
    }
    free(completion);
}

static void completion_ready(struct wlr_drm_syncobj_timeline_waiter *waiter)
{
    struct buffer_completion *completion = wl_container_of(waiter, completion, waiter);
    /* wlroots also invokes the callback on an eventfd read/error failure. */
    bool ready = false;
    if (!wlr_drm_syncobj_timeline_check(completion->timeline, completion->point,
            DRM_SYNCOBJ_WAIT_FLAGS_WAIT_FOR_SUBMIT, &ready) || !ready) {
        wl_display_terminate(completion->display);
        ready = false;
    }
    completion_finish(completion, ready);
}

static void completion_loop_destroy(struct wl_listener *listener, void *data)
{
    struct buffer_completion *completion = wl_container_of(listener, completion, loop_destroy);
    bool ready = false;
    if (!wlr_drm_syncobj_timeline_check(completion->timeline, completion->point,
            DRM_SYNCOBJ_WAIT_FLAGS_WAIT_FOR_SUBMIT, &ready)) {
        ready = false;
    }
    completion_finish(completion, ready);
}

bool ky_buffer_hold_until(struct wlr_buffer *buffer,
    struct wlr_drm_syncobj_timeline *timeline, uint64_t point, struct wl_display *display)
{
    struct wl_event_loop *loop = wl_display_get_event_loop(display);
    struct wlr_client_buffer *client = wlr_client_buffer_get(buffer);
    if (client) {
        buffer = client->source;
    }
    if (!buffer) {
        return false;
    }
    bool ready = false;
    if (wlr_drm_syncobj_timeline_check(timeline, point,
            DRM_SYNCOBJ_WAIT_FLAGS_WAIT_FOR_SUBMIT, &ready) && ready) {
        return true;
    }
    /* Lock before allocation: even OOM must not turn an outstanding read into
     * a premature release. The caller exits on false, without unlocking. */
    wlr_buffer_lock(buffer);
    struct buffer_completion *completion = calloc(1, sizeof(*completion));
    if (!completion) {
        return false;
    }
    /* EVENTFD uses flags=0 for actual signal; WAIT_FOR_SUBMIT is a WAIT ioctl
     * flag and is rejected by EVENTFD. No WAIT_AVAILABLE: materialization is
     * not completion. EVENTFD naturally supports not-yet-submitted points. */
    if (!wlr_drm_syncobj_timeline_waiter_init(&completion->waiter, timeline, point,
            0, loop, completion_ready)) {
        free(completion);
        return false;
    }
    completion->buffer = buffer;
    completion->timeline = wlr_drm_syncobj_timeline_ref(timeline);
    completion->point = point;
    completion->display = display;
    completion->loop_destroy.notify = completion_loop_destroy;
    wl_event_loop_add_destroy_listener(loop, &completion->loop_destroy);
    return true;
}
