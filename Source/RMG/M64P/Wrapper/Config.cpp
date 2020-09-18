/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
#include <M64P/Wrapper/Config.hpp>
#include <M64P/Api.hpp>

using namespace M64P::Wrapper;

Config::Config(void)
{
}

Config::~Config(void)
{
}

bool Config::Init()
{
    m64p_error ret;

    if (!M64P::Config.IsHooked())
    {
        this->error_Message = "M64P::Config is not hooked!";
        return false;
    }

    /*ret = M64P::Config.ExternalOpen("Config/RMG.ini", &this->handle);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Config::Init M64P::Config.ExternalOpen Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
        return false;
    }*/

    return true;
}

bool Config::SetOption(QString section, QString key, int value)
{
    return this->section_Open(section) &&
           this->value_Set(key, M64TYPE_INT, (void *)&value);
}

bool Config::SetOption(QString section, QString key, float value)
{
    return this->section_Open(section) &&
           this->value_Set(key, M64TYPE_FLOAT, (void *)&value);
}

bool Config::SetOption(QString section, QString key, bool value)
{
    return this->section_Open(section) &&
           this->value_Set(key, M64TYPE_BOOL, (void *)&value);
}

bool Config::SetOption(QString section, QString key, QString value)
{
    const char *a = value.toStdString().c_str();

    return this->section_Open(section) &&
           this->value_Set(key, M64TYPE_STRING, (void *)a);
}

bool Config::SetOption(QString section, QString key, char *value)
{
    return this->section_Open(section) &&
           this->value_Set(key, M64TYPE_STRING, (void *)value);
}

bool Config::GetOption(QString section, QString key, int *value)
{
    return this->section_Open(section) &&
           this->value_Get(key, M64TYPE_INT, value, sizeof(*value));
}

bool Config::GetOption(QString section, QString key, float *value)
{
    return this->section_Open(section) &&
           this->value_Get(key, M64TYPE_INT, value, sizeof(*value));
}

bool Config::GetOption(QString section, QString key, bool *value)
{
    return this->section_Open(section) &&
           this->value_Get(key, M64TYPE_BOOL, value, sizeof(*value));
}

bool Config::GetOption(QString section, QString key, QString *value)
{
    char data[300];
    std::string str;

    if (!this->section_Open(section))
        return false;

    if (!this->value_Get(key, M64TYPE_STRING, data, sizeof(data)))
        return false;

    str = std::string(data);

    *value = QString::fromStdString(str);

    return true;
}

bool Config::section_Open(QString section)
{
    m64p_error ret;

    ret = M64P::Config.OpenSection(section.toStdString().c_str(), &this->section_Handle);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Config::section_Open Failed: ";
        this->error_Message = M64P::Core.ErrorMessage(ret);
        return false;
    }

    return true;
}

bool Config::value_Set(QString key, m64p_type type, void *value)
{
    m64p_error ret;

    ret = M64P::Config.SetParameter(this->section_Handle, key.toStdString().c_str(), type, value);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Config::value_Set M64P::Config.SetParameter Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
        return false;
    }

    ret = M64P::Config.SaveFile();
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Config::value_Set M64P::Config.SaveFile Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
        return false;
    }

    return true;
}

bool Config::value_Get(QString key, m64p_type type, void *data, int maxSize)
{
    m64p_error ret;

    ret = M64P::Config.GetParameter(this->section_Handle, key.toStdString().c_str(), type, data, maxSize);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Config::value_Get M64P::Config.GetParameter Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
        return false;
    }

    return true;
}

QString Config::GetLastError(void)
{
    return this->error_Message;
}
