// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include <spd_logger/logging.h>

import nom.spd_logger;

int main()
{
    // The namespace-scope logger already exists; explicit initialization configures it.
    INFO("logging before explicit configuration");
    nom::init_main_logger();
    int evaluations = 0;
    DEBUG("suppressed {}", ++evaluations);
    if (evaluations != 0)
        return 1;
    nom::set_main_log_level(nom::log_level::debug);
    DEBUG("enabled {}", ++evaluations);
    ALWAYS("consumer finished");
    nom::flush_main_logger();
    return evaluations == 1 ? 0 : 1;
}
