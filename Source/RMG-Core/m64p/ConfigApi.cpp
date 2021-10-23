/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#include "ConfigApi.hpp"
#include "Macros.hpp"

using namespace m64p;

ConfigApi::ConfigApi(void)
{
}

ConfigApi::~ConfigApi(void)
{
}

bool ConfigApi::Hook(m64p_dynlib_handle handle)
{
    this->errorMessage = "ConfigApi::Hook Failed: ";

    HOOK_FUNC(handle, Config, ListSections);
    HOOK_FUNC(handle, Config, OpenSection);
    HOOK_FUNC(handle, Config, ListParameters);
    HOOK_FUNC(handle, Config, SaveFile);
    HOOK_FUNC(handle, Config, SaveSection);
    HOOK_FUNC(handle, Config, HasUnsavedChanges);
    HOOK_FUNC(handle, Config, DeleteSection);
    HOOK_FUNC(handle, Config, RevertChanges);
    HOOK_FUNC(handle, Config, SetParameter);
    HOOK_FUNC(handle, Config, SetParameterHelp);
    HOOK_FUNC(handle, Config, GetParameter);
    HOOK_FUNC(handle, Config, GetParameterType);
    HOOK_FUNC(handle, Config, GetParameterHelp);
    HOOK_FUNC(handle, Config, SetDefaultInt);
    HOOK_FUNC(handle, Config, SetDefaultFloat);
    HOOK_FUNC(handle, Config, SetDefaultBool);
    HOOK_FUNC(handle, Config, SetDefaultString);
    HOOK_FUNC(handle, Config, GetParamInt);
    HOOK_FUNC(handle, Config, GetParamFloat);
    HOOK_FUNC(handle, Config, GetParamBool);
    HOOK_FUNC(handle, Config, GetParamString);
    HOOK_FUNC(handle, Config, GetSharedDataFilepath);
    HOOK_FUNC(handle, Config, GetUserConfigPath);
    HOOK_FUNC(handle, Config, GetUserDataPath);
    HOOK_FUNC(handle, Config, GetUserCachePath);
    HOOK_FUNC(handle, Config, ExternalOpen);
    HOOK_FUNC(handle, Config, ExternalClose);
    HOOK_FUNC(handle, Config, ExternalGetParameter);
    HOOK_FUNC(handle, Config, SendNetplayConfig);
    HOOK_FUNC(handle, Config, ReceiveNetplayConfig);
    HOOK_FUNC(handle, Config, OverrideUserPaths);

    this->hooked = true;
    return true;
}

bool ConfigApi::IsHooked(void)
{
    return this->hooked;
}

std::string ConfigApi::GetLastError(void)
{
    return this->errorMessage;
}
