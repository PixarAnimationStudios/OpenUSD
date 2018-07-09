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
#include "usdMaya/primWriterRegistry.h"
#include "usdMaya/translatorSkel.h"
#include "usdMaya/translatorUtil.h"
#include "usdMaya/usdWriteJobCtx.h"
#include "usdMaya/util.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdSkel/animation.h"
#include "pxr/usd/usdSkel/bindingAPI.h"
#include "pxr/usd/usdSkel/root.h"
#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdSkel/utils.h"

#include "pxr/usd/sdf/pathTable.h"

#include <maya/MAnimUtil.h>
#include <maya/MFnTransform.h>
#include <maya/MItDag.h>
#include <maya/MMatrix.h>


PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_REGISTER_WRITER(joint, MayaSkeletonWriter);
PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(joint, UsdSkelSkeleton);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens, 
    (Animation)
    (Skeleton)
);


static SdfPath
_GetAnimationPath(const SdfPath& skelPath)
{
    return skelPath.AppendChild(_tokens->Animation);
}


/// Gets all of the components of the joint hierarchy rooted at \p dagPath.
/// The \p skelXformPath will hold the path to a joint that defines
/// the transform of a UsdSkelSkeleton. It may be invalid if no
/// joint explicitly defines that transform.
/// The \p animXformPath will hold the path to a joint that defines
/// the transform of a UsdSkelSkeleton's animation source, if any.
/// The \p joints array, if provided, will be filled with the ordered set of
/// joint paths, excluding the set of joints described above.
/// The \p jointHierarchyRootPath will hold the common parent path of
/// all of the returned joints.
static 
void
_GetJointHierarchyComponents(const MDagPath& dagPath,
                             MDagPath* skelXformPath,
                             MDagPath* animXformPath,
                             MDagPath* jointHierarchyRootPath,
                             std::vector<MDagPath>* joints=nullptr)
{
    if(joints)
        joints->clear();
    *skelXformPath = MDagPath();
    *animXformPath = MDagPath();

    MItDag dagIter(MItDag::kDepthFirst, MFn::kJoint);
    dagIter.reset(dagPath, MItDag::kDepthFirst, MFn::kJoint);
    
    // The first joint may be the root of a Skeleton.
    if (!dagIter.isDone()) {
        MDagPath path;
        dagIter.getPath(path);
        if (PxrUsdMayaTranslatorSkel::IsUsdSkelTransform(path)) {
            *skelXformPath = path;
            dagIter.next();
        }
    }

    // The next joint may be the transform corresponding to
    // a UsdSkelAnimation.
    if (!dagIter.isDone()) {
        MDagPath path;
        dagIter.getPath(path);
        if (PxrUsdMayaTranslatorSkel::IsUsdSkelAnimTransform(path)) {
            *animXformPath = path;
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
    
    if (animXformPath->isValid()) {
        *jointHierarchyRootPath = *animXformPath;
    } else if(skelXformPath->isValid()) {
        *jointHierarchyRootPath = *skelXformPath;
    } else {
        *jointHierarchyRootPath = dagPath;  
        jointHierarchyRootPath->pop();
    }
}


// Note: we currently don't support instanceSource for joints, but we have to
// have the argument in order to register the writer plugin.
MayaSkeletonWriter::MayaSkeletonWriter(const MDagPath& iDag,
                                       const SdfPath& uPath,
                                       bool /*instanceSource */,
                                       usdWriteJobCtx& jobCtx)
    : MayaPrimWriter(iDag, uPath, jobCtx),
      _valid(false), _animXformIsAnimated(false)
{
    const TfToken& exportSkels = _GetExportArgs().exportSkels;
    if (exportSkels != PxrUsdExportJobArgsTokens->auto_ &&
        exportSkels != PxrUsdExportJobArgsTokens->explicit_) {
        return;
    }

    SdfPath skelPath =
        GetSkeletonPath(iDag, _GetExportArgs().stripNamespaces);

    _skel = UsdSkelSkeleton::Define(GetUsdStage(), skelPath);
    if (!TF_VERIFY(_skel))
        return;

    _usdPrim = _skel.GetPrim();
}


VtTokenArray
MayaSkeletonWriter::GetJointNames(
    const std::vector<MDagPath>& joints,
    const MDagPath& rootDagPath,
    bool stripNamespaces)
{
    MDagPath skelXformPath, animXformPath, jointHierarchyRootPath;
    _GetJointHierarchyComponents(rootDagPath, &skelXformPath,
                                 &animXformPath, &jointHierarchyRootPath);

    // Get paths relative to the root of the joint hierarchy.
    // Joints have to be transforms, so mergeTransformAndShape
    // shouldn't matter here. (Besides, we're not actually using these
    // to point to prims.)
    SdfPath rootPath = PxrUsdMayaUtil::MDagPathToUsdPath(
            jointHierarchyRootPath, /*mergeTransformAndShape*/ false,
            stripNamespaces);

    VtTokenArray result;
    for (const MDagPath& joint : joints) {

        SdfPath path = PxrUsdMayaUtil::MDagPathToUsdPath(
                joint, /*mergeTransformAndShape*/ false, stripNamespaces);
        result.push_back(path.MakeRelativePath(rootPath).GetToken());
    }
    return result;
}


SdfPath
MayaSkeletonWriter::GetSkeletonPath(const MDagPath& rootJoint,
                                    bool stripNamespaces)
{
    return PxrUsdMayaUtil::MDagPathToUsdPath(
        rootJoint, /*mergeTransformAndShape*/ false, stripNamespaces);
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


/// Gets the world-space rest transform for a single dag path.
static GfMatrix4d
_GetJointWorldRestTransform(const MDagPath& dagPath)
{
    MFnDagNode dagNode(dagPath);
    MMatrix restTransformWorld;
    if (PxrUsdMayaUtil::getPlugMatrix(
            dagNode, "bindPose", &restTransformWorld)) {
        return GfMatrix4d(restTransformWorld.matrix);
    }
    // No bindPose. Assume it's identity.
    return GfMatrix4d(1);
}


/// Gets rest transforms for all the specified dag paths.
static VtMatrix4dArray
_GetJointLocalRestTransforms(
    const UsdSkelTopology& topology,
    const std::vector<MDagPath>& jointDagPaths)
{
    size_t numJoints = jointDagPaths.size();
    VtMatrix4dArray worldXforms(numJoints);
    for (size_t jointIndex = 0; jointIndex < numJoints; ++jointIndex) {
        worldXforms[jointIndex] =
            _GetJointWorldRestTransform(jointDagPaths[jointIndex]);
    }
    
    VtMatrix4dArray worldInvXforms(worldXforms);
    for (auto& xf : worldInvXforms)
        xf = xf.GetInverse();

    VtMatrix4dArray localXforms;
    UsdSkelComputeJointLocalTransforms(topology, worldXforms,
                                       worldInvXforms, &localXforms);
    return localXforms;
}


/// Gets the world-space transform of \p dagPath at the current time.
static GfMatrix4d
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
static GfMatrix4d
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
static bool
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
static bool
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
static bool
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
static void
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
    TF_AXIOM(usdJointNames.size() == restXforms.size());

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
MayaSkeletonWriter::_WriteRestState()
{
    // Check if the root joint is the special root joint created
    // for round-tripping UsdSkel data.
    bool haveUsdSkelXform =
        PxrUsdMayaTranslatorSkel::IsUsdSkelTransform(GetDagPath());
    
    _GetJointHierarchyComponents(GetDagPath(),
                                 &_skelXformPath,
                                 &_animXformPath,
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
    const UsdSkelBindingAPI binding = PxrUsdMayaTranslatorUtil
        ::GetAPISchemaForAuthoring<UsdSkelBindingAPI>(_skel.GetPrim());

    // Mark the bindings for post processing.

    SdfPath skelPath = _skel.GetPrim().GetPath();
    _writeJobCtx.getSkelBindingsWriter().MarkBindings(
        skelPath, skelPath, _GetExportArgs().exportSkels);
        
    VtMatrix4dArray restXforms =
        _GetJointLocalRestTransforms(_topology, _joints);

    _SetAttribute(_skel.GetJointsAttr(), skelJointNames);
    _SetAttribute(_skel.GetRestTransformsAttr(), restXforms);

    VtTokenArray animJointNames;
    _GetAnimatedJoints(_topology, skelJointNames, GetDagPath(),
                       _joints, restXforms,
                       &animJointNames, &_animatedJoints,
                       !_GetExportArgs().timeInterval.IsEmpty());

    if (haveUsdSkelXform) {
        _skelXformAttr = _skel.MakeMatrixXform();
        if (!_GetExportArgs().timeInterval.IsEmpty()) {
            MObject node = _skelXformPath.node();
            _skelXformIsAnimated = PxrUsdMayaUtil::isAnimated(node);
        } else {
            _skelXformIsAnimated = false;
        }
    }

    if (_animXformPath.isValid() || animJointNames.size() > 0) {

        // TODO: pull the name from the anim transform dag path.
        SdfPath animPath = _GetAnimationPath(skelPath);
        _skelAnim = UsdSkelAnimation::Define(GetUsdStage(), animPath);

        if (TF_VERIFY(_skelAnim)) {

            if (_animXformPath.isValid()) {

                // The root joint (current dag path) holds the
                // anim transform for the joint animation.
                // Create a matrix attr to hold that transform.
                
                _animXformAttr = _skelAnim.MakeMatrixXform();

                if (!_GetExportArgs().timeInterval.IsEmpty()) {
                    MObject node = _animXformPath.node();
                    _animXformIsAnimated = PxrUsdMayaUtil::isAnimated(node);
                } else {
                    _animXformIsAnimated = false;
                }
            }

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


void
MayaSkeletonWriter::Write(const UsdTimeCode &usdTime)
{
    if (usdTime.IsDefault()) {
        _valid = _WriteRestState();
    }

    if (!_valid)
        return;

    if ((usdTime.IsDefault() || _skelXformIsAnimated) && _skelXformAttr) {

        // We have a joint which provides the transform of the Skeleton,
        // instead of the transform of a joint in the hierarchy.
        GfMatrix4d localXf = _GetJointLocalTransform(_skelXformPath);
        _SetAttribute(_skelXformAttr, localXf, usdTime);
    }

    if ((usdTime.IsDefault() || _animXformIsAnimated) && _animXformAttr) {

        // If we have an anim transform attr to write to, the local transform
        // of the current dag path provides the anim transform.
        GfMatrix4d localXf = _GetJointLocalTransform(_animXformPath);
        _SetAttribute(_animXformAttr, localXf, usdTime);
    }

    // Time-varying step: write the packed joint animation transforms once per
    // time code. We do want to run this @ default time also so that any
    // deviations from the rest pose are exported as the default values on the
    // SkelAnimation.
    if (_animatedJoints.size() > 0) {

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

bool
MayaSkeletonWriter::ExportsGprims() const
{
    // Nether the Skeleton nor its animation sources are gprims.
    return false;
}

bool
MayaSkeletonWriter::ShouldPruneChildren() const
{
    return true;
}

bool
MayaSkeletonWriter::_IsShapeAnimated() const
{
    // Either the root xform or the SkelAnimation beneath it
    // may be animated.
    return _animXformIsAnimated || _animatedJoints.size() > 0;
}


PXR_NAMESPACE_CLOSE_SCOPE
