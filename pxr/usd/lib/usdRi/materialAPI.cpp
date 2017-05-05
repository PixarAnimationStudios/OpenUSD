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

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdRiMaterialAPI,
        TfType::Bases< UsdSchemaBase > >();
    
}

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
                       SdfVariabilityUniform,
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
                       SdfVariabilityUniform,
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
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRiMaterialAPI::GetBxdfAttr() const
{
    return GetPrim().GetAttribute(UsdRiTokens->outputsRiBxdf);
}

UsdAttribute
UsdRiMaterialAPI::CreateBxdfAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRiTokens->outputsRiBxdf,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
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
        UsdRiTokens->outputsRiBxdf,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdSchemaBase::GetSchemaAttributeNames(true),
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

#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/material.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((defaultOutputName, "outputs:out"))

    // These tokens are required for backwards compatibility. They're 
    // redefined here so we can stop relying on UsdRiLookAPI entirely.
    (ri)
    ((riLookBxdf, "riLook:bxdf"))
    ((riLookDisplacement, "riLook:displacement"))
    ((riLookSurface, "riLook:surface"))
    ((riLookVolume, "riLook:volume"))


);

template <typename ShaderType>
ShaderType
UsdRiMaterialAPI::_GetSourceShaderObject(const UsdShadeOutput &output,
                                         bool ignoreBaseMaterial) const
{
    // If output doesn't have a valid property, return an invalid shader.
    if (!output.GetProperty()) {
        return ShaderType();
    }

    if (ignoreBaseMaterial && 
        UsdShadeConnectableAPI::IsSourceFromBaseMaterial(output)) {
        return ShaderType();
    }

    UsdShadeConnectableAPI source;
    TfToken sourceName;
    UsdShadeAttributeType sourceType;

    if (UsdShadeConnectableAPI::GetConnectedSource(output, 
            &source, &sourceName, &sourceType)) {
        return ShaderType(source.GetPrim());
    }

    return ShaderType();
}

UsdShadeOutput 
UsdRiMaterialAPI::_GetShadeOutput(const UsdAttribute &outputAttr, 
                                  const TfToken &oldEncodingRelName) const
{
    if (outputAttr) {
        return UsdShadeOutput(outputAttr);
    } else if (UsdShadeUtils::ReadOldEncoding()) {
        if (UsdRelationship rel = GetPrim().GetRelationship(oldEncodingRelName))
        {
            return UsdShadeOutput(rel);
        }
    }
    return UsdShadeOutput();
}

UsdRiRslShader 
UsdRiMaterialAPI::GetSurface(bool ignoreBaseMaterial) const
{
    return _GetSourceShaderObject<UsdRiRslShader>(GetSurfaceOutput(), 
                                                  ignoreBaseMaterial);
}

UsdRiRslShader 
UsdRiMaterialAPI::GetDisplacement(bool ignoreBaseMaterial) const
{
    return _GetSourceShaderObject<UsdRiRslShader>(GetDisplacementOutput(), 
                                                  ignoreBaseMaterial);
}

UsdRiRslShader 
UsdRiMaterialAPI::GetVolume(bool ignoreBaseMaterial) const
{
    return _GetSourceShaderObject<UsdRiRslShader>(GetVolumeOutput(), 
                                                  ignoreBaseMaterial);    
}

UsdRiRisBxdf 
UsdRiMaterialAPI::GetBxdf(bool ignoreBaseMaterial) const
{
    return _GetSourceShaderObject<UsdRiRisBxdf>(GetBxdfOutput(), 
                                                ignoreBaseMaterial);
}

UsdShadeOutput 
UsdRiMaterialAPI::GetSurfaceOutput() const
{
    return _GetShadeOutput(GetSurfaceAttr(), _tokens->riLookSurface);
}

UsdShadeOutput 
UsdRiMaterialAPI::GetDisplacementOutput() const
{
    return _GetShadeOutput(GetDisplacementAttr(), _tokens->riLookDisplacement);
}

UsdShadeOutput 
UsdRiMaterialAPI::GetVolumeOutput() const
{
    return _GetShadeOutput(GetVolumeAttr(), _tokens->riLookVolume);
}

UsdShadeOutput 
UsdRiMaterialAPI::GetBxdfOutput() const
{
    return _GetShadeOutput(GetBxdfAttr(), _tokens->riLookBxdf);
}

bool
UsdRiMaterialAPI::SetSurfaceSource(const SdfPath &surfacePath) const
{
    if (UsdShadeUtils::WriteNewEncoding()) {
        UsdShadeOutput surfaceOutput(CreateSurfaceAttr());
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
        UsdShadeOutput displacementOutput(CreateDisplacementAttr());
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
        UsdShadeOutput volumeOutput(CreateDisplacementAttr());
        return UsdShadeConnectableAPI::ConnectToSource(
            volumeOutput, volumePath.IsPropertyPath() ? 
                volumePath :
                volumePath.AppendProperty(_tokens->defaultOutputName));
    } else if (UsdRelationship volumeRel = GetPrim().CreateRelationship(
                    _tokens->riLookDisplacement, /*custom*/ false)) {
        return volumeRel.SetTargets(std::vector<SdfPath>{volumePath});
    }
    return false;
}

bool
UsdRiMaterialAPI::SetBxdfSource(const SdfPath &bxdfPath) const
{
    if (UsdShadeUtils::WriteNewEncoding()) {
        UsdShadeOutput bxdfOutput(CreateBxdfAttr());
        return UsdShadeConnectableAPI::ConnectToSource(
            bxdfOutput, bxdfPath.IsPropertyPath() ? 
                bxdfPath :
                bxdfPath.AppendProperty(_tokens->defaultOutputName));
    } else if (UsdRelationship bxdfRel = GetPrim().CreateRelationship(
                    _tokens->riLookBxdf, /*custom*/ false)) {
        return bxdfRel.SetTargets(std::vector<SdfPath>{bxdfPath});
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
