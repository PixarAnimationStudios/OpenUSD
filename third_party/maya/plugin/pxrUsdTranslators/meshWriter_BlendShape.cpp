//
// Copyright 2019 Pixar
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

#include "pxrUsdTranslators/meshWriter.h"
#include "pxrUsdTranslators/jointWriter.h"

#include <maya/MFloatArray.h>
#include <maya/MFnAttribute.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnBlendShapeDeformer.h>
#include <maya/MItDependencyGraph.h>

#include "usdMaya/primWriter.h"
#include "usdMaya/translatorUtil.h"

#include "pxr/base/tf/token.h"
#include "pxr/usd/usdSkel/blendShape.h"
#include "pxr/usd/usdSkel/bindingAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Gets the closest upstream skin cluster for the mesh at the given dag path.
/// Warns if there is more than one skin cluster.
static MObject
_GetSkinCluster(const MDagPath& dagPath)
{
    MStatus status = MS::kSuccess;

    MObject currentDagObject = dagPath.node(&status);
    CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);

    MItDependencyGraph itDG(currentDagObject, MFn::kSkinClusterFilter,
            MItDependencyGraph::kUpstream, MItDependencyGraph::kDepthFirst,
            MItDependencyGraph::kNodeLevel, &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);

    if (itDG.isDone()) {
        // No skin clusters.
        return MObject::kNullObj;
    }

    MObject skinClusterObj = itDG.currentItem(&status);
    CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);

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

static const std::pair<MObject, unsigned int>
INVALID_OBJECT_AND_INDEX(MObject::kNullObj, -1);

/// Gets the closest upstream blend shape deformer for the mesh at the given
/// dag path. Warns if there is more than one blend shape deformer.
static std::pair<MObject, unsigned int>
_GetBlendShapeDeformer(MObject& currentDagObject)
{
    MStatus status = MS::kSuccess;

    if (!currentDagObject.hasFn(MFn::kDependencyNode)) {
        return INVALID_OBJECT_AND_INDEX;
    }

    MItDependencyGraph itDG(currentDagObject, MFn::kBlendShape,
            MItDependencyGraph::kUpstream, MItDependencyGraph::kDepthFirst,
            MItDependencyGraph::kPlugLevel, &status);
    CHECK_MSTATUS_AND_RETURN(status, INVALID_OBJECT_AND_INDEX);

    if (itDG.isDone()) {
        // No blend shape deformers.
        return INVALID_OBJECT_AND_INDEX;
    }

    MPlug blendShapePlug = itDG.thisPlug(&status);
    CHECK_MSTATUS_AND_RETURN(status, INVALID_OBJECT_AND_INDEX);

    unsigned int outputIndex = blendShapePlug.logicalIndex(&status);
    CHECK_MSTATUS_AND_RETURN(status, INVALID_OBJECT_AND_INDEX);

    MObject blendShapeObj = blendShapePlug.node(&status);
    CHECK_MSTATUS_AND_RETURN(status, INVALID_OBJECT_AND_INDEX);

    // If there's another blend shape deformer, then we have multiple blend
    // shape deformers.
    if (itDG.next() && !itDG.isDone()) {
        TF_WARN(
            "Multiple blendShape deformers upstream of '%s'; using closest "
            "blendShape deformer '%s'",
            MFnDependencyNode(currentDagObject).name().asChar(),
            MFnDependencyNode(blendShapeObj).name().asChar());
    }

    return std::make_pair(blendShapeObj, outputIndex);
}

static TfToken
_GetTargetNameToken(MFnBlendShapeDeformer& blendShape,
                    const MFnMesh& targetMesh, unsigned int index)
{
    MStatus status;

    MPlug weightsPlug = blendShape.findPlug("weight", true, &status);
    CHECK_MSTATUS_AND_RETURN(status, TfToken());

    MPlug weightPlug = weightsPlug.elementByLogicalIndex(index, &status);
    CHECK_MSTATUS_AND_RETURN(status, TfToken());

    MString targetName = blendShape.plugsAlias(weightPlug, &status);
    CHECK_MSTATUS_AND_RETURN(status, TfToken());

    if (targetName.length()) {
        return TfToken(targetName.asChar());
    }

    return TfToken(targetMesh.name().asChar());
}

static float _CalculateTargetWeightValue(unsigned int targetItemIndex)
{
    // caluate weight value from targetItemIndex:
    // index = wt * 1000 + 5000
    // targetWeightValue = (index - 5000) / 1000
    return (targetItemIndex - 5000.0f) / 1000.0f;
}

static VtArray<GfVec3f>
_CalculateTargetOffsets(MFnBlendShapeDeformer& deformer,
                        const VtArray<GfVec3f>& basePoints,
                        MFnMesh& targetMesh,
                        unsigned int weightIndex,
                        unsigned int targetItemIndex)
{
    MStatus status;

    // assume deformer has been prepped for evaluation:
    // weights all set to 0
    // envelope set to 1.

    float targetWeightValue = _CalculateTargetWeightValue(targetItemIndex);

    // set weight[weightIndex] to targetWeightValue
    status = deformer.setWeight(weightIndex, targetWeightValue);
    CHECK_MSTATUS_AND_RETURN(status, VtArray<GfVec3f>());

    const float* targetRawPoints = targetMesh.getRawPoints(&status);
    CHECK_MSTATUS_AND_RETURN(status, VtArray<GfVec3f>());

    VtArray<GfVec3f> offsets(targetMesh.numVertices(&status));
    CHECK_MSTATUS_AND_RETURN(status, VtArray<GfVec3f>());

    assert(offsets.size() == basePoints.size());
    for (unsigned int i = 0; i < offsets.size(); i++) {
        GfVec3f tmp(targetRawPoints[i * 3 + 0],
                    targetRawPoints[i * 3 + 1],
                    targetRawPoints[i * 3 + 2]);
        offsets[i] = tmp - basePoints[i];
    }

    deformer.setWeight(weightIndex, 0.0f);

    return offsets;
}

// Class to manage blend shape deformer edits in a RAII fashion so that the
// deformer is always cleaned up properly after the edits.
class DeformerEditScope
{
public:
    DeformerEditScope(MFnBlendShapeDeformer& deformer);
    ~DeformerEditScope();

    bool IsValid() const { return !MFAIL(_status); }

private:
    MFnBlendShapeDeformer& _deformer;
    MStatus _status;

    // connections

    // weight values
    MIntArray _weightIndices;
    MFloatArray _weights;

    // envelope value
    float _envelope;

    MStatus PushState();
    MStatus PopState();
};

DeformerEditScope::DeformerEditScope(MFnBlendShapeDeformer& deformer)
    : _deformer(deformer),
      _status(MS::kSuccess)
{
    _status = PushState();
}

DeformerEditScope::~DeformerEditScope()
{
    _status = PopState();
}

MStatus DeformerEditScope::PushState()
{
    MStatus status = MS::kSuccess;

    _deformer.weightIndexList(_weightIndices);

    // FIXME: save any connections and disconnect.

    _weights.setLength(_weightIndices.length());
    for (unsigned int i = 0; i < _weights.length(); i++)
    {
        _weights[i] = _deformer.weight(_weightIndices[i], &status);
        CHECK_MSTATUS_AND_RETURN_IT(status);
    }

    _envelope = _deformer.envelope(&status);
    CHECK_MSTATUS_AND_RETURN_IT(status);

    return status;
}

MStatus DeformerEditScope::PopState()
{
    _deformer.setEnvelope(_envelope);

    // weight index list should be identical

    for (unsigned int i = 0; i < _weights.length(); i++)
    {
        _deformer.setWeight(_weightIndices[i], _weights[i]);
    }

    // FIXME: restore connections.
    return MS::kSuccess;
}

static
MObject _GetDeformerBaseMesh(MFnBlendShapeDeformer& deformer, unsigned int index)
{
    MStatus status = MS::kSuccess;

    MPlug inputPlug = deformer.findPlug("input", true, &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);

    MPlug inputPlug0 = inputPlug.elementByLogicalIndex(index, &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);

    MPlug inputGeometry = inputPlug0.child(0, &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);

    return inputGeometry.asMObject();
}

static
MObject _GetTargetObject(MFnBlendShapeDeformer& deformer, unsigned int baseIndex,
                         unsigned int weightIndex, unsigned int targetItemIndex)
{
    MStatus status = MS::kSuccess;

    MPlug inputTargets = deformer.findPlug("inputTarget", true, &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);

    MPlug inputTarget =
        inputTargets.elementByLogicalIndex(baseIndex, &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);

    MPlug inputTargetGroups = inputTarget.child(0, &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);

    MPlug inputTargetGroup =
        inputTargetGroups.elementByLogicalIndex(weightIndex, &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);

    MPlug inputTargetItems = inputTargetGroup.child(0, &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);

    MPlug inputTargetItem =
        inputTargetItems.elementByLogicalIndex(targetItemIndex, &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);

    MPlug inputGeomTarget = inputTargetItem.child(0, &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);

    MPlug source = inputGeomTarget.source(&status);
    CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);

    return source.node();
}

static
MStatus _CleanName(MString& dirtyName)
{
    return dirtyName.substitute(".", "_");
}

static
MString _GetInbetweenTargetName(MFnBlendShapeDeformer& deformer,
                                unsigned int weightIndex,
                                unsigned int targetItemIndex)
{
    MStatus status = MS::kSuccess;

    MPlug infoGroups = deformer.findPlug("inbetweenInfoGroup", true, &status);
    CHECK_MSTATUS_AND_RETURN(status, MString());

    MPlug infoGroup = infoGroups.elementByLogicalIndex(weightIndex, &status);
    CHECK_MSTATUS_AND_RETURN(status, MString());

    MPlug infos = infoGroup.child(0, &status);
    CHECK_MSTATUS_AND_RETURN(status, MString());

    MPlug info = infos.elementByLogicalIndex(targetItemIndex, &status);
    CHECK_MSTATUS_AND_RETURN(status, MString());

    MPlug targetName = info.child(1, &status);
    CHECK_MSTATUS_AND_RETURN(status, MString());

    MString result = targetName.asString(&status);
    CHECK_MSTATUS_AND_RETURN(status, MString());

    status = _CleanName(result);
    CHECK_MSTATUS_AND_RETURN(status, MString());

    return result;
}

static const int WEIGHT_1_INDEX = 6000;

static SdfPath
_GetBlendShapePath(const UsdGeomMesh& primSchema, const TfToken name,
                   MFnBlendShapeDeformer& deformer,
                   unsigned int baseIndex, unsigned int weightIndex)
{
    MStatus status = MS::kSuccess;

    TfToken targetToken(name);

    MObject targetObject =
        _GetTargetObject(deformer, baseIndex, weightIndex, WEIGHT_1_INDEX);

    if (targetObject.hasFn(MFn::kMesh))
    {
        MFnMesh targetMesh(targetObject, &status);
        CHECK_MSTATUS_AND_RETURN(status, SdfPath());

        targetToken = TfToken(targetMesh.name().asChar());
    }

    return primSchema.GetPath().AppendChild(targetToken);
}

MObject PxrUsdTranslators_MeshWriter::writeBlendShapeData(
    UsdGeomMesh& primSchema)
{
    MStatus status = MS::kSuccess;

    const TfToken& exportBlendShapes = _GetExportArgs().exportBlendShapes;
    if (exportBlendShapes != UsdMayaJobExportArgsTokens->auto_ &&
        exportBlendShapes != UsdMayaJobExportArgsTokens->explicit_) {
        return MObject::kNullObj;
    }

    MObject finalObj = _GetSkinCluster(GetDagPath());
    if (finalObj.isNull()) {
        finalObj = GetMayaObject();
    }

    auto deformerAndBaseIndex = _GetBlendShapeDeformer(finalObj);
    MObject deformerObj = deformerAndBaseIndex.first;
    unsigned int baseIndex = deformerAndBaseIndex.second;

    if (deformerObj.isNull()) {
        return MObject::kNullObj;
    }
    MFnBlendShapeDeformer deformer(deformerObj, &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);

    MObjectArray baseObjects;
    status = deformer.getBaseObjects(baseObjects);
    CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);

    MObject baseMeshObj = baseObjects[baseIndex];
    MFnMesh baseMesh(baseMeshObj, &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);

    MIntArray weightIndices;
    status = deformer.weightIndexList(weightIndices);
    CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);

    SdfPathVector blendShapePaths(weightIndices.length());

    VtTokenArray blendShapeNames(weightIndices.length());

    // push deformer state so we can make edits. The state will be restored
    // when this goes out of scope.
    DeformerEditScope deformerScope(deformer);
    if (!deformerScope.IsValid()) {
        return MObject::kNullObj;
    }

    // zero out weights and set to 1 in preparation for evaluation
    for (unsigned int i = 0; i < weightIndices.length(); i++)
    {
        status = deformer.setWeight(weightIndices[i], 0.0f);
        CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);
    }
    status = deformer.setEnvelope(1.0f);
    CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);

    // capture current (undeformed) mesh points.
    const float* baseRawPoints = baseMesh.getRawPoints(&status);
    CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);

    VtArray<GfVec3f> basePoints(baseMesh.numVertices());
    memcpy(basePoints.data(), baseRawPoints,
           basePoints.size() * sizeof(GfVec3f));

    for (unsigned int i = 0; i < weightIndices.length(); i++)
    {
        blendShapeNames[i] =
            _GetTargetNameToken(deformer, baseMesh, weightIndices[i]);

        blendShapePaths[i] = _GetBlendShapePath(
            primSchema, blendShapeNames[i],
            deformer, baseIndex, weightIndices[i]
        );

        UsdSkelBlendShape blendShape =
            UsdSkelBlendShape::Define(GetUsdStage(), blendShapePaths[i]);

        // calculate index 6000 offsets first since this corresponds to a
        // weight 1.0 target, which is the default blend shape offset in USD.
        unsigned int weightIndex = static_cast<unsigned int>(weightIndices[i]);

        VtArray<GfVec3f> offsets =
            _CalculateTargetOffsets(deformer, basePoints, baseMesh,
                                    weightIndex, WEIGHT_1_INDEX);

        _SetAttribute(blendShape.CreateOffsetsAttr(), &offsets);

        // now iterate over rest of the target items to fill in in-betweens.

        MIntArray targetItemIndices;
        status = deformer.targetItemIndexList(
            weightIndex, baseMeshObj, targetItemIndices
        );
        CHECK_MSTATUS_AND_RETURN(status, MObject::kNullObj);

        for (unsigned int j = 0; j < targetItemIndices.length(); ++j)
        {
            unsigned int itemIndex =
                static_cast<unsigned int>(targetItemIndices[j]);

            // skip weight 1.0 (target index 6000) since it's already encoded
            // in the blendshape offset.
            if (itemIndex == WEIGHT_1_INDEX) {
                continue;
            }

            TfToken nameToken;
            MString name = _GetInbetweenTargetName(deformer, weightIndex,
                                                  itemIndex);
            if (name.length()) {
                nameToken = TfToken(name.asChar());
            } else {
                std::stringstream nameStream;
                nameStream << "inbetween_" << itemIndex;
                nameToken = TfToken(nameStream.str().c_str());
            }

            UsdSkelInbetweenShape inbetween(
                blendShape.CreateInbetween(nameToken)
            );
            offsets = _CalculateTargetOffsets(
                deformer, basePoints, baseMesh, weightIndex, itemIndex
            );
            inbetween.SetOffsets(offsets);
            inbetween.SetWeight(_CalculateTargetWeightValue(itemIndex));
        }

        // FIXME: add normal offsets if normals are defined on the meshes.
    }

    const UsdSkelBindingAPI bindingAPI = UsdMayaTranslatorUtil
        ::GetAPISchemaForAuthoring<UsdSkelBindingAPI>(primSchema.GetPrim());

    _SetAttribute(bindingAPI.CreateBlendShapesAttr(), &blendShapeNames);

    bindingAPI.CreateBlendShapeTargetsRel().SetTargets(blendShapePaths);

    // Mark the bindings for post processing.
    _writeJobCtx.MarkSkelBindings(
        primSchema.GetPrim().GetPath(),
        SdfPath(), exportBlendShapes);

    return _GetDeformerBaseMesh(deformer, baseIndex);
}

PXR_NAMESPACE_CLOSE_SCOPE
