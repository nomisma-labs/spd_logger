// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

module;

#include <spd_logger/logging.h>

#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <cstddef>
#include <limits>
#include <memory>
#include <stdexcept>

module nom.spd_logger;

namespace nom
{
    namespace
    {
        spdlog::level::level_enum to_spdlog_level(log_level level)
        {
            switch (level)
            {
            case log_level::debug:
                return spdlog::level::debug;
            case log_level::info:
                return spdlog::level::info;
            case log_level::warn:
                return spdlog::level::warn;
            case log_level::error:
                return spdlog::level::err;
            case log_level::always:
                return spdlog::level::critical;
            }
            throw std::invalid_argument("invalid log level");
        }
    } // namespace

    void init_main_logger(const log_config& config)
    {
        const auto level = to_spdlog_level(config.level);
        // A rotating sink measures the existing file when it is constructed. Publish
        // pending records first, including when the new destination is the same file.
        detail::main_logger.flush();

        std::shared_ptr<spdlog::sinks::sink> sink;
        if (config.file)
        {
            if (config.file->name.empty())
                throw std::invalid_argument("log file name must not be empty");

            if (config.file->rotation_size)
            {
                const auto rotation_size = config.file->rotation_size->get();
                if (rotation_size == 0 || rotation_size > std::numeric_limits<std::size_t>::max())
                    throw std::invalid_argument("log rotation size must be a positive size_t value");
                sink = std::make_shared<spdlog::sinks::rotating_file_sink_st>(
                    config.file->name, static_cast<std::size_t>(rotation_size), 10);
            }
            else
                sink = std::make_shared<spdlog::sinks::basic_file_sink_st>(config.file->name, false);
        }
        else
            sink = std::make_shared<spdlog::sinks::stdout_color_sink_st>();

        spdlog::logger replacement{"main", sink};
        replacement.set_level(level);
        replacement.set_pattern("%Y-%m-%d %H:%M:%S.%fZ %-8l %s:%# %v", spdlog::pattern_time_type::utc);
        replacement.flush_on(spdlog::level::err);

        detail::main_logger.swap(replacement);
    }

    void set_main_log_level(log_level level)
    {
        detail::main_logger.set_level(to_spdlog_level(level));
    }

    void flush_main_logger()
    {
        detail::main_logger.flush();
    }
} // namespace nom
