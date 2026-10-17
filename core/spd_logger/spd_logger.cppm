// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

module;

#include <string_view>

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
            return "ALWAYS";
        }
        return "UNKNOWN";
    }

    struct logfile_config
    {
        cfg_string name;
        // Rotate at this size in bytes, retaining ten older files. Unset means no rotation.
        cfg_optional<cfg_uint> rotation_size{};
    };

    struct log_config
    {
        log_level level = log_level::info;
        // Unset selects the console. File logging appends to an existing file.
        cfg_optional<logfile_config> file{};
    };

    template <> inline constexpr bool enable_config_type<log_level> = true;
    template <> inline constexpr bool enable_config_type<logfile_config> = true;
    template <> inline constexpr bool enable_config_type<log_config> = true;

    // Configures the existing global logger. Sequential reinitialization is supported;
    // a configuration or sink-construction failure leaves its previous setup intact.
    void init_main_logger(const log_config& config = {});
    void set_main_log_level(log_level level);
    // Flushes on the calling thread. ERROR and ALWAYS also flush after writing.
    void flush_main_logger();
} // namespace nom
