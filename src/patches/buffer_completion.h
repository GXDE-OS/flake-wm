// SPDX-License-Identifier: GPL-1.0-or-later
#ifndef KY_BUFFER_COMPLETION_H
#define KY_BUFFER_COMPLETION_H
#include <stdbool.h>
#include <stdint.h>
struct wlr_buffer;
struct wlr_drm_syncobj_timeline;
struct wl_display;
/* Hold the raw source until actual completion, not merely fence availability.
 * On failure the lock is quarantined: the caller must terminate its display.
 * Never unlock a failed/unknown completion and thereby signal early release. */
bool ky_buffer_hold_until(struct wlr_buffer *buffer,
    struct wlr_drm_syncobj_timeline *timeline, uint64_t point, struct wl_display *display);
#endif
