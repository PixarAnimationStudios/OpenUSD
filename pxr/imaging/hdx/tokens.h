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
    (drawTargetRenderPasses)    \
    (aovBindings)

TF_DECLARE_PUBLIC_TOKENS(HdxTokens, HDX_API, HDX_TOKENS);

#define HDX_PRIMITIVE_TOKENS    \
    (lightTypePositional)       \
    (lightTypeDirectional)      \
    (lightTypeSpot)             \
                                \
    (aovInputTask)              \
    (drawTargetTask)            \
    (drawTargetResolveTask)     \
    (colorizeSelectionTask)     \
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

// XXX Deprecated Use: HdStMaterialTagTokens
#define HDX_MATERIALTAG_TOKENS   \
    (additive)                   \
    (translucent)

TF_DECLARE_PUBLIC_TOKENS(HdxMaterialTagTokens, HDX_API, HDX_MATERIALTAG_TOKENS);

#define HDX_COLOR_CORRECTION_TOKENS             \
    (disabled)                                  \
    (sRGB)                                      \
    (openColorIO)                               \
    (channelsOnly)

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

TF_DECLARE_PUBLIC_TOKENS(HdxAovTokens, HDX_API, 
                         HDX_AOV_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HDX_TOKENS_H
