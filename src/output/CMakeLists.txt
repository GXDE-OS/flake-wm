target_sources(flakewm PRIVATE
  gamma.c
  config.c
  layout.c
  modes.c
  output.c
  xdg_output.c
  ky_output.c
)

if(WLCOM_KDE_OUTPUT)
  target_sources(flakewm PRIVATE
    kde_output.c
  )
endif()

if(WLCOM_WLR_OUTPUT)
  target_sources(flakewm PRIVATE
    wlr_output.c
  )
endif()

if(WLCOM_UKUI_OUTPUT)
  target_sources(flakewm PRIVATE
    ukui_output.c
  )
endif()
