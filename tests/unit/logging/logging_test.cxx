// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>

#include <spd_logger/logging.h>

#include <fmt/format.h>

#ifdef SPDLOG_VERSION
#error "The public logger interface must not expose spdlog headers"
#endif

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <iterator>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>

import nom.spd_logger;

static_assert(std::is_aggregate_v<nom::logfile_config>);
static_assert(std::is_aggregate_v<nom::log_config>);
static_assert(!std::is_copy_constructible_v<nom::logger>);
static_assert(!std::is_copy_assignable_v<nom::logger>);
static_assert(!std::is_move_constructible_v<nom::logger>);
static_assert(!std::is_move_assignable_v<nom::logger>);

struct formatted_event
{
    int value;
};

struct throwing_event
{
    int* evaluations;
};

struct throwing_formatted_event
{
    int* evaluations;
};

int format_as(const throwing_event& event)
{
    ++*event.evaluations;
    throw std::runtime_error("format_as failed");
}

template <> struct fmt::formatter<formatted_event> : fmt::formatter<int>
{
    template <typename FormatContext> auto format(const formatted_event& event, FormatContext& context) const
    {
        return fmt::formatter<int>::format(event.value, context);
    }
};

template <> struct fmt::formatter<throwing_formatted_event> : fmt::formatter<int>
{
    template <typename FormatContext>
    auto format(const throwing_formatted_event& event, FormatContext& context) const -> decltype(context.out())
    {
        ++*event.evaluations;
        throw std::logic_error("formatter failed");
    }
};

namespace
{
    class log_files
    {
    public:
        log_files()
        {
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            directory_ = std::filesystem::temp_directory_path() / ("nom_spd_logger_" + std::to_string(stamp));
            if (!std::filesystem::create_directory(directory_))
                throw std::runtime_error("could not create logger test directory");
        }

        ~log_files()
        {
            // Release the test file before deleting its directory.
            try
            {
                nom::init_logger(nom::main_logger);
            }
            catch (...)
            {
            }
            std::error_code error;
            std::filesystem::remove_all(directory_, error);
        }

        std::filesystem::path path(const std::string& name = "main.log") const
        {
            return directory_ / name;
        }

        nom::log_config config(nom::log_level level = nom::log_level::info, const std::string& name = "main.log") const
        {
            return {.level = level, .file = nom::logfile_config{.name = path(name).string()}};
        }

        std::string read(const std::string& name = "main.log") const
        {
            std::ifstream input(path(name));
            return {std::istreambuf_iterator<char>{input}, std::istreambuf_iterator<char>{}};
        }

        std::size_t count() const
        {
            return static_cast<std::size_t>(
                std::distance(std::filesystem::directory_iterator(directory_), std::filesystem::directory_iterator{}));
        }

    private:
        std::filesystem::path directory_;
    };
} // namespace

TEST_CASE("severity filtering skips arguments and supports all public levels", "[logging]")
{
    INFO("The default INFO threshold must suppress DEBUG arguments");
    const log_files files;
    nom::init_logger(nom::main_logger, files.config());

    int evaluations = 0;
    LDEBUG("suppressed {}", ++evaluations);
    CHECK(evaluations == 0);

    LINFO("enabled {}", ++evaluations);
    LWARN("warning record");
    LERROR("error record");
    LALWAYS("always record");
    CHECK(evaluations == 1);

    nom::set_log_level(nom::main_logger, nom::log_level::always);
    LERROR("suppressed error {}", ++evaluations);
    LALWAYS("highest threshold record");
    CHECK(evaluations == 1);

    nom::set_log_level(nom::main_logger, nom::log_level::debug);
    LDEBUG("debug enabled {}", ++evaluations);
    nom::flush_logger(nom::main_logger);
    CHECK(evaluations == 2);

    const auto output = files.read();
    CHECK(output.find("suppressed") == std::string::npos);
    CHECK(output.find("enabled 1") != std::string::npos);
    CHECK(output.find("warning record") != std::string::npos);
    CHECK(output.find("error record") != std::string::npos);
    CHECK(output.find("always record") != std::string::npos);
    CHECK(output.find("highest threshold record") != std::string::npos);
    CHECK(output.find("debug enabled 2") != std::string::npos);
    for (const auto label : {"DEBUG", "INFO", "WARN", "ERROR", "ALWAYS"})
        CHECK(output.find(std::string{"Z "} + label + ' ') != std::string::npos);
    CHECK(output.find("critical") == std::string::npos);
}

TEST_CASE("messages retain braces and identify their calling source line", "[logging]")
{
    const log_files files;
    nom::init_logger(nom::main_logger, files.config());

    const auto source_line = __LINE__ + 1;
    LINFO("literal {braces}");
    const std::string runtime_message = "runtime {message}";
    LINFO(runtime_message);
    LINFO("formatted {} {}", 42, "value");
    if (true)
        LINFO("statement macro");
    else
        LINFO("unused branch");
    nom::flush_logger(nom::main_logger);

    const auto output = files.read();
    CHECK(output.find("logging_test.cxx:" + std::to_string(source_line) + " literal {braces}") != std::string::npos);
    CHECK(output.find("runtime {message}") != std::string::npos);
    CHECK(output.find("formatted 42 value") != std::string::npos);
    CHECK(output.find("statement macro") != std::string::npos);
    CHECK(output.find("unused branch") == std::string::npos);
    CHECK(output.find("Z INFO") != std::string::npos);
}

TEST_CASE("caller-provided fmt formatters retain their formatting options", "[logging]")
{
    const log_files files;
    nom::init_logger(nom::main_logger, files.config());

    LINFO("custom {:04}", formatted_event{17});
    const auto source_line = __LINE__ + 1;
    LINFO(formatted_event{23});
    nom::flush_logger(nom::main_logger);

    CHECK(files.read().find("custom 0017") != std::string::npos);
    CHECK(files.read().find("logging_test.cxx:" + std::to_string(source_line) + " 23") != std::string::npos);
}

TEST_CASE("exceptions from ADL format_as propagate and leave the logger usable", "[logging]")
{
    const log_files files;
    nom::init_logger(nom::main_logger, files.config());

    int evaluations = 0;
    CHECK_THROWS_AS([&] { LINFO("format_as record {}", throwing_event{&evaluations}); }(), std::runtime_error);
    CHECK(evaluations == 1);
    LINFO("record after failed formatting");
    nom::flush_logger(nom::main_logger);

    const auto output = files.read();
    CHECK(output.find("format_as record") == std::string::npos);
    CHECK(output.find("record after failed formatting") != std::string::npos);
}

TEST_CASE("exceptions from a custom fmt formatter propagate through checked logging", "[logging]")
{
    const log_files files;
    nom::logger selected{files.config()};

    int evaluations = 0;
    CHECK_THROWS_AS(selected.log(nom::log_level::info,
                                 nom::log_location::current(),
                                 "formatter record {}",
                                 throwing_formatted_event{&evaluations}),
                    std::logic_error);
    CHECK(evaluations == 1);
    NOM_LOG(selected, nom::log_level::info, "record after formatter exception");
    nom::flush_logger(selected);

    const auto output = files.read();
    CHECK(output.find("formatter record") == std::string::npos);
    CHECK(output.find("record after formatter exception") != std::string::npos);
}

TEST_CASE("checked logging suppresses argument conversion and message formatting", "[logging]")
{
    const log_files files;
    nom::logger selected{files.config()};
    int conversions = 0;
    int formats = 0;

    CHECK_NOTHROW(selected.log(
        nom::log_level::debug, nom::log_location::current(), "suppressed conversion {}", throwing_event{&conversions}));
    CHECK_NOTHROW(selected.log(nom::log_level::debug,
                               nom::log_location::current(),
                               "suppressed formatter {}",
                               throwing_formatted_event{&formats}));
    CHECK_NOTHROW(
        selected.log(nom::log_level::debug, nom::log_location::current(), throwing_formatted_event{&formats}));
    CHECK(conversions == 0);
    CHECK(formats == 0);

    selected.log(nom::log_level::info, nom::log_location::current(), "checked raw {message}");
    nom::flush_logger(selected);
    CHECK(files.read().find("suppressed") == std::string::npos);
    CHECK(files.read().find("checked raw {message}") != std::string::npos);
}

TEST_CASE("prechecked logging accepts raw formatted and custom messages with the supplied source", "[logging]")
{
    const log_files files;
    nom::logger selected{files.config()};
    const auto level = nom::log_level::info;
    const auto location = nom::log_location::current();
    const std::string raw_message = "prechecked raw {message}";

    if (selected.should_log(level))
    {
        selected.log_prechecked_level(level, location, raw_message);
        selected.log_prechecked_level(level, location, "prechecked formatted {:04}", 17);
        selected.log_prechecked_level(level, location, formatted_event{23});
    }
    nom::flush_logger(selected);

    const auto output = files.read();
    const auto source = "logging_test.cxx:" + std::to_string(location.line()) + ' ';
    CHECK(output.find(source + raw_message) != std::string::npos);
    CHECK(output.find(source + "prechecked formatted 0017") != std::string::npos);
    CHECK(output.find(source + "23") != std::string::npos);
}

TEST_CASE("an empty logger can be destroyed or initialized explicitly", "[logging]")
{
    CHECK_NOTHROW([] { nom::logger empty; }());

    const log_files files;
    nom::logger selected;
    auto invalid = files.config();
    invalid.file->name.clear();
    CHECK_THROWS_AS(nom::init_logger(selected, invalid), std::invalid_argument);

    nom::init_logger(selected, files.config());
    NOM_LOG(selected, nom::log_level::info, "initialized after a rejected configuration");
    nom::flush_logger(selected);
    CHECK(files.read().find("initialized after a rejected configuration") != std::string::npos);
}

TEST_CASE("independent logger instances own their levels destinations and lifetimes", "[logging]")
{
    const log_files files;
    nom::logger first{files.config(nom::log_level::error, "first.log")};
    {
        nom::logger second{files.config(nom::log_level::debug, "second.log")};
        NOM_LOG(first, nom::log_level::info, "suppressed first record");
        NOM_LOG(second, nom::log_level::debug, "second logger record");
        nom::set_log_level(first, nom::log_level::info);
        NOM_LOG(first, nom::log_level::info, "first logger record");
    }

    // Destroying the second logger releases its output without disturbing the first.
    const auto second_output = files.read("second.log");
    CHECK(second_output.find("second logger record") != std::string::npos);
    CHECK(second_output.find("first logger record") == std::string::npos);

    NOM_LOG(first, nom::log_level::info, "first logger survives");
    nom::flush_logger(first);
    const auto first_output = files.read("first.log");
    CHECK(first_output.find("first logger record") != std::string::npos);
    CHECK(first_output.find("first logger survives") != std::string::npos);
    CHECK(first_output.find("suppressed first record") == std::string::npos);
    CHECK(first_output.find("second logger record") == std::string::npos);
}

TEST_CASE("explicit logging evaluates its instance and level once and skips suppressed arguments", "[logging]")
{
    const log_files files;
    nom::logger selected{files.config()};
    int instance_evaluations = 0;
    int level_evaluations = 0;
    int argument_evaluations = 0;
    const auto select_instance = [&]() -> nom::logger&
    {
        ++instance_evaluations;
        return selected;
    };
    const auto select_level = [&]
    {
        ++level_evaluations;
        return nom::log_level::debug;
    };

    NOM_LOG(select_instance(), select_level(), "suppressed {}", ++argument_evaluations);
    CHECK(instance_evaluations == 1);
    CHECK(level_evaluations == 1);
    CHECK(argument_evaluations == 0);

    nom::set_log_level(selected, nom::log_level::debug);
    NOM_LOG(select_instance(), select_level(), "enabled {}", ++argument_evaluations);
    NOM_LOG(selected, nom::log_level::info, "explicit {braces}");
    nom::flush_logger(selected);
    CHECK(instance_evaluations == 2);
    CHECK(level_evaluations == 2);
    CHECK(argument_evaluations == 1);
    CHECK(files.read().find("suppressed") == std::string::npos);
    CHECK(files.read().find("enabled 1") != std::string::npos);
    CHECK(files.read().find("explicit {braces}") != std::string::npos);
}

TEST_CASE("reinitialization preserves the global object and changes its output", "[logging]")
{
    const log_files files;
    const auto* original = &nom::main_logger;
    nom::init_logger(nom::main_logger, files.config());
    LINFO("first destination");

    nom::init_logger(nom::main_logger, files.config(nom::log_level::debug, "other.log"));
    LDEBUG("second destination");
    nom::flush_logger(nom::main_logger);

    CHECK(&nom::main_logger == original);
    CHECK(files.read().find("first destination") != std::string::npos);
    CHECK(files.read().find("second destination") == std::string::npos);
    const auto other_output = files.read("other.log");
    CHECK(other_output.find("second destination") != std::string::npos);
    CHECK(other_output.find("second destination") == other_output.rfind("second destination"));

    nom::init_logger(nom::main_logger, files.config());
    int evaluations = 0;
    LDEBUG("suppressed after reinitialization {}", ++evaluations);
    CHECK(evaluations == 0);
    LINFO("appended record");
    nom::flush_logger(nom::main_logger);
    CHECK(files.read().find("first destination") != std::string::npos);
    CHECK(files.read().find("appended record") != std::string::npos);
    CHECK(files.read().find("suppressed after reinitialization") == std::string::npos);
}

TEST_CASE("failed initialization keeps the previous configuration usable", "[logging]")
{
    const log_files files;
    nom::init_logger(nom::main_logger, files.config());
    const auto* original = &nom::main_logger;
    LINFO("buffered record before invalid configuration");
    const auto before_validation = files.read();

    auto invalid = files.config(nom::log_level::debug);
    invalid.file->name.clear();
    CHECK_THROWS_AS(nom::init_logger(nom::main_logger, invalid), std::invalid_argument);
    const auto validation = invalid.validate();
    REQUIRE_FALSE(validation.has_value());
    CHECK_THROWS_WITH(nom::init_logger(nom::main_logger, invalid), validation.error());
    CHECK(files.read() == before_validation);

    invalid = files.config(nom::log_level::debug);
    invalid.file->rotation_size = nom::cfg_uint{0};
    CHECK_THROWS_AS(nom::init_logger(nom::main_logger, invalid), std::invalid_argument);
    CHECK(files.read() == before_validation);

    // A regular file cannot also be a parent directory for a replacement log.
    invalid = files.config(nom::log_level::debug);
    invalid.file->name = (files.path() / "child.log").string();
    CHECK_THROWS_AS(nom::init_logger(nom::main_logger, invalid), std::runtime_error);

    int evaluations = 0;
    LDEBUG("suppressed after rejected configuration {}", ++evaluations);
    CHECK(evaluations == 0);
    LINFO("still working");
    nom::flush_logger(nom::main_logger);
    CHECK(&nom::main_logger == original);
    CHECK(files.read().find("buffered record before invalid configuration") != std::string::npos);
    CHECK(files.read().find("still working") != std::string::npos);
    CHECK(files.read().find("suppressed after rejected configuration") == std::string::npos);
}

TEST_CASE("embedded NUL filenames are rejected before touching the active file", "[logging]")
{
    const log_files files;
    nom::init_logger(nom::main_logger, files.config());
    LINFO("record before invalid configuration");

    for (const bool rotate : {false, true})
    {
        auto invalid = files.config();
        invalid.file->name.push_back('\0');
        invalid.file->name += "ignored.txt";
        if (rotate)
            invalid.file->rotation_size = nom::cfg_uint{80};
        CHECK_THROWS_AS(nom::init_logger(nom::main_logger, invalid), std::invalid_argument);
        LINFO("record after rejection {}", rotate);
    }
    nom::flush_logger(nom::main_logger);

    const auto output = files.read();
    CHECK(output.find("record before invalid configuration") != std::string::npos);
    CHECK(output.find("record after rejection false") != std::string::npos);
    CHECK(output.find("record after rejection true") != std::string::npos);
    CHECK(files.count() == 1);
}

TEST_CASE("rotation bounds retained files and flush makes the last record visible", "[logging]")
{
    const log_files files;
    auto config = files.config();
    config.file->rotation_size = nom::cfg_uint{256};
    nom::init_logger(nom::main_logger, config);

    for (int index = 0; index < 20; ++index)
        LINFO("record {} {}", index, std::string(180, 'x'));
    nom::flush_logger(nom::main_logger);

    CHECK(std::filesystem::exists(files.path("main.1.log")));
    CHECK(files.count() > 1);
    CHECK(files.count() <= 11);
    CHECK(files.read().find("record 19 ") != std::string::npos);
}

TEST_CASE("error and always records flush synchronously", "[logging]")
{
    const log_files files;
    nom::init_logger(nom::main_logger, files.config());
    LERROR("error is visible immediately");
    CHECK(files.read().find("error is visible immediately") != std::string::npos);
    LALWAYS("always is visible immediately");
    CHECK(files.read().find("always is visible immediately") != std::string::npos);
}

TEST_CASE("reinitializing a rotating destination accounts for buffered records", "[logging]")
{
    const log_files files;
    auto config = files.config();
    config.file->rotation_size = nom::cfg_uint{256};
    nom::init_logger(nom::main_logger, config);
    LINFO("before {}", std::string(180, 'x'));

    nom::init_logger(nom::main_logger, config);
    LINFO("after {}", std::string(180, 'y'));
    nom::flush_logger(nom::main_logger);

    CHECK(files.read("main.1.log").find("before ") != std::string::npos);
    CHECK(files.read().find("before ") == std::string::npos);
    CHECK(files.read().find("after ") != std::string::npos);
}

TEST_CASE("operational flush failures do not prevent reconfiguration", "[logging]")
{
#if defined(__linux__)
    if (!std::filesystem::exists("/dev/full"))
        SKIP("The /dev/full device is unavailable");

    const log_files files;
    nom::logger output{nom::log_config{.file = nom::logfile_config{.name = "/dev/full"}}};
    NOM_LOG(output, nom::log_level::info, "buffered record before an explicit failed flush");
    CHECK_NOTHROW(nom::flush_logger(output));

    NOM_LOG(output, nom::log_level::info, "buffered record before reconfiguration flushes");
    CHECK_NOTHROW(nom::init_logger(output, files.config()));
    NOM_LOG(output, nom::log_level::info, "replacement destination is usable");
    nom::flush_logger(output);
    CHECK(files.read().find("replacement destination is usable") != std::string::npos);
#else
    SKIP("The /dev/full failure test requires Linux");
#endif
}
