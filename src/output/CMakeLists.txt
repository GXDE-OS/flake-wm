target_sources(gxde-wlcom PRIVATE
  gamma.c
  config.c
  layout.c
  modes.c
  output.c
  xdg_output.c
  ky_output.c
)

if(WLCOM_KDE_OUTPUT)
  target_sources(gxde-wlcom PRIVATE
    kde_output.c
  )
endif()

if(WLCOM_WLR_OUTPUT)
  target_sources(gxde-wlcom PRIVATE
    wlr_output.c
  )
endif()

if(WLCOM_UKUI_OUTPUT)
  target_sources(gxde-wlcom PRIVATE
    ukui_output.c
  )
endif()
