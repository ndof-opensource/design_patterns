cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Always fetch GTest to ensure ABI compatibility with the active compiler.
include(FetchContent)
FetchContent_Declare(
    gtest
    URL https://github.com/google/googletest/archive/release-1.12.1.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(gtest)

include(GoogleTest)
enable_testing()

add_executable(test_icloneable test_icloneable.cpp)
target_link_libraries(test_icloneable PRIVATE gtest_main)

gtest_discover_tests(test_icloneable)
