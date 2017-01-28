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
#include "pxr/usd/usdRi/lookAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdRiLookAPI,
        TfType::Bases< UsdSchemaBase > >();
    
}

/* virtual */
UsdRiLookAPI::~UsdRiLookAPI()
{
}

/* static */
UsdRiLookAPI
UsdRiLookAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiLookAPI();
    }
    return UsdRiLookAPI(stage->GetPrimAtPath(path));
}


/* static */
const TfType &
UsdRiLookAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdRiLookAPI>();
    return tfType;
}

/* static */
bool 
UsdRiLookAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdRiLookAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdRelationship
UsdRiLookAPI::GetSurfaceRel() const
{
    return GetPrim().GetRelationship(UsdRiTokens->riLookSurface);
}

UsdRelationship
UsdRiLookAPI::CreateSurfaceRel() const
{
    return GetPrim().CreateRelationship(UsdRiTokens->riLookSurface,
                       /* custom = */ false);
}

UsdRelationship
UsdRiLookAPI::GetDisplacementRel() const
{
    return GetPrim().GetRelationship(UsdRiTokens->riLookDisplacement);
}

UsdRelationship
UsdRiLookAPI::CreateDisplacementRel() const
{
    return GetPrim().CreateRelationship(UsdRiTokens->riLookDisplacement,
                       /* custom = */ false);
}

UsdRelationship
UsdRiLookAPI::GetVolumeRel() const
{
    return GetPrim().GetRelationship(UsdRiTokens->riLookVolume);
}

UsdRelationship
UsdRiLookAPI::CreateVolumeRel() const
{
    return GetPrim().CreateRelationship(UsdRiTokens->riLookVolume,
                       /* custom = */ false);
}

UsdRelationship
UsdRiLookAPI::GetCoshadersRel() const
{
    return GetPrim().GetRelationship(UsdRiTokens->riLookCoshaders);
}

UsdRelationship
UsdRiLookAPI::CreateCoshadersRel() const
{
    return GetPrim().CreateRelationship(UsdRiTokens->riLookCoshaders,
                       /* custom = */ false);
}

UsdRelationship
UsdRiLookAPI::GetBxdfRel() const
{
    return GetPrim().GetRelationship(UsdRiTokens->riLookBxdf);
}

UsdRelationship
UsdRiLookAPI::CreateBxdfRel() const
{
    return GetPrim().CreateRelationship(UsdRiTokens->riLookBxdf,
                       /* custom = */ false);
}

UsdRelationship
UsdRiLookAPI::GetPatternsRel() const
{
    return GetPrim().GetRelationship(UsdRiTokens->riLookPatterns);
}

UsdRelationship
UsdRiLookAPI::CreatePatternsRel() const
{
    return GetPrim().CreateRelationship(UsdRiTokens->riLookPatterns,
                       /* custom = */ false);
}

/*static*/
const TfTokenVector&
UsdRiLookAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdSchemaBase::GetSchemaAttributeNames(true);

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

#include "pxr/usd/usdShade/material.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (ri)
);

static UsdRiRslShader
UsdRiLookAPI_GetSingleTargetShaderObject(const UsdRelationship &rel)
{
    if (rel) {
        SdfPathVector targetPaths;
        rel.GetForwardedTargets(&targetPaths);
        if ((targetPaths.size() == 1) && targetPaths.front().IsPrimPath()) {
            return UsdRiRslShader(
                rel.GetStage()->GetPrimAtPath(targetPaths.front()));
        }
    }
    return UsdRiRslShader();
}

UsdRiRslShader 
UsdRiLookAPI::GetSurface() const
{
    return UsdRiLookAPI_GetSingleTargetShaderObject(GetSurfaceRel());
}

UsdRiRslShader 
UsdRiLookAPI::GetDisplacement() const
{
    return UsdRiLookAPI_GetSingleTargetShaderObject(GetDisplacementRel());
}

UsdRiRslShader 
UsdRiLookAPI::GetVolume() const
{
    return UsdRiLookAPI_GetSingleTargetShaderObject(GetVolumeRel());
}

std::vector<UsdRiRslShader> 
UsdRiLookAPI::GetCoshaders() const
{
    std::vector<UsdRiRslShader> coshaders;

    if (UsdRelationship rel = GetCoshadersRel()) {
        SdfPathVector targetPaths;
        rel.GetForwardedTargets(&targetPaths);
        TF_FOR_ALL(path, targetPaths) {
            if (path->IsPrimPath()) {
                UsdRiRslShader coshader(
                    GetPrim().GetStage()->GetPrimAtPath(*path));
                if (coshader)
                    coshaders.push_back(coshader);
            }
        }
    }

    return coshaders;
}

UsdRiRisBxdf
UsdRiLookAPI::GetBxdf()
{
    if (UsdRelationship rel = GetBxdfRel()) {
        SdfPathVector targetPaths;
        rel.GetForwardedTargets(&targetPaths);
        if (targetPaths.size() == 1 && targetPaths.front().IsPrimPath()) {
            return UsdRiRisBxdf(
                GetPrim().GetStage()->GetPrimAtPath(targetPaths.front()));
        }
    }
    return UsdRiRisBxdf();
}

std::vector<UsdRiRisPattern>
UsdRiLookAPI::GetPatterns()
{
    std::vector<UsdRiRisPattern> patterns;

    if (UsdRelationship rel = GetPatternsRel()) {
        SdfPathVector targetPaths;
        rel.GetForwardedTargets(&targetPaths);
        TF_FOR_ALL(path, targetPaths) {
            if (path->IsPrimPath()) {
                UsdRiRisPattern pattern(
                    GetPrim().GetStage()->GetPrimAtPath(*path));
                if (pattern)
                    patterns.push_back(pattern);
            }
        }
    }

    return patterns;
}

bool
UsdRiLookAPI::SetInterfaceRecipient(
        UsdShadeInterfaceAttribute& interfaceAttr,
        const SdfPath& recipientPath)
{
    return interfaceAttr.SetRecipient(
            _tokens->ri,
            recipientPath);
}

bool
UsdRiLookAPI::SetInterfaceRecipient(
        UsdShadeInterfaceAttribute& interfaceAttr,
        const UsdShadeParameter& recipient)
{
    return interfaceAttr.SetRecipient(
            _tokens->ri,
            recipient);
}

std::vector<UsdShadeParameter>
UsdRiLookAPI::GetInterfaceRecipientParameters(
        const UsdShadeInterfaceAttribute& interfaceAttr) const
{
    return interfaceAttr.GetRecipientParameters(_tokens->ri);
}

std::vector<UsdShadeInterfaceAttribute> 
UsdRiLookAPI::GetInterfaceAttributes() const
{
    return UsdShadeMaterial(GetPrim()).GetInterfaceAttributes(_tokens->ri);
}

PXR_NAMESPACE_CLOSE_SCOPE

