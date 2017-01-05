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
#include "pxr/base/arch/vsnprintf.h"
#include "pxr/base/arch/error.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string.h>

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

    ARCH_AXIOM(ArchStringPrintf(long_fmt).size() == 8191);

    return 0;
}
