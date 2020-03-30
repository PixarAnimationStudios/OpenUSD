//
// Copyright 2020 benmalartre
//
// Unlicensed
//
#ifndef PXR_IMAGING_PLUGIN_LOFI_TOKENS_H
#define PXR_IMAGING_PLUGIN_LOFI_TOKENS_H

#include "pxr/pxr.h"
#include "pxr/imaging/plugin/LoFi/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

#define LOFI_TOKENS                             \
    (elementBuffer)                             \
    (glslProgram)                               \
    (glslShader)                                \
    (vertexArray)                               \
    (vertexBuffer)                              \

TF_DECLARE_PUBLIC_TOKENS(LoFiTokens, LOFI_API, LOFI_TOKENS);

PXR_NAMESPACE_CLOSE_SCOPE

#endif //PXR_IMAGING_HD_TOKENS_H
