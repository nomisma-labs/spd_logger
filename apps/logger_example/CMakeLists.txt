# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

nom_app(logger_example SOURCES main.cpp DEPS nom::spd_logger)
if(TARGET logger_example)
    nom_enable_std_modules(logger_example)
endif()
