//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/vt/valueRef.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"

PXR_NAMESPACE_USING_DIRECTIVE

static VtValue
_test_ValueRefFromPython(VtValueRef valueRef)
{
    return valueRef;
}

void wrapValueRef(nanobind::module_& m)
{
    m.def("_test_ValueRefFromPython", _test_ValueRefFromPython);
}
