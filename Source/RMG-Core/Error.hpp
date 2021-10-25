/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_ERROR_HPP
#define CORE_ERROR_HPP

#include <string>

// sets error message
void CoreSetError(std::string error);

// retrieves error message
std::string CoreGetError(void);

#endif // CORE_ERROR_HPP