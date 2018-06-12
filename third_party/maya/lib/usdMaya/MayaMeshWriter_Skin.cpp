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
#include "usdMaya/MayaMeshWriter.h"
#include "usdMaya/MayaSkeletonWriter.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdSkel/bindingAPI.h"
#include "pxr/usd/usdSkel/root.h"
#include "pxr/usd/usdSkel/utils.h"

#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MTransformationMatrix.h>
#include <maya/MMatrix.h>
#include <maya/MQuaternion.h>

#include <ostream>

PXR_NAMESPACE_OPEN_SCOPE

/// Gets the closest upstream skin cluster for the mesh at the given dag path.
/// Warns if there is more than one skin cluster.
MObject
_GetSkinCluster(const MDagPath& dagPath) {
    MObject currentDagObject = dagPath.node();

    MItDependencyGraph itDG(currentDagObject, MFn::kSkinClusterFilter,
            MItDependencyGraph::kUpstream);
    if (itDG.isDone()) {
        // No skin clusters.
        return MObject::kNullObj;
    }

    MObject skinClusterObj = itDG.currentItem();
    // If there's another skin cluster, then we have multiple skin clusters.
    if (itDG.next() && !itDG.isDone()) {
        MGlobal::displayWarning(TfStringPrintf(
            "Multiple skinClusters upstream of '%s'; using closest "
            "skinCluster '%s'",
            dagPath.fullPathName().asChar(),
            MFnDependencyNode(skinClusterObj).name().asChar()).c_str());
    }

    return skinClusterObj;
}

/// Finds the input (pre-skin) mesh for the given skin cluster.
/// Warning, do not use MFnSkinCluster::getInputGeometry; it will give you
/// the wrong results (or rather, not the ones we want here).
/// Given the following (simplified) DG:
///     pCubeShape1Orig.worldMesh[0] -> tweak1.inputGeometry
///     tweak1.outputGeometry[0] -> skinCluster1.input[0].inputGeometry
///     skinCluster1.outputGeometry[0] -> pCubeShape1.inMesh
/// Requesting the input geometry for skinCluster1 will give you the mesh
///     pCubeShape1Orig
/// and not
///     tweak1.outputGeometry
/// as desired for this use case.
/// For best results, read skinCluster1.input[0].inputGeometry directly.
/// Note that the Maya documentation states "a skinCluster node can deform
/// only a single geometry" [1] so we are free to ignore any input geometries
/// after the first one.
/// [1]: http://help.autodesk.com/view/MAYAUL/2018/ENU/?guid=__cpp_ref_class_m_fn_skin_cluster_html
MObject
_GetInputMesh(const MFnSkinCluster& skinCluster) {
    MStatus status;
    MPlug inputPlug = skinCluster.findPlug("input", true, &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject());

    MPlug inputPlug0 = inputPlug.elementByLogicalIndex(0, &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject());

    MPlug inputGeometry = inputPlug0.child(0, &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject());

    MObject inputGeometryObj =
            inputGeometry.asMObject(MDGContext::fsNormal, &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject());

    if (!inputGeometryObj.hasFn(MFn::kMesh)) {
        MGlobal::displayWarning(TfStringPrintf(
                "%s is not a mesh; unable to obtain input mesh for %s",
                inputGeometry.name().asChar(), skinCluster.name().asChar())
                .c_str());
        return MObject();
    }

    return inputGeometryObj;
}

/// Gets the unique root joint of the given joint dag paths, or an invalid
/// MDagPath if there is no such unique joint (i.e. the joints form two
/// separate joint hierarchies). Currently, we don't support skin bound to
/// multiple joint hierarchies.
static MDagPath
_GetRootJoint(const std::vector<MDagPath>& jointDagPaths)
{
    MDagPath uniqueRoot;

    for (const MDagPath& dagPath : jointDagPaths) {
        // Find the roostmost joint in my ancestor chain.
        // (It's OK if there are intermediary non-joints; just skip them.)
        MDagPath curPath = dagPath;
        MDagPath rootmostJoint = dagPath;
        while (curPath.length() > 0) {
            curPath.pop();
            if (curPath.hasFn(MFn::kJoint)) {
                rootmostJoint = curPath;
            }
        }

        // All root joints must match.
        if (uniqueRoot.isValid()) {
            if (!(uniqueRoot == rootmostJoint)) {
                return MDagPath();
            }
        }
        else {
            uniqueRoot = rootmostJoint;
        }
    }

    return uniqueRoot;
}

/// Gets skin weights, and compresses them into the form expected by
/// UsdSkelBindingAPI, which allows us to omit zero-weight influences from the
/// joint weights list.
static int
_GetCompressedSkinWeights(
    const MFnMesh& mesh,
    const MFnSkinCluster& skinCluster,
    VtIntArray* usdJointIndices,
    VtFloatArray* usdJointWeights)
{
    // Get the single output dag path from the skin cluster.
    // Note that we can't get the dag path from the mesh because it's the input
    // mesh (and also may not have a dag path).
    MDagPath outputDagPath;
    MStatus status = skinCluster.getPathAtIndex(0, outputDagPath);
    if (!status) {
        TF_CODING_ERROR(
                "Calling code should have guaranteed that skinCluster "
                "'%s' has at least one output",
                skinCluster.name().asChar());
        return 0;
    }

    // Get all of the weights from the skinCluster in one batch.
    unsigned int numVertices = mesh.numVertices();
    MFnSingleIndexedComponent components;
    components.create(MFn::kMeshVertComponent);
    components.setCompleteData(numVertices);
    MDoubleArray weights;
    unsigned int numInfluences;
    skinCluster.getWeights(
            outputDagPath, components.object(), weights, numInfluences);

    // Determine how many influence/weight "slots" we actually need per point.
    // For example, if there are the joints /a, /a/b, and /a/c, but each point
    // only has non-zero weighting for a single joint, then we only need one
    // slot instead of three.
    int maxInfluenceCount = 0;
    for (unsigned int vert = 0; vert < numVertices; ++vert) {
        // Looping through each vertex.
        const unsigned int offset = vert * numInfluences;
        int influenceCount = 0;
        for (unsigned int i = 0; i < numInfluences; ++i) {
            // Looping through each weight for vertex.
            if (weights[offset + i] != 0.0) {
                influenceCount++;
            }
        }
        maxInfluenceCount = std::max(maxInfluenceCount, influenceCount);
    }

    usdJointIndices->assign(maxInfluenceCount * numVertices, 0);
    usdJointWeights->assign(maxInfluenceCount * numVertices, 0.0);
    for (unsigned int vert = 0; vert < numVertices; ++vert) {
        // Looping through each vertex.
        const unsigned int inputOffset = vert * numInfluences;
        int outputOffset = vert * maxInfluenceCount;
        for (unsigned int i = 0; i < numInfluences; ++i) {
            // Looping through each weight for vertex.
            float weight = weights[inputOffset + i];
            if (weight != 0.0) {
                (*usdJointIndices)[outputOffset] = i;
                (*usdJointWeights)[outputOffset] = weight;
                outputOffset++;
            }
        }
    }

    return maxInfluenceCount;
}

/// Finds the rootmost ancestor of the prim at startPath that is an Xform
/// or SkelRoot type prim.
static UsdPrim
_FindRootmostXformOrSkelRoot(const UsdStagePtr& stage, const SdfPath& startPath)
{
    UsdPrim currentPrim = stage->GetPrimAtPath(startPath.GetParentPath());
    UsdPrim rootmost;
    while (currentPrim) {
        if (currentPrim.IsA<UsdGeomXform>()) {
            rootmost = currentPrim;
        }
        else if (currentPrim.IsA<UsdSkelRoot>()) {
            rootmost = currentPrim;
        }
        currentPrim = currentPrim.GetParent();
    }

    return rootmost;
}

/// Changes an ancestor prim's typename to SkelRoot if necessary and allowed
/// by the exportSkin job arg.
/// \p outMadeSkelRoot must be non-null; it will be set to indicate whether any
/// auto-renaming actually occured (true) or whether there was already a
/// SkelRoot, so no renaming was necessary (false).
/// Coding error if the prim has no SkelRoot ancestor and no prim can be turned
/// into a SkelRoot (these prims should have been skipped for export).
/// Maya error if the existing SkelRoot ancestor is nested inside another
/// SkelRoot.
static SdfPath
_VerifyOrMakeSkelRoot(
    const UsdStagePtr& stage,
    const SdfPath& meshPath,
    const TfToken& exportSkinConfig,
    bool* outMadeSkelRoot)
{
    // Only try to auto-rename to SkelRoot if we're not already a
    // descendant of one. Otherwise, verify that the user tagged it in a sane
    // way.
    if (UsdSkelRoot root = UsdSkelRoot::Find(stage->GetPrimAtPath(meshPath))) {
        // Verify that the SkelRoot isn't nested in another SkelRoot.
        // This is necessary because UsdSkel doesn't handle nested skel roots
        // very well currently; this restriction may be loosened in the future.
        if (UsdSkelRoot root2 = UsdSkelRoot::Find(root.GetPrim().GetParent())) {
            MGlobal::displayError(TfStringPrintf("The SkelRoot <%s> is nested "
                    "inside another SkelRoot <%s>. This might cause unexpected "
                    "behavior. ",
                    root.GetPath().GetText(),
                    root2.GetPath().GetText()).c_str());
            *outMadeSkelRoot = false;
            return SdfPath();
        }
        else {
            *outMadeSkelRoot = false;
            return root.GetPath();
        }
    } else {
        // If auto-generating the SkelRoot, find the rootmost
        // UsdGeomXform and turn it into a SkelRoot.
        // XXX: It might be good to also consider model hierarchy here, and not
        // go past our ancestor component when trying to generate the SkelRoot.
        // (Example: in a scene with /World, /World/Char_1, /World/Char_2, we
        // might want SkelRoots to stop at Char_1 and Char_2.) Unfortunately,
        // the current structure precludes us from accessing model hierarchy
        // here.
        if (exportSkinConfig == PxrUsdExportJobArgsTokens->auto_) {
            if (UsdPrim root =
                    _FindRootmostXformOrSkelRoot(stage, meshPath)) {
                UsdSkelRoot::Define(stage, root.GetPath());
                *outMadeSkelRoot = true;
                return root.GetPath();
            }
            else {
                TF_CODING_ERROR(
                        "No UsdGeomXform ancestor of <%s>; it should have "
                        "been skipped for skel data export",
                        meshPath.GetText());
                *outMadeSkelRoot = false;
                return SdfPath();
            }
        }
        else {
            TF_CODING_ERROR(
                    "No existing SkelRoot ancestor and not auto-generating "
                    "SkelRoot ancestor for <%s>; it should have been skipped "
                    "for skel data export",
                    meshPath.GetText());
            *outMadeSkelRoot = false;
            return SdfPath();
        }
    }
}

MObject
MayaMeshWriter::writeSkinningData(UsdGeomMesh& primSchema)
{
    const TfToken& exportSkin = mWriteJobCtx.getArgs().exportSkin;
    if (exportSkin != PxrUsdExportJobArgsTokens->auto_ &&
            exportSkin != PxrUsdExportJobArgsTokens->explicit_) {
        return MObject();
    }

    // Figure out if we even have a skin cluster in the first place.
    MObject skinClusterObj = _GetSkinCluster(getDagPath());
    if (skinClusterObj.isNull()) {
        return MObject();
    }
    MFnSkinCluster skinCluster(skinClusterObj);

    MObject inMeshObj = _GetInputMesh(skinCluster);
    if (inMeshObj.isNull()) {
        return MObject();
    }
    MFnMesh inMesh(inMeshObj);

    // At this point, we know we have a skin cluster.
    // If exportSkin=explicit and we're not under a SkelRoot, then silently skip
    // (it's what the user asked for, after all).
    if (exportSkin == PxrUsdExportJobArgsTokens->explicit_ &&
            !UsdSkelRoot::Find(primSchema.GetPrim())) {
        return MObject();
    }

    // If exportSkin=auto, we need to make sure there's a UsdGeomXform
    // ancestor that we can turn into a SkelRoot. Otherwise, *error* and return.
    if (exportSkin == PxrUsdExportJobArgsTokens->auto_ &&
            !_FindRootmostXformOrSkelRoot(
                getUsdStage(), primSchema.GetPath())) {
        MGlobal::displayError(TfStringPrintf(
                "Cannot automatically make SkelRoot above "
                "<%s> because it has no Xform ancestors. Try grouping it "
                "under a Maya transform node.",
                primSchema.GetPath().GetText()).c_str());
        return MObject();
    }

    // Get all influences and find the rootmost joint.
    MDagPathArray jointDagPathArr;
    if (!skinCluster.influenceObjects(jointDagPathArr)) {
        return MObject();
    }

    std::vector<MDagPath> jointDagPaths;
    for (unsigned int i = 0; i < jointDagPathArr.length(); ++i) {
        jointDagPaths.push_back(jointDagPathArr[i]);
    }

    MDagPath rootJoint = _GetRootJoint(jointDagPaths);
    if (!rootJoint.isValid()) {
        // No roots or multiple roots!
        // XXX: This is a somewhat arbitrary restriction due to the way that
        // we currently export skeletons in MayaSkeletonWriter. We treat an
        // entire joint hierarchy rooted at a single joint as a single skeleton,
        // so when binding the mesh to a skeleton, we have to make sure that
        // we're only binding to a single skeleton. This might change if we
        // ever generate multiple-root skeletons.
        return MObject();
    }

    // Get joint name tokens how MayaSkeletonWriter would generate them.
    // We don't need to check that they actually exist.
    VtTokenArray jointNames = MayaSkeletonWriter::GetJointNames(
            jointDagPaths, rootJoint, getArgs().stripNamespaces);

    // The data in the skinCluster is essentially already in the same format 
    // as UsdSkel expects, but we're going to compress it by only outputting
    // the nonzero weights.
    VtIntArray jointIndices;
    VtFloatArray jointWeights;
    int maxInfluenceCount = _GetCompressedSkinWeights(
            inMesh, skinCluster, &jointIndices, &jointWeights);
    if (maxInfluenceCount == 0) {
        return MObject();
    }
    UsdSkelSortInfluences(&jointIndices, &jointWeights, maxInfluenceCount);

    // The skeleton space is the exclusive transform of the root joint, i.e.
    // the transform of its parent.
    GfMatrix4d skeletonSpace(rootJoint.exclusiveMatrix().matrix);

    // Obtain geometry bind transform, which moves the mesh into skeleton
    // space. Note that we use the root joint space as skeleton space.
    MMatrix geomMatrixWorld;
    if (!PxrUsdMayaUtil::getPlugMatrix(
            skinCluster, "geomMatrix", &geomMatrixWorld)) {
        // All skinClusters should have geomMatrix, but if not...
        MGlobal::displayError(TfStringPrintf(
                "Couldn't read geomMatrix from skinCluster '%s'",
                skinCluster.name().asChar()).c_str());
        return MObject();
    }
    GfMatrix4d geomBindTransform =
            skeletonSpace.GetInverse() * GfMatrix4d(geomMatrixWorld.matrix);

    // Write everything to USD once we know that we have OK data.
    const UsdSkelBindingAPI bindingAPI(primSchema);
    _SetAttribute(
            bindingAPI.CreateJointIndicesPrimvar(false, maxInfluenceCount),
            &jointIndices);
    _SetAttribute(
            bindingAPI.CreateJointWeightsPrimvar(false, maxInfluenceCount),
            &jointWeights);
    _SetAttribute(bindingAPI.CreateGeomBindTransformAttr(), 
            &geomBindTransform);

    bool madeSkelRoot;
    const SdfPath skelRootPath = _VerifyOrMakeSkelRoot(
            getUsdStage(), getUsdPath(),
            mWriteJobCtx.getArgs().exportSkin, &madeSkelRoot);
    mWriteJobCtx.getSkelBindingsWriter().MarkBinding(
            getUsdPath(), skelRootPath, rootJoint, madeSkelRoot);
    return inMeshObj;
}


PXR_NAMESPACE_CLOSE_SCOPE
