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
#ifndef PXR_IMAGING_HD_ST_TOKENS_H
#define PXR_IMAGING_HD_ST_TOKENS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE
            
#define HDST_GLSL_PROGRAM_TOKENS                \
    (smoothNormalsFloatToFloat)                 \
    (smoothNormalsFloatToPacked)                \
    (smoothNormalsDoubleToDouble)               \
    (smoothNormalsDoubleToPacked)               \
    (flatNormalsTriFloatToFloat)                \
    (flatNormalsTriFloatToPacked)               \
    (flatNormalsTriDoubleToDouble)              \
    (flatNormalsTriDoubleToPacked)              \
    (flatNormalsQuadFloatToFloat)               \
    (flatNormalsQuadFloatToPacked)              \
    (flatNormalsQuadDoubleToDouble)             \
    (flatNormalsQuadDoubleToPacked)             \
    (flatNormalsTriQuadFloatToFloat)            \
    (flatNormalsTriQuadFloatToPacked)           \
    (flatNormalsTriQuadDoubleToDouble)          \
    (flatNormalsTriQuadDoubleToPacked)          \
    (quadrangulateFloat)                        \
    (quadrangulateDouble)

#define HDST_TOKENS                             \
    (constantLighting)                          \
    (packedSmoothNormals)                       \
    (smoothNormals)                             \
    (packedFlatNormals)                         \
    (flatNormals)                               \
    (scale)                                     \
    (bias)                                      \
    (rotation)                                  \
    (translation)                               \
    (sRGB)                                      \
    (raw)                                       \
    ((_double, "double"))                       \
    ((_float, "float"))                         \
    ((_int, "int"))                             \
    ((colorSpaceAuto, "auto"))                  \
    (fvarIndices)                               \
    (fvarPatchParam)                            \
    (coarseFaceIndex)                           \
    (processedFaceCounts)                       \
    (processedFaceIndices)                      \
    (geomSubsetFaceIndices)                     \
    (pointSizeScale)                            \
    (screenSpaceWidths)                         \
    (minScreenSpaceWidths)                      \
    (shadowCompareTextures)

#define HDST_TEXTURE_TOKENS                     \
    (wrapS)                                     \
    (wrapT)                                     \
    (wrapR)                                     \
    (black)                                     \
    (clamp)                                     \
    (mirror)                                    \
    (repeat)                                    \
    (useMetadata)                               \
    (minFilter)                                 \
    (magFilter)                                 \
    (linear)                                    \
    (nearest)                                   \
    (linearMipmapLinear)                        \
    (linearMipmapNearest)                       \
    (nearestMipmapLinear)                       \
    (nearestMipmapNearest)

#define HDST_RENDER_BUFFER_TOKENS                       \
    ((stormMsaaSampleCount, "storm:msaaSampleCount"))

#define HDST_RENDER_SETTINGS_TOKENS             \
    (enableTinyPrimCulling)                     \
    (volumeRaymarchingStepSize)                 \
    (volumeRaymarchingStepSizeLighting)         \
    (volumeMaxTextureMemoryPerField)            \
    (maxLights)

// Material tags help bucket prims into different queues for draw submission.
// The tags supported by Storm are:
//    defaultMaterialTag : opaque geometry
//    masked : opaque geometry that uses cutout masks (e.g., foliage)
//    translucentToSelection: opaque geometry that allows occluded selection
//                            to show through
//    additive : transparent geometry (cheap OIT solution w/o sorting)
//    translucent: transparent geometry (OIT solution w/ sorted fragment lists)
//    volume : transparent geoometry (raymarched)
#define HDST_MATERIAL_TAG_TOKENS                \
    (defaultMaterialTag)                        \
    (masked)                                    \
    (translucentToSelection)                    \
    (additive)                                  \
    (translucent)                               \
    (volume)

#define HDST_SDR_METADATA_TOKENS                \
    (swizzle)

#define HDST_PERF_TOKENS                        \
    (copyBufferGpuToGpu)                        \
    (copyBufferCpuToGpu)                        \
    (drawItemsCacheHit)                         \
    (drawItemsCacheMiss)                        \
    (drawItemsCacheStale)                       \
    (drawItemsFetched)

TF_DECLARE_PUBLIC_TOKENS(HdStGLSLProgramTokens, HDST_API,
                         HDST_GLSL_PROGRAM_TOKENS);

TF_DECLARE_PUBLIC_TOKENS(HdStTokens, HDST_API, HDST_TOKENS);

TF_DECLARE_PUBLIC_TOKENS(HdStTextureTokens, HDST_API, HDST_TEXTURE_TOKENS);

TF_DECLARE_PUBLIC_TOKENS(HdStRenderBufferTokens, HDST_API,
                         HDST_RENDER_BUFFER_TOKENS);

TF_DECLARE_PUBLIC_TOKENS(HdStRenderSettingsTokens, HDST_API,
                         HDST_RENDER_SETTINGS_TOKENS);

TF_DECLARE_PUBLIC_TOKENS(HdStMaterialTagTokens, HDST_API,
                         HDST_MATERIAL_TAG_TOKENS);

TF_DECLARE_PUBLIC_TOKENS(HdStSdrMetadataTokens, HDST_API, 
                         HDST_SDR_METADATA_TOKENS);   

TF_DECLARE_PUBLIC_TOKENS(HdStPerfTokens, HDST_API, HDST_PERF_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_ST_TOKENS_H
