//
// Copyright 2019 Pixar
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
#include "pxr/base/arch/pragmas.h"

ARCH_PRAGMA_PUSH
ARCH_PRAGMA_PLACEMENT_NEW  // because of pyFunction.h and boost::function

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
}

ARCH_PRAGMA_POP
