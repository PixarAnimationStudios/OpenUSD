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
#include "pxrUsdTranslators/meshWriter.h"

#include "pxrUsdTranslators/jointWriter.h"

#include "usdMaya/translatorSkel.h"
#include "usdMaya/translatorUtil.h"
#include "usdMaya/writeJobContext.h"

#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/usd/usdSkel/bindingAPI.h"
#include "pxr/usd/usdSkel/root.h"
#include "pxr/usd/usdSkel/utils.h"

#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MDoubleArray.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MMatrix.h>

#include <ostream>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((skelJointIndices, "skel:jointIndices"))
    ((skelJointWeights, "skel:jointWeights"))
    ((skelGeomBindTransform, "skel:geomBindTransform"))
);


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
        TF_WARN(
            "Multiple skinClusters upstream of '%s'; using closest "
            "skinCluster '%s'",
            dagPath.fullPathName().asChar(),
            MFnDependencyNode(skinClusterObj).name().asChar());
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
        TF_WARN(
                "%s is not a mesh; unable to obtain input mesh for %s",
                inputGeometry.name().asChar(), skinCluster.name().asChar());
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
            if (!GfIsClose(weight, 0.0, 1e-8)) {
                (*usdJointIndices)[outputOffset] = i;
                (*usdJointWeights)[outputOffset] = weight;
                outputOffset++;
            }
        }
    }
    return maxInfluenceCount;
}


/// Check if a skinned primitive has an unsupported post-deformation
/// transformation. These transformations aren't represented in UsdSkel.
///
/// When a SkinCluster deforms meshes, the results are transformed back into the
/// space of the mesh. The output is then plugged up to the final mesh, which
/// has its own transform. Usually this change in transformation -- from putting
/// the deformation results back into the space of the source mesh, and then
/// transforming the result by the output mesh -- share the same transformation,
/// such that there's no overall change in transformation. This is not always
/// the case. In particular, 'broken' rigs may have the transformations out of
/// sync (the result of which being that the deformed meshes drift away from the
/// skeleton that drives them).
///
/// We have no nice way of encoding a mesh-specific post-deformation transform
/// in UsdSkel, and so can only try and warn the user.
static void
_WarnForPostDeformationTransform(const SdfPath& path,
                                 const MDagPath& deformedMeshDag,
                                 const MFnSkinCluster& skinCluster)
{
    MStatus status;

    MMatrix deformedMeshWorldXf = deformedMeshDag.inclusiveMatrix(&status);
    if (!status)
        return;

    MMatrix bindPreMatrix;
    if (UsdMayaUtil::getPlugMatrix(
            skinCluster, "bindPreMatrix", &bindPreMatrix)) {

        if (!GfIsClose(GfMatrix4d(deformedMeshWorldXf.matrix),
                       GfMatrix4d(bindPreMatrix.matrix), 1e-5)) {
            TF_WARN("Mesh <%s> appears to have a non-identity post-deformation "
                    "transform (the 'bindPreMatrix' property of the skinCluster "
                    "does not match the inclusive matrix of the deformed mesh). "
                    "The resulting skinning in USD may be incorrect.",
                    path.GetText());
        }
    }
}


/// Compute the geomBindTransform for a mesh using \p skinCluster.
static bool
_GetGeomBindTransform(const MFnSkinCluster& skinCluster,
                      GfMatrix4d* geomBindXf)
{
    MMatrix geomWorldRestXf;
    if (!UsdMayaUtil::getPlugMatrix(
            skinCluster, "geomMatrix", &geomWorldRestXf)) {
        // All skinClusters should have geomMatrix, but if not...
        TF_RUNTIME_ERROR(
                "Couldn't read geomMatrix from skinCluster '%s'",
                skinCluster.name().asChar());
        return false;
    }

    *geomBindXf = GfMatrix4d(geomWorldRestXf.matrix);
    return true;
}


/// Compute and write joint influences.
static bool
_WriteJointInfluences(const MFnSkinCluster& skinCluster,
                      const MFnMesh& inMesh,
                      const UsdSkelBindingAPI& binding)
{
    // The data in the skinCluster is essentially already in the same format
    // as UsdSkel expects, but we're going to compress it by only outputting
    // the nonzero weights.
    VtIntArray jointIndices;
    VtFloatArray jointWeights;
    int maxInfluenceCount = _GetCompressedSkinWeights(
        inMesh, skinCluster, &jointIndices, &jointWeights);

    if (maxInfluenceCount <= 0)
        return false;

    UsdSkelSortInfluences(&jointIndices, &jointWeights, maxInfluenceCount);

    UsdGeomPrimvar indicesPrimvar =
        binding.CreateJointIndicesPrimvar(false, maxInfluenceCount);
    indicesPrimvar.Set(jointIndices);

    UsdGeomPrimvar weightsPrimvar =
        binding.CreateJointWeightsPrimvar(false, maxInfluenceCount);
    weightsPrimvar.Set(jointWeights);

    return true;
}


static bool
_WriteJointOrder(const MDagPath& rootJoint,
                 const std::vector<MDagPath>& jointDagPaths,
                 const UsdSkelBindingAPI& binding,
                 const bool stripNamespaces)
{
    // Get joint name tokens how PxrUsdTranslators_JointWriter would generate
    // them. We don't need to check that they actually exist.
    VtTokenArray jointNames = PxrUsdTranslators_JointWriter::GetJointNames(
        jointDagPaths, rootJoint, stripNamespaces);

    binding.CreateJointsAttr().Set(jointNames);
    return true;
}


MObject
PxrUsdTranslators_MeshWriter::writeSkinningData(UsdGeomMesh& primSchema)
{
    const TfToken& exportSkin = _GetExportArgs().exportSkin;
    if (exportSkin != UsdMayaJobExportArgsTokens->auto_ &&
        exportSkin != UsdMayaJobExportArgsTokens->explicit_) {
        return MObject();
    }

    // Figure out if we even have a skin cluster in the first place.
    MObject skinClusterObj = _GetSkinCluster(GetDagPath());
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
    if (exportSkin == UsdMayaJobExportArgsTokens->explicit_ &&
            !UsdSkelRoot::Find(primSchema.GetPrim())) {
        return MObject();
    }

    // Get all influences and find the rootmost joint.
    MDagPathArray jointDagPathArr;
    if (!skinCluster.influenceObjects(jointDagPathArr)) {
        return MObject();
    }

    std::vector<MDagPath> jointDagPaths(jointDagPathArr.length());
    for (unsigned int i = 0; i < jointDagPathArr.length(); ++i) {
        jointDagPaths[i] = jointDagPathArr[i];
    }

    MDagPath rootJoint = _GetRootJoint(jointDagPaths);
    if (!rootJoint.isValid()) {
        // No roots or multiple roots!
        // XXX: This is a somewhat arbitrary restriction due to the way that
        // we currently export skeletons in PxrUsdTranslators_JointWriter. We treat an
        // entire joint hierarchy rooted at a single joint as a single skeleton,
        // so when binding the mesh to a skeleton, we have to make sure that
        // we're only binding to a single skeleton.
        //
        // This restrction is largely a consequence of UsdSkel encoding joint
        // transforms in 'skeleton space': We need something that defines a rest
        // (or bind) transform, since otherwise transforming into skeleton space
        // is undefined for the rest pose.
        return MObject();
    }

    // Write everything to USD once we know that we have OK data.
    const UsdSkelBindingAPI bindingAPI = UsdMayaTranslatorUtil
        ::GetAPISchemaForAuthoring<UsdSkelBindingAPI>(primSchema.GetPrim());

    if (_WriteJointInfluences(skinCluster, inMesh, bindingAPI)) {
        _WriteJointOrder(rootJoint, jointDagPaths, bindingAPI,
                         _GetExportArgs().stripNamespaces);
    }

    GfMatrix4d geomBindTransform;
    if (_GetGeomBindTransform(skinCluster,&geomBindTransform)) {
        _SetAttribute(bindingAPI.CreateGeomBindTransformAttr(),
                      &geomBindTransform);
    }

    _WarnForPostDeformationTransform(GetUsdPath(), GetDagPath(), skinCluster);

    const SdfPath skelPath =
        PxrUsdTranslators_JointWriter::GetSkeletonPath(
            rootJoint, _GetExportArgs().stripNamespaces);

    // Export will create a Skeleton at the location corresponding to
    // the root joint. Configure this mesh to be bound to the same skel.
    bindingAPI.CreateSkeletonRel().SetTargets({skelPath});

    // Add all skel primvars to the exclude set.
    // We don't want later processing to stomp on any of our data.
    _excludeColorSets.insert({_tokens->skelJointIndices,
                              _tokens->skelJointWeights,
                              _tokens->skelGeomBindTransform});

    // Mark the bindings for post processing.
    _writeJobCtx.MarkSkelBindings(
        primSchema.GetPrim().GetPath(),
        skelPath, exportSkin);

    return inMeshObj;
}


PXR_NAMESPACE_CLOSE_SCOPE
