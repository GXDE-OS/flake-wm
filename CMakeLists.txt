cmake_minimum_required(VERSION 3.20)
project(FlakeWM LANGUAGES CXX)
include(ExternalProject)

find_program(MESON_EXECUTABLE meson REQUIRED)
set(WLROOTS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/wlroots")
set(WLROOTS_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/wlroots-build")
set(WAYLAND_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs/wayland")
set(VENDOR_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/vendor-install")
set(WAYLAND_SERVER_LIBRARY
    "${VENDOR_INSTALL_DIR}/lib/libwayland-server.so"
)
set(WAYLAND_CLIENT_LIBRARY
    "${VENDOR_INSTALL_DIR}/lib/libwayland-client.so"
)
set(WLROOTS_LIBRARY
    "${VENDOR_INSTALL_DIR}/lib/libwlroots-0.20.so"
)

ExternalProject_Add(wayland_external
    SOURCE_DIR "${WAYLAND_SOURCE_DIR}"
    BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/wayland-build"
    CONFIGURE_COMMAND
        "${MESON_EXECUTABLE}" setup
        "<BINARY_DIR>"
        "<SOURCE_DIR>"
        "--prefix=${VENDOR_INSTALL_DIR}"
        "--libdir=lib"
        "--buildtype=debugoptimized"
        "-Ddocumentation=false"
        "-Ddtd_validation=false"
        "-Dtests=false"
    BUILD_COMMAND
        "${MESON_EXECUTABLE}" compile -C "<BINARY_DIR>"
    INSTALL_COMMAND
        "${MESON_EXECUTABLE}" install -C "<BINARY_DIR>"
    BUILD_BYPRODUCTS
        "${WAYLAND_SERVER_LIBRARY}"
        "${WAYLAND_CLIENT_LIBRARY}"
)

ExternalProject_Add(wlroots_external
    SOURCE_DIR "${WLROOTS_SOURCE_DIR}"
    BINARY_DIR "${WLROOTS_BUILD_DIR}"
    CONFIGURE_COMMAND
        "${CMAKE_COMMAND}" -E env
        "PKG_CONFIG_PATH=${VENDOR_INSTALL_DIR}/lib/pkgconfig"
        "PATH=${VENDOR_INSTALL_DIR}/bin:$ENV{PATH}"
        "${MESON_EXECUTABLE}" setup
        "<BINARY_DIR>"
        "<SOURCE_DIR>"
        "--prefix=${VENDOR_INSTALL_DIR}"
        "--libdir=lib"
        "--buildtype=debugoptimized"
        "-Dexamples=false"
        "-Dwerror=false"
        "-Ddefault_library=shared"
    BUILD_COMMAND
        "${MESON_EXECUTABLE}" compile -C "<BINARY_DIR>"
    INSTALL_COMMAND
        "${MESON_EXECUTABLE}" install -C "<BINARY_DIR>"
    BUILD_ALWAYS TRUE
    BUILD_BYPRODUCTS "${WLROOTS_LIBRARY}"
    DEPENDS wayland_external
)

# The directory is populated when wlroots is installed. It must exist during
# CMake generation so it can be exposed by the imported target.
file(MAKE_DIRECTORY "${VENDOR_INSTALL_DIR}/include/wlroots-0.20")

add_library(wlroots SHARED IMPORTED GLOBAL)
set_target_properties(wlroots PROPERTIES
    IMPORTED_LOCATION "${WLROOTS_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES
        "${VENDOR_INSTALL_DIR}/include/wlroots-0.20"
)
add_dependencies(wlroots wlroots_external)

add_library(wayland_server SHARED IMPORTED GLOBAL)
set_target_properties(wayland_server PROPERTIES
    IMPORTED_LOCATION "${WAYLAND_SERVER_LIBRARY}"
)
add_dependencies(wayland_server wayland_external)

add_library(wayland_client SHARED IMPORTED GLOBAL)
set_target_properties(wayland_client PROPERTIES
    IMPORTED_LOCATION "${WAYLAND_CLIENT_LIBRARY}"
)
add_dependencies(wayland_client wayland_external)

add_executable(flakewm main.cc)
target_compile_features(flakewm PRIVATE cxx_std_20)
target_compile_definitions(flakewm PRIVATE WLR_USE_UNSTABLE)
target_link_libraries(flakewm PRIVATE
    wlroots
    wayland_server
    wayland_client
)

# main.cc is intentionally empty of wlroots calls. Keep the shared library in
# DT_NEEDED even when the linker defaults to --as-needed.
target_link_options(flakewm PRIVATE "LINKER:--no-as-needed")
set_target_properties(flakewm PROPERTIES
    BUILD_RPATH "${VENDOR_INSTALL_DIR}/lib"
)
