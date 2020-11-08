/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef M64P_WRAPPER_TYPES_HPP
#define M64P_WRAPPER_TYPES_HPP

#include "../api/m64p_types.h"

#include <QString>

namespace M64P
{
namespace Wrapper
{
enum PluginType
{
    Gfx = 0,
    Audio,
    Input,
    Rsp,
    Invalid
};

struct RomInfo_t
{
    QString FileName;
    m64p_rom_settings Settings;
    m64p_rom_header Header;
};

struct Plugin_t
{
    QString Name;
    QString FileName;
    PluginType Type;
    int Version;
    int ApiVersion;
    int Capabilities;
};
} // namespace Wrapper
} // namespace M64P

#endif // M64P_WRAPPER_TYPES_HPP
