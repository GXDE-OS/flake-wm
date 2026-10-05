pkg_get_variable(WL_PROTOCOL_DIR wayland-protocols pkgdatadir)
if(WLROOTS_VENDOR_WAYLAND_PREFIX)
  set(WAYLAND_SCANNER_EXECUTABLE "${WLROOTS_VENDOR_WAYLAND_PREFIX}/bin/wayland-scanner")
  set(_wlcom_scanner_dependency wlroots_vendor_wayland)
else()
  pkg_get_variable(WAYLAND_SCANNER_EXECUTABLE wayland-scanner wayland_scanner)
endif()

# name -> XML. The name is what consumers use to pick a client protocol, see
# wlcom-client-protocol-<name> below.
set(protocols
  wl-drm drm.xml
  ext-idle-notifier-v1 ${WL_PROTOCOL_DIR}/staging/ext-idle-notify/ext-idle-notify-v1.xml
  ext-foreign-toplevel-list-v1 ${WL_PROTOCOL_DIR}/staging/ext-foreign-toplevel-list/ext-foreign-toplevel-list-v1.xml
  ext-image-capture-source-v1 ${WL_PROTOCOL_DIR}/staging/ext-image-capture-source/ext-image-capture-source-v1.xml
  ext-image-copy-capture-v1 ${WL_PROTOCOL_DIR}/staging/ext-image-copy-capture/ext-image-copy-capture-v1.xml
  xdg-shell ${WL_PROTOCOL_DIR}/stable/xdg-shell/xdg-shell.xml
  xdg-output-unstable-v1 ${WL_PROTOCOL_DIR}/unstable/xdg-output/xdg-output-unstable-v1.xml
  zwp-tablet-manager-v2 ${WL_PROTOCOL_DIR}/unstable/tablet/tablet-unstable-v2.xml
  zwp-text-input-v1 ${WL_PROTOCOL_DIR}/unstable/text-input/text-input-unstable-v1.xml
  zwp-text-input-v2 text-input-unstable-v2.xml
  xdg-decoration ${WL_PROTOCOL_DIR}/unstable/xdg-decoration/xdg-decoration-unstable-v1.xml
  xdg-dialog ${WL_PROTOCOL_DIR}/staging/xdg-dialog/xdg-dialog-v1.xml
  org-kde-kwin-idle idle.xml
  zwp-pointer-constraints-v1 ${WL_PROTOCOL_DIR}/unstable/pointer-constraints/pointer-constraints-unstable-v1.xml
  ext-transient-seat-v1 ${WL_PROTOCOL_DIR}/staging/ext-transient-seat/ext-transient-seat-v1.xml
  xdg-toplevel-drag-manager-v1 ${WL_PROTOCOL_DIR}/staging/xdg-toplevel-drag/xdg-toplevel-drag-v1.xml
  wptearing-contorl-v1 ${WL_PROTOCOL_DIR}/staging/tearing-control/tearing-control-v1.xml
)

list(APPEND protocols
  kywc-capture-manager-v1 kywc-capture-v1.xml
  kywc-output-manager-v1 kywc-output-v1.xml
  kywc-toplevel-manager-v1 kywc-toplevel-v1.xml
  kywc-workspace-manager-v1 kywc-workspace-v1.xml
  gxde-identifier-v1 gxde-identifier-v1.xml
  gxde-screenshot-manager-v1 gxde-screenshot-v1.xml
)

if(WLCOM_KDE_OUTPUT)
  list(APPEND protocols
    kde-output-device-v2 kde-output-device-v2.xml
    kde-output-management-v2 kde-output-management-v2.xml
    kde-primary-output-v1 kde-primary-output-v1.xml
    org-kde-kwin-dpms-manager dpms.xml
  )
endif()

if(WLCOM_KDE_VIRTUAL_DESKTOP)
  list(APPEND protocols
    org-kde-plasma-virtual-desktop-management plasma-virtual-desktop.xml
  )
endif()

if(WLCOM_WLR_OUTPUT)
  list(APPEND protocols
    zwlr-output-power-manager-v1 wlr-output-power-management-unstable-v1.xml
  )
endif()

if(WLCOM_WLR_LAYER_SHELL)
  list(APPEND protocols
    zwlr-layer-shell-v1 wlr-layer-shell-unstable-v1.xml
  )
endif()

if(WLCOM_KDE_PLASMA_SHELL)
  list(APPEND protocols org-kde-plasma-shell plasma-shell.xml)
endif()

if(WLCOM_KDE_PLASMA_WINDOW_MANAGEMENT)
  list(APPEND protocols
    org-kde-plasma-window-management plasma-window-management.xml
  )
endif()

if(WLCOM_KDE_APPMENU)
  list(APPEND protocols org-kde-kwin-appmenu-manager appmenu.xml)
endif()

if(WLCOM_KDE_BLUR)
  list(APPEND protocols org-kde-kwin-blur-manager blur.xml)
endif()

if(WLCOM_KDE_SLIDE)
  list(APPEND protocols org-kde-kwin-slide-manager slide.xml)
endif()

if(WLCOM_KDE_KEYSTATE)
  list(APPEND protocols org-kde-kwin-keystate-manager kde-keystate.xml)
endif()

if(WLCOM_UKUI_SHELL)
  list(APPEND protocols ukui-shell ukui-shell.xml)
endif()

if(WLCOM_UKUI_WINDOW_MANAGEMENT)
  list(APPEND protocols ukui-window-management ukui-window-management.xml)
endif()

if(WLCOM_UKUI_OUTPUT)
  list(APPEND protocols ukui-output-v1 ukui-output-v1.xml)
endif()

if(WLCOM_UKUI_BLUR)
  list(APPEND protocols ukui-blur ukui-blur-v1.xml)
endif()

# 从Treeland移植的部分协议
list(APPEND protocols
  dde-shell dde-shell.xml
  treeland-dde-shell-v1 treeland-dde-shell-v1.xml
  treeland-app-id-resolver-v1 treeland-app-id-resolver-v1.xml
  treeland-capture-unstable-v1 treeland-capture-unstable-v1.xml
  treeland-personalization-manager-v1 treeland-personalization-manager-v1.xml
)

set(protocol_code "")
set(protocol_server_headers "")
set(protocol_outputs "")
set(protocol_names "")

list(LENGTH protocols protocols_length)
math(EXPR protocols_last "${protocols_length} - 1")
foreach(index RANGE 0 ${protocols_last} 2)
  math(EXPR xml_index "${index} + 1")
  list(GET protocols ${index} name)
  list(GET protocols ${xml_index} xml)
  cmake_path(ABSOLUTE_PATH xml BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  cmake_path(GET xml STEM basename)

  set(code "${CMAKE_CURRENT_BINARY_DIR}/${basename}-protocol.c")
  set(server_header "${CMAKE_CURRENT_BINARY_DIR}/${basename}-protocol.h")
  set(client_header "${CMAKE_CURRENT_BINARY_DIR}/${basename}-client-protocol.h")

  add_custom_command(
    OUTPUT "${code}"
    COMMAND "${WAYLAND_SCANNER_EXECUTABLE}" private-code "${xml}" "${code}"
    DEPENDS "${xml}" ${_wlcom_scanner_dependency}
    VERBATIM
  )
  add_custom_command(
    OUTPUT "${server_header}"
    COMMAND "${WAYLAND_SCANNER_EXECUTABLE}" server-header "${xml}" "${server_header}"
    DEPENDS "${xml}" ${_wlcom_scanner_dependency}
    VERBATIM
  )
  add_custom_command(
    OUTPUT "${client_header}"
    COMMAND "${WAYLAND_SCANNER_EXECUTABLE}" client-header "${xml}" "${client_header}"
    DEPENDS "${xml}" ${_wlcom_scanner_dependency}
    VERBATIM
  )

  list(APPEND protocol_code "${code}")
  list(APPEND protocol_server_headers "${server_header}")
  list(APPEND protocol_outputs "${code}" "${server_header}")
  list(APPEND protocol_names "${name}")
  set(protocol_client_${name} "${code}" "${client_header}")
endforeach()

# Every generated file is listed by more than one target below. Funnel the
# generation through one target so parallel builds never run a rule twice.
add_custom_target(wlcom-protocol-sources DEPENDS ${protocol_outputs})

# Server side: linked into gxde-wlcom.
add_library(wlcom-protocols OBJECT ${protocol_code} ${protocol_server_headers})
add_dependencies(wlcom-protocols wlcom-protocol-sources)
target_include_directories(wlcom-protocols PUBLIC "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(wlcom-protocols PUBLIC PkgConfig::WAYLAND_SERVER)

# Client side: one object library per protocol, only built when something
# (libkywc, examples) links it.
foreach(name IN LISTS protocol_names)
  add_library(wlcom-client-protocol-${name} OBJECT EXCLUDE_FROM_ALL
    ${protocol_client_${name}}
  )
  add_dependencies(wlcom-client-protocol-${name} wlcom-protocol-sources)
  set_target_properties(wlcom-client-protocol-${name} PROPERTIES
    POSITION_INDEPENDENT_CODE ON
  )
  target_include_directories(wlcom-client-protocol-${name} PUBLIC
    "${CMAKE_CURRENT_BINARY_DIR}"
  )
  target_link_libraries(wlcom-client-protocol-${name} PUBLIC
    PkgConfig::WAYLAND_CLIENT
  )
endforeach()
