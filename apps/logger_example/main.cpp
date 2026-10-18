// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include <spd_logger/logging.h>

import nom.spd_logger;

int main()
{
    struct app_config
    {
        nom::log_config log{.level = nom::log_level::info};
    };

    const app_config config;
    nom::init_logger(nom::main_logger, config.log);
    LINFO("Application started at level {}", nom::to_string(config.log.level));

    int evaluations = 0;
    LDEBUG("Suppressed argument: {}", ++evaluations);
    LINFO("Suppressed DEBUG arguments evaluated {} times", evaluations);

    nom::set_log_level(nom::main_logger, nom::log_level::debug);
    LDEBUG("Enabled argument: {}", ++evaluations);
    LWARN("An example warning");
    LERROR("An example error");
    LALWAYS("Application finished");
    nom::flush_logger(nom::main_logger);
}
