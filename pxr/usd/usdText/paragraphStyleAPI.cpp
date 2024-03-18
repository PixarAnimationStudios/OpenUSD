//
// Copyright 2023 Pixar
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
#include "pxr/usd/usdText/paragraphStyleAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdTextParagraphStyleAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdTextParagraphStyleAPI::~UsdTextParagraphStyleAPI()
{
}

/* static */
UsdTextParagraphStyleAPI
UsdTextParagraphStyleAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdTextParagraphStyleAPI();
    }
    return UsdTextParagraphStyleAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdTextParagraphStyleAPI::_GetSchemaKind() const
{
    return UsdTextParagraphStyleAPI::schemaKind;
}

/* static */
bool
UsdTextParagraphStyleAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdTextParagraphStyleAPI>(whyNot);
}

/* static */
UsdTextParagraphStyleAPI
UsdTextParagraphStyleAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdTextParagraphStyleAPI>()) {
        return UsdTextParagraphStyleAPI(prim);
    }
    return UsdTextParagraphStyleAPI();
}

/* static */
const TfType &
UsdTextParagraphStyleAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdTextParagraphStyleAPI>();
    return tfType;
}

/* static */
bool 
UsdTextParagraphStyleAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdTextParagraphStyleAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdTextParagraphStyleAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdAPISchemaBase::GetSchemaAttributeNames(true);

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
PXR_NAMESPACE_OPEN_SCOPE
UsdRelationship
UsdTextParagraphStyleAPI::GetBindingRel() const
{
    return GetPrim().GetRelationship(UsdTextTokens->paragraphStyleBinding);
}

UsdTextParagraphStyleAPI::ParagraphStyleBinding::ParagraphStyleBinding(
    const UsdRelationship &bindingRel, SdfPath const& textPrimPath) :
    _bindingRel(bindingRel)
{
    // Get the paragraphstyle path.
    SdfPathVector targetPaths;
    _bindingRel.GetForwardedTargets(&targetPaths);
    if (targetPaths.size() >= 1 &&
        targetPaths.front().IsPrimPath()) {
        for (auto path : targetPaths)
            _paragraphStylePaths.push_back(path);
    }

    // Add a binding to the cache.
    if (!_paragraphStylePaths.empty()) {
        for (auto path : _paragraphStylePaths)
            UsdTextParagraphStyleAPI::AddBindToCache(path, textPrimPath);
    }

}

std::vector<UsdTextParagraphStyle>
UsdTextParagraphStyleAPI::ParagraphStyleBinding::GetParagraphStyles() const
{
    std::vector<UsdTextParagraphStyle> paragraphStyles;
    // Get the paragraphstyle prim.
    if (!_paragraphStylePaths.empty()) {
        for(auto path : _paragraphStylePaths)
            paragraphStyles.push_back(UsdTextParagraphStyle(_bindingRel.GetStage()->GetPrimAtPath(
                path)));
    }
    return paragraphStyles;
}

UsdTextParagraphStyleAPI::ParagraphStyleBinding
UsdTextParagraphStyleAPI::GetParagraphStyleBinding(SdfPath const& primPath) const
{
    UsdRelationship bindingRel = GetBindingRel();
    return ParagraphStyleBinding(bindingRel, primPath);
}

UsdRelationship
UsdTextParagraphStyleAPI::_CreateBindingRel() const
{
    return GetPrim().CreateRelationship(UsdTextTokens->paragraphStyleBinding, /*custom*/ false);
}

bool
UsdTextParagraphStyleAPI::Bind(const std::vector<UsdTextParagraphStyle> paragraphStyles) const
{
    if (UsdRelationship bindingRel = _CreateBindingRel())
    {
        SdfPathVector targetPaths;
        for (auto paragraphStyle:paragraphStyles)
            targetPaths.push_back(paragraphStyle.GetPath());
            
        return bindingRel.SetTargets(targetPaths);
    }

    return false;
}

bool
UsdTextParagraphStyleAPI::CanContainPropertyName(const TfToken &name)
{
    return TfStringStartsWith(name, UsdTextTokens->paragraphStyle);
}

UsdTextParagraphStyleAPI::ParagraphStyleBindingCache UsdTextParagraphStyleAPI::_styleBindingCache;
PXR_NAMESPACE_CLOSE_SCOPE
