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

#include "pxr/pxr.h"

#include "pxr/base/arch/systemInfo.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/error.h"
#include <cstdlib>
#include <functional>

#if defined(ARCH_OS_LINUX)

    #include <sys/types.h>
    #include <sys/stat.h>
    #include <unistd.h>

#elif defined(ARCH_OS_DARWIN)

    #include <unistd.h>
    #include <mach-o/dyld.h>

#elif defined(ARCH_OS_WINDOWS)

    #include <Windows.h>
    #include <direct.h>
    #define getcwd(buffer_, size_) _getcwd(buffer_, size_)

#else

    #error Unknown architecture.    

#endif

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

std::string
ArchGetCwd()
{
    // Try a fixed size buffer.
    char buffer[ARCH_PATH_MAX];
    if (getcwd(buffer, ARCH_PATH_MAX)) {
        return std::string(buffer);
    }

    // Let the system allocate the buffer.
    if (char* buf = getcwd(NULL, 0)) {
        std::string result(buf);
        free(buf);
        return result;
    }

    ARCH_WARNING("can't determine working directory");
    return ".";
}

namespace {

// Getting the executable path requires a dynamically allocated buffer
// on all platforms.  This helper function handles the allocation.
static std::string
_DynamicSizedRead(
    size_t initialSize,
    const std::function<bool(char*, size_t*)>& callback)
{
    // Make a buffer for the data.
    // We use an explicit deleter to work around libc++ bug.
    // See https://llvm.org/bugs/show_bug.cgi?id=18350.
    std::unique_ptr<char, std::default_delete<char[]> > buffer;
    buffer.reset(new char[initialSize]);

    // Repeatedly invoke the callback with our buffer until it's big enough.
    size_t size = initialSize;
    while (!callback(buffer.get(), &size)) {
        if (size == static_cast<size_t>(-1)) {
            // callback is never going to succeed.
            return std::string();
        }
        buffer.reset(new char[size]);
    }

    // Make a string.
    return std::string(buffer.get());
}

}

std::string
ArchGetExecutablePath()
{
#if defined(ARCH_OS_LINUX)

    // On Linux the executable path is retrieved from the /proc/self/exe
    // symlink.
    return
        _DynamicSizedRead(ARCH_PATH_MAX,
            [](char* buffer, size_t* size) {
                const ssize_t n = readlink("/proc/self/exe", buffer, *size);
                // Need to do this check BEFORE  the n >= *size, because
                // when comparing signed and unsigned, the signed is
                // automatically converted to unsigned, so -1 >= 2U is true.
                if (n == -1) {
                    ARCH_WARNING("Unable to read /proc/self/exe to obtain "
                                 "executable path");
                    *size = -1;
                    return false;
                }
                else if (static_cast<size_t>(n) >= *size) {
                    // Find out how much space we need.
                    struct stat sb;
                    if (lstat("/proc/self/exe", &sb) == 0) {
                        *size = sb.st_size + 1;
                    }
                    else {
                        // Try iterating on the size.
                        *size *= 2;
                    }
                    return false;
                }
                else {
                    buffer[n] = '\0';
                    return true;
                }
            });

#elif defined(ARCH_OS_DARWIN)

    // On Darwin _NSGetExecutablePath() returns the executable path.
    return
        _DynamicSizedRead(ARCH_PATH_MAX,
            [](char* buffer, size_t* size) {
                uint32_t bufsize = *size;
                if (_NSGetExecutablePath(buffer, &bufsize) == -1) {
                    // We're told the correct size.
                    *size = bufsize;
                    return false;
                }
                else {
                    return true;
                }
            });

#elif defined(ARCH_OS_WINDOWS)

    // On Windows GetModuleFileName() returns the executable path.
    return
        _DynamicSizedRead(ARCH_PATH_MAX,
            [](char* buffer, size_t* size) {
                DWORD nSize = *size;
                const DWORD n = GetModuleFileName(NULL, buffer, nSize);
                if (n == 0) {
                    ARCH_WARNING("Unable to read GetModuleFileName() to obtain "
                                 "executable path");
                    *size = -1;
                    return false;
                }
                else if (n >= nSize) {
                    // We have to iterate to find a suitable size.
                    *size *= 2;
                    return false;
                }
                else {
                    return true;
                }
            });

#endif
}

PXR_NAMESPACE_CLOSE_SCOPE
