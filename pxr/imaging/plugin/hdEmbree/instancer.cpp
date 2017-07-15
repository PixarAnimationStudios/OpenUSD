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
#include "pxr/imaging/hdEmbree/instancer.h"

#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/quaternion.h"
#include "pxr/base/tf/staticTokens.h"

PXR_NAMESPACE_OPEN_SCOPE

// Define local tokens for the names of the primvars the instancer
// consumes.
// XXX: These should be hydra tokens...
TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (instanceTransform)
    (rotate)
    (scale)
    (translate)
);

HdEmbreeInstancer::HdEmbreeInstancer(HdSceneDelegate* delegate,
                                     SdfPath const& id,
                                     SdfPath const &parentId)
    : HdInstancer(delegate, id, parentId)
{
}

void
HdEmbreeInstancer::_SyncPrimvars()
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdChangeTracker &changeTracker = 
        _GetDelegate()->GetRenderIndex().GetChangeTracker();
    SdfPath const& id = GetId();

    // Use the double-checked locking pattern to check if this instancer's
    // primvars are dirty.
    int dirtyBits = changeTracker.GetInstancerDirtyBits(id);
    if (HdChangeTracker::IsAnyPrimVarDirty(dirtyBits, id)) {
        std::lock_guard<std::mutex> lock(_instanceLock);

        dirtyBits = changeTracker.GetInstancerDirtyBits(id);
        if (HdChangeTracker::IsAnyPrimVarDirty(dirtyBits, id)) {

            // If this instancer has dirty primvars, get the list of
            // primvar names and then cache each one.

            TfTokenVector primVarNames;
            primVarNames = _GetDelegate()->GetPrimVarInstanceNames(id);

            TF_FOR_ALL(nameIt, primVarNames) {
                if (HdChangeTracker::IsPrimVarDirty(dirtyBits, id, *nameIt)) {
                    VtValue value = _GetDelegate()->Get(id, *nameIt);
                    if (!value.IsEmpty()) {
                        _primvarMap[*nameIt] = value;
                    }
                }
            }

            // Mark the instancer as clean
            changeTracker.MarkInstancerClean(id);
        }
    }
}

VtMatrix4dArray
HdEmbreeInstancer::ComputeInstanceTransforms(SdfPath const &prototypeId)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    _SyncPrimvars();

    // The transforms for this level of instancer are computed by:
    // foreach(index : indices) {
    //     instancerTransform * translate(index) * rotate(index) *
    //     scale(index) * instanceTransform(index)
    // }
    // If any transform isn't provided, it's assumed to be the identity.

    GfMatrix4d instancerTransform =
        _GetDelegate()->GetInstancerTransform(GetId(), prototypeId);
    VtIntArray instanceIndices =
        _GetDelegate()->GetInstanceIndices(GetId(), prototypeId);

    VtMatrix4dArray transforms(instanceIndices.size());
    for (size_t i = 0; i < instanceIndices.size(); ++i) {
        transforms[i] = instancerTransform;
    }

    // XXX: We should handle VtValues more robustly.

    // "translate" is by convention a VtVec3fArray, holding a translation
    // vector for each index.
    if (_primvarMap.count(_tokens->translate) > 0 &&
        _primvarMap[_tokens->translate].IsHolding<VtVec3fArray>()) {
        VtVec3fArray translate = _primvarMap[_tokens->translate]
            .UncheckedGet<VtVec3fArray>();
        for (size_t i = 0; i < instanceIndices.size(); ++i) {
            GfMatrix4d translateMat(1);
            translateMat.SetTranslate(GfVec3d(translate[instanceIndices[i]]));
            transforms[i] = translateMat * transforms[i];
        }
    }

    // "rotate" is by convention a VtVec4fArray, holding a quaternion in
    // <real, i, j, k> format for each index.
    if (_primvarMap.count(_tokens->rotate) > 0 &&
        _primvarMap[_tokens->rotate].IsHolding<VtVec4fArray>()) {
        VtVec4fArray rotate = _primvarMap[_tokens->rotate]
            .UncheckedGet<VtVec4fArray>();
        for (size_t i = 0; i < instanceIndices.size(); ++i) {
            GfMatrix4d rotateMat(1);
            GfVec4f quat = rotate[instanceIndices[i]];
            rotateMat.SetRotate(GfRotation(GfQuaternion(
                quat[0], GfVec3d(quat[1], quat[2], quat[3]))));
            transforms[i] = rotateMat * transforms[i];
        }
    }

    // "scale" is by convention a VtVec3fArray, holding an axis-aligned
    // scale vector for each index.
    if (_primvarMap.count(_tokens->scale) > 0 &&
        _primvarMap[_tokens->scale].IsHolding<VtVec3fArray>()) {
        VtVec3fArray scale = _primvarMap[_tokens->scale]
            .UncheckedGet<VtVec3fArray>();
        for (size_t i = 0; i < instanceIndices.size(); ++i) {
            GfMatrix4d scaleMat(1);
            scaleMat.SetScale(GfVec3d(scale[i]));
            transforms[i] = scaleMat * transforms[i];
        }
    }

    // "instanceTransform" is by convention a VtMatrix4dArray, holding a
    // transformation matrix for each index.
    if (_primvarMap.count(_tokens->instanceTransform) > 0 &&
        _primvarMap[_tokens->instanceTransform].IsHolding<VtMatrix4dArray>()) {
        VtMatrix4dArray instanceTransform =
            _primvarMap[_tokens->instanceTransform]
            .UncheckedGet<VtMatrix4dArray>();
        for (size_t i = 0; i < instanceIndices.size(); ++i) {
            transforms[i] = instanceTransform[instanceIndices[i]] *
                            transforms[i];
        }
    }

    if (GetParentId().IsEmpty()) {
        return transforms;
    }

    HdInstancer *parentInstancer =
        _GetDelegate()->GetRenderIndex().GetInstancer(GetParentId());
    if (!TF_VERIFY(parentInstancer)) {
        return transforms;
    }

    // The transforms taking nesting into account are computed by:
    // parentTransforms = parentInstancer->ComputeInstanceTransforms(GetId())
    // foreach (parentXf : parentTransforms, xf : transforms) {
    //     parentXf * xf
    // }
    VtMatrix4dArray parentTransforms =
        static_cast<HdEmbreeInstancer*>(parentInstancer)->
            ComputeInstanceTransforms(GetId());

    VtMatrix4dArray final(parentTransforms.size() * transforms.size());
    for (size_t i = 0; i < parentTransforms.size(); ++i) {
        for (size_t j = 0; j < transforms.size(); ++j) {
            final[i * transforms.size() + j] = transforms[j] *
                                               parentTransforms[i];
        }
    }
    return final;
}

PXR_NAMESPACE_CLOSE_SCOPE

