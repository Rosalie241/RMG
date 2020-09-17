#ifndef M64P_CONFIGAPI_HPP
#define M64P_CONFIGAPI_HPP

#include "api/m64p_config.h"

#include <QString>

namespace M64P
{
    class ConfigApi
    {
    public:
        ConfigApi(void);
        ~ConfigApi(void);

        bool Hook(m64p_dynlib_handle handle);
        bool IsHooked(void);

        QString GetLastError(void);

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

    private:
        bool hooked = false;

        QString error_Message;
    };
} // namespace M64P

#endif // M64P_CONFIGAPI_HPP
