#include "CoreApi.hpp"
#include "Macros.hpp"

using namespace M64P;

CoreApi::CoreApi(void)
{
}

CoreApi::~CoreApi(void)
{
}

bool CoreApi::Hook(m64p_dynlib_handle handle)
{
    this->error_Message = "CoreApi::Hook Failed: ";

    HOOK_FUNC(handle, Core, Startup);
    HOOK_FUNC(handle, Core, Shutdown);
    HOOK_FUNC(handle, Core, AttachPlugin);
    HOOK_FUNC(handle, Core, DetachPlugin);
    HOOK_FUNC(handle, Core, DoCommand);
    HOOK_FUNC(handle, Core, OverrideVidExt);
    HOOK_FUNC(handle, Core, AddCheat);
    HOOK_FUNC(handle, Core, CheatEnabled);
    HOOK_FUNC(handle, Core, GetRomSettings);
    HOOK_FUNC(handle, Core, GetAPIVersions);
    HOOK_FUNC(handle, Core, ErrorMessage);

    this->hooked = true;
    return true;
}

bool CoreApi::IsHooked(void)
{
    return this->hooked;
}

QString CoreApi::GetLastError(void)
{
    return this->error_Message;
}
