#ifndef M64P_COREAPI_HPP
#define M64P_COREAPI_HPP

#include "api/m64p_frontend.h"
#include "api/m64p_common.h"

#include <QString>

namespace M64P
{
    class CoreApi
    {
    public:
        CoreApi();
        ~CoreApi();

        bool Hook(m64p_dynlib_handle handle);
        bool IsHooked(void);

        QString GetLastError(void);

        ptr_CoreStartup Startup;
        ptr_CoreShutdown Shutdown;
        ptr_CoreAttachPlugin AttachPlugin;
        ptr_CoreDetachPlugin DetachPlugin;
        ptr_CoreDoCommand DoCommand;
        ptr_CoreOverrideVidExt OverrideVidExt;
        ptr_CoreAddCheat AddCheat;
        ptr_CoreCheatEnabled CheatEnabled;
        ptr_CoreGetRomSettings GetRomSettings;
        ptr_CoreGetAPIVersions GetAPIVersions;
        ptr_CoreErrorMessage ErrorMessage;

    private:
        bool hooked = false;
        
        QString error_Message;
    };
} // namespace M64P

#endif // M64P_COREAPI_HPP
