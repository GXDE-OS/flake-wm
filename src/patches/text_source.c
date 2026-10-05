// SPDX-License-Identifier: GPL-1.0-or-later
#define _POSIX_C_SOURCE 200809L
#include "text_source.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <wlr/types/wlr_data_device.h>

/* wlroots maps text/plain to X11 TEXT. Qt's X11 DnD receiver decodes that
 * target as Latin-1, regardless of the returned property's type. Advertise
 * the UTF-8 alias before the drag starts, and translate it back on send and
 * accept: never ask a Wayland client for a MIME type it didn't offer.
 *
 * Keep the original source object (and all of its existing lifetime listeners).
 * The public impl is decorated, not its private client/resource layout. During
 * callbacks restore its implementation identity, which client sources assert.
 * The destroy listener restores it permanently before wlroots frees the MIME
 * array and invokes the original destroy implementation.
 */
static const char utf8[] = "text/plain;charset=utf-8";
struct text_source {
    struct wl_list link;
    struct wlr_data_source *source;
    const struct wlr_data_source_impl *original;
    struct wlr_data_source_impl impl;
    struct wl_listener destroy;
};
static struct wl_list sources = { &sources, &sources };

static struct text_source *find(struct wlr_data_source *source)
{
    struct text_source *text;
    wl_list_for_each(text, &sources, link) {
        if (text->source == source)
            return text;
    }
    return NULL;
}

static void restore(struct wlr_data_source *source)
{
    /* A callback is permitted to destroy its source. Do not dereference the
     * old adapter/source after the callback unless it is still registered. */
    struct text_source *text = find(source);
    if (text)
        source->impl = &text->impl;
}

static void send(struct wlr_data_source *source, const char *mime, int32_t fd)
{
    struct text_source *text = find(source);
    assert(text);
    source->impl = text->original;
    source->impl->send(source, !strcmp(mime, utf8) ? "text/plain" : mime, fd);
    restore(source);
}

static void accept(struct wlr_data_source *source, uint32_t serial, const char *mime)
{
    struct text_source *text = find(source);
    assert(text);
    source->impl = text->original;
    source->impl->accept(source, serial, mime && !strcmp(mime, utf8) ? "text/plain" : mime);
    restore(source);
}

static void drop(struct wlr_data_source *source)
{
    struct text_source *text = find(source);
    assert(text);
    source->impl = text->original;
    source->impl->dnd_drop(source);
    restore(source);
}

static void finish(struct wlr_data_source *source)
{
    struct text_source *text = find(source);
    assert(text);
    source->impl = text->original;
    source->impl->dnd_finish(source);
    restore(source);
}

static void action(struct wlr_data_source *source, enum wl_data_device_manager_dnd_action value)
{
    struct text_source *text = find(source);
    assert(text);
    source->impl = text->original;
    source->impl->dnd_action(source, value);
    restore(source);
}

static void destroyed(struct wl_listener *listener, void *data)
{
    struct text_source *text = wl_container_of(listener, text, destroy);
    text->source->impl = text->original;
    wl_list_remove(&text->destroy.link);
    wl_list_remove(&text->link);
    free(text);
}

void ky_text_source_add_utf8(struct wlr_data_source *source)
{
    if (!source || find(source))
        return;
    bool plain = false;
    char **mime;
    wl_array_for_each(mime, &source->mime_types) {
        if (!strcmp(*mime, utf8))
            return;
        if (!strcmp(*mime, "text/plain"))
            plain = true;
    }
    if (!plain)
        return;
    struct text_source *text = calloc(1, sizeof(*text));
    char *alias = strdup(utf8);
    if (!text || !alias) {
        free(text);
        free(alias);
        return;
    }
    mime = wl_array_add(&source->mime_types, sizeof(*mime));
    if (!mime) {
        free(text);
        free(alias);
        return;
    }
    *mime = alias;
    text->source = source;
    text->original = source->impl;
    text->impl = *source->impl;
    text->impl.send = send;
    if (text->impl.accept)
        text->impl.accept = accept;
    if (text->impl.dnd_drop)
        text->impl.dnd_drop = drop;
    if (text->impl.dnd_finish)
        text->impl.dnd_finish = finish;
    if (text->impl.dnd_action)
        text->impl.dnd_action = action;
    text->destroy.notify = destroyed;
    /* Run first so existing destroy listeners observe the original identity. */
    wl_list_insert(&source->events.destroy.listener_list, &text->destroy.link);
    wl_list_insert(&sources, &text->link);
    source->impl = &text->impl;
}
