/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Version.hpp"
#include "Library.hpp"
#include "Config.hpp"

//
// Exported Functions
//

CORE_EXPORT std::string CoreGetVersion(void)
{
    return std::string(CORE_VERSION);
}
