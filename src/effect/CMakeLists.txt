target_sources(flakewm PRIVATE
  animator.c
  blur.c
  capture.c
  circle_progressbar.c
  config.c
  effect.c
  ext_image_copy_capture.c
  fade.c
  gxde_screenshot.c
  locate_pointer.c
  magic_lamp.c
  mouse_click.c
  mouse_trail.c
  move.c
  output_transform.c
  showfps.c
  scale.c
  shake_cursor.c
  shake_view.c
  soft_gamma.c
  tap_ripple.c
  touch_click.c
  touch_long.c
  touch_trail.c
  trail.c
  translation.c
  transform.c
  zoom.c
)

target_sources(flakewm PRIVATE
  ky_capture.c
)

target_sources(flakewm PRIVATE
  treeland_capture.c
)

if(WLCOM_UKUI_SCREENSHOT)
  target_sources(flakewm PRIVATE
    screenshot.c
  )
endif()

if(WLCOM_UKUI_WATERMARK)
  target_sources(flakewm PRIVATE
    watermark.c
  )
endif()

if(WLCOM_KDE_SLIDE)
  target_sources(flakewm PRIVATE
    slide.c
  )
endif()

add_subdirectory(shaders)
