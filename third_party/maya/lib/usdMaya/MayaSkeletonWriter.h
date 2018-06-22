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
#ifndef _usdExport_MayaSkeletonWriter_h_
#define _usdExport_MayaSkeletonWriter_h_

#include "pxr/pxr.h"
#include "usdMaya/MayaPrimWriter.h"

#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdSkel/animation.h"
#include "pxr/usd/usdSkel/animMapper.h"
#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdSkel/topology.h"


PXR_NAMESPACE_OPEN_SCOPE


/// Exports joint hierarchies (the hierarchies of DAG nodes rooted at a joint)
/// as a UsdSkelSkeleton, along with a UsdSkelAnimation if the joints are
/// animated or posed differently from their rest pose. Currently, each joint
/// hierarchy is treated as a separate skeleton, meaning that this prim writer
/// will never produce skeletons with multiple root joints.
///
/// If the joints are posed differently from the rest pose on the export frame
/// (the current frame when the export command is run), a UsdSkelAnimation is
/// created to encode the pose.
/// If the exportAnimation flag is enabled for the write job and the joints do
/// contain animation, then a UsdSkelAnimation is created to encode the joint
/// animations.
class MayaSkeletonWriter : public MayaPrimWriter
{
public:
    MayaSkeletonWriter(const MDagPath& iDag,
                       const SdfPath& uPath,
                       usdWriteJobCtx& jobCtx);
    
    void write(const UsdTimeCode &usdTime) override;
    bool exportsGprims() const override;
    bool shouldPruneChildren() const override;
    bool isShapeAnimated() const override;
    bool getAllAuthoredUsdPaths(SdfPathVector* outPaths) const override;

    /// Gets the joint name tokens for the given dag paths, assuming a joint
    /// hierarchy with the given root joint.
    static VtTokenArray GetJointNames(const std::vector<MDagPath>& joints,
                                      const MDagPath& rootJoint,
                                      bool stripNamespaces);

    /// Gets the expected path where a skeleton will be exported for
    /// the given root joint. The skeleton both binds a skeleton and
    /// holds root transformations of the joint hierarchy.
    static SdfPath GetSkeletonPath(const MDagPath& rootJoint,
                                   bool stripNamespaces);

private:
    bool _WriteRestState();

    bool _valid;
    UsdSkelSkeleton _skel;
    UsdSkelAnimation _skelAnim;

    /// The dag path defining the root transform of the Skeleton.
    MDagPath _skelXformPath;

    /// The dag path providing the comopnent of root transformation
    /// that comes from an animatio source.
    MDagPath _animXformPath;

    /// The common parent path of all proper joints.
    MDagPath _jointHierarchyRootPath;

    UsdSkelTopology _topology;
    UsdSkelAnimMapper _skelToAnimMapper;
    std::vector<MDagPath> _joints, _animatedJoints;
    UsdAttribute _skelXformAttr, _animXformAttr;
    bool _skelXformIsAnimated, _animXformIsAnimated;
};

typedef std::shared_ptr<MayaSkeletonWriter> MayaSkeletonWriterPtr;


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // _usdExport_MayaSkeletonWriter_h_
