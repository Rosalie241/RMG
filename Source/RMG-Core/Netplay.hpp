/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_NETPLAY_HPP
#define CORE_NETPLAY_HPP

#include <string>

// attempts to initialize netplay
bool CoreInitNetplay(std::string address, int port, int player);

// returns whether netplay has been initialized
bool CoreHasInitNetplay(void);

// attempts to shutdown netplay
bool CoreShutdownNetplay(void);

#endif // CORE_NETPLAY_HPP