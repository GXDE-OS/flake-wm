target_sources(gxde-wlcom PRIVATE
  config.c
  icon.c
  theme.c
)

if(WLCOM_UKUI_THEME)
  target_sources(gxde-wlcom PRIVATE
    ukui_theme.c
  )
endif()

add_subdirectory(icons)
