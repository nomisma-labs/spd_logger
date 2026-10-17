// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include <string_view>

import nom.spd_logger;

static_assert(nom::is_config_value<nom::log_config>);
static_assert(nom::is_config_value<nom::logfile_config>);
static_assert(nom::is_config_value<nom::log_level>);
static_assert(nom::to_string(nom::log_level::warn) == std::string_view{"WARN"});

int main()
{
    const nom::log_config config{.level = nom::log_level::error};
    nom::init_main_logger(config);
    nom::set_main_log_level(nom::log_level::debug);
    nom::flush_main_logger();
    return 0;
}
