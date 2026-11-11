// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: MIT

import std;

import nom.spd_logger;

#ifdef SPDLOG_VERSION
#error "Importing the logger module must not expose spdlog headers"
#endif

static_assert(nom::to_string(nom::log_level::warn) == std::string_view{"WARN"});
static_assert(
    std::is_same_v<decltype(std::declval<const nom::logfile_config&>().validate()), std::expected<void, std::string>>);
static_assert(
    std::is_same_v<decltype(std::declval<const nom::log_config&>().validate()), std::expected<void, std::string>>);

int main()
{
    const nom::log_config config{.level = {nom::log_level::error}};
    if (!config.validate())
        return 1;
    const nom::logfile_config invalid_file;
    const auto file_result = invalid_file.validate();
    if (file_result || file_result.error() != "/name must not be empty")
        return 1;
    const nom::log_config invalid_config{.file = invalid_file};
    const auto config_result = invalid_config.validate();
    if (config_result || config_result.error() != "/file/name must not be empty")
        return 1;

    nom::logger local{config};
    nom::set_log_level(local, nom::log_level::debug);
    local.log(nom::log_level::info, nom::log_location::current(), "checked import-only {message}");
    if (local.should_log(nom::log_level::info))
        local.log_prechecked_level(nom::log_level::info, nom::log_location::current(), "prechecked import-only {}", 42);
    nom::flush_logger(local);
    nom::init_logger(nom::main_logger, config);
    nom::flush_logger(nom::main_logger);
    return 0;
}
