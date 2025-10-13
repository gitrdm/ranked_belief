# Tests CMakeLists.txt

# Create test executable
add_executable(ranked_belief_tests
    rank_test.cpp
    promise_test.cpp
    ranking_element_test.cpp
    ranking_iterator_test.cpp
    ranking_function_test.cpp
    constructors_test.cpp
    operations/map_test.cpp
    operations/filter_test.cpp
    operations/merge_test.cpp
    operations/merge_apply_test.cpp
    operations/observe_test.cpp
    operations/nrm_exc_test.cpp
    autocast_test.cpp
    operators_test.cpp
    integration_test.cpp
    type_erasure_test.cpp
)

target_link_libraries(ranked_belief_tests
    PRIVATE
        ranked_belief
        GTest::gtest
        GTest::gtest_main
)

# Enable testing
include(GoogleTest)
gtest_discover_tests(ranked_belief_tests)

# Pure C API conformance tests
add_executable(ranked_belief_c_api_test
    c_api_test.c
)
target_include_directories(ranked_belief_c_api_test
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)
target_link_libraries(ranked_belief_c_api_test
    PRIVATE
        ranked_belief_c_api
)
set_target_properties(ranked_belief_c_api_test PROPERTIES LINKER_LANGUAGE CXX)
add_test(NAME ranked_belief_c_api_test COMMAND ranked_belief_c_api_test)
