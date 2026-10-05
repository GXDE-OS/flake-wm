# The upstream Deepin/KWin implementation is kept verbatim in upstream/ for
# layout, animation, shader and asset parity.  It is deliberately not compiled:
# wlcom_multitask.c is the compositor-native adapter and has no KWin dependency.
set(MULTITASK_SOURCE_ASSET_DIR "${CMAKE_CURRENT_SOURCE_DIR}/upstream/buttons")
set(MULTITASK_INSTALL_ASSET_DIR
  "${CMAKE_INSTALL_FULL_DATADIR}/flakewm/multitask/buttons"
)
file(CONFIGURE
  OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/multitask_assets.h"
  CONTENT [[
#pragma once

#define MULTITASK_INSTALL_ASSET_DIR "@MULTITASK_INSTALL_ASSET_DIR@"

#define MULTITASK_SOURCE_ASSET_DIR "@MULTITASK_SOURCE_ASSET_DIR@"
]]
  @ONLY
)

install(DIRECTORY upstream/buttons upstream/shaders
  DESTINATION ${CMAKE_INSTALL_DATADIR}/flakewm/multitask
)

target_sources(flakewm PRIVATE
  wlcom_multitask.c
  wlcom_multitask_dbus.c
)
