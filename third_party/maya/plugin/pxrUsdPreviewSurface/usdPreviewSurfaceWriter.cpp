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
#include "pxrUsdPreviewSurface/usdPreviewSurfaceWriter.h"

#include "pxrUsdPreviewSurface/usdPreviewSurface.h"
#include "usdMaya/primWriterRegistry.h"
#include "usdMaya/shaderWriter.h"
#include "usdMaya/util.h"
#include "usdMaya/writeJobContext.h"
#include "usdMaya/writeUtil.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/output.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdShade/tokens.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>


PXR_NAMESPACE_OPEN_SCOPE


PXRUSDMAYA_REGISTER_WRITER(
    pxrUsdPreviewSurface,
    PxrMayaUsdPreviewSurface_Writer);


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // XXX: We duplicate this token here rather than create a dependency on
    // usdImaging in case the plugin is being built with imaging disabled.
    // If/when it moves out of usdImaging to a place that is always available,
    // it should be pulled from there instead.
    (UsdPreviewSurface)
);


PxrMayaUsdPreviewSurface_Writer::PxrMayaUsdPreviewSurface_Writer(
        const MFnDependencyNode& depNodeFn,
        const SdfPath& usdPath,
        UsdMayaWriteJobContext& jobCtx) :
    UsdMayaShaderWriter(depNodeFn, usdPath, jobCtx)
{
    UsdShadeShader shaderSchema =
        UsdShadeShader::Define(GetUsdStage(), GetUsdPath());
    if (!TF_VERIFY(
            shaderSchema,
            "Could not define UsdShadeShader at path '%s'\n",
            GetUsdPath().GetText())) {
        return;
    }

    shaderSchema.CreateIdAttr(VtValue(_tokens->UsdPreviewSurface));

    _usdPrim = shaderSchema.GetPrim();
    if (!TF_VERIFY(
            _usdPrim,
            "Could not get UsdPrim for UsdShadeShader at path '%s'\n",
            shaderSchema.GetPath().GetText())) {
        return;
    }

    // Surface Output
    shaderSchema.CreateOutput(
        UsdShadeTokens->surface,
        SdfValueTypeNames->Token);

    // Displacement Output
    shaderSchema.CreateOutput(
        UsdShadeTokens->displacement,
        SdfValueTypeNames->Token);
}

static
bool
_AuthorShaderInputFromShadingNodeAttr(
        const MFnDependencyNode& depNodeFn,
        const MObject& shadingNodeAttr,
        UsdShadeShader& shaderSchema,
        const TfToken& shaderInputName,
        const SdfValueTypeName& shaderInputTypeName,
        const UsdTimeCode usdTime,
        const bool mayaBoolAsUsdInt = false)
{
    MStatus status;

    // If the USD shader input type is int but the Maya attribute type is bool,
    // we do a conversion (e.g. for "useSpecularWorkflow").
    const bool convertBoolToInt = (mayaBoolAsUsdInt &&
        (shaderInputTypeName == SdfValueTypeNames->Int));

    MPlug shadingNodePlug =
        depNodeFn.findPlug(
            shadingNodeAttr,
            /* wantNetworkedPlug = */ true,
            &status);
    if (status != MS::kSuccess) {
        return false;
    }

    const bool isDestination = shadingNodePlug.isDestination(&status);
    if (status != MS::kSuccess) {
        return false;
    }

    if (UsdMayaUtil::IsAuthored(shadingNodePlug)) {
        // Color values are all linear on the shader, so do not re-linearize
        // them.
        VtValue value =
            UsdMayaWriteUtil::GetVtValue(
                shadingNodePlug,
                convertBoolToInt ?
                    SdfValueTypeNames->Bool :
                    shaderInputTypeName,
                /* linearizeColors = */ false);

        if (value.IsEmpty()) {
            return false;
        }

        UsdShadeInput shaderInput =
            shaderSchema.CreateInput(shaderInputName, shaderInputTypeName);

        // For attributes that are the destination of a connection, we create
        // the input on the shader but we do *not* author a value for it. We
        // expect its actual value to come from the source of its connection.
        // We'll leave it to the shading export to handle creating
        // the connections in USD.
        if (!isDestination) {
            if (convertBoolToInt) {
                if (value.UncheckedGet<bool>()) {
                    value = 1;
                } else {
                    value = 0;
                }
            }

            shaderInput.Set(value, usdTime);
        }
    }

    return true;
}

/* virtual */
void
PxrMayaUsdPreviewSurface_Writer::Write(const UsdTimeCode& usdTime)
{
    UsdMayaShaderWriter::Write(usdTime);

    MStatus status;

    const MFnDependencyNode depNodeFn(GetMayaObject(), &status);
    if (status != MS::kSuccess) {
        return;
    }

    UsdShadeShader shaderSchema(_usdPrim);
    if (!TF_VERIFY(
            shaderSchema,
            "Could not get UsdShadeShader schema for UsdPrim at path '%s'\n",
            _usdPrim.GetPath().GetText())) {
        return;
    }

    // Clearcoat
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        PxrMayaUsdPreviewSurface::clearcoatAttr,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->ClearcoatAttrName,
        SdfValueTypeNames->Float,
        usdTime);

    // Clearcoat Roughness
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        PxrMayaUsdPreviewSurface::clearcoatRoughnessAttr,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->ClearcoatRoughnessAttrName,
        SdfValueTypeNames->Float,
        usdTime);

    // Diffuse Color
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        PxrMayaUsdPreviewSurface::diffuseColorAttr,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName,
        SdfValueTypeNames->Color3f,
        usdTime);

    // Displacement
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        PxrMayaUsdPreviewSurface::displacementAttr,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->DisplacementAttrName,
        SdfValueTypeNames->Float,
        usdTime);

    // Emissive Color
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        PxrMayaUsdPreviewSurface::emissiveColorAttr,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName,
        SdfValueTypeNames->Color3f,
        usdTime);

    // Ior
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        PxrMayaUsdPreviewSurface::iorAttr,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->IorAttrName,
        SdfValueTypeNames->Float,
        usdTime);

    // Metallic
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        PxrMayaUsdPreviewSurface::metallicAttr,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->MetallicAttrName,
        SdfValueTypeNames->Float,
        usdTime);

    // Normal
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        PxrMayaUsdPreviewSurface::normalAttr,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->NormalAttrName,
        SdfValueTypeNames->Normal3f,
        usdTime);

    // Occlusion
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        PxrMayaUsdPreviewSurface::occlusionAttr,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->OcclusionAttrName,
        SdfValueTypeNames->Float,
        usdTime);

    // Opacity
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        PxrMayaUsdPreviewSurface::opacityAttr,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->OpacityAttrName,
        SdfValueTypeNames->Float,
        usdTime);

    // Roughness
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        PxrMayaUsdPreviewSurface::roughnessAttr,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName,
        SdfValueTypeNames->Float,
        usdTime);

    // Specular Color
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        PxrMayaUsdPreviewSurface::specularColorAttr,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->SpecularColorAttrName,
        SdfValueTypeNames->Color3f,
        usdTime);

    // Use Specular Workflow
    // The Maya attribute is bool-typed, while the USD attribute is int-typed.
    _AuthorShaderInputFromShadingNodeAttr(
        depNodeFn,
        PxrMayaUsdPreviewSurface::useSpecularWorkflowAttr,
        shaderSchema,
        PxrMayaUsdPreviewSurfaceTokens->UseSpecularWorkflowAttrName,
        SdfValueTypeNames->Int,
        usdTime,
        /* mayaBoolAsUsdInt = */ true);
}

/* virtual */
TfToken
PxrMayaUsdPreviewSurface_Writer::GetShadingPropertyNameForMayaAttrName(
        const TfToken& mayaAttrName)
{
    if (!_usdPrim) {
        return TfToken();
    }

    TfToken usdAttrName;

    if (mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->OutColorAttrName) {
        usdAttrName =
            TfToken(
                TfStringPrintf(
                    "%s%s",
                    UsdShadeTokens->outputs.GetText(),
                    UsdShadeTokens->surface.GetText()).c_str());
    }
    else if (mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->ClearcoatAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->ClearcoatRoughnessAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->DiffuseColorAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->DisplacementAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->EmissiveColorAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->IorAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->MetallicAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->NormalAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->OcclusionAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->OpacityAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->RoughnessAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->SpecularColorAttrName ||
            mayaAttrName == PxrMayaUsdPreviewSurfaceTokens->UseSpecularWorkflowAttrName) {
        usdAttrName =
            TfToken(
                TfStringPrintf(
                    "%s%s",
                    UsdShadeTokens->inputs.GetText(),
                    mayaAttrName.GetText()).c_str());
    }

    return usdAttrName;
}


PXR_NAMESPACE_CLOSE_SCOPE
