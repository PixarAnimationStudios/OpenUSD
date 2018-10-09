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
#include "usdMaya/translatorSkel.h"

#include "usdMaya/translatorUtil.h"
#include "usdMaya/translatorXformable.h"
#include "usdMaya/util.h"

#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/staticTokens.h"

#include "pxr/usd/usdSkel/skeleton.h"
#include "pxr/usd/usdSkel/skeletonQuery.h"
#include "pxr/usd/usdSkel/skinningQuery.h"
#include "pxr/usd/usdSkel/topology.h"

#include <maya/MDoubleArray.h>
#include <maya/MDagModifier.h>
#include <maya/MDGModifier.h>
#include <maya/MFnAnimCurve.h>
#include <maya/MFnComponentListData.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnMatrixData.h>
#include <maya/MFnMesh.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MMatrix.h>
#include <maya/MObjectHandle.h>
#include <maya/MPlug.h>
#include <maya/MPlugArray.h>


PXR_NAMESPACE_OPEN_SCOPE


// There are a lot of nodes and connections that go into a basic skinning rig.
// The following is an overview of everything that must be rigged up:
//
// PER SKELETON:
//    Create joints:
//
//    create transform node to serve as container for joints
//      apply skel's anim transform to this
//    create joint node for each joint
//      set joint.bindPose to joint's skel-space transform
//      apply skel's joint anim to each joint
//      set joint.segmentScaleCompent = false
//
//    Create dagPose:
//
//      Not necessary for skinning, but enables things like the dagPose cmd.
//      Also a requirement for round-tripping the Skeleton's restTransforms.
//
//    create dagPose node
//    connect joint_i.message -> dagPose.members[i]
//    connect bindPose.members[x] -> dagPose.parents[y]
//      where x,y establish proper parent-child relationships
//    connect bindPose.world -> bindPose.parents[i] for each root joint.
//    set bindPOse.worldMatrix[i] = jointSkelRestXforms[i]
//    set bindPose.xformMatrix[i] = jointLocalRestXforms[i]
//
//  PER SKINNED MESH:
//
//   Create a SkinCluster rig:
//
//    set mesh's transform to inheritsTransform=0 to prevent double transforms
//    set mesh's transform to match the USD gprim's geomBindTransform
//      sgustafson: Seems like this should be unnecessary, but I see incorrect
//      results without doing this.
//    create skinClusterGroupParts node of type groupParts
//      set groupParts.inputComponents = vtx[*]
//    create skinClusterGroupId node of type groupId
//    create skinCluster node of type skinCluster
//      set skinCluster weights. Weights are stored as:
//          weights[vertex][joint]
//      set skinCluster.geomMatrix to USD gprim's geomBindTransform.
//
//    create restMesh as a copy of the input mesh
//      set restMesh.intermediateObject = true
//    connect restMesh.outMesh -> skinClusterGroupParts.inputGeometry
//
//    connect skinClusterGroupId.groupId -> skinClusterGroupParts.groupId
//    connect skinClusterGroupParts.groupId -> skinCluster.input[0].groupId
//  TODO:
//    connect groupId.groupId ->
//      mesh.instObjGroups[0].objectGroups[0].objectGroupId
//
//    connect skinClusterGroupParts.outputGeometry ->
//      skinCluster.input[0].inputGeometry
//    connect skinCluster.outputGeometry[0] -> mesh.inMesh
//    connect joints[i].worldMatrix[0] -> skinCluster.matrix[i]
//    connect bindPose.message -> skinCluster.bindPose
//    set skinCluster.bindPreMatrix[i] to the inverse of the skel-space
//      transform of joint i


namespace {


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (Animation)
    (bindPose)
    (Maya)
    (generated)
    (Skeleton)
);


struct _MayaTokensData {
    // Types
    const MString dagPoseType{"dagPose"};
    const MString groupIdType{"groupId"};
    const MString groupPartsType{"groupParts"};
    const MString jointType{"joint"};
    const MString meshType{"mesh"};
    const MString skinClusterType{"skinCluster"};

    // Plugs, etc.
    const MString bindPose{"bindPose"};
    const MString bindPreMatrix{"bindPreMatrix"};
    const MString drawStyle{"drawStyle"};
    const MString geomMatrix{"geomMatrix"};
    const MString groupId{"groupId"};
    const MString inheritsTransform{"inheritsTransform"};
    const MString inputComponents{"inputComponents"};
    const MString input{"input"};
    const MString inputGeometry{"inputGeometry"};
    const MString inMesh{"inMesh"};
    const MString intermediateObject{"intermediateObject"};
    const MString instObjGroups{"instObjGroups"};
    const MString USD_isUsdSkeleton{"USD_isUsdSkeleton"};
    const MString matrix{"matrix"};
    const MString members{"members"};
    const MString message{"message"};
    const MString none{"none"};
    const MString normalizeWeights{"normalizeWeights"};
    const MString objectGroups{"objectGroups"};
    const MString objectGroupId{"objectGroupId"};
    const MString outputGeometry{"outputGeometry"};
    const MString outMesh{"outMesh"};
    const MString parents{"parents"};
    const MString radius{"radius"};
    const MString segmentScaleCompensate{"segmentScaleCompensate"};
    const MString skinClusterGroupId{"skinClusterGroupId"};
    const MString skinClusterGroupParts{"skinClusterGroupParts"};
    const MString Skeleton{"Skeleton"};
    const MString weightList{"weightList"};
    const MString world{"world"};
    const MString worldMatrix{"worldMatrix"};
    const MString xformMatrix{"xformMatrix"};

    // Translate/rotate/scale

    const MString translates[3] {"translateX","translateY","translateZ"};
    const MString rotates[3] {"rotateX","rotateY","rotateZ"};
    const MString scales[3] {"scaleX","scaleY","scaleZ"};
};


TfStaticData<_MayaTokensData> _MayaTokens;


/// Set keyframes on \p depNode using \p values keyed at \p times.
bool
_SetAnimPlugData(MFnDependencyNode& depNode,
                 const MString& attr,
                 MDoubleArray& values, 
                 MTimeArray& times,
                 const UsdMayaPrimReaderContext* context)
{
    MStatus status;

    MPlug plug = depNode.findPlug(attr, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    if (!plug.isKeyable()) {
        status = plug.setKeyable(true);
        CHECK_MSTATUS_AND_RETURN(status, false);
    }

    MFnAnimCurve animFn;
    MObject animObj = animFn.create(plug, nullptr, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // XXX: Why do the input arrays need to be mutable here?
    status = animFn.addKeys(&times, &values);
    CHECK_MSTATUS_AND_RETURN(status, false);
    
    if (context) {
        // Register node for undo/redo
        context->RegisterNewMayaNode(animFn.name().asChar(), animObj);
    }
    return true;
}


/// Set animation on \p transformNode.
/// The \p xforms holds transforms at each time, while the \p times
/// array holds the corresponding times.
bool
_SetTransformAnim(MFnDependencyNode& transformNode,
                  const std::vector<GfMatrix4d>& xforms,
                  MTimeArray& times,
                  const UsdMayaPrimReaderContext* context)
{
    if (xforms.size() != times.length()) {
        TF_WARN("xforms size [%zu] != times size [%du].",
                xforms.size(), times.length());
        return false;
    }
    if (xforms.empty())
        return true;

    MStatus status;

    const unsigned int numSamples = times.length();

    if (numSamples > 1) {
        MDoubleArray translates[3] = {
            MDoubleArray(numSamples),
            MDoubleArray(numSamples),   
            MDoubleArray(numSamples)
        };
        MDoubleArray rotates[3] = {
            MDoubleArray(numSamples),
            MDoubleArray(numSamples),
            MDoubleArray(numSamples)
        };
        MDoubleArray scales[3] = {
            MDoubleArray(numSamples, 1),
            MDoubleArray(numSamples, 1),
            MDoubleArray(numSamples, 1)
        };

        // Decompose all transforms.
        for (unsigned int i = 0; i < numSamples; ++i) {
            const auto& xform = xforms[i];
            GfVec3d t, r, s;
            if (UsdMayaTranslatorXformable::ConvertUsdMatrixToComponents(
                   xform, &t, &r, &s)) {
                for (int c = 0 ; c < 3; ++c) {
                    translates[c][i] = t[c];
                    rotates[c][i] = r[c];
                    scales[c][i] = s[c];
                }
            }
        }

        for (int c = 0; c < 3; ++c) {
            if (!_SetAnimPlugData(transformNode, _MayaTokens->translates[c],
                                 translates[c], times, context) ||
               !_SetAnimPlugData(transformNode, _MayaTokens->rotates[c],
                                 rotates[c], times, context) ||
               !_SetAnimPlugData(transformNode, _MayaTokens->scales[c],
                                 scales[c], times, context)) {
                return false;
            }
        }
    } else {
        const auto& xform = xforms.front();
        GfVec3d t, r, s;
        if (UsdMayaTranslatorXformable::ConvertUsdMatrixToComponents(
               xform, &t, &r, &s)) {
            for (int c = 0; c < 3; ++c) {
                if (!UsdMayaUtil::setPlugValue(
                       transformNode, _MayaTokens->translates[c], t[c]) ||
                   !UsdMayaUtil::setPlugValue(
                       transformNode, _MayaTokens->rotates[c], r[c]) ||
                   !UsdMayaUtil::setPlugValue(
                       transformNode, _MayaTokens->scales[c], s[c])) {
                    return false;
                }
            }
        }
    }
    return true;
}


void
_GetJointAnimTimeSamples(const UsdSkelSkeletonQuery& skelQuery,
                         const UsdMayaPrimReaderArgs& args,
                         std::vector<double>* times)
{
    if (!args.GetTimeInterval().IsEmpty()) {
        if (UsdSkelAnimQuery animQuery = skelQuery.GetAnimQuery()) {
            // BUG 157462: Querying time samples over an interval may be
            // incorrect at the boundaries of the interval. It's more
            // correct to use 'GetBracketingTimeSamples'. But UsdSkel is
            // waiting on alternate time-querying API before providing
            // such queries.
            animQuery.GetJointTransformTimeSamplesInInterval(
                    args.GetTimeInterval(), times);
        }
    }
    if (times->empty()) {
        // Sample at just the earliest time.
        // It's *okay* that the single value fallback is not the default time.
        times->resize(1, UsdTimeCode::EarliestTime().GetValue());
    }
}


/// Get the absolute path to \p joint, within \p containerPath.
SdfPath
_GetJointPath(const SdfPath& containerPath, const TfToken& joint)
{
    SdfPath jointPath(joint);
    if (jointPath.IsAbsolutePath()) {
        jointPath = jointPath.MakeRelativePath(SdfPath::AbsoluteRootPath());
    }
    if (!jointPath.IsEmpty()) {
        return containerPath.AppendPath(jointPath);
    }
    return SdfPath();
}


/// Create joint nodes for each joint in the joint order of \p skelQuery.
/// If successful, \p jointNodes holds the ordered set of joint nodes.
bool
_CreateJointNodes(const UsdSkelSkeletonQuery& skelQuery,
                  const SdfPath& containerPath,
                  UsdMayaPrimReaderContext* context,
                  VtArray<MObject>* jointNodes)
{
    MStatus status;
    
    VtTokenArray jointNames = skelQuery.GetJointOrder();

    const size_t numJoints = jointNames.size();

    jointNodes->resize(numJoints);
    
    // Joints are ordered so that ancestors precede descendants.
    // So we can iterate over joints in order and be assured that parent
    // joints will be created before their children.
    for (size_t i = 0; i < numJoints; ++i) {
        
        const SdfPath jointPath = _GetJointPath(containerPath, jointNames[i]);
        if (!jointPath.IsPrimPath())
            continue;

        MObject parentJoint =
            context->GetMayaNode(jointPath.GetParentPath(), true);
        if (parentJoint.isNull()) {
            TF_WARN("Could not find parent node for joint <%s>.",
                    jointPath.GetText());
            return false;
        }

        if (!UsdMayaTranslatorUtil::CreateNode(jointPath,
                                                  _MayaTokens->jointType,
                                                  parentJoint, context,
                                                  &status, &(*jointNodes)[i])) {
            return false;
        }
    }
    return true;
}


/// Set the radius of joint nodes in proportion to the average length of
/// each child bone. This uses the same scaling factor as UsdSkelImaging,
/// with the intent of trying to maintain some consistenty in the skel
/// display. But note that, whereas UsdSkelImaging produces a
/// bone per (parent,child) pair, a Maya joint has its own, distinct spherical
/// representation, so the imaging representations cannot be identical.
bool
_SetJointRadii(const UsdSkelSkeletonQuery& skelQuery,
               const VtArray<MObject>& jointNodes,
               const VtMatrix4dArray& restXforms)
{
    MStatus status;
    MFnDependencyNode jointDep;

    const size_t numJoints = jointNodes.size();

    std::vector<float> radii(numJoints, 1);
    std::vector<int> childCounts(numJoints, 0);
    for (size_t i = 0; i < numJoints; ++i) {
        const GfVec3d pivot = restXforms[i].ExtractTranslation();

        int parent = skelQuery.GetTopology().GetParent(i);
        if (parent >= 0 && static_cast<size_t>(parent) < numJoints) {
            GfVec3d parentPivot = restXforms[parent].ExtractTranslation();
            double length = (pivot - parentPivot).GetLength();

            // TODO: Scaling factor matches UsdSkelImaging, but should
            // have a common, static variable to reference.
            double radius = length*0.1;
            radii[parent] = radius;
            ++childCounts[parent];
        }
    }

    // Compute average radii for parent joints, and set resolved values.
    for (size_t i = 0; i < numJoints; ++i) {
        if (jointDep.setObject(jointNodes[i])) {
            int count = childCounts[i];
            double radius = 1.0;
            if (count > 0) {
                radius = radii[i]/count;
            } else {
                int parent = skelQuery.GetTopology().GetParent(i);
                // Leaf joint. Use the same size as the parent joint.
                if (parent >= 0 && static_cast<size_t>(parent) < numJoints) {
                    radius = radii[parent];
                }
            }
            radii[i] = radius;

            if (!UsdMayaUtil::setPlugValue(
                   jointDep, _MayaTokens->radius, radius)) {
                return false;
            }
        }
    }
    return true;
}               


/// Set various rest state properties for \p jointNodes based on the
/// state of the equivalent joints as defined in \p skelQuery.
bool
_CopyJointRestStatesFromSkel(const UsdSkelSkeletonQuery& skelQuery,
                             const VtArray<MObject>& jointNodes)
{
    const size_t numJoints = jointNodes.size();
    // Compute skel-space rest xforms to store as the bindPose of each joint.
    VtMatrix4dArray restXforms;
    if (!skelQuery.ComputeJointSkelTransforms(
           &restXforms, UsdTimeCode::Default(), /*atRest*/ true)) {
        return false;
    }

    if (!TF_VERIFY(restXforms.size() == numJoints))
        return false;

    MStatus status;
    MFnDependencyNode jointDep;

    for (size_t i = 0; i < numJoints; ++i) {
        
        if (jointDep.setObject(jointNodes[i])) {
            if (!UsdMayaUtil::setPlugMatrix(jointDep, _MayaTokens->bindPose,
                                              restXforms[i])) {
                return false;
            }

            // Scale does not inherit as expected without disabling
            // segmentScaleCompensate
            if (!UsdMayaUtil::setPlugValue(
                   jointDep, _MayaTokens->segmentScaleCompensate, false)) {
                return false;
            }
        }

        // TODO:
        // Other joint attrs that we should consider setting:
        //     objectColor,useObjectColor -- for debugging
        //     lockInfluenceWeights
        // There may be other attrs required to allow joints to be repainted.
        // Will revisit this as-needed.  
    }

    if (!_SetJointRadii(skelQuery, jointNodes, restXforms))
        return false;
    
    return true;
}


/// Apply joint animation, as computed from from \p skelQuery,
/// onto \p jointNodes.
/// If \p jointContainerIsSkeleton is true, the \p jointContainer node
/// represents the Skeleton itself, and should hold the local transform
/// anim of the Skeleton. Otherwise, the local transform of the Skeleton
/// is concatenated onto the root joints.
bool
_CopyAnimFromSkel(const UsdSkelSkeletonQuery& skelQuery,
                  const MObject& jointContainer,
                  const VtArray<MObject>& jointNodes,
                  bool jointContainerIsSkeleton,
                  const UsdMayaPrimReaderArgs& args,
                  UsdMayaPrimReaderContext* context)
{
    std::vector<double> usdTimes;
    _GetJointAnimTimeSamples(skelQuery, args, &usdTimes);
    MTimeArray mayaTimes;
    mayaTimes.setLength(usdTimes.size());
    for (size_t i = 0; i < usdTimes.size(); ++i) {
        mayaTimes[i] = usdTimes[i];
    }

    MStatus status;

    // Pre-sample the Skeleton's local transforms.
    std::vector<GfMatrix4d> skelLocalXforms(usdTimes.size());
    UsdGeomXformable::XformQuery xfQuery(skelQuery.GetSkeleton());
    for (size_t i = 0; i < usdTimes.size(); ++i) {
        if (!xfQuery.GetLocalTransformation(&skelLocalXforms[i], usdTimes[i])) {
            skelLocalXforms[i].SetIdentity();
        }
    }

    if (jointContainerIsSkeleton) {
        // The jointContainer is being used to represent the Skeleton.
        // Copy the Skeleton's local transforms onto the container.

        MFnDependencyNode skelXformDep(jointContainer, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        if (!_SetTransformAnim(skelXformDep, skelLocalXforms,
                               mayaTimes, context)) {
            return false;
        }
    }

    // Pre-sample all joint animation.
    std::vector<VtMatrix4dArray> samples(usdTimes.size());
    for (size_t i = 0; i < samples.size(); ++i) {
        if (!skelQuery.ComputeJointLocalTransforms(&samples[i], usdTimes[i])) {
            return false;
        }
        if (!jointContainerIsSkeleton) {
            // We do not have a node to receive the local transforms of the
            // Skeleton, so any local transforms on the Skeleton must be
            // concatened onto the root joints instead.
            for (size_t j = 0; j < skelQuery.GetTopology().GetNumJoints(); ++j){
                if (skelQuery.GetTopology().GetParent(j) < 0) {
                    // This is a root joint. Concat by the local skel xform.
                    samples[i][j] *= skelLocalXforms[i];
                }
            }
        }
    }

    MFnDependencyNode jointDep;

    std::vector<GfMatrix4d> xforms(samples.size());

    for (size_t jointIdx = 0; jointIdx < jointNodes.size(); ++jointIdx) {

        if (!jointDep.setObject(jointNodes[jointIdx]))
            continue;

        // Get the transforms of just this joint.
        for (size_t i = 0; i < samples.size(); ++i) {
            xforms[i] = samples[i][jointIdx];
        }

        if (!_SetTransformAnim(jointDep, xforms, mayaTimes, context))
            return false;
    }
    return true;
}


} // namespace


/* static */
bool
UsdMayaTranslatorSkel::IsUsdSkeleton(const MDagPath& joint)
{
    MFnDependencyNode jointDep(joint.node());
    MPlug plug = jointDep.findPlug(_MayaTokens->USD_isUsdSkeleton);
    if (!plug.isNull()) {
        return plug.asBool();
    }
    return false;
}


/* static */
bool
UsdMayaTranslatorSkel::IsSkelMayaGenerated(const UsdSkelSkeleton& skel)
{
    VtValue mayaData = skel.GetPrim().GetCustomDataByKey(_tokens->Maya);
    if (mayaData.IsHolding<VtDictionary>()) {
        const VtDictionary& mayaDict = mayaData.UncheckedGet<VtDictionary>();
        const VtValue* val = mayaDict.GetValueAtPath(_tokens->generated);
        if (val && val->IsHolding<bool>())
            return val->UncheckedGet<bool>();
    }
    return false;
}


/* static */
void
UsdMayaTranslatorSkel::MarkSkelAsMayaGenerated(const UsdSkelSkeleton& skel)
{
    VtValue mayaData = skel.GetPrim().GetCustomDataByKey(_tokens->Maya);

    VtDictionary newDict;
    if (mayaData.IsHolding<VtDictionary>())
        newDict = mayaData.UncheckedGet<VtDictionary>();
    newDict[_tokens->generated] = VtValue(true);
    skel.GetPrim().SetCustomDataByKey(_tokens->Maya, VtValue(newDict));
}


/* static */
bool
UsdMayaTranslatorSkel::CreateJointHierarchy(
    const UsdSkelSkeletonQuery& skelQuery,
    MObject& parentNode,
    const UsdMayaPrimReaderArgs& args,
    UsdMayaPrimReaderContext* context,
    VtArray<MObject>* joints)
{
    if (!skelQuery) {    
        TF_CODING_ERROR("'skelQuery' is invalid");
        return false;
    }
    if (!joints) {   
        TF_CODING_ERROR("'joints' is null");
        return false;
    }

    MStatus status;

    SdfPath jointContainerPath;
    bool jointContainerIsSkeleton = false;
    MObject jointContainer;
    if (IsSkelMayaGenerated(skelQuery.GetSkeleton())) {
        // If a joint hierarchy was originally exported from Maya, then
        // we do not want to add a node to represent the Skeleton in Maya,
        // because the originally joint hierarchy didn't have one. We prefer
        // to try and maintain the originally joint hierarchy.
        // Instead of creating a joint for the Skeleton, we will put joints
        // beneath the parent of the Skeleton.
        jointContainerPath = skelQuery.GetPrim().GetPath().GetParentPath();
        jointContainer = parentNode;
    } else {
        // The Skeleton did not originate from Maya. It may have multiple root
        // joints, and the Skeleton prim itself may have local transforms.
        // The best way to preserve the source data is by creating a node for
        // the Skeleton itself. For convenience, we create an additional joint
        // node to represent the Skeleton, since the current export support
        // for skeletons converts the root-most joint of a joint hierarchy
        // to a Skeleton prim.
        jointContainerPath = skelQuery.GetPrim().GetPath();
        jointContainerIsSkeleton = true;

        // Create a joint to represent thte Skeleton.
        if (!UsdMayaTranslatorUtil::CreateNode(
                jointContainerPath, _MayaTokens->jointType, parentNode,
                context, &status, &jointContainer)) {
            return false;
        }

        MFnDependencyNode skelXformJointDep(jointContainer, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        // Create an attribute to indicate to export that this joint
        // represents UsdSkelSkeleton's transform.
        MObject attrObj = MFnNumericAttribute().create(
                _MayaTokens->USD_isUsdSkeleton,
                _MayaTokens->USD_isUsdSkeleton,
                MFnNumericData::kBoolean,
                true,
                &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        status = skelXformJointDep.addAttribute(attrObj);
        CHECK_MSTATUS_AND_RETURN(status, false);

        UsdMayaUtil::setPlugValue(
                skelXformJointDep, _MayaTokens->USD_isUsdSkeleton, true);

        // Change the draw style of the extra joints so that it is not drawn.
        UsdMayaUtil::setPlugValue(skelXformJointDep, 
                                     _MayaTokens->drawStyle, 2 /*None*/);
    }

    return _CreateJointNodes(skelQuery, jointContainerPath, context, joints) &&
           _CopyJointRestStatesFromSkel(skelQuery, *joints) &&
           _CopyAnimFromSkel(skelQuery, jointContainer, *joints,
                             jointContainerIsSkeleton, args, context);
}


bool
UsdMayaTranslatorSkel::GetJoints(const UsdSkelSkeletonQuery& skelQuery,
                                    UsdMayaPrimReaderContext* context,
                                    VtArray<MObject>* joints)
{
    if (!skelQuery) {
        TF_CODING_ERROR("'skelQuery is invalid.");
        return false;
    }
    if (!joints) {
        TF_CODING_ERROR("'joints' pointer is null.");
        return false;
    }

    joints->clear();
    joints->reserve(skelQuery.GetJointOrder().size());

    // Depending on whether or not the prim has Maya:generated metadata,
    // we may have a node in Maya that represents the Skeleton, or we might
    // not. See UsdMayaTranslatorSkel::CreateJointHierarchy for a deeper
    // explanation of why there is a difference.
    SdfPath jointContainerPath;
    if (IsSkelMayaGenerated(skelQuery.GetSkeleton())) {
        jointContainerPath =
            skelQuery.GetSkeleton().GetPrim().GetPath().GetParentPath();
    } else {
        jointContainerPath = skelQuery.GetSkeleton().GetPrim().GetPath();
    }

    for (const TfToken& joint : skelQuery.GetJointOrder()) {
        const SdfPath jointPath = _GetJointPath(jointContainerPath, joint);

        MObject jointObj;
        if (jointPath.IsPrimPath()) {
            jointObj = context->GetMayaNode(jointPath, false);
        }
        joints->push_back(jointObj);
    }
    return true;
}


namespace {


SdfPath
_GetBindPosePrimPath(const SdfPath& skelPath)
{
    return skelPath.AppendChild(TfToken(skelPath.GetName() + "_bindPose"));
}


/// Create a dagPose node for the objects in \p members, whose transforms
/// are given by \p localXforms and \p worldXforms.
/// The \p parentIndices array gives the index of the parent of each member,
/// or -1 if a member has no parent.
bool
_CreateDagPose(const SdfPath& path,
               const VtArray<MObject>& members,
               const VtIntArray& parentIndices,
               const VtMatrix4dArray& localXforms,
               const VtMatrix4dArray& worldXforms,
               UsdMayaPrimReaderContext* context,
               MObject* dagPoseNode)
{
    MStatus status;
    MDGModifier dgMod;

    *dagPoseNode = dgMod.createNode(_MayaTokens->dagPoseType, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    status = dgMod.renameNode(*dagPoseNode, MString(path.GetName().c_str()));
    CHECK_MSTATUS_AND_RETURN(status, false);

    MFnDependencyNode dagPoseDep(*dagPoseNode, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    context->RegisterNewMayaNode(path.GetText(), *dagPoseNode);

    const size_t numMembers = members.size();

    MPlug membersPlug = dagPoseDep.findPlug(_MayaTokens->members, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    status = membersPlug.setNumElements(numMembers);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MPlug worldPlug = dagPoseDep.findPlug(_MayaTokens->world, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MPlug worldMatrixPlug =
        dagPoseDep.findPlug(_MayaTokens->worldMatrix, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    status = worldMatrixPlug.setNumElements(numMembers);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MPlug xformMatrixPlug =
        dagPoseDep.findPlug(_MayaTokens->xformMatrix, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    status = xformMatrixPlug.setNumElements(numMembers);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MPlug parentsPlug = dagPoseDep.findPlug(_MayaTokens->parents, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    status = parentsPlug.setNumElements(numMembers);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // Wire up per-member connections.
    MFnDependencyNode memberDep;
    for (size_t i = 0; i < numMembers; ++i) {

        status = memberDep.setObject(members[i]);
        CHECK_MSTATUS_AND_RETURN(status, false);

        // Connect members[i].message -> dagPose.members[i]
        MPlug memberMessagePlug =
            memberDep.findPlug(_MayaTokens->message, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        status = dgMod.connect(memberMessagePlug,
                               membersPlug.elementByLogicalIndex(i));
        CHECK_MSTATUS_AND_RETURN(status, false);

        int parentIdx = parentIndices[i];

        MPlug parentsI = parentsPlug.elementByLogicalIndex(i);

        if (parentIdx >= 0 && static_cast<size_t>(parentIdx) < numMembers) {
            // Connect dagPose.members[parent] -> dagPose.parents[child]
            MPlug parentMemberPlug =
                membersPlug.elementByLogicalIndex(parentIdx);

            status = dgMod.connect(parentMemberPlug, parentsI);
            CHECK_MSTATUS_AND_RETURN(status, false);
        } else {
            // Connect bindPose.world -> bindPose.parents[i]
            status = dgMod.connect(worldPlug, parentsI);
            CHECK_MSTATUS_AND_RETURN(status, false);
        }

        // Set worldMatrix[i] = worldXforms[i]
        MPlug worldMatrixI = worldMatrixPlug.elementByLogicalIndex(i);
        if (!UsdMayaUtil::setPlugMatrix(worldXforms[i], worldMatrixI)) {
            return false;
        }

        // Set xformMatrix[i] = localXforms[i]
        MPlug xformMatrixI = xformMatrixPlug.elementByLogicalIndex(i);
        if (!UsdMayaUtil::setPlugMatrix(localXforms[i], xformMatrixI)) {
            return false;
        }
    }

    status = dgMod.doIt();
    CHECK_MSTATUS_AND_RETURN(status, false);

    return UsdMayaUtil::setPlugValue(
        dagPoseDep, _MayaTokens->bindPose, true);
}


} // namespace


bool
UsdMayaTranslatorSkel::CreateBindPose(
    const UsdSkelSkeletonQuery& skelQuery,
    const VtArray<MObject>& joints,
    UsdMayaPrimReaderContext* context,
    MObject* bindPoseNode)
{
    if (!skelQuery) {
        TF_CODING_ERROR("'skelQuery' is invalid.");
        return false;
    }
    if (!bindPoseNode) {
        TF_CODING_ERROR("'bindPoseNode' is null.");
        return false;
    }

    VtMatrix4dArray localXforms, worldXforms;
    if (!skelQuery.ComputeJointLocalTransforms(
            &localXforms, UsdTimeCode::Default(), /*atRest*/ true)) {
        TF_WARN("%s -- Failed reading rest transforms. No dagPose "
                "will be created for the Skeleton.",
                skelQuery.GetPrim().GetPath().GetText());
        return false;
    }
    if (!skelQuery.GetJointWorldBindTransforms(&worldXforms)) {
        TF_WARN("%s -- Failed reading bind transforms. No dagPose "
                "will be created for the Skeleton.",
                skelQuery.GetPrim().GetPath().GetText());
        return false;
    }

    SdfPath path = _GetBindPosePrimPath(skelQuery.GetPrim().GetPath());

    return _CreateDagPose(path, joints,
                          skelQuery.GetTopology().GetParentIndices(),
                          localXforms, worldXforms, context, bindPoseNode);
}


MObject
UsdMayaTranslatorSkel::GetBindPose(const UsdSkelSkeletonQuery& skelQuery,
                                      UsdMayaPrimReaderContext* context)
{
    return context->GetMayaNode(
        _GetBindPosePrimPath(skelQuery.GetPrim().GetPath()), false);
}


namespace {


bool
_SetVaryingJointInfluences(const MFnMesh& meshFn,
                           const MObject& skinCluster,
                           const VtArray<MObject>& joints,
                           const VtIntArray& indices,
                           const VtFloatArray& weights,
                           int numInfluencesPerPoint,
                           unsigned int numPoints)
{
    if (joints.empty())
        return true;

    MStatus status;

    MDagPath dagPath;
    status = meshFn.getPath(dagPath);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MFnSkinCluster skinClusterFn(skinCluster, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    const unsigned int numJoints = static_cast<unsigned int>(joints.size());

    // Compute a vertex-ordered weight arrays. Weights are stored as:
    //   vert_0_joint_0 ... vert_0_joint_n ... vert_n_joint_0 ... vert_n_joint_n
    MDoubleArray vertOrderedWeights(numPoints*numJoints, 0.0f); 
    for (unsigned int pt = 0; pt < numPoints; ++pt) {
        for (int c = 0; c < numInfluencesPerPoint; ++c) {
            int jointIdx = indices[pt*numInfluencesPerPoint+c];
            if (jointIdx >= 0 
               && static_cast<unsigned int>(jointIdx) < numJoints) {
                float w = weights[pt*numInfluencesPerPoint+c];
                // There may be multiple influences referencing the same joint
                // for this point. eg., 'unweighted' points are assigned
                // index 0 and weight 0. Sum the weight contributions to ensure
                // that we properly account for this.
                vertOrderedWeights[pt*numJoints + jointIdx] += w;
            }
        }
    }

    MIntArray influenceIndices(numJoints);
    for (unsigned int i = 0; i < numJoints; ++i) {
        influenceIndices[i] = i;
    }

    // Set all weights in one batch 
    MFnSingleIndexedComponent components;
    components.create(MFn::kMeshVertComponent);
    components.setCompleteData(numPoints);

    // XXX: Note that weights are expected to be pre-normalized in USD.
    // In order to faithfully transfer our source data, we do not perform
    // any normalization on import. Maya's weight normalization also seems
    // finicky w.r.t. precision, and tends to throw warnings even when the
    // weights have been properly normalized, so this also saves us from 
    // unnecessary warning spam.
    
    // If the 'normalizeWeights' attribute of the skinCluster is set to
    // 'interactive' -- and by default, it is -- then weights are still
    // normalized even if we set normalize=false on
    // MFnSkinCluster::normalize(). This fact is unfortunately not made
    // clear in the MFnSkinCluster documentation... 
    // Temporarily set the attr to 'none'

    MPlug normalizeWeights =
        skinClusterFn.findPlug(_MayaTokens->normalizeWeights, &status);
    MString initialNormalizeWeights;
    if (!normalizeWeights.isNull()) {
        initialNormalizeWeights = normalizeWeights.asString();
        normalizeWeights.setString(_MayaTokens->none);
    }

    status = skinClusterFn.setObject(skinCluster);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // Apply the weights. Note that this fails with kInvalidParameter
    // if the influenceIndices are invalid. Validity is based on the
    // set of joints wired up to the skinCluster.
    status = skinClusterFn.setWeights(dagPath, components.object(),
                                      influenceIndices, vertOrderedWeights,
                                      /*normalize*/ false);
    CHECK_MSTATUS_AND_RETURN(status, false);


    // Reset the normalization flag to its previous value.
    if (!normalizeWeights.isNull()) {
        normalizeWeights.setString(initialNormalizeWeights);
    }


    return true;
}


bool
_ComputeAndSetJointInfluences(const UsdSkelSkinningQuery& skinningQuery,
                              const VtArray<MObject>& joints,
                              const MObject& skinCluster,
                              const MObject& shapeToSkin)
{
    MStatus status;

    MFnMesh meshFn(shapeToSkin, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    unsigned int numPoints = meshFn.numVertices(&status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    VtIntArray indices;
    VtFloatArray weights;
    if (skinningQuery.ComputeVaryingJointInfluences(
           numPoints, &indices, &weights)) {

        return _SetVaryingJointInfluences(
            meshFn, skinCluster, joints, indices, weights,
            skinningQuery.GetNumInfluencesPerComponent(), numPoints);
    }
    return false;
}


/// Create a copy of mesh \p inputMesh beneath \p parent,
/// for use as an input mesh for deformers.
bool
_CreateRestMesh(const MObject& inputMesh,
                const MObject& parent,
                MObject* restMesh)
{
    MStatus status;
    MFnMesh meshFn(inputMesh, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    
    *restMesh = meshFn.copy(inputMesh, parent, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // Determine a new name for the rest mesh, and rename the copy.
    static const MString restSuffix("_rest");
    MString restMeshName = meshFn.name() + restSuffix;
    MDGModifier dgMod;
    status = dgMod.renameNode(*restMesh, restMeshName);
    CHECK_MSTATUS_AND_RETURN(status, false);

    status = dgMod.doIt();
    CHECK_MSTATUS_AND_RETURN(status, false);

    return UsdMayaUtil::setPlugValue(
        *restMesh, _MayaTokens->intermediateObject, true);
}


/// Clear any incoming connections on \p plug.
bool
_ClearIncomingConnections(MPlug& plug)
{
    MPlugArray connections;
    if (plug.connectedTo(connections, /*asDst*/ true, /*asSrc*/ false)) {
        MStatus status;
        MDGModifier dgMod;
        for (unsigned int i = 0; i < connections.length(); ++i) {
            status = dgMod.disconnect(plug, connections[i]);
            CHECK_MSTATUS_AND_RETURN(status, false);
        }
        status = dgMod.doIt();
        CHECK_MSTATUS_AND_RETURN(status, false);
    }
    return true;
}


/// Configure the transform node of a skinned object.
bool
_ConfigureSkinnedObjectTransform(const UsdSkelSkinningQuery& skinningQuery,
                                 const MObject& transform)
{
    MStatus status;
    MFnDependencyNode transformDep(transform, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // Make sure transforms are not ineherited.
    // Otherwise we get a double transform when a transform ancestor
    // affects both this object and the joints that drive the skinned object.
    if (!UsdMayaUtil::setPlugValue(
           transformDep, _MayaTokens->inheritsTransform, false)) {
        return false;
    }

    // The transform needs to be set to the geomBindTransform.
    GfVec3d t, r, s;
    if (UsdMayaTranslatorXformable::ConvertUsdMatrixToComponents(
           skinningQuery.GetGeomBindTransform(), &t, &r, &s)) {

        for (const auto& pair : {std::make_pair(t, _MayaTokens->translates),
                                 std::make_pair(r, _MayaTokens->rotates),
                                 std::make_pair(s, _MayaTokens->scales)}) {

            for (int c = 0; c < 3; ++c) {
                MPlug plug = transformDep.findPlug(pair.second[c], &status);
                CHECK_MSTATUS_AND_RETURN(status, false);

                // Before setting each plug, make sure there are no connections.
                // Usd import may have already wired up some connections
                // (eg., animation channels)
                if (!_ClearIncomingConnections(plug))
                    return false;

                status = plug.setValue(pair.first[c]);
                CHECK_MSTATUS_AND_RETURN(status, false);
            }
        }
    }
    
    return true;
}


} // namespace


/* static */
bool
UsdMayaTranslatorSkel::CreateSkinCluster(
    const UsdSkelSkeletonQuery& skelQuery,
    const UsdSkelSkinningQuery& skinningQuery,
    const VtArray<MObject>& joints,
    const UsdPrim& primToSkin,
    const UsdMayaPrimReaderArgs& args,
    UsdMayaPrimReaderContext* context,
    const MObject& bindPose)
{
    MStatus status;

    if (!skelQuery) {
        TF_CODING_ERROR("'skelQuery' is invalid");
        return false;
    }
    if (!skinningQuery) {
        TF_CODING_ERROR("'skinningQuery is invalid");
    }
    if (!primToSkin) {
        TF_CODING_ERROR("'primToSkin 'is invalid"); 
        return false;
    }

    // Resolve the input mesh.
    MObject objToSkin = context->GetMayaNode(primToSkin.GetPath(), false);
    if (objToSkin.isNull()) {
        // XXX: Not an error (import may have chosen to exclude the prim).
        return true;
    }

    MDagPath shapeDagPath;
    status = MDagPath::getAPathTo(objToSkin, shapeDagPath);
    CHECK_MSTATUS_AND_RETURN(status, false);
    status = shapeDagPath.extendToShape();
    CHECK_MSTATUS_AND_RETURN(status, false);

    MObject shapeToSkin = shapeDagPath.node(&status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    
    if (shapeToSkin.apiType() != MFn::kMesh) {
        // USD considers this prim skinnable, but in Maya, we currently only
        // know how to skin meshes. Skip it.
        return true;
    }

    MObject parentTransform = shapeDagPath.transform(&status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MObject restMesh;
    if (!_CreateRestMesh(shapeToSkin, parentTransform, &restMesh)) {
        return false;
    }

    if (!_ConfigureSkinnedObjectTransform(skinningQuery, parentTransform)){ 
        return false;
    }

    MDGModifier dgMod;

    MObject skinCluster =
        dgMod.createNode(_MayaTokens->skinClusterType, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    std::string skinClusterName = 
        TfStringPrintf("skinCluster_%s", primToSkin.GetName().GetText());
    status = dgMod.renameNode(skinCluster, MString(skinClusterName.c_str()));
                              
    CHECK_MSTATUS_AND_RETURN(status, false);

    MObject groupId = dgMod.createNode(_MayaTokens->groupIdType, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    status = dgMod.renameNode(groupId, _MayaTokens->skinClusterGroupId);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MObject groupParts = dgMod.createNode(_MayaTokens->groupPartsType, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    status = dgMod.renameNode(groupParts, _MayaTokens->skinClusterGroupParts);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MFnDependencyNode groupIdDep(groupId, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MFnDependencyNode groupPartsDep(groupParts, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MFnDependencyNode restMeshDep(restMesh, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MFnDependencyNode shapeToSkinDep(shapeToSkin, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MFnDependencyNode skinClusterDep(skinCluster, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // Register all new nodes on the context.
    context->RegisterNewMayaNode(restMeshDep.name().asChar(), restMesh);
    context->RegisterNewMayaNode(skinClusterDep.name().asChar(), skinCluster);
    context->RegisterNewMayaNode(groupIdDep.name().asChar(), groupId);
    context->RegisterNewMayaNode(groupPartsDep.name().asChar(), groupParts);

    // set groupParts.inputComponents = vtx[*]
    {
        MFnSingleIndexedComponent componentsFn;
        MObject vertComponents = componentsFn.create(MFn::kMeshVertComponent);
        componentsFn.setComplete(true);

        MFnComponentListData componentListFn;
        MObject componentList = componentListFn.create();
        status = componentListFn.add(vertComponents);
        CHECK_MSTATUS_AND_RETURN(status, false);

        MPlug inputComponentsPlug =
            groupPartsDep.findPlug(_MayaTokens->inputComponents, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        status = inputComponentsPlug.setValue(componentList);
        CHECK_MSTATUS_AND_RETURN(status, false);
    }
    
    // Connect restMesh.outMesh -> groupParts->inputGeometry
    {
        MPlug restMeshOutMesh =
            restMeshDep.findPlug(_MayaTokens->outMesh, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);
        MPlug groupPartsInputGeometry =
            groupPartsDep.findPlug(_MayaTokens->inputGeometry, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);
        
        status = dgMod.connect(restMeshOutMesh, groupPartsInputGeometry);
        CHECK_MSTATUS_AND_RETURN(status, false);
    }
    
    MPlug groupIdGroupId = groupIdDep.findPlug(_MayaTokens->groupId, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // Connect groupId.groupId -> groupParts.groupId
    {
        MPlug groupPartsGroupId =
            groupPartsDep.findPlug(_MayaTokens->groupId, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        status = dgMod.connect(groupIdGroupId, groupPartsGroupId);
        CHECK_MSTATUS_AND_RETURN(status, false);
    }

    // connect groupId.groupId ->
    //     shapeToSkin.instObjGroups[0].objectGroups[0].objectGroupId
    {
        MPlug instObjGroups =
            shapeToSkinDep.findPlug(_MayaTokens->instObjGroups, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        MPlug instObjGroups0 =
            instObjGroups.elementByLogicalIndex(0, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        MPlug objectGroups =
            UsdMayaUtil::FindChildPlugByName(instObjGroups0,
                                                _MayaTokens->objectGroups);
        //number of objectGroups
        unsigned int count = objectGroups.numElements();
        MPlug objectGroups0 =
            objectGroups.elementByLogicalIndex(count, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        MPlug objectGroupId =
            UsdMayaUtil::FindChildPlugByName(objectGroups0,
                                                _MayaTokens->objectGroupId);
        
        status = dgMod.connect(groupIdGroupId, objectGroupId);
    }

    MPlug skinClusterInput =
        skinClusterDep.findPlug(_MayaTokens->input, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);
    status = skinClusterInput.setNumElements(1);
    CHECK_MSTATUS_AND_RETURN(status, false);

    MPlug skinClusterInput0 =
        skinClusterInput.elementByLogicalIndex(0, &status);
    CHECK_MSTATUS_AND_RETURN(status, false);

    // groupParts.outputGeometry -> skinCluster.input[0].inputGeometry
    {
        MPlug skinClusterInputGeometry =
            UsdMayaUtil::FindChildPlugByName(skinClusterInput0,
                                                _MayaTokens->inputGeometry);

        MPlug groupPartsOutputGeometry =
            groupPartsDep.findPlug(_MayaTokens->outputGeometry, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        status = dgMod.connect(groupPartsOutputGeometry,
                               skinClusterInputGeometry);
        CHECK_MSTATUS_AND_RETURN(status, false);
    }

    // Connect groupId.groupId -> skinCluster.input[0].groupId
    {
        MPlug skinClusterGroupId =
            UsdMayaUtil::FindChildPlugByName(skinClusterInput0,
                                                _MayaTokens->groupId);

        status = dgMod.connect(groupIdGroupId, skinClusterGroupId);
        CHECK_MSTATUS_AND_RETURN(status, false);
    }

    // Connect skinCluster.outputGeometry[0] -> shapeToSkin.inMesh
    {
        MPlug skinClusterOutputGeometry =
            skinClusterDep.findPlug(_MayaTokens->outputGeometry, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        MPlug skinClusterOutputGeometry0 =
            skinClusterOutputGeometry.elementByLogicalIndex(0, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        MPlug shapeToSkinInMesh =
            shapeToSkinDep.findPlug(_MayaTokens->inMesh, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        status = dgMod.connect(skinClusterOutputGeometry0, shapeToSkinInMesh);
        CHECK_MSTATUS_AND_RETURN(status, false);
    }
    
    // Connect joints[i].worldMatrix[0] -> skinCluster.matrix[i]
    // Set skinCluster.bindPreMatrix[i] = inv(jointWorldBindXforms[i])
    {
        VtMatrix4dArray bindXforms;
        if (!skelQuery.GetJointWorldBindTransforms(&bindXforms)) {
            return false;
        }

        MPlug skinClusterMatrix =
            skinClusterDep.findPlug(_MayaTokens->matrix, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);
        status = skinClusterMatrix.setNumElements(joints.size());
        CHECK_MSTATUS_AND_RETURN(status, false);

        MPlug bindPreMatrix =
            skinClusterDep.findPlug(_MayaTokens->bindPreMatrix, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);
        status = bindPreMatrix.setNumElements(joints.size());
        CHECK_MSTATUS_AND_RETURN(status, false);

        MFnDependencyNode jointDep;
        for (size_t i = 0; i < joints.size(); ++i) {
            status = jointDep.setObject(joints[i]);
            CHECK_MSTATUS_AND_RETURN(status, false);

            MPlug jointWorldMatrix =
                jointDep.findPlug(_MayaTokens->worldMatrix);
            CHECK_MSTATUS_AND_RETURN(status, false);

            MPlug jointWorldMatrix0 =
                jointWorldMatrix.elementByLogicalIndex(0, &status);
            CHECK_MSTATUS_AND_RETURN(status, false);

            MPlug skinClusterMatrixI =
                skinClusterMatrix.elementByLogicalIndex(i, &status);
            CHECK_MSTATUS_AND_RETURN(status, false);

            status = dgMod.connect(jointWorldMatrix0, skinClusterMatrixI);
            CHECK_MSTATUS_AND_RETURN(status, false);

            MPlug bindPreMatrixI =
                bindPreMatrix.elementByLogicalIndex(i, &status);
            CHECK_MSTATUS_AND_RETURN(status, false);
            if (!UsdMayaUtil::setPlugMatrix(
                   bindXforms[i].GetInverse(), bindPreMatrixI)) {
                return false;
            }
        }
    }

    // Connect dagPose.message -> skinCluster.bindPose, if any bind pose exists.
    if (!bindPose.isNull()) {
        MFnDependencyNode bindPoseDep(bindPose, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);
        MPlug bindPoseMessage =
            bindPoseDep.findPlug(_MayaTokens->message, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);
        MPlug skinClusterBindPose =
            skinClusterDep.findPlug(_MayaTokens->bindPose, &status);
        CHECK_MSTATUS_AND_RETURN(status, false);

        status = dgMod.connect(bindPoseMessage, skinClusterBindPose);
        CHECK_MSTATUS_AND_RETURN(status, false);
    }

    status = dgMod.doIt();
    CHECK_MSTATUS_AND_RETURN(status, false);

    if (!UsdMayaUtil::setPlugMatrix(skinClusterDep, _MayaTokens->geomMatrix,
                                       skinningQuery.GetGeomBindTransform())) {
        return false;
    }

    return _ComputeAndSetJointInfluences(skinningQuery, joints,
                                         skinCluster, shapeToSkin);
}


PXR_NAMESPACE_CLOSE_SCOPE
