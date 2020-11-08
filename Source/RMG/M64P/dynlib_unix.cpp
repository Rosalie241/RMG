#include "dynlib.hpp"
#include <dlfcn.h>

void *dynlib_open(const char *file)
{
    return dlopen(file, RTLD_LAZY);
}

void dynlib_close(void *dynlib)
{
    dlclose(dynlib);
}

void *dynlib_sym(void *dynlib, const char *sym)
{
    return dlsym(dynlib, sym);
}

QString dynlib_strerror(void)
{
    return QString::fromStdString(dlerror());
}
