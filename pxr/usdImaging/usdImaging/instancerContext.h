//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_INSTANCER_CONTEXT_H
#define PXR_USD_IMAGING_USD_IMAGING_INSTANCER_CONTEXT_H

/// \file usdImaging/instancerContext.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE


using UsdImagingPrimAdapterSharedPtr = 
    std::shared_ptr<class UsdImagingPrimAdapter>;

/// \class UsdImagingInstancerContext
///
/// Object used by instancer prim adapters to pass along context
/// about the instancer and instance prim to prototype prim adapters.
///
class UsdImagingInstancerContext
{
public:
    /// The cachePath of the instancer.
    SdfPath instancerCachePath;

    /// The name of the child prim, typically used for prototypes.
    TfToken childName;

    /// The USD path to the material bound to the instance prim
    /// being processed.
    SdfPath instancerMaterialUsdPath;

    /// The draw mode bound to the instance prim being processed.
    TfToken instanceDrawMode;

    // The inheritable purpose bound to the instance prim being processed. If 
    // the instance prim can provide this, prototypes without an explicit or 
    // inherited purpose will inherit this purpose from the instance.
    TfToken instanceInheritablePurpose;

    /// The instancer's prim Adapter. Useful when an adapter is needed, but the
    /// default adapter may be overridden for the sake of instancing.
    UsdImagingPrimAdapterSharedPtr instancerAdapter;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_INSTANCER_CONTEXT_H
