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
#include "pxr/base/arch/vsnprintf.h"

#include <string>

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

int ArchVsnprintf(char *str, size_t size, const char *format, va_list ap)
{
#if defined(ARCH_OS_LINUX) || defined(ARCH_OS_DARWIN)
    /*
     * Built-in vsnprintf either prints into str, or aborts the print
     * but tells you how much room was needed.  
     */
    return vsnprintf(str, size, format, ap);
#elif defined(ARCH_OS_WINDOWS)
    int n = _vscprintf(format, ap);
    if (n < size)
        return vsnprintf_s(str, size /*size of buffer */, n, format, ap);
    else
        return n;
#else
#error Unknown system architecture.    
#endif
}

string
ArchVStringPrintf(const char *fmt, va_list ap)
{
    // on architectures where arguments are passed in registers and
    // thus va_list is not just a pointer to the stack, we need to make
    // a copy of 'ap' in case we need to call ArchVsnprintf twice.
    va_list apcopy;
    va_copy(apcopy, ap);

    char buf[4096]; // past this size, we'll incur a new/delete.
    size_t needed = ArchVsnprintf(buf, sizeof(buf), fmt, ap) + 1;
    string s(needed <= sizeof(buf) ? buf : string());

    if (s.empty()) {
        char* tmp = new char[needed];
        ArchVsnprintf(tmp, needed, fmt, apcopy);
        s = string(tmp);
        delete [] tmp;
    }

    va_end(apcopy);
 
    return s;
}

string
ArchStringPrintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    string s = ArchVStringPrintf(fmt, ap);
    va_end(ap);
    return s;
}

PXR_NAMESPACE_CLOSE_SCOPE
