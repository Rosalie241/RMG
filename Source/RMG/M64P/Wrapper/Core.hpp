#ifndef M64P_WRAPPER_CORE_HPP
#define M64P_WRAPPER_CORE_HPP

#include "Types.hpp"
#include "Plugin.hpp"

#include <QString>
#include <QList>

namespace M64P
{
    namespace Wrapper
    {
        class Core
        {
        public:
            Core(void);
            ~Core(void);

            bool Init(m64p_dynlib_handle);

            bool HasPluginConfig(PluginType);
            bool OpenPluginConfig(PluginType);

            QList<Plugin_t> GetPlugins(PluginType);
            bool SetPlugin(Plugin_t);

            bool GetRomInfo(QString, RomInfo_t*);

            bool LaunchEmulation(QString);

            QString GetLastError(void);

        private:
            QString error_Message;

            m64p_dynlib_handle handle;

            M64P::Wrapper::Plugin plugin_Rsp;
            M64P::Wrapper::Plugin plugin_Gfx;
            M64P::Wrapper::Plugin plugin_Audio;
            M64P::Wrapper::Plugin plugin_Input;

            M64P::Wrapper::Plugin* plugin_Get(PluginType);
            bool plugin_Attach(Plugin*);
            bool plugins_Attach(void);
            bool plugins_Detach(void);

            bool rom_Open(QString);
            bool rom_Close(void);
        };
    } // namespace Wrapper
} // namespace M64P

#endif // M64P_WRAPPER_CORE_HPP
