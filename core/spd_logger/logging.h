// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#pragma once

// Import nom.spd_logger before expanding these macros. The logger and level are
// evaluated once; suppressed messages do not evaluate their formatting arguments.
#define NOM_LOG(target, level, ...)                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        auto& nom_log_target = (target);                                                                               \
        const auto nom_log_severity = (level);                                                                         \
        if (nom_log_target.should_log(nom_log_severity))                                                               \
            nom_log_target.log_prechecked_level(nom_log_severity, ::nom::log_location::current(), __VA_ARGS__);        \
    } while (false)

#define DEBUG(...) NOM_LOG(::nom::main_logger, ::nom::log_level::debug, __VA_ARGS__)
#define INFO(...) NOM_LOG(::nom::main_logger, ::nom::log_level::info, __VA_ARGS__)
#define WARN(...) NOM_LOG(::nom::main_logger, ::nom::log_level::warn, __VA_ARGS__)
#define ERROR(...) NOM_LOG(::nom::main_logger, ::nom::log_level::error, __VA_ARGS__)
#define ALWAYS(...) NOM_LOG(::nom::main_logger, ::nom::log_level::always, __VA_ARGS__)
