// SPDX-FileCopyrightText: 2026 Alexey Tkachenko
// SPDX-License-Identifier: LicenseRef-Nomisma-Labs-Proprietary

module nom.spd_logger;

namespace nom
{
    // Only the wrapper exists before main(); init_logger explicitly creates its backend.
    logger main_logger;
} // namespace nom
