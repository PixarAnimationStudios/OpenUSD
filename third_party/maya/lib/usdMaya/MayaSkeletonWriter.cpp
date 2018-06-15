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
#include "usdMaya/translatorUtil.h"
#include "usdMaya/util.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/usdGeom/xform.h"
#include "pxr/usd/usdSkel/bindingAPI.h"
#include "pxr/usd/usdSkel/packedJointAnimation.h"
#include "pxr/usd/usdSkel/root.h"
#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdSkel/utils.h"

#include "pxr/usd/sdf/pathTable.h"

#include <maya/MAnimUtil.h>
#include <maya/MFnTransform.h>
#include <maya/MItDag.h>
#include <maya/MMatrix.h>


PXR_NAMESPACE_OPEN_SCOPE

PXRUSDMAYA_REGISTER_ADAPTOR_SCHEMA(MFn::kJoint, UsdSkelSkeleton);

TF_DEFINE_PRIVATE_TOKENS(
    _tokens, 
    (Animation)
    (Skeleton)
);


static SdfPath
_GetSkeletonPath(const SdfPath& skelInstancePath)
{
    return skelInstancePath.AppendChild(_tokens->Skeleton);
}


static SdfPath
_GetAnimationPath(const SdfPath& skelInstancePath)
{
    return _GetSkeletonPath(skelInstancePath).AppendChild(_tokens->Animation);
}


MayaSkeletonWriter::MayaSkeletonWriter(const MDagPath & iDag,
                                       const SdfPath& uPath,
                                       usdWriteJobCtx& jobCtx)
    : MayaPrimWriter(iDag, uPath, jobCtx),
      _valid(false), _animXformIsAnimated(false)
{
    const TfToken& exportSkels = jobCtx.getArgs().exportSkels;
    if (exportSkels != PxrUsdExportJobArgsTokens->auto_ &&
        exportSkels != PxrUsdExportJobArgsTokens->explicit_) {
        return;
    }

    SdfPath skelInstancePath = GetSkeletonInstancePath(
            iDag, mWriteJobCtx.getArgs().stripNamespaces);

    SdfPath skelPath = _GetSkeletonPath(skelInstancePath);

    _skel = UsdSkelSkeleton::Define(getUsdStage(), skelPath);
    if (!TF_VERIFY(_skel))
        return;

    // The skeleton may be bound at this scope, or at the parent scope.
    if (skelInstancePath == uPath) {

        // The skeleton will be bound at the path corresponding to the joint.
        // This means that we need to produce a new xform at the joint,
        // which we use to bind the skel instance.

        _skelInstance = UsdGeomXform::Define(getUsdStage(),
                                             skelInstancePath).GetPrim();
        if (TF_VERIFY(_skelInstance)) {
            mUsdPrim = _skelInstance;
        }

    } else {
        // The skel instance will be bound on an ancestor prim,
        // which we expect to already exist.
        _skelInstance = getUsdStage()->GetPrimAtPath(skelInstancePath);
        if (TF_VERIFY(_skelInstance)) {
            mUsdPrim = _skel.GetPrim();
        }
    }
}


static bool
_IsUsdJointHierarchyRoot(const MDagPath& joint)
{
    MFnDependencyNode jointDep(joint.node());
    MPlug plug = jointDep.findPlug("USD_isJointHierarchyRoot");
    if (!plug.isNull()) {
        return plug.asBool();
    }
    return false;
}


VtTokenArray
MayaSkeletonWriter::GetJointNames(
    const std::vector<MDagPath>& joints,
    const MDagPath& rootDagPath,
    bool stripNamespaces)
{

    // Get paths relative to the root.
    // Joints have to be transforms, so mergeTransformAndShape
    // shouldn't matter here. (Besides, we're not actually using these
    // to point to prims.)
    SdfPath rootPath = PxrUsdMayaUtil::MDagPathToUsdPath(
            rootDagPath, /*mergeTransformAndShape*/ false, stripNamespaces);

    if (!_IsUsdJointHierarchyRoot(rootDagPath)) {   
        // Compute joint names  relative to the path of the parent node.
        rootPath = rootPath.GetParentPath();
    }

    VtTokenArray result;
    for (const MDagPath& joint : joints) {
        SdfPath path = PxrUsdMayaUtil::MDagPathToUsdPath(
                joint, /*mergeTransformAndShape*/ false, stripNamespaces);
        result.push_back(path.MakeRelativePath(rootPath).GetToken());
    }
    return result;
}


SdfPath
MayaSkeletonWriter::GetSkeletonInstancePath(
    const MDagPath& rootJoint,
    bool stripNamespaces)
{
    SdfPath rootJointPath = PxrUsdMayaUtil::MDagPathToUsdPath(
        rootJoint, /*mergeTransformAndShape*/ false, stripNamespaces);

    if (_IsUsdJointHierarchyRoot(rootJoint)) {
        // The root joint is the special joint created for round-tripping
        // UsdSkel data. The skeleton instance will be bound on the parent
        // path, with the skeleton created beneath it.
        
        if (!rootJointPath.IsRootPrimPath()) {
            return rootJointPath.GetParentPath();
        } else {
            TF_WARN("Ignoring invalid attr 'USD_isJointHierarchyRoot' on node "
                    "<%s>: attr is only valid on non-root nodes.",
                    rootJointPath.GetText());
        }
    }
    return rootJointPath;
}



/// Gets all of the joints rooted at the given dag path.
/// If \p includeRoot, the set of joints includes the given dag path as well.
/// The vector returned ensures that ancestor joints come before descendant
/// joints.
static std::vector<MDagPath>
_GetJoints(const MDagPath& dagPath, bool includeRoot)
{
    MItDag dagIter(MItDag::kDepthFirst, MFn::kJoint);
    dagIter.reset(dagPath, MItDag::kDepthFirst, MFn::kJoint);

    if (!includeRoot)
        dagIter.next();

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


/// Gets the root transformation of the skeleton instance exported for a joint.
static GfMatrix4d
_GetSkelLocalToWorldTransform(const MDagPath& dagPath)
{
    if (_IsUsdJointHierarchyRoot(dagPath)) {
        // The input path is the special root joint used to hold the
        // anim transform during round tripping; it gives us the complete
        // local-to-world transform of the skel.
        return -_GetJointWorldTransform(dagPath);
    } else if(dagPath.length() > 1) {
        // The input dagPath is expected to be our root joint.
        // The transform of the parent will provide the local-to-world
        // transform of the skel.
        MDagPath parent(dagPath);
        parent.pop();
        return GfMatrix4d(parent.inclusiveMatrix().matrix);
    }

    // Can't find a reasonable root transform for the skel instance.
    // Fall back to the identity.
    return GfMatrix4d(1);
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
    // order, because UsdSkel allows arbitrary order on PackedJointAnimations.
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
    bool isUsdJointHierarchyRoot = _IsUsdJointHierarchyRoot(getDagPath());

    _joints = _GetJoints(getDagPath(), !isUsdJointHierarchyRoot);
    if (!isUsdJointHierarchyRoot && _joints.size() == 0) {
        TF_CODING_ERROR("There should be at least one joint "
                        "since this prim is a joint");
        return false;
    }

    VtTokenArray skelJointNames = GetJointNames(
            _joints, getDagPath(), mWriteJobCtx.getArgs().stripNamespaces);
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
        ::GetAPISchemaForAuthoring<UsdSkelBindingAPI>(_skelInstance);

    binding.CreateSkeletonRel().SetTargets({_skel.GetPrim().GetPath()});

    // Mark the bindings for post processing.
    SdfPath skelInstancePath = GetSkeletonInstancePath(
            getDagPath(), mWriteJobCtx.getArgs().stripNamespaces);
    mWriteJobCtx.getSkelBindingsWriter().MarkBindings(
        skelInstancePath, skelInstancePath,
        mWriteJobCtx.getArgs().exportSkels);
        
    VtMatrix4dArray restXforms =
        _GetJointLocalRestTransforms(_topology, _joints);

    _SetAttribute(_skel.GetJointsAttr(), skelJointNames);
    _SetAttribute(_skel.GetRestTransformsAttr(), restXforms);

    VtTokenArray animJointNames;
    _GetAnimatedJoints(_topology, skelJointNames, getDagPath(),
                       _joints, restXforms,
                       &animJointNames, &_animatedJoints,
                       !mWriteJobCtx.getArgs().timeInterval.IsEmpty());
    
    if (isUsdJointHierarchyRoot || animJointNames.size() > 0) {

        SdfPath animPath = _GetAnimationPath(_skelInstance.GetPath());
        _skelAnim = UsdSkelPackedJointAnimation::Define(
            getUsdStage(), animPath);

        if (TF_VERIFY(_skelAnim)) {

            if (isUsdJointHierarchyRoot) {

                // The root joint (current dag path) holds the
                // anim transform for the joint animation.
                // Create a matrix attr to hold that transform.
                
                _animXformAttr = _skelAnim.MakeMatrixXform();

                if (!getArgs().timeInterval.IsEmpty()) {
                    MObject node = getDagPath().node();
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
MayaSkeletonWriter::write(const UsdTimeCode &usdTime)
{
    if (usdTime.IsDefault()) {
        _valid = _WriteRestState();
    }

    if (!_valid)
        return;
 
    if ((usdTime.IsDefault() || _animXformIsAnimated) && _animXformAttr) {

        // If we have an anim transform attr to write to, the local transform
        // of the current dag path provides the anim transform.
        GfMatrix4d localXf = _GetJointLocalTransform(getDagPath());
        _SetAttribute(_animXformAttr, localXf, usdTime);
    }

    // Time-varying step: write the packed joint animation transforms once per
    // time code. We do want to run this @ default time also so that any
    // deviations from the rest pose are exported as the default values on the
    // PackedJointAnimation.
    if (_animatedJoints.size() > 0) {

        if (!_skelAnim) {

            SdfPath animPath = _GetAnimationPath(_skelInstance.GetPath());

            TF_CODING_ERROR(
                "PackedJointAnimation <%s> doesn't exist but should "
                "have been created during default-time pass.",
                animPath.GetText());
            return;
        }

        GfMatrix4d rootXf = _GetSkelLocalToWorldTransform(getDagPath());

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
                    // postExport to remove redundant time samples.
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
MayaSkeletonWriter::exportsGprims() const
{
    // Nether the Skeleton nor its animation sources are gprims.
    return false;
}

bool
MayaSkeletonWriter::shouldPruneChildren() const
{
    return true;
}

bool
MayaSkeletonWriter::isShapeAnimated() const
{
    // Either the root xform or the PackedJointAnimation beneath it
    // may be animated.
    return _animXformIsAnimated || _animatedJoints.size() > 0;
}

bool
MayaSkeletonWriter::getAllAuthoredUsdPaths(SdfPathVector* outPaths) const
{
    bool hasPrims = MayaPrimWriter::getAllAuthoredUsdPaths(outPaths);

    SdfPath skelInstancePath = GetSkeletonInstancePath(
            getDagPath(), mWriteJobCtx.getArgs().stripNamespaces);
    SdfPath skelPath = _GetSkeletonPath(skelInstancePath);
    SdfPath animPath = _GetAnimationPath(skelInstancePath);
    
    for (const SdfPath& path : {skelPath, animPath}) {  
        if (getUsdStage()->GetPrimAtPath(path)) {
            outPaths->push_back(path);
            hasPrims = true;
        }
    }
    return hasPrims;
}


PXR_NAMESPACE_CLOSE_SCOPE
