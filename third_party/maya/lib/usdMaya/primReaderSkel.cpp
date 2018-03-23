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

#include <maya/MObject.h>


PXR_NAMESPACE_OPEN_SCOPE


class PxrUsdMayaPrimReaderSkelRoot : public PxrUsdMayaPrimReader
{
public:
    PxrUsdMayaPrimReaderSkelRoot(const PxrUsdMayaPrimReaderArgs& args,
                                 PxrUsdMayaPrimReaderContext* ctx)
        : PxrUsdMayaPrimReader(args, ctx) {}

    virtual ~PxrUsdMayaPrimReaderSkelRoot() {}

    virtual bool Read() override;

    virtual bool HasPostReadSubtree() const override { return true; }

    virtual void PostReadSubtree() override;

protected:
    bool _ReadSkel(const UsdSkelSkeletonQuery& skelQuery,
                   MObject& bindingSiteNode);

private:
    // TODO: Ideally, we'd share the cache across different models
    // if importing multiple skel roots.
    UsdSkelCache _cache;
};



TF_REGISTRY_FUNCTION_WITH_TAG(PxrUsdMayaPrimReaderRegistry, UsdSkelRoot) {
    PxrUsdMayaPrimReaderRegistry::Register<UsdSkelRoot>(
        [](const PxrUsdMayaPrimReaderArgs& args,
           PxrUsdMayaPrimReaderContext* ctx)
        {
            return PxrUsdMayaPrimReaderPtr(
                new PxrUsdMayaPrimReaderSkelRoot(args, ctx));
        });
}


bool
PxrUsdMayaPrimReaderSkelRoot::Read()
{
    const UsdPrim& prim = _GetArgs().GetUsdPrim();
    if(!TF_VERIFY(prim))
        return false;

    // First pass through:
    // The skel root itself is a transform, so produce a transform.
    // The rest of the skel is produced as a post sub-tree process.
    MObject parentNode =
        _GetContext()->GetMayaNode(prim.GetPath().GetParentPath(),
                                   /*findAncestors*/ true);

    MStatus status;
    MObject obj;
    return PxrUsdMayaTranslatorUtil::CreateTransformNode(prim, parentNode,
                                                         _GetArgs(),
                                                         _GetContext(),
                                                         &status, &obj);
}


void
PxrUsdMayaPrimReaderSkelRoot::PostReadSubtree()
{
    UsdSkelRoot skelRoot(_GetArgs().GetUsdPrim());
    if(!TF_VERIFY(skelRoot))
        return;

    _cache.Populate(skelRoot);

    for(const UsdPrim& prim : UsdPrimRange(skelRoot.GetPrim())) {
        if(UsdSkelSkeletonQuery skelQuery = _cache.GetSkelQuery(prim)) {
            MObject bindingSiteNode =
                _GetContext()->GetMayaNode(prim.GetPath(),
                                           /*findAncestors*/ true);
            if(!bindingSiteNode.isNull()) {
                _ReadSkel(skelQuery, bindingSiteNode);
            }
        }
    }
}


bool
PxrUsdMayaPrimReaderSkelRoot::_ReadSkel(const UsdSkelSkeletonQuery& skelQuery,
                                        MObject& bindingSiteNode)
{
    // Build out a joint hierarchy.
    std::vector<MObject> joints;
    if(!PxrUsdMayaTranslatorSkel::CreateJoints(
           skelQuery, bindingSiteNode,
           _GetArgs(), _GetContext(), &joints)) {
        return false;
    }

    // Add a bind pose. This is not necessary for skinning to function in Maya,
    // but may a requirement of some exporters. The dagPose command also
    // functions based on the definition of the bind pose.
    MObject bindPose;
    if(!PxrUsdMayaTranslatorSkel::CreateBindPose(
           skelQuery, joints, _GetContext(), &bindPose)) {
        return false;
    }

    bool success = true;

    // Find all meshes skinned by this skel instance.
    std::vector<std::pair<UsdPrim,UsdSkelSkinningQuery> > pairs;
    if(_cache.ComputeSkinnedPrims(skelQuery.GetPrim(), &pairs)) {
        for(const auto& pair : pairs) {
            // TODO: Support custom joint orderings on skinned prims.
            // Can do this by remapping the joints array to so that we have
            // a joint MObject array ordered to match the prim being skinned.
            // Need to make UsdSkelAnimMapper compatible with extra array
            // types first, however...

            // Add a skin cluster to skin this prim.
            success &= PxrUsdMayaTranslatorSkel::CreateSkinCluster(
                skelQuery, pair.second, joints, pair.first,
                _GetArgs(), _GetContext(), bindPose);
        }
    }
    return success;
}


PXR_NAMESPACE_CLOSE_SCOPE
