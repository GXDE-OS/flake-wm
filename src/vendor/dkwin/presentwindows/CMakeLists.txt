# The upstream Deepin/KWin implementation is kept verbatim in upstream/ for
# layout, animation and interaction parity.  It is deliberately not compiled:
# wlcom_presentwindows.c is the compositor-native adapter and has no KWin
# dependency.
target_sources(gxde-wlcom PRIVATE
  wlcom_presentwindows.c
)
