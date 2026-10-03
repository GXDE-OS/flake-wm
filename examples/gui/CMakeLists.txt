if(NOT CMAKE_CXX_COMPILER)
  return()
endif()

find_package(Qt5 QUIET COMPONENTS Core Gui Widgets Qml Quick QuickWidgets)
if(NOT Qt5_FOUND)
  return()
endif()

pkg_check_modules(GUI_EXAMPLE_DEPS REQUIRED IMPORTED_TARGET egl glesv2 libdrm)

qt5_add_resources(gui_resources qml.qrc)

set(gui_qt_libraries
  Qt5::Core
  Qt5::Gui
  Qt5::Widgets
  Qt5::Qml
  Qt5::Quick
  Qt5::QuickWidgets
)

add_executable(kywc-gui
  main.cpp
  context.cpp
  context.h
  ui.cpp
  widget.h
  workspace.cpp
  output.cpp
  toplevel.cpp
  thumbnail.cpp
  ThumbnailItem.cpp
  ThumbnailItem.h
  ThumInfo.cpp
  ThumInfo.h
  ${gui_resources}
)

add_executable(kywc-gui-thumbnail
  ThumMain.cpp
  context.cpp
  context.h
  workspace.cpp
  output.cpp
  toplevel.cpp
  thumbnail.cpp
  ThumbnailItem.cpp
  ThumbnailItem.h
  ThumInfo.cpp
  ThumInfo.h
  ${gui_resources}
)

foreach(target kywc-gui kywc-gui-thumbnail)
  set_target_properties(${target} PROPERTIES AUTOMOC ON)
  target_compile_options(${target} PRIVATE -Wno-unused-parameter)
  target_link_libraries(${target} PRIVATE
    ${gui_qt_libraries}
    kywc
    PkgConfig::GUI_EXAMPLE_DEPS
  )
  # For <qpa/qplatformnativeinterface.h>.
  target_include_directories(${target} PRIVATE ${Qt5Gui_PRIVATE_INCLUDE_DIRS})
endforeach()

# Meson named this one `thumbnail` too, in its own build subdirectory; CMake
# target names are global, so keep the file name but not the target name.
set_target_properties(kywc-gui-thumbnail PROPERTIES OUTPUT_NAME thumbnail)
