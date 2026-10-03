target_sources(gxde-wlcom PRIVATE
  action.c
  config.c
  decoration.c
  global_authentication.c
  interactive.c
  maximize_switcher.c
  window_switcher.c
  modal.c
  positioner.c
  session_lock.c
  split_screen_switcher.c
  ssd.c
  stack_mode.c
  tile_preview.c
  view.c
  window_menu.c
  workspace.c
  xdg_dialog.c
  xdg_popup.c
  xdg_shell.c
  xdg_activation.c
)

target_sources(gxde-wlcom PRIVATE
  ky_toplevel.c
  dbus_toplevel.c
  ky_workspace.c
)

if(WLCOM_KDE_VIRTUAL_DESKTOP)
  target_sources(gxde-wlcom PRIVATE
    kde_virtual_desktop.c
  )
endif()

if(WLCOM_WLR_FOREIGN_TOPLEVEL)
  target_sources(gxde-wlcom PRIVATE
    wlr_foreign_toplevel.c
  )
endif()

if(WLCOM_WLR_LAYER_SHELL)
  target_sources(gxde-wlcom PRIVATE
    wlr_layer_shell.c
  )
endif()

if(WLCOM_KDE_PLASMA_SHELL)
  target_sources(gxde-wlcom PRIVATE
    kde_plasma_shell.c
  )
endif()

if(WLCOM_KDE_PLASMA_WINDOW_MANAGEMENT)
  target_sources(gxde-wlcom PRIVATE
    kde_plasma_window.c
  )
endif()

if(WLCOM_KDE_APPMENU)
  target_sources(gxde-wlcom PRIVATE
    kde_appmenu.c
  )
endif()

if(WLCOM_KDE_BLUR)
  target_sources(gxde-wlcom PRIVATE
    kde_blur.c
  )
endif()

if(WLCOM_KDE_SLIDE)
  target_sources(gxde-wlcom PRIVATE
    kde_slide.c
  )
endif()

if(WLCOM_UKUI_SHELL)
  target_sources(gxde-wlcom PRIVATE
    ukui_shell.c
  )
endif()

if(WLCOM_UKUI_WINDOW_MANAGEMENT)
  target_sources(gxde-wlcom PRIVATE
    ukui_window.c
  )
endif()

if(WLCOM_UKUI_BLUR)
  target_sources(gxde-wlcom PRIVATE
    ukui_blur.c
  )
endif()

# 从Treeland移植的部分DDE Shell协议的实现
target_sources(gxde-wlcom PRIVATE
  dde_shell.c
  treeland_dde_shell.c
  treeland_personalization.c
  treeland_color_scheme.c
)
