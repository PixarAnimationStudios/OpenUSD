//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_IMAGING_USD_IMAGING_TOKENS_H
#define PXR_USD_IMAGING_USD_IMAGING_TOKENS_H

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


#define USDIMAGING_TOKENS   \
    ((collectionLightLink, "collection:lightLink")) \
    ((collectionShadowLink, "collection:shadowLink")) \
    ((configPrefix, "config:")) \
    (faceIndexPrimvar)      \
    (faceOffsetPrimvar)     \
    ((primvarsNormals, "primvars:normals")) \
    ((primvarsWidths,  "primvars:widths")) \
    (ptexFaceIndex)         \
    (ptexFaceOffset)        \
    (usdPopulatedPrimCount) \
    (usdVaryingExtent)      \
    (usdVaryingPrimvar)     \
    (usdVaryingTopology)    \
    (usdVaryingVisibility)  \
    (usdVaryingWidths)      \
    (usdVaryingNormals)     \
    (usdVaryingXform)       \
    (usdVaryingTexture)     \
    (uvPrimvar)             \
    (UsdPreviewSurface)     \
    (UsdUVTexture)          \
    (UsdPrimvarReader_float)\
    (UsdPrimvarReader_float2)\
    (UsdPrimvarReader_float3)\
    (UsdPrimvarReader_float4)\
    (UsdPrimvarReader_int)   \
    (UsdTransform2d)         \
    ((stageSceneIndexRepopulate,  "__usdStageSceneIndexRepopulate"))

TF_DECLARE_PUBLIC_TOKENS(
    UsdImagingTokens,
    USDIMAGING_API, USDIMAGING_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_USD_IMAGING_USD_IMAGING_TOKENS_H
