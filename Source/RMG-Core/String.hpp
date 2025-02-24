/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_STRING_HPP
#define CORE_STRING_HPP

#include <string>

// returns lowercase string
std::string CoreLowerString(std::string str);

// attempts to parse int32_t from string
bool CoreStringToInt(std::string str, int32_t& num);

#endif // CORE_STRING_HPP
