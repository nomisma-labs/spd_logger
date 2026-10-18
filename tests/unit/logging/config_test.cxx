// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

#include <spd_logger/logging.h>

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <expected>
#include <initializer_list>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>

import nom.spd_logger;

static_assert(
    std::is_same_v<decltype(std::declval<const nom::logfile_config&>().validate()), std::expected<void, std::string>>);
static_assert(
    std::is_same_v<decltype(std::declval<const nom::log_config&>().validate()), std::expected<void, std::string>>);

namespace
{
    void check_invalid_file(const nom::logfile_config& config, const std::string& message)
    {
        INFO("Expected validation error: " << message);
        const auto result = config.validate();
        REQUIRE_FALSE(result.has_value());
        CHECK(result.error() == message);

        const nom::log_config full_config{.file = config};
        const auto full_result = full_config.validate();
        REQUIRE_FALSE(full_result.has_value());
        CHECK(full_result.error() == "/file" + message);
    }
} // namespace

TEST_CASE("console and unrotated file configurations validate for every named severity", "[logging][config]")
{
    const nom::log_config defaults;
    CHECK(defaults.validate().has_value());

    const nom::logfile_config file{.name = "application.log"};
    CHECK(file.validate().has_value());
    for (const auto level : {nom::log_level::debug,
                             nom::log_level::info,
                             nom::log_level::warn,
                             nom::log_level::error,
                             nom::log_level::always})
    {
        const nom::log_config console{.level = level};
        CHECK(console.validate().has_value());
        const nom::log_config configured_file{.level = level, .file = file};
        CHECK(configured_file.validate().has_value());
    }
}

TEST_CASE("file name validation identifies the local and nested field", "[logging][config]")
{
    check_invalid_file(nom::logfile_config{}, "/name must not be empty");

    std::string name = "application.log";
    name.push_back('\0');
    name += "ignored.txt";
    check_invalid_file(nom::logfile_config{.name = name}, "/name must not contain NUL characters");
}

TEST_CASE("rotation validation accepts its positive size_t boundaries", "[logging][config]")
{
    for (const auto size : {std::uint64_t{1}, static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())})
    {
        const nom::logfile_config file{.name = "application.log", .rotation_size = nom::cfg_uint{size}};
        CHECK(file.validate().has_value());
        const nom::log_config config{.file = file};
        CHECK(config.validate().has_value());
    }
}

TEST_CASE("rotation validation rejects values outside the positive size_t range", "[logging][config]")
{
    check_invalid_file(nom::logfile_config{.name = "application.log", .rotation_size = nom::cfg_uint{0}},
                       "/rotation_size must be a positive size_t value");

    if constexpr (std::numeric_limits<std::size_t>::digits < std::numeric_limits<std::uint64_t>::digits)
    {
        auto size = static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max());
        ++size;
        check_invalid_file(nom::logfile_config{.name = "application.log", .rotation_size = nom::cfg_uint{size}},
                           "/rotation_size must be a positive size_t value");
    }
}
