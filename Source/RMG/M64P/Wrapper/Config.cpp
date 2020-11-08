/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include <M64P/Api.hpp>
#include <M64P/Wrapper/Config.hpp>

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

    return true;
}

bool Config::OverrideUserPaths(QString data, QString cache)
{
    m64p_error ret;

    ret = M64P::Config.OverrideUserPaths(data.toStdString().c_str(), cache.toStdString().c_str());
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Config::OverrideUserPaths M64P::Config.OverrideUserPaths() Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    return ret == M64ERR_SUCCESS;
}

bool Config::SetOption(QString section, QString key, int value)
{
    return this->section_Open(section) && this->value_Set(key, M64TYPE_INT, (void *)&value);
}

bool Config::SetOption(QString section, QString key, float value)
{
    return this->section_Open(section) && this->value_Set(key, M64TYPE_FLOAT, (void *)&value);
}

bool Config::SetOption(QString section, QString key, bool value)
{
    int boolValue = value ? 1 : 0;

    return this->section_Open(section) && this->value_Set(key, M64TYPE_BOOL, (void *)&boolValue);
}

bool Config::SetOption(QString section, QString key, QString value)
{
    std::string tmpStr = value.toStdString();

    return this->section_Open(section) && this->value_Set(key, M64TYPE_STRING, (void *)tmpStr.c_str());
}

bool Config::SetOption(QString section, QString key, char *value)
{
    return this->section_Open(section) && this->value_Set(key, M64TYPE_STRING, (void *)value);
}

bool Config::SetOption(QString section, QString key, const char *value)
{
    return this->SetOption(section, key, (char *)value);
}

bool Config::GetOption(QString section, QString key, int *value)
{
    return this->section_Open(section) && this->value_Get(key, M64TYPE_INT, value, sizeof(value));
}

bool Config::GetOption(QString section, QString key, float *value)
{
    return this->section_Open(section) && this->value_Get(key, M64TYPE_FLOAT, value, sizeof(value));
}

bool Config::GetOption(QString section, QString key, bool *value)
{
    return this->section_Open(section) && this->value_Get(key, M64TYPE_BOOL, value, sizeof(value));
}

bool Config::GetOption(QString section, QString key, char *value)
{
    return this->section_Open(section) && this->value_Get(key, M64TYPE_STRING, value, sizeof(value));
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

bool Config::SectionExists(QString section)
{
    m64p_error ret;

    this->section_List.clear();

    ret = M64P::Config.ListSections(this, &this->section_List_Handler);
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Config::SectionExists M64P::Config.ListSections Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
        return false;
    }

    return this->section_List.contains(section);
}

bool Config::DeleteSection(QString section)
{
    m64p_error ret;

    ret = M64P::Config.DeleteSection(section.toStdString().c_str());
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Config::DeleteSection M64P::Config.DeleteSection Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    return ret == M64ERR_SUCCESS;
}

bool Config::Save(void)
{
    m64p_error ret;

    ret = M64P::Config.SaveFile();
    if (ret != M64ERR_SUCCESS)
    {
        this->error_Message = "Config::Save: M64P::Config.SaveFile Failed: ";
        this->error_Message += M64P::Core.ErrorMessage(ret);
    }

    return ret == M64ERR_SUCCESS;
}

void Config::section_List_Handler(void *context, const char *section)
{
    ((Config *)context)->section_List.append(QString(section));
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

    return this->Save();
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
