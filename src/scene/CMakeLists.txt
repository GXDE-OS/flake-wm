target_sources(flakewm PRIVATE
  animation.c
  box.c
  buffer.c
  decoration.c
  output.c
  rect.c
  render.c
  scene.c
  surface.c
  subsurface.c
  thumbnail.c
  xdg_shell.c
)

add_subdirectory(shaders)
