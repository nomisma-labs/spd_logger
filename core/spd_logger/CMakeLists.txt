# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

nom_lib(spd_logger
    MODULES spd_logger.cppm
    SOURCES logging.cpp logger_state.cpp
    HEADERS logging.h
    PUBLIC_DEPS nom::config_types spdlog::spdlog)

if(nom_install)
    nom_install_package(TARGETS spd_logger DEPENDENCIES base "fmt 12" spdlog)
endif()
