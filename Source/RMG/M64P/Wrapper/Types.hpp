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
            Rsp,
            Gfx,
            Audio,
            Input,
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
