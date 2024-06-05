//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/threads.h"
#include "pxr/base/arch/error.h"

PXR_NAMESPACE_USING_DIRECTIVE

int main()
{
    ARCH_AXIOM(ArchIsMainThread());

    return 0;
}

