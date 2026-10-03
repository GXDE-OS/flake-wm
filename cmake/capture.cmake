if(NOT DEFINED OUTPUT)
  message(FATAL_ERROR "capture.cmake: OUTPUT is not set")
endif()

set(_command "")
set(_collect FALSE)
math(EXPR _last "${CMAKE_ARGC} - 1")

foreach(_i RANGE ${_last})
  if(_collect)
    list(APPEND _command "${CMAKE_ARGV${_i}}")
  elseif(CMAKE_ARGV${_i} STREQUAL "--")
    set(_collect TRUE)
  endif()
endforeach()

if(NOT _command)
  message(FATAL_ERROR "capture.cmake: no command given after --")
endif()

set(_input_args "")
if(DEFINED INPUT)
  set(_input_args INPUT_FILE "${INPUT}")
endif()

execute_process(
  COMMAND ${_command}
  ${_input_args}
  OUTPUT_FILE "${OUTPUT}.tmp"
  RESULT_VARIABLE _result
)

if(NOT _result EQUAL 0)
  file(REMOVE "${OUTPUT}.tmp")
  message(FATAL_ERROR "capture.cmake: '${_command}' failed: ${_result}")
endif()

file(RENAME "${OUTPUT}.tmp" "${OUTPUT}")
