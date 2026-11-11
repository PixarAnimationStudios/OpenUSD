//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/base/tf/diagnosticLite.h"
#include "pxr/base/tf/type.h"
#include "pxr/exec/execTest/publicClass.h"

PXR_NAMESPACE_USING_DIRECTIVE

// Test that the public class from the test-only library can be used, and that
// it is defined in the TfType registry by a registry function in that library.
//
static void
TestPublicClass()
{
    ExecTestPublicClass publicClass;
    (void)publicClass;

    TF_AXIOM(!TfType::Find<ExecTestPublicClass>().IsUnknown());
}

int
main()
{
    TestPublicClass();

    return 0;
}
