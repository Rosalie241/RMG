#ifndef M64P_PLUGINAPI_HPP
#define M64P_PLUGINAPI_HPP

#include "api/m64p_common.h"
#include "api/m64p_custom.h"

#include <QString>

namespace M64P
{
    class PluginApi
    {
    public:
        PluginApi(void);
        ~PluginApi(void);

        bool Hook(m64p_dynlib_handle);

        QString GetLastError(void);

        ptr_PluginStartup Startup;
        ptr_PluginShutdown Shutdown;
        ptr_PluginConfig Config;
        ptr_PluginGetVersion GetVersion;

    private:
        QString error_Message;
    };
} // namespace M64P

#endif // M64P_PLUGINAPI_HPP
