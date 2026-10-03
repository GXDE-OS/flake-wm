add_executable(gxde-wlcom)

target_sources(gxde-wlcom PRIVATE
  main.c
  server.c
  gxde_identifier.c
  app_id_resolver.c

  plugin/config.c
  plugin/plugin.c
)

add_subdirectory(backend)
add_subdirectory(config)
add_subdirectory(effect)
add_subdirectory(input)
add_subdirectory(output)
add_subdirectory(painter)
add_subdirectory(render)
add_subdirectory(scene)
add_subdirectory(security)
add_subdirectory(theme)
add_subdirectory(util)
add_subdirectory(view)
add_subdirectory(widget)

if(HAVE_XWAYLAND)
  add_subdirectory(xwayland)
endif()

add_subdirectory(vendor/dkwin/multitask)
add_subdirectory(vendor/dkwin/presentwindows)

target_include_directories(gxde-wlcom PRIVATE
  "${PROJECT_SOURCE_DIR}"
  "${PROJECT_BINARY_DIR}"
  "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(gxde-wlcom PRIVATE
  wlcom-protocols
  Wlroots::wlroots
  PkgConfig::WAYLAND_SERVER
  PkgConfig::WLCOM_DEPS
  Threads::Threads
  m
  dl
  rt
)

if(HAVE_XWAYLAND)
  target_link_libraries(gxde-wlcom PRIVATE PkgConfig::WLCOM_XWAYLAND_DEPS)
endif()

if(HAVE_LIBUNWIND)
  target_link_libraries(gxde-wlcom PRIVATE PkgConfig::LIBUNWIND)
endif()

if(WLCOM_TRACY)
  target_link_libraries(gxde-wlcom PRIVATE PkgConfig::TRACY)
endif()

# Plugins resolve compositor symbols at load time, see wlcom.syms.
set(wlcom_symbols_file "${PROJECT_SOURCE_DIR}/wlcom.syms")
set_target_properties(gxde-wlcom PROPERTIES
  # Keep ./build/gxde-wlcom working for nested test runs.
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
  ENABLE_EXPORTS ON
  LINK_DEPENDS "${wlcom_symbols_file}"
)
target_link_options(gxde-wlcom PRIVATE
  "LINKER:--version-script,${wlcom_symbols_file}"
)

install(TARGETS gxde-wlcom RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
