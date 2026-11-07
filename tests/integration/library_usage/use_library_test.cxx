// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include <spd_logger/logging.h>

#ifdef SPDLOG_VERSION
#error "The public logger interface must not expose spdlog headers"
#endif

import nom.spd_logger;

int main()
{
    nom::init_logger(nom::main_logger);
    LINFO("logging after explicit configuration");
    int evaluations = 0;
    LDEBUG("suppressed {}", ++evaluations);
    if (evaluations != 0)
        return 1;
    nom::set_log_level(nom::main_logger, nom::log_level::debug);
    LDEBUG("enabled {}", ++evaluations);
    LALWAYS("consumer finished");
    nom::flush_logger(nom::main_logger);

    nom::logger local{nom::log_config{.level = {nom::log_level::error}}};
    NOM_LOG(local, nom::log_level::info, "suppressed local {}", ++evaluations);
    NOM_LOG(local, nom::log_level::always, "independent consumer logger");
    nom::flush_logger(local);
    return evaluations == 1 ? 0 : 1;
}
