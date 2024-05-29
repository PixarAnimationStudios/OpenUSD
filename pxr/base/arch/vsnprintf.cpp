//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/vsnprintf.h"

#include <string>

using std::string;

PXR_NAMESPACE_OPEN_SCOPE

int ArchVsnprintf(char *str, size_t size, const char *format, va_list ap)
{
    /*
     * vsnprintf either prints into str, or aborts the print
     * but tells you how much room was needed.  
     */
    return vsnprintf(str, size, format, ap);
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
