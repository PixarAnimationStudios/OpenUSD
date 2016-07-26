//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#include "pxr/base/arch/errno.h"
#include "pxr/base/arch/defines.h"
#include <cerrno>
#include <cstring>

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
#endif // _GNU_SOURCE

#if defined(ARCH_COMPILER_MSVC)
    strerror_s(msg_buf, 256, errorCode);
#else
    strerror_r(errorCode, msg_buf, 256);
#endif
    return msg_buf;
}
