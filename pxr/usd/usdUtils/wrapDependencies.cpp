//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file usdUtils/wrapDependencies.cpp

#include "pxr/pxr.h"
#include "pxr/base/tf/pyNanobindBridge.h"

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/vector.h>

#include "pxr/base/tf/pyFunction.h"

#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/usdUtils/dependencies.h"

PXR_NAMESPACE_USING_DIRECTIVE

namespace nb = nanobind;

namespace {

static nb::tuple
_ExtractExternalReferences(
    const std::string& filePath,
    const UsdUtilsExtractExternalReferencesParams& params = {})
{
    std::vector<std::string> subLayers, references, payloads;
    UsdUtilsExtractExternalReferences(filePath,
        &subLayers, &references, &payloads, params);
    return nb::make_tuple(subLayers, references, payloads);
}

static nb::tuple
_ComputeAllDependencies(
    const SdfAssetPath &assetPath,
    std::function<UsdUtilsProcessingFunc> processingFunc) 
{
    std::vector<SdfLayerRefPtr> layers;
    std::vector<std::string> assets, unresolvedPaths;
    
    UsdUtilsComputeAllDependencies(assetPath, &layers, &assets, 
                                   &unresolvedPaths, processingFunc);
    return nb::make_tuple(TfNbSequenceToList(layers), assets, unresolvedPaths);
}

} // anonymous namespace 

void wrapDependencies(nanobind::module_ &m)
{
    typedef UsdUtilsExtractExternalReferencesParams ExtractRefParams;
    nb::class_<ExtractRefParams>(m, "ExtractExternalReferencesParams")
        .def(nb::init<>())
        .def("SetResolveUdimPaths", &ExtractRefParams::SetResolveUdimPaths)
        .def("GetResolveUdimPaths", &ExtractRefParams::GetResolveUdimPaths)
        ;

    m.def("ExtractExternalReferences", _ExtractExternalReferences,
            nb::arg("filePath"),
            nb::arg("parameters") = ExtractRefParams());

    m.def("CreateNewUsdzPackage", UsdUtilsCreateNewUsdzPackage,
            nb::arg("assetPath"),
            nb::arg("usdzFilePath"),
            nb::arg("firstLayerName") = std::string(),
            nb::arg("editLayersInPlace") = false);

    m.def("CreateNewARKitUsdzPackage", UsdUtilsCreateNewARKitUsdzPackage,
            nb::arg("assetPath"),
            nb::arg("usdzFilePath"),
            nb::arg("firstLayerName") = std::string(),
            nb::arg("editLayersInPlace") = false);

    m.def("ComputeAllDependencies", _ComputeAllDependencies,
            nb::arg("assetPath"),
            nb::arg("processingFunc").none() = nb::none());

    using Py_UsdUtilsModifyAssetPathFn = std::string(const std::string&);
    TfPyFunctionFromPython<Py_UsdUtilsModifyAssetPathFn>();
    m.def("ModifyAssetPaths", &UsdUtilsModifyAssetPaths,
            nb::arg("layer"), 
            nb::arg("modifyFn"), 
            nb::arg("keepEmptyPathsInArrays") = false);

}
