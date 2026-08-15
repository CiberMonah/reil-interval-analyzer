add_executable(reil-tests tests.cpp)
target_link_libraries(reil-tests PRIVATE reil-core)
target_compile_definitions(reil-tests PRIVATE
    TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
    PROJECT_SOURCE_DIR_PATH="${PROJECT_SOURCE_DIR}"
)
add_test(NAME unit_and_algorithm_tests COMMAND reil-tests)

function(add_interval_test name file lower upper expected)
    add_test(NAME ${name}
        COMMAND reil-analyzer ${file} ${lower} ${upper}
    )
    set_tests_properties(${name} PROPERTIES
        PASS_REGULAR_EXPRESSION "Output: [[]${expected}[]]"
    )
endfunction()

add_interval_test(testcase_0_2
    ${PROJECT_SOURCE_DIR}/examples/testcase.reil 0 2 "0, 2")
add_interval_test(testcase_1_5
    ${PROJECT_SOURCE_DIR}/examples/testcase.reil 1 5 "1, 5")

foreach(op jge jg jle jl)
    add_interval_test(
        jump_${op}_split
        ${CMAKE_CURRENT_SOURCE_DIR}/${op}.reil
        2 4 "0, 1"
    )
endforeach()

add_interval_test(jump_jge_true ${CMAKE_CURRENT_SOURCE_DIR}/jge.reil 3 3 "1, 1")
add_interval_test(jump_jge_false ${CMAKE_CURRENT_SOURCE_DIR}/jge.reil 2 2 "0, 0")
add_interval_test(jump_jg_true ${CMAKE_CURRENT_SOURCE_DIR}/jg.reil 4 4 "1, 1")
add_interval_test(jump_jg_false ${CMAKE_CURRENT_SOURCE_DIR}/jg.reil 3 3 "0, 0")
add_interval_test(jump_jle_true ${CMAKE_CURRENT_SOURCE_DIR}/jle.reil 3 3 "1, 1")
add_interval_test(jump_jle_false ${CMAKE_CURRENT_SOURCE_DIR}/jle.reil 4 4 "0, 0")
add_interval_test(jump_jl_true ${CMAKE_CURRENT_SOURCE_DIR}/jl.reil 2 2 "1, 1")
add_interval_test(jump_jl_false ${CMAKE_CURRENT_SOURCE_DIR}/jl.reil 3 3 "0, 0")
add_interval_test(register_comparison
    ${CMAKE_CURRENT_SOURCE_DIR}/jge_register.reil 2 4 "0, 1")
