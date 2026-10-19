// SPDX-FileCopyrightText: 2024 KylinSoft Co., Ltd.
//
// SPDX-License-Identifier: GPL-1.0-or-later

#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <string.h>

#include <kywc/log.h>

#include "server.h"
#include "util/dbus.h"
#include "util/string.h"

#define DBUS_RECONNECT_INTERVAL 1000
#define DBUS_RECONNECT_MAX_ATTEMPTS 10

struct dbus_object {
    struct wl_list link;
    sd_bus_slot *slot;
    /* kept to register again when the user bus is reconnected */
    char *name, *path, *interface;
    const sd_bus_vtable *vtable;
    void *data;
};

/* matches are floating slots owned by the bus, kept to add again on reconnect */
struct dbus_match {
    struct wl_list link;
    bool system;
    char *match;
    char *sender, *path, *interface, *member;
    sd_bus_message_handler_t callback;
    void *data;
};

struct dbus_context {
    struct wl_event_loop *event_loop;
    /* system bus */
    struct sd_bus *system;
    struct wl_event_source *system_event;
    /* user bus */
    struct sd_bus *user;
    struct wl_event_source *user_event;

    /* buses closed by the other side, e.g. startdde restarts dbus.service on login */
    bool system_lost, user_lost;
    struct wl_event_source *reconnect_timer;
    int reconnect_attempts;

    struct wl_list objects;
    struct wl_list matches;
    struct wl_listener display_destroy;
    struct wl_listener server_destroy;
};

static struct dbus_context *context = NULL;

static char *strdup_or_null(const char *str)
{
    return str ? strdup(str) : NULL;
}

static void dbus_disconnect(bool system)
{
    if (system) {
        wl_event_source_remove(context->system_event);
        context->system_event = NULL;
        context->system = sd_bus_close_unref(context->system);
        context->system_lost = true;
    } else {
        struct dbus_object *object;
        wl_list_for_each(object, &context->objects, link) {
            object->slot = sd_bus_slot_unref(object->slot);
        }
        wl_event_source_remove(context->user_event);
        context->user_event = NULL;
        context->user = sd_bus_close_unref(context->user);
        context->user_lost = true;
    }

    context->reconnect_attempts = 0;
    wl_event_source_timer_update(context->reconnect_timer, DBUS_RECONNECT_INTERVAL);
}

static int dbus_process(int fd, uint32_t mask, void *data)
{
    struct sd_bus *bus = data;
    while (sd_bus_process(bus, NULL) > 0) {
        ;
    }

    /* the fd of a closed bus stays readable, keeping it in the loop makes the loop spin */
    if ((mask & (WL_EVENT_HANGUP | WL_EVENT_ERROR)) || sd_bus_is_open(bus) <= 0) {
        bool system = bus == context->system;
        kywc_log(KYWC_WARN, "Disconnected from %s bus", system ? "system" : "user");
        dbus_disconnect(system);
    }
    return 0;
}

static bool dbus_connect_system(void)
{
    int ret = sd_bus_open_system(&context->system);
    if (ret < 0) {
        kywc_log(KYWC_WARN, "Failed to connect to system bus: %s", strerror(-ret));
        return false;
    }

    int fd = sd_bus_get_fd(context->system);
    context->system_event = wl_event_loop_add_fd(context->event_loop, fd, WL_EVENT_READABLE,
                                                 dbus_process, context->system);
    wl_event_source_check(context->system_event);
    return true;
}

static bool dbus_connect_user(void)
{
    int ret = sd_bus_open_user(&context->user);
    if (ret < 0) {
        kywc_log(KYWC_WARN, "Failed to connect to user bus: %s", strerror(-ret));
        return false;
    }

    ret = sd_bus_request_name(context->user, "com.kylin.Wlcom", 0);
    if (ret < 0) {
        kywc_log(KYWC_ERROR, "Failed to acquire service name: %s", strerror(-ret));
        if (ret == -EEXIST) {
            kywc_log(KYWC_ERROR, "Is a flakewm already running?");
        }
        context->user = sd_bus_flush_close_unref(context->user);
        return false;
    }

    int fd = sd_bus_get_fd(context->user);
    context->user_event = wl_event_loop_add_fd(context->event_loop, fd, WL_EVENT_READABLE,
                                               dbus_process, context->user);
    wl_event_source_check(context->user_event);
    return true;
}

static void dbus_add_matches(bool system)
{
    struct sd_bus *bus = system ? context->system : context->user;
    struct dbus_match *match;
    wl_list_for_each(match, &context->matches, link) {
        if (match->system != system) {
            continue;
        }
        int ret = match->match ? sd_bus_add_match(bus, NULL, match->match, match->callback,
                                                  match->data)
                               : sd_bus_match_signal(bus, NULL, match->sender, match->path,
                                                     match->interface, match->member,
                                                     match->callback, match->data);
        if (ret < 0) {
            kywc_log(KYWC_ERROR, "Failed to add match again: %s",
                     match->match ? match->match : match->member);
        }
    }
}

static void dbus_register_objects(void)
{
    struct dbus_object *object;
    wl_list_for_each(object, &context->objects, link) {
        if (object->name) {
            sd_bus_request_name_async(context->user, NULL, object->name, 0, NULL, NULL);
        }
        if (object->vtable) {
            int ret = sd_bus_add_object_vtable(context->user, &object->slot, object->path,
                                               object->interface, object->vtable, object->data);
            if (ret < 0) {
                kywc_log(KYWC_ERROR, "Failed to register D-Bus interface %s at %s again: %s",
                         object->interface, object->path, strerror(-ret));
            }
        }
    }
}

static int dbus_reconnect(void *data)
{
    if (context->system_lost && dbus_connect_system()) {
        context->system_lost = false;
        dbus_add_matches(true);
        kywc_log(KYWC_INFO, "Reconnected to system bus");
    }
    if (context->user_lost && dbus_connect_user()) {
        context->user_lost = false;
        dbus_register_objects();
        dbus_add_matches(false);
        kywc_log(KYWC_INFO, "Reconnected to user bus");
    }

    if (!context->system_lost && !context->user_lost) {
        return 0;
    }

    if (++context->reconnect_attempts < DBUS_RECONNECT_MAX_ATTEMPTS) {
        wl_event_source_timer_update(context->reconnect_timer, DBUS_RECONNECT_INTERVAL);
    } else {
        kywc_log(KYWC_ERROR, "Failed to reconnect to D-Bus, giving up");
    }
    return 0;
}

static void dbus_match_destroy(struct dbus_match *match)
{
    wl_list_remove(&match->link);
    free(match->match);
    free(match->sender);
    free(match->path);
    free(match->interface);
    free(match->member);
    free(match);
}

static void dbus_match_record(bool system, const char *match_str, const char *sender,
                              const char *path, const char *interface, const char *member,
                              sd_bus_message_handler_t callback, void *data)
{
    struct dbus_match *match = calloc(1, sizeof(*match));
    if (!match) {
        return;
    }

    match->system = system;
    match->match = strdup_or_null(match_str);
    match->sender = strdup_or_null(sender);
    match->path = strdup_or_null(path);
    match->interface = strdup_or_null(interface);
    match->member = strdup_or_null(member);
    match->callback = callback;
    match->data = data;
    wl_list_insert(&context->matches, &match->link);
}

static void handle_server_destroy(struct wl_listener *listener, void *data)
{
    wl_list_remove(&context->server_destroy.link);

    struct dbus_object *object, *tmp;
    wl_list_for_each_safe(object, tmp, &context->objects, link) {
        dbus_unregister_object(object);
    }

    struct dbus_match *match, *tmp_match;
    wl_list_for_each_safe(match, tmp_match, &context->matches, link) {
        dbus_match_destroy(match);
    }

    sd_bus_flush_close_unref(context->system);
    sd_bus_flush_close_unref(context->user);

    free(context);
    context = NULL;
}

static void handle_display_destroy(struct wl_listener *listener, void *data)
{
    wl_list_remove(&context->display_destroy.link);

    if (context->user_event) {
        wl_event_source_remove(context->user_event);
    }
    if (context->system_event) {
        wl_event_source_remove(context->system_event);
    }
    if (context->reconnect_timer) {
        wl_event_source_remove(context->reconnect_timer);
    }
}

bool dbus_context_create(struct server *server)
{
    context = calloc(1, sizeof(*context));
    if (!context) {
        return false;
    }

    context->event_loop = server->event_loop;
    wl_list_init(&context->objects);
    wl_list_init(&context->matches);

    dbus_connect_system();
    dbus_connect_user();

    if (!context->system && !context->user) {
        kywc_log(KYWC_ERROR, "Failed to init system and user bus");
        free(context);
        context = NULL;
        return false;
    }

    context->reconnect_timer = wl_event_loop_add_timer(server->event_loop, dbus_reconnect, NULL);

    context->display_destroy.notify = handle_display_destroy;
    wl_display_add_destroy_listener(server->display, &context->display_destroy);
    context->server_destroy.notify = handle_server_destroy;
    server_add_destroy_listener(server, &context->server_destroy);

    return true;
}

struct dbus_object *dbus_register_object(const char *name, const char *path, const char *interface,
                                         const sd_bus_vtable *vtable, void *data)
{
    if (!context || !context->user) {
        return NULL;
    }

    struct dbus_object *object = calloc(1, sizeof(*object));
    if (!object) {
        return NULL;
    }

    bool name_acquired = false;
    if (name) {
        int ret = sd_bus_request_name(context->user, name, 0);
        if (ret < 0 && ret != -EALREADY) {
            kywc_log(KYWC_ERROR,
                "(DBus) Accquire: Failed to acquire D-Bus service name %s. %s",
                name, strerror(-ret));
            free(object);
            return NULL;
        }

        name_acquired = ret > 0;
    }

    if (path && interface && vtable) {
        int ret =
            sd_bus_add_object_vtable(context->user, &object->slot, path, interface, vtable, data);
        if (ret < 0) {
            kywc_log(KYWC_ERROR,
                "(DBus) Register: Failed to register D-Bus interface %s at %s. %s",
                interface, path, strerror(-ret));
            if (name_acquired) {
                sd_bus_release_name(context->user, name);
            }
            free(object);
            return NULL;
        }

        object->path = strdup_or_null(path);
        object->interface = strdup_or_null(interface);
        object->vtable = vtable;
    }

    object->name = strdup_or_null(name);
    object->data = data;
    wl_list_insert(&context->objects, &object->link);
    return object;
}

void dbus_unregister_object(struct dbus_object *object)
{
    if (!object) {
        return;
    }

    sd_bus_slot_unref(object->slot);
    wl_list_remove(&object->link);
    free(object->name);
    free(object->path);
    free(object->interface);
    free(object);
}

bool dbus_call_method(const char *service, const char *path, const char *interface,
                      const char *method, sd_bus_message_handler_t callback, void *data)
{
    if (!context || !context->user) {
        return false;
    }

    return sd_bus_call_method_async(context->user, NULL, service, path, interface, method, callback,
                                    data, NULL) >= 0;
}

bool dbus_call_system_method(const char *service, const char *path, const char *interface,
                             const char *method, sd_bus_message_handler_t callback, void *data)
{
    if (!context || !context->system) {
        return false;
    }

    return sd_bus_call_method_async(context->system, NULL, service, path, interface, method,
                                    callback, data, NULL) >= 0;
}

/* Fire-and-forget call of a method that takes a single string argument. */
bool dbus_call_method_str(const char *service, const char *path, const char *interface,
                          const char *method, const char *arg)
{
    if (!context || !context->user) {
        return false;
    }

    sd_bus_message *m = NULL;
    int r = sd_bus_message_new_method_call(context->user, &m, service, path, interface, method);
    if (r < 0) {
        kywc_log(KYWC_ERROR, "dbus: failed to create method call %s", method);
        return false;
    }

    r = sd_bus_message_append(m, "s", arg);
    if (r < 0) {
        kywc_log(KYWC_ERROR, "dbus: failed to append argument for %s", method);
        sd_bus_message_unref(m);
        return false;
    }

    r = sd_bus_call_async(context->user, NULL, m, NULL, NULL, 0);
    sd_bus_message_unref(m);
    if (r < 0) {
        kywc_log(KYWC_ERROR, "dbus: failed to call method %s", method);
        return false;
    }
    return true;
}

bool dbus_call_method_str3(const char *service, const char *path, const char *interface,
        const char *method, const char *arg1, const char *arg2,
        const char *arg3) {
    if (!context || !context->user) {
        return false;
    }

    sd_bus_message *m = NULL;
    int r = sd_bus_message_new_method_call(context->user, &m, service, path, interface, method);
    if (r < 0) {
        return false;
    }

    r = sd_bus_message_append(m, "sss", arg1, arg2, arg3);
    if (r < 0) {
        sd_bus_message_unref(m);
        return false;
    }

    r = sd_bus_call_async(context->user, NULL, m, NULL, NULL, 0);
    sd_bus_message_unref(m);
    if (r < 0) {
        kywc_log(KYWC_ERROR, "dbus: failed to call method %s", method);
        return false;
    }
    return true;
}

bool dbus_update_activation_environment(const char *name, const char *value)
{
    if (!context || !context->user || !name || !value) {
        return false;
    }

    int ret = sd_bus_call_method(context->user, "org.freedesktop.DBus", "/org/freedesktop/DBus",
                                 "org.freedesktop.DBus", "UpdateActivationEnvironment", NULL, NULL,
                                 "a{ss}", 1, name, value);
    if (ret < 0) {
        kywc_log(KYWC_WARN, "Failed to update D-Bus activation environment: %s",
                 strerror(-ret));
        return false;
    }

    char *assignment = string_create("%s=%s", name, value);
    if (!assignment) {
        return false;
    }

    ret = sd_bus_call_method(context->user, "org.freedesktop.systemd1",
                             "/org/freedesktop/systemd1", "org.freedesktop.systemd1.Manager",
                             "SetEnvironment", NULL, NULL, "as", 1, assignment);
    free(assignment);

    if (ret < 0) {
        kywc_log(KYWC_WARN, "Failed to update systemd user environment: %s", strerror(-ret));
        return false;
    }

    return true;
}

void dbus_notify(const char *name, const char *summary, const char *body, const char *icon)
{
    if (!context || !context->user) {
        return;
    }

    sd_bus_call_method_async(context->user, NULL, "org.freedesktop.Notifications",
                             "/org/freedesktop/Notifications", "org.freedesktop.Notifications",
                             "Notify", NULL, NULL, "susssasa{sv}i", name, 0, icon ? icon : "",
                             summary, body, 0, 0, 5000);
}

int dbus_add_match(const char *match, sd_bus_message_handler_t callback, void *data)
{
    if (!context || !context->user) {
        return false;
    }

    int ret = sd_bus_add_match(context->user, NULL, match, callback, data);
    if (ret < 0) {
        kywc_log(KYWC_ERROR, "Failed to add user match: %s", match);
        return false;
    }

    dbus_match_record(false, match, NULL, NULL, NULL, NULL, callback, data);
    return true;
}

bool dbus_match_signal(const char *sender, const char *path, const char *interface,
                       const char *member, sd_bus_message_handler_t callback, void *data)
{
    if (!context || !context->user) {
        return false;
    }

    int ret =
        sd_bus_match_signal(context->user, NULL, sender, path, interface, member, callback, data);
    if (ret < 0) {
        kywc_log(KYWC_ERROR, "Failed to add user signal match: %s %s", interface, member);
        return false;
    }

    dbus_match_record(false, NULL, sender, path, interface, member, callback, data);
    return true;
}

bool dbus_match_system_signal(const char *sender, const char *path, const char *interface,
                              const char *member, sd_bus_message_handler_t callback, void *data)
{
    if (!context || !context->system) {
        return false;
    }

    int ret =
        sd_bus_match_signal(context->system, NULL, sender, path, interface, member, callback, data);
    if (ret < 0) {
        kywc_log(KYWC_ERROR, "Failed to add system signal match: %s %s", interface, member);
        return false;
    }

    dbus_match_record(true, NULL, sender, path, interface, member, callback, data);
    return true;
}

void dbus_emit_signal(const char *path, const char *interface, const char *member,
                      const char *types, ...)
{
    if (!context || !context->user) {
        return;
    }

    va_list args;
    va_start(args, types);
    sd_bus_emit_signalv(context->user, path, interface, member, types, args);
    va_end(args);
}
