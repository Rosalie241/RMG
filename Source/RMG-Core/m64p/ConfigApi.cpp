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
    this->Unhook();
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

bool ConfigApi::Unhook(void)
{
    UNHOOK_FUNC(Config, ListSections);
    UNHOOK_FUNC(Config, OpenSection);
    UNHOOK_FUNC(Config, ListParameters);
    UNHOOK_FUNC(Config, SaveFile);
    UNHOOK_FUNC(Config, SaveSection);
    UNHOOK_FUNC(Config, HasUnsavedChanges);
    UNHOOK_FUNC(Config, DeleteSection);
    UNHOOK_FUNC(Config, RevertChanges);
    UNHOOK_FUNC(Config, SetParameter);
    UNHOOK_FUNC(Config, SetParameterHelp);
    UNHOOK_FUNC(Config, GetParameter);
    UNHOOK_FUNC(Config, GetParameterType);
    UNHOOK_FUNC(Config, GetParameterHelp);
    UNHOOK_FUNC(Config, SetDefaultInt);
    UNHOOK_FUNC(Config, SetDefaultFloat);
    UNHOOK_FUNC(Config, SetDefaultBool);
    UNHOOK_FUNC(Config, SetDefaultString);
    UNHOOK_FUNC(Config, GetParamInt);
    UNHOOK_FUNC(Config, GetParamFloat);
    UNHOOK_FUNC(Config, GetParamBool);
    UNHOOK_FUNC(Config, GetParamString);
    UNHOOK_FUNC(Config, GetSharedDataFilepath);
    UNHOOK_FUNC(Config, GetUserConfigPath);
    UNHOOK_FUNC(Config, GetUserDataPath);
    UNHOOK_FUNC(Config, GetUserCachePath);
    UNHOOK_FUNC(Config, ExternalOpen);
    UNHOOK_FUNC(Config, ExternalClose);
    UNHOOK_FUNC(Config, ExternalGetParameter);
    UNHOOK_FUNC(Config, SendNetplayConfig);
    UNHOOK_FUNC(Config, ReceiveNetplayConfig);
    UNHOOK_FUNC(Config, OverrideUserPaths);

    this->hooked = false;
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
