//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HGI_TOKENS_H
#define PXR_IMAGING_HGI_TOKENS_H

#include "pxr/pxr.h"
#include "pxr/base/tf/staticTokens.h"

#include "pxr/imaging/hgi/api.h"

PXR_NAMESPACE_OPEN_SCOPE

#define HGI_TOKENS    \
    (taskDriver)      \
    (renderDriver)    \
    (OpenGL)          \
    (Metal)           \
    (Vulkan)

TF_DECLARE_PUBLIC_TOKENS(HgiTokens, HGI_API, HGI_TOKENS);

#define HGI_SHADER_KEYWORD_TOKENS    \
    (hdPosition) \
    (hdPointCoord) \
    (hdClipDistance) \
    (hdCullDistance) \
    (hdVertexID) \
    (hdInstanceID) \
    (hdPrimitiveID) \
    (hdSampleID) \
    (hdSamplePosition) \
    (hdFragCoord) \
    (hdFrontFacing) \
    (hdLayer) \
    (hdBaseVertex) \
    (hdBaseInstance) \
    (hdViewportIndex) \
    (hdPositionInPatch) \
    (hdPatchID) \
    (hdGlobalInvocationID) \
    (hdBaryCoordNoPersp)  \
    (hdSampleMaskIn) \
    (hdSampleMask) \

TF_DECLARE_PUBLIC_TOKENS(
    HgiShaderKeywordTokens, HGI_API, HGI_SHADER_KEYWORD_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif
