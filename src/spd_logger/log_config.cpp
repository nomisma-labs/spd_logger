// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: MIT

module nom.spd_logger;

import std;

namespace nom
{
    std::expected<void, std::string> logfile_config::validate() const
    {
        if (name.empty())
            return std::unexpected("/name must not be empty");
        if (name.find('\0') != std::string::npos)
            return std::unexpected("/name must not contain NUL characters");
        if (rotation_size)
        {
            const auto size = rotation_size->get();
            if (size == 0 || size > std::numeric_limits<std::size_t>::max())
                return std::unexpected("/rotation_size must be a positive size_t value");
        }
        return {};
    }

    std::expected<void, std::string> log_config::validate() const
    {
        if (file)
        {
            if (const auto result = file->validate(); !result)
                return std::unexpected("/file" + result.error());
        }
        return {};
    }
} // namespace nom
