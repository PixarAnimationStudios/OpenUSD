//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    (shadowCompareTextures)                     \
    (storm)

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
//    displayInOverlay : geometry that should be drawn on top (e.g. guides)
//    translucentToSelection: opaque geometry that allows occluded selection
//                            to show through
//    additive : transparent geometry (cheap OIT solution w/o sorting)
//    translucent: transparent geometry (OIT solution w/ sorted fragment lists)
//    volume : transparent geoometry (raymarched)
#define HDST_MATERIAL_TAG_TOKENS                \
    (defaultMaterialTag)                        \
    (masked)                                    \
    (displayInOverlay)                          \
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
