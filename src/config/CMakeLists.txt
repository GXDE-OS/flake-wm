target_sources(gxde-wlcom PRIVATE
  common.c
  config.c
  gtk_theme.c
)

if(WLCOM_KDE_GLOBAL_ACCEL)
  add_executable(qtkey-mapper qtkey_mapper.c)
  wlcom_capture_header(gxde-wlcom qtkey_map_table.h qtkey-mapper)

  target_sources(gxde-wlcom PRIVATE
    kde_global_accel.c
  )
endif()

if(WLCOM_KDE_INPUT)
  target_sources(gxde-wlcom PRIVATE
    kde_input.c
  )
endif()

if(WLCOM_UKUI_SHORTCUT)
  target_sources(gxde-wlcom PRIVATE
    ukui_shortcut.c
  )
endif()

if(WLCOM_UKUI_GSETTINGS)
  target_sources(gxde-wlcom PRIVATE
    ukui_gsettings.c
  )
endif()

if(WLCOM_UKUI_VIEW_MODE)
  target_sources(gxde-wlcom PRIVATE
    ukui_view_mode.c
  )
endif()
