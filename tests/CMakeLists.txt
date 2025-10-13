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
