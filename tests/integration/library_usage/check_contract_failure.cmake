# SPDX-FileCopyrightText: 2026 Alexey Tkachenko
# SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

execute_process(
    COMMAND "${CONTRACT_EXECUTABLE}" "${CONTRACT_OPERATION}"
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE diagnostic)

if(NOT "${result}" STREQUAL "0")
    message(FATAL_ERROR "Expected contract failure handler to run; got ${result}: ${output}${diagnostic}")
endif()
if(NOT diagnostic MATCHES "${CONTRACT_DIAGNOSTIC}")
    message(FATAL_ERROR "Missing contract diagnostic '${CONTRACT_DIAGNOSTIC}': ${diagnostic}")
endif()
