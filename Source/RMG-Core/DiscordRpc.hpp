/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_DISCORDRPC_HPP
#define CORE_DISCORDRPC_HPP

// initializes the discord rpc library
void CoreDiscordRpcInit(void);

// shuts down the discord rpc library
void CoreDiscordRpcShutdown(void);

// updates the state of the discord rpc library
void CoreDiscordRpcUpdate(bool inGame);

#endif // CORE_DISCORDRPC_HPP