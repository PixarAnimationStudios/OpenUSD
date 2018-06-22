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
#include "pxr/usd/usdSkel/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdSkelTokensType::UsdSkelTokensType() :
    blendShapes("blendShapes", TfToken::Immortal),
    blendShapeWeights("blendShapeWeights", TfToken::Immortal),
    joints("joints", TfToken::Immortal),
    offsets("offsets", TfToken::Immortal),
    pointIndices("pointIndices", TfToken::Immortal),
    primvarsSkelGeomBindTransform("primvars:skel:geomBindTransform", TfToken::Immortal),
    primvarsSkelJointIndices("primvars:skel:jointIndices", TfToken::Immortal),
    primvarsSkelJointWeights("primvars:skel:jointWeights", TfToken::Immortal),
    restTransforms("restTransforms", TfToken::Immortal),
    rotations("rotations", TfToken::Immortal),
    scales("scales", TfToken::Immortal),
    skelAnimationSource("skel:animationSource", TfToken::Immortal),
    skelBlendShapes("skel:blendShapes", TfToken::Immortal),
    skelBlendShapeTargets("skel:blendShapeTargets", TfToken::Immortal),
    skelJoints("skel:joints", TfToken::Immortal),
    skelSkeleton("skel:skeleton", TfToken::Immortal),
    translations("translations", TfToken::Immortal),
    weight("weight", TfToken::Immortal),
    allTokens({
        blendShapes,
        blendShapeWeights,
        joints,
        offsets,
        pointIndices,
        primvarsSkelGeomBindTransform,
        primvarsSkelJointIndices,
        primvarsSkelJointWeights,
        restTransforms,
        rotations,
        scales,
        skelAnimationSource,
        skelBlendShapes,
        skelBlendShapeTargets,
        skelJoints,
        skelSkeleton,
        translations,
        weight
    })
{
}

TfStaticData<UsdSkelTokensType> UsdSkelTokens;

PXR_NAMESPACE_CLOSE_SCOPE
