// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#define CATCH_CONFIG_PREFIX_MESSAGES
#include <catch2/catch_test_macros.hpp>

#include <spd_logger/logging.h>

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>

import nom.spd_logger;

static_assert(nom::is_config_value<nom::log_level>);
static_assert(nom::is_config_value<nom::logfile_config>);
static_assert(nom::is_config_value<nom::log_config>);
static_assert(std::is_aggregate_v<nom::logfile_config>);
static_assert(std::is_aggregate_v<nom::log_config>);

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
                nom::init_main_logger();
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
    const log_files files;
    nom::init_main_logger(files.config());

    int evaluations = 0;
    DEBUG("suppressed {}", ++evaluations);
    CHECK(evaluations == 0);

    INFO("enabled {}", ++evaluations);
    WARN("warning record");
    ERROR("error record");
    ALWAYS("always record");
    CHECK(evaluations == 1);

    nom::set_main_log_level(nom::log_level::always);
    ERROR("suppressed error {}", ++evaluations);
    ALWAYS("highest threshold record");
    CHECK(evaluations == 1);

    nom::set_main_log_level(nom::log_level::debug);
    DEBUG("debug enabled {}", ++evaluations);
    nom::flush_main_logger();
    CHECK(evaluations == 2);

    const auto output = files.read();
    CHECK(output.find("suppressed") == std::string::npos);
    CHECK(output.find("enabled 1") != std::string::npos);
    CHECK(output.find("warning record") != std::string::npos);
    CHECK(output.find("error record") != std::string::npos);
    CHECK(output.find("always record") != std::string::npos);
    CHECK(output.find("highest threshold record") != std::string::npos);
    CHECK(output.find("debug enabled 2") != std::string::npos);
    CHECK(output.find("critical") != std::string::npos);
}

TEST_CASE("messages retain braces and identify their calling source line", "[logging]")
{
    const log_files files;
    nom::init_main_logger(files.config());

    const auto source_line = __LINE__ + 1;
    INFO("literal {braces}");
    const std::string runtime_message = "runtime {message}";
    INFO(runtime_message);
    INFO("formatted {} {}", 42, "value");
    if (true)
        INFO("statement macro");
    else
        INFO("unused branch");
    nom::flush_main_logger();

    const auto output = files.read();
    CHECK(output.find("logging_test.cxx:" + std::to_string(source_line) + " literal {braces}") != std::string::npos);
    CHECK(output.find("runtime {message}") != std::string::npos);
    CHECK(output.find("formatted 42 value") != std::string::npos);
    CHECK(output.find("statement macro") != std::string::npos);
    CHECK(output.find("unused branch") == std::string::npos);
    CHECK(output.find("Z info") != std::string::npos);
}

TEST_CASE("reinitialization preserves the global object and changes its output", "[logging]")
{
    const log_files files;
    const auto* original = &nom::detail::main_logger;
    const auto unrelated_default = spdlog::default_logger();
    nom::init_main_logger(files.config());
    INFO("first destination");

    nom::init_main_logger(files.config(nom::log_level::info, "other.log"));
    INFO("second destination");
    nom::flush_main_logger();

    CHECK(&nom::detail::main_logger == original);
    CHECK(nom::detail::main_logger.sinks().size() == 1);
    CHECK(spdlog::default_logger() == unrelated_default);
    CHECK(files.read().find("first destination") != std::string::npos);
    CHECK(files.read().find("second destination") == std::string::npos);
    CHECK(files.read("other.log").find("second destination") != std::string::npos);

    nom::init_main_logger(files.config());
    INFO("appended record");
    nom::flush_main_logger();
    CHECK(files.read().find("first destination") != std::string::npos);
    CHECK(files.read().find("appended record") != std::string::npos);
}

TEST_CASE("failed initialization keeps the previous configuration usable", "[logging]")
{
    const log_files files;
    nom::init_main_logger(files.config());
    const auto* original = &nom::detail::main_logger;

    auto invalid = files.config();
    invalid.file->name.clear();
    CHECK_THROWS_AS(nom::init_main_logger(invalid), std::invalid_argument);

    invalid = files.config();
    invalid.file->rotation_size = nom::cfg_uint{0};
    CHECK_THROWS_AS(nom::init_main_logger(invalid), std::invalid_argument);

    invalid = files.config();
    invalid.level = static_cast<nom::log_level>(100);
    CHECK_THROWS_AS(nom::init_main_logger(invalid), std::invalid_argument);
    CHECK_THROWS_AS(nom::set_main_log_level(invalid.level), std::invalid_argument);

    // A regular file cannot also be a parent directory for a replacement log.
    invalid = files.config();
    invalid.file->name = (files.path() / "child.log").string();
    CHECK_THROWS_AS(nom::init_main_logger(invalid), spdlog::spdlog_ex);

    INFO("still working");
    nom::flush_main_logger();
    CHECK(&nom::detail::main_logger == original);
    CHECK(files.read().find("still working") != std::string::npos);
}

TEST_CASE("rotation bounds retained files and flush makes the last record visible", "[logging]")
{
    const log_files files;
    auto config = files.config();
    config.file->rotation_size = nom::cfg_uint{256};
    nom::init_main_logger(config);

    for (int index = 0; index < 20; ++index)
        INFO("record {} {}", index, std::string(180, 'x'));
    nom::flush_main_logger();

    CHECK(std::filesystem::exists(files.path("main.1.log")));
    CHECK(files.count() > 1);
    CHECK(files.count() <= 11);
    CHECK(files.read().find("record 19 ") != std::string::npos);
}

TEST_CASE("error and always records flush synchronously", "[logging]")
{
    const log_files files;
    nom::init_main_logger(files.config());
    ERROR("error is visible immediately");
    CHECK(files.read().find("error is visible immediately") != std::string::npos);
    ALWAYS("always is visible immediately");
    CHECK(files.read().find("always is visible immediately") != std::string::npos);
}

TEST_CASE("reinitializing a rotating destination accounts for buffered records", "[logging]")
{
    const log_files files;
    auto config = files.config();
    config.file->rotation_size = nom::cfg_uint{256};
    nom::init_main_logger(config);
    INFO("before {}", std::string(180, 'x'));

    nom::init_main_logger(config);
    INFO("after {}", std::string(180, 'y'));
    nom::flush_main_logger();

    CHECK(files.read("main.1.log").find("before ") != std::string::npos);
    CHECK(files.read().find("before ") == std::string::npos);
    CHECK(files.read().find("after ") != std::string::npos);
}
