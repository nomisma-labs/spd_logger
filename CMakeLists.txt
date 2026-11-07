# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

cmake_minimum_required(VERSION 4.4)

include(cmake/kit/std_modules.cmake)

# The version lives only here; releases are tagged v<version> (docs/kit/build.md#versioning).
project(spd_logger
    VERSION 0.7.0
    DESCRIPTION "Synchronous single-threaded application logging for Nomisma"
    LANGUAGES CXX)

include(cmake/kit/nom_kit.cmake)
nom_project_options()
# Populate directory-local install paths even when a sibling already included the shared kit.
include(GNUInstallDirs)

# Dependencies come from external/ submodules (docs/kit/howto.md#a-dependency). In a superbuild the
# outermost FetchContent_Declare wins, so every repo shares one copy.

nom_require_submodule(external/base)
FetchContent_Declare(base SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/base)
FetchContent_MakeAvailable(base)

# spdlog uses the separate fmt dependency; fmt's C++ module is not needed by its headers.
nom_require_submodule(external/fmt)
set(FMT_MODULE OFF)
set(FMT_INSTALL ${nom_install})
FetchContent_Declare(fmt SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/fmt SYSTEM)
FetchContent_MakeAvailable(fmt)

nom_require_submodule(external/spdlog)
set(SPDLOG_FMT_EXTERNAL ON)
set(SPDLOG_BUILD_PIC ON)
# All logger operations are confined to the application thread; thread IDs are not printed.
set(SPDLOG_NO_ATOMIC_LEVELS ON)
set(SPDLOG_NO_THREAD_ID ON)
set(SPDLOG_INSTALL ${nom_install})
FetchContent_Declare(spdlog SOURCE_DIR ${PROJECT_SOURCE_DIR}/external/spdlog SYSTEM)
FetchContent_MakeAvailable(spdlog)
if(nom_install)
    install(FILES ${spdlog_SOURCE_DIR}/LICENSE
        DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/spdlog)
    install(FILES ${fmt_SOURCE_DIR}/LICENSE
        DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/fmt)
endif()

# Discover Catch2 before any code directory can register an inline test suite.
if(nom_build_tests)
    enable_testing()
    nom_find_catch2()
endif()

add_subdirectory(src)

if(nom_build_apps)
    add_subdirectory(apps)
endif()

if(nom_build_tests)
    add_subdirectory(tests)
endif()
