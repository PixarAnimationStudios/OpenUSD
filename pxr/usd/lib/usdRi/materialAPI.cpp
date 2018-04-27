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
#include "pxr/usd/usdRi/materialAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdRiMaterialAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (RiMaterialAPI)
);

/* virtual */
UsdRiMaterialAPI::~UsdRiMaterialAPI()
{
}

/* static */
UsdRiMaterialAPI
UsdRiMaterialAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiMaterialAPI();
    }
    return UsdRiMaterialAPI(stage->GetPrimAtPath(path));
}

/*virtual*/
bool 
UsdRiMaterialAPI::_IsAppliedAPISchema() const 
{
    return true;
}

/* static */
UsdRiMaterialAPI
UsdRiMaterialAPI::Apply(const UsdPrim &prim)
{
    return UsdAPISchemaBase::_ApplyAPISchema<UsdRiMaterialAPI>(
            prim, _schemaTokens->RiMaterialAPI);
}

/* static */
const TfType &
UsdRiMaterialAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdRiMaterialAPI>();
    return tfType;
}

/* static */
bool 
UsdRiMaterialAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdRiMaterialAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdRiMaterialAPI::GetSurfaceAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->outputsRiSurface);
}

UsdAttribute
UsdRiMaterialAPI::CreateSurfaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->outputsRiSurface,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiMaterialAPI::GetDisplacementAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->outputsRiDisplacement);
}

UsdAttribute
UsdRiMaterialAPI::CreateDisplacementAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->outputsRiDisplacement,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiMaterialAPI::GetVolumeAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->outputsRiVolume);
}

UsdAttribute
UsdRiMaterialAPI::CreateVolumeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->outputsRiVolume,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

namespace {
static inline TfTokenVector
_ConcatenateAttributeNames(const TfTokenVector& left,const TfTokenVector& right)
{
    TfTokenVector result;
    result.reserve(left.size() + right.size());
    result.insert(result.end(), left.begin(), left.end());
    result.insert(result.end(), right.begin(), right.end());
    return result;
}
}

/*static*/
const TfTokenVector&
UsdRiMaterialAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdRiTokens->outputsRiSurface,
        UsdRiTokens->outputsRiDisplacement,
        UsdRiTokens->outputsRiVolume,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdAPISchemaBase::GetSchemaAttributeNames(true),
            localNames);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/base/tf/envSetting.h"
#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/material.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((defaultOutputName, "outputs:out"))

    // These tokens are required for backwards compatibility. They're 
    // redefined here so we can stop relying on UsdRiLookAPI entirely.
    (ri)
    ((riLookDisplacement, "riLook:displacement"))
    ((riLookSurface, "riLook:surface"))
    ((riLookVolume, "riLook:volume"))

    // deprecated tokens for handling backwards compatibility.
    ((bxdfOutputName, "ri:bxdf"))
    ((bxdfOutputAttrName, "outputs:ri:bxdf"))
    ((riLookBxdf, "riLook:bxdf"))
);

TF_DEFINE_ENV_SETTING(
    USD_RI_WRITE_BXDF_OUTPUT, true, 
    "If set to false, then \"ri:surface\" output is created instead of the "
    "\"ri:bxdf\" output, when UsdRiMaterialAPI::SetSurfaceSource() is "
    "invoked.");

UsdShadeShader
UsdRiMaterialAPI::_GetSourceShaderObject(const UsdShadeOutput &output,
                                         bool ignoreBaseMaterial) const
{
    // If output doesn't have a valid property, return an invalid shader.
    if (!output.GetProperty()) {
        return UsdShadeShader();
    }

    if (ignoreBaseMaterial && 
        UsdShadeConnectableAPI::IsSourceConnectionFromBaseMaterial(output)) {
        return UsdShadeShader();
    }

    UsdShadeConnectableAPI source;
    TfToken sourceName;
    UsdShadeAttributeType sourceType;

    if (UsdShadeConnectableAPI::GetConnectedSource(output, 
            &source, &sourceName, &sourceType)) {
        return source;
    }

    return UsdShadeShader();
}

UsdShadeOutput
UsdRiMaterialAPI::_GetBxdfOutput(const UsdPrim &materialPrim) const
{
    // Check if the deprecated bxdf output is present.
    if (const UsdAttribute bxdfAttr = materialPrim.GetAttribute(
            _tokens->bxdfOutputAttrName)) {
        return UsdShadeOutput(bxdfAttr);
    } else if (UsdShadeUtils::ReadOldEncoding()) {
        if (const UsdRelationship rel = materialPrim.GetRelationship(
                _tokens->riLookBxdf)) {
            return UsdShadeOutput(rel);
        }
    }
    return UsdShadeOutput();    
}

UsdShadeShader
UsdRiMaterialAPI::GetSurface(bool ignoreBaseMaterial) const
{
    if (UsdShadeShader surface = 
            _GetSourceShaderObject(GetSurfaceOutput(), ignoreBaseMaterial)) {
        return surface;
    }

    if (UsdShadeOutput bxdfOutput = _GetBxdfOutput(GetPrim())) {
        return _GetSourceShaderObject(bxdfOutput, ignoreBaseMaterial);
    }

    return UsdShadeShader();
}

UsdShadeShader
UsdRiMaterialAPI::GetDisplacement(bool ignoreBaseMaterial) const
{
    return _GetSourceShaderObject(GetDisplacementOutput(), ignoreBaseMaterial);
}

UsdShadeShader 
UsdRiMaterialAPI::GetVolume(bool ignoreBaseMaterial) const
{
    return _GetSourceShaderObject(GetVolumeOutput(), ignoreBaseMaterial);    
}

UsdShadeOutput 
UsdRiMaterialAPI::GetSurfaceOutput() const
{
    const UsdShadeOutput op = UsdShadeMaterial(GetPrim()).GetSurfaceOutput(
            _tokens->ri);
    if (op) {
        return op;
    }

    if (const UsdRelationship rel = GetPrim().GetRelationship(
                _tokens->riLookSurface)) {
        return UsdShadeOutput(rel);
    }

    return op;
}

UsdShadeOutput 
UsdRiMaterialAPI::GetDisplacementOutput() const
{
    const UsdShadeOutput op = UsdShadeMaterial(GetPrim()).GetDisplacementOutput(
            _tokens->ri);
    if (op) {
        return op;
    }

    if (const UsdRelationship rel = GetPrim().GetRelationship(
                _tokens->riLookDisplacement)) {
        return UsdShadeOutput(rel);
    }

    return op;
}

UsdShadeOutput 
UsdRiMaterialAPI::GetVolumeOutput() const
{
    const UsdShadeOutput op = UsdShadeMaterial(GetPrim()).GetVolumeOutput(
            _tokens->ri);
    if (op) {
        return op;
    }

    if (const UsdRelationship rel = GetPrim().GetRelationship(
                _tokens->riLookVolume)) {
        return UsdShadeOutput(rel);
    }

    return op;
}

bool
UsdRiMaterialAPI::SetSurfaceSource(const SdfPath &surfacePath) const
{
    static const bool writeBxdfOutput = 
            TfGetEnvSetting(USD_RI_WRITE_BXDF_OUTPUT);
    if (writeBxdfOutput) {
        if (UsdShadeUtils::WriteNewEncoding()) {
            if (UsdShadeOutput bxdfOutput = UsdShadeMaterial(GetPrim())
                    .CreateOutput(_tokens->bxdfOutputName, 
                                  SdfValueTypeNames->Token)) {
                const SdfPath sourcePath = surfacePath.IsPropertyPath() ? 
                    surfacePath : 
                    surfacePath.AppendProperty(_tokens->defaultOutputName);
                return UsdShadeConnectableAPI::ConnectToSource(
                    bxdfOutput, sourcePath);
            }
        } else if (UsdRelationship bxdfRel= GetPrim().CreateRelationship(
                    _tokens->riLookBxdf, /*custom*/ false)) {
            return bxdfRel.SetTargets(
                    std::vector<SdfPath>{surfacePath.GetPrimPath()});
        }
        return false;
    }

    if (UsdShadeUtils::WriteNewEncoding()) {
        UsdShadeOutput surfaceOutput = UsdShadeMaterial(GetPrim())
                .CreateSurfaceOutput(/*purpose*/ _tokens->ri);
        return UsdShadeConnectableAPI::ConnectToSource(
            surfaceOutput, surfacePath.IsPropertyPath() ? surfacePath :
                surfacePath.AppendProperty(_tokens->defaultOutputName));
    } else if (UsdRelationship surfaceRel = GetPrim().CreateRelationship(
                    _tokens->riLookSurface, /*custom*/ false)) {
        return surfaceRel.SetTargets(std::vector<SdfPath>{surfacePath});
    }

    
    return false;
}

bool
UsdRiMaterialAPI::SetDisplacementSource(const SdfPath &displacementPath) const
{
    if (UsdShadeUtils::WriteNewEncoding()) {
        UsdShadeOutput displacementOutput = UsdShadeMaterial(GetPrim())
                .CreateDisplacementOutput(/*purpose*/ _tokens->ri);
        return UsdShadeConnectableAPI::ConnectToSource(
            displacementOutput, displacementPath.IsPropertyPath() ? 
                displacementPath :
                displacementPath.AppendProperty(_tokens->defaultOutputName));
    } else if (UsdRelationship displacementRel = GetPrim().CreateRelationship(
                    _tokens->riLookDisplacement, /*custom*/ false)) {
        return displacementRel.SetTargets(
            std::vector<SdfPath>{displacementPath});
    }
    return false;
}

bool
UsdRiMaterialAPI::SetVolumeSource(const SdfPath &volumePath) const
{
    if (UsdShadeUtils::WriteNewEncoding()) {
        UsdShadeOutput volumeOutput = UsdShadeMaterial(GetPrim())
                .CreateVolumeOutput(/*purpose*/ _tokens->ri);
        return UsdShadeConnectableAPI::ConnectToSource(
            volumeOutput, volumePath.IsPropertyPath() ? 
                volumePath :
                volumePath.AppendProperty(_tokens->defaultOutputName));
    } else if (UsdRelationship volumeRel = GetPrim().CreateRelationship(
                    _tokens->riLookVolume, /*custom*/ false)) {
        return volumeRel.SetTargets(std::vector<SdfPath>{volumePath});
    }
    return false;
}

bool 
UsdRiMaterialAPI::SetInterfaceInputConsumer(UsdShadeInput &interfaceInput, 
                                            const UsdShadeInput &consumer) const
{
    return UsdShadeConnectableAPI::_ConnectToSource(consumer, 
        interfaceInput, _tokens->ri);
}

UsdShadeNodeGraph::InterfaceInputConsumersMap
UsdRiMaterialAPI::ComputeInterfaceInputConsumersMap(
        bool computeTransitiveConsumers) const
{
    return UsdShadeNodeGraph(GetPrim())._ComputeInterfaceInputConsumersMap(
        computeTransitiveConsumers, _tokens->ri);
}

std::vector<UsdShadeInput> 
UsdRiMaterialAPI::GetInterfaceInputs() const
{
    return UsdShadeMaterial(GetPrim())._GetInterfaceInputs(_tokens->ri);
}

PXR_NAMESPACE_CLOSE_SCOPE
