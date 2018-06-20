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
#include "pxr/pxr.h"

#include "usdMaya/roundTripUtil.h"
#include "usdMaya/shadingModeExporter.h"
#include "usdMaya/shadingModeExporterContext.h"
#include "usdMaya/shadingModeRegistry.h"
#include "usdMaya/util.h"
#include "usdMaya/writeUtil.h"

// Defines the RenderMan for Maya mapping between Pxr objects and Maya internal nodes
#include "usdMaya/shadingModePxrRis_rfm_map.h"

#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/usd/sdf/valueTypeName.h"
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
#include <maya/MFnNumericAttribute.h>
#include <maya/MPlug.h>


PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((PxrShaderPrefix, "Pxr"))
    ((DefaultShaderOutputName, "out"))
    ((MayaShaderOutputName, "outColor"))
);


namespace {
class PxrRisShadingModeExporter : public PxrUsdMayaShadingModeExporter {
public:
    PxrRisShadingModeExporter() {}
private:
    TfToken
    _GetShaderTypeName(const MFnDependencyNode& depNode)
    {
        const TfToken mayaTypeName(depNode.typeName().asChar());

        // Now look into the RIS TABLE if the typeName doesn't starts with Pxr.
        if (!TfStringStartsWith(mayaTypeName, _tokens->PxrShaderPrefix)) {
            for (size_t i = 0u; i < _RFM_RISNODE_TABLE.size(); ++i) {
                if (_RFM_RISNODE_TABLE[i].first == mayaTypeName) {
                    return _RFM_RISNODE_TABLE[i].second;
                }
            }
        }

        return mayaTypeName;
    }

    UsdPrim
    _ExportShadingNodeHelper(const UsdPrim& materialPrim,
                             const MFnDependencyNode& depNode,
                             const PxrUsdMayaShadingModeExportContext& context,
                             SdfPathSet* processedPaths)
    {
        UsdStagePtr stage = materialPrim.GetStage();

        // XXX: would be nice to write out the current display color as
        // well.  currently, when we re-import, we don't get the display color so
        // it shows up as black.

        const TfToken shaderPrimName(
            PxrUsdMayaUtil::SanitizeName(depNode.name().asChar()));
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

            if (!PxrUsdMayaUtil::IsAuthored(attrPlug)) {
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
                PxrUsdMayaWriteUtil::GetUsdTypeName(attrPlug);
            if (!attrTypeName) {
                continue;
            }

            UsdShadeInput input = shaderSchema.CreateInput(attrName,
                                                           attrTypeName);
            if (!input) {
                continue;
            }

            if (attrPlug.isElement()) {
                PxrUsdMayaRoundTripUtil::MarkAttributeAsArray(input.GetAttr(),
                                                              0u);
            }

            PxrUsdMayaWriteUtil::SetUsdAttr(attrPlug,
                                            input.GetAttr(),
                                            UsdTimeCode::Default());

            // Now handle plug connections and recurse if necessary.
            if (!attrPlug.isConnected() || !attrPlug.isDestination()) {
                continue;
            }

            const MPlug connectedPlug(PxrUsdMayaUtil::GetConnected(attrPlug));
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
    _ExportShadingNode(const UsdPrim& materialPrim,
                       const MFnDependencyNode& depNode,
                       const PxrUsdMayaShadingModeExportContext& context)
    {
        SdfPathSet processedNodes;
        return _ExportShadingNodeHelper(materialPrim,
                                        depNode,
                                        context,
                                        &processedNodes);
    }

    void Export(const PxrUsdMayaShadingModeExportContext& context,
                UsdShadeMaterial* const mat,
                SdfPathSet* const boundPrimPaths) override
    {
        const PxrUsdMayaShadingModeExportContext::AssignmentVector& assignments =
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

        MStatus status;
        const MFnDependencyNode ssDepNode(context.GetSurfaceShader(), &status);
        if (status != MS::kSuccess) {
            return;
        }

        UsdPrim shaderPrim = _ExportShadingNode(materialPrim,
                                                ssDepNode,
                                                context);
        UsdShadeShader shaderSchema(shaderPrim);
        if (!shaderSchema) {
            return;
        }

        UsdShadeOutput shaderDefaultOutput =
            shaderSchema.CreateOutput(_tokens->DefaultShaderOutputName,
                                      SdfValueTypeNames->Token);
        if (!shaderDefaultOutput) {
            return;
        }

        UsdRiMaterialAPI riMaterialAPI(materialPrim);
        riMaterialAPI.SetSurfaceSource(shaderDefaultOutput.GetAttr().GetPath());
    }
};
}

TF_REGISTRY_FUNCTION_WITH_TAG(PxrUsdMayaShadingModeExportContext, pxrRis)
{
    PxrUsdMayaShadingModeRegistry::GetInstance().RegisterExporter(
        "pxrRis",
        []() -> PxrUsdMayaShadingModeExporterPtr {
            return PxrUsdMayaShadingModeExporterPtr(
                static_cast<PxrUsdMayaShadingModeExporter*>(
                    new PxrRisShadingModeExporter()));
        }
    );
}

namespace _importer {

static
MObject
_CreateShaderObject(
        const UsdShadeShader& shaderSchema,
        PxrUsdMayaShadingModeImportContext* context);

static
MObject
_GetOrCreateShaderObject(
        const UsdShadeShader& shaderSchema,
        PxrUsdMayaShadingModeImportContext* context)
{
    MObject shaderObj;
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
    if (PxrUsdMayaRoundTripUtil::GetAttributeArray(usdAttr, &index)) {
        mayaAttrPlug = mayaAttrPlug.elementByLogicalIndex(index, &status);
        if (status != MS::kSuccess) {
            return MPlug();
        }
    }

    PxrUsdMayaUtil::setPlugValue(usdAttr, mayaAttrPlug);

    return mayaAttrPlug;
}

// Should only be called by _GetOrCreateShaderObject, no one else.
MObject
_CreateShaderObject(
        const UsdShadeShader& shaderSchema,
        PxrUsdMayaShadingModeImportContext* context)
{
    TfToken shaderId;
    shaderSchema.GetIdAttr().Get(&shaderId);

    TfToken mayaTypeName = shaderId;

    // Now remap the mayaTypeName if found in the RIS table.
    for (size_t i = 0u; i < _RFM_RISNODE_TABLE.size(); ++i) {
        if (_RFM_RISNODE_TABLE[i].second == mayaTypeName) {
            mayaTypeName = _RFM_RISNODE_TABLE[i].first;
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

        PxrUsdMayaUtil::Connect(srcAttr, mayaAttr, false);
    }

    return shaderObj;
}

}; // namespace _importer

DEFINE_SHADING_MODE_IMPORTER(pxrRis, context)
{
    // This expects the renderman for maya plugin is loaded.
    // How do we ensure that it is?
    const UsdShadeMaterial& shadeMaterial = context->GetShadeMaterial();

    // First try the "surface" output of the material.
    UsdShadeShader surfaceShader = shadeMaterial.ComputeSurfaceSource(
        UsdShadeTokens->surface);

    // Otherwise fall back to trying the "surface" output  of the UsdRi schema.
    if (!surfaceShader) {
        surfaceShader = UsdRiMaterialAPI(shadeMaterial).GetSurface();
        if (!surfaceShader) {
            return MPlug();
        }
    }

    MStatus status;
    MObject shaderObj = _importer::_GetOrCreateShaderObject(surfaceShader,
                                                            context);
    MFnDependencyNode shaderDepFn(shaderObj, &status);
    if (status != MS::kSuccess) {
        return MPlug();
    }

    MPlug outputPlug =
        shaderDepFn.findPlug(_tokens->MayaShaderOutputName.GetText(), &status);
    if (status != MS::kSuccess) {
        return MPlug();
    }

    return outputPlug;
}


PXR_NAMESPACE_CLOSE_SCOPE
