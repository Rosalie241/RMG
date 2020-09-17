#include "PluginApi.hpp"
#include "Macros.hpp"

using namespace M64P;

PluginApi::PluginApi(void)
{
}

PluginApi::~PluginApi(void)
{
}

bool PluginApi::Hook(m64p_dynlib_handle handle)
{
    this->error_Message = "PluginApi::Hook Failed: ";

    HOOK_FUNC(handle, Plugin, Startup);
    HOOK_FUNC(handle, Plugin, Shutdown);
    HOOK_FUNC_OPT(handle, Plugin, Config);
    HOOK_FUNC(handle, Plugin, GetVersion);

    return true;
}

QString PluginApi::GetLastError(void)
{
    return this->error_Message;
}
