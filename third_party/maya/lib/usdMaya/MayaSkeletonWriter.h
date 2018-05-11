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

PXR_NAMESPACE_OPEN_SCOPE


/// Exports joint hierarchies (the hierarchies of DAG nodes rooted at a joint)
/// as a UsdSkelSkeleton, along with a UsdSkelPackedJointAnimation if the
/// joints are animated or posed differently from their rest pose. Currently,
/// each joint hierarchy is treated as a separate skeleton, meaning that this
/// prim writer will never produce skeletons with multiple root joints.
///
/// If the joints are posed differently from the rest pose on the export frame
/// (the current frame when the export command is run), a
/// UsdSkelPackedJointAnimation is created to encode the pose.
/// If the exportAnimation flag is enabled for the write job and the joints do
/// contain animation, then a UsdSkelPackedJointAnimation is created to encode
/// the joint animations.
///
/// UsdSkelSkeleton is not Xformable in the UsdSkel schema. Rather, the joint
/// transforms are encoded in attributes inside the Skeleton and the
/// PackedJointAnimation. Thus, a joint hierarchy in Maya will export as
/// a single UsdSkelSkeleton at its root joint, along with a child
/// PackedJointAnimation named "Animation" if necessary.
class MayaSkeletonWriter : public MayaPrimWriter
{
public:
    MayaSkeletonWriter(const MDagPath & iDag,
            const SdfPath& uPath,
            usdWriteJobCtx& jobCtx);
    
    void write(const UsdTimeCode &usdTime) override;
    bool exportsGprims() const override;
    bool shouldPruneChildren() const override;
    bool isShapeAnimated() const override;
    bool getAllAuthoredUsdPaths(SdfPathVector* outPaths) const override;

    /// Gets the joint name tokens for the given dag paths, assuming a joint
    /// hierarchy with the given root joint.
    static VtTokenArray GetJointNames(
            const std::vector<MDagPath>& joints,
            const MDagPath& rootJoint);
    /// Gets the expected path where a UsdSkelSkeleton prim will be exported
    /// for the given root joint.
    static SdfPath GetSkeletonPath(const MDagPath& rootJoint);
    /// Gets the expected path where a UsdSkelPackedJointAnimation prim will be
    /// exported for the given root joint.
    static SdfPath GetAnimationPath(const MDagPath& rootJoint);

private:
    std::vector<MDagPath> _animatedJoints;
};

typedef std::shared_ptr<MayaSkeletonWriter> MayaSkeletonWriterPtr;


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // _usdExport_MayaSkeletonWriter_h_
