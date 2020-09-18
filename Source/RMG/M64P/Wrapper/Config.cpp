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
    if (!M64P::Config.IsHooked())
    {
        this->error_Message = "M64P::Config is not hooked!";
        return false;
    }

    return true;
}

bool Config::SetOption(QString section, QString key, int value)
{

}

bool Config::SetOption(QString section, QString key, float value)
{

}

bool Config::SetOption(QString section, QString key, bool value)
{

}

bool Config::SetOption(QString section, QString key, QString value)
{

}

QString Config::GetLastError(void)
{
    return this->error_Message;
}
