// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: MIT

#include <spd_logger/logging.h>

#include <csignal>

import std;

import nom.spd_logger;

#ifdef NDEBUG
int main()
{
    // This helper must never execute undefined logger misuse with assertions disabled.
    return 77;
}
#else
int main(int argc, char** argv)
{
    if (argc != 2)
        return 1;

    // An assertion must abort directly, rather than call std::terminate or throw.
    std::set_terminate([] { std::_Exit(3); });
    if (std::signal(SIGABRT, [](int) { std::_Exit(0); }) == SIG_ERR)
        return 4;
    nom::logger empty;
    try
    {
        const std::string_view operation{argv[1]};
        if (operation == "global")
            LINFO("global logging before initialization");
        else if (operation == "log")
            NOM_LOG(empty, nom::log_level::info, "local logging before initialization");
        else if (operation == "set")
            nom::set_log_level(empty, nom::log_level::info);
        else if (operation == "flush")
            nom::flush_logger(empty);
        else if (operation == "prechecked")
            empty.log_prechecked_level(
                nom::log_level::info, nom::log_location::current(), "prechecked logging before initialization");
    }
    catch (...)
    {
        return 2;
    }
    return 1;
}
#endif
