#ifndef VIDEXT_HPP
#define VIDEXT_HPP

#include "api/m64p_types.h"
#include <functional>

struct VidExtFuncs
{
    std::function<m64p_error(void)> VidExtFuncInit;
    std::function<m64p_error(void)> VidExtFuncQuit;
};

class VidExt
{
public:
    VidExt();
    ~VidExt();

    void VidExt_Register(VidExtFuncs funcs);
    m64p_video_extension_functions* VidExt_GetFuncs();

private:
    VidExtFuncs funcs;
    m64p_video_extension_functions m64p_vidext_funcs;

};

#endif // VIDEXT_HPP