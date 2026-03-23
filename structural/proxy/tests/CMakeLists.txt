cmake_minimum_required(VERSION 3.20)
project(ProxyTests)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# Try to find system-installed GoogleTest (e.g., Homebrew)
# find_package(GTest QUIET)
find_package(GTest)
if (GTest_FOUND)
    message(STATUS "Found system-installed GoogleTest")
    message(STATUS "GTest_INCLUDE_DIRS: ${GTEST_INCLUDE_DIRS}")
    message(STATUS "GTest_LIBRARIES: ${GTEST_LIBRARIES}")
    message(STATUS "GTest_MAIN_LIBRARIES: ${GTEST_MAIN_LIBRARIES}")
else()
    message(STATUS "System GTest not found. Fetching GoogleTest...")
endif()

# TODO: check to see if GoogleTest is empty.
if (GTest_FOUND)

    message(STATUS "Using system-installed GoogleTest")
    include(GoogleTest)
    enable_testing()
else()
    message(STATUS "System GTest not found. Fetching GoogleTest...")

    include(FetchContent)
    FetchContent_Declare(
      gtest
      URL https://github.com/google/googletest/archive/release-1.12.1.zip
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(gtest)

    include(GoogleTest)
    enable_testing()
endif()

# Fetch callable_traits from a Git repository
set(CALLABLE_TRAITS_REPO_URL https://github.com/ndof-opensource/callable_traits.git)
set(CALLABLE_TRAITS_REPO_TAG main)


# Fetch callable_traits from a Git repository
include(FetchContent)
FetchContent_Declare(
    callable_traits
    GIT_REPOSITORY ${CALLABLE_TRAITS_REPO_URL}
    GIT_TAG ${CALLABLE_TRAITS_REPO_TAG}
)
FetchContent_MakeAvailable(callable_traits)

 
# TODO: Fix this.  Intellisense should be configured to point to the first directory.
#       and the directory should be created if it doesn't exist.
# # Install the fetched content into the /include directory
install(DIRECTORY ${callable_traits_SOURCE_DIR}/include/
        DESTINATION ${callable_traits_SOURCE_DIR}/include/ndof-os/callable_traits
)
 
        

# Include both this project and the callable_traits project
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}

)

 
# Add test target
add_executable(
    test_proxy
    test_proxy.cpp
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

gtest_discover_tests(test_proxy)

