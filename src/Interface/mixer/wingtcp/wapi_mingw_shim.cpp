/*
  ==============================================================================

    wapi_mingw_shim.cpp
    Created: 17 Apr 2026

    On Windows, the prebuilt libwapi.a is compiled with MinGW GCC which uses
    wrapper symbols like __mingw_printf / __mingw_sscanf / __mingw_fprintf /
    __mingw_sprintf. These are not present in MSVC's CRT, causing LNK2001
    linker errors when the app is built with Visual Studio.

    This file provides shim implementations that simply forward to MSVC's
    standard stdio equivalents. It compiles to nothing on non-Windows
    platforms and on MinGW builds (where the real symbols are already
    provided by the MinGW runtime).

  ==============================================================================
*/

#if defined(_WIN32) && !defined(__MINGW32__)

#include <cstdarg>
#include <cstdio>

extern "C" {

int __mingw_printf(const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const int r = vprintf(fmt, ap);
    va_end(ap);
    return r;
}

int __mingw_fprintf(FILE* stream, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const int r = vfprintf(stream, fmt, ap);
    va_end(ap);
    return r;
}

int __mingw_sprintf(char* buffer, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const int r = vsprintf(buffer, fmt, ap);
    va_end(ap);
    return r;
}

int __mingw_sscanf(const char* buffer, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const int r = vsscanf(buffer, fmt, ap);
    va_end(ap);
    return r;
}

} // extern "C"

#endif
