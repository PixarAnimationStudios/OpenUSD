//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Written by dhl (10 Jul 2006)
//

#include "pxr/pxr.h"
#include "pxr/base/arch/error.h"
#include "pxr/base/arch/testArchUtil.h"

PXR_NAMESPACE_USING_DIRECTIVE

int main(int argc, char** argv)
{
    ArchTestCrashArgParse(argc, argv);
    ArchTestCrash(ArchTestCrashMode::Error);
}
