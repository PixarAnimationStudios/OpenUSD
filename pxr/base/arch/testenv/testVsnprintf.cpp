//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/vsnprintf.h"
#include "pxr/base/arch/error.h"

#include <cstdlib>
#include <iostream>
#include <string.h>

PXR_NAMESPACE_USING_DIRECTIVE

using std::string;

static int ArchSnprintf(char *str, size_t size, const char* fmt, ...) {
    int n;
    va_list ap;
    va_start(ap, fmt);

    n = ArchVsnprintf(str, size, fmt, ap);

    va_end(ap);
    return n;
}

int main()
{
    char str[1] = "";

    // ArchSnprintf should report 3 characters not written 
    ARCH_AXIOM(ArchSnprintf(str, strlen(str), "   ") == 3);

    // ensure that a string longer than 4096 works
    // create a long format string
    char long_fmt[8192];
    for(int i=0;i<8191;++i) {
        long_fmt[i] = ' ';
    }
    long_fmt[8191] = '\0';

    ARCH_AXIOM(ArchStringPrintf("%s", long_fmt).size() == 8191);

    return 0;
}
