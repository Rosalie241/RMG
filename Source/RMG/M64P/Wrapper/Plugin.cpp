/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "Plugin.hpp"
#include "../Api.hpp"
#include "../Macros.hpp"
#include "Types.hpp"

#include <QFileInfo>

using namespace M64P::Wrapper;

Plugin::Plugin(void)
{
}

Plugin::~Plugin(void)
{
}

#include <iostream>
bool Plugin::Init(QString file, m64p_dynlib_handle coreHandle)
{
    m64p_error ret;
    const char *name = nullptr;

    this->fileName = file;
    this->handle = dynlib_open((char *)file.toStdString().c_str());

    this->error_Message = "Plugin::Init Failed: ";

    if (this->handle == NULL)
    {
        this->error_Message += dynlib_strerror();
        return false;
    }

    if (!this->plugin.Hook(this->handle))
    {
        this->error_Message += this->plugin.GetLastError();
        return false;
    }

    this->coreHandle = coreHandle;

    ret = this->plugin.GetVersion(&this->type, &this->version, &this->apiVersion, &name,
                                  &this->capabilities);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message += this->plugin.GetLastError();
        return false;
    }

    // if plugin doesn't provide us with a name,
    // use basename of filepath instead
    if (name == nullptr)
    {
        QFileInfo info(this->fileName);
        this->name = info.fileName();
    }
    else
    {
        this->name = QString(name);
    }

    std::cout << "Plugin::Init name = " << this->name.toStdString() << std::endl;

    return true;
}

bool Plugin::Startup(void)
{
    m64p_error ret;

    ret = this->plugin.Startup(this->coreHandle, NULL, NULL);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Plugin::Startup Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    return (ret == M64ERR_SUCCESS || ret == M64ERR_ALREADY_INIT);
}

bool Plugin::Shutdown(void)
{
    m64p_error ret;

    ret = this->plugin.Shutdown();
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Plugin::Shutdown Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    return ret == M64ERR_SUCCESS;
}

bool Plugin::HasConfig(void)
{
    return this->plugin.Config != NULL;
}

bool Plugin::OpenConfig(void)
{
    m64p_error ret;

    ret = this->plugin.Config();
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Plugin::OpenConfig Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    return ret == M64ERR_SUCCESS;
}

QString Plugin::GetName(void)
{
    return this->name;
}

QString Plugin::GetFileName(void)
{
    return this->fileName;
}

int Plugin::GetVersion(void)
{
    return this->version;
}

int Plugin::GetApiVersion(void)
{
    return this->apiVersion;
}

int Plugin::GetCapabilities(void)
{
    return this->capabilities;
}

m64p_dynlib_handle Plugin::GetHandle(void)
{
    std::cout << "Plugin::GetHandle " << this << ": " << this->handle << std::endl;

    return this->handle;
}

m64p_plugin_type Plugin::GetType(void)
{
    return this->type;
}

QString Plugin::GetLastError(void)
{
    return this->error_Message;
}
