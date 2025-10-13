cmake_minimum_required(VERSION 3.20)
project(ranked_belief VERSION 1.0.0 LANGUAGES C CXX)

# Require C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Require C11 for C-facing components
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Compiler version checks
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "10.0")
        message(FATAL_ERROR "GCC version must be at least 10.0 for C++20 support. Found: ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "13.0")
        message(FATAL_ERROR "Clang version must be at least 13.0 for C++20 support. Found: ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "19.29")
        message(FATAL_ERROR "MSVC version must be at least 19.29 (VS 2019 16.11) for C++20 support. Found: ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "14.0")
        message(FATAL_ERROR "AppleClang version must be at least 14.0 for C++20 support. Found: ${CMAKE_CXX_COMPILER_VERSION}")
    endif()
endif()

# Compiler warnings
if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# Build type default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Options
option(RANKED_BELIEF_BUILD_TESTS "Build tests" ON)
option(RANKED_BELIEF_BUILD_EXAMPLES "Build examples" ON)
option(RANKED_BELIEF_BUILD_PYTHON_BINDINGS "Build Python bindings" OFF)
option(RANKED_BELIEF_BUILD_R_BINDINGS "Build R bindings" OFF)

# Header-only library
add_library(ranked_belief INTERFACE)
target_include_directories(ranked_belief INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(ranked_belief INTERFACE cxx_std_20)

# C API library (bridges ranked_belief to pure C consumers)
add_library(ranked_belief_c_api STATIC
    src/c_api.cpp
)
target_link_libraries(ranked_belief_c_api
    PUBLIC
        ranked_belief
)
target_include_directories(ranked_belief_c_api
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_features(ranked_belief_c_api PUBLIC cxx_std_20)

# Google Test integration
if(RANKED_BELIEF_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    # For Windows: Prevent overriding the parent project's compiler/linker settings
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    
    enable_testing()
    add_subdirectory(tests)
endif()

# Examples
if(RANKED_BELIEF_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Python bindings
if(RANKED_BELIEF_BUILD_PYTHON_BINDINGS)
    add_subdirectory(bindings/python)
endif()

# Installation
install(DIRECTORY include/ranked_belief
    DESTINATION include
    FILES_MATCHING
        PATTERN "*.hpp"
        PATTERN "*.h"
)

install(TARGETS ranked_belief ranked_belief_c_api
    EXPORT ranked_belief-targets
    INCLUDES DESTINATION include
)

install(EXPORT ranked_belief-targets
    FILE ranked_belief-targets.cmake
    NAMESPACE ranked_belief::
    DESTINATION lib/cmake/ranked_belief
)

# CMake package config
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ranked_belief-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ranked_belief-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ranked_belief-config.cmake"
    INSTALL_DESTINATION lib/cmake/ranked_belief
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ranked_belief-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ranked_belief-config-version.cmake"
    DESTINATION lib/cmake/ranked_belief
)
