//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/error.h"
#include "pxr/base/arch/debugger.h"
#include <stdio.h>

PXR_NAMESPACE_OPEN_SCOPE

void
Arch_Error(const char* cond, const char* funcName, size_t lineNo, const char* fileName)
{
    fprintf(stderr, " ArchError: %s\n", cond);
    fprintf(stderr, "  Function: %s\n", funcName);
    fprintf(stderr, "      File: %s\n", fileName);
    fprintf(stderr, "      Line: %zu\n", lineNo);
    ArchAbort();
}

void
Arch_Warning(const char* cond, const char* funcName, size_t lineNo, const char* fileName)
{
    fprintf(stderr, " ArchWarn: %s\n", cond);
    fprintf(stderr, " Function: %s\n", funcName);
    fprintf(stderr, "     File: %s\n", fileName);
    fprintf(stderr, "     Line: %zu\n", lineNo);
}

PXR_NAMESPACE_CLOSE_SCOPE
