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
#include "pxr/usd/usdRender/renderPass.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdRenderRenderPass,
        TfType::Bases< UsdTyped > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("RenderPass")
    // to find TfType<UsdRenderRenderPass>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdRenderRenderPass>("RenderPass");
}

/* virtual */
UsdRenderRenderPass::~UsdRenderRenderPass()
{
}

/* static */
UsdRenderRenderPass
UsdRenderRenderPass::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRenderRenderPass();
    }
    return UsdRenderRenderPass(stage->GetPrimAtPath(path));
}

/* static */
UsdRenderRenderPass
UsdRenderRenderPass::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("RenderPass");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRenderRenderPass();
    }
    return UsdRenderRenderPass(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdRenderRenderPass::_GetSchemaKind() const
{
    return UsdRenderRenderPass::schemaKind;
}

/* static */
const TfType &
UsdRenderRenderPass::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdRenderRenderPass>();
    return tfType;
}

/* static */
bool 
UsdRenderRenderPass::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdRenderRenderPass::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdRenderRenderPass::GetPassTypeAttr() const
{
    return GetPrim().GetAttribute(UsdRenderTokens->passType);
}

UsdAttribute
UsdRenderRenderPass::CreatePassTypeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRenderTokens->passType,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRenderRenderPass::GetCommandAttr() const
{
    return GetPrim().GetAttribute(UsdRenderTokens->command);
}

UsdAttribute
UsdRenderRenderPass::CreateCommandAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRenderTokens->command,
                       SdfValueTypeNames->StringArray,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRenderRenderPass::GetFileNameAttr() const
{
    return GetPrim().GetAttribute(UsdRenderTokens->fileName);
}

UsdAttribute
UsdRenderRenderPass::CreateFileNameAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRenderTokens->fileName,
                       SdfValueTypeNames->Asset,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdRenderRenderPass::GetDenoiseEnableAttr() const
{
    return GetPrim().GetAttribute(UsdRenderTokens->denoiseEnable);
}

UsdAttribute
UsdRenderRenderPass::CreateDenoiseEnableAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdRenderTokens->denoiseEnable,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdRelationship
UsdRenderRenderPass::GetRenderSourceRel() const
{
    return GetPrim().GetRelationship(UsdRenderTokens->renderSource);
}

UsdRelationship
UsdRenderRenderPass::CreateRenderSourceRel() const
{
    return GetPrim().CreateRelationship(UsdRenderTokens->renderSource,
                       /* custom = */ false);
}

UsdRelationship
UsdRenderRenderPass::GetInputPassesRel() const
{
    return GetPrim().GetRelationship(UsdRenderTokens->inputPasses);
}

UsdRelationship
UsdRenderRenderPass::CreateInputPassesRel() const
{
    return GetPrim().CreateRelationship(UsdRenderTokens->inputPasses,
                       /* custom = */ false);
}

UsdRelationship
UsdRenderRenderPass::GetDenoisePassRel() const
{
    return GetPrim().GetRelationship(UsdRenderTokens->denoisePass);
}

UsdRelationship
UsdRenderRenderPass::CreateDenoisePassRel() const
{
    return GetPrim().CreateRelationship(UsdRenderTokens->denoisePass,
                       /* custom = */ false);
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
UsdRenderRenderPass::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdRenderTokens->passType,
        UsdRenderTokens->command,
        UsdRenderTokens->fileName,
        UsdRenderTokens->denoiseEnable,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdTyped::GetSchemaAttributeNames(true),
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
