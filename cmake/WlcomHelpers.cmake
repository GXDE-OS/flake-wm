include_guard(GLOBAL)

set(WLCOM_CAPTURE_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/capture.cmake")
set(WLCOM_EMBED_SCRIPT "${PROJECT_SOURCE_DIR}/data/embed.sh")

# Generate headers for <target> that are produced by a command, in the calling
# directory's binary dir, and make that dir an include dir of <target>.
#
# The headers are driven by a per-directory custom target rather than added to
# <target>'s sources, because a custom command only gets a build rule in the
# directory that declares it, while <target> lives in src/.
function(_wlcom_attach_generated_headers target)
  file(RELATIVE_PATH rel "${PROJECT_BINARY_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
  string(MAKE_C_IDENTIFIER "${rel}" rel)
  set(generator "wlcom-generated-${rel}")
  add_custom_target(${generator} DEPENDS ${ARGN})
  add_dependencies(${target} ${generator})
  target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
endfunction()

# wlcom_embed_files(<target> [SUFFIX <suffix>] FILES <file>...)
#
# Turn each file into a C header holding `static const char <name><suffix>[]`
# with data/embed.sh, where <name> is the file name with every character that
# is not valid in a C identifier replaced by '_' (blur.vert -> blur_vert). The
# header is called <name><suffix>.h.
function(wlcom_embed_files target)
  cmake_parse_arguments(PARSE_ARGV 1 ARG "" "SUFFIX" "FILES")

  set(outputs "")
  foreach(file IN LISTS ARG_FILES)
    string(MAKE_C_IDENTIFIER "${file}" var)
    string(APPEND var "${ARG_SUFFIX}")
    set(input "${CMAKE_CURRENT_SOURCE_DIR}/${file}")
    set(output "${CMAKE_CURRENT_BINARY_DIR}/${var}.h")
    add_custom_command(
      OUTPUT "${output}"
      COMMAND "${CMAKE_COMMAND}"
        "-DINPUT=${input}" "-DOUTPUT=${output}"
        -P "${WLCOM_CAPTURE_SCRIPT}" -- "${WLCOM_EMBED_SCRIPT}" "${var}"
      DEPENDS "${input}" "${WLCOM_EMBED_SCRIPT}" "${WLCOM_CAPTURE_SCRIPT}"
      VERBATIM
    )
    list(APPEND outputs "${output}")
  endforeach()

  _wlcom_attach_generated_headers(${target} ${outputs})
endfunction()

# wlcom_capture_header(<target> <output> <command>...)
#
# Generate <output> (relative to the current binary dir) from the stdout of
# <command>. Target names used in <command> are added as dependencies.
function(wlcom_capture_header target output)
  set(output "${CMAKE_CURRENT_BINARY_DIR}/${output}")
  set(depends "${WLCOM_CAPTURE_SCRIPT}")
  set(command "")
  foreach(arg IN LISTS ARGN)
    if(TARGET "${arg}")
      list(APPEND depends "${arg}")
      list(APPEND command "$<TARGET_FILE:${arg}>")
    else()
      list(APPEND command "${arg}")
    endif()
  endforeach()

  add_custom_command(
    OUTPUT "${output}"
    COMMAND "${CMAKE_COMMAND}" "-DOUTPUT=${output}"
      -P "${WLCOM_CAPTURE_SCRIPT}" -- ${command}
    DEPENDS ${depends}
    VERBATIM
  )

  _wlcom_attach_generated_headers(${target} "${output}")
endfunction()
