// SPDX-License-Identifier: GPL-1.0-or-later
#include <stdlib.h>
#include <wlr/types/wlr_buffer.h>
#include "damage_ring.h"

struct damage_buffer {
    struct wlr_buffer *buffer;
    pixman_region32_t damage;
    struct wl_listener destroy;
    struct wl_list link;
};

static void buffer_destroy(struct wl_listener *listener, void *data)
{
    struct damage_buffer *entry = wl_container_of(listener, entry, destroy);
    wl_list_remove(&entry->destroy.link);
    wl_list_remove(&entry->link);
    pixman_region32_fini(&entry->damage);
    free(entry);
}

void ky_damage_ring_init(struct ky_damage_ring *ring)
{
    *ring = (struct ky_damage_ring){0};
    wl_list_init(&ring->buffers);
    pixman_region32_init(&ring->current);
}

void ky_damage_ring_finish(struct ky_damage_ring *ring)
{
    struct damage_buffer *entry, *tmp;
    wl_list_for_each_safe(entry, tmp, &ring->buffers, link) {
        buffer_destroy(&entry->destroy, NULL);
    }
    pixman_region32_fini(&ring->current);
}

bool ky_damage_ring_add(struct ky_damage_ring *ring, const pixman_region32_t *damage)
{
    pixman_region32_t clipped;
    pixman_region32_init(&clipped);
    pixman_region32_intersect_rect(&clipped, damage, 0, 0, ring->width, ring->height);
    bool changed = pixman_region32_not_empty(&clipped);
    pixman_region32_union(&ring->current, &ring->current, &clipped);
    struct damage_buffer *entry;
    wl_list_for_each(entry, &ring->buffers, link) {
        pixman_region32_union(&entry->damage, &entry->damage, &clipped);
    }
    pixman_region32_fini(&clipped);
    return changed;
}

void ky_damage_ring_add_whole(struct ky_damage_ring *ring)
{
    pixman_region32_t damage;
    pixman_region32_init_rect(&damage, 0, 0, ring->width, ring->height);
    ky_damage_ring_add(ring, &damage);
    pixman_region32_fini(&damage);
}

void ky_damage_ring_set_bounds(struct ky_damage_ring *ring, int width, int height)
{
    if (ring->width != width || ring->height != height) {
        ring->width = width;
        ring->height = height;
        ky_damage_ring_add_whole(ring);
    }
}

void ky_damage_ring_get_buffer_damage(struct ky_damage_ring *ring, struct wlr_buffer *buffer,
                                     pixman_region32_t *damage)
{
    struct damage_buffer *entry;
    wl_list_for_each(entry, &ring->buffers, link) {
        if (entry->buffer == buffer) {
            pixman_region32_intersect_rect(damage, &entry->damage, 0, 0, ring->width, ring->height);
            return;
        }
    }
    pixman_region32_clear(damage);
    pixman_region32_union_rect(damage, damage, 0, 0, ring->width, ring->height);
    entry = calloc(1, sizeof(*entry));
    if (!entry) {
        return; // An untracked buffer is always repainted in full.
    }
    entry->buffer = buffer;
    pixman_region32_init_rect(&entry->damage, 0, 0, ring->width, ring->height);
    entry->destroy.notify = buffer_destroy;
    wl_signal_add(&buffer->events.destroy, &entry->destroy);
    wl_list_insert(&ring->buffers, &entry->link);
}

void ky_damage_ring_rotate_buffer(struct ky_damage_ring *ring, struct wlr_buffer *buffer)
{
    struct damage_buffer *entry;
    wl_list_for_each(entry, &ring->buffers, link) {
        if (entry->buffer == buffer) {
            pixman_region32_clear(&entry->damage);
            break;
        }
    }
    pixman_region32_clear(&ring->current);
}
