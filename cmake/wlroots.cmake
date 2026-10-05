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

# Ninja may rerun CMake without the environment used on the first configure.
# Preserve the explicitly selected dependency prefix across such reruns.
set(WLCOM_DEPENDENCY_PKG_CONFIG_PATH "$ENV{PKG_CONFIG_PATH}" CACHE STRING
  "Dependency pkg-config search path (preserved across automatic reconfigure)")
set(ENV{PKG_CONFIG_PATH} "${WLCOM_DEPENDENCY_PKG_CONFIG_PATH}")

if(NOT DEFINED WLCOM_ROOT_DIR)
  get_filename_component(WLCOM_ROOT_DIR
    "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE
  )
endif()

find_program(MESON_EXECUTABLE meson REQUIRED)
find_package(PkgConfig REQUIRED)
execute_process(COMMAND "${MESON_EXECUTABLE}" --version
  OUTPUT_VARIABLE _wlcom_meson_version OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY)
if(_wlcom_meson_version VERSION_LESS "1.3.0")
  message(FATAL_ERROR "wlroots 0.20.2 requires Meson >= 1.3.0")
endif()

set(WLCOM_WLROOTS_RENDERERS "gles2,vulkan" CACHE STRING "wlroots renderers")
option(WLCOM_XWAYLAND "Build with Xwayland support" ON)
set(WLROOTS_HAVE_XWAYLAND ${WLCOM_XWAYLAND})
if(WLCOM_XWAYLAND)
  set(_wlcom_xwayland_option enabled)
else()
  set(_wlcom_xwayland_option disabled)
endif()

option(WLCOM_ALLOW_VENDORED_WLROOTS_DEPS
  "Allow vendored fallbacks when system wlroots dependencies are too old"
  ON
)

# Keep foundational libraries supplied by the operating system whenever they
# satisfy wlroots. Vendored copies are compatibility fallbacks for old systems,
# not the normal dependency strategy.
foreach(_component WAYLAND_SERVER WAYLAND_CLIENT WAYLAND_SCANNER LIBDRM PIXMAN
                   XKBCOMMON WAYLAND_PROTOCOLS)
  unset(__pkg_config_checked_WLCOM_SYSTEM_${_component} CACHE)
endforeach()
pkg_check_modules(WLCOM_SYSTEM_WAYLAND_SERVER QUIET wayland-server)
pkg_check_modules(WLCOM_SYSTEM_WAYLAND_CLIENT QUIET wayland-client)
pkg_check_modules(WLCOM_SYSTEM_WAYLAND_SCANNER QUIET wayland-scanner)
pkg_check_modules(WLCOM_SYSTEM_LIBDRM QUIET libdrm)
pkg_check_modules(WLCOM_SYSTEM_PIXMAN QUIET pixman-1)
pkg_check_modules(WLCOM_SYSTEM_XKBCOMMON QUIET xkbcommon)
pkg_check_modules(WLCOM_SYSTEM_WAYLAND_PROTOCOLS QUIET wayland-protocols)

# Everything not vendored by this project remains a hard system dependency.
pkg_check_modules(WLROOTS_REQUIRED_SYSTEM_DEPS REQUIRED
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

set(WLROOTS_VENDOR_PREFIX "${CMAKE_BINARY_DIR}/_deps/wlroots-dependencies")
set(WLROOTS_VENDOR_DEPENDENCY_TARGETS "")
set(WLROOTS_VENDOR_PKGCONFIG_DIRS "")
set(WLROOTS_LINK_DIRECTORIES "")
set(WLROOTS_RUNTIME_LIBRARY_DIRS "")

function(_wlcom_vendor_fallback_warning dependency detected required source)
  if(NOT WLCOM_ALLOW_VENDORED_WLROOTS_DEPS)
    message(FATAL_ERROR
      "WLCOM: system ${dependency} is ${detected}; wlroots requires "
      "${required}. Vendored fallback is disabled by "
      "WLCOM_ALLOW_VENDORED_WLROOTS_DEPS=OFF."
    )
  endif()

  message(WARNING
    "WLCOM: system ${dependency} is ${detected}; wlroots requires "
    "${required}. Falling back to ${source}. Vendoring foundational system "
    "libraries is a last-resort compatibility measure. WLCOM will use these "
    "copies only because the system dependency is too old."
  )
endfunction()

set(_wlcom_system_wayland_ok TRUE)
foreach(_component SERVER CLIENT SCANNER)
  if(NOT WLCOM_SYSTEM_WAYLAND_${_component}_FOUND OR
     WLCOM_SYSTEM_WAYLAND_${_component}_VERSION VERSION_LESS "1.24.0")
    set(_wlcom_system_wayland_ok FALSE)
  endif()
endforeach()

if(_wlcom_system_wayland_ok)
  message(STATUS
    "WLCOM: using system Wayland ${WLCOM_SYSTEM_WAYLAND_SERVER_VERSION}"
  )
else()
  string(CONCAT _wlcom_wayland_detected
    "server ${WLCOM_SYSTEM_WAYLAND_SERVER_VERSION}, "
    "client ${WLCOM_SYSTEM_WAYLAND_CLIENT_VERSION}, "
    "scanner ${WLCOM_SYSTEM_WAYLAND_SCANNER_VERSION}"
  )
  _wlcom_vendor_fallback_warning(
    "Wayland"
    "${_wlcom_wayland_detected}"
    ">= 1.24.0"
    "${WLCOM_ROOT_DIR}/libs/wayland"
  )

  set(WLROOTS_VENDOR_WAYLAND_PREFIX "${WLROOTS_VENDOR_PREFIX}/wayland")
  ExternalProject_Add(wlroots_vendor_wayland
    SOURCE_DIR "${WLCOM_ROOT_DIR}/libs/wayland"
    BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/wayland-build"
    CONFIGURE_COMMAND
      "${MESON_EXECUTABLE}" setup
      "<BINARY_DIR>"
      "<SOURCE_DIR>"
      "--prefix=${WLROOTS_VENDOR_WAYLAND_PREFIX}"
      "--libdir=lib"
      "--buildtype=debugoptimized"
      "--wrap-mode=nodownload"
      "-Dtests=false"
      "-Ddocumentation=false"
      "-Ddtd_validation=false"
    BUILD_COMMAND "${MESON_EXECUTABLE}" compile -C "<BINARY_DIR>"
    INSTALL_COMMAND "${MESON_EXECUTABLE}" install -C "<BINARY_DIR>"
    BUILD_ALWAYS TRUE
    BUILD_BYPRODUCTS
      "${WLROOTS_VENDOR_WAYLAND_PREFIX}/lib/libwayland-server.so"
      "${WLROOTS_VENDOR_WAYLAND_PREFIX}/lib/libwayland-client.so"
  )
  list(APPEND WLROOTS_VENDOR_DEPENDENCY_TARGETS wlroots_vendor_wayland)
  list(APPEND WLROOTS_VENDOR_PKGCONFIG_DIRS
    "${WLROOTS_VENDOR_WAYLAND_PREFIX}/lib/pkgconfig"
  )
  list(APPEND WLROOTS_RUNTIME_LIBRARY_DIRS
    "${WLROOTS_VENDOR_WAYLAND_PREFIX}/lib"
  )
endif()

if(WLCOM_SYSTEM_LIBDRM_FOUND AND
   NOT WLCOM_SYSTEM_LIBDRM_VERSION VERSION_LESS "2.4.129")
  message(STATUS
    "WLCOM: using system libdrm ${WLCOM_SYSTEM_LIBDRM_VERSION}"
  )
else()
  _wlcom_vendor_fallback_warning(
    "libdrm"
    "${WLCOM_SYSTEM_LIBDRM_VERSION}"
    ">= 2.4.129"
    "${WLCOM_ROOT_DIR}/libs/libdrm"
  )

  set(WLROOTS_VENDOR_LIBDRM_PREFIX "${WLROOTS_VENDOR_PREFIX}/libdrm")
  ExternalProject_Add(wlroots_vendor_libdrm
    SOURCE_DIR "${WLCOM_ROOT_DIR}/libs/libdrm"
    BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/libdrm-build"
    CONFIGURE_COMMAND
      "${MESON_EXECUTABLE}" setup
      "<BINARY_DIR>"
      "<SOURCE_DIR>"
      "--prefix=${WLROOTS_VENDOR_LIBDRM_PREFIX}"
      "--libdir=lib"
      "--buildtype=debugoptimized"
      "--wrap-mode=nodownload"
      "-Dauto_features=disabled"
      "-Dtests=false"
      "-Dinstall-test-programs=false"
    BUILD_COMMAND "${MESON_EXECUTABLE}" compile -C "<BINARY_DIR>"
    INSTALL_COMMAND "${MESON_EXECUTABLE}" install -C "<BINARY_DIR>"
    BUILD_ALWAYS TRUE
    BUILD_BYPRODUCTS
      "${WLROOTS_VENDOR_LIBDRM_PREFIX}/lib/libdrm.so"
  )
  list(APPEND WLROOTS_VENDOR_DEPENDENCY_TARGETS wlroots_vendor_libdrm)
  list(APPEND WLROOTS_VENDOR_PKGCONFIG_DIRS
    "${WLROOTS_VENDOR_LIBDRM_PREFIX}/lib/pkgconfig"
  )
  list(APPEND WLROOTS_RUNTIME_LIBRARY_DIRS
    "${WLROOTS_VENDOR_LIBDRM_PREFIX}/lib"
  )
endif()

if(WLCOM_SYSTEM_PIXMAN_FOUND AND
   NOT WLCOM_SYSTEM_PIXMAN_VERSION VERSION_LESS "0.46.0")
  message(STATUS
    "WLCOM: using system pixman ${WLCOM_SYSTEM_PIXMAN_VERSION}"
  )
else()
  _wlcom_vendor_fallback_warning(
    "pixman"
    "${WLCOM_SYSTEM_PIXMAN_VERSION}"
    ">= 0.46.0"
    "${WLCOM_ROOT_DIR}/libs/pixman"
  )

  set(WLROOTS_VENDOR_PIXMAN_PREFIX "${WLROOTS_VENDOR_PREFIX}/pixman")
  ExternalProject_Add(wlroots_vendor_pixman
    SOURCE_DIR "${WLCOM_ROOT_DIR}/libs/pixman"
    BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/pixman-build"
    CONFIGURE_COMMAND
      "${MESON_EXECUTABLE}" setup
      "<BINARY_DIR>"
      "<SOURCE_DIR>"
      "--prefix=${WLROOTS_VENDOR_PIXMAN_PREFIX}"
      "--libdir=lib"
      "--buildtype=debugoptimized"
      "--wrap-mode=nodownload"
      "-Dtests=disabled"
      "-Ddemos=disabled"
      "-Dgtk=disabled"
      "-Dlibpng=disabled"
      "-Dopenmp=disabled"
    BUILD_COMMAND "${MESON_EXECUTABLE}" compile -C "<BINARY_DIR>"
    INSTALL_COMMAND "${MESON_EXECUTABLE}" install -C "<BINARY_DIR>"
    BUILD_ALWAYS TRUE
    BUILD_BYPRODUCTS
      "${WLROOTS_VENDOR_PIXMAN_PREFIX}/lib/libpixman-1.so"
  )
  list(APPEND WLROOTS_VENDOR_DEPENDENCY_TARGETS wlroots_vendor_pixman)
  list(APPEND WLROOTS_VENDOR_PKGCONFIG_DIRS
    "${WLROOTS_VENDOR_PIXMAN_PREFIX}/lib/pkgconfig"
  )
  list(APPEND WLROOTS_RUNTIME_LIBRARY_DIRS
    "${WLROOTS_VENDOR_PIXMAN_PREFIX}/lib"
  )
endif()

if(WLCOM_SYSTEM_XKBCOMMON_FOUND AND
   NOT WLCOM_SYSTEM_XKBCOMMON_VERSION VERSION_LESS "1.8.0")
  message(STATUS
    "WLCOM: using system xkbcommon ${WLCOM_SYSTEM_XKBCOMMON_VERSION}"
  )
else()
  _wlcom_vendor_fallback_warning(
    "xkbcommon"
    "${WLCOM_SYSTEM_XKBCOMMON_VERSION}"
    ">= 1.8.0"
    "${WLCOM_ROOT_DIR}/libs/xkbcommon"
  )

  set(WLROOTS_VENDOR_XKBCOMMON_PREFIX "${WLROOTS_VENDOR_PREFIX}/xkbcommon")
  ExternalProject_Add(wlroots_vendor_xkbcommon
    SOURCE_DIR "${WLCOM_ROOT_DIR}/libs/xkbcommon"
    BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/xkbcommon-build"
    CONFIGURE_COMMAND
      "${MESON_EXECUTABLE}" setup
      "<BINARY_DIR>"
      "<SOURCE_DIR>"
      "--prefix=${WLROOTS_VENDOR_XKBCOMMON_PREFIX}"
      "--libdir=lib"
      "--buildtype=debugoptimized"
      "--wrap-mode=nodownload"
      "-Denable-tools=false"
      "-Denable-x11=false"
      "-Denable-wayland=false"
      "-Denable-xkbregistry=false"
      "-Denable-docs=false"
      "-Denable-bash-completion=false"
    # Build only the library target: unlike wayland/pixman there is no
    # `tests` option, and `meson compile` would otherwise build the whole
    # test suite as part of the default target.
    BUILD_COMMAND "${MESON_EXECUTABLE}" compile -C "<BINARY_DIR>" xkbcommon
    INSTALL_COMMAND "${MESON_EXECUTABLE}" install -C "<BINARY_DIR>"
    BUILD_ALWAYS TRUE
    BUILD_BYPRODUCTS
      "${WLROOTS_VENDOR_XKBCOMMON_PREFIX}/lib/libxkbcommon.so"
  )
  list(APPEND WLROOTS_VENDOR_DEPENDENCY_TARGETS wlroots_vendor_xkbcommon)
  list(APPEND WLROOTS_VENDOR_PKGCONFIG_DIRS
    "${WLROOTS_VENDOR_XKBCOMMON_PREFIX}/lib/pkgconfig"
  )
  list(APPEND WLROOTS_RUNTIME_LIBRARY_DIRS
    "${WLROOTS_VENDOR_XKBCOMMON_PREFIX}/lib"
  )
endif()

if(WLCOM_SYSTEM_WAYLAND_PROTOCOLS_FOUND AND
   NOT WLCOM_SYSTEM_WAYLAND_PROTOCOLS_VERSION VERSION_LESS "1.47")
  message(STATUS
    "WLCOM: using system wayland-protocols "
    "${WLCOM_SYSTEM_WAYLAND_PROTOCOLS_VERSION}"
  )
else()
  _wlcom_vendor_fallback_warning(
    "wayland-protocols"
    "${WLCOM_SYSTEM_WAYLAND_PROTOCOLS_VERSION}"
    ">= 1.47"
    "${WLCOM_ROOT_DIR}/libs/wayland-protocols"
  )

  set(WLROOTS_VENDOR_WAYLAND_PROTOCOLS_PREFIX
    "${WLROOTS_VENDOR_PREFIX}/wayland-protocols"
  )
  ExternalProject_Add(wlroots_vendor_wayland_protocols
    SOURCE_DIR "${WLCOM_ROOT_DIR}/libs/wayland-protocols"
    BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/wayland-protocols-build"
    CONFIGURE_COMMAND
      "${MESON_EXECUTABLE}" setup
      "<BINARY_DIR>"
      "<SOURCE_DIR>"
      "--prefix=${WLROOTS_VENDOR_WAYLAND_PROTOCOLS_PREFIX}"
      "--libdir=lib"
      "--buildtype=debugoptimized"
      "--wrap-mode=nodownload"
      "-Dtests=false"
    BUILD_COMMAND "${MESON_EXECUTABLE}" compile -C "<BINARY_DIR>"
    INSTALL_COMMAND "${MESON_EXECUTABLE}" install -C "<BINARY_DIR>"
    BUILD_ALWAYS TRUE
    BUILD_BYPRODUCTS
      "${WLROOTS_VENDOR_WAYLAND_PROTOCOLS_PREFIX}/share/wayland-protocols/stable/xdg-shell/xdg-shell.xml"
  )
  list(APPEND WLROOTS_VENDOR_DEPENDENCY_TARGETS wlroots_vendor_wayland_protocols)

  # wlroots 0.20's public headers (wlr/types/wlr_color_management_v1.h and
  # wlr_color_representation_v1.h) #include <wayland-protocols/*-enum.h>: the
  # enum-only headers wayland-protocols generates with `wayland-scanner
  # enum-header` (scanner >= 1.22.90) and installs under ${includedir}/
  # wayland-protocols. wlroots also generates its own *-protocol.h
  # (server-header) from the same XML; that header re-emits the enum behind
  # #ifndef <ENUM> and, separately, an is_valid() that references the newest
  # enum values. Both guards exist, but they are distinct, so the enum that
  # ends up used is the one from whichever -enum.h is included first.
  #
  # Neither the upstream wayland-protocols.pc.in nor the .pc meson installs
  # here declares an include dir for those -enum.h headers. So when wlroots
  # compiles, <wayland-protocols/color-management-v1-enum.h> resolves to the
  # *system* wayland-protocols (Trixie ships 1.44) via the default /usr/include.
  # That 1.44 enum lacks the trailing 1.47 values, and because the .c files
  # include the public header (hence the -enum.h) before the generated
  # -protocol.h, its #ifndef <ENUM> guard suppresses the enum wlroots generated
  # from the vendored 1.47 XML — while the 1.47 is_valid() still references
  # those values, so the compiler reports them undeclared.
  #
  # Point pkgdatadir at the vendored XML (so *-protocol.h is 1.47) and expose
  # the vendored include dir (so *-enum.h is 1.47 too). PREPEND this .pc so it
  # shadows the meson-installed one.
  set(WLROOTS_VENDOR_WAYLAND_PROTOCOLS_PCDIR
    "${CMAKE_BINARY_DIR}/_deps/wayland-protocols-pkgconfig"
  )
  file(MAKE_DIRECTORY "${WLROOTS_VENDOR_WAYLAND_PROTOCOLS_PCDIR}")
  file(WRITE "${WLROOTS_VENDOR_WAYLAND_PROTOCOLS_PCDIR}/wayland-protocols.pc"
    "prefix=${WLCOM_ROOT_DIR}/libs/wayland-protocols\n"
    "pkgdatadir=${WLCOM_ROOT_DIR}/libs/wayland-protocols\n"
    "\n"
    "Name: Wayland Protocols\n"
    "Description: Wayland protocol files\n"
    "Version: 1.47\n"
    "Cflags: -I${WLROOTS_VENDOR_WAYLAND_PROTOCOLS_PREFIX}/include\n"
  )
  list(PREPEND WLROOTS_VENDOR_PKGCONFIG_DIRS
    "${WLROOTS_VENDOR_WAYLAND_PROTOCOLS_PCDIR}"
  )
  list(APPEND WLROOTS_VENDOR_PKGCONFIG_DIRS
    "${WLROOTS_VENDOR_WAYLAND_PROTOCOLS_PREFIX}/share/pkgconfig"
  )
endif()

set(WLROOTS_VERSION 0.20.2)
set(WLROOTS_VERSION_MAJOR 0)
set(WLROOTS_VERSION_MINOR 20)
set(WLROOTS_VERSION_PATCH 2)

set(WLROOTS_SOURCE_DIR "${WLCOM_ROOT_DIR}/libs/wlroots")
set(WLROOTS_BUILD_DIR "${CMAKE_BINARY_DIR}/_deps/wlroots-build")
set(WLROOTS_INSTALL_DIR "${CMAKE_BINARY_DIR}/_deps/wlroots-install")
set(WLROOTS_LIBRARY
  "${WLROOTS_INSTALL_DIR}/lib/libwlroots-0.20.a"
)
set(WLROOTS_PKGCONFIG_DIR
  "${CMAKE_BINARY_DIR}/_deps/wlroots-pkgconfig"
)

list(JOIN WLROOTS_VENDOR_PKGCONFIG_DIRS ":" _wlroots_pkg_config_path)
if(DEFINED ENV{PKG_CONFIG_PATH} AND NOT "$ENV{PKG_CONFIG_PATH}" STREQUAL "")
  if(_wlroots_pkg_config_path)
    string(APPEND _wlroots_pkg_config_path ":$ENV{PKG_CONFIG_PATH}")
  else()
    set(_wlroots_pkg_config_path "$ENV{PKG_CONFIG_PATH}")
  endif()
endif()

list(JOIN WLROOTS_RUNTIME_LIBRARY_DIRS ":" _wlroots_library_path)
if(DEFINED ENV{LD_LIBRARY_PATH} AND NOT "$ENV{LD_LIBRARY_PATH}" STREQUAL "")
  if(_wlroots_library_path)
    string(APPEND _wlroots_library_path ":$ENV{LD_LIBRARY_PATH}")
  else()
    set(_wlroots_library_path "$ENV{LD_LIBRARY_PATH}")
  endif()
endif()

ExternalProject_Add(wlroots_external
  SOURCE_DIR "${WLROOTS_SOURCE_DIR}"
  BINARY_DIR "${WLROOTS_BUILD_DIR}"
  CONFIGURE_COMMAND
    "${CMAKE_COMMAND}" -E env
    "PKG_CONFIG_PATH=${_wlroots_pkg_config_path}"
    "LD_LIBRARY_PATH=${_wlroots_library_path}"
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
    "-Drenderers=${WLCOM_WLROOTS_RENDERERS}"
    "-Dbackends=drm,libinput,x11"
    "-Dsession=enabled"
    "-Dxwayland=${_wlcom_xwayland_option}"
    "-Dxcb-errors=disabled"
    "-Dlibliftoff=disabled"
    "-Dcolor-management=enabled"
  BUILD_COMMAND
    "${CMAKE_COMMAND}" -E env
    "PKG_CONFIG_PATH=${_wlroots_pkg_config_path}"
    "LD_LIBRARY_PATH=${_wlroots_library_path}"
    "${MESON_EXECUTABLE}" compile -C "<BINARY_DIR>"
  INSTALL_COMMAND
    "${CMAKE_COMMAND}" -E env
    "LD_LIBRARY_PATH=${_wlroots_library_path}"
    "${MESON_EXECUTABLE}" install -C "<BINARY_DIR>"
  BUILD_ALWAYS TRUE
  BUILD_BYPRODUCTS "${WLROOTS_LIBRARY}"
)

if(WLROOTS_VENDOR_DEPENDENCY_TARGETS)
  add_dependencies(wlroots_external ${WLROOTS_VENDOR_DEPENDENCY_TARGETS})
endif()

list(PREPEND WLROOTS_LINK_DIRECTORIES "${WLROOTS_INSTALL_DIR}/lib")

set(WLROOTS_LINK_STUB_DIR "${CMAKE_BINARY_DIR}/_deps/wlroots-link-stubs")
file(MAKE_DIRECTORY "${WLROOTS_LINK_STUB_DIR}")
if(WLROOTS_RUNTIME_LIBRARY_DIRS)
  list(APPEND WLROOTS_LINK_DIRECTORIES "${WLROOTS_LINK_STUB_DIR}")
endif()

file(MAKE_DIRECTORY
  "${WLROOTS_INSTALL_DIR}/include/wlroots-0.20"
  "${WLROOTS_BUILD_DIR}/protocol"
  "${WLROOTS_PKGCONFIG_DIR}"
)

# Provide configure-time metadata for the selected fallbacks. The paths point
# to their eventual install trees, which are populated before wlroots builds.
# Only remove our own generated metadata, never installed/system libraries.
# This also permits switching a build tree back to newer system dependencies.
foreach(_name wayland-server wayland-client libdrm pixman-1 xkbcommon)
  file(REMOVE "${WLROOTS_PKGCONFIG_DIR}/${_name}.pc")
endforeach()
foreach(_name wayland-server wayland-client drm pixman-1 xkbcommon)
  file(REMOVE "${WLROOTS_LINK_STUB_DIR}/lib${_name}.so")
endforeach()
if(NOT _wlcom_system_wayland_ok)
  file(MAKE_DIRECTORY
    "${WLROOTS_VENDOR_WAYLAND_PREFIX}/include"
    "${WLROOTS_VENDOR_WAYLAND_PREFIX}/lib"
  )
  file(WRITE "${WLROOTS_LINK_STUB_DIR}/libwayland-server.so"
    "INPUT(${WLROOTS_VENDOR_WAYLAND_PREFIX}/lib/libwayland-server.so)\n"
  )
  file(WRITE "${WLROOTS_LINK_STUB_DIR}/libwayland-client.so"
    "INPUT(${WLROOTS_VENDOR_WAYLAND_PREFIX}/lib/libwayland-client.so)\n"
  )
  foreach(_wayland_library server client)
    file(WRITE
      "${WLROOTS_PKGCONFIG_DIR}/wayland-${_wayland_library}.pc"
"prefix=${WLROOTS_VENDOR_WAYLAND_PREFIX}
libdir=${WLROOTS_LINK_STUB_DIR}
includedir=\${prefix}/include

Name: Wayland ${_wayland_library}
Description: WLCOM compatibility fallback
Version: 1.26.0
Libs: -L\${libdir} -lwayland-${_wayland_library} -lm
Cflags: -I\${includedir}
")
  endforeach()
endif()

if(NOT (WLCOM_SYSTEM_LIBDRM_FOUND AND
        NOT WLCOM_SYSTEM_LIBDRM_VERSION VERSION_LESS "2.4.129"))
  file(MAKE_DIRECTORY
    "${WLROOTS_VENDOR_LIBDRM_PREFIX}/include/libdrm"
    "${WLROOTS_VENDOR_LIBDRM_PREFIX}/lib"
  )
  file(WRITE "${WLROOTS_LINK_STUB_DIR}/libdrm.so"
    "INPUT(${WLROOTS_VENDOR_LIBDRM_PREFIX}/lib/libdrm.so)\n"
  )
  file(WRITE "${WLROOTS_PKGCONFIG_DIR}/libdrm.pc"
"prefix=${WLROOTS_VENDOR_LIBDRM_PREFIX}
libdir=${WLROOTS_LINK_STUB_DIR}
includedir=\${prefix}/include

Name: libdrm
Description: WLCOM compatibility fallback
Version: 2.4.134
Libs: -L\${libdir} -ldrm
Cflags: -I\${includedir} -I\${includedir}/libdrm
")
endif()

if(NOT (WLCOM_SYSTEM_PIXMAN_FOUND AND
        NOT WLCOM_SYSTEM_PIXMAN_VERSION VERSION_LESS "0.46.0"))
  file(MAKE_DIRECTORY
    "${WLROOTS_VENDOR_PIXMAN_PREFIX}/include/pixman-1"
    "${WLROOTS_VENDOR_PIXMAN_PREFIX}/lib"
  )
  file(WRITE "${WLROOTS_LINK_STUB_DIR}/libpixman-1.so"
    "INPUT(${WLROOTS_VENDOR_PIXMAN_PREFIX}/lib/libpixman-1.so)\n"
  )
  file(WRITE "${WLROOTS_PKGCONFIG_DIR}/pixman-1.pc"
"prefix=${WLROOTS_VENDOR_PIXMAN_PREFIX}
libdir=${WLROOTS_LINK_STUB_DIR}
includedir=\${prefix}/include/pixman-1

Name: Pixman
Description: WLCOM compatibility fallback
Version: 0.46.4
Libs: -L\${libdir} -lpixman-1
Cflags: -I\${includedir}
")
endif()

if(NOT (WLCOM_SYSTEM_XKBCOMMON_FOUND AND
        NOT WLCOM_SYSTEM_XKBCOMMON_VERSION VERSION_LESS "1.8.0"))
  file(MAKE_DIRECTORY
    "${WLROOTS_VENDOR_XKBCOMMON_PREFIX}/include/xkbcommon"
    "${WLROOTS_VENDOR_XKBCOMMON_PREFIX}/lib"
  )
  file(WRITE "${WLROOTS_LINK_STUB_DIR}/libxkbcommon.so"
    "INPUT(${WLROOTS_VENDOR_XKBCOMMON_PREFIX}/lib/libxkbcommon.so)\n"
  )
  file(WRITE "${WLROOTS_PKGCONFIG_DIR}/xkbcommon.pc"
"prefix=${WLROOTS_VENDOR_XKBCOMMON_PREFIX}
libdir=${WLROOTS_LINK_STUB_DIR}
includedir=\${prefix}/include

Name: xkbcommon
Description: WLCOM compatibility fallback
Version: 1.8.0
Libs: -L\${libdir} -lxkbcommon
Cflags: -I\${includedir}
")
endif()

# Expose the Meson-built static archive to CMake through the same pkg-config
# contract that upstream wlroots installs. The archive and generated headers
# are produced by wlroots_external before consumers are linked.
file(WRITE "${WLROOTS_PKGCONFIG_DIR}/wlroots-0.20.pc"
"prefix=${WLROOTS_INSTALL_DIR}
exec_prefix=\${prefix}
libdir=\${prefix}/lib
includedir=\${prefix}/include/wlroots-0.20

Name: wlroots
Description: WLCOM upstream wlroots build
Version: ${WLROOTS_VERSION}
Requires: wayland-server wayland-client libdrm xkbcommon pixman-1 egl gbm glesv2 vulkan lcms2 libudev libseat libdisplay-info libinput xcb xcb-composite xcb-dri3 xcb-ewmh xcb-icccm xcb-present xcb-render xcb-renderutil xcb-res xcb-shm xcb-xfixes xcb-xinput
Cflags: -I\${includedir} -I${WLROOTS_BUILD_DIR}/protocol -DWLR_USE_UNSTABLE
Libs: -L\${libdir} -lwlroots-0.20 -ldl -lm -lrt

wlroots_version=${WLROOTS_VERSION}
wlroots_version_major=${WLROOTS_VERSION_MAJOR}
wlroots_version_minor=${WLROOTS_VERSION_MINOR}
wlroots_version_patch=${WLROOTS_VERSION_PATCH}
wlroots_features=DRM_BACKEND X11_BACKEND LIBINPUT_BACKEND XWAYLAND GLES2_RENDERER VULKAN_RENDERER GBM_ALLOCATOR UDMABUF_ALLOCATOR SESSION COLOR_MANAGEMENT
")

set(_wlcom_saved_pkg_config_path "$ENV{PKG_CONFIG_PATH}")
if(_wlcom_saved_pkg_config_path)
  set(ENV{PKG_CONFIG_PATH}
    "${WLROOTS_PKGCONFIG_DIR}:${_wlroots_pkg_config_path}"
  )
else()
  set(ENV{PKG_CONFIG_PATH} "${WLROOTS_PKGCONFIG_DIR}:${_wlroots_pkg_config_path}")
endif()

# pkg-config's CMake cache does not account for PKG_CONFIG_PATH changes. In
# particular a fallback selected after an earlier configure must not keep
# absolute paths to an older system library in the compositor's link line.
get_cmake_property(_wlcom_cache_variables CACHE_VARIABLES)
foreach(_variable IN LISTS _wlcom_cache_variables)
  if(_variable MATCHES "^(pkgcfg_lib_(WLCOM_WLROOTS|WLCOM_DEPS|WAYLAND_SERVER|WAYLAND_CLIENT)_|__pkg_config_checked_(WLCOM_WLROOTS|WLCOM_DEPS|WAYLAND_SERVER|WAYLAND_CLIENT)$)")
    unset(${_variable} CACHE)
  endif()
endforeach()

pkg_check_modules(WLCOM_WLROOTS REQUIRED IMPORTED_TARGET GLOBAL
  "wlroots-0.20=${WLROOTS_VERSION}"
)

# Keep the selected dependency metadata visible to compositor pkg-config lookups.

add_dependencies(PkgConfig::WLCOM_WLROOTS wlroots_external)
target_link_directories(PkgConfig::WLCOM_WLROOTS INTERFACE
  ${WLROOTS_LINK_DIRECTORIES}
)

list(APPEND CMAKE_BUILD_RPATH ${WLROOTS_RUNTIME_LIBRARY_DIRS})

# FindPkgConfig combines all -L options, so a system dependency listed first
# can otherwise resolve libdrm/pixman to the old system copy despite selecting
# the fallback .pc. Pin to the selected .pc's directory, including external
# prefixes supplied by the user (not just our own fallback link stubs).
function(wlcom_pin_wlroots_dependencies target)
  get_target_property(_libraries ${target} INTERFACE_LINK_LIBRARIES)
  if(NOT _libraries)
    return()
  endif()
  set(_selected "")
  foreach(_library IN LISTS _libraries)
    foreach(_name wayland-server wayland-client drm pixman-1 xkbcommon)
      if(_library STREQUAL "${_name}" OR _library MATCHES "/lib${_name}\\.(so|a)$")
        set(_package "${_name}")
        if(_name STREQUAL "drm")
          set(_package libdrm)
        endif()
        pkg_get_variable(_selected_libdir "${_package}" libdir)
        if(EXISTS "${_selected_libdir}/lib${_name}.so")
          set(_library "${_selected_libdir}/lib${_name}.so")
        elseif(EXISTS "${_selected_libdir}/lib${_name}.a")
          set(_library "${_selected_libdir}/lib${_name}.a")
        endif()
        break()
      endif()
    endforeach()
    list(APPEND _selected "${_library}")
  endforeach()
  set_property(TARGET ${target} PROPERTY INTERFACE_LINK_LIBRARIES "${_selected}")
endfunction()
wlcom_pin_wlroots_dependencies(PkgConfig::WLCOM_WLROOTS)

if(WLROOTS_RUNTIME_LIBRARY_DIRS)
  # Use an application-private directory, not the system's library namespace.
  set(WLCOM_PRIVATE_LIBDIR "${CMAKE_INSTALL_LIBDIR}/flakewm")
  foreach(_runtime_dir IN LISTS WLROOTS_RUNTIME_LIBRARY_DIRS)
    install(DIRECTORY "${_runtime_dir}/" DESTINATION "${WLCOM_PRIVATE_LIBDIR}"
      FILES_MATCHING
      PATTERN "libwayland-server.so.*" PATTERN "libwayland-client.so.*"
      PATTERN "libdrm.so.*" PATTERN "libpixman-1.so.*" PATTERN "libxkbcommon.so.*"
      PATTERN "pkgconfig" EXCLUDE
    )
  endforeach()
endif()

add_library(Wlroots::wlroots ALIAS PkgConfig::WLCOM_WLROOTS)
