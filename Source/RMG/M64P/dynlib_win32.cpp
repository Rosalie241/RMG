#include "dynlib.hpp"
#include <windows.h>

HMODULE dynlib_open(char *file)
{
    return LoadLibrary(file);
}

void dynlib_close(HMODULE dynlib)
{
    FreeLibrary(dynlib);
}

FARPROC dynlib_sym(HMODULE dynlib, char *sym)
{
    return GetProcAddress(dynlib, sym);
}

QString dynlib_strerror(void)
{
    DWORD err = GetLastError();
    LPSTR buf = nullptr;

    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&buf, 0, NULL);

    std::string msg(buf, size);

    LocalFree(buf);

    return QString::fromStdString(msg);
}
