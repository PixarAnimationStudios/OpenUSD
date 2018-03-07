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

#include "usdMaya/shadingModeExporter.h"
#include "usdMaya/shadingModeExporterContext.h"
#include "usdMaya/shadingModeRegistry.h"
#include "usdMaya/translatorMaterial.h"

#include "pxr/base/gf/gamma.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usdGeom/primvar.h"
#include "pxr/usd/usdRi/materialAPI.h"
#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/material.h"
#include "pxr/usd/usdShade/output.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/tokens.h"

#include <maya/MColor.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnLambertShader.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <string>


PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (displayColor)
    (displayOpacity)
    (diffuseColor)
    (transmissionColor)
    (transparency)

    ((MayaShaderName, "lambert"))
    ((DefaultShaderId, "PxrDiffuse"))
    ((DefaultShaderOutputName, "out"))
);


namespace {
class DisplayColorShadingModeExporter : public PxrUsdMayaShadingModeExporter {
public:
    DisplayColorShadingModeExporter() {}
private:
    void Export(const PxrUsdMayaShadingModeExportContext& context,
                UsdShadeMaterial * const mat, 
                SdfPathSet * const boundPrimPaths) override 
    {
        MStatus status;
        const MFnDependencyNode ssDepNode(context.GetSurfaceShader(), &status);
        if (status != MS::kSuccess) {
            return;
        }
        const MFnLambertShader lambertFn(ssDepNode.object(), &status);
        if (status != MS::kSuccess) {
            return;
        }

        const PxrUsdMayaShadingModeExportContext::AssignmentVector& assignments =
            context.GetAssignments();
        if (assignments.empty()) {
            return;
        }

        const UsdStageRefPtr& stage = context.GetUsdStage();
        const MColor mayaColor = lambertFn.color();
        const MColor mayaTransparency = lambertFn.transparency();
        const GfVec3f color = GfConvertDisplayToLinear(
            GfVec3f(mayaColor[0], mayaColor[1], mayaColor[2]));
        const GfVec3f transparency = GfConvertDisplayToLinear(
            GfVec3f(mayaTransparency[0], mayaTransparency[1], mayaTransparency[2]));

        VtArray<GfVec3f> displayColorAry;
        displayColorAry.push_back(color);

        // The simple UsdGeomGprim display shading schema only allows for a
        // scalar opacity.  We compute it as the unweighted average of the
        // components since it would be ridiculous to apply the inverse weighting
        // (of the common graycale conversion) on re-import
        // The average is compute from the Maya color as is
        VtArray<float> displayOpacityAry;
        const float transparencyAvg = (mayaTransparency[0] +
                                       mayaTransparency[1] +
                                       mayaTransparency[2]) / 3.0f;
        if (transparencyAvg > 0.0f) {
            displayOpacityAry.push_back(1.0f - transparencyAvg);
        }

        TF_FOR_ALL(iter, assignments) {
            const SdfPath& boundPrimPath = iter->first;
            const VtIntArray& faceIndices = iter->second;
            if (!faceIndices.empty()) {
                continue;
            }

            UsdPrim boundPrim = stage->GetPrimAtPath(boundPrimPath);
            if (!boundPrim) {
                MGlobal::displayError("No prim bound to: " +
                                      MString(boundPrimPath.GetText()));
                continue;
            }

            UsdGeomGprim primSchema(boundPrim);
            // Set color if not already authored
            //
            // XXX: Note that this will not update the display color
            // in the presence of a SdfValueBlock which is a valid
            // 'authored value opinion' in the eyes of Usd.
            if (!primSchema.GetDisplayColorAttr().HasAuthoredValueOpinion()) {
                // not animatable
                primSchema.CreateDisplayColorPrimvar().Set(displayColorAry);
            }
            if (transparencyAvg > 0.0f &&
                    !primSchema.GetDisplayOpacityAttr().HasAuthoredValueOpinion()) {
                // not animatable
                primSchema.CreateDisplayOpacityPrimvar().Set(displayOpacityAry);
            }
        }

        UsdPrim materialPrim = context.MakeStandardMaterialPrim(assignments,
                                                                std::string(),
                                                                boundPrimPaths);
        UsdShadeMaterial material(materialPrim);
        if (material) {
            *mat = material;

            // Create a Diffuse RIS shader for the Material.
            // Although Maya can't yet make use of it, downstream apps
            // can make use of Material interface inputs, so create one to
            // drive the shader's color.
            //
            // NOTE!  We do not set any values directly on the shaders;
            // instead we set the values only on the material's interface,
            // emphasizing that the interface is a value provider for
            // its shading networks.
            UsdShadeInput dispColorIA =
                material.CreateInput(_tokens->displayColor,
                                     SdfValueTypeNames->Color3f);
            dispColorIA.Set(VtValue(color));

            const std::string shaderName =
                TfStringPrintf("%s_lambert", materialPrim.GetName().GetText());
            const TfToken shaderPrimName(shaderName);
            UsdShadeShader shaderSchema =
                UsdShadeShader::Define(
                    stage,
                    materialPrim.GetPath().AppendChild(shaderPrimName));
            shaderSchema.CreateIdAttr(VtValue(_tokens->DefaultShaderId));
            UsdShadeInput diffuse =
                shaderSchema.CreateInput(_tokens->diffuseColor,
                                         SdfValueTypeNames->Color3f);

            diffuse.ConnectToSource(dispColorIA);

            // Make an interface input for transparency, which we will hook up
            // to the shader, and a displayOpacity, for any shader that might
            // want to consume it.  Only author a *value* if we got a
            // non-zero transparency
            UsdShadeInput transparencyIA =
                material.CreateInput(_tokens->transparency,
                                     SdfValueTypeNames->Color3f);
            UsdShadeInput dispOpacityIA =
                material.CreateInput(_tokens->displayOpacity,
                                     SdfValueTypeNames->Float);

            // PxrDiffuse's transmissionColor may not produce similar
            // results to MfnLambertShader's transparency, but it's in
            // the general ballpark...
            UsdShadeInput transmission =
                shaderSchema.CreateInput(_tokens->transmissionColor,
                                         SdfValueTypeNames->Color3f);
            transmission.ConnectToSource(transparencyIA);

            if (transparencyAvg > 0.0f) {
                transparencyIA.Set(VtValue(transparency));
                dispOpacityIA.Set(VtValue(1.0f - transparencyAvg));
            }

            UsdShadeOutput shaderDefaultOutput =
                shaderSchema.CreateOutput(_tokens->DefaultShaderOutputName,
                                          SdfValueTypeNames->Token);
            if (!shaderDefaultOutput) {
                return;
            }

            UsdShadeOutput materialSurfaceOutput =
                material.CreateOutput(UsdShadeTokens->surface,
                                      shaderDefaultOutput.GetTypeName());
            if (!materialSurfaceOutput) {
                return;
            }

            materialSurfaceOutput.ConnectToSource(shaderDefaultOutput);

            // XXX: For backwards compatibility, we continue to author the
            // UsdRi Bxdf source until consumers (e.g. PxrUsdIn) are updated to
            // look at the outputs:surface terminal.
            UsdRiMaterialAPI(materialPrim).SetBxdfSource(
                shaderDefaultOutput.GetAttr().GetPath());

        }
    }
};
}

TF_REGISTRY_FUNCTION_WITH_TAG(PxrUsdMayaShadingModeExportContext, displayColor)
{
    PxrUsdMayaShadingModeRegistry::GetInstance().RegisterExporter(
        "displayColor",
        []() -> PxrUsdMayaShadingModeExporterPtr {
            return PxrUsdMayaShadingModeExporterPtr(
                static_cast<PxrUsdMayaShadingModeExporter*>(
                    new DisplayColorShadingModeExporter()));
        }
    );
}

DEFINE_SHADING_MODE_IMPORTER(displayColor, context)
{
    const UsdShadeMaterial& shadeMaterial = context->GetShadeMaterial();
    const UsdGeomGprim& primSchema = context->GetBoundPrim();

    MStatus status;

    // Note that we always couple the source of the displayColor with the
    // source of the displayOpacity.  It would not make sense to get the
    // displayColor from a bound Material, while getting the displayOpacity from
    // the gprim itself, for example, even if the Material did not have
    // displayOpacity authored. When the Material or gprim does not have
    // displayOpacity authored, we fall back to full opacity.
    bool gotDisplayColorAndOpacity=false;

    // Get Display Color from USD (linear) and convert to Display
    GfVec3f linearDisplayColor(.5,.5,.5), linearTransparency(0, 0, 0);

    UsdShadeInput shadeInput = shadeMaterial ? 
        shadeMaterial.GetInput(_tokens->displayColor) :
        UsdShadeInput();

    if (!shadeInput || !shadeInput.Get(&linearDisplayColor)) {

        VtArray<GfVec3f> gprimDisplayColor(1);
        if (primSchema && 
            primSchema.GetDisplayColorPrimvar().ComputeFlattened(&gprimDisplayColor)) 
        {
            linearDisplayColor=gprimDisplayColor[0];
            VtArray<float> gprimDisplayOpacity(1);
            if (primSchema.GetDisplayOpacityPrimvar().GetAttr()
                                                     .HasAuthoredValueOpinion()
                && primSchema.GetDisplayOpacityPrimvar().ComputeFlattened(&gprimDisplayOpacity)) {
                float trans = 1.0 - gprimDisplayOpacity[0];
                linearTransparency = GfVec3f(trans, trans, trans);
            }
            gotDisplayColorAndOpacity = true;
        } else {
            MString warn = MString("Unable to retrieve DisplayColor on Material: ");
            warn += shadeMaterial ? shadeMaterial.GetPrim().GetPath().GetText() : "<NONE>";
            warn += " or GPrim: ";
            warn += primSchema ? primSchema.GetPrim().GetPath().GetText() 
                               : "<NONE>";
            MGlobal::displayWarning(warn);
        }
    } else {
        shadeMaterial
            .GetInput(_tokens->transparency)
            .GetAttr()
            .Get(&linearTransparency);
        gotDisplayColorAndOpacity = true;
    }

    GfVec3f displayColor = GfConvertLinearToDisplay(linearDisplayColor);
    GfVec3f transparencyColor = GfConvertLinearToDisplay(linearTransparency);
    if (gotDisplayColorAndOpacity) {

        std::string shaderName(_tokens->MayaShaderName.GetText());
        SdfPath shaderParentPath = SdfPath::AbsoluteRootPath();
        if (shadeMaterial) {
            const UsdPrim& shadeMaterialPrim = shadeMaterial.GetPrim();
            shaderName = TfStringPrintf("%s_%s",
                shadeMaterialPrim.GetName().GetText(),
                _tokens->MayaShaderName.GetText());
            shaderParentPath = shadeMaterialPrim.GetPath();
        }

        MString mShaderName(shaderName.c_str());
        // Construct the lambert shader
        MFnLambertShader lambertFn;
        MObject shadingObj = lambertFn.create();
        lambertFn.setName( mShaderName );
        lambertFn.setColor(MColor(displayColor[0], displayColor[1], displayColor[2]));
        lambertFn.setTransparency(MColor(transparencyColor[0], transparencyColor[1], transparencyColor[2]));

        const SdfPath lambertPath = shaderParentPath.AppendChild(
            TfToken(lambertFn.name().asChar()));
        context->AddCreatedObject(lambertPath, shadingObj);

        // Connect the lambert surfaceShader to the Shading Group (set)
        return lambertFn.findPlug("outColor", &status);
    } 

    return MPlug();
}


PXR_NAMESPACE_CLOSE_SCOPE

