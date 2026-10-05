// SPDX-License-Identifier: GPL-1.0-or-later
#ifndef KY_VULKAN_SYNC_H
#define KY_VULKAN_SYNC_H

#include <stdbool.h>
struct wlr_renderer;
struct wlr_drm_syncobj_timeline;
struct wl_event_loop;

/* Append a completion signal after the renderer's submitted queue work.
 * Does not replace wlroots' implicit buffer synchronization. Signals point 1.
 * On export failure, waits for the device before CPU signalling. */
bool ky_vulkan_signal_completion(struct wlr_renderer *renderer,
                                struct wlr_drm_syncobj_timeline *timeline,
                                struct wl_event_loop *loop);

#endif
