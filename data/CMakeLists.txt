install(FILES udev-rules/90-framebuffer.rules
  DESTINATION /usr/lib/udev/rules.d
)

install(FILES gxde-wlcom.desktop
  DESTINATION /usr/share/wayland-sessions
)

install(PROGRAMS startgxde_wlcom
  DESTINATION /usr/bin
)

install(FILES config.json
  DESTINATION /etc/gxde-wlcom
)

set(GETTEXT_PACKAGE "${PROJECT_NAME}")
configure_file(
  org.ukui.peripherals-mouse.gschema.xml
  org.ukui.peripherals-mouse.gschema.xml
  @ONLY
)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/org.ukui.peripherals-mouse.gschema.xml"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/glib-2.0/schemas
)
