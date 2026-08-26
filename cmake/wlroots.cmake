# Copyright (C) 2026 CharOfString <root@charofstring.cc>
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see <https://www.gnu.org/licenses/>.
# -----------------------------------------------------------------------------
# This file handles Wlroots dependency management. Wlroots is built using Meson
# so this adapter is needed when using CMake.

include_guard(GLOBAL)
include(ExternalProject)

find_program(MESON_EXECUTABLE meson REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)

pkg_check_modules(WLROOTS_SYSTEM_DEPS REQUIRED IMPORTED_TARGET
  wayland-server>=1.24.0
  wayland-client>=1.24.0
  libdrm>=2.4.129
  xkbcommon>=1.8.0
  pixman-1>=0.43.0
  egl
  gbm>=21.1
  glesv2
  lcms2
  libudev
  libseat>=0.2.0
  libdisplay-info>=0.2.0
  libinput>=1.19.0
  xcb
  xcb-composite
  xcb-dri3
  xcb-errors
  xcb-ewmh
  xcb-icccm
  xcb-present
  xcb-render
  xcb-renderutil
  xcb-res
  xcb-shm
  xcb-xfixes>=1.15
  xcb-xinput
)

set(WLROOTS_VERSION 0.20.2)
set(WLROOTS_VERSION_MAJOR 0)
set(WLROOTS_VERSION_MINOR 20)
set(WLROOTS_VERSION_PATCH 2)

# These options match the Meson auto-detected feature set required above and
# are consumed by Waylib's generated public wconfig.h.
set(WLR_HAVE_DRM_BACKEND 1)
set(WLR_HAVE_X11_BACKEND 1)
set(WLR_HAVE_LIBINPUT_BACKEND 1)
set(WLR_HAVE_XWAYLAND 1)
set(WLR_HAVE_GLES2_RENDERER 1)
set(WLR_HAVE_VULKAN_RENDERER 0)
set(WLR_HAVE_GBM_ALLOCATOR 1)
set(WLR_HAVE_UDMABUF_ALLOCATOR 1)
set(WLR_HAVE_SESSION 1)
set(WLR_HAVE_COLOR_MANAGEMENT 1)

set(WLROOTS_SOURCE_DIR "${PROJECT_SOURCE_DIR}/libs/wlroots")
set(WLROOTS_BUILD_DIR "${PROJECT_BINARY_DIR}/wlroots-static-build")
set(WLROOTS_INSTALL_DIR "${PROJECT_BINARY_DIR}/wlroots-static-install")
set(WLROOTS_LIBRARY
  "${WLROOTS_INSTALL_DIR}/lib/libwlroots-0.20.a"
)
set(WLROOTS_PKGCONFIG_DIR
  "${PROJECT_BINARY_DIR}/wlroots-pkgconfig"
)

ExternalProject_Add(wlroots_external
  SOURCE_DIR "${WLROOTS_SOURCE_DIR}"
  BINARY_DIR "${WLROOTS_BUILD_DIR}"
  CONFIGURE_COMMAND
    "${MESON_EXECUTABLE}" setup
    "<BINARY_DIR>"
    "<SOURCE_DIR>"
    "--prefix=${WLROOTS_INSTALL_DIR}"
    "--libdir=lib"
    "--buildtype=debugoptimized"
    "--wrap-mode=nodownload"
    "-Dexamples=false"
    "-Dwerror=false"
    "-Ddefault_library=static"
    "-Drenderers=gles2"
  BUILD_COMMAND
    "${MESON_EXECUTABLE}" compile -C "<BINARY_DIR>"
  INSTALL_COMMAND
    "${MESON_EXECUTABLE}" install -C "<BINARY_DIR>"
  BUILD_ALWAYS TRUE
  BUILD_BYPRODUCTS "${WLROOTS_LIBRARY}"
)

file(MAKE_DIRECTORY
  "${WLROOTS_INSTALL_DIR}/include/wlroots-0.20"
  "${WLROOTS_BUILD_DIR}/protocol"
  "${WLROOTS_PKGCONFIG_DIR}"
)

# Waylib supports standalone wlroots through this pkg-config contract. The
# archive and generated headers are produced by wlroots_external before
# waylibserver is compiled.
file(WRITE "${WLROOTS_PKGCONFIG_DIR}/waylib-wlroots.pc"
"prefix=${WLROOTS_INSTALL_DIR}
exec_prefix=\${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include/wlroots-0.20

Name: waylib-wlroots
Description: FlakeWM vendored upstream wlroots
Version: ${WLROOTS_VERSION}
Requires: wayland-server wayland-client libdrm xkbcommon pixman-1 egl gbm glesv2 lcms2 libudev libseat libdisplay-info libinput xcb xcb-composite xcb-dri3 xcb-errors xcb-ewmh xcb-icccm xcb-present xcb-render xcb-renderutil xcb-res xcb-shm xcb-xfixes xcb-xinput
Cflags: -I\${includedir} -I${WLROOTS_BUILD_DIR}/protocol -DWLR_USE_UNSTABLE
Libs: -L\${libdir} -lwlroots-0.20 -ldl -lm -lrt

wlroots_version=${WLROOTS_VERSION}
wlroots_version_major=${WLROOTS_VERSION_MAJOR}
wlroots_version_minor=${WLROOTS_VERSION_MINOR}
wlroots_version_patch=${WLROOTS_VERSION_PATCH}
wlroots_features=DRM_BACKEND X11_BACKEND LIBINPUT_BACKEND XWAYLAND GLES2_RENDERER GBM_ALLOCATOR UDMABUF_ALLOCATOR SESSION COLOR_MANAGEMENT
")
