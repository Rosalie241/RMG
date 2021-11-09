/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_HPP
#define CORE_HPP

#include "Settings/Settings.hpp"
#include "RomSettings.hpp"
#include "Screenshot.hpp"
#include "Emulation.hpp"
#include "SaveState.hpp"
#include "RomHeader.hpp"
#include "Plugins.hpp"
#include "Error.hpp"
#include "Video.hpp"
#include "Key.hpp"

// initializes the core library,
// returns false when failed
bool CoreInit(void);

// shuts down the core library
void CoreShutdown(void);

#endif // CORE_HPP
