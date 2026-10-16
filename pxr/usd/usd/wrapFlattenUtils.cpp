//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/arch/pragmas.h"

#include "pxr/usd/usd/flattenUtils.h"
#include "pxr/usd/pcp/layerStack.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/vt/valueFromPython.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/makePyConstructor.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

static
SdfLayerRefPtr
_UsdFlattenLayerStack2(
    const PcpLayerStackRefPtr &layerStack,
    const std::string& tag)
{
    return UsdFlattenLayerStack(layerStack, tag);
}

using Py_UsdFlattenResolveAssetPathSig = 
    std::string(const SdfLayerHandle&, const std::string&);
using Py_UsdFlattenResolveAssetPathFn = 
    std::function<Py_UsdFlattenResolveAssetPathSig>;

static
SdfLayerRefPtr
_UsdFlattenLayerStack3(
    const PcpLayerStackRefPtr &layerStack,
    const Py_UsdFlattenResolveAssetPathFn& resolveAssetPathFn,
    const std::string& tag)
{
    return UsdFlattenLayerStack(layerStack, resolveAssetPathFn, tag);
}

void wrapUsdFlattenUtils(nanobind::module_ &m)
{
    m.def("FlattenLayerStack",
        &_UsdFlattenLayerStack2,
        nb::arg("layerStack"), nb::arg("tag")=std::string());

    TfPyFunctionFromPython<Py_UsdFlattenResolveAssetPathSig>();
    m.def("FlattenLayerStack",
        &_UsdFlattenLayerStack3,
        nb::arg("layerStack"), nb::arg("resolveAssetPathFn"),
        nb::arg("tag")=std::string());

    m.def("FlattenLayerStackResolveAssetPath",
        &UsdFlattenLayerStackResolveAssetPath,
        nb::arg("sourceLayer"), nb::arg("assetPath"));

    using Context = UsdFlattenResolveAssetPathContext;
    nb::class_<Context>(m, "FlattenResolveAssetPathContext")
        .def_prop_ro("sourceLayer", 
            +[](const Context& c) { return c.sourceLayer; })
        .def_prop_ro("assetPath", 
            +[](const Context& c) { return c.assetPath; })
        .def_prop_ro("expressionVariables", 
            +[](const Context& c) { return c.expressionVariables; })
        ;

    using Py_UsdFlattenResolveAssetPathAdvancedSig =
        std::string(const UsdFlattenResolveAssetPathContext&);

    TfPyFunctionFromPython<Py_UsdFlattenResolveAssetPathAdvancedSig>();

    // This function requires a different name to distinguish itself
    // from the other FlattenLayerStack function that also takes a
    // callback because TfPyFunctionFromFunction doesn't let us
    // distinguish between the two different callback types.
    m.def("FlattenLayerStackAdvanced",
        (SdfLayerRefPtr(*)(
            const PcpLayerStackRefPtr&,
            const UsdFlattenResolveAssetPathAdvancedFn&,
            const std::string&))&UsdFlattenLayerStack,
        nb::arg("layerStack"), nb::arg("resolveAssetPathFn"), 
        nb::arg("tag")=std::string());

    m.def("FlattenLayerStackResolveAssetPathAdvanced",
        &UsdFlattenLayerStackResolveAssetPathAdvanced,
        nb::arg("context"));
}

