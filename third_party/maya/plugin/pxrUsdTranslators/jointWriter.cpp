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
#include "pxrUsdTranslators/jointWriter.h"

#include "usdMaya/adaptor.h"
#include "usdMaya/primWriter.h"
#include "usdMaya/primWriterRegistry.h"
#include "usdMaya/translatorSkel.h"
#include "usdMaya/translatorUtil.h"
#include "usdMaya/util.h"
#include "usdMaya/writeJobContext.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/timeCode.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdSkel/animation.h"
#include "pxr/usd/usdSkel/bindingAPI.h"
#include "pxr/usd/usdSkel/root.h"
#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdSkel/utils.h"

#include "pxr/usd/sdf/pathTable.h"

#include <maya/MAnimUtil.h>
#include <maya/MDagPath.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnTransform.h>
#include <maya/MItDag.h>
#include <maya/MMatrix.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>

#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


PXRUSDMAYA_REGISTER_WRITER(joint, PxrUsdTranslators_JointWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(joint, UsdSkelSkeleton);


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (Animation)
    (Skeleton)
);


static
SdfPath
_GetAnimationPath(const SdfPath& skelPath)
{
    return skelPath.AppendChild(_tokens->Animation);
}


/// Gets all of the components of the joint hierarchy rooted at \p dagPath.
/// The \p skelXformPath will hold the path to a joint that defines
/// the transform of a UsdSkelSkeleton. It may be invalid if no
/// joint explicitly defines that transform.
/// The \p joints array, if provided, will be filled with the ordered set of
/// joint paths, excluding the set of joints described above.
/// The \p jointHierarchyRootPath will hold the common parent path of
/// all of the returned joints.
static
void
_GetJointHierarchyComponents(
        const MDagPath& dagPath,
        MDagPath* skelXformPath,
        MDagPath* jointHierarchyRootPath,
        std::vector<MDagPath>* joints=nullptr)
{
    if(joints)
        joints->clear();
    *skelXformPath = MDagPath();

    MItDag dagIter(MItDag::kDepthFirst, MFn::kJoint);
    dagIter.reset(dagPath, MItDag::kDepthFirst, MFn::kJoint);

    // The first joint may be the root of a Skeleton.
    if (!dagIter.isDone()) {
        MDagPath path;
        dagIter.getPath(path);
        if (UsdMayaTranslatorSkel::IsUsdSkeleton(path)) {
            *skelXformPath = path;
            dagIter.next();
        }
    }

    // All remaining joints are treated as normal joints.
    if (joints) {
        while (!dagIter.isDone()) {
            MDagPath path;
            dagIter.getPath(path);
            joints->push_back(path);
            dagIter.next();
        }
    }

    if(skelXformPath->isValid()) {
        *jointHierarchyRootPath = *skelXformPath;
    } else {
        *jointHierarchyRootPath = dagPath;
        jointHierarchyRootPath->pop();
    }
}


PxrUsdTranslators_JointWriter::PxrUsdTranslators_JointWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath& usdPath,
        UsdMayaWriteJobContext& jobCtx) :
    UsdMayaPrimWriter(depNodeFn, usdPath, jobCtx),
    _valid(false)
{
    TF_AXIOM(GetDagPath().isValid());

    const TfToken& exportSkels = _GetExportArgs().exportSkels;
    if (exportSkels != UsdMayaJobExportArgsTokens->auto_ &&
            exportSkels != UsdMayaJobExportArgsTokens->explicit_) {
        return;
    }

    SdfPath skelPath =
        GetSkeletonPath(GetDagPath(), _GetExportArgs().stripNamespaces);

    _skel = UsdSkelSkeleton::Define(GetUsdStage(), skelPath);
    if (!TF_VERIFY(_skel)) {
        return;
    }

    _usdPrim = _skel.GetPrim();
}

VtTokenArray
PxrUsdTranslators_JointWriter::GetJointNames(
        const std::vector<MDagPath>& joints,
        const MDagPath& rootDagPath,
        bool stripNamespaces)
{
    MDagPath skelXformPath, jointHierarchyRootPath;
    _GetJointHierarchyComponents(rootDagPath, &skelXformPath,
                                 &jointHierarchyRootPath);

    // Get paths relative to the root of the joint hierarchy.
    // Joints have to be transforms, so mergeTransformAndShape
    // shouldn't matter here. (Besides, we're not actually using these
    // to point to prims.)
    SdfPath rootPath = UsdMayaUtil::MDagPathToUsdPath(
            jointHierarchyRootPath, /*mergeTransformAndShape*/ false,
            stripNamespaces);

    VtTokenArray result;
    for (const MDagPath& joint : joints) {

        SdfPath path = UsdMayaUtil::MDagPathToUsdPath(
                joint, /*mergeTransformAndShape*/ false, stripNamespaces);
        result.push_back(path.MakeRelativePath(rootPath).GetToken());
    }
    return result;
}

SdfPath
PxrUsdTranslators_JointWriter::GetSkeletonPath(
        const MDagPath& rootJoint,
        bool stripNamespaces)
{
    return UsdMayaUtil::MDagPathToUsdPath(
        rootJoint, /*mergeTransformAndShape*/ false, stripNamespaces);
}

/// Whether the transform plugs on a transform node are animated.
static
bool
_IsTransformNodeAnimated(const MDagPath& dagPath)
{
    MFnDependencyNode node(dagPath.node());
    return UsdMayaUtil::isPlugAnimated(node.findPlug("translateX")) ||
           UsdMayaUtil::isPlugAnimated(node.findPlug("translateY")) ||
           UsdMayaUtil::isPlugAnimated(node.findPlug("translateZ")) ||
           UsdMayaUtil::isPlugAnimated(node.findPlug("rotateX")) ||
           UsdMayaUtil::isPlugAnimated(node.findPlug("rotateY")) ||
           UsdMayaUtil::isPlugAnimated(node.findPlug("rotateZ")) ||
           UsdMayaUtil::isPlugAnimated(node.findPlug("scaleX")) ||
           UsdMayaUtil::isPlugAnimated(node.findPlug("scaleY")) ||
           UsdMayaUtil::isPlugAnimated(node.findPlug("scaleZ"));
}

/// Gets the world-space rest transform for a single dag path.
static
GfMatrix4d
_GetJointWorldBindTransform(const MDagPath& dagPath)
{
    MFnDagNode dagNode(dagPath);
    MMatrix restTransformWorld;
    if (UsdMayaUtil::getPlugMatrix(
            dagNode, "bindPose", &restTransformWorld)) {
        return GfMatrix4d(restTransformWorld.matrix);
    }
    // No bindPose. Assume it's identity.
    return GfMatrix4d(1);
}

/// Gets world-space bind transforms for all specified dag paths.
static
VtMatrix4dArray
_GetJointWorldBindTransforms(
        const UsdSkelTopology& topology,
        const std::vector<MDagPath>& jointDagPaths)
{
    size_t numJoints = jointDagPaths.size();
    VtMatrix4dArray worldXforms(numJoints);
    for (size_t i = 0; i < jointDagPaths.size(); ++i) {
        worldXforms[i] = _GetJointWorldBindTransform(jointDagPaths[i]);
    }
    return worldXforms;
}

/// Find a dagPose that holds a bind pose for \p dagPath.
static
MObject
_FindBindPose(const MDagPath& dagPath)
{
    MStatus status;

    MFnDependencyNode depNode(dagPath.node(), &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject());

    MPlug msgPlug = depNode.findPlug("message", &status);

    MPlugArray outputs;
    msgPlug.connectedTo(outputs, /*asDst*/ false, /*asSrc*/ true, &status);

    for (unsigned int i = 0; i < outputs.length(); ++i) {
        MObject outputNode = outputs[i].node();

        if (outputNode.apiType() == MFn::kDagPose) {

            // dagPose nodes have a 'bindPose' bool that determines whether
            // or not they represent a bind pose.

            MFnDependencyNode poseDep(outputNode, &status);
            MPlug bindPosePlug = poseDep.findPlug("bindPose", &status);
            if (status) {
                if (bindPosePlug.asBool()) {
                    return outputNode;
                }
            }

            return outputNode;
        }
    }
    return MObject();
}

/// Get the member indices of all objects in \p dagPaths within the
/// members array plug of a dagPose.
/// Returns true only if all \p dagPaths can be mapped to a dagPose member.
static
bool
_FindDagPoseMembers(
        const MFnDependencyNode& dagPoseDep,
        const std::vector<MDagPath>& dagPaths,
        std::vector<unsigned int>* indices)
{
    MStatus status;
    MPlug membersPlug = dagPoseDep.findPlug("members", &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // Build a map of dagPath->index.

    struct _HashObjectHandle {
        std::size_t operator()(const MObjectHandle& o) const {
            return o.hashCode();
        }
    };

    std::unordered_map<MObjectHandle,size_t,_HashObjectHandle> pathIndexMap;
    for (size_t i = 0; i < dagPaths.size(); ++i) {
        pathIndexMap[MObjectHandle(dagPaths[i].node())] = i;
    }

    MPlugArray inputs;

    indices->clear();
    indices->resize(membersPlug.numElements(), -1);

    for (unsigned int i = 0; i < membersPlug.numElements(); ++i) {

        MPlug memberPlug = membersPlug[i];
        memberPlug.connectedTo(inputs, /*asDst*/ true, /*asSrc*/ false);

        for (unsigned int j = 0; j < inputs.length(); ++j) {
            MObjectHandle connNode(inputs[j].node());
            auto it = pathIndexMap.find(connNode);
            if (it != pathIndexMap.end()) {
                (*indices)[it->second] = i;
            }
        }
    }

    // Validate that all of the input dagPaths are members.
    for (size_t i = 0; i < indices->size(); ++i) {
        int index = (*indices)[i];
        if (index < 0) {
            TF_WARN("Node '%s' is not a member of dagPose '%s'.",
                    MFnDependencyNode(dagPaths[i].node()).name().asChar(),
                    dagPoseDep.name().asChar());
            return false;
        }
    }
    return true;
}

bool
_GetLocalTransformForDagPoseMember(
        const MFnDependencyNode& dagPoseDep,
        unsigned int index,
        GfMatrix4d* xform)
{
    MStatus status;

    MPlug xformMatrixPlug = dagPoseDep.findPlug("xformMatrix");
    if (index < xformMatrixPlug.numElements()) {
        MPlug xformPlug = xformMatrixPlug[index];

        MObject plugObj = xformPlug.asMObject(MDGContext::fsNormal, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        MFnMatrixData plugMatrixData(plugObj, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        *xform = GfMatrix4d(plugMatrixData.matrix().matrix);
        return true;
    }
    return false;
}

/// Get local-space bind transforms to use as rest transforms.
/// The dagPose is expected to hold the local transforms.
static
bool
_GetJointLocalRestTransformsFromDagPose(
        const SdfPath& skelPath,
        const MDagPath& rootJoint,
        const std::vector<MDagPath>& jointDagPaths,
        VtMatrix4dArray* xforms)
{
    // Use whatever bindPose the root joint is a member of.
    MObject bindPose = _FindBindPose(rootJoint);
    if (bindPose.isNull()) {
        TF_WARN("%s -- Could not find a dagPose node holding a bind pose: "
                "The Skeleton's 'restTransforms' property will not be "
                "authored.", skelPath.GetText());
        return false;
    }

    MStatus status;
    MFnDependencyNode bindPoseDep(bindPose, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    std::vector<unsigned int> memberIndices;
    if (!_FindDagPoseMembers(bindPoseDep, jointDagPaths, &memberIndices)) {
        return false;
    }

    xforms->resize(jointDagPaths.size());
    for (size_t i = 0; i < xforms->size(); ++i) {
        if (!_GetLocalTransformForDagPoseMember(
                bindPoseDep, memberIndices[i], xforms->data()+i)) {
            TF_WARN("%s -- Failed retrieving the local transform of joint '%s' "
                    "from dagPose '%s': The Skeleton's 'restTransforms' "
                    "property will not be authored.", skelPath.GetText(),
                    jointDagPaths[i].fullPathName().asChar(),
                    bindPoseDep.name().asChar());
            return false;
        }
    }
    return true;
}

/// Gets the world-space transform of \p dagPath at the current time.
static
GfMatrix4d
_GetJointWorldTransform(const MDagPath& dagPath)
{
    // Don't use Maya's built-in getTranslation(), etc. when extracting the
    // transform because:
    // - The rotation won't account for the jointOrient rotation, so
    //   you'd have to query that from MFnIkJoint and combine.
    // - The scale is special on joints because the scale on a parent
    //   joint isn't inherited by children, due to an implicit
    //   (inverse of parent scale) factor when computing joint
    //   transformation matrices.
    // In short, no matter what you do, there will be cases where the
    // Maya joint transform can't be perfectly replicated in UsdSkel;
    // it's much easier to ensure correctness by letting UsdSkel work
    // with raw transform data, and perform its own decomposition later
    // with UsdSkelDecomposeTransforms.

    MStatus status;
    MMatrix mx = dagPath.inclusiveMatrix(&status);
    return status ? GfMatrix4d(mx.matrix) : GfMatrix4d(1);
}

/// Gets the world-space transform of \p dagPath at the current time.
static
GfMatrix4d
_GetJointLocalTransform(const MDagPath& dagPath)
{
    MStatus status;
    MFnTransform xform(dagPath, &status);
    if (status) {
        MTransformationMatrix mx = xform.transformation(&status);
        if (status) {
            return GfMatrix4d(mx.asMatrix().matrix);
        }
    }
    return GfMatrix4d(1);
}

/// Computes world-space joint transforms for all specified dag paths
/// at the current time.
static
bool
_GetJointWorldTransforms(
        const std::vector<MDagPath>& dagPaths,
        VtMatrix4dArray* xforms)
{
    xforms->resize(dagPaths.size());
    GfMatrix4d* xformsData = xforms->data();
    for (size_t i = 0; i < dagPaths.size(); ++i) {
        xformsData[i] = _GetJointWorldTransform(dagPaths[i]);
    }
    return true;
}

/// Computes joint-local transforms for all specified dag paths
/// at the current time.
static
bool
_GetJointLocalTransforms(
        const UsdSkelTopology& topology,
        const std::vector<MDagPath>& dagPaths,
        const GfMatrix4d& rootXf,
        VtMatrix4dArray* localXforms)
{
    VtMatrix4dArray worldXforms;
    if (_GetJointWorldTransforms(dagPaths, &worldXforms)) {

        GfMatrix4d rootInvXf = rootXf.GetInverse();

        VtMatrix4dArray worldInvXforms(worldXforms);
        for (auto& xf : worldInvXforms)
            xf = xf.GetInverse();

        return UsdSkelComputeJointLocalTransforms(topology, worldXforms,
                                                  worldInvXforms,
                                                  localXforms, &rootInvXf);
    }
    return true;
}

/// Returns true if the joint's transform definitely matches its rest transform
/// over all exported frames.
static
bool
_JointMatchesRestPose(
        size_t jointIdx,
        const MDagPath& dagPath,
        const VtMatrix4dArray& xforms,
        const VtMatrix4dArray& restXforms,
        bool exportingAnimation)
{
    if (exportingAnimation && _IsTransformNodeAnimated(dagPath))
        return false;
    else if (jointIdx < xforms.size())
        return GfIsClose(xforms[jointIdx], restXforms[jointIdx], 1e-8);
    return false;
}

/// Given the list of USD joint names and dag paths, returns the joints that
/// (1) are moved from their rest poses or (2) have animation, if we are going
/// to export animation.
static
void
_GetAnimatedJoints(
        const UsdSkelTopology& topology,
        const VtTokenArray& usdJointNames,
        const MDagPath& rootDagPath,
        const std::vector<MDagPath>& jointDagPaths,
        const VtMatrix4dArray& restXforms,
        VtTokenArray* animatedJointNames,
        std::vector<MDagPath>* animatedJointPaths,
        bool exportingAnimation)
{
    TF_AXIOM(usdJointNames.size() == jointDagPaths.size());

    if (restXforms.size() != usdJointNames.size()) {
        // Either have invalid restXforms or no restXforms at all
        // (the latter happens when a user deletes the dagPose).
        // Must treat all joinst as animated.
        *animatedJointNames = usdJointNames;
        *animatedJointPaths = jointDagPaths;
        return;
    }

    VtMatrix4dArray localXforms;
    if (!exportingAnimation) {
        // Compute the current local xforms of all joints so we can decide
        // whether or not they need to have a value encoded on the anim prim.
        GfMatrix4d rootXform = _GetJointWorldTransform(rootDagPath);
        _GetJointLocalTransforms(topology, jointDagPaths,
                                 rootXform, &localXforms);
    }

    // The resulting vector contains only animated joints or joints not
    // in their rest pose. The order is *not* guaranteed to be the Skeleton
    // order, because UsdSkel allows arbitrary order on SkelAnimation.
    for (size_t i = 0; i < jointDagPaths.size(); ++i) {
        const TfToken& jointName = usdJointNames[i];
        const MDagPath& dagPath = jointDagPaths[i];

        if (!_JointMatchesRestPose(i, jointDagPaths[i], localXforms,
                                   restXforms, exportingAnimation)) {
            animatedJointNames->push_back(jointName);
            animatedJointPaths->push_back(dagPath);
        }
    }
}

bool
PxrUsdTranslators_JointWriter::_WriteRestState()
{
    // Check if the root joint is the special root joint created
    // for round-tripping UsdSkel data.
    bool haveUsdSkelXform =
        UsdMayaTranslatorSkel::IsUsdSkeleton(GetDagPath());

    if (!haveUsdSkelXform) {
        // We don't have a joint that represents the Skeleton.
        // This means that the joint hierarchy is originating from Maya.
        // Mark it, so that the exported results can be reimported in
        // a structure-preserving way.
        UsdMayaTranslatorSkel::MarkSkelAsMayaGenerated(_skel);
    }

    _GetJointHierarchyComponents(GetDagPath(),
                                 &_skelXformPath,
                                 &_jointHierarchyRootPath,
                                 &_joints);

    VtTokenArray skelJointNames =
        GetJointNames(_joints, GetDagPath(),
                      _GetExportArgs().stripNamespaces);
    _topology = UsdSkelTopology(skelJointNames);
    std::string whyNotValid;
    if (!_topology.Validate(&whyNotValid)) {
        TF_CODING_ERROR("Joint topology is invalid: %s",
                        whyNotValid.c_str());
        return false;
    }

    // Setup binding relationships on the instance prim,
    // so that the root xform establishes a skeleton instance
    // with the right transform.
    const UsdSkelBindingAPI binding = UsdMayaTranslatorUtil
        ::GetAPISchemaForAuthoring<UsdSkelBindingAPI>(_skel.GetPrim());

    // Mark the bindings for post processing.

    _SetAttribute(_skel.GetJointsAttr(), skelJointNames);

    SdfPath skelPath = _skel.GetPrim().GetPath();
    _writeJobCtx.MarkSkelBindings(
        skelPath, skelPath, _GetExportArgs().exportSkels);

    VtMatrix4dArray bindXforms =
        _GetJointWorldBindTransforms(_topology, _joints);
    _SetAttribute(_skel.GetBindTransformsAttr(), bindXforms);

    VtMatrix4dArray restXforms;
    if (_GetJointLocalRestTransformsFromDagPose(
            skelPath, GetDagPath(), _joints, &restXforms)) {
        _SetAttribute(_skel.GetRestTransformsAttr(), restXforms);
    }

    VtTokenArray animJointNames;
    _GetAnimatedJoints(_topology, skelJointNames, GetDagPath(),
                       _joints, restXforms,
                       &animJointNames, &_animatedJoints,
                       !_GetExportArgs().timeSamples.empty());

    if (haveUsdSkelXform) {
        _skelXformAttr = _skel.MakeMatrixXform();
        if (!_GetExportArgs().timeSamples.empty()) {
            MObject node = _skelXformPath.node();
            _skelXformIsAnimated = UsdMayaUtil::isAnimated(node);
        } else {
            _skelXformIsAnimated = false;
        }
    }

    if (!animJointNames.empty()) {

        SdfPath animPath = _GetAnimationPath(skelPath);
        _skelAnim = UsdSkelAnimation::Define(GetUsdStage(), animPath);

        if (TF_VERIFY(_skelAnim)) {
            _skelToAnimMapper =
                UsdSkelAnimMapper(skelJointNames, animJointNames);

            _SetAttribute(_skelAnim.GetJointsAttr(), animJointNames);

            binding.CreateAnimationSourceRel().SetTargets({animPath});
        } else {
            return false;
        }
    }
    return true;
}

/* virtual */
void
PxrUsdTranslators_JointWriter::Write(const UsdTimeCode& usdTime)
{
    if (usdTime.IsDefault()) {
        _valid = _WriteRestState();
    }

    if (!_valid) {
        return;
    }

    if ((usdTime.IsDefault() || _skelXformIsAnimated) && _skelXformAttr) {

        // We have a joint which provides the transform of the Skeleton,
        // instead of the transform of a joint in the hierarchy.
        GfMatrix4d localXf = _GetJointLocalTransform(_skelXformPath);
        _SetAttribute(_skelXformAttr, localXf, usdTime);
    }

    // Time-varying step: write the packed joint animation transforms once per
    // time code. We do want to run this @ default time also so that any
    // deviations from the rest pose are exported as the default values on the
    // SkelAnimation.
    if (!_animatedJoints.empty()) {

        if (!_skelAnim) {

            SdfPath animPath = _GetAnimationPath(_skel.GetPrim().GetPath());

            TF_CODING_ERROR(
                "SkelAnimation <%s> doesn't exist but should "
                "have been created during default-time pass.",
                animPath.GetText());
            return;
        }

        GfMatrix4d rootXf = _GetJointWorldTransform(_jointHierarchyRootPath);

        VtMatrix4dArray localXforms;
        if (_GetJointLocalTransforms(_topology, _joints,
                                     rootXf, &localXforms)) {

            // Remap local xforms into the (possibly sparse) anim order.
            VtMatrix4dArray animLocalXforms;
            if (_skelToAnimMapper.Remap(localXforms, &animLocalXforms)) {

                VtVec3fArray translations;
                VtQuatfArray rotations;
                VtVec3hArray scales;
                if (UsdSkelDecomposeTransforms(animLocalXforms, &translations,
                                               &rotations, &scales)) {

                    // XXX It is difficult for us to tell which components are
                    // actually animated since we rely on decomposition to get
                    // separate anim components.
                    // In the future, we may want to RLE-compress the data in
                    // PostExport to remove redundant time samples.
                    _SetAttribute(_skelAnim.GetTranslationsAttr(),
                                  &translations, usdTime);
                    _SetAttribute(_skelAnim.GetRotationsAttr(),
                                  &rotations, usdTime);
                    _SetAttribute(_skelAnim.GetScalesAttr(),
                                  &scales, usdTime);
                }
            }
        }
    }
}

/* virtual */
bool
PxrUsdTranslators_JointWriter::ExportsGprims() const
{
    // Nether the Skeleton nor its animation sources are gprims.
    return false;
}

/* virtual */
bool
PxrUsdTranslators_JointWriter::ShouldPruneChildren() const
{
    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE
