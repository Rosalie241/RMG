/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "String.hpp"

#include <algorithm>

//
// Exported Function
//

std::string CoreLowerString(std::string str)
{
	std::string resultString = str;

    std::transform(resultString.begin(), resultString.end(), resultString.begin(), 
        [](unsigned char c)
        { 
            return std::tolower(c); 
        }
    );

    return resultString;
}
