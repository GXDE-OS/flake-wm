target_sources(flakewm PRIVATE
  config.c
  icon.c
  theme.c
)

if(WLCOM_UKUI_THEME)
  target_sources(flakewm PRIVATE
    ukui_theme.c
  )
endif()

add_subdirectory(icons)
