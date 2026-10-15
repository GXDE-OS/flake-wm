#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
build_dir=$(mktemp -d /tmp/flake-writer-test.XXXXXX)
trap 'rm -rf "$build_dir"' EXIT HUP INT TERM
fixture=
wrap=
if [ "${1:-}" = --fixture ]; then
    fixture=tests/resource-writer-fixture.c
    for symbol in wl_client_create wl_client_destroy wl_client_get_display wl_resource_get_client wl_resource_create wl_resource_add_destroy_listener wl_resource_get_destroy_listener wl_resource_destroy; do
        wrap="$wrap -Wl,--wrap=$symbol"
    done
fi
# Intentional splitting for compiler flags, pkg-config output and optional fixture.
${CC:-cc} -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter \
    ${CFLAGS:-} -Iinclude tests/resource-writer.c $fixture src/util/wayland.c \
    $(pkg-config --cflags --libs wayland-server) $wrap -o "$build_dir/test"
timeout 20 "$build_dir/test"
printf '%s\n' 'resource writer tests passed'
