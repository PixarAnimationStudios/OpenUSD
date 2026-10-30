//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/wrapArray.h"
#include "pxr/base/vt/valueFromPython.h"

#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>

PXR_NAMESPACE_OPEN_SCOPE

namespace Vt_WrapArray {
    template <>
    std::string GetVtArrayName< VtArray<SdfPath> >() {
        return "PathArray";
    }
}

template<>
SdfPath VtZero() {
    return SdfPath();
}

PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

void wrapArrayPath(nanobind::module_ &) {
    VtWrapArray<VtArray<SdfPath> >();
    VtValueFromPythonLValue<VtArray<SdfPath> >();
}
