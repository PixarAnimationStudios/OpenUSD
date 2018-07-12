//
// Copyright 2018 Pixar
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
#include "pxr/pxr.h"
#include "usdMaya/primReaderRegistry.h"
#include "usdMaya/translatorSkel.h"
#include "usdMaya/translatorUtil.h"

#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdSkel/cache.h"
#include "pxr/usd/usdSkel/root.h"
#include "pxr/usd/usdSkel/skeletonQuery.h"
#include "pxr/usd/usdSkel/skinningQuery.h"

#include <boost/unordered_map.hpp>

#include <maya/MObject.h>


PXR_NAMESPACE_OPEN_SCOPE


/// Prim reader for skeletons.
/// This produce a joint hierarchy, possibly animated, corresponding
/// to a UsdSkelSkeleton.
class PxrUsdMayaPrimReaderSkeleton : public PxrUsdMayaPrimReader
{
public:
    PxrUsdMayaPrimReaderSkeleton(const PxrUsdMayaPrimReaderArgs& args)
        : PxrUsdMayaPrimReader(args) {}

    ~PxrUsdMayaPrimReaderSkeleton() override {}

    bool Read(PxrUsdMayaPrimReaderContext* context) override;

private:
    // TODO: Ideally, we'd share the cache across different models
    // if importing multiple skel roots.
    UsdSkelCache _cache;
};


TF_REGISTRY_FUNCTION_WITH_TAG(PxrUsdMayaPrimReaderRegistry, UsdSkelSkeleton) {
    PxrUsdMayaPrimReaderRegistry::Register<UsdSkelSkeleton>(
        [](const PxrUsdMayaPrimReaderArgs& args)
        {
            return PxrUsdMayaPrimReaderSharedPtr(
                new PxrUsdMayaPrimReaderSkeleton(args));
        });
}


/// Prim reader for a UsdSkelRoot.
/// This post-processes the skinnable prims beneath a UsdSkelRoot
/// to define skin clusters, etc. for bound skeletons.
class PxrUsdMayaPrimReaderSkelRoot : public PxrUsdMayaPrimReader
{
public:
    PxrUsdMayaPrimReaderSkelRoot(const PxrUsdMayaPrimReaderArgs& args)
        : PxrUsdMayaPrimReader(args) {}

    ~PxrUsdMayaPrimReaderSkelRoot() override {}

    bool Read(PxrUsdMayaPrimReaderContext* context) override;

    bool HasPostReadSubtree() const override { return true; }

    void PostReadSubtree(PxrUsdMayaPrimReaderContext* context) override;

private:
    // TODO: Ideally, we'd share the cache across different models
    // if importing multiple skel roots.
    UsdSkelCache _cache;
};



TF_REGISTRY_FUNCTION_WITH_TAG(PxrUsdMayaPrimReaderRegistry, UsdSkelRoot) {
    PxrUsdMayaPrimReaderRegistry::Register<UsdSkelRoot>(
        [](const PxrUsdMayaPrimReaderArgs& args)
        {
            return PxrUsdMayaPrimReaderSharedPtr(
                new PxrUsdMayaPrimReaderSkelRoot(args));
        });
}


bool
PxrUsdMayaPrimReaderSkeleton::Read(PxrUsdMayaPrimReaderContext* context)
{
    UsdSkelSkeleton skel(_GetArgs().GetUsdPrim());
    if (!TF_VERIFY(skel))
        return false;

    if (UsdSkelSkeletonQuery skelQuery = _cache.GetSkelQuery(skel)) {

        MObject parentNode = context->GetMayaNode(
            skel.GetPrim().GetPath().GetParentPath(), true);

        // Build out a joint hierarchy.
        VtArray<MObject> joints;
        if (PxrUsdMayaTranslatorSkel::CreateJointHierarchy(
                skelQuery, parentNode, _GetArgs(), context, &joints)) {
            
            // Add a bind pose. This is not necessary for skinning to function
            // in Maya, but may be a requirement of some exporters. The dagPose
            // command also functions based on the definition of the bind pose.
            MObject bindPose;
            if (PxrUsdMayaTranslatorSkel::CreateBindPose(
                    skelQuery, joints, context, &bindPose)) {
                return true;
            }
        }
    }
    return false;
}


bool
PxrUsdMayaPrimReaderSkelRoot::Read(PxrUsdMayaPrimReaderContext* context)
{
    UsdSkelRoot skelRoot(_GetArgs().GetUsdPrim());
    if (!TF_VERIFY(skelRoot))
        return false;

    // First pass through:
    // The skel root itself is a transform, so produce a transform.
    // Skeletal bindings will be handled as a post sub-tree process.
    MObject parentNode =
        context->GetMayaNode(skelRoot.GetPrim().GetPath().GetParentPath(),
                             /*findAncestors*/ true);

    MStatus status;
    MObject obj;
    return PxrUsdMayaTranslatorUtil::CreateTransformNode(
        skelRoot.GetPrim(), parentNode, _GetArgs(), context, &status, &obj);
}


void
PxrUsdMayaPrimReaderSkelRoot::PostReadSubtree(
    PxrUsdMayaPrimReaderContext* context)
{
    UsdSkelRoot skelRoot(_GetArgs().GetUsdPrim());
    if (!TF_VERIFY(skelRoot))
        return;
    
    // Compute skel bindings and create skin clusters for bound skels
    // We do this in a post-subtree stage to ensure that any skinnable
    // prims we produce skin clusters for have been processed first.

    _cache.Populate(skelRoot);

    std::vector<UsdSkelBinding> bindings;
    if (!_cache.ComputeSkelBindings(skelRoot, &bindings)) {
        return;
    }

    for (const UsdSkelBinding& binding : bindings) {
        if (binding.GetSkinningTargets().empty())
            continue;

        if (const UsdSkelSkeletonQuery& skelQuery =
            _cache.GetSkelQuery(binding.GetSkeleton())) {
            
            VtArray<MObject> joints;
            if (!PxrUsdMayaTranslatorSkel::GetJoints(
                    skelQuery, context, &joints)) {
                continue;
            }
            
            for (const auto& skinningQuery : binding.GetSkinningTargets()) {

                const UsdPrim& skinnedPrim = skinningQuery.GetPrim();

                // Get an ordering of the joints that matches the ordering of
                // the binding.
                VtArray<MObject> skinningJoints;
                if (!skinningQuery.GetMapper()) {
                    skinningJoints = joints;
                } else {
                    // TODO:
                    // UsdSkelAnimMapper currently only supports remapping
                    // of Sdf value types, so we can't easily use it here.
                    // For now, we can delegate remapping behavior by
                    // remapping ordered joint indices.
                    VtIntArray indices(joints.size());
                    for (size_t i = 0; i < joints.size(); ++i)
                        indices[i] = i;
                    
                    VtIntArray remappedIndices;
                    if (!skinningQuery.GetMapper()->Remap(
                            indices, &remappedIndices)) {
                        continue;
                    }
                    
                    skinningJoints.resize(remappedIndices.size());
                    for (size_t i = 0; i < remappedIndices.size(); ++i) {
                        int index = remappedIndices[i];
                        if (index >= 0 && static_cast<size_t>(index) < joints.size()) {
                            skinningJoints[i] = joints[index];
                        }
                    }
                }

                MObject bindPose =
                    PxrUsdMayaTranslatorSkel::GetBindPose(skelQuery, context);

                // Add a skin cluster to skin this prim.
                PxrUsdMayaTranslatorSkel::CreateSkinCluster(
                    skelQuery, skinningQuery, skinningJoints,
                    skinnedPrim, _GetArgs(), context, bindPose);
            }
        }
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
