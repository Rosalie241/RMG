#include "Plugin.hpp"
#include "Types.hpp"
#include "../Macros.hpp"
#include "../Api.hpp"

using namespace M64P::Wrapper;

Plugin::Plugin(void)
{
}

Plugin::~Plugin(void)
{
}

bool Plugin::Init(QString file, m64p_dynlib_handle coreHandle)
{
    m64p_error ret;

    this->fileName = file;
    this->handle = DLOPEN(file.toStdString().c_str());
    
    this->error_Message = "Plugin::Init Failed: ";

    if (this->handle == NULL)
    {
        this->error_Message += DLGETERRSTR();
        return false;
    }

    if (!this->plugin.Hook(this->handle))
    {
        this->error_Message += this->plugin.GetLastError();
        return false;
    }

    ret = this->plugin.Startup(coreHandle, NULL, NULL);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message += M64P::Core.ErrorMessage(ret);
        return false;
    }

    if (!this->plugin_t_Get())
        return false;

    return true;
}

bool Plugin::Shutdown(void)
{
    return this->plugin.Shutdown() == M64ERR_SUCCESS;
}

bool Plugin::HasConfig(void)
{
    return this->plugin.Config != NULL;
}

bool Plugin::OpenConfig(void)
{
    return this->plugin.Config() == M64ERR_SUCCESS;
}

m64p_dynlib_handle Plugin::GetHandle(void)
{
    return this->handle;
}

m64p_plugin_type Plugin::GetType(void)
{
    return this->type;
}

Plugin_t Plugin::GetPlugin_t(void)
{
   return this->plugin_t;
}

bool Plugin::plugin_t_Get(void)
{
    const char* name;
    m64p_error ret;

    ret = this->plugin.GetVersion(&this->type, &this->plugin_t.Version, &this->plugin_t.ApiVersion, &name, &this->plugin_t.Capabilities);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message += this->plugin.GetLastError();
        return false;
    }

    this->plugin_t.FileName = this->fileName;
    this->plugin_t.Name = QString::fromStdString(name);
    
    // TODO
    switch (this->type)
    {
        case M64PLUGIN_GFX:
            this->plugin_t.Type = PluginType::Gfx;
            break;
        case M64PLUGIN_AUDIO:
            this->plugin_t.Type = PluginType::Audio;
            break;
        case M64PLUGIN_RSP:
            this->plugin_t.Type = PluginType::Rsp;
            break;
        case M64PLUGIN_INPUT:
            this->plugin_t.Type = PluginType::Input;
            break;
        default:
            this->plugin_t.Type = PluginType::Invalid;
            break;
    }

    return true;
}

QString Plugin::GetLastError(void)
{
    return this->error_Message;
}
