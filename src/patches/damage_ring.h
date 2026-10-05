// SPDX-License-Identifier: GPL-1.0-or-later
#ifndef KY_DAMAGE_RING_H
#define KY_DAMAGE_RING_H

#include <stdbool.h>
#include <pixman.h>
#include <wayland-server-core.h>

struct wlr_buffer;

/* The compositor paints in output-local logical coordinates, unlike wlroots'
 * buffer-local damage ring. Track buffer identity rather than removed ages. */
struct ky_damage_ring {
    pixman_region32_t current;
    struct wl_list buffers;
    int width, height;
};

void ky_damage_ring_init(struct ky_damage_ring *ring);
void ky_damage_ring_finish(struct ky_damage_ring *ring);
void ky_damage_ring_set_bounds(struct ky_damage_ring *ring, int width, int height);
bool ky_damage_ring_add(struct ky_damage_ring *ring, const pixman_region32_t *damage);
void ky_damage_ring_add_whole(struct ky_damage_ring *ring);
void ky_damage_ring_get_buffer_damage(struct ky_damage_ring *ring, struct wlr_buffer *buffer,
                                     pixman_region32_t *damage);
void ky_damage_ring_rotate_buffer(struct ky_damage_ring *ring, struct wlr_buffer *buffer);

#endif
