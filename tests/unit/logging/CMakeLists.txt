# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

nom_c2test(logging_tests IMPORT_STD SOURCES logging_test.cxx config_test.cxx DEPS nom::spd_logger)
