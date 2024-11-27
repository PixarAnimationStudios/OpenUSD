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

#if defined(ARCH_OS_WINDOWS)
PXR_NAMESPACE_OPEN_SCOPE
// On Windows, the VtArray functions are not being defined in the translation
// unit and are left as undefined symbols during linking. Forcing the
// instantiation here to force the symbols to be created for linking.
template class VtArray<SdfAssetPath>;
PXR_NAMESPACE_CLOSE_SCOPE
#endif // defined(ARCH_OS_WINDOWS)
