//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/wrapArray.h"
#include "pxr/base/vt/valueFromPython.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace Vt_WrapArray {
    template <>
    std::string GetVtArrayName< VtArray<SdfAssetPath> >() {
        return "AssetPathArray";
    }
}

template<>
SdfAssetPath VtZero() {
    return SdfAssetPath();
}

PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

void wrapArrayAssetPath() {
    VtWrapArray<VtArray<SdfAssetPath> >();
    VtValueFromPythonLValue<VtArray<SdfAssetPath> >();
}
