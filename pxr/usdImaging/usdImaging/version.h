//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_VERSION_H
#define PXR_USD_IMAGING_USD_IMAGING_VERSION_H

#include "pxr/pxr.h"

PXR_NAMESPACE_OPEN_SCOPE



// Version 3 -- add support for nested instancers in InsertInstancer.
// Version 4 -- Populate returns SdfPath, HdxSelectionInstanceMap.
// Version 5 -- GetPathForInstanceIndex returns absoluteInstanceIndex.
// Version 6 -- PrimAdater::GetDependPaths.
// Version 7 -- GetPathForInstanceIndex returns instanceContext.
// Version 8 -- GetPathForInstanceIndex returns instanceContext (as 
//              SdfPathVector) and rprimPath  separately.
// Version 9 -- Rework UsdImagingEngineGL::RenderParams API to conform to
//              updated purpose tokens and make proxy imaging optional.
// Version 10 - Add "UsdPrim" parameter to adapter PopulateSelection.
// Version 11 - PopulateSelection takes "usdPath" (relative to USD stage),
//              rather than a path with the delegate root.
// Version 12 - Adapter PopulateSelection signature change to adapt to
//              flat instance indices in hydra selection.
// Version 13 - Deleted GetPathForInstanceIndex; added GetScenePrimPath.
// Version 14 - Added HdInstancerContext to GetScenePrimPath.
// Version 15 - CanPopulateMaster renamed to CanPopulateUsdInstance.
// Version 16 - InsertRprim/InsertInstancer no longer take an instancer path.
// Version 17 - RequestTrackVariability/RequestUpdateForTime, and UpdateForTime
//              no longer automatically called.
// Version 18 - Geom subsets accessed via UsdImagingDelegate::GetMeshTopology()
//              will now have correctly prefixed index paths for id and 
//              materialId.
// Version 19 - UsdImagingPrimAdapter::InvalidateImagingSubprim takes
//              invalidationType as argument.
// Version 20 - Adding UsdImagingCreateSceneIndices.
// Version 21 - Removes UsdImagingDirectMaterialBindingsSchema and 
//              UsdImagingCollectionMaterial BindingsSchema in favor of
//              UsdImagingMaterialBindingsSchema and 
//              UsdImagingMaterialBindingSchema.

#define USD_IMAGING_API_VERSION 21


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_VERSION_H
