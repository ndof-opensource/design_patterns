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

# Fetch callable_traits from a Git repository
set(CALLABLE_TRAITS_REPO_URL https://github.com/ndof-opensource/callable_traits.git)
set(CALLABLE_TRAITS_REPO_TAG master) # or any specific tag/branch you want to use

# Cache the current value of FETCHCONTENT_FULLY_DISCONNECTED
set(_original_FETCHCONTENT_FULLY_DISCONNECTED ${FETCHCONTENT_FULLY_DISCONNECTED})

# Temporarily set FETCHCONTENT_FULLY_DISCONNECTED to ON
set(FETCHCONTENT_FULLY_DISCONNECTED ON)

# Fetch callable_traits from a Git repository
include(FetchContent)
FetchContent_Declare(
    callable_traits
    GIT_REPOSITORY ${CALLABLE_TRAITS_REPO_URL}
    GIT_TAG ${CALLABLE_TRAITS_REPO_TAG}
)
FetchContent_MakeAvailable(callable_traits)

# Restore the original value of FETCHCONTENT_FULLY_DISCONNECTED
set(FETCHCONTENT_FULLY_DISCONNECTED ${_original_FETCHCONTENT_FULLY_DISCONNECTED})
unset(_original_FETCHCONTENT_FULLY_DISCONNECTED) # Clean up the temporary variable

# # Install the fetched content into the /include directory
install(DIRECTORY ${callable_traits_SOURCE_DIR}/include/
        DESTINATION ${CMAKE_BINARY_DIR}/includes)

# Include both this project and the callable_traits project
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${callable_traits_SOURCE_DIR}
)
# #TODO: Add this to the callable_traits project.
# #Install header-only library into /includes after test cases run
# add_custom_target(
#     install_headers ALL
#     COMMAND ${CMAKE_COMMAND} -E copy_directory
#     ${CMAKE_CURRENT_SOURCE_DIR}/callable_traits/include
#     ${CMAKE_BINARY_DIR}/includes
#     COMMENT "Installing header-only library into /includes"
# )

 



add_dependencies(install_headers test_proxy)
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

