cmake_minimum_required(VERSION 3.20)
project(ProxyTests)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Try to find system-installed GoogleTest (e.g., Homebrew)
find_package(GTest QUIET)

if (GTest_FOUND)
    message(STATUS "Using system-installed GoogleTest")
    include(GoogleTest)
    enable_testing()
else()
    message(STATUS "System GTest not found. Fetching GoogleTest...")

    include(FetchContent)
    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/release-1.12.1.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    include(GoogleTest)
    enable_testing()
endif()

# Include both this project and the callable_traits project
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../callable_traits
)

# Add test target
add_executable(
    test_proxy
    tests/test_proxy.cpp
    proxy.hpp
)

if (GTest_FOUND)
    target_link_libraries(test_proxy GTest::gtest_main)
else()
    target_link_libraries(test_proxy gtest_main)
endif()

gtest_discover_tests(test_proxy)

