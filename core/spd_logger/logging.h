// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#pragma once

#include <spdlog/spdlog.h>

namespace nom::detail
{
    // Defined once at namespace scope, with a synchronous console sink before main().
    // Every operation on this object must run on the application's logging thread.
    extern spdlog::logger main_logger;
} // namespace nom::detail

// NOM_LOG is an implementation helper; its severity parameter is a spdlog enum.
// Checking before the call avoids evaluating arguments for a suppressed message.
// A lone message is passed through; a format string with arguments is checked by fmt.
#define NOM_LOG(level, ...)                                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        const auto nom_spd_log_severity = (level);                                                                     \
        if (::nom::detail::main_logger.should_log(nom_spd_log_severity))                                               \
            ::nom::detail::main_logger.log(                                                                            \
                ::spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, nom_spd_log_severity, __VA_ARGS__);         \
    } while (false)

#define DEBUG(...) NOM_LOG(::spdlog::level::debug, __VA_ARGS__)
#define INFO(...) NOM_LOG(::spdlog::level::info, __VA_ARGS__)
#define WARN(...) NOM_LOG(::spdlog::level::warn, __VA_ARGS__)
#define ERROR(...) NOM_LOG(::spdlog::level::err, __VA_ARGS__)
#define ALWAYS(...) NOM_LOG(::spdlog::level::critical, __VA_ARGS__)
