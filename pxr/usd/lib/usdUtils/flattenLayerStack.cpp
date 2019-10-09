//
// Copyright 2017 Pixar
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
#include "pxr/usd/usdUtils/flattenLayerStack.h"
#include "pxr/usd/usd/flattenUtils.h"
#include "pxr/usd/usd/prim.h"

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////
// UsdUtilsFlattenLayerStack
//
// The approach to merge layer stacks into a single layer is as follows:
//
// - _FlattenSpecs() recurses down the typed hierarchy of specs,
//   using PcpComposeSiteChildNames() to discover child names
//   of each type of spec, and creating them in the output layer.
//
// - At each output site, _FlattenFields() flattens field data
//   using a _Reduce() helper to apply composition rules for
//   particular value types and fields.  It uses _ApplyLayerOffset()
//   to handle time-remapping needed, depending on the field.

SdfLayerRefPtr
UsdUtilsFlattenLayerStack(const UsdStagePtr &stage, const std::string& tag)
{
    return UsdUtilsFlattenLayerStack(
        stage, UsdUtilsFlattenLayerStackResolveAssetPath, tag);
}

SdfLayerRefPtr
UsdUtilsFlattenLayerStack(
    const UsdStagePtr &stage, 
    const UsdUtilsResolveAssetPathFn& resolveAssetPathFn,
    const std::string& tag)
{
    PcpPrimIndex index = stage->GetPseudoRoot().GetPrimIndex();
    return UsdFlattenLayerStack(
            index.GetRootNode().GetLayerStack(),
            resolveAssetPathFn, 
            tag);
}

std::string 
UsdUtilsFlattenLayerStackResolveAssetPath(
    const SdfLayerHandle &sourceLayer, 
    const std::string &assetPath)
{
    return UsdFlattenLayerStackResolveAssetPath(sourceLayer, assetPath);
}


PXR_NAMESPACE_CLOSE_SCOPE
