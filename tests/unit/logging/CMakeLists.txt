# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: MIT

nom_c2test(logging_tests IMPORT_STD SOURCES logging_test.cxx noexcept_test.cxx config_test.cxx DEPS nom::spd_logger)
