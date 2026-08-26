//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_TEST_PUBLIC_CLASS_H
#define PXR_EXEC_EXEC_TEST_PUBLIC_CLASS_H

#include "pxr/pxr.h"

#include "pxr/exec/execTest/api.h"

PXR_NAMESPACE_OPEN_SCOPE

/// An example class defined in a library created with `pxr_test_library`.
///
/// TODO: This only exists to verify unit-tests are able to build against
/// execTest and load it succesfully at runtime. After more functionality is
/// added to this library, this sample class can be removed.
///
class ExecTestPublicClass
{
public:
    EXECTEST_API
    ExecTestPublicClass();

    EXECTEST_API
    ~ExecTestPublicClass();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif