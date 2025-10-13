cmake_minimum_required(VERSION 3.20)

# Discover Python and pybind11
find_package(Python3 COMPONENTS Interpreter Development.Module REQUIRED)

include(FetchContent)
FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v2.12.0
)
FetchContent_MakeAvailable(pybind11)

set(RB_PY_PACKAGE_NAME "ranked_belief")
set(RB_PY_BUILD_PACKAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/${RB_PY_PACKAGE_NAME}")

# Ensure the pure-Python package files are available alongside the compiled module
add_custom_target(ranked_belief_python_package ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/${RB_PY_PACKAGE_NAME}"
            "${RB_PY_BUILD_PACKAGE_DIR}"
    COMMENT "Copying Python package sources"
)

# Build the extension module
pybind11_add_module(_ranked_belief_core MODULE src/module.cpp)
target_link_libraries(_ranked_belief_core PRIVATE ranked_belief)
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_options(_ranked_belief_core PRIVATE -static-libstdc++ -static-libgcc)
endif()
set_target_properties(_ranked_belief_core PROPERTIES
    OUTPUT_NAME "_ranked_belief_core"
    LIBRARY_OUTPUT_DIRECTORY "${RB_PY_BUILD_PACKAGE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY "${RB_PY_BUILD_PACKAGE_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${RB_PY_BUILD_PACKAGE_DIR}"
)
add_dependencies(_ranked_belief_core ranked_belief_python_package)

# Pytest integration (optional)
find_program(RANKED_BELIEF_PYTEST_EXECUTABLE NAMES pytest)
if(RANKED_BELIEF_PYTEST_EXECUTABLE)
        add_test(
            NAME ranked_belief_python_tests
            COMMAND "${Python3_EXECUTABLE}" -m pytest -q
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        )
        set_tests_properties(ranked_belief_python_tests PROPERTIES
            ENVIRONMENT "PYTHONPATH=${CMAKE_CURRENT_BINARY_DIR}:${CMAKE_CURRENT_SOURCE_DIR}:$ENV{PYTHONPATH}"
        )
else()
    message(WARNING "pytest not found; Python binding tests will be skipped.")
endif()
