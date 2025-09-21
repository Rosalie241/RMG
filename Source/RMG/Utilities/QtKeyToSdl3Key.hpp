/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020-2025 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef QTKEYTOSDL3KEY_HPP
#define QTKEYTOSDL3KEY_HPP

#include <QObject>

namespace Utilities
{
int QtKeyToSdl3Key(int key);
int QtModKeyToSdl3ModKey(Qt::KeyboardModifiers modifiers);
} // namespace Utilities

#endif // QTKEYTOSDL3KEY_HPP
