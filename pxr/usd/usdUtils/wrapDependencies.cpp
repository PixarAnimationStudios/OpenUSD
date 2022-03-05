//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
///
/// \file usdUtils/wrapDependencies.cpp

#include "pxr/pxr.h"
#include <boost/python/def.hpp>
#include <boost/python/list.hpp>
#include <boost/python/tuple.hpp>

#include "pxr/base/tf/makePyConstructor.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyResultConversions.h"

#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/usdUtils/dependencies.h"


PXR_NAMESPACE_USING_DIRECTIVE

namespace {

static boost::python::tuple
_ExtractExternalReferences(
    const std::string& filePath)
{
    std::vector<std::string> subLayers, references, payloads;
    UsdUtilsExtractExternalReferences(filePath,
        &subLayers, &references, &payloads);
    return boost::python::make_tuple(subLayers, references, payloads);
}

// Helper for creating a python object holding a layer ref ptr.
boost::python::object
_LayerRefToObj(const SdfLayerRefPtr& layer)
{
    using RefPtrFactory = Tf_MakePyConstructor::RefPtrFactory<>::
            apply<SdfLayerRefPtr>::type;
    return boost::python::object(boost::python::handle<>(RefPtrFactory()(layer)));
}

static boost::python::tuple
_ComputeAllDependencies(const SdfAssetPath &assetPath) 
{
    std::vector<SdfLayerRefPtr> layers;
    std::vector<std::string> assets, unresolvedPaths;
    
    UsdUtilsComputeAllDependencies(assetPath, &layers, &assets, 
                                   &unresolvedPaths);
    boost::python::list layersList;
    for (auto &l: layers) { 
        layersList.append(_LayerRefToObj(l)); 
    }
    return boost::python::make_tuple(layersList, assets, unresolvedPaths);
}

} // anonymous namespace 

void wrapDependencies()
{
    boost::python::def("ExtractExternalReferences", _ExtractExternalReferences,
            boost::python::arg("filePath"));

    boost::python::def("CreateNewUsdzPackage", UsdUtilsCreateNewUsdzPackage,
            (boost::python::arg("assetPath"),
             boost::python::arg("usdzFilePath"),
             boost::python::arg("firstLayerName") = std::string()));

    boost::python::def("CreateNewARKitUsdzPackage", UsdUtilsCreateNewARKitUsdzPackage,
            (boost::python::arg("assetPath"),
             boost::python::arg("usdzFilePath"),
             boost::python::arg("firstLayerName") = std::string()));

    boost::python::def("ComputeAllDependencies", _ComputeAllDependencies,
            (boost::python::arg("assetPath")));

    using Py_UsdUtilsModifyAssetPathFn = std::string(const std::string&);
    TfPyFunctionFromPython<Py_UsdUtilsModifyAssetPathFn>();
    boost::python::def("ModifyAssetPaths", &UsdUtilsModifyAssetPaths,
        (boost::python::arg("layer"), boost::python::arg("modifyFn")));

}
