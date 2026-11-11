// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: MIT

module;

#include <fmt/format.h>
#include <spdlog/logger.h>
#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include <cassert>
#include <cstdio>

module nom.spd_logger;

import std;

namespace nom
{
    namespace
    {
        constexpr spdlog::level::level_enum to_spdlog_level(log_level level) noexcept
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
            default:
                return spdlog::level::critical;
            }
        }

        constexpr log_level from_spdlog_level(spdlog::level::level_enum level) noexcept
        {
            switch (level)
            {
            case spdlog::level::debug:
                return log_level::debug;
            case spdlog::level::info:
                return log_level::info;
            case spdlog::level::warn:
                return log_level::warn;
            case spdlog::level::err:
                return log_level::error;
            case spdlog::level::critical:
                return log_level::always;
            case spdlog::level::trace:
            case spdlog::level::off:
            case spdlog::level::n_levels:
            default:
                return log_level::always;
            }
        }

        class level_name_formatter final : public spdlog::custom_flag_formatter
        {
        public:
            void format(const spdlog::details::log_msg& message,
                        const std::tm&,
                        spdlog::memory_buf_t& destination) override
            {
                fmt::format_to(std::back_inserter(destination), "{:<8}", to_string(from_spdlog_level(message.level)));
            }

            std::unique_ptr<spdlog::custom_flag_formatter> clone() const override
            {
                return std::make_unique<level_name_formatter>();
            }
        };

        std::shared_ptr<spdlog::sinks::sink> make_sink(const log_config& config)
        {
            if (!config.file)
                return std::make_shared<spdlog::sinks::stdout_color_sink_st>();
            if (config.file->rotation_size)
                return std::make_shared<spdlog::sinks::rotating_file_sink_st>(
                    config.file->name, static_cast<std::size_t>(config.file->rotation_size->get()), 10);
            return std::make_shared<spdlog::sinks::basic_file_sink_st>(config.file->name, false);
        }

        spdlog::source_loc source_location(log_location location) noexcept
        {
            return {location.file_name(), static_cast<int>(location.line()), location.function_name()};
        }
    } // namespace

    struct logger::implementation
    {
        spdlog::logger sink_logger;

        implementation(const log_config& config, spdlog::level::level_enum level)
            : sink_logger{"nom", make_sink(config)}
        {
            auto formatter = std::make_unique<spdlog::pattern_formatter>(spdlog::pattern_time_type::utc);
            formatter->add_flag<level_name_formatter>('l').set_pattern("%Y-%m-%d %H:%M:%S.%fZ %l %s:%# %v");
            sink_logger.set_formatter(std::move(formatter));
            sink_logger.set_level(level);
            sink_logger.flush_on(spdlog::level::err);
        }
    };

    logger::logger() noexcept = default;

    logger::logger(const log_config& config)
    {
        init_logger(*this, config);
    }

    logger::~logger() = default;

    void detail::report_log_failure(std::string_view message, const char* error) noexcept
    {
        // A view need not be NUL-terminated. Copy bounded chunks without allocating or formatting.
        while (!message.empty())
        {
            char chunk[256];
            const auto part = message.substr(0, sizeof(chunk) - 1);
            const auto size = std::min(part.size(), part.find('\0'));
            std::copy_n(message.data(), size, chunk);
            chunk[size] = '\0';
            std::fputs(chunk, stderr);
            message.remove_prefix(size);
            if (!message.empty() && message.front() == '\0')
            {
                std::fputs("\\0", stderr);
                message.remove_prefix(1);
            }
        }
        std::fputs("\n", stderr);
        std::fputs(error, stderr);
        std::fputs("\n", stderr);
    }

    void logger::report_error(log_location location, const char* message) noexcept
    {
        std::fprintf(
            stderr, "nom::logger: %s [%s:%u]\n", message, location.file_name(), static_cast<unsigned>(location.line()));
    }

    void logger::write_message(log_level level, log_location location, std::string_view message)
    {
        backend_->sink_logger.log(
            source_location(location), to_spdlog_level(level), spdlog::string_view_t{message.data(), message.size()});
    }

    void logger::log_formatted(log_level level, log_location location, fmt::string_view format, fmt::format_args args)
    {
        fmt::memory_buffer buffer;
        fmt::vformat_to(fmt::appender(buffer), format, args);
        write_message(level, location, std::string_view{buffer.data(), buffer.size()});
    }

    void init_logger(logger& target, const log_config& config)
    {
        if (const auto validation = config.validate(); !validation)
            throw std::invalid_argument(validation.error());
        const auto level = to_spdlog_level(config.level);
        // Publish pending records before a rotating replacement measures the same file.
        // Operational flush failures are reported but do not prevent reconfiguration.
        if (target.backend_)
            flush_logger(target);

        try
        {
            auto replacement = std::make_unique<logger::implementation>(config, level);
            target.backend_.swap(replacement);
            target.level_ = config.level;
        }
        catch (const spdlog::spdlog_ex& error)
        {
            throw std::runtime_error(error.what());
        }
    }

    void set_log_level(logger& target, log_level level) noexcept
    {
        assert(target.backend_ && "logger is not initialized");
        target.backend_->sink_logger.set_level(to_spdlog_level(level));
        target.level_ = level;
    }

    void flush_logger(logger& target) noexcept
    {
        assert(target.backend_ && "logger is not initialized");
        try
        {
            target.backend_->sink_logger.flush();
        }
        catch (const std::exception& error)
        {
            logger::report_error(log_location::current(), error.what());
        }
        catch (...)
        {
            logger::report_error(log_location::current(), "unknown flush failure");
        }
    }
} // namespace nom
