//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    return TfStringStartsWith(name, UsdTextTokens->paragraphStyleBinding);
}

UsdTextParagraphStyleAPI::ParagraphStyleBindingCache UsdTextParagraphStyleAPI::_styleBindingCache;
PXR_NAMESPACE_CLOSE_SCOPE
