#!/bin/sh
# The CMake install already supplies a relative RUNPATH and only the fallback
# libraries actually selected by this build. Keep them private to flakewm.
set -eu

: > debian/shlibs.local
for source_dir in debian/tmp/usr/lib/flakewm debian/tmp/usr/lib/*/flakewm; do
    [ -d "$source_dir" ] || continue
    relative_dir=${source_dir#debian/tmp/}
    dh_install -pflakewm "$relative_dir/*.so.*" "$relative_dir"
    for soname in libwayland-server.so.0 libwayland-client.so.0 libdrm.so.2 libpixman-1.so.0 libxkbcommon.so.0; do
        [ -e "$source_dir/$soname" ] || continue
        printf '%s %s flakewm\n' "${soname%%.so.*}" "${soname#*.so.}" >> debian/shlibs.local
    done
done
