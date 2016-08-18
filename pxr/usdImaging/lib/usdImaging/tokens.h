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
#ifndef USDIMAGING_TOKENS_H
#define USDIMAGING_TOKENS_H

#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/base/tf/staticTokens.h"

#define USDIMAGING_TOKENS   \
    ((infoSource, "info:source")) \
    (faceIndexPrimvar)      \
    (faceOffsetPrimvar)     \
    (ptexFaceIndex)         \
    (ptexFaceOffset)        \
    (usdPopulatedPrimCount) \
    (usdVaryingExtent)      \
    (usdVaryingPrimVar)     \
    (usdVaryingTopology)    \
    (usdVaryingVisibility)  \
    (usdVaryingWidths)      \
    (usdVaryingNormals)     \
    (usdVaryingXform)       \
    (uvPrimvar)

TF_DECLARE_PUBLIC_TOKENS(UsdImagingTokens, USDIMAGING_API, USDIMAGING_TOKENS);

#define USDIMAGING_COLLECTION_TOKENS                    \
    (geometryAndGuides)                                 \
    (geometryAndInteractiveGuides)                      \
    (geometryAndRenderGuides)

TF_DECLARE_PUBLIC_TOKENS(UsdImagingCollectionTokens, USDIMAGING_API, USDIMAGING_COLLECTION_TOKENS);

#endif //USDIMAGING_TOKENS_H
