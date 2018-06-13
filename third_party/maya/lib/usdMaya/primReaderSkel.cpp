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


class PxrUsdMayaPrimReaderSkelRoot : public PxrUsdMayaPrimReader
{
public:
    PxrUsdMayaPrimReaderSkelRoot(const PxrUsdMayaPrimReaderArgs& args)
        : PxrUsdMayaPrimReader(args) {}

    virtual ~PxrUsdMayaPrimReaderSkelRoot() {}

    virtual bool Read(PxrUsdMayaPrimReaderContext* context) override;

    virtual bool HasPostReadSubtree() const override { return true; }

    virtual void PostReadSubtree(PxrUsdMayaPrimReaderContext* context) override;

protected:
    struct _SkelInstanceData {
        MObject rootXformNode;
        MObject bindPose;
        VtArray<MObject> joints;
    };

    bool _ReadSkel(const UsdSkelSkeletonQuery& skelQuery,
                   MObject& xformNode,
                   PxrUsdMayaPrimReaderContext* context,
                   _SkelInstanceData* skelInstanceData);

private:
    // TODO: Ideally, we'd share the cache across different models
    // if importing multiple skel roots.
    UsdSkelCache _cache;
};



TF_REGISTRY_FUNCTION_WITH_TAG(PxrUsdMayaPrimReaderRegistry, UsdSkelRoot) {
    PxrUsdMayaPrimReaderRegistry::Register<UsdSkelRoot>(
        [](const PxrUsdMayaPrimReaderArgs& args)
        {
            return PxrUsdMayaPrimReaderPtr(
                new PxrUsdMayaPrimReaderSkelRoot(args));
        });
}


bool
PxrUsdMayaPrimReaderSkelRoot::Read(PxrUsdMayaPrimReaderContext* context)
{
    const UsdPrim& prim = _GetArgs().GetUsdPrim();
    if(!TF_VERIFY(prim))
        return false;

    // First pass through:
    // The skel root itself is a transform, so produce a transform.
    // The rest of the skel is produced as a post sub-tree process.
    MObject parentNode =
        context->GetMayaNode(prim.GetPath().GetParentPath(),
                            /*findAncestors*/ true);

    MStatus status;
    MObject obj;
    return PxrUsdMayaTranslatorUtil::CreateTransformNode(prim, parentNode,
                                                         _GetArgs(),
                                                         context,
                                                         &status, &obj);
}


void
PxrUsdMayaPrimReaderSkelRoot::PostReadSubtree(
    PxrUsdMayaPrimReaderContext* context)
{
    UsdSkelRoot skelRoot(_GetArgs().GetUsdPrim());
    if (!TF_VERIFY(skelRoot))
        return;

    _cache.Populate(skelRoot);

    // Map of queries for each unique skel instance, mapped to the
    // MObject holding that skel instance.
    boost::unordered_map<
        UsdSkelSkeletonQuery,
        std::shared_ptr<_SkelInstanceData> > skelInstanceObjectMap;

    // We import in two stages:

    // First traversal: Find the full set of unique skel instances.
    // TODO: UsdSkel should have a more direct way of providing this.
    for (const UsdPrim& prim : UsdPrimRange(skelRoot.GetPrim())) {
        if (UsdSkelSkeletonQuery skelQuery = _cache.GetSkelQuery(prim)) {
            auto it = skelInstanceObjectMap.find(skelQuery);
            if (it != skelInstanceObjectMap.end()) {
                continue;
            }
            
            // We need to define a joint hierarchy to represent this skel
            // instance. Since we want that hierarchy to inherit the transform
            // of whatever primitive is transforming the skel -- which is not
            // necessarily the prim we queried the cache with above! -- we wish
            // to define the hierarchy as a child of that transform node.
            // Note that if the skel:xform rel is being used, it is invalid
            // for the xform prim to be outside of the SkelRoot, so we should
            // expect import to have already produced the transform node.
            MObject instanceNode =
                context->GetMayaNode(skelQuery.GetPrim().GetPath(),
                                     /*findAncestors*/ true);
            if (!instanceNode.isNull()) {
                auto skelInstanceData = std::make_shared<_SkelInstanceData>();
                _ReadSkel(skelQuery, instanceNode, context,
                          skelInstanceData.get());
                
                skelInstanceObjectMap[skelQuery] = skelInstanceData;
            }
        }
    }

    // Second traversal: Configure deformers on skinnable prims.
    std::vector<std::pair<UsdPrim,UsdSkelSkinningQuery> > pairs;    
    for (const UsdPrim& prim : UsdPrimRange(skelRoot.GetPrim())) {

        if (UsdSkelSkeletonQuery skelQuery = _cache.GetSkelQuery(prim)) {

            auto it = skelInstanceObjectMap.find(skelQuery);
            if (it != skelInstanceObjectMap.end()) {
                const auto& skelInstanceData = it->second;
                
                if (skelInstanceData &&
                    _cache.ComputeSkinnedPrims(prim, &pairs)) {

                    for (const auto& pair : pairs) {

                        VtArray<MObject> joints(skelInstanceData->joints);
                        if (pair.second.GetMapper()) {
                            // TODO:
                            // UsdSkelAnimMapper needs to provide a Remap method
                            // that can handle arbitrary container/value types.
                            // For now, we can compute an appropriate remapping
                            // using the result of remapping ordered indices.
                            VtIntArray indices(joints.size());
                            for (size_t i = 0; i < indices.size(); ++i) {
                                indices[i] = static_cast<int>(i);
                            }
                            
                            VtIntArray remappedIndices;
                            if (!pair.second.GetMapper()->Remap(
                                    indices, &remappedIndices)) {
                                continue;
                            }
                            joints.resize(remappedIndices.size());
                            for (size_t i = 0; i < remappedIndices.size(); ++i) {
                                joints[i] =
                                    skelInstanceData->joints[remappedIndices[i]];
                            }
                        }

                        // Add a skin cluster to skin this prim.
                        PxrUsdMayaTranslatorSkel::CreateSkinCluster(
                            skelQuery, pair.second, joints, pair.first,
                            _GetArgs(), context, skelInstanceData->bindPose);
                    }
                }
            }
        }
    }
}


bool
PxrUsdMayaPrimReaderSkelRoot::_ReadSkel(const UsdSkelSkeletonQuery& skelQuery,
                                        MObject& xformNode,
                                        PxrUsdMayaPrimReaderContext* context,
                                        _SkelInstanceData* skelInstanceData)
{
    skelInstanceData->rootXformNode = xformNode;

    // Build out a joint hierarchy.
    if (!PxrUsdMayaTranslatorSkel::CreateJoints(
            skelQuery, xformNode, _GetArgs(), context,
            &skelInstanceData->joints)) {
        return false;
    }

    // Add a bind pose. This is not necessary for skinning to function in Maya,
    // but may be a requirement of some exporters. The dagPose command also
    // functions based on the definition of the bind pose.
    if (!PxrUsdMayaTranslatorSkel::CreateBindPose(
            skelQuery, skelInstanceData->joints, context,
            &skelInstanceData->bindPose)) {
        return false;
    }

    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE
