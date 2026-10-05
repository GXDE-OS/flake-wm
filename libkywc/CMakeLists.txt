add_library(kywc SHARED
  libkywc.c
  output.c
  toplevel.c
  thumbnail.c
  workspace.c
  # providers
  kywc_capture.c
  kywc_output.c
  kywc_toplevel.c
  kywc_workspace.c
  gxde_identifier.c
)

target_link_libraries(kywc
  PRIVATE
    wlcom-client-protocol-kywc-capture-manager-v1
    wlcom-client-protocol-kywc-output-manager-v1
    wlcom-client-protocol-kywc-toplevel-manager-v1
    wlcom-client-protocol-kywc-workspace-manager-v1
    wlcom-client-protocol-gxde-identifier-v1
  PUBLIC
    PkgConfig::WAYLAND_CLIENT
)

target_include_directories(kywc PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
)

set(kywc_symbols_file "${CMAKE_CURRENT_SOURCE_DIR}/kywc.syms")
target_compile_options(kywc PRIVATE -fvisibility=default)
target_link_options(kywc PRIVATE
  "LINKER:--version-script,${kywc_symbols_file}"
  LINKER:--no-undefined
)
set_target_properties(kywc PROPERTIES
  OUTPUT_NAME flakewm-client
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  LINK_DEPENDS "${kywc_symbols_file}"
)

install(TARGETS kywc LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES libkywc.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/flakewm)

file(CONFIGURE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/flakewm-client.pc"
  CONTENT [[
prefix=@CMAKE_INSTALL_PREFIX@
includedir=${prefix}/@CMAKE_INSTALL_INCLUDEDIR@
libdir=${prefix}/@CMAKE_INSTALL_LIBDIR@

Name: flakewm-client
Description: flakewm client library
URL: https://gitee.com/openkylin/kylin-wayland-compositor
Version: @PROJECT_VERSION@
Libs: -L${libdir} -lflakewm-client
Cflags: -I${includedir}/flakewm
]]
  @ONLY
)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/flakewm-client.pc"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)
