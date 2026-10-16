//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// GENERATED FILE.  DO NOT EDIT.
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyUtils.h"
#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>
#include "pxr/usd/usdSkel/tokens.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define _ADD_TOKEN(cls, name) \
    cls.def_prop_ro_static(#name, [](nanobind::handle) { return UsdSkelTokens->name.GetString(); }, nanobind::rv_policy::copy);

void wrapUsdSkelTokens(nanobind::module_ &m)
{
    nanobind::class_<UsdSkelTokensType> cls(m, "Tokens");
    cls.def("__init__", [](nanobind::args, nanobind::kwargs) {
        TfPyThrowRuntimeError("This class cannot be instantiated from Python");
    });
    _ADD_TOKEN(cls, bindTransforms);
    _ADD_TOKEN(cls, blendShapes);
    _ADD_TOKEN(cls, blendShapeWeights);
    _ADD_TOKEN(cls, classicLinear);
    _ADD_TOKEN(cls, dualQuaternion);
    _ADD_TOKEN(cls, jointNames);
    _ADD_TOKEN(cls, joints);
    _ADD_TOKEN(cls, normalOffsets);
    _ADD_TOKEN(cls, offsets);
    _ADD_TOKEN(cls, pointIndices);
    _ADD_TOKEN(cls, primvarsSkelGeomBindTransform);
    _ADD_TOKEN(cls, primvarsSkelJointIndices);
    _ADD_TOKEN(cls, primvarsSkelJointWeights);
    _ADD_TOKEN(cls, primvarsSkelSkinningMethod);
    _ADD_TOKEN(cls, restTransforms);
    _ADD_TOKEN(cls, rotations);
    _ADD_TOKEN(cls, scales);
    _ADD_TOKEN(cls, skelAnimationSource);
    _ADD_TOKEN(cls, skelBlendShapes);
    _ADD_TOKEN(cls, skelBlendShapeTargets);
    _ADD_TOKEN(cls, skelJoints);
    _ADD_TOKEN(cls, skelSkeleton);
    _ADD_TOKEN(cls, translations);
    _ADD_TOKEN(cls, weight);
    _ADD_TOKEN(cls, BlendShape);
    _ADD_TOKEN(cls, SkelAnimation);
    _ADD_TOKEN(cls, SkelBindingAPI);
    _ADD_TOKEN(cls, Skeleton);
    _ADD_TOKEN(cls, SkelRoot);
}
