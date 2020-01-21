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
#include "pxr/pxr.h"
#include <boost/python/def.hpp>

#include "pxr/usd/usdUtils/flattenLayerStack.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyPtrHelpers.h"
#include "pxr/base/tf/makePyConstructor.h"

using namespace boost::python;

PXR_NAMESPACE_USING_DIRECTIVE

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

void wrapFlattenLayerStack()
{
    def("FlattenLayerStack",
        &_UsdUtilsFlattenLayerStack2,
        (arg("stage"), arg("tag")=std::string()),
        boost::python::return_value_policy<
        TfPyRefPtrFactory<SdfLayerHandle> >());

    TfPyFunctionFromPython<Py_UsdUtilsResolveAssetPathSig>();
    def("FlattenLayerStack",
        &_UsdUtilsFlattenLayerStack3,
        (arg("stage"), arg("resolveAssetPathFn"), arg("tag")=std::string()),
        boost::python::return_value_policy<
        TfPyRefPtrFactory<SdfLayerHandle> >());

    def("FlattenLayerStackResolveAssetPath",
        UsdUtilsFlattenLayerStackResolveAssetPath,
        (arg("sourceLayer"), arg("assetPath")));
}
