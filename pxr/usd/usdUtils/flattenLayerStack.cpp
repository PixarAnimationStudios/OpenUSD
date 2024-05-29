//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
