//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

#include "pxr/usd/usdUtils/flattenLayerStack.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/pyFunction.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

static
SdfLayerRefPtr
_UsdUtilsFlattenLayerStack2(
    const UsdStagePtr &stage, 
    const std::string& tag)
{
    return UsdUtilsFlattenLayerStack(stage, tag);
}

using Py_UsdUtilsResolveAssetPathSig = std::string(const SdfLayerHandle&, const std::string&);
using Py_UsdUtilsResolveAssetPathFn = std::function<Py_UsdUtilsResolveAssetPathSig>;

static
SdfLayerRefPtr
_UsdUtilsFlattenLayerStack3(
    const UsdStagePtr &stage,
    const Py_UsdUtilsResolveAssetPathFn& resolveAssetPathFn,
    const std::string& tag)
{
    return UsdUtilsFlattenLayerStack(stage, resolveAssetPathFn, tag);
}

void wrapFlattenLayerStack(nanobind::module_ &m)
{
    m.def("FlattenLayerStack",
        &_UsdUtilsFlattenLayerStack2,
        nb::arg("stage"), nb::arg("tag")=std::string());

    TfPyFunctionFromPython<Py_UsdUtilsResolveAssetPathSig>();
    m.def("FlattenLayerStack",
        &_UsdUtilsFlattenLayerStack3,
        nb::arg("stage"), nb::arg("resolveAssetPathFn"), nb::arg("tag")=std::string());

    m.def("FlattenLayerStackResolveAssetPath",
        UsdUtilsFlattenLayerStackResolveAssetPath,
        nb::arg("sourceLayer"), nb::arg("assetPath"));
}
