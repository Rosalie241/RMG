/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef VRU_WORDS_HPP
#define VRU_WORDS_HPP

#include <string>
#include <vector>

struct VRUWordEntry
{
    std::string hex;
    std::string words;
};

std::vector<VRUWordEntry> GetVRUWordEntries(void);

#endif // VRU_WORDS_HPP