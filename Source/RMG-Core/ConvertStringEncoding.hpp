/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_CONVERTSTRINGENCODING_HPP
#define CORE_CONVERTSTRINGENCODING_HPP
#ifdef CORE_INTERNAL

#include <string>

enum class CoreStringEncoding : int
{
    EUC_JP = 0,
    Shift_JIS = 1
};

// attempts to convert the string (which uses encoding) to UTF-8,
// on failure, returns original string
std::string CoreConvertStringEncoding(std::string str, CoreStringEncoding encoding);

#endif // CORE_INTERNAL
#endif // CORE_CONVERTSTRINGENCODING_HPP