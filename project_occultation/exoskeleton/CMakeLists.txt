if(NOT CMAKE_CXX_COMPILER)
  return()
endif()

# thumbnail_item uses <qpa/...>. Qt >= 6.9 ships the private module as its own
# package, older releases create Qt6::GuiPrivate along with Qt6::Gui.
find_package(Qt6 QUIET COMPONENTS Core DBus Gui Quick Qml
  OPTIONAL_COMPONENTS GuiPrivate
)
if(NOT Qt6_FOUND OR NOT TARGET Qt6::GuiPrivate)
  message(WARNING
    "Qt 6 Quick was not found; occultation exoskeleton will not be built"
  )
  return()
endif()

pkg_check_modules(OCCULTATION_DEPS REQUIRED IMPORTED_TARGET egl glesv2 libdrm)

qt6_add_resources(occultation_resources resources.qrc)

add_library(occultation-exoskeleton STATIC
  exoskeleton/exoskeleton.cc
  exoskeleton/exoskeleton.h
  icon_item/icon_item.cc
  icon_item/icon_item.h
  registration/registration.cc
  switcher_item/switcher_item.cc
  switcher_item/switcher_item.h
  switcher_model/switcher_model.cc
  switcher_model/switcher_model.h
  thumbnail_item/thumbnail_item.cc
  thumbnail_item/thumbnail_item.h
  ${occultation_resources}
)
set_target_properties(occultation-exoskeleton PROPERTIES AUTOMOC ON)
target_compile_options(occultation-exoskeleton PUBLIC -Wno-unused-parameter)
target_include_directories(occultation-exoskeleton PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(occultation-exoskeleton PUBLIC
  Qt6::Core
  Qt6::DBus
  Qt6::Gui
  Qt6::GuiPrivate
  Qt6::Quick
  Qt6::Qml
  kywc
  PkgConfig::OCCULTATION_DEPS
)

add_executable(occultation-qml-smoke EXCLUDE_FROM_ALL
  smoke/smoke.cc
)
target_link_libraries(occultation-qml-smoke PRIVATE occultation-exoskeleton)

add_executable(occultation-switcher-preview EXCLUDE_FROM_ALL
  preview/preview.cc
  preview_model/preview_model.cc
  preview_model/preview_model.h
  preview_switcher/preview_switcher.cc
  preview_switcher/preview_switcher.h
  preview_thumbnail_item/preview_thumbnail_item.cc
  preview_thumbnail_item/preview_thumbnail_item.h
)
set_target_properties(occultation-switcher-preview PROPERTIES AUTOMOC ON)
target_link_libraries(occultation-switcher-preview PRIVATE
  occultation-exoskeleton
)

# occultation-qml-smoke is not built by default; like `meson test`, build it
# on demand before the test runs.
add_test(NAME occultation-qml-smoke-build
  COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}"
    --target occultation-qml-smoke --config $<CONFIG>
)
set_tests_properties(occultation-qml-smoke-build PROPERTIES
  FIXTURES_SETUP occultation-qml-smoke-binary
)
add_test(NAME occultation-qml-smoke COMMAND occultation-qml-smoke)
set_tests_properties(occultation-qml-smoke PROPERTIES
  LABELS occultation
  FIXTURES_REQUIRED occultation-qml-smoke-binary
  ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=software"
)
