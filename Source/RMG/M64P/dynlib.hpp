#ifndef DYNLIB_HPP
#define DYNLIB_HPP

#include <QString>

#ifdef _WIN32
#include <windows.h>
HMODULE dynlib_open(char *);
void dynlib_close(HMODULE);
FARPROC dynlib_sym(HMODULE, const char *);
#else
void *dynlib_open(const char *);
void dynlib_close(void *);
void *dynlib_sym(void *, const char *);
#endif // _WIN32

QString dynlib_strerror(void);

#endif // DYNLIB_HPP