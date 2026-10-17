// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include <spd_logger/logging.h>

#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <memory>

namespace nom::detail
{
    // This is an ordinary translation unit because the macro header declares the object
    // in the global module. No function-local static or first-call accessor is involved.
    spdlog::logger main_logger = []
    {
        spdlog::logger logger{"main", std::make_shared<spdlog::sinks::stdout_color_sink_st>()};
        logger.set_level(spdlog::level::info);
        logger.set_pattern("%Y-%m-%d %H:%M:%S.%fZ %-8l %s:%# %v", spdlog::pattern_time_type::utc);
        logger.flush_on(spdlog::level::err);
        return logger;
    }();
} // namespace nom::detail
