/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef VRU_HPP
#define VRU_HPP

// attempts to initialize VRU capabilities
bool InitVRU(void);

// returns whether VRU has been initialized
bool IsVRUInit(void);

// attempts to de-initialize VRY capabilities
bool QuitVRU(void);

int GetVRUMicState(void);

#endif // VRU_HPP