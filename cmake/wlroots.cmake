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
  wayland-server>=1.23.1
  wayland-client>=1.23.1
  libdrm>=2.4.122
  xkbcommon
  pixman-1>=0.43.0
  egl
  gbm>=21.1
  glesv2
  vulkan>=1.2.182
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

set(WLROOTS_SOURCE_DIR "${PROJECT_SOURCE_DIR}/libs/wlroots")
set(WLROOTS_BUILD_DIR "${PROJECT_BINARY_DIR}/wlroots-static-build")
set(WLROOTS_INSTALL_DIR "${PROJECT_BINARY_DIR}/wlroots-static-install")
set(WLROOTS_LIBRARY
  "${WLROOTS_INSTALL_DIR}/lib/libwlroots-0.19.a"
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
  BUILD_COMMAND
    "${MESON_EXECUTABLE}" compile -C "<BINARY_DIR>"
  INSTALL_COMMAND
    "${MESON_EXECUTABLE}" install -C "<BINARY_DIR>"
  BUILD_ALWAYS TRUE
  BUILD_BYPRODUCTS "${WLROOTS_LIBRARY}"
)

file(MAKE_DIRECTORY "${WLROOTS_INSTALL_DIR}/include/wlroots-0.19")
add_library(wlroots_archive STATIC IMPORTED GLOBAL)
set_target_properties(wlroots_archive PROPERTIES
  IMPORTED_LOCATION "${WLROOTS_LIBRARY}"
  INTERFACE_INCLUDE_DIRECTORIES
    "${WLROOTS_INSTALL_DIR}/include/wlroots-0.19"
)
add_dependencies(wlroots_archive wlroots_external)

add_library(wlroots INTERFACE)
target_compile_definitions(wlroots INTERFACE WLR_USE_UNSTABLE)
target_link_libraries(wlroots INTERFACE
  "$<LINK_LIBRARY:WHOLE_ARCHIVE,wlroots_archive>"
  PkgConfig::WLROOTS_SYSTEM_DEPS
  Threads::Threads
  ${CMAKE_DL_LIBS}
  m
  rt
)

add_library(Wlroots::Wlroots ALIAS wlroots)
