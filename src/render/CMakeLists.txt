target_sources(gxde-wlcom PRIVATE
  pass.c
  pixel_format.c
  renderer.c
  shm_buffer.c
  wayland_drm.c
)

if(WLCOM_TRACY)
  target_sources(gxde-wlcom PRIVATE
    profile.c
    opengl/profile.c
  )
endif()

add_subdirectory(opengl)
