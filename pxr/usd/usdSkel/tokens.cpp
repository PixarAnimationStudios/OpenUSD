//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdSkel/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdSkelTokensType::UsdSkelTokensType() :
    bindTransforms("bindTransforms", TfToken::Immortal),
    blendShapes("blendShapes", TfToken::Immortal),
    blendShapeWeights("blendShapeWeights", TfToken::Immortal),
    classicLinear("classicLinear", TfToken::Immortal),
    dualQuaternion("dualQuaternion", TfToken::Immortal),
    jointNames("jointNames", TfToken::Immortal),
    joints("joints", TfToken::Immortal),
    normalOffsets("normalOffsets", TfToken::Immortal),
    offsets("offsets", TfToken::Immortal),
    pointIndices("pointIndices", TfToken::Immortal),
    primvarsSkelGeomBindTransform("primvars:skel:geomBindTransform", TfToken::Immortal),
    primvarsSkelJointIndices("primvars:skel:jointIndices", TfToken::Immortal),
    primvarsSkelJointWeights("primvars:skel:jointWeights", TfToken::Immortal),
    primvarsSkelSkinningMethod("primvars:skel:skinningMethod", TfToken::Immortal),
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
    BlendShape("BlendShape", TfToken::Immortal),
    SkelAnimation("SkelAnimation", TfToken::Immortal),
    SkelBindingAPI("SkelBindingAPI", TfToken::Immortal),
    Skeleton("Skeleton", TfToken::Immortal),
    SkelRoot("SkelRoot", TfToken::Immortal),
    allTokens({
        bindTransforms,
        blendShapes,
        blendShapeWeights,
        classicLinear,
        dualQuaternion,
        jointNames,
        joints,
        normalOffsets,
        offsets,
        pointIndices,
        primvarsSkelGeomBindTransform,
        primvarsSkelJointIndices,
        primvarsSkelJointWeights,
        primvarsSkelSkinningMethod,
        restTransforms,
        rotations,
        scales,
        skelAnimationSource,
        skelBlendShapes,
        skelBlendShapeTargets,
        skelJoints,
        skelSkeleton,
        translations,
        weight,
        BlendShape,
        SkelAnimation,
        SkelBindingAPI,
        Skeleton,
        SkelRoot
    })
{
}

TfStaticData<UsdSkelTokensType> UsdSkelTokens;

PXR_NAMESPACE_CLOSE_SCOPE
