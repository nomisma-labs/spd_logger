# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: MIT

nom_lib(spd_logger IMPORT_STD
    MODULES spd_logger.cppm
    SOURCES logging.cpp logger_state.cpp log_config.cpp
    HEADERS logging.h
    PUBLIC_DEPS nom::config_types fmt::fmt
    PRIVATE_DEPS spdlog::spdlog)

if(nom_install)
    nom_install_package(TARGETS spd_logger DEPENDENCIES base "fmt 12" spdlog)
endif()
