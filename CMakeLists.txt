# Copyright 2026 ALTESSA SOLUTIONS INC.
# Use of this source code is governed by an MIT-style license that can be
# found in the LICENSE file.

cmake_minimum_required(VERSION 3.21)

# The version lives in the public header and nowhere else: read out of it
# here rather than written a second time, so that a release is one edit and the
# library cannot report a number its package does not carry.
foreach(part MAJOR MINOR PATCH)
    file(STRINGS include/rmmarks/rmmarks.h line
         REGEX "^#define RMMARKS_VERSION_${part} [0-9]+$")
    if(NOT line MATCHES "^#define RMMARKS_VERSION_${part} ([0-9]+)$")
        message(FATAL_ERROR "RMMARKS_VERSION_${part} not found in include/rmmarks/rmmarks.h")
    endif()
    set(RMMARKS_VERSION_${part} "${CMAKE_MATCH_1}")
endforeach()

project(rmmarks
    VERSION ${RMMARKS_VERSION_MAJOR}.${RMMARKS_VERSION_MINOR}.${RMMARKS_VERSION_PATCH}
    DESCRIPTION "Geometry and text matching for reMarkable highlights"
    HOMEPAGE_URL "https://github.com/altessa-s/rmmarks-lib"
    LANGUAGES C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
    set(RMMARKS_TOP_LEVEL ON)
else()
    set(RMMARKS_TOP_LEVEL OFF)
endif()

option(RMMARKS_BUILD_TESTS "Build the test suite" ${RMMARKS_TOP_LEVEL})
option(RMMARKS_WERROR "Turn compiler warnings into errors" OFF)
option(RMMARKS_BUILD_FUZZERS "Build the libFuzzer targets (clang only)" OFF)
if(RMMARKS_BUILD_FUZZERS AND NOT RMMARKS_BUILD_TESTS)
    message(FATAL_ERROR "RMMARKS_BUILD_FUZZERS requires RMMARKS_BUILD_TESTS")
endif()

# ---------------------------------------------------------------------------
# The library
# ---------------------------------------------------------------------------

set(RMMARKS_SOURCES src/rmm_support.c src/rmm_model.c src/rmm_unicode.c src/rmm_unicode_data.c src/rmm_text.c)
set(RMMARKS_PUBLIC_HEADERS include/rmmarks/rmmarks.h include/rmmarks/export.h
    include/rmmarks/types.h include/rmmarks/text.h)

# Honours BUILD_SHARED_LIBS: static by default, shared with
# -DBUILD_SHARED_LIBS=ON. One target either way, so the exported CMake package
# and the pkg-config file describe whatever was built.
add_library(rmmarks ${RMMARKS_SOURCES})
add_library(RMMarks::rmmarks ALIAS rmmarks)

target_include_directories(rmmarks
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)

set_target_properties(rmmarks PROPERTIES
    C_STANDARD 23
    C_STANDARD_REQUIRED ON
    C_EXTENSIONS OFF
    # The plumbing — the arena, the matching scratch storage, the Unicode tables — is not part of
    # the contract and must not reach a consumer's symbol table.
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
    POSITION_INDEPENDENT_CODE ON
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    PUBLIC_HEADER "${RMMARKS_PUBLIC_HEADERS}")

target_compile_features(rmmarks PUBLIC c_std_23)
if(BUILD_SHARED_LIBS)
    target_compile_definitions(rmmarks PRIVATE RMMARKS_BUILDING_SHARED
                                      INTERFACE RMMARKS_USING_SHARED)
endif()

# The warning set, and it is one set: the library is built with clang or
# gcc on every platform, Windows included. MSVC's C compiler stops at the
# first `constexpr`, `nullptr`, `[[nodiscard]]` and typed enum — all of them
# C23, all of them throughout — and a build that carries a second warning
# dialect for a compiler that cannot read the code is a build with a branch
# nobody can exercise.
target_compile_options(rmmarks PRIVATE
    -Wall -Wextra -Wshadow -Wconversion -Wsign-conversion
    -Wstrict-prototypes -Wmissing-prototypes -Wpointer-arith -Wcast-qual)
if(RMMARKS_WERROR)
    target_compile_options(rmmarks PRIVATE -Werror)
endif()
# Standard math functions are in libm on platforms that split it out.
include(CheckLibraryExists)
check_library_exists(m ceil "" RMMARKS_HAVE_LIBM)
if(RMMARKS_HAVE_LIBM)
    target_link_libraries(rmmarks PUBLIC m)
endif()

# ---------------------------------------------------------------------------
# The tests
# ---------------------------------------------------------------------------

if(RMMARKS_BUILD_TESTS)
    enable_testing()

    # Internal tests exercise allocation failures and UTF-8 validation.
    # Compile a test-only copy to keep fault-injection hooks out of the ABI.
    add_library(rmmarks_testing STATIC EXCLUDE_FROM_ALL ${RMMARKS_SOURCES})
    target_compile_definitions(rmmarks_testing PUBLIC RMMARKS_TESTING)
    target_include_directories(rmmarks_testing PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src)
    set_target_properties(rmmarks_testing PROPERTIES
        C_STANDARD 23 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
    if(RMMARKS_HAVE_LIBM)
        target_link_libraries(rmmarks_testing PUBLIC m)
    endif()

    foreach(suite model passages blocks brought errors unicode fuzz_smoke)
        add_executable(test_${suite} EXCLUDE_FROM_ALL tests/test_${suite}.c)
        target_link_libraries(test_${suite} PRIVATE rmmarks_testing)
        target_include_directories(test_${suite} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
        set_target_properties(test_${suite} PROPERTIES
            C_STANDARD 23 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
        add_test(NAME ${suite} COMMAND test_${suite})
    endforeach()

    target_sources(test_fuzz_smoke PRIVATE tests/fuzz_marks.c)
    set_tests_properties(model passages blocks brought errors unicode fuzz_smoke PROPERTIES TIMEOUT 30)

    if(RMMARKS_BUILD_FUZZERS)
        if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
            message(FATAL_ERROR "libFuzzer requires clang")
        endif()
        add_executable(fuzz_marks tests/fuzz_marks.c)
        target_link_libraries(fuzz_marks PRIVATE rmmarks_testing)
        target_compile_options(fuzz_marks PRIVATE -fsanitize=fuzzer)
        target_link_options(fuzz_marks PRIVATE -fsanitize=fuzzer)
        set_target_properties(fuzz_marks PROPERTIES C_STANDARD 23 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
    endif()

    # `ctest` builds nothing on its own, and a suite that was never compiled is
    # a suite that silently did not run.
    add_custom_target(build_tests DEPENDS test_model test_passages test_blocks
        test_brought test_errors test_unicode test_fuzz_smoke)
    # Under a multi-configuration generator — Xcode, Visual Studio — a
    # `ctest` that is not told which configuration to run finds no tests at
    # all and says so five times, so the configuration is passed along. Passed
    # conditionally: under a single-configuration generator with no build type
    # set, `$<CONFIG>` is empty, and `-C ""` is an error of its own. The
    # expression yields the two arguments or nothing, and COMMAND_EXPAND_LISTS
    # is what lets "nothing" vanish rather than arrive as an empty argument.
    add_custom_target(check
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
                "$<$<BOOL:$<CONFIG>>:-C;$<CONFIG>>"
        DEPENDS build_tests
        COMMAND_EXPAND_LISTS)
endif()

# ---------------------------------------------------------------------------
# Installation
# ---------------------------------------------------------------------------

install(TARGETS rmmarks
    EXPORT RMMarksTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/rmmarks)


install(EXPORT RMMarksTargets
    FILE RMMarksTargets.cmake
    NAMESPACE RMMarks::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RMMarks)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/RMMarksConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/RMMarksConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RMMarks)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/RMMarksConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/RMMarksConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/RMMarksConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/RMMarks)

# pkg-config, for the builds that never learned CMake.
#
# The way back from the `.pc` to the prefix is worked out here rather than
# written as a fixed `../..`: CMAKE_INSTALL_LIBDIR is `lib` on one
# distribution, `lib64` on another and `lib/x86_64-linux-gnu` on a third, and
# only one of those is two levels down.
file(RELATIVE_PATH RMMARKS_PC_PREFIX_RELATIVE
     "${CMAKE_INSTALL_FULL_LIBDIR}/pkgconfig" "${CMAKE_INSTALL_PREFIX}")
if(RMMARKS_PC_PREFIX_RELATIVE STREQUAL "")
    set(RMMARKS_PC_PREFIX_RELATIVE ".")
endif()
string(REGEX REPLACE "/$" "" RMMARKS_PC_PREFIX_RELATIVE "${RMMARKS_PC_PREFIX_RELATIVE}")

set(RMMARKS_PC_LIBS_PRIVATE "")
set(RMMARKS_PC_CFLAGS "")
if(WIN32 AND BUILD_SHARED_LIBS)
    set(RMMARKS_PC_CFLAGS "-DRMMARKS_USING_SHARED")
endif()
if(RMMARKS_HAVE_LIBM)
    set(RMMARKS_PC_LIBS_PRIVATE "-lm")
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/rmmarks.pc.in
               ${CMAKE_CURRENT_BINARY_DIR}/rmmarks.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/rmmarks.pc
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

install(FILES LICENSE COPYRIGHT docs/UNICODE-LICENSE.txt
        DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/rmmarks)
