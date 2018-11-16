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
#include "pxrUsdTranslators/fileTextureWriter.h"

#include "usdMaya/primWriterRegistry.h"
#include "usdMaya/shaderWriter.h"
#include "usdMaya/util.h"
#include "usdMaya/writeJobContext.h"

#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/usdShade/input.h"
#include "pxr/usd/usdShade/output.h"
#include "pxr/usd/usdShade/shader.h"
#include "pxr/usd/usdUtils/pipeline.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include <maya/MFnDependencyNode.h>
#include <maya/MObject.h>
#include <maya/MPlug.h>
#include <maya/MStatus.h>
#include <maya/MString.h>


PXR_NAMESPACE_OPEN_SCOPE


PXRUSDMAYA_REGISTER_WRITER(file, PxrUsdTranslators_FileTextureWriter);


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // Maya "file" node attribute names
    (alphaGain)
    (alphaOffset)
    (colorGain)
    (colorOffset)
    (defaultColor)
    (fileTextureName)
    (outAlpha)
    (outColor)
    (wrapU)
    (wrapV)

    // UsdPrimvarReader_float2 Prim Name
    ((PrimvarReaderShaderName, "TexCoordReader"))

    // UsdPrimvarReader_float2 Input Names
    (varname)

    // UsdPrimvarReader_float2 Output Name
    (result)

    // UsdUVTexture Input Names
    (bias)
    (fallback)
    (file)
    (scale)
    (st)
    (wrapS)
    (wrapT)

    // Values for wrapS and wrapT
    (black)
    (repeat)

    // UsdUVTexture Output Name
    ((TextureOutputName, "rgba"))
);


PxrUsdTranslators_FileTextureWriter::PxrUsdTranslators_FileTextureWriter(
        const MFnDependencyNode& depNodeFn,
        const SdfPath& usdPath,
        UsdMayaWriteJobContext& jobCtx) :
    UsdMayaShaderWriter(depNodeFn, usdPath, jobCtx)
{
    // Create a UsdUVTexture shader as the "primary" shader for this writer.
    UsdShadeShader texShaderSchema =
        UsdShadeShader::Define(GetUsdStage(), GetUsdPath());
    TF_AXIOM(texShaderSchema);

    texShaderSchema.CreateIdAttr(VtValue(UsdImagingTokens->UsdUVTexture));

    _usdPrim = texShaderSchema.GetPrim();
    TF_AXIOM(_usdPrim);

    texShaderSchema.CreateOutput(
        _tokens->TextureOutputName,
        SdfValueTypeNames->Float4);

    // Now create a UsdPrimvarReader shader that the UsdUvTexture shader will
    // use.
    const SdfPath primvarReaderShaderPath =
        texShaderSchema.GetPath().AppendChild(_tokens->PrimvarReaderShaderName);
    UsdShadeShader primvarReaderShaderSchema =
        UsdShadeShader::Define(GetUsdStage(), primvarReaderShaderPath);

    primvarReaderShaderSchema.CreateIdAttr(
        VtValue(UsdImagingTokens->UsdPrimvarReader_float2));

    // XXX: We'll eventually need to to determine which UV set to use if we're
    // not using the default (i.e. "map1" in Maya -> "st" in USD).
    primvarReaderShaderSchema.CreateInput(
        _tokens->varname,
        SdfValueTypeNames->Token).Set(UsdUtilsGetPrimaryUVSetName());

    UsdShadeOutput primvarReaderOutput =
        primvarReaderShaderSchema.CreateOutput(
            _tokens->result,
            SdfValueTypeNames->Float2);

    // Connect the output of the primvar reader to the texture coordinate
    // input of the UV texture.
    texShaderSchema.CreateInput(
        _tokens->st,
        SdfValueTypeNames->Float2).ConnectToSource(primvarReaderOutput);
}

/* virtual */
void
PxrUsdTranslators_FileTextureWriter::Write(const UsdTimeCode& usdTime)
{
    UsdMayaShaderWriter::Write(usdTime);

    MStatus status;

    const MFnDependencyNode depNodeFn(GetMayaObject(), &status);
    if (status != MS::kSuccess) {
        return;
    }

    UsdShadeShader shaderSchema(_usdPrim);
    TF_AXIOM(shaderSchema);

    // File
    const MPlug fileTextureNamePlug =
        depNodeFn.findPlug(
            _tokens->fileTextureName.GetText(),
            /* wantNetworkedPlug = */ true,
            &status);
    if (status != MS::kSuccess) {
        return;
    }


    const MString fileTextureName =
#if MAYA_API_VERSION >= 20180000
        fileTextureNamePlug.asString(&status);
#else
        fileTextureNamePlug.asString(MDGContext::fsNormal, &status);
#endif
    if (status != MS::kSuccess) {
        return;
    }

    shaderSchema.CreateInput(
        _tokens->file,
        SdfValueTypeNames->Asset).Set(
            SdfAssetPath(fileTextureName.asChar()),
            usdTime);

    // The Maya file node's 'colorGain' and 'alphaGain' attributes map to the
    // UsdUVTexture's scale input.
    bool isScaleAuthored = false;
    GfVec4f scale(1.0f, 1.0f, 1.0f, 1.0f);

    // Color Gain
    const MPlug colorGainPlug =
        depNodeFn.findPlug(
            _tokens->colorGain.GetText(),
            /* wantNetworkedPlug = */ true,
            &status);
    if (status != MS::kSuccess) {
        return;
    }

    if (UsdMayaUtil::IsAuthored(colorGainPlug)) {
        for (size_t i = 0u; i < GfVec3f::dimension; ++i) {
            scale[i] =
#if MAYA_API_VERSION >= 20180000
                colorGainPlug.child(i).asFloat(&status);
#else
                colorGainPlug.child(i).asFloat(MDGContext::fsNormal, &status);
#endif
            if (status != MS::kSuccess) {
                return;
            }
        }

        isScaleAuthored = true;
    }

    // Alpha Gain
    const MPlug alphaGainPlug =
        depNodeFn.findPlug(
            _tokens->alphaGain.GetText(),
            /* wantNetworkedPlug = */ true,
            &status);
    if (status != MS::kSuccess) {
        return;
    }

    if (UsdMayaUtil::IsAuthored(alphaGainPlug)) {
        scale[3u] =
#if MAYA_API_VERSION >= 20180000
            alphaGainPlug.asFloat(&status);
#else
            alphaGainPlug.asFloat(MDGContext::fsNormal, &status);
#endif
        if (status != MS::kSuccess) {
            return;
        }

        isScaleAuthored = true;
    }

    if (isScaleAuthored) {
        shaderSchema.CreateInput(
            _tokens->scale,
            SdfValueTypeNames->Float4).Set(scale, usdTime);
    }

    // The Maya file node's 'colorOffset' and 'alphaOffset' attributes map to
    // the UsdUVTexture's bias input.
    bool isBiasAuthored = false;
    GfVec4f bias(0.0f, 0.0f, 0.0f, 0.0f);

    // Color Offset
    const MPlug colorOffsetPlug =
        depNodeFn.findPlug(
            _tokens->colorOffset.GetText(),
            /* wantNetworkedPlug = */ true,
            &status);
    if (status != MS::kSuccess) {
        return;
    }

    if (UsdMayaUtil::IsAuthored(colorOffsetPlug)) {
        for (size_t i = 0u; i < GfVec3f::dimension; ++i) {
            bias[i] =
#if MAYA_API_VERSION >= 20180000
                colorOffsetPlug.child(i).asFloat(&status);
#else
                colorOffsetPlug.child(i).asFloat(MDGContext::fsNormal, &status);
#endif
            if (status != MS::kSuccess) {
                return;
            }
        }

        isBiasAuthored = true;
    }

    // Alpha Offset
    const MPlug alphaOffsetPlug =
        depNodeFn.findPlug(
            _tokens->alphaOffset.GetText(),
            /* wantNetworkedPlug = */ true,
            &status);
    if (status != MS::kSuccess) {
        return;
    }

    if (UsdMayaUtil::IsAuthored(alphaOffsetPlug)) {
        bias[3u] =
#if MAYA_API_VERSION >= 20180000
            alphaOffsetPlug.asFloat(&status);
#else
            alphaOffsetPlug.asFloat(MDGContext::fsNormal, &status);
#endif
        if (status != MS::kSuccess) {
            return;
        }

        isBiasAuthored = true;
    }

    if (isBiasAuthored) {
        shaderSchema.CreateInput(
            _tokens->bias,
            SdfValueTypeNames->Float4).Set(bias, usdTime);
    }

    // Default Color
    const MPlug defaultColorPlug =
        depNodeFn.findPlug(
            _tokens->defaultColor.GetText(),
            /* wantNetworkedPlug = */ true,
            &status);
    if (status != MS::kSuccess) {
        return;
    }

    // The defaultColor plug does not include an alpha, so only look for
    // three components, even though we're putting the values in a GfVec4f.
    // We also don't check whether it is authored in Maya, since Maya's
    // unauthored value (0.5, 0.5, 0.5) differs from UsdUVTexture's fallback
    // value.
    GfVec4f fallback(0.0f, 0.0f, 0.0f, 1.0f);
    for (size_t i = 0u; i < GfVec3f::dimension; ++i) {
        fallback[i] =
#if MAYA_API_VERSION >= 20180000
            defaultColorPlug.child(i).asFloat(&status);
#else
            defaultColorPlug.child(i).asFloat(MDGContext::fsNormal, &status);
#endif
        if (status != MS::kSuccess) {
            return;
        }
    }

    shaderSchema.CreateInput(
        _tokens->fallback,
        SdfValueTypeNames->Float4).Set(fallback, usdTime);

    // Wrap U
    const MPlug wrapUPlug =
        depNodeFn.findPlug(
            _tokens->wrapU.GetText(),
            /* wantNetworkedPlug = */ true,
            &status);
    if (status != MS::kSuccess) {
        return;
    }

    if (UsdMayaUtil::IsAuthored(wrapUPlug)) {
        const bool wrapU =
#if MAYA_API_VERSION >= 20180000
            wrapUPlug.asBool(&status);
#else
            wrapUPlug.asBool(MDGContext::fsNormal, &status);
#endif
        if (status != MS::kSuccess) {
            return;
        }

        const TfToken wrapS = wrapU ? _tokens->repeat : _tokens->black;
        shaderSchema.CreateInput(
            _tokens->wrapS,
            SdfValueTypeNames->Token).Set(wrapS, usdTime);
    }

    // Wrap V
    const MPlug wrapVPlug =
        depNodeFn.findPlug(
            _tokens->wrapV.GetText(),
            /* wantNetworkedPlug = */ true,
            &status);
    if (status != MS::kSuccess) {
        return;
    }

    if (UsdMayaUtil::IsAuthored(wrapVPlug)) {
        const bool wrapV =
#if MAYA_API_VERSION >= 20180000
            wrapVPlug.asBool(&status);
#else
            wrapVPlug.asBool(MDGContext::fsNormal, &status);
#endif
        if (status != MS::kSuccess) {
            return;
        }

        const TfToken wrapT = wrapV ? _tokens->repeat : _tokens->black;
        shaderSchema.CreateInput(
            _tokens->wrapT,
            SdfValueTypeNames->Token).Set(wrapT, usdTime);
    }
}

/* virtual */
TfToken
PxrUsdTranslators_FileTextureWriter::GetShadingPropertyNameForMayaAttrName(
        const TfToken& mayaAttrName)
{
    if (!_usdPrim) {
        return TfToken();
    }

    TfToken usdAttrName;

    if (mayaAttrName == _tokens->outColor ||
            mayaAttrName == _tokens->outAlpha) {
        usdAttrName =
            TfToken(
                TfStringPrintf(
                    "%s%s",
                    UsdShadeTokens->outputs.GetText(),
                    _tokens->TextureOutputName.GetText()).c_str());
    }

    return usdAttrName;
}


PXR_NAMESPACE_CLOSE_SCOPE
