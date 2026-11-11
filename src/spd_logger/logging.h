// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: MIT

#pragma once

// Import nom.spd_logger before expanding these macros. The logger and level are
// evaluated once; suppressed messages do not evaluate their formatting arguments.
#define NOM_LOG(target, level, ...)                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        auto& nom_log_target = (target);                                                                               \
        const auto nom_log_severity = (level);                                                                         \
        if (nom_log_target.should_log(nom_log_severity))                                                               \
            nom_log_target.log_prechecked_level(nom_log_severity, ::nom::log_location::current(), __VA_ARGS__);        \
    } while (false)

// Keep argument evaluation inside the guard. If it fails before a message exists,
// the source expression is the only safe diagnostic; never evaluate it a second time.
#define NOM_LOG_NOEXCEPT(target, level, ...)                                                                           \
    do                                                                                                                 \
    {                                                                                                                  \
        try                                                                                                            \
        {                                                                                                              \
            auto& nom_log_target = (target);                                                                           \
            const auto nom_log_severity = (level);                                                                     \
            if (nom_log_target.should_log(nom_log_severity))                                                           \
                ::nom::detail::log_prechecked_noexcept(                                                                \
                    nom_log_target, nom_log_severity, ::nom::log_location::current(), #__VA_ARGS__, __VA_ARGS__);      \
        }                                                                                                              \
        catch (const ::nom::detail::log_exception& nom_log_error)                                                      \
        {                                                                                                              \
            ::nom::detail::report_log_failure(#__VA_ARGS__, nom_log_error.what());                                     \
        }                                                                                                              \
        catch (...)                                                                                                    \
        {                                                                                                              \
            ::nom::detail::report_log_failure(#__VA_ARGS__, "unknown logging exception");                              \
        }                                                                                                              \
    } while (false)

#define LDEBUG(...) NOM_LOG(::nom::main_logger, ::nom::log_level::debug, __VA_ARGS__)
#define LINFO(...) NOM_LOG(::nom::main_logger, ::nom::log_level::info, __VA_ARGS__)
#define LWARN(...) NOM_LOG(::nom::main_logger, ::nom::log_level::warn, __VA_ARGS__)
#define LERROR(...) NOM_LOG_NOEXCEPT(::nom::main_logger, ::nom::log_level::error, __VA_ARGS__)
#define LALWAYS(...) NOM_LOG_NOEXCEPT(::nom::main_logger, ::nom::log_level::always, __VA_ARGS__)
