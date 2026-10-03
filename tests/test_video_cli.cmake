# A redirected video invocation must fail before emitting terminal controls.
execute_process(
    COMMAND "${PROGRAM}" "${INPUT}" --columns 8
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT status EQUAL 1 OR NOT output STREQUAL "" OR
   NOT error MATCHES "interactive terminal")
    message(FATAL_ERROR "Noninteractive video playback was not rejected: ${status}: ${output}: ${error}")
endif()

# Video exports are unsupported, including valueless -o requests.
foreach(destination IN ITEMS "out.txt" "out.png" "out.mp4")
    execute_process(
        COMMAND "${PROGRAM}" "${INPUT}" -o "${destination}"
        RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
    if(NOT status EQUAL 2 OR NOT output STREQUAL "" OR
       NOT error MATCHES "Video playback does not support --output")
        message(FATAL_ERROR "Video export was not rejected: ${status}: ${output}: ${error}")
    endif()
endforeach()

execute_process(
    COMMAND "${PROGRAM}" "${INPUT}" -o
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT status EQUAL 2 OR NOT output STREQUAL "")
    message(FATAL_ERROR "Valueless video output was not rejected")
endif()
