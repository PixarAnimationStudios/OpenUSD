//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/arch/pragmas.h"

#include <boost/python/class.hpp>
#include <boost/python/def.hpp>

#include "pxr/usd/usd/flattenUtils.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/makePyConstructor.h"

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

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

void wrapUsdFlattenUtils()
{
    def("FlattenLayerStack",
        &_UsdFlattenLayerStack2,
        (arg("layerStack"), arg("tag")=std::string()),
        boost::python::return_value_policy<
        TfPyRefPtrFactory<SdfLayerHandle> >());

    TfPyFunctionFromPython<Py_UsdFlattenResolveAssetPathSig>();
    def("FlattenLayerStack",
        &_UsdFlattenLayerStack3,
        (arg("layerStack"), arg("resolveAssetPathFn"), arg("tag")=std::string()),
        boost::python::return_value_policy<
        TfPyRefPtrFactory<SdfLayerHandle> >());

    def("FlattenLayerStackResolveAssetPath",
        UsdFlattenLayerStackResolveAssetPath,
        (arg("sourceLayer"), arg("assetPath")));

    using Context = UsdFlattenResolveAssetPathContext;
    class_<Context>
        ("FlattenResolveAssetPathContext", no_init)
        .add_property("sourceLayer", 
            +[](const Context& c) { return c.sourceLayer; })
        .add_property("assetPath", 
            +[](const Context& c) { return c.assetPath; })
        .add_property("expressionVariables", 
            +[](const Context& c) { return c.expressionVariables; })
        ;

    using Py_UsdFlattenResolveAssetPathAdvancedSig =
        std::string(const UsdFlattenResolveAssetPathContext&);

    TfPyFunctionFromPython<Py_UsdFlattenResolveAssetPathAdvancedSig>();

    // This function requires a different name to distinguish itself
    // from the other FlattenLayerStack function that also takes a
    // callback because TfPyFunctionFromFunction doesn't let us
    // distinguish between the two different callback types.
    def("FlattenLayerStackAdvanced",
        (SdfLayerRefPtr(*)(
            const PcpLayerStackRefPtr&,
            const UsdFlattenResolveAssetPathAdvancedFn&,
            const std::string&))&UsdFlattenLayerStack,
        (arg("layerStack"), arg("resolveAssetPathFn"), 
            arg("tag")=std::string()),
        return_value_policy<TfPyRefPtrFactory<SdfLayerHandle>>());

    def("FlattenLayerStackResolveAssetPathAdvanced",
        &UsdFlattenLayerStackResolveAssetPathAdvanced,
        (arg("context")));
}
