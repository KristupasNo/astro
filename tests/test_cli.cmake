if(NOT DEFINED TLMFRAME)
    message(FATAL_ERROR "TLMFRAME executable path is required")
endif()

function(expect_success command input expected)
    execute_process(
        COMMAND "${TLMFRAME}" "${command}" "${input}"
        RESULT_VARIABLE status
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )
    if(NOT status EQUAL 0 OR NOT output STREQUAL "${expected}\n" OR NOT error STREQUAL "")
        message(FATAL_ERROR "${command} failed: status=${status}, output=${output}, error=${error}")
    endif()
endfunction()

function(expect_failure)
    execute_process(
        COMMAND "${TLMFRAME}" ${ARGV}
        RESULT_VARIABLE status
        OUTPUT_VARIABLE output
        ERROR_VARIABLE error
    )
    if(status EQUAL 0 OR NOT output STREQUAL "" OR error STREQUAL "")
        message(FATAL_ERROR "Expected failure for '${ARGV}': status=${status}, output=${output}, error=${error}")
    endif()
endfunction()

expect_success(encode "abcd" "1ACFFC1D0002ABCD23C5")
expect_success(decode "1acffc1d0002abcd23c5" "ABCD")
expect_success(encode "" "1ACFFC1D00001D0F")
expect_success(decode "1ACFFC1D00001D0F" "")

execute_process(
    COMMAND "${TLMFRAME}" --version
    RESULT_VARIABLE status
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error
)
if(NOT status EQUAL 0 OR NOT output STREQUAL "tlmframe 0.1.0\n" OR NOT error STREQUAL "")
    message(FATAL_ERROR "Unexpected --version result: ${output}${error}")
endif()

expect_failure()
expect_failure(encode)
expect_failure(encode AB extra)
expect_failure(unknown AB)
expect_failure(--version extra)
expect_failure(encode A)
expect_failure(encode GG)
expect_failure(encode "AB CD")
expect_failure(decode 1ACFFC1D0002ABCD23)
expect_failure(decode 1ACFFC1D0002AACD23C5)

string(REPEAT "AB" 1025 oversized_payload)
expect_failure(encode "${oversized_payload}")
