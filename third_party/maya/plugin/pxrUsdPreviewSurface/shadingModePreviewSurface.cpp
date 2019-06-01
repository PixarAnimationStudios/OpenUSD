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
#include "usdMaya/colorSpace.h"
#include "usdMaya/roundTripUtil.h"
#include "usdMaya/shadingModeExporter.h"
#include "usdMaya/shadingModeExporterContext.h"
#include "usdMaya/shadingModeRegistry.h"
#include "usdMaya/translatorMaterial.h"
#include "usdMaya/translatorUtil.h"

#include "pxrUsdPreviewSurface/usdPreviewSurface.h"
#include "pxrUsdPreviewSurface/debugCodes.h"

#include "pxr/pxr.h"

#include "pxr/base/gf/gamma.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/value.h"

#include "pxr/imaging/hio/glslfx.h"

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

#include "pxr/usdImaging/usdImaging/tokens.h"

#include <maya/MAnimControl.h>
#include <maya/MColor.h>
#include <maya/MFnDependencyNode.h>
#include <maya/MFnLambertShader.h>
#include <maya/MFnSet.h>
#include <maya/MGlobal.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>

#include <algorithm>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (black)
    (clamp)
    (displayColor)
    (displayOpacity)
    (diffuseColor)
    (file)
    (fileTextureName)
    (lambert)
    (mirror)
    (outColor)
    (outUV)
    (place2dTexture)
    (repeat)
    (result)
    (rgb)
    (st)
    (transmissionColor)
    (transparency)
    (uvCoord)
    (wrapS)
    (wrapT)

    ((DefaultShaderId, "PxrDiffuse"))
    ((DefaultShaderOutputName, "out"))
);

PXR_NAMESPACE_CLOSE_SCOPE

namespace {

PXR_NAMESPACE_USING_DIRECTIVE;

typedef std::function<TfToken(const UsdShadeShader&)> MayaTypeNameFunction;
typedef std::function<TfToken(
        const UsdShadeShader&, TfToken, bool)> MayaPlugNameFunction;

TfToken GetMayaTypeName(const UsdShadeShader& shaderSchema) {
    TfToken shaderId;
    shaderSchema.GetIdAttr().Get(&shaderId);

    if (shaderId == UsdImagingTokens->UsdPreviewSurface) {
        return PxrMayaUsdPreviewSurfaceTokens->MayaTypeName;
    } else if (shaderId == UsdImagingTokens->UsdUVTexture) {
        return _tokens->file;
    } else if (shaderId == UsdImagingTokens->UsdPrimvarReader_float2) {
        return _tokens->place2dTexture;
    }
    return shaderId;
}

TfToken GetMayaPlugName(const UsdShadeShader& shaderSchema, TfToken attrBase, bool asInput) {
    TfToken shaderId;
    shaderSchema.GetIdAttr().Get(&shaderId);

    if (shaderId == UsdImagingTokens->UsdUVTexture) {
        if (attrBase == _tokens->rgb) {
            return _tokens->outColor;
        } else if (attrBase == _tokens->file) {
            return _tokens->fileTextureName;
        } else if (attrBase == _tokens->st) {
            return _tokens->uvCoord;
        }
    } else if (shaderId == UsdImagingTokens->UsdPrimvarReader_float2) {
        if (attrBase == _tokens->result) {
            return _tokens->outUV;
        }
    }
    return attrBase;
}

MObject
_CreateAndPopulateShaderObject(
        const UsdShadeShader& shaderSchema,
        UsdMayaShadingModeImportContext* context,
        MayaTypeNameFunction mayaTypeNameFunction,
        MayaPlugNameFunction mayaPlugNameFunction);

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

    shaderObj = _CreateAndPopulateShaderObject(
            shaderSchema, context, GetMayaTypeName, GetMayaPlugName);
    return context->AddCreatedObject(shaderSchema.GetPrim(), shaderObj);
}

MPlug
_ImportAttr(const UsdAttribute& usdAttr, const MFnDependencyNode& fnDep,
            TfToken mayaAttrName)
{
    MStatus status;

    MPlug mayaAttrPlug = fnDep.findPlug(mayaAttrName.GetText(), &status);
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
_CreateAndPopulateShaderObject(
        const UsdShadeShader& shaderSchema,
        UsdMayaShadingModeImportContext* context,
        MayaTypeNameFunction mayaTypeNameFunction,
        MayaPlugNameFunction mayaPlugNameFunction)
{
    TfToken mayaTypeName = mayaTypeNameFunction(shaderSchema);

    TF_DEBUG(PXRUSDMAYA_PREVIEWSURFACE_IMPORT).Msg(
            "Making: %s (mayaType: %s)",
            shaderSchema.GetPrim().GetPath().GetText(),
            mayaTypeName.GetText());

    MStatus status;
    MObject shaderObj;
    MFnDependencyNode depFn;

    UsdMayaShadingNodeType shadingNodeType = UsdMayaShadingNodeType::NonShading;
    if (mayaTypeName == _tokens->lambert
            || mayaTypeName == PxrMayaUsdPreviewSurfaceTokens->MayaTypeName) {
        shadingNodeType = UsdMayaShadingNodeType::Shader;
    } else if (mayaTypeName == _tokens->file) {
        shadingNodeType = UsdMayaShadingNodeType::Texture;
    } else if (mayaTypeName == _tokens->place2dTexture) {
        shadingNodeType = UsdMayaShadingNodeType::Utility;
    }
    if (!(UsdMayaTranslatorUtil::CreateShaderNode(
                MString(shaderSchema.GetPrim().GetName().GetText()),
                mayaTypeName.GetText(),
                shadingNodeType,
                &status,
                &shaderObj)
            && depFn.setObject(shaderObj))) {
        TF_DEBUG(PXRUSDMAYA_PREVIEWSURFACE_IMPORT).Msg(
                "Error making node of type %s for %s",
                mayaTypeName.GetText(),
                shaderSchema.GetPrim().GetPath().GetText());
        return MObject();
    }

    auto inputs =  shaderSchema.GetInputs();

    if (mayaTypeName == _tokens->file) {
        // Make sure that we do the uv input first, as that will will create
        // a placeTexture2d node which will hook into other attrs (ie, wrapS/wrapT)
        auto isST = [](const UsdShadeInput& input) {
            return input.GetBaseName() == _tokens->st;
        };
        auto stPos = std::find_if(inputs.begin(), inputs.end(), isST);
        if (stPos != inputs.begin() && stPos != inputs.end()) {
            std::iter_swap(inputs.begin(), stPos);
            TF_DEBUG(PXRUSDMAYA_PREVIEWSURFACE_IMPORT).Msg(
                    "Swapped st input from position %lu to start",
                    stPos - inputs.begin());
        }
    }

    for (const UsdShadeInput& input : inputs) {
        auto usdAttr = input.GetAttr();
        TfToken usdAttrBaseName = usdAttr.GetBaseName();
        TfToken mayaAttrName = mayaPlugNameFunction(
                shaderSchema, usdAttrBaseName, true);

        TF_DEBUG(PXRUSDMAYA_PREVIEWSURFACE_IMPORT).Msg(
                "Attempting to import attr: %s.%s (%s.%s)",
                depFn.name().asChar(), mayaAttrName.GetText(),
                shaderSchema.GetPath().GetText(),
                input.GetAttr().GetName().GetText());

        if (usdAttrBaseName == _tokens->wrapS || usdAttrBaseName == _tokens->wrapT) {
            // since wrap/mirror attrs are highly likely to be connected to a
            // placeTexture2d node, we need to hunt upstream for the plug to "really"
            // set
            auto setUpstreamBool = [](MPlug& origPlug, bool newVal) {
                MPlug thePlug = origPlug;
                // set some crazy uppper limit
                for (size_t i = 0; i < 10000000; ++i) {
                    MPlug sourcePlug = origPlug.source();
                    if (sourcePlug.isNull()) {
                        break;
                    }
                    thePlug = sourcePlug;
                }
                thePlug.setBool(newVal);
            };

            MString mirrorAttrName;
            MString wrapAttrName;

            if (usdAttrBaseName == _tokens->wrapS) {
                mirrorAttrName = "mirrorU";
                wrapAttrName = "wrapU";
            } else {
                mirrorAttrName = "mirrorV";
                wrapAttrName = "wrapV";
            }

            VtValue val;
            if (!usdAttr.Get(&val, MAnimControl::currentTime().value())) {
                continue;
            }
            if (!val.IsHolding<TfToken>()) {
                continue;
            }

            TfToken wrapVal = val.UncheckedGet<TfToken>();
            if (wrapVal == _tokens->repeat) {
                // do nothing - will repeat by default
            } else if (wrapVal == _tokens->mirror) {
                MPlug mirrorAttr = depFn.findPlug(mirrorAttrName, &status);
                if (status != MS::kSuccess) {
                    continue;
                }
                setUpstreamBool(mirrorAttr, true);
            } else if (wrapVal == _tokens->black
                    // Note that this isn't proper clamp support - maya's
                    // placeTexture2d doesn't support that by itself, we
                    // would need to insert another (clamp) node
                    || wrapVal == _tokens->clamp) {
                MPlug wrapAttr = depFn.findPlug(wrapAttrName, &status);
                if (status != MS::kSuccess) {
                    continue;
                }
                setUpstreamBool(wrapAttr, false);
            }
            continue;
        }

        MPlug mayaAttr = _ImportAttr(input.GetAttr(), depFn, mayaAttrName);
        if (mayaAttr.isNull()) {
            continue;
        }

        TF_DEBUG(PXRUSDMAYA_PREVIEWSURFACE_IMPORT).Msg(
                "...successfully imported!");

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

        TF_DEBUG(PXRUSDMAYA_PREVIEWSURFACE_IMPORT).Msg(
                "...usd connected to: %s.outputs:%s",
                source.GetPath().GetText(), sourceOutputName.GetText());


        UsdShadeShader sourceShaderSchema = UsdShadeShader(source.GetPrim());
        if (!sourceShaderSchema) {
            continue;
        }

        MObject sourceObj = _GetOrCreateShaderObject(
            sourceShaderSchema,
            context);

        MFnDependencyNode sourceDepFn(sourceObj, &status);
        if (status != MS::kSuccess) {
            continue;
        }

        TfToken mayaOutputName = mayaPlugNameFunction(
                sourceShaderSchema, sourceOutputName, false);

        TF_DEBUG(PXRUSDMAYA_PREVIEWSURFACE_IMPORT).Msg(
                "...trying to connect to: %s.%s",
                sourceDepFn.name().asChar(), mayaOutputName.GetText());
        MPlug srcAttr = sourceDepFn.findPlug(mayaOutputName.GetText(),
                                             &status);

        if (status != MS::kSuccess) {
            continue;
        }

        if (srcAttr.isArray()) {
            const unsigned int numElements = srcAttr.evaluateNumElements();
            if (numElements > 0u) {
                if (numElements > 1u) {
                    TF_DEBUG(PXRUSDMAYA_PREVIEWSURFACE_IMPORT).Msg(
                        "Array with multiple elements encountered at '%s'. "
                        "Currently, only arrays with a single element are "
                        "supported. Not connecting attribute.",
                        srcAttr.name().asChar());
                    continue;
                }

                srcAttr = srcAttr[0];
            }
        }

        if (sourceObj.hasFn(MFn::kPlace2dTexture)
                && mayaTypeName == _tokens->file) {
            MString cmd;
            cmd.format("fileTexturePlacementConnectNoEcho \"^1s\" \"^2s\"",
                       depFn.name(), sourceDepFn.name());
            TF_DEBUG(PXRUSDMAYA_PREVIEWSURFACE_IMPORT).Msg(cmd.asChar());
            status = MGlobal::executeCommand(cmd, false, false);
            if (!status) {
                status.perror("Error connecting place2dTexture: ");
            }
        } else {
            UsdMayaUtil::Connect(srcAttr, mayaAttr, false);
        }

        TF_DEBUG(PXRUSDMAYA_PREVIEWSURFACE_IMPORT).Msg(
                "...successfully connected to: %s",
                srcAttr.name().asChar());

    }

    TF_DEBUG(PXRUSDMAYA_PREVIEWSURFACE_IMPORT).Msg(
            "Made: %s (mayaType: %s)",
            depFn.name().asChar(),
            mayaTypeName.GetText());

    return shaderObj;
}

MObject MakeDisplayColorShader(UsdMayaShadingModeImportContext* context) {

    const UsdShadeMaterial& shadeMaterial = context->GetShadeMaterial();
    const UsdGeomGprim& primSchema = context->GetBoundPrim();

    MStatus status;

    // Note that we always couple the source of the displayColor with the
    // source of the displayOpacity.  It would not make sense to get the
    // displayColor from a bound Material, while getting the displayOpacity from
    // the gprim itself, for example, even if the Material did not have
    // displayOpacity authored. When the Material or gprim does not have
    // displayOpacity authored, we fall back to full opacity.
    bool gotDisplayColorAndOpacity = false;

    // Get Display Color from USD (linear) and convert to Display
    GfVec3f linearDisplayColor(.5,.5,.5);
    GfVec3f linearTransparency(0, 0, 0);

    UsdShadeInput shadeInput = shadeMaterial ?
        shadeMaterial.GetInput(_tokens->displayColor) :
        UsdShadeInput();

    if (!shadeInput || !shadeInput.Get(&linearDisplayColor)) {

        VtVec3fArray gprimDisplayColor(1);
        if (primSchema &&
                primSchema.GetDisplayColorPrimvar().ComputeFlattened(&gprimDisplayColor)) {
            linearDisplayColor = gprimDisplayColor[0];
            VtFloatArray gprimDisplayOpacity(1);
            if (primSchema.GetDisplayOpacityPrimvar().GetAttr().HasAuthoredValue() &&
                    primSchema.GetDisplayOpacityPrimvar().ComputeFlattened(&gprimDisplayOpacity)) {
                const float trans = 1.0 - gprimDisplayOpacity[0];
                linearTransparency = GfVec3f(trans, trans, trans);
            }
            gotDisplayColorAndOpacity = true;
        } else {
            TF_DEBUG(PXRUSDMAYA_PREVIEWSURFACE_IMPORT).Msg(
                    "Unable to retrieve displayColor on Material: %s "
                    "or Gprim: %s",
                    shadeMaterial ?
                        shadeMaterial.GetPrim().GetPath().GetText() :
                        "<NONE>",
                    primSchema ?
                        primSchema.GetPrim().GetPath().GetText() :
                        "<NONE>");
        }
    } else {
        shadeMaterial
            .GetInput(_tokens->transparency)
            .GetAttr()
            .Get(&linearTransparency);
        gotDisplayColorAndOpacity = true;
    }

    if (!gotDisplayColorAndOpacity) {
        return MObject();
    }

    const GfVec3f displayColor =
        UsdMayaColorSpace::ConvertLinearToMaya(linearDisplayColor);
    const GfVec3f transparencyColor =
        UsdMayaColorSpace::ConvertLinearToMaya(linearTransparency);

    std::string shaderName(_tokens->lambert.GetText());
    SdfPath shaderParentPath = SdfPath::AbsoluteRootPath();
    if (shadeMaterial) {
        const UsdPrim& shadeMaterialPrim = shadeMaterial.GetPrim();
        shaderName =
            TfStringPrintf("%s_%s",
                shadeMaterialPrim.GetName().GetText(),
                _tokens->lambert.GetText());
        shaderParentPath = shadeMaterialPrim.GetPath();
    }

    // Construct the lambert shader.
    MFnLambertShader lambertFn;
    MObject shadingObj = lambertFn.create();
    lambertFn.setName(shaderName.c_str());
    lambertFn.setColor(
        MColor(displayColor[0], displayColor[1], displayColor[2]));
    lambertFn.setTransparency(
        MColor(transparencyColor[0], transparencyColor[1], transparencyColor[2]));

    // We explicitly set diffuse coefficient to 1.0 here since new lamberts
    // default to 0.8. This is to make sure the color value matches visually
    // when roundtripping since we bake the diffuseCoeff into the diffuse color
    // at export.
    lambertFn.setDiffuseCoeff(1.0);

    const SdfPath lambertPath =
        shaderParentPath.AppendChild(TfToken(lambertFn.name().asChar()));
    context->AddCreatedObject(lambertPath, shadingObj);

    // Find the outColor plug so we can connect it as the surface shader of the
    // shading engine.
    MPlug outputPlug = lambertFn.findPlug("outColor", &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject());

    // Create the shading engine.
    MObject shadingEngine = context->CreateShadingEngine();
    if (shadingEngine.isNull()) {
        return MObject();
    }
    MFnSet fnSet(shadingEngine, &status);
    if (status != MS::kSuccess) {
        return MObject();
    }

    const TfToken surfaceShaderPlugName = context->GetSurfaceShaderPlugName();
    if (!surfaceShaderPlugName.IsEmpty()) {
        MPlug seSurfaceShaderPlg =
            fnSet.findPlug(surfaceShaderPlugName.GetText(), &status);
        CHECK_MSTATUS_AND_RETURN(status, MObject());
        UsdMayaUtil::Connect(outputPlug,
                                seSurfaceShaderPlg,
                                /* clearDstPlug = */ true);
    }

    return shadingEngine;
}

MObject MakePreviewSurfaceShader(UsdMayaShadingModeImportContext* context) {
    const UsdShadeMaterial& shadeMaterial = context->GetShadeMaterial();
    if (!shadeMaterial) {
        return MObject();
    }

    UsdShadeShader surfaceShader = shadeMaterial.ComputeSurfaceSource(HioGlslfxTokens->glslfx);
    const TfToken surfaceShaderPlugName = context->GetSurfaceShaderPlugName();
    if (surfaceShaderPlugName.IsEmpty()) {
        return MObject();
    }

    MObject surfaceShaderObj = _GetOrCreateShaderObject(
            surfaceShader, context);
    if (surfaceShaderObj.isNull()) {
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

    MFnDependencyNode depNodeFn(surfaceShaderObj, &status);
    if (status != MS::kSuccess) {
        return MObject();
    }

    MPlug shaderOutputPlug =
        depNodeFn.findPlug(_tokens->outColor.GetText(), &status);
    if (status != MS::kSuccess || shaderOutputPlug.isNull()) {
        return MObject();
    }

    MPlug seInputPlug =
        fnSet.findPlug(surfaceShaderPlugName.GetText(), &status);
    CHECK_MSTATUS_AND_RETURN(status, MObject());

    UsdMayaUtil::Connect(shaderOutputPlug,
                            seInputPlug,
                            /* clearDstPlug = */ true);
    return shadingEngine;
}

} // namespace

PXR_NAMESPACE_OPEN_SCOPE

DEFINE_SHADING_MODE_IMPORTER(previewSurface, context)
{
    MObject outputNode = MakePreviewSurfaceShader(context);
    if (!outputNode.isNull()) {
        TF_DEBUG(PXRUSDMAYA_PREVIEWSURFACE_IMPORT).Msg(
                "Successfully made preview shader for %s!",
                context->GetBoundPrim().GetPath().GetText());
        return outputNode;
    }
    // Fallback to displayColor

    TF_DEBUG(PXRUSDMAYA_PREVIEWSURFACE_IMPORT).Msg(
            "Unable to make preview shader for %s - falling back to display"
            " color", context->GetBoundPrim().GetPath().GetText());

    return MakeDisplayColorShader(context);
}


PXR_NAMESPACE_CLOSE_SCOPE
