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
#include "usdMaya/roundTripUtil.h"
#include "usdMaya/shadingModeExporter.h"
#include "usdMaya/shadingModeExporterContext.h"
#include "usdMaya/shadingModeImporter.h"
// Defines the RenderMan for Maya mapping between Pxr objects and Maya internal nodes
#include "usdMaya/shadingModePxrRis_rfm_map.h"
#include "usdMaya/shadingModeRegistry.h"
#include "usdMaya/util.h"
#include "usdMaya/writeUtil.h"

#include "pxr/pxr.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usdRi/materialAPI.h"
#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/output.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/tokens.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MFnSet.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>
#include <maya/MGlobal.h>

#include <vector>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((PxrShaderPrefix, "Pxr"))
    ((DefaultShaderOutputName, "out"))
    ((MayaShaderOutputName, "outColor"))

    ((RmanPlugPreferenceName, "rfmShadingEngineUseRmanPlugs"))

    ((RmanVolumeShaderPlugName, "volumeShader"))
);


namespace {

struct _ShadingPlugs {
    const TfToken surface;
    const TfToken displacement;
};

static const _ShadingPlugs _RmanPlugs { 
    TfToken("rman__surface"), 
    TfToken("rman__displacement") 
};

static const _ShadingPlugs _MayaPlugs { 
    TfToken("surfaceShader"), 
    TfToken("displacementShader") 
};

static 
_ShadingPlugs
_GetShadingPlugs()
{
    // Check for rfmShadingEngineUseRmanPlugs preference
    // If set to 1, use rman__surface and rman__displacement plug names
    // Otherwise, fallback to Maya's surfaceShader and displacementShader
    bool exists = false;
    int useRmanPlugs = MGlobal::optionVarIntValue(
            _tokens->RmanPlugPreferenceName.GetText(), &exists);
    return (exists && useRmanPlugs) ? _RmanPlugs : _MayaPlugs;
}

class PxrRisShadingModeExporter : public UsdMayaShadingModeExporter {
public:
    PxrRisShadingModeExporter() {}
private:

    void
    PreExport(UsdMayaShadingModeExportContext* context) override
    {
        context->SetVolumeShaderPlugName(_tokens->RmanVolumeShaderPlugName);

        const auto shadingPlugs = _GetShadingPlugs();
        context->SetSurfaceShaderPlugName(shadingPlugs.surface);
        context->SetDisplacementShaderPlugName(shadingPlugs.displacement);
    }

    TfToken
    _GetShaderTypeName(const MFnDependencyNode& depNode)
    {
        const TfToken mayaTypeName(depNode.typeName().asChar());

        // Now look into the RIS TABLE if the typeName doesn't starts with Pxr.
        if (!TfStringStartsWith(mayaTypeName, _tokens->PxrShaderPrefix)) {
            for (const auto& i : _RFM_RISNODE_TABLE) {
                if (i.first == mayaTypeName) {
                    return i.second;
                }
            }
        }

        return mayaTypeName;
    }

    UsdPrim
    _ExportShadingNodeHelper(
            const UsdPrim& materialPrim,
            const MFnDependencyNode& depNode,
            const UsdMayaShadingModeExportContext& context,
            SdfPathSet* processedPaths)
    {
        UsdStagePtr stage = materialPrim.GetStage();

        // XXX: would be nice to write out the current display color as
        // well.  currently, when we re-import, we don't get the display color so
        // it shows up as black.

        const TfToken shaderPrimName(
            UsdMayaUtil::SanitizeName(depNode.name().asChar()));
        const SdfPath shaderPath = materialPrim.GetPath().AppendChild(shaderPrimName);
        if (processedPaths->count(shaderPath) == 1u) {
            return stage->GetPrimAtPath(shaderPath);
        }

        processedPaths->insert(shaderPath);

        // Determine the risShaderType that will correspond to the USD shader ID.
        const TfToken risShaderType = _GetShaderTypeName(depNode);

        if (!TfStringStartsWith(risShaderType, _tokens->PxrShaderPrefix)) {
            TF_RUNTIME_ERROR(
                    "Skipping '%s' because its type '%s' is not Pxr-prefixed.",
                    depNode.name().asChar(),
                    risShaderType.GetText());
            return UsdPrim();
        }

        UsdShadeShader shaderSchema = UsdShadeShader::Define(stage, shaderPath);
        shaderSchema.CreateIdAttr(VtValue(risShaderType));

        MStatus status = MS::kFailure;

        std::vector<MPlug> attrPlugs;

        // gather all the attrPlugs we want to process.
        for (unsigned int i = 0u; i < depNode.attributeCount(); ++i) {
            MPlug attrPlug = depNode.findPlug(depNode.attribute(i), true);
            if (attrPlug.isProcedural()) {
                // maya docs says these should not be saved off.  we skip them
                // here.
                continue;
            }

            if (attrPlug.isChild()) {
                continue;
            }

            if (!UsdMayaUtil::IsAuthored(attrPlug)) {
                continue;
            }

            // For now, we only support arrays of length 1.  if we encounter
            // such an array, we emit it's 0-th element.
            if (attrPlug.isArray()) {
                const unsigned int numElements = attrPlug.evaluateNumElements();
                if (numElements > 0u) {
                    attrPlugs.push_back(attrPlug[0]);
                    if (numElements > 1u) {
                        TF_WARN(
                            "Array with multiple elements encountered at '%s'. "
                            "Currently, only arrays with a single element are "
                            "supported.",
                            attrPlug.name().asChar());
                    }
                }
            }
            else {
                attrPlugs.push_back(attrPlug);
            }
        }

        for (const auto& attrPlug : attrPlugs) {
            // this is writing out things that live on the MFnDependencyNode.
            // maybe that's OK?  nothing downstream cares about it.

            const TfToken attrName = TfToken(
                context.GetStandardAttrName(attrPlug, false));
            if (attrName.IsEmpty()) {
                continue;
            }

            const SdfValueTypeName attrTypeName =
                UsdMayaWriteUtil::GetUsdTypeName(attrPlug);
            if (!attrTypeName) {
                continue;
            }

            UsdShadeInput input = shaderSchema.CreateInput(attrName,
                                                           attrTypeName);
            if (!input) {
                continue;
            }

            if (attrPlug.isElement()) {
                UsdMayaRoundTripUtil::MarkAttributeAsArray(input.GetAttr(),
                                                              0u);
            }

            UsdMayaWriteUtil::SetUsdAttr(attrPlug,
                                            input.GetAttr(),
                                            UsdTimeCode::Default());

            // Now handle plug connections and recurse if necessary.
            if (!attrPlug.isConnected() || !attrPlug.isDestination()) {
                continue;
            }

            const MPlug connectedPlug(UsdMayaUtil::GetConnected(attrPlug));
            const MFnDependencyNode connectedDepFn(connectedPlug.node(),
                                                   &status);
            if (status != MS::kSuccess) {
                continue;
            }

            if (UsdPrim cPrim = _ExportShadingNodeHelper(materialPrim,
                                                         connectedDepFn,
                                                         context,
                                                         processedPaths)) {
                UsdShadeConnectableAPI::ConnectToSource(
                    input,
                    UsdShadeShader(cPrim),
                    TfToken(context.GetStandardAttrName(connectedPlug, false)));
            }
        }

        return shaderSchema.GetPrim();
    }

    UsdPrim
    _ExportShadingNode(
            const UsdPrim& materialPrim,
            const MFnDependencyNode& depNode,
            const UsdMayaShadingModeExportContext& context)
    {
        SdfPathSet processedNodes;
        return _ExportShadingNodeHelper(materialPrim,
                                        depNode,
                                        context,
                                        &processedNodes);
    }

    void
    Export(
            const UsdMayaShadingModeExportContext& context,
            UsdShadeMaterial* const mat,
            SdfPathSet* const boundPrimPaths) override
    {
        const UsdMayaShadingModeExportContext::AssignmentVector& assignments =
            context.GetAssignments();
        if (assignments.empty()) {
            return;
        }

        UsdPrim materialPrim = context.MakeStandardMaterialPrim(assignments,
                                                                std::string(),
                                                                boundPrimPaths);
        UsdShadeMaterial material(materialPrim);
        if (!material) {
            return;
        }

        if (mat != nullptr) {
            *mat = material;
        }

        UsdRiMaterialAPI riMaterialAPI(materialPrim);

        MStatus status;

        const MFnDependencyNode surfaceDepNodeFn(
            context.GetSurfaceShader(),
            &status);
        if (status == MS::kSuccess) {
            UsdPrim surfaceShaderPrim =
                _ExportShadingNode(materialPrim,
                                   surfaceDepNodeFn,
                                   context);
            UsdShadeShader surfaceShaderSchema(surfaceShaderPrim);
            if (surfaceShaderSchema) {
                UsdShadeOutput surfaceShaderOutput =
                    surfaceShaderSchema.CreateOutput(
                        _tokens->DefaultShaderOutputName,
                        SdfValueTypeNames->Token);

                riMaterialAPI.SetSurfaceSource(
                    surfaceShaderOutput.GetAttr().GetPath());
            }
        }

        const MFnDependencyNode volumeDepNodeFn(
            context.GetVolumeShader(),
            &status);
        if (status == MS::kSuccess) {
            UsdPrim volumeShaderPrim =
                _ExportShadingNode(materialPrim,
                                   volumeDepNodeFn,
                                   context);
            UsdShadeShader volumeShaderSchema(volumeShaderPrim);
            if (volumeShaderSchema) {
                UsdShadeOutput volumeShaderOutput =
                    volumeShaderSchema.CreateOutput(
                        _tokens->DefaultShaderOutputName,
                        SdfValueTypeNames->Token);

                riMaterialAPI.SetVolumeSource(
                    volumeShaderOutput.GetAttr().GetPath());
            }
        }

        const MFnDependencyNode displacementDepNodeFn(
            context.GetDisplacementShader(),
            &status);
        if (status == MS::kSuccess) {
            UsdPrim displacementShaderPrim =
                _ExportShadingNode(materialPrim,
                                   displacementDepNodeFn,
                                   context);
            UsdShadeShader displacementShaderSchema(displacementShaderPrim);
            if (displacementShaderSchema) {
                UsdShadeOutput displacementShaderOutput =
                    displacementShaderSchema.CreateOutput(
                        _tokens->DefaultShaderOutputName,
                        SdfValueTypeNames->Token);

                riMaterialAPI.SetDisplacementSource(
                    displacementShaderOutput.GetAttr().GetPath());
            }
        }
    }
};
}

TF_REGISTRY_FUNCTION_WITH_TAG(UsdMayaShadingModeExportContext, pxrRis)
{
    UsdMayaShadingModeRegistry::GetInstance().RegisterExporter(
        "pxrRis",
        []() -> UsdMayaShadingModeExporterPtr {
            return UsdMayaShadingModeExporterPtr(
                static_cast<UsdMayaShadingModeExporter*>(
                    new PxrRisShadingModeExporter()));
        }
    );
}

namespace {

static
MObject
_CreateShaderObject(
        const UsdShadeShader& shaderSchema,
        UsdMayaShadingModeImportContext* context);

static
MObject
_GetOrCreateShaderObject(
        const UsdShadeShader& shaderSchema,
        UsdMayaShadingModeImportContext* context)
{
    MObject shaderObj;
    if (!shaderSchema) {
        return shaderObj;
    }

    if (context->GetCreatedObject(shaderSchema.GetPrim(), &shaderObj)) {
        return shaderObj;
    }

    shaderObj = _CreateShaderObject(shaderSchema, context);
    return context->AddCreatedObject(shaderSchema.GetPrim(), shaderObj);
}

static
MPlug
_ImportAttr(const UsdAttribute& usdAttr, const MFnDependencyNode& fnDep)
{
    const std::string mayaAttrName = usdAttr.GetBaseName().GetString();
    MStatus status;

    MPlug mayaAttrPlug = fnDep.findPlug(mayaAttrName.c_str(), &status);
    if (status != MS::kSuccess) {
        return MPlug();
    }

    unsigned int index = 0u;
    if (UsdMayaRoundTripUtil::GetAttributeArray(usdAttr, &index)) {
        mayaAttrPlug = mayaAttrPlug.elementByLogicalIndex(index, &status);
        if (status != MS::kSuccess) {
            return MPlug();
        }
    }

    UsdMayaUtil::setPlugValue(usdAttr, mayaAttrPlug);

    return mayaAttrPlug;
}

// Should only be called by _GetOrCreateShaderObject, no one else.
MObject
_CreateShaderObject(
        const UsdShadeShader& shaderSchema,
        UsdMayaShadingModeImportContext* context)
{
    TfToken shaderId;
    shaderSchema.GetIdAttr().Get(&shaderId);

    TfToken mayaTypeName = shaderId;

    // Now remap the mayaTypeName if found in the RIS table.
    for (const auto & i : _RFM_RISNODE_TABLE) {
        if (i.second == mayaTypeName) {
            mayaTypeName = i.first;
            break;
        }
    }

    MStatus status;
    MFnDependencyNode depFn;
    MObject shaderObj =
        depFn.create(MString(mayaTypeName.GetText()),
                     MString(shaderSchema.GetPrim().GetName().GetText()),
                     &status);
    if (status != MS::kSuccess) {
        // we need to make sure assumes those types are loaded..
        TF_RUNTIME_ERROR(
                "Could not create node of type '%s' for shader '%s'. "
                "Probably missing a loadPlugin.\n",
                mayaTypeName.GetText(),
                shaderSchema.GetPrim().GetName().GetText());
        return MObject();
    }

    // The rest of this is not really RIS specific at all.
    for (const UsdShadeInput& input : shaderSchema.GetInputs()) {
        MPlug mayaAttr = _ImportAttr(input.GetAttr(), depFn);
        if (mayaAttr.isNull()) {
            continue;
        }

        UsdShadeConnectableAPI source;
        TfToken sourceOutputName;
        UsdShadeAttributeType sourceType;

        // Follow shader connections and recurse.
        if (!UsdShadeConnectableAPI::GetConnectedSource(input,
                                                        &source,
                                                        &sourceOutputName,
                                                        &sourceType)) {
            continue;
        }

        UsdShadeShader sourceShaderSchema = UsdShadeShader(source.GetPrim());
        if (!sourceShaderSchema) {
            continue;
        }

        MObject sourceObj = _GetOrCreateShaderObject(sourceShaderSchema,
                                                     context);
        MFnDependencyNode sourceDepFn(sourceObj, &status);
        if (status != MS::kSuccess) {
            continue;
        }

        MPlug srcAttr = sourceDepFn.findPlug(sourceOutputName.GetText());
        if (srcAttr.isArray()) {
            const unsigned int numElements = srcAttr.evaluateNumElements();
            if (numElements > 0u) {
                if (numElements > 1u) {
                    TF_WARN(
                        "Array with multiple elements encountered at '%s'. "
                        "Currently, only arrays with a single element are "
                        "supported. Not connecting attribute.",
                        srcAttr.name().asChar());
                    continue;
                }

                srcAttr = srcAttr[0];
            }
        }

        UsdMayaUtil::Connect(srcAttr, mayaAttr, false);
    }

    return shaderObj;
}

}; // anonymous namespace

DEFINE_SHADING_MODE_IMPORTER(pxrRis, context)
{
    // RenderMan for Maya wants the shader nodes to get hooked into the shading
    // group via its own plugs.
    context->SetVolumeShaderPlugName(_tokens->RmanVolumeShaderPlugName);

    const auto shadingPlugs = _GetShadingPlugs();
    context->SetSurfaceShaderPlugName(shadingPlugs.surface);
    context->SetDisplacementShaderPlugName(shadingPlugs.displacement);

    // This expects the renderman for maya plugin is loaded.
    // How do we ensure that it is?
    const UsdShadeMaterial& shadeMaterial = context->GetShadeMaterial();
    if (!shadeMaterial) {
        return MObject();
    }

    // Get the surface, volume, and/or displacement shaders of the material.
    // First we try computing the sources via the material, and otherwise we
    // fall back to querying the UsdRiMaterialAPI.
    UsdShadeShader surfaceShader = shadeMaterial.ComputeSurfaceSource();
    if (!surfaceShader) {
        surfaceShader = UsdRiMaterialAPI(shadeMaterial).GetSurface();
    }

    UsdShadeShader volumeShader = shadeMaterial.ComputeVolumeSource();
    if (!volumeShader) {
        volumeShader = UsdRiMaterialAPI(shadeMaterial).GetVolume();
    }

    UsdShadeShader displacementShader = shadeMaterial.ComputeDisplacementSource();
    if (!displacementShader) {
        displacementShader = UsdRiMaterialAPI(shadeMaterial).GetDisplacement();
    }

    MObject surfaceShaderObj = _GetOrCreateShaderObject(surfaceShader, context);
    MObject volumeShaderObj = _GetOrCreateShaderObject(volumeShader, context);
    MObject displacementShaderObj = _GetOrCreateShaderObject(displacementShader, context);

    if (surfaceShaderObj.isNull() &&
            volumeShaderObj.isNull() &&
            displacementShaderObj.isNull()) {
        return MObject();
    }

    // Create the shading engine.
    MObject shadingEngine = context->CreateShadingEngine();
    if (shadingEngine.isNull()) {
        return MObject();
    }
    MStatus status;
    MFnSet fnSet(shadingEngine, &status);
    if (status != MS::kSuccess) {
        return MObject();
    }

    const TfToken surfaceShaderPlugName = context->GetSurfaceShaderPlugName();
    if (!surfaceShaderPlugName.IsEmpty() && !surfaceShaderObj.isNull()) {
        MFnDependencyNode depNodeFn(surfaceShaderObj, &status);
        if (status != MS::kSuccess) {
            return MObject();
        }

        MPlug shaderOutputPlug =
            depNodeFn.findPlug(_tokens->MayaShaderOutputName.GetText(), &status);
        if (status != MS::kSuccess || shaderOutputPlug.isNull()) {
            return MObject();
        }

        MPlug seInputPlug =
            fnSet.findPlug(surfaceShaderPlugName.GetText(), &status);
        CHECK_MSTATUS_AND_RETURN(status, MObject());

        UsdMayaUtil::Connect(shaderOutputPlug,
                                seInputPlug,
                                /* clearDstPlug = */ true);
    }

    const TfToken volumeShaderPlugName = context->GetVolumeShaderPlugName();
    if (!volumeShaderPlugName.IsEmpty() && !volumeShaderObj.isNull()) {
        MFnDependencyNode depNodeFn(volumeShaderObj, &status);
        if (status != MS::kSuccess) {
            return MObject();
        }

        MPlug shaderOutputPlug =
            depNodeFn.findPlug(_tokens->MayaShaderOutputName.GetText(), &status);
        if (status != MS::kSuccess || shaderOutputPlug.isNull()) {
            return MObject();
        }

        MPlug seInputPlug =
            fnSet.findPlug(volumeShaderPlugName.GetText(), &status);
        CHECK_MSTATUS_AND_RETURN(status, MObject());

        UsdMayaUtil::Connect(shaderOutputPlug,
                                seInputPlug,
                                /* clearDstPlug = */ true);
    }

    const TfToken displacementShaderPlugName = context->GetDisplacementShaderPlugName();
    if (!displacementShaderPlugName.IsEmpty() && !displacementShaderObj.isNull()) {
        MFnDependencyNode depNodeFn(displacementShaderObj, &status);
        if (status != MS::kSuccess) {
            return MObject();
        }

        MPlug shaderOutputPlug =
            depNodeFn.findPlug(_tokens->MayaShaderOutputName.GetText(), &status);
        if (status != MS::kSuccess || shaderOutputPlug.isNull()) {
            return MObject();
        }

        MPlug seInputPlug =
            fnSet.findPlug(displacementShaderPlugName.GetText(), &status);
        CHECK_MSTATUS_AND_RETURN(status, MObject());

        UsdMayaUtil::Connect(shaderOutputPlug,
                                seInputPlug,
                                /* clearDstPlug = */ true);
    }

    return shadingEngine;
}


PXR_NAMESPACE_CLOSE_SCOPE
