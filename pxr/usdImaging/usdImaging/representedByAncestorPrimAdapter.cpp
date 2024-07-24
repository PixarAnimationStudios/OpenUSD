//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/usdImaging/usdImaging/representedByAncestorPrimAdapter.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingRepresentedByAncestorPrimAdapter Adapter;
    TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    // No factory here, UsdImagingRepresentedByAncestorPrimAdapter is abstract.
}

TfTokenVector
UsdImagingRepresentedByAncestorPrimAdapter::GetImagingSubprims(UsdPrim const& prim)
{
    return TfTokenVector();
}

UsdImagingPrimAdapter::PopulationMode
UsdImagingRepresentedByAncestorPrimAdapter::GetPopulationMode()
{
    return RepresentedByAncestor;
}

SdfPath
UsdImagingRepresentedByAncestorPrimAdapter::Populate(
    UsdPrim const& prim,
    UsdImagingIndexProxy* index,
    UsdImagingInstancerContext const *instancerContext)
{
    return SdfPath::EmptyPath();
}

HdDirtyBits
UsdImagingRepresentedByAncestorPrimAdapter::ProcessPropertyChange(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    TfToken const& propertyName)
{
    return HdChangeTracker::Clean;
}

PXR_NAMESPACE_CLOSE_SCOPE
