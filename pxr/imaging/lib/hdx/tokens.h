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
#ifndef HDX_TOKENS_H
#define HDX_TOKENS_H

#include "pxr/imaging/hdx/version.h"
#include "pxr/base/tf/staticTokens.h"

#define HDX_TOKENS              \
    (hdxSelectionBuffer)        \
    (imagerVersion)             \
    (lightingContext)           \
    (lightingShader)            \
    (renderPassState)           \
    (renderIndexVersion)        \
    (selection)                 \
    (selectionState)            \
    (selectionOffsets)          \
    (selectionUniforms)         \
    (selColor)                  \
    (selLocateColor)            \
    (selMaskColor)              \
    (selOffset)                 \
    (selOffsetMinMax)           \
    (selValue)                  \
    (drawTargetRenderPasses)

TF_DECLARE_PUBLIC_TOKENS(HdxTokens, HDX_TOKENS);

#define HDX_PRIMITIVE_TOKENS    \
    (lightTypePositional)       \
    (lightTypeDirectional)      \
    (lightTypeSpot)             \
                                \
    (renderTask)                \
    (renderSetupTask)           \
    (simpleLightTask)           \
    (shadowTask)                \
    (drawTargetTask)            \
    (drawTargetResolveTask)

TF_DECLARE_PUBLIC_TOKENS(HdxPrimitiveTokens, HDX_PRIMITIVE_TOKENS);

#define HDX_OPTION_TOKENS    \
    (taskSetAlphaToCoverage)

TF_DECLARE_PUBLIC_TOKENS(HdxOptionTokens, HDX_OPTION_TOKENS);

#endif //HDX_TOKENS_H
