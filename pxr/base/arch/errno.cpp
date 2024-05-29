//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/errno.h"
#include <cerrno>
#include <cstring>
#if defined(ARCH_OS_WINDOWS)
#include <Windows.h>
#endif

PXR_NAMESPACE_OPEN_SCOPE

std::string
ArchStrerror()
{
    return ArchStrerror(errno);
}

std::string
ArchStrerror(int errorCode)
{
    char msg_buf[256];
   
#if defined(_GNU_SOURCE)
    // from strerror_r(3):
    //
    //   The GNU-specific strerror_r() returns a pointer to a string
    //   containing the error message. This may be either a pointer to a
    //   string that the function stores in buf, or a pointer to some
    //   (immutable) static string (in which case buf is unused). If the
    //   function stores a string in buf, then at most buflen bytes are stored
    //   (the string may be truncated if buflen is too small and errnum is
    //   unknown). The string always includes a terminating null byte.
    //
    return strerror_r(errorCode, msg_buf, 256);
#elif !defined(ARCH_COMPILER_MSVC)
    strerror_r(errorCode, msg_buf, 256);
#else
    strerror_s(msg_buf, 256, errorCode);
#endif // _GNU_SOURCE
    return msg_buf;
}

#if defined(ARCH_OS_WINDOWS)
std::string ArchStrSysError(unsigned long errorCode)
{
    if(errorCode == 0)
        return std::string();

    LPSTR buffer = nullptr;
    size_t len = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                               FORMAT_MESSAGE_FROM_SYSTEM |
                               FORMAT_MESSAGE_IGNORE_INSERTS,
                               nullptr,
                               errorCode,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                               (LPSTR)&buffer,
                               0,
                               nullptr);
    std::string message(buffer, len);
    LocalFree(buffer);

    return message;
}
#endif

PXR_NAMESPACE_CLOSE_SCOPE
