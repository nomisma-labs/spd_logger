# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

cmake_minimum_required(VERSION 3.30)
project(spd_logger_library_usage LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(LIBRARY_USAGE_MODE STREQUAL "package")
    find_package(spd_logger CONFIG REQUIRED)
else()
    include(FetchContent)
    FetchContent_Declare(spd_logger SOURCE_DIR ${LIBRARY_USAGE_SOURCE_DIR})
    FetchContent_MakeAvailable(spd_logger)
endif()

enable_testing()

add_executable(use_library use_library_test.cxx)
target_link_libraries(use_library PRIVATE nom::spd_logger)
add_test(NAME use_library COMMAND use_library)

# This consumer must see the enum and configuration types without the macro header.
add_executable(import_only import_only_test.cxx)
target_link_libraries(import_only PRIVATE nom::spd_logger)
add_test(NAME import_only COMMAND import_only)

# Initialization misuse is an assertion contract; Release must never execute it.
string(TOLOWER "${CMAKE_BUILD_TYPE}" test_build_type)
if(CMAKE_CONFIGURATION_TYPES OR test_build_type STREQUAL "debug")
    add_executable(uninitialized uninitialized_test.cxx)
    target_link_libraries(uninitialized PRIVATE nom::spd_logger)
    set(assertion_configurations)
    if(CMAKE_CONFIGURATION_TYPES)
        set(assertion_configurations CONFIGURATIONS Debug)
    endif()
    foreach(operation IN ITEMS global log debug set flush prechecked)
        add_test(NAME uninitialized_${operation}
            COMMAND ${CMAKE_COMMAND}
                -DCONTRACT_EXECUTABLE=$<TARGET_FILE:uninitialized>
                -DCONTRACT_OPERATION=${operation}
                "-DCONTRACT_DIAGNOSTIC=not initialized"
                -P ${CMAKE_CURRENT_SOURCE_DIR}/check_contract_failure.cmake
            ${assertion_configurations})
    endforeach()
endif()
