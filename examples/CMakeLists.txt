pkg_check_modules(EXAMPLES_EGL_DEPS REQUIRED IMPORTED_TARGET
  egl
  glesv2
  wayland-egl
)

# wlcom_example(<name> SOURCES <src>... [PROTOCOLS <name>...] [LIBRARIES <lib>...])
function(wlcom_example name)
  cmake_parse_arguments(PARSE_ARGV 1 ARG "" "" "SOURCES;PROTOCOLS;LIBRARIES")
  add_executable(${name} ${ARG_SOURCES})
  target_link_libraries(${name} PRIVATE PkgConfig::WAYLAND_CLIENT ${ARG_LIBRARIES})
  foreach(protocol IN LISTS ARG_PROTOCOLS)
    target_link_libraries(${name} PRIVATE wlcom-client-protocol-${protocol})
  endforeach()
endfunction()

if(WLCOM_KDE_OUTPUT)
  wlcom_example(output-doctor
    SOURCES output_doctor.c
    PROTOCOLS
      kde-output-management-v2
      kde-output-device-v2
      kde-primary-output-v1
      org-kde-kwin-dpms-manager
  )
endif()

if(WLCOM_KDE_VIRTUAL_DESKTOP)
  wlcom_example(virtual-desktop
    SOURCES virtual_desktop.c
    PROTOCOLS org-kde-plasma-virtual-desktop-management
  )
endif()

if(WLCOM_LIBKYWC)
  wlcom_example(monitor
    SOURCES monitor.c
    LIBRARIES kywc
  )

  wlcom_example(thumbnail
    SOURCES thumbnail.c buffer.c
    PROTOCOLS xdg-shell xdg-decoration
    LIBRARIES kywc PkgConfig::EXAMPLES_EGL_DEPS
  )
endif()

if(WLCOM_KDE_KEYSTATE)
  wlcom_example(kde_keystate
    SOURCES kde_keystate.c
    PROTOCOLS org-kde-kwin-keystate-manager
  )
endif()

if(WLCOM_LIBKYWC)
  add_subdirectory(gui)
endif()

add_library(simple MODULE simple_plugin.c)
set_target_properties(simple PROPERTIES PREFIX "")
target_include_directories(simple PRIVATE "${PROJECT_SOURCE_DIR}/include")
