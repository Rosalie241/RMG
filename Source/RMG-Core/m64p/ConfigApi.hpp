/*
 * Rosalie's Mupen GUI - https://github.com/Rosalie241/RMG
 *  Copyright (C) 2020 Rosalie Wanders <rosalie@mailbox.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 3.
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef M64P_CONFIGAPI_HPP
#define M64P_CONFIGAPI_HPP

#include "api/m64p_config.h"

#include <string>

namespace m64p
{
class ConfigApi
{
  public:
    ConfigApi(void);
    ~ConfigApi(void);

    bool Hook(m64p_dynlib_handle handle);
    bool IsHooked(void);

    std::string GetLastError(void);

    ptr_ConfigListSections ListSections;
    ptr_ConfigOpenSection OpenSection;
    ptr_ConfigListParameters ListParameters;
    ptr_ConfigSaveFile SaveFile;
    ptr_ConfigSaveSection SaveSection;
    ptr_ConfigHasUnsavedChanges HasUnsavedChanges;
    ptr_ConfigDeleteSection DeleteSection;
    ptr_ConfigRevertChanges RevertChanges;
    ptr_ConfigSetParameter SetParameter;
    ptr_ConfigSetParameterHelp SetParameterHelp;
    ptr_ConfigGetParameter GetParameter;
    ptr_ConfigGetParameterType GetParameterType;
    ptr_ConfigGetParameterHelp GetParameterHelp;
    ptr_ConfigSetDefaultInt SetDefaultInt;
    ptr_ConfigSetDefaultFloat SetDefaultFloat;
    ptr_ConfigSetDefaultBool SetDefaultBool;
    ptr_ConfigSetDefaultString SetDefaultString;
    ptr_ConfigGetParamInt GetParamInt;
    ptr_ConfigGetParamFloat GetParamFloat;
    ptr_ConfigGetParamBool GetParamBool;
    ptr_ConfigGetParamString GetParamString;
    ptr_ConfigGetSharedDataFilepath GetSharedDataFilepath;
    ptr_ConfigGetUserConfigPath GetUserConfigPath;
    ptr_ConfigGetUserDataPath GetUserDataPath;
    ptr_ConfigGetUserCachePath GetUserCachePath;
    ptr_ConfigExternalOpen ExternalOpen;
    ptr_ConfigExternalClose ExternalClose;
    ptr_ConfigExternalGetParameter ExternalGetParameter;
    ptr_ConfigSendNetplayConfig SendNetplayConfig;
    ptr_ConfigReceiveNetplayConfig ReceiveNetplayConfig;
    ptr_ConfigOverrideUserPaths OverrideUserPaths;

  private:
    bool hooked = false;

    std::string errorMessage;
};
} // namespace M64P

#endif // M64P_CONFIGAPI_HPP
