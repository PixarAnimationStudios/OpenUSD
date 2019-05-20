//
// Copyright 2016 Pixar
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
#include "usdMaya/shadingModeExporterContext.h"

#include "usdMaya/jobArgs.h"
#include "usdMaya/util.h"
#include "usdMaya/writeJobContext.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/types.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/scope.h"
#include "pxr/usd/usdGeom/subset.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/materialBindingAPI.h"
#include "pxr/usd/usdShade/shader.h"

#include <maya/MDagPath.h>
#include <maya/MDagPathArray.h>
#include <maya/MDGContext.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MNamespace.h>
#include <maya/MObject.h>
#include <maya/MObjectArray.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <string>
#include <utility>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (surfaceShader)
    (volumeShader)
    (displacementShader)
);


UsdMayaShadingModeExportContext::UsdMayaShadingModeExportContext(
        const MObject& shadingEngine,
        UsdMayaWriteJobContext& writeJobContext,
        const UsdMayaUtil::MDagPathMap<SdfPath>& dagPathToUsdMap) :
    _shadingEngine(shadingEngine),
    _stage(writeJobContext.GetUsdStage()),
    _dagPathToUsdMap(dagPathToUsdMap),
    _writeJobContext(writeJobContext),
    _surfaceShaderPlugName(_tokens->surfaceShader),
    _volumeShaderPlugName(_tokens->volumeShader),
    _displacementShaderPlugName(_tokens->displacementShader)
{
    if (GetExportArgs().dagPaths.empty()) {
        // if none specified, push back '/' which encompasses all
        _bindableRoots.insert(SdfPath::AbsoluteRootPath());
    }
    else {
        TF_FOR_ALL(bindableRootIter, GetExportArgs().dagPaths) {
            const MDagPath& bindableRootDagPath = *bindableRootIter;

            auto iter = _dagPathToUsdMap.find(bindableRootDagPath);
            if (iter == _dagPathToUsdMap.end()) {
                // Geometry w/ this material bound doesn't seem to exist in USD.
                continue;
            }

            SdfPath usdPath = iter->second;

            // If usdModelRootOverridePath is not empty, replace the root
            // namespace with it.
            if (!GetExportArgs().usdModelRootOverridePath.IsEmpty()) {
                usdPath = usdPath.ReplacePrefix(
                    usdPath.GetPrefixes()[0],
                    GetExportArgs().usdModelRootOverridePath);
            }

            _bindableRoots.insert(usdPath);
        }
    }
}

void
UsdMayaShadingModeExportContext::SetSurfaceShaderPlugName(
        const TfToken& surfaceShaderPlugName)
{
    _surfaceShaderPlugName = surfaceShaderPlugName;
}

void
UsdMayaShadingModeExportContext::SetVolumeShaderPlugName(
        const TfToken& volumeShaderPlugName)
{
    _volumeShaderPlugName = volumeShaderPlugName;
}

void
UsdMayaShadingModeExportContext::SetDisplacementShaderPlugName(
        const TfToken& displacementShaderPlugName)
{
    _displacementShaderPlugName = displacementShaderPlugName;
}

static
MPlug
_GetShaderPlugFromShadingEngine(
        const MObject& shadingEngine,
        const TfToken& shaderPlugName)
{
    MStatus status;

    const MFnDependencyNode seDepNodeFn(shadingEngine, &status);
    if (status != MS::kSuccess) {
        return MPlug();
    }

    const MPlug shaderPlug =
        seDepNodeFn.findPlug(
            shaderPlugName.GetText(),
            /* wantNetworkedPlug = */ true,
            &status);
    if (status != MS::kSuccess) {
        return MPlug();
    }

    return shaderPlug;
}

static
MObject
_GetShaderFromShadingEngine(
        const MObject& shadingEngine,
        const TfToken& shaderPlugName)
{
    MStatus status;

    const MPlug shaderPlug =
        _GetShaderPlugFromShadingEngine(shadingEngine, shaderPlugName);
    if (shaderPlug.isNull()) {
        return MObject();
    }

    MObject shaderObj =
#if MAYA_API_VERSION >= 20180000
        shaderPlug.asMObject(&status);
#else
        shaderPlug.asMObject(MDGContext::fsNormal, &status);
#endif
    if (status != MS::kSuccess || shaderObj.isNull()) {
        return MObject();
    }

    return UsdMayaUtil::GetConnected(shaderPlug).node();
}

MPlug
UsdMayaShadingModeExportContext::GetSurfaceShaderPlug() const
{
    return _GetShaderPlugFromShadingEngine(
        _shadingEngine,
        _surfaceShaderPlugName);
}

MObject
UsdMayaShadingModeExportContext::GetSurfaceShader() const
{
    return _GetShaderFromShadingEngine(
        _shadingEngine,
        _surfaceShaderPlugName);
}

MPlug
UsdMayaShadingModeExportContext::GetVolumeShaderPlug() const
{
    return _GetShaderPlugFromShadingEngine(
        _shadingEngine,
        _volumeShaderPlugName);
}

MObject
UsdMayaShadingModeExportContext::GetVolumeShader() const
{
    return _GetShaderFromShadingEngine(
        _shadingEngine,
        _volumeShaderPlugName);
}

MPlug
UsdMayaShadingModeExportContext::GetDisplacementShaderPlug() const
{
    return _GetShaderPlugFromShadingEngine(
        _shadingEngine,
        _displacementShaderPlugName);
}

MObject
UsdMayaShadingModeExportContext::GetDisplacementShader() const
{
    return _GetShaderFromShadingEngine(
        _shadingEngine,
        _displacementShaderPlugName);
}

UsdMayaShadingModeExportContext::AssignmentVector
UsdMayaShadingModeExportContext::GetAssignments() const
{
    AssignmentVector ret;

    MStatus status;
    MFnDependencyNode seDepNode(_shadingEngine, &status);
    if (!status) {
        return ret;
    }

    MPlug dsmPlug = seDepNode.findPlug("dagSetMembers", true, &status);
    if (!status) {
        return ret;
    }

    SdfPathSet seenBoundPrimPaths;
    for (unsigned int i = 0; i < dsmPlug.numConnectedElements(); i++) {
        MPlug dsmElemPlug(dsmPlug.connectionByPhysicalIndex(i));
        MStatus status = MS::kFailure;
        MPlug connectedPlug = UsdMayaUtil::GetConnected(dsmElemPlug);

        // Maya connects shader bindings for instances based on element indices
        // of the instObjGroups[x] or instObjGroups[x].objectGroups[y] plugs.
        // The instance number is the index of instObjGroups[x]; the face set
        // (if any) is the index of objectGroups[y].
        if (connectedPlug.isElement() && connectedPlug.array().isChild()) {
            // connectedPlug is instObjGroups[x].objectGroups[y] (or its
            // equivalent), so go up two levels to get to instObjGroups[x].
            MPlug objectGroups = connectedPlug.array();
            MPlug instObjGroupsElem = objectGroups.parent();
            connectedPlug = instObjGroupsElem;
        }
        // connectedPlug should be instObjGroups[x] here. Get the index.
        unsigned int instanceNumber = connectedPlug.logicalIndex();

        // Get the correct DAG path for this instance number.
        MDagPathArray allDagPaths;
        MDagPath::getAllPathsTo(connectedPlug.node(), allDagPaths);
        if (instanceNumber >= allDagPaths.length()) {
            TF_RUNTIME_ERROR(
                    "Instance number is %d (from plug '%s') but node only has "
                    "%d paths",
                    instanceNumber,
                    connectedPlug.name().asChar(),
                    allDagPaths.length());
            continue;
        }

        MDagPath dagPath = allDagPaths[instanceNumber];
        TF_VERIFY(dagPath.instanceNumber() == instanceNumber);
        MFnDagNode dagNode(dagPath, &status);
        if (!status) {
            continue;
        }

        auto iter = _dagPathToUsdMap.find(dagPath);
        if (iter == _dagPathToUsdMap.end()) {
            // Geometry w/ this material bound doesn't seem to exist in USD.
            continue;
        }
        SdfPath usdPath = iter->second;

        // If usdModelRootOverridePath is not empty, replace the
        // root namespace with it.
        if (!GetExportArgs().usdModelRootOverridePath.IsEmpty()) {
            usdPath = usdPath.ReplacePrefix(
                usdPath.GetPrefixes()[0],
                GetExportArgs().usdModelRootOverridePath);
        }

        // If this path has already been processed, skip it.
        if (!seenBoundPrimPaths.insert(usdPath).second) {
            continue;
        }

        // If the bound prim's path is not below a bindable root, skip it.
        if (SdfPathFindLongestPrefix(
                _bindableRoots.begin(),
                _bindableRoots.end(),
                usdPath) == _bindableRoots.end()) {
            continue;
        }

        MObjectArray sgObjs, compObjs;
        status = dagNode.getConnectedSetsAndMembers(
            instanceNumber,
            sgObjs,
            compObjs,
            true);
        if (status != MS::kSuccess) {
            continue;
        }

        for (unsigned int j = 0u; j < sgObjs.length(); ++j) {
            // If the shading group isn't the one we're interested in, skip it.
            if (sgObjs[j] != _shadingEngine) {
                continue;
            }

            VtIntArray faceIndices;
            if (!compObjs[j].isNull()) {
                MItMeshPolygon faceIt(dagPath, compObjs[j]);
                faceIndices.reserve(faceIt.count());
                for (faceIt.reset(); !faceIt.isDone(); faceIt.next()) {
                    faceIndices.push_back(faceIt.index());
                }
            }
            ret.push_back(std::make_pair(usdPath, faceIndices));
        }
    }
    return ret;
}

static
UsdPrim
_GetMaterialParent(
        const UsdStageRefPtr& stage,
        const TfToken& materialsScopeName,
        const UsdMayaShadingModeExportContext::AssignmentVector& assignments)
{
    SdfPath commonAncestor;
    TF_FOR_ALL(iter, assignments) {
        const SdfPath& assn = iter->first;
        if (stage->GetPrimAtPath(assn)) {
            if (commonAncestor.IsEmpty()) {
                commonAncestor = assn;
            }
            else {
                commonAncestor = commonAncestor.GetCommonPrefix(assn);
            }
        }
    }

    if (commonAncestor.IsEmpty()) {
        return UsdPrim();
    }

    if (commonAncestor == SdfPath::AbsoluteRootPath()) {
        return stage->GetPseudoRoot();
    }

    SdfPath shaderExportLocation = commonAncestor;
    while (!shaderExportLocation.IsRootPrimPath()) {
        shaderExportLocation = shaderExportLocation.GetParentPath();
    }

    shaderExportLocation = shaderExportLocation.AppendChild(materialsScopeName);

    return UsdGeomScope::Define(stage, shaderExportLocation).GetPrim();
}

/// Determines if the \p path would be an instance proxy path on \p stage if
/// it existed, i.e., if any of its ancestor paths are instances.
/// (Note that if \p path itself is an instance, then it is _not_ an instance
/// proxy path.)
static
bool
_IsInstanceProxyPath(const UsdStageRefPtr& stage, const SdfPath& path)
{
    for (const SdfPath& prefix : path.GetParentPath().GetPrefixes()) {
        if (const UsdPrim prim = stage->GetPrimAtPath(prefix)) {
            if (prim.IsInstance()) {
                return true;
            }
        }
    }

    return false;
}

/// Ensures that a prim exists at \p path on \p stage and that the prim is
/// neither an instance nor an instance proxy.
static
UsdPrim
_UninstancePrim(
        const UsdStageRefPtr& stage,
        const SdfPath& path,
        const std::string& reason)
{
    bool didUninstance = false;
    for (const SdfPath& prefix : path.GetPrefixes()) {
        if (const UsdPrim prim = stage->GetPrimAtPath(prefix)) {
            if (prim.IsInstance()) {
                prim.SetInstanceable(false);
                didUninstance = true;
            }
        }
        else {
            break;
        }
    }

    if (didUninstance) {
        TF_WARN("Uninstanced <%s> (and ancestors) because: %s",
                path.GetText(),
                reason.c_str());
    }

    return stage->OverridePrim(path);
}

UsdPrim
UsdMayaShadingModeExportContext::MakeStandardMaterialPrim(
        const AssignmentVector& assignmentsToBind,
        const std::string& name,
        SdfPathSet * const boundPrimPaths) const
{
    UsdPrim ret;

    std::string materialName = name;
    if (materialName.empty()) {
        MStatus status;
        MFnDependencyNode seDepNode(_shadingEngine, &status);
        if (!status) {
            return ret;
        }
        MString seName = seDepNode.name();
        materialName = MNamespace::stripNamespaceFromName(seName).asChar();
    }

    materialName = UsdMayaUtil::SanitizeName(materialName);
    UsdStageRefPtr stage = GetUsdStage();
    if (UsdPrim materialParent = _GetMaterialParent(
            stage,
            GetExportArgs().materialsScopeName,
            assignmentsToBind)) {
        SdfPath materialPath = materialParent.GetPath().AppendChild(
                TfToken(materialName));
        UsdShadeMaterial material = UsdShadeMaterial::Define(
                GetUsdStage(), materialPath);

        UsdPrim materialPrim = material.GetPrim();

        // could use this to determine where we want to export.
        TF_FOR_ALL(iter, assignmentsToBind) {
            const SdfPath &boundPrimPath = iter->first;
            const VtIntArray &faceIndices = iter->second;

            // In the standard material binding case, skip if we're authoring
            // direct (non-collection-based) bindings and we're an instance
            // proxy.
            // In the case of per-face bindings, un-instance the prim in order
            // to author the append face sets or create a geom subset, since
            // collection-based bindings won't help us here.
            if (faceIndices.empty()) {
                if (!GetExportArgs().exportCollectionBasedBindings) {
                    if (_IsInstanceProxyPath(stage, boundPrimPath)) {
                        // XXX: If we wanted to, we could try to author the
                        // binding on the parent prim instead if it's an
                        // instance prim with only one child (i.e. if it's the
                        // transform prim corresponding to our shape prim).
                        TF_WARN("Can't author direct material binding on "
                                "instance proxy <%s>; try enabling "
                                "collection-based material binding",
                                boundPrimPath.GetText());
                    }
                    else {
                        UsdPrim boundPrim = stage->OverridePrim(boundPrimPath);
                        UsdShadeMaterialBindingAPI(boundPrim).Bind(material);
                    }
                }

                if (boundPrimPaths) {
                    boundPrimPaths->insert(boundPrimPath);
                }
            } else {
                UsdPrim boundPrim = _UninstancePrim(
                        stage, boundPrimPath, "authoring per-face materials");
                UsdGeomSubset faceSubset = UsdShadeMaterialBindingAPI(
                        boundPrim).CreateMaterialBindSubset(
                            /* subsetName */ TfToken(materialName),
                            faceIndices,
                            /* elementType */ UsdGeomTokens->face);

                if (!GetExportArgs().exportCollectionBasedBindings) {
                    material.Bind(faceSubset.GetPrim());
                }

                if (boundPrimPaths) {
                    boundPrimPaths->insert(faceSubset.GetPath());
                }

                UsdShadeMaterialBindingAPI(boundPrim)
                    .SetMaterialBindSubsetsFamilyType(UsdGeomTokens->partition);
            }
        }

        return materialPrim;
    }

    return UsdPrim();
}

std::string
UsdMayaShadingModeExportContext::GetStandardAttrName(
        const MPlug& plug,
        const bool allowMultiElementArrays) const
{
    if (plug.isElement()) {
        MString mayaPlgName = plug.array().partialName(false, false, false, false, false, true);
        unsigned int logicalIdx = plug.logicalIndex();
        if (allowMultiElementArrays) {
            return TfStringPrintf("%s_%d", mayaPlgName.asChar(), logicalIdx);
        }
        else if (logicalIdx == 0) {
            return mayaPlgName.asChar();
        }
        else {
            return TfToken();
        }
    }
    else {
        MString mayaPlgName = plug.partialName(false, false, false, false, false, true);
        return mayaPlgName.asChar();
    }
}


PXR_NAMESPACE_CLOSE_SCOPE
