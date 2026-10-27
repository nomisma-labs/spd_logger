# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

nom_c2test(logging_tests SOURCES logging_test.cxx config_test.cxx DEPS nom::spd_logger)
if(TARGET ${PROJECT_NAME}_logging_tests)
    nom_enable_std_modules(${PROJECT_NAME}_logging_tests)
endif()
