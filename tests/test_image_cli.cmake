execute_process(
    COMMAND "${PROGRAM}" "${INPUT}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)

if(NOT result EQUAL 0)
    message(FATAL_ERROR "image CLI failed: ${result}: ${error}")
endif()
if(NOT error STREQUAL "")
    message(FATAL_ERROR "image CLI wrote to stderr: ${error}")
endif()
string(LENGTH "${output}" output_length)
if(NOT output_length EQUAL 3)
    message(FATAL_ERROR "expected two characters and LF, got ${output_length} bytes")
endif()
string(SUBSTRING "${output}" 2 1 line_end)
if(NOT line_end STREQUAL "\n")
    message(FATAL_ERROR "image CLI did not end the row with LF")
endif()

set(unicode_input "${CMAKE_CURRENT_BINARY_DIR}/测试图像.png")
file(COPY_FILE "${INPUT}" "${unicode_input}")
execute_process(
    COMMAND "${PROGRAM}" "${unicode_input}"
    RESULT_VARIABLE unicode_result
    OUTPUT_VARIABLE unicode_output
    ERROR_VARIABLE unicode_error)
if(NOT unicode_result EQUAL 0)
    message(FATAL_ERROR "Unicode image path failed: ${unicode_result}: ${unicode_error}")
endif()
if(NOT unicode_output STREQUAL output)
    message(FATAL_ERROR "Unicode path changed image output")
endif()

execute_process(COMMAND "${PROGRAM}" --help
    RESULT_VARIABLE result OUTPUT_VARIABLE help ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT error STREQUAL "" OR NOT help MATCHES "--columns")
    message(FATAL_ERROR "Help must succeed without opening an input: ${error}")
endif()
execute_process(COMMAND "${PROGRAM}" "${INPUT}" --columns 1 --font-size 32
    RESULT_VARIABLE result OUTPUT_VARIABLE narrowed ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT error STREQUAL "")
    message(FATAL_ERROR "Custom settings failed: ${error}")
endif()
string(LENGTH "${narrowed}" narrowed_length)
if(NOT narrowed_length EQUAL 2)
    message(FATAL_ERROR "Expected one character and LF with --columns 1")
endif()
execute_process(COMMAND "${PROGRAM}" "${INPUT}" --columns 0
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT result EQUAL 2 OR NOT output STREQUAL "" OR error STREQUAL "")
    message(FATAL_ERROR "Invalid config must be reported on stderr with exit code 2")
endif()

execute_process(COMMAND "${PROGRAM}" -h
    RESULT_VARIABLE result OUTPUT_VARIABLE short_help ERROR_VARIABLE error)
if(NOT result EQUAL 0 OR NOT error STREQUAL "" OR NOT short_help STREQUAL help)
    message(FATAL_ERROR "-h must produce the same help as --help")
endif()
