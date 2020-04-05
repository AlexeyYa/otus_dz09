cmake_minimum_required(VERSION 3.5)

if($ENV{TRAVIS_BUILD_NUMBER})
    project(bayan VERSION 0.1.$ENV{TRAVIS_BUILD_NUMBER} LANGUAGES CXX)
elseif($ENV{APPVEYOR_BUILD_NUMBER})
    project(bayan VERSION 0.1.$ENV{APPVEYOR_BUILD_NUMBER} LANGUAGES CXX)
else()
    project(bayan VERSION 0.1.1 LANGUAGES CXX)
endif()

# Download and unpack googletest at configure time
configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
  RESULT_VARIABLE result
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download )
if(result)
  message(FATAL_ERROR "Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
                 ${CMAKE_CURRENT_BINARY_DIR}/googletest-build
                 EXCLUDE_FROM_ALL)
# End of gtest import

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost)
include_directories(${Boost_INCLUDE_DIR})

configure_file(bintray.json.in bintray.json)

add_executable(bayan main.cpp bayan.cpp)

add_executable(tests tests.cpp)

if($ENV{APPVEYOR_BUILD_NUMBER})
    target_link_libraries(tests PRIVATE gtest gtest_main)
elseif($ENV{TRAVIS_BUILD_NUMBER})
    target_link_libraries(tests PRIVATE gtest gtest_main stdc++fs)
    target_link_libraries(bayan stdc++fs)
    set_target_properties(bayan PROPERTIES COMPILE_DEFINITIONS BOOST_TEST_DYN_LINK)
else()
    target_link_libraries(tests PRIVATE gtest gtest_main)
    set_target_properties(bayan PROPERTIES COMPILE_DEFINITIONS BOOST_TEST_DYN_LINK)
endif()

install(TARGETS bayan DESTINATION bin)

include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_VERSION_MAJOR "${bayan_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${bayan_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${bayan_VERSION_PATCH}")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Alexey")
include(CPack)

enable_testing()
add_test(unit_tests tests)

if (MSVC)
    target_compile_options(
        bayan PRIVATE
        /W4
    )
    add_compile_definitions(_WIN32_WINNT=0x0601)
else ()
    target_compile_options(
        bayan PRIVATE
        -Wall -Wextra -pedantic -Werror
    )
endif()
