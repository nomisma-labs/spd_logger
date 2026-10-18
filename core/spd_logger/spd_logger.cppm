// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

module;

#include <cassert>
#include <expected>
#include <memory>
#include <source_location>
#include <string>
#include <string_view>
#include <type_traits>

#include <fmt/base.h>

export module nom.spd_logger;

export import nom.config_types;

export namespace nom
{
    enum class log_level
    {
        debug,
        info,
        warn,
        error,
        always
    };

    constexpr std::string_view to_string(log_level level) noexcept
    {
        switch (level)
        {
        case log_level::debug:
            return "DEBUG";
        case log_level::info:
            return "INFO";
        case log_level::warn:
            return "WARN";
        case log_level::error:
            return "ERROR";
        case log_level::always:
        default:
            return "ALWAYS";
        }
    }

    struct logfile_config
    {
        cfg_string name;
        // Rotate at this size in bytes, retaining ten older files. Unset means no rotation.
        cfg_optional<cfg_uint> rotation_size{};

        std::expected<void, std::string> validate() const;
    };

    struct log_config
    {
        log_level level = log_level::info;
        // Unset selects the console. File logging appends to an existing file.
        cfg_optional<logfile_config> file{};

        std::expected<void, std::string> validate() const;
    };

    using log_location = std::source_location;

    class logger
    {
    public:
        // An empty wrapper owns no sink. Initialize it before logging, setting or flushing.
        logger() noexcept;
        explicit logger(const log_config& config);
        ~logger();
        logger(const logger&) = delete;
        logger& operator=(const logger&) = delete;
        logger(logger&&) = delete;
        logger& operator=(logger&&) = delete;

        inline bool should_log(log_level level) const noexcept
        {
            return level >= level_;
        }

        inline void log(log_level level, log_location location, std::string_view message)
        {
            if (should_log(level))
                write_message(level, location, message);
        }

        template <typename... Args>
            requires(sizeof...(Args) > 0)
        inline void log(log_level level, log_location location, fmt::format_string<Args...> format, Args&&... args)
        {
            if (should_log(level))
                log_formatted(level, location, format.get(), fmt::make_format_args(args...));
        }

        template <typename Message>
            requires(!std::is_convertible_v<const Message&, std::string_view>)
        inline void log(log_level level, log_location location, const Message& message)
        {
            log(level, location, "{}", message);
        }

        // Call after should_log(level) succeeds. Only the wrapper's level check is skipped.
        inline void log_prechecked_level(log_level level, log_location location, std::string_view message)
        {
            assert(backend_ && "logger is not initialized");
            write_message(level, location, message);
        }

        template <typename... Args>
            requires(sizeof...(Args) > 0)
        inline void log_prechecked_level(log_level level,
                                         log_location location,
                                         fmt::format_string<Args...> format,
                                         Args&&... args)
        {
            assert(backend_ && "logger is not initialized");
            log_formatted(level, location, format.get(), fmt::make_format_args(args...));
        }

        template <typename Message>
            requires(!std::is_convertible_v<const Message&, std::string_view>)
        inline void log_prechecked_level(log_level level, log_location location, const Message& message)
        {
            log_prechecked_level(level, location, "{}", message);
        }

    private:
        struct implementation;
        std::unique_ptr<implementation> backend_;
        // Updated together with the backend so filtering can inline without spdlog types.
        log_level level_ = log_level::info;

        void write_message(log_level level, log_location location, std::string_view message);
        void log_formatted(log_level level, log_location location, fmt::string_view format, fmt::format_args args);
        static void report_error(log_location location, const char* message) noexcept;

        friend void init_logger(logger& target, const log_config& config);
        friend void set_log_level(logger& target, log_level level) noexcept;
        friend void flush_logger(logger& target) noexcept;
    };

    // A stable, empty global wrapper; init_logger creates its backend explicitly.
    extern logger main_logger;

    // Configuration failures preserve the previous backend. Runtime output failures are reported.
    void init_logger(logger& target, const log_config& config = {});
    void set_log_level(logger& target, log_level level) noexcept;
    void flush_logger(logger& target) noexcept;
} // namespace nom
