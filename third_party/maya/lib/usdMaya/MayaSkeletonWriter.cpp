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
#include "usdMaya/MayaSkeletonWriter.h"

#include "usdMaya/adaptor.h"
#include "usdMaya/util.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/usdSkel/packedJointAnimation.h"
#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdSkel/utils.h"

#include <maya/MAnimUtil.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnTransform.h>
#include <maya/MItDag.h>
#include <maya/MMatrix.h>
#include <maya/MQuaternion.h>

PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(MFn::kJoint, UsdSkelSkeleton);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens, 
    (Animation)
);

// Returns a nicer name for a UsdSkelSkeleton than just the name of the root
// joint. Note that this is only OK because MayaSkeletonWriter declares
// shouldPruneChildren=true; thus no other prim writers should depend on
// the hierarchy under this prim.
static SdfPath
_GetNiceSkeletonPath(const MDagPath& dagPath, const SdfPath& usdPath)
{
    std::string newName =
            TfStringPrintf("skeleton_%s", usdPath.GetName().c_str());

    // Check to see if there are any siblings named newName. If so, then use
    // the existing path just to be safe.
    MDagPath parentPath(dagPath);
    parentPath.pop();
    for (unsigned int i = 0; i < parentPath.childCount(); ++i) {
        MFnDagNode child(parentPath.child(i));
        if (child.name() == newName.c_str() && !(child.dagPath() == dagPath)) {
            return usdPath;
        }
    }

    // All clear, use our prettified name!
    return usdPath.GetParentPath().AppendChild(TfToken(newName));
}

MayaSkeletonWriter::MayaSkeletonWriter(const MDagPath & iDag,
    const SdfPath& uPath,
    usdWriteJobCtx& jobCtx)
    : MayaPrimWriter(iDag, _GetNiceSkeletonPath(iDag, uPath), jobCtx)
{
    UsdSkelSkeleton primSchema =
            UsdSkelSkeleton::Define(getUsdStage(), getUsdPath());
    TF_AXIOM(primSchema);
    mUsdPrim = primSchema.GetPrim();
    TF_AXIOM(mUsdPrim);
}

VtTokenArray
MayaSkeletonWriter::GetJointNames(
    const std::vector<MDagPath>& joints,
    const MDagPath& rootDagPath)
{
    // Get relative paths.
    // Joints have to be transforms, so mergeTransformAndShape
    // shouldn't matter here. (Besides, we're not actually using these
    // to point to prims.)
    SdfPath rootPath = PxrUsdMayaUtil::MDagPathToUsdPath(
            rootDagPath, /*mergeTransformAndShape*/ false).GetParentPath();
    VtTokenArray result;
    for (const MDagPath& joint : joints) {
        SdfPath path = PxrUsdMayaUtil::MDagPathToUsdPath(
                joint, /*mergeTransformAndShape*/ false);
        result.push_back(path.MakeRelativePath(rootPath).GetToken());
    }
    return result;
}

SdfPath
MayaSkeletonWriter::GetSkeletonPath(const MDagPath& rootJoint)
{
    // Joints are always transforms!
    return _GetNiceSkeletonPath(
            rootJoint,
            PxrUsdMayaUtil::MDagPathToUsdPath(
                rootJoint, /*mergeTransformAndShape*/ false));
}

SdfPath
MayaSkeletonWriter::GetAnimationPath(const MDagPath& rootJoint)
{
    return GetSkeletonPath(rootJoint).AppendChild(_tokens->Animation);
}

/// Gets all of the joints rooted at the given dag path, including the dag path.
/// The vector returned ensures that ancestor joints come before descendant
/// joints.
static std::vector<MDagPath>
_GetJoints(const MDagPath& dagPath)
{
    MItDag dagIter(MItDag::kDepthFirst, MFn::kJoint);
    dagIter.reset(dagPath, MItDag::kDepthFirst, MFn::kJoint);
    std::vector<MDagPath> result;
    while (!dagIter.isDone()) {
        MDagPath path;
        dagIter.getPath(path);
        result.push_back(path);
        dagIter.next();
    }
    return result;
}

/// Whether the transform plugs on a transform node are animated.
static bool
_IsTransformNodeAnimated(const MDagPath& dagPath)
{
    MFnDependencyNode node(dagPath.node());
    return PxrUsdMayaUtil::isPlugAnimated(node.findPlug("translateX")) ||
            PxrUsdMayaUtil::isPlugAnimated(node.findPlug("translateY")) ||
            PxrUsdMayaUtil::isPlugAnimated(node.findPlug("translateZ")) ||
            PxrUsdMayaUtil::isPlugAnimated(node.findPlug("rotateX")) ||
            PxrUsdMayaUtil::isPlugAnimated(node.findPlug("rotateY")) ||
            PxrUsdMayaUtil::isPlugAnimated(node.findPlug("rotateZ")) ||
            PxrUsdMayaUtil::isPlugAnimated(node.findPlug("scaleX")) ||
            PxrUsdMayaUtil::isPlugAnimated(node.findPlug("scaleY")) ||
            PxrUsdMayaUtil::isPlugAnimated(node.findPlug("scaleZ"));
}

/// Gets the local-space rest transform for a single dag path.
/// If a joint's parent is not a joint, then the local-space rest transform
/// (bind pose) is computed with respect to the parent's world transformation
/// matrix.
static MMatrix
_GetRestTransform(const MDagPath& dagPath)
{
    MFnDagNode dagNode(dagPath);
    MMatrix restTransformWorld;
    if (!PxrUsdMayaUtil::getPlugMatrix(
                dagNode, "bindPose", &restTransformWorld)) {
        // No bindPose. Assume it's identity.
        return MMatrix();
    }

    MDagPath parentDagPath(dagPath);
    parentDagPath.pop();
    MFnDagNode parentDagNode(parentDagPath);
    MMatrix parentRestTransformWorld;
    if (!PxrUsdMayaUtil::getPlugMatrix(
                parentDagNode, "bindPose", &parentRestTransformWorld)) {
        // In this exporter, the parent of the root joint defines the
        // skeleton space. The parent might not be a joint itself, so
        // we just read the parent (exclusive) matrix from the _original_
        // dag path.
        parentRestTransformWorld = dagPath.exclusiveMatrix();
    }

    // All of the bindPose's are world-space matrices, so we have to
    // make them into joint-local space.
    MMatrix restTransformLocal =
            restTransformWorld * parentRestTransformWorld.inverse();
    return restTransformLocal;
}

/// Gets rest transforms for all the specified dag paths.
static VtMatrix4dArray
_GetRestTransforms(
    const std::vector<MDagPath>& jointDagPaths)
{
    size_t numJoints = jointDagPaths.size();
    VtMatrix4dArray result(numJoints);

    for (size_t jointIndex = 0; jointIndex < numJoints; ++jointIndex) {
        MDagPath jointDagPath = jointDagPaths[jointIndex];
        result[jointIndex] = GfMatrix4d(_GetRestTransform(jointDagPath).matrix);
    }

    return result;
}

/// Determines whether the joint at the given dag path is currently in the
/// rest pose.
/// A node is in its rest pose if the localized bindPose matches the local
/// transformation matrix.
static bool
_IsTransformNodeInRestPose(const MDagPath& dagPath)
{
    MStatus status;
    MFnTransform xf(dagPath, &status);
    CHECK_MSTATUS_AND_RETURN(status, true);

    MTransformationMatrix transformLocal = xf.transformation(&status);
    CHECK_MSTATUS_AND_RETURN(status, true);

    MMatrix restTransformLocal = _GetRestTransform(dagPath);
    return transformLocal.asMatrix().isEquivalent(restTransformLocal);
}

/// Given the list of USD joint names and dag paths, returns the joints that 
/// (1) are moved from their rest poses or (2) have animation, if we are going
/// to export animation.
static void
_GetAnimatedJoints(
    const VtTokenArray& usdJointNames,
    const std::vector<MDagPath>& jointDagPaths,
    VtTokenArray* animatedJointNames,
    std::vector<MDagPath>* animatedJointPaths,
    bool exportingAnimation)
{
    TF_AXIOM(usdJointNames.size() == jointDagPaths.size());

    // The resulting vector contains only animated joints or joints not
    // in their rest pose. The order is *not* guaranteed to be the Skeleton
    // order, because UsdSkel allows arbitrary order on PackedJointAnimations.
    for (size_t i = 0; i < jointDagPaths.size(); ++i) {
        const TfToken& jointName = usdJointNames[i];
        const MDagPath& dagPath = jointDagPaths[i];

        if ((exportingAnimation && _IsTransformNodeAnimated(dagPath)) ||
                !_IsTransformNodeInRestPose(dagPath)) {
            animatedJointNames->push_back(jointName);
            animatedJointPaths->push_back(dagPath);
        }
    }
}

/// Gets the T/R/S transformation components for the given dag paths at the
/// current time.
static void
_GetAnimationData(
    const std::vector<MDagPath>& dagPaths,
    VtVec3fArray* outTrans,
    VtQuatfArray* outRot,
    VtVec3hArray* outScale)
{
    outTrans->reserve(dagPaths.size());
    outRot->reserve(dagPaths.size());
    outScale->reserve(dagPaths.size());

    for (size_t i = 0; i < dagPaths.size(); ++i) {
        const MDagPath& dagPath = dagPaths[i];
        MFnDependencyNode influenceDepNode(dagPath.node());
        MPlug matrixPlug = influenceDepNode.findPlug("matrix");
        if (matrixPlug.isNull()) {
            outTrans->push_back(GfVec3f(0.0));
            outRot->push_back(GfQuatf(0.0));
            outScale->push_back(GfVec3h(1.0));
        }
        else {
            MFnMatrixData matrixData(matrixPlug.asMObject());
            MTransformationMatrix xf = matrixData.transformation();

            // Don't use Maya's built-in getTranslation(), etc. here because:
            // - The rotation won't account for the jointOrient rotation, so
            //   you'd have to query that from MFnIkJoint and combine.
            // - The scale is special on joints because the scale on a parent
            //   joint isn't inherited by children, due to an implicit
            //   (inverse of parent scale) factor when computing joint
            //   transformation matrices.
            // In short, no matter what you do, there will be cases where the
            // Maya joint transform can't be perfectly replicated in UsdSkel,
            // so just use UsdSkelDecomposeTransform instead to get something
            // that matches the transformation order required by UsdSkel.
            GfMatrix4d mat(xf.asMatrix().matrix);
            GfVec3f trans;
            GfQuatf rot;
            GfVec3h scale;
            UsdSkelDecomposeTransform(mat, &trans, &rot, &scale);
            outTrans->push_back(trans);
            outRot->push_back(rot);
            outScale->push_back(scale);
        }
    }
}

void
MayaSkeletonWriter::write(const UsdTimeCode &usdTime)
{
    SdfPath animPath = getUsdPath().AppendChild(_tokens->Animation);

    if (usdTime.IsDefault()) {
        std::vector<MDagPath> jointDags = _GetJoints(getDagPath());
        if (jointDags.size() == 0) {
            TF_CODING_ERROR("There should be at least one joint "
                    "since this prim is a joint");
            return;
        }

        VtTokenArray jointNames = GetJointNames(jointDags, jointDags[0]);
        std::string whyNotValid;
        if (!UsdSkelTopology(jointNames).Validate(&whyNotValid)) {
            TF_CODING_ERROR("Joint topology invalid: %s", whyNotValid.c_str());
            return;
        }

        VtMatrix4dArray restTransforms = _GetRestTransforms(jointDags);

        UsdSkelSkeleton primSchema(mUsdPrim);
        _SetAttribute(primSchema.CreateJointsAttr(), jointNames);
        _SetAttribute(primSchema.CreateRestTransformsAttr(), &restTransforms);

        VtTokenArray animJointNames;
        _GetAnimatedJoints(
                jointNames, jointDags,
                &animJointNames, &_animatedJoints,
                mWriteJobCtx.getArgs().exportAnimation);
        if (animJointNames.size() > 0) {
            UsdSkelPackedJointAnimation anim =
                    UsdSkelPackedJointAnimation::Define(
                        getUsdStage(), animPath);
            _SetAttribute(anim.CreateJointsAttr(), animJointNames);
        }
    }

    // Time-varying step: write the packed joint animation transforms once per
    // time code. We do want to run this @ default time also so that any
    // deviations from the rest pose are exported as the default values on the
    // PackedJointAnimation.
    if (isShapeAnimated()) {
        UsdSkelPackedJointAnimation skelAnim =
                UsdSkelPackedJointAnimation::Get(getUsdStage(), animPath);
        if (!skelAnim) {
            TF_CODING_ERROR(
                    "PackedJointAnimation <%s> doesn't exist but should have "
                    "been created during default-time pass",
                    animPath.GetText());
            return;
        }

        // XXX It is difficult for us to tell which components are actually
        // animated since _GetAnimationData relies on a matrix decomposition
        // to get separate components.
        // In the future, we may want to RLE-compress the data in postExport
        // to remove redundant time samples.
        VtVec3fArray translations;
        VtQuatfArray rotations;
        VtVec3hArray scales;
        _GetAnimationData(
                _animatedJoints, &translations, &rotations, &scales);
        _SetAttribute(skelAnim.CreateTranslationsAttr(), &translations, usdTime);
        _SetAttribute(skelAnim.CreateRotationsAttr(), &rotations, usdTime);
        _SetAttribute(skelAnim.CreateScalesAttr(), &scales, usdTime);
    }
}

bool
MayaSkeletonWriter::exportsGprims() const
{
    return true;
}

bool
MayaSkeletonWriter::shouldPruneChildren() const
{
    return true;
}

bool
MayaSkeletonWriter::isShapeAnimated() const
{
    // Technically, the UsdSkelSkeleton isn't animated, but we're going to put
    // the PackedJointAnimation underneath, and that does have animation.
    return _animatedJoints.size() > 0;
}

bool
MayaSkeletonWriter::getAllAuthoredUsdPaths(SdfPathVector* outPaths) const
{
    bool hasPrims = MayaPrimWriter::getAllAuthoredUsdPaths(outPaths);
    SdfPath animPath = getUsdPath().AppendChild(_tokens->Animation);
    if (getUsdStage()->GetPrimAtPath(animPath)) {
        outPaths->push_back(animPath);
        hasPrims = true;
    }
    return hasPrims;
}

PXR_NAMESPACE_CLOSE_SCOPE
