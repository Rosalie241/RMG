/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CORE_MOUSE_HPP
#define CORE_MOUSE_HPP

// sets mouse movement
bool CoreSetMouseMove(int x, int y);

// sets mouse buttons
bool CoreSetMouseButton(int left, int right);

#endif // CORE_MOUSE_HPP
