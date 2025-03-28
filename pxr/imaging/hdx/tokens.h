//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HDX_TOKENS_H
#define PXR_IMAGING_HDX_TOKENS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdx/api.h"
#include "pxr/imaging/hdx/version.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE


#define HDX_TOKENS              \
    (hdxOitCounterBuffer)       \
    (hdxOitDataBuffer)          \
    (hdxOitDepthBuffer)         \
    (hdxOitIndexBuffer)         \
    (hdxSelectionBuffer)        \
    (imagerVersion)             \
    (lightingContext)           \
    (lightingShader)            \
    (occludedSelectionOpacity)  \
    (oitCounter)                \
    (oitData)                   \
    (oitDepth)                  \
    (oitIndices)                \
    (oitUniforms)               \
    (oitCounterBufferBar)       \
    (oitDataBufferBar)          \
    (oitDepthBufferBar)         \
    (oitIndexBufferBar)         \
    (oitUniformBar)             \
    (oitRenderPassState)        \
    (oitScreenSize)             \
    (oitRequestFlag)            \
    (oitClearedFlag)            \
    (renderPassState)           \
    (renderIndexVersion)        \
    (selection)                 \
    (selectionState)            \
    (selectionOffsets)          \
    (selectionUniforms)         \
    (selColor)                  \
    (selLocateColor)            \
    (selectionPointColors)      \
    (drawTargetRenderPasses)

TF_DECLARE_PUBLIC_TOKENS(HdxTokens, HDX_API, HDX_TOKENS);

#define HDX_PRIMITIVE_TOKENS    \
    (lightTypePositional)       \
    (lightTypeDirectional)      \
    (lightTypeSpot)             \
                                \
    (aovInputTask)              \
    (boundingBoxTask)           \
    (colorCorrectionTask)       \
    (colorizeSelectionTask)     \
    (drawTargetTask)            \
    (drawTargetResolveTask)     \
    (oitRenderTask)             \
    (oitResolveTask)            \
    (oitVolumeRenderTask)       \
    (pickTask)                  \
    (pickFromRenderBufferTask)  \
    (presentTask)               \
    (renderTask)                \
    (renderSetupTask)           \
    (simpleLightTask)           \
    (shadowTask)

TF_DECLARE_PUBLIC_TOKENS(HdxPrimitiveTokens, HDX_API, HDX_PRIMITIVE_TOKENS);

// inCameraGuide is for guides for a camera that only show up when looking
// through that camera.

#define HDX_RENDERTAG_TOKENS   \
    (renderingGuide)            \
    (label)                     \
    (cameraGuide)               \
    (inCameraGuide)             \
    (streamline)                \
    (interactiveOnlyGeom)       \
    (path)

TF_DECLARE_PUBLIC_TOKENS(HdxRenderTagTokens, HDX_API, HDX_RENDERTAG_TOKENS);

#define HDX_COLOR_CORRECTION_TOKENS             \
    (disabled)                                  \
    (sRGB)                                      \
    (openColorIO)

TF_DECLARE_PUBLIC_TOKENS(HdxColorCorrectionTokens, HDX_API, 
                         HDX_COLOR_CORRECTION_TOKENS);

// Color channels
#define HDX_COLOR_CHANNEL_TOKENS  \
    (color)                         \
    (red)                           \
    (green)                         \
    (blue)                          \
    (alpha)                         \
    (luminance)

TF_DECLARE_PUBLIC_TOKENS(HdxColorChannelTokens, HDX_API, 
                         HDX_COLOR_CHANNEL_TOKENS);

// Color channels
#define HDX_AOV_TOKENS  \
    /* colorIntermediate->colorIntermediate is used to ping-pong
     * between two color targets when a task wishes to
     * read from the color target and also write into it.
     */                                         \
    (colorIntermediate)                         \
    /* depthIntermediate functions analogously for depth targets.
     */                                         \
    (depthIntermediate)

TF_DECLARE_PUBLIC_TOKENS(HdxAovTokens, HDX_API, 
                         HDX_AOV_TOKENS);

// Simple lighting
#define HDX_SIMPLELIGHTTASK_TOKENS \
    (lighting)                     \
    (lightingContext)              \
    (useLighting)                  \
    (useColorMaterialDiffuse)      \
    (lightSource)                  \
    (position)                     \
    (ambient)                      \
    (diffuse)                      \
    (specular)                     \
    (spotDirection)                \
    (spotCutoff)                   \
    (spotFalloff)                  \
    (attenuation)                  \
    (worldToLightTransform)        \
    (shadowIndexStart)             \
    (shadowIndexEnd)               \
    (hasShadow)                    \
    (isIndirectLight)              \
    (shadow)                       \
    (worldToShadowMatrix)          \
    (shadowToWorldMatrix)          \
    (blur)                         \
    (bias)                         \
    (material)                     \
    (emission)                     \
    (sceneColor)                   \
    (shininess)     

TF_DECLARE_PUBLIC_TOKENS(HdxSimpleLightTaskTokens, HDX_API, 
                         HDX_SIMPLELIGHTTASK_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDX_TOKENS_H
