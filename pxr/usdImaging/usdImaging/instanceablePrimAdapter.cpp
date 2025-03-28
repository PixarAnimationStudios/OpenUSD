//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usdImaging/usdImaging/instanceablePrimAdapter.h"

#include "pxr/usdImaging/usdImaging/instancerContext.h"
#include "pxr/usdImaging/usdImaging/primAdapter.h"

#include "pxr/usd/sdf/path.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingInstanceablePrimAdapter Adapter;
    TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter>>();
    // No factory; UsdImagingInstanceablePrimAdapter is abstract
}

SdfPath
UsdImagingInstanceablePrimAdapter::ResolveCachePath(
    const SdfPath& usdPath,
    const UsdImagingInstancerContext* instancerContext) const
{
    // For non-instanced prims, cachePath and usdPath will be the same, however
    // for instanced prims, cachePath will be something like:
    //
    // primPath: /__Prototype_1/cube
    // cachePath: /Models/cube_0/proto_cube_id0
    //
    // The name-mangling is so that multiple instancers/adapters can track the
    // same underlying UsdPrim.
    SdfPath cachePath = usdPath;

    if (instancerContext != nullptr) {
        const SdfPath& instancer = instancerContext->instancerCachePath;
        const TfToken& childName = instancerContext->childName;

        if (!instancer.IsEmpty()) {
            cachePath = instancer;
        }
        if (!childName.IsEmpty()) {
            cachePath = cachePath.AppendChild(childName);
        }
    }
    return cachePath;
}

SdfPath
UsdImagingInstanceablePrimAdapter::ResolveProxyPrimPath(
    const SdfPath& cachePath,
    const UsdImagingInstancerContext* instancerContext) const
{
    if (instancerContext && !instancerContext->instancerCachePath.IsEmpty()) {
        return instancerContext->instancerCachePath.GetAbsoluteRootOrPrimPath();
    }
    return cachePath;
}

PXR_NAMESPACE_CLOSE_SCOPE
