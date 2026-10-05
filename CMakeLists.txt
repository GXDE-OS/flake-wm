cmake_minimum_required(VERSION 3.21)

project(gxde-wlcom VERSION 1.3.0 LANGUAGES C)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build" FORCE)
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

foreach(config RELWITHDEBINFO MINSIZEREL)
  string(REPLACE "-DNDEBUG" "" CMAKE_C_FLAGS_${config}
    "${CMAKE_C_FLAGS_${config}}"
  )
  string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_${config}
    "${CMAKE_CXX_FLAGS_${config}}"
  )
endforeach()

include(GNUInstallDirs)
include(CheckCCompilerFlag)
include(CheckLanguage)
include(cmake/WlcomHelpers.cmake)

set(WLCOM_PLUGIN_DIRECTORY
  "" CACHE STRING
  "Location for plugin"
)

set(WLCOM_LIBUNWIND "auto" CACHE STRING "Use libunwind for stack-traces")
set(WLCOM_NLS "auto" CACHE STRING "Enable native language support")
set_property(CACHE WLCOM_LIBUNWIND WLCOM_NLS PROPERTY STRINGS
  auto enabled disabled
)

option(WLCOM_EXAMPLES "Build example applications" ON)
option(WLCOM_LIBKYWC "Build libkywc for client" ON)
option(WLCOM_TRACY "Enable tracy profiler" OFF)
set(WLCOM_EXTRA_ICON_PATH "" CACHE STRING "Add extra icon path")
set(WLCOM_EXTRA_APPS_PATH "" CACHE STRING "Add extra apps path")

set(wlcom_feature_options
  KDE_OUTPUT "Enable kde output protocols support" ON
  KDE_VIRTUAL_DESKTOP "Enable kde virtual desktop protocol support" ON
  WLR_OUTPUT "Enable wlroots output protocols support" ON
  WLR_FOREIGN_TOPLEVEL "Enable wlroots foreign toplevel protocol support" ON
  WLR_LAYER_SHELL "Enable wlroots layer shell protocol support" ON
  KDE_GLOBAL_ACCEL "Enable kde global accel dbus server support" ON
  KDE_INPUT "Enable kde input dbus server support" ON
  KDE_PLASMA_SHELL "Enable kde plasma shell protocol support" ON
  KDE_PLASMA_WINDOW_MANAGEMENT "Enable kde plasma window management protocol support" ON
  KDE_APPMENU "Enable kde appmenu protocol support" ON
  KDE_BLUR "Enable kde blur protocol support" ON
  KDE_SLIDE "Enable kde slide protocol support" ON
  UKUI_SCREENSHOT "Enable ukui screenshot dbus server support" ON
  UKUI_SHORTCUT "Enable ukui shortcut block dbus manager support" ON
  UKUI_GSETTINGS "Enable ukui configuration gsettings support" ON
  UKUI_THEME "Enable ukui widget theme support" OFF
  KDE_KEYSTATE "Enable kde keystate protocol support" ON
  KDE_CLIPBOARD "Enable kde clipboard dbus" ON
  CLIPBOARD_PERSIST "Keep clipboard content after the source client exits" ON
  UKUI_WATERMARK "Enable ukui watermark dbus" ON
  UKUI_SHELL "Enable ukui shell protocol support" ON
  UKUI_WINDOW_MANAGEMENT "Enable ukui window management protocol support" ON
  UKUI_OUTPUT "Enable ukui output protocol support" ON
  UKUI_BLUR "Enable ukui blur protocol support" ON
  UKUI_VIEW_MODE "Enable ukui view mode dbus support" ON
)

set(wlcom_feature_names "")
list(LENGTH wlcom_feature_options wlcom_feature_options_length)
math(EXPR wlcom_feature_options_last "${wlcom_feature_options_length} - 1")
foreach(index RANGE 0 ${wlcom_feature_options_last} 3)
  math(EXPR description_index "${index} + 1")
  math(EXPR default_index "${index} + 2")
  list(GET wlcom_feature_options ${index} name)
  list(GET wlcom_feature_options ${description_index} description)
  list(GET wlcom_feature_options ${default_index} default)
  option(WLCOM_${name} "${description}" ${default})
  list(APPEND wlcom_feature_names ${name})
endforeach()

if(WLCOM_PLUGIN_DIRECTORY)
  set(WLCOM_PLUGIN_DIR "${WLCOM_PLUGIN_DIRECTORY}")
else()
  set(WLCOM_PLUGIN_DIR "${CMAKE_INSTALL_PREFIX}/lib/gxde-wlcom/plugins")
endif()

add_compile_options(-Wall -Wextra)
add_compile_definitions(_FILE_OFFSET_BITS=64)
set(wlcom_warning_flags
  -Wundef
  -Wlogical-op
  -Wmissing-include-dirs
  -Wold-style-definition
  -Wpointer-arith
  -Winit-self
  -Wstrict-prototypes
  -Wimplicit-fallthrough=2
  -Wendif-labels
  -Wstrict-aliasing=2
  -Woverflow
  -Wmissing-prototypes
  -Walloca
  -Wunused-macros
  -Wno-unused-parameter
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  list(APPEND wlcom_warning_flags
    -Wno-unused-variable
    -Wno-unused-but-set-variable
  )
endif()

foreach(flag IN LISTS wlcom_warning_flags)
  string(MAKE_C_IDENTIFIER "WLCOM_HAS_C_FLAG${flag}" flag_variable)
  check_c_compiler_flag("${flag}" ${flag_variable})
  if(${flag_variable})
    add_compile_options("$<$<COMPILE_LANGUAGE:C>:${flag}>")
  endif()
endforeach()

add_link_options(LINKER:--as-needed)

set(WLCOM_VERSION_STRING "${PROJECT_VERSION}")
find_program(DPKG_PARSECHANGELOG_EXECUTABLE dpkg-parsechangelog)
if(DPKG_PARSECHANGELOG_EXECUTABLE)
  execute_process(
    COMMAND "${DPKG_PARSECHANGELOG_EXECUTABLE}" -SVersion
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    RESULT_VARIABLE dpkg_result
    OUTPUT_VARIABLE dpkg_version
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  if(dpkg_result EQUAL 0)
    set(WLCOM_VERSION_STRING "${dpkg_version}")
  endif()
endif()

find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(
    COMMAND "${GIT_EXECUTABLE}" rev-parse --short=7 HEAD
    WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
    RESULT_VARIABLE git_result
    OUTPUT_VARIABLE git_commit
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  if(git_result EQUAL 0)
    string(APPEND WLCOM_VERSION_STRING ".g${git_commit}")
  endif()
endif()

include(cmake/wlroots.cmake)
find_package(Threads REQUIRED)
pkg_check_modules(WAYLAND_SERVER REQUIRED IMPORTED_TARGET
  "wayland-server>=1.19.0"
)
pkg_check_modules(WAYLAND_CLIENT REQUIRED IMPORTED_TARGET wayland-client)
pkg_check_modules(WAYLAND_PROTOCOLS REQUIRED "wayland-protocols>=1.41")
pkg_check_modules(WLCOM_DEPS REQUIRED IMPORTED_TARGET
  xkbcommon
  libdrm
  "libinput>=1.14"
  libudev
  pixman-1
  xcb
  "json-c>=0.13"
  libsystemd
  cairo
  pango
  pangocairo
  fontconfig
  "librsvg-2.0>=2.46"
  libjpeg
  libpng
  "gio-2.0>=2.64"
  "epoxy>=1.5.4"
  "gbm>=17.1.0"
  openssl
)

# Foundational libraries must be identical to those used by wlroots.
wlcom_pin_wlroots_dependencies(PkgConfig::WLCOM_DEPS)
wlcom_pin_wlroots_dependencies(PkgConfig::WAYLAND_SERVER)
wlcom_pin_wlroots_dependencies(PkgConfig::WAYLAND_CLIENT)

if(WLCOM_XWAYLAND AND WLROOTS_HAVE_XWAYLAND)
  set(HAVE_XWAYLAND ON)
  pkg_check_modules(WLCOM_XWAYLAND_DEPS REQUIRED IMPORTED_TARGET
    xcb-shape
    xcb-xfixes
  )
else()
  set(HAVE_XWAYLAND OFF)
endif()

set(HAVE_LIBUNWIND OFF)
if(NOT WLCOM_LIBUNWIND STREQUAL "disabled")
  if(WLCOM_LIBUNWIND STREQUAL "enabled")
    pkg_check_modules(LIBUNWIND REQUIRED IMPORTED_TARGET libunwind)
  else()
    pkg_check_modules(LIBUNWIND QUIET IMPORTED_TARGET libunwind)
  endif()
  set(HAVE_LIBUNWIND ${LIBUNWIND_FOUND})
endif()

set(HAVE_NLS OFF)
if(NOT WLCOM_NLS STREQUAL "disabled")
  if(WLCOM_NLS STREQUAL "enabled")
    find_program(MSGFMT_EXECUTABLE msgfmt REQUIRED)
  else()
    find_program(MSGFMT_EXECUTABLE msgfmt)
  endif()
  if(MSGFMT_EXECUTABLE)
    set(HAVE_NLS ON)
  endif()
endif()

if(WLCOM_TRACY)
  pkg_check_modules(TRACY REQUIRED IMPORTED_TARGET "tracy>=0.11.0")
  if(NOT TRACY_VERSION VERSION_LESS "0.12.0")
    message(FATAL_ERROR "tracy ${TRACY_VERSION} found, but < 0.12.0 is required")
  endif()
  add_compile_definitions(TRACY_ENABLE)
endif()

check_language(CXX)
if(CMAKE_CXX_COMPILER)
  enable_language(CXX)
endif()

macro(wlcom_define_bool name value)
  if(${value})
    add_compile_definitions(${name}=1)
  else()
    add_compile_definitions(${name}=0)
  endif()
endmacro()

add_compile_definitions(
  "KYWC_VERSION=\"${WLCOM_VERSION_STRING}\""
  WLR_USE_UNSTABLE
  "PLUGIN_DIR=\"${WLCOM_PLUGIN_DIR}\""
)

wlcom_define_bool(HAVE_XWAYLAND HAVE_XWAYLAND)
wlcom_define_bool(HAVE_LIBUNWIND HAVE_LIBUNWIND)
foreach(name IN LISTS wlcom_feature_names)
  wlcom_define_bool(HAVE_${name} WLCOM_${name})
endforeach()

add_compile_definitions(
  "EXTRA_ICON_PATH=\"${WLCOM_EXTRA_ICON_PATH}\""
  "EXTRA_APPS_PATH=\"${WLCOM_EXTRA_APPS_PATH}\""
)

wlcom_define_bool(HAVE_NLS HAVE_NLS)
if(HAVE_NLS)
  add_compile_definitions(
    "GETTEXT_PACKAGE=\"${PROJECT_NAME}\""
    "LOCALEDIR=\"${CMAKE_INSTALL_FULL_LOCALEDIR}\""
  )
endif()

enable_testing()
if(HAVE_NLS)
  add_subdirectory(po)
endif()

add_subdirectory(data)
add_subdirectory(protocols)
add_subdirectory(src)

if(WLCOM_LIBKYWC)
  add_subdirectory(libkywc)
  add_subdirectory(project_occultation/exoskeleton)
endif()

if(WLCOM_EXAMPLES)
  add_subdirectory(examples)
endif()

function(wlcom_summary_line name value)
  if(value STREQUAL "ON" OR value STREQUAL "TRUE" OR value STREQUAL "1")
    set(value "YES")
  elseif(value STREQUAL "OFF" OR value STREQUAL "FALSE" OR value STREQUAL "0")
    set(value "NO")
  endif()
  string(LENGTH "${name}" name_length)
  math(EXPR padding "28 - ${name_length}")
  string(REPEAT " " ${padding} pad)
  message(STATUS "    ${name}${pad}: ${value}")
endfunction()

message(STATUS "")
message(STATUS "${PROJECT_NAME} ${WLCOM_VERSION_STRING}")
message(STATUS "")
message(STATUS "  Directories")
wlcom_summary_line(prefix "${CMAKE_INSTALL_PREFIX}")
wlcom_summary_line(plugin "${WLCOM_PLUGIN_DIR}")
message(STATUS "")
message(STATUS "  Configuration")
wlcom_summary_line(build_type "${CMAKE_BUILD_TYPE}")
wlcom_summary_line(wlroots_renderers "${WLCOM_WLROOTS_RENDERERS}")
wlcom_summary_line(xwayland "${HAVE_XWAYLAND}")
wlcom_summary_line(examples "${WLCOM_EXAMPLES}")
wlcom_summary_line(libunwind "${HAVE_LIBUNWIND}")
wlcom_summary_line(nls "${HAVE_NLS}")
wlcom_summary_line(libkywc "${WLCOM_LIBKYWC}")
wlcom_summary_line(tracy "${WLCOM_TRACY}")

foreach(name IN LISTS wlcom_feature_names)
  string(TOLOWER "${name}" lower_name)
  wlcom_summary_line(${lower_name} "${WLCOM_${name}}")
endforeach()

wlcom_summary_line(extra_icon_path "${WLCOM_EXTRA_ICON_PATH}")
wlcom_summary_line(extra_apps_path "${WLCOM_EXTRA_APPS_PATH}")
message(STATUS "")
