# Counterpart of Meson's i18n.gettext(preset: 'glib'): build and install the
# catalogs listed in LINGUAS, plus <project>-pot / <project>-update-po helpers.

file(STRINGS LINGUAS linguas_lines REGEX "^[^#]")
set(linguas "")
foreach(line IN LISTS linguas_lines)
  string(REGEX MATCHALL "[^ \t]+" line_linguas "${line}")
  list(APPEND linguas ${line_linguas})
endforeach()

set(gmo_files "")
foreach(lang IN LISTS linguas)
  set(po "${CMAKE_CURRENT_SOURCE_DIR}/${lang}.po")
  set(gmo "${CMAKE_CURRENT_BINARY_DIR}/${lang}.gmo")
  add_custom_command(
    OUTPUT "${gmo}"
    COMMAND "${MSGFMT_EXECUTABLE}" -o "${gmo}" "${po}"
    DEPENDS "${po}"
    VERBATIM
  )
  install(FILES "${gmo}"
    DESTINATION ${CMAKE_INSTALL_LOCALEDIR}/${lang}/LC_MESSAGES
    RENAME ${PROJECT_NAME}.mo
  )
  list(APPEND gmo_files "${gmo}")
endforeach()
add_custom_target(${PROJECT_NAME}-gmo ALL DEPENDS ${gmo_files})

find_program(XGETTEXT_EXECUTABLE xgettext)
find_program(MSGMERGE_EXECUTABLE msgmerge)

set(pot "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.pot")
if(XGETTEXT_EXECUTABLE)
  add_custom_target(${PROJECT_NAME}-pot
    COMMAND "${XGETTEXT_EXECUTABLE}"
      "--package-name=${PROJECT_NAME}"
      "--files-from=${CMAKE_CURRENT_SOURCE_DIR}/POTFILES.in"
      "--directory=${PROJECT_SOURCE_DIR}"
      "--output=${pot}"
      --add-comments=TRANSLATORS
      --keyword=tr
      --from-code=UTF-8
      --keyword=_
      --keyword=N_
      --keyword=C_:1c,2
      --keyword=NC_:1c,2
      --keyword=g_dcgettext:2
      --keyword=g_dngettext:2,3
      --keyword=g_dpgettext2:2c,3
      --flag=N_:1:pass-c-format
      --flag=C_:2:pass-c-format
      --flag=NC_:2:pass-c-format
      --flag=g_dngettext:2:pass-c-format
      --flag=g_strdup_printf:1:c-format
      --flag=g_string_printf:2:c-format
      --flag=g_string_append_printf:2:c-format
      --flag=g_error_new:3:c-format
      --flag=g_set_error:4:c-format
      --flag=g_markup_printf_escaped:1:c-format
      --flag=g_log:3:c-format
      --flag=g_print:1:c-format
      --flag=g_printerr:1:c-format
      --flag=g_printf:1:c-format
      --flag=g_fprintf:2:c-format
      --flag=g_sprintf:2:c-format
      --flag=g_snprintf:3:c-format
    COMMENT "Generating ${PROJECT_NAME}.pot"
    VERBATIM
  )
endif()

if(MSGMERGE_EXECUTABLE)
  set(update_commands "")
  foreach(lang IN LISTS linguas)
    list(APPEND update_commands
      COMMAND "${MSGMERGE_EXECUTABLE}" -q -o
        "${CMAKE_CURRENT_SOURCE_DIR}/${lang}.po"
        "${CMAKE_CURRENT_SOURCE_DIR}/${lang}.po"
        "${pot}"
    )
  endforeach()
  add_custom_target(${PROJECT_NAME}-update-po
    ${update_commands}
    COMMENT "Updating translations from ${PROJECT_NAME}.pot"
    VERBATIM
  )
  if(TARGET ${PROJECT_NAME}-pot)
    add_dependencies(${PROJECT_NAME}-update-po ${PROJECT_NAME}-pot)
  endif()
endif()
