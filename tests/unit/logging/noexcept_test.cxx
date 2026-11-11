// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: MIT

#include <catch2/catch_test_macros.hpp>
#include <fmt/format.h>
#include <spd_logger/logging.h>

#include <cstdio>
#include <unistd.h>

import std;
import nom.spd_logger;

namespace
{
    class stderr_capture
    {
    public:
        stderr_capture() : file_{std::tmpfile(), &std::fclose}
        {
            if (!file_)
                throw std::runtime_error("cannot create stderr capture file");
            std::fflush(stderr);
            saved_ = ::dup(::fileno(stderr));
            if (saved_ < 0 || ::dup2(::fileno(file_.get()), ::fileno(stderr)) < 0)
            {
                restore();
                throw std::runtime_error("cannot redirect stderr");
            }
        }

        ~stderr_capture()
        {
            restore();
        }

        std::string finish()
        {
            restore();
            std::rewind(file_.get());
            std::string output;
            char buffer[512];
            while (const auto size = std::fread(buffer, 1, sizeof(buffer), file_.get()))
                output.append(buffer, size);
            return output;
        }

    private:
        void restore() noexcept
        {
            if (saved_ >= 0)
            {
                std::fflush(stderr);
                ::dup2(saved_, ::fileno(stderr));
                ::close(saved_);
                saved_ = -1;
            }
        }

        std::unique_ptr<std::FILE, int (*)(std::FILE*)> file_{nullptr, &std::fclose};
        int saved_ = -1;
    };
} // namespace

struct failed_conversion
{
    int* calls;
};

int format_as(const failed_conversion& value)
{
    ++*value.calls;
    throw std::runtime_error("conversion failed");
}

struct failed_formatter
{
    int* calls;
};

template <> struct fmt::formatter<failed_formatter> : fmt::formatter<int>
{
    template <typename FormatContext>
    auto format(const failed_formatter& value, FormatContext& context) const -> decltype(context.out())
    {
        ++*value.calls;
        throw std::logic_error("formatting failed");
    }
};

TEST_CASE("error and always macros contain formatting failures without retrying", "[logging][noexcept]")
{
    nom::init_logger(nom::main_logger);
    int conversions = 0;
    int formats = 0;
    stderr_capture capture;
    LERROR("failed conversion {}", failed_conversion{&conversions});
    LALWAYS("failed formatter {}", failed_formatter{&formats});
    const auto output = capture.finish();

    CHECK(conversions == 1);
    CHECK(formats == 1);
    CHECK(output == "failed conversion {}\nconversion failed\nfailed formatter {}\nformatting failed\n");
}

TEST_CASE("noexcept logging contains argument evaluation and nonstandard exceptions", "[logging][noexcept]")
{
    nom::init_logger(nom::main_logger);
    int calls = 0;
    const auto argument = [&]() -> int
    {
        ++calls;
        throw std::runtime_error("argument failed");
    };
    const auto message = [&]() -> std::string
    {
        ++calls;
        throw 17;
    };
    stderr_capture capture;
    LERROR("argument {}", argument());
    LALWAYS(message());
    const auto output = capture.finish();

    CHECK(calls == 2);
    CHECK(output == "\"argument {}\", argument()\nargument failed\nmessage()\nunknown logging exception\n");
}

TEST_CASE("noexcept logging protects logger and level evaluation", "[logging][noexcept]")
{
    nom::logger selected{nom::log_config{}};
    int target_calls = 0;
    int level_calls = 0;
    int argument_calls = 0;
    const auto target = [&]() -> nom::logger&
    {
        ++target_calls;
        throw std::runtime_error("target failed");
    };
    const auto level = [&]() -> nom::log_level
    {
        ++level_calls;
        throw std::runtime_error("level failed");
    };
    stderr_capture capture;
    NOM_LOG_NOEXCEPT(target(), level(), "record {}", ++argument_calls);
    NOM_LOG_NOEXCEPT(selected, level(), "record {}", ++argument_calls);
    const auto output = capture.finish();

    CHECK(target_calls == 1);
    CHECK(level_calls == 1);
    CHECK(argument_calls == 0);
    CHECK(output == "\"record {}\", ++argument_calls\ntarget failed\n"
                    "\"record {}\", ++argument_calls\nlevel failed\n");
}

TEST_CASE("suppressed noexcept calls evaluate only the logger and level", "[logging][noexcept]")
{
    nom::init_logger(nom::main_logger, nom::log_config{.level = {nom::log_level::always}});
    int targets = 0;
    int levels = 0;
    int arguments = 0;
    const auto target = [&]() -> nom::logger&
    {
        ++targets;
        return nom::main_logger;
    };
    const auto level = [&]
    {
        ++levels;
        return nom::log_level::error;
    };
    const auto argument = [&]() -> int
    {
        ++arguments;
        throw std::runtime_error("must not evaluate");
    };
    stderr_capture capture;
    NOM_LOG_NOEXCEPT(target(), level(), "suppressed {}", argument());
    LERROR("also suppressed {}", argument());
    const auto output = capture.finish();

    CHECK(targets == 1);
    CHECK(levels == 1);
    CHECK(arguments == 0);
    CHECK(output.empty());
}

TEST_CASE("single values use their source expression when formatting fails", "[logging][noexcept]")
{
    nom::init_logger(nom::main_logger);
    int calls = 0;
    stderr_capture capture;
    LERROR(failed_formatter{&calls});
    const auto output = capture.finish();

    CHECK(calls == 1);
    CHECK(output == "failed_formatter{&calls}\nformatting failed\n");
}

TEST_CASE("stderr fallback respects format view bounds and handles long text", "[logging][noexcept]")
{
    nom::init_logger(nom::main_logger);
    int calls = 0;
    std::string text(700, 'x');
    text.append("\0{}", 3);
    const auto format_size = text.size();
    text += "do not print this suffix";
    stderr_capture capture;
    LERROR(fmt::runtime(fmt::string_view{text.data(), format_size}), failed_formatter{&calls});
    const auto output = capture.finish();

    CHECK(calls == 1);
    CHECK(output == std::string(700, 'x') + "\\0{}\nformatting failed\n");
}
