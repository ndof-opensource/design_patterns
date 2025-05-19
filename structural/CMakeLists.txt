cmake_minimum_required(VERSION 3.20)
project(ProxyTests)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

# Fetch the library
FetchContent_Declare(
    callable_traits
    GIT_REPOSITORY https://github.com/ndof-opensource/callable_traits.git
    GIT_TAG main  # or any specific tag/commit
)

# Make it available
FetchContent_MakeAvailable(callable_traits)

# Try to find system-installed GoogleTest (e.g., Homebrew)
find_package(GTest QUIET)

if (GTest_FOUND)
    message(STATUS "Using system-installed GoogleTest")
    # Try different module names that might be available
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/GoogleTest.cmake")
        include(GoogleTest)
    elseif(EXISTS "${CMAKE_CURRENT_LIST_DIR}/gtest.cmake")
        include(gtest)
    else()
        # If no module is found, we can still use the targets directly
        message(STATUS "No GoogleTest module found, using targets directly")
        # The GTest::gtest_main target should be available from find_package
        if(NOT TARGET GTest::gtest_main)
            # If the target isn't available, we need to fetch it
            message(STATUS "GTest targets not found, falling back to FetchContent")
            FetchContent_Declare(
              googletest
              URL https://github.com/google/googletest/archive/release-1.12.1.zip
            )
            set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
            FetchContent_MakeAvailable(googletest)
            include(GoogleTest)
        endif()
    endif()
    enable_testing()
else()
    message(STATUS "System GTest not found. Fetching GoogleTest...")

    FetchContent_Declare(
      googletest
      URL https://github.com/google/googletest/archive/release-1.12.1.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    # When using FetchContent, we know the module will be available
    include(GoogleTest)
    enable_testing()
endif()

# Include only this project's directory
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add test target
add_executable(
    test_proxy
    tests/test_proxy.cpp
    proxy.hpp
)

# Link callable_traits to the test target
target_link_libraries(test_proxy PRIVATE callable_traits)

# Add callable_traits include directories
target_include_directories(test_proxy PRIVATE ${callable_traits_SOURCE_DIR}/include)

if (GTest_FOUND)
    target_link_libraries(test_proxy PRIVATE GTest::gtest_main)
else()
    target_link_libraries(test_proxy PRIVATE gtest_main)
endif()

# Check if gtest_discover_tests is available
if(COMMAND gtest_discover_tests)
    gtest_discover_tests(test_proxy)
else()
    # Fallback to basic test registration
    add_test(NAME test_proxy COMMAND test_proxy)
endif()

