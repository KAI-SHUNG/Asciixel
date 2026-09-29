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

# Default output stays beside the input, preserving a multi-dot UTF-8 stem.
set(default_input "${CMAKE_CURRENT_BINARY_DIR}/测试.example.png")
set(default_output "${CMAKE_CURRENT_BINARY_DIR}/测试.example_asciixel.png")
file(COPY_FILE "${INPUT}" "${default_input}")
foreach(flag IN ITEMS -o --output)
    file(REMOVE "${default_output}")
    execute_process(COMMAND "${PROGRAM}" "${default_input}" ${flag} --columns 1
        RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
    if(NOT result EQUAL 0 OR NOT stdout STREQUAL "" OR NOT stderr STREQUAL "")
        message(FATAL_ERROR "Default output failed: ${result}: ${stderr}")
    endif()
    file(READ "${default_output}" signature LIMIT 8 HEX)
    if(NOT signature STREQUAL "89504e470d0a1a0a")
        message(FATAL_ERROR "Default output is not PNG")
    endif()
endforeach()
file(SHA256 "${default_output}" before)
execute_process(COMMAND "${PROGRAM}" "${default_input}" -o
    RESULT_VARIABLE result OUTPUT_QUIET ERROR_QUIET)
file(SHA256 "${default_output}" after)
if(NOT result EQUAL 1 OR NOT before STREQUAL after)
    message(FATAL_ERROR "Default output must not overwrite existing files")
endif()
file(REMOVE "${default_input}" "${default_output}")

set(text_output "${CMAKE_CURRENT_BINARY_DIR}/文本.TXT")
file(REMOVE "${text_output}")
execute_process(COMMAND "${PROGRAM}" "${INPUT}"
    RESULT_VARIABLE result OUTPUT_VARIABLE terminal ERROR_VARIABLE stderr)
execute_process(COMMAND "${PROGRAM}" "${INPUT}" -o "${text_output}"
    RESULT_VARIABLE result OUTPUT_VARIABLE stdout ERROR_VARIABLE stderr)
if(NOT result EQUAL 0 OR NOT stdout STREQUAL "" OR NOT stderr STREQUAL "")
    message(FATAL_ERROR "TXT export failed: ${result}: ${stderr}")
endif()
file(READ "${text_output}" text)
if(NOT text STREQUAL terminal)
    message(FATAL_ERROR "TXT must match terminal text")
endif()
file(SHA256 "${text_output}" before)
execute_process(COMMAND "${PROGRAM}" "${INPUT}" -o "${text_output}"
    RESULT_VARIABLE result OUTPUT_QUIET ERROR_QUIET)
file(SHA256 "${text_output}" after)
if(NOT result EQUAL 1 OR NOT before STREQUAL after)
    message(FATAL_ERROR "TXT must not overwrite existing files")
endif()
file(REMOVE "${text_output}")
if(help MATCHES "--format" OR NOT help MATCHES "\\.txt")
    message(FATAL_ERROR "Help must describe TXT and omit the removed format option")
endif()
