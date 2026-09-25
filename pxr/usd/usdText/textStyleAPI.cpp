//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdText/textStyleAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdTextTextStyleAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdTextTextStyleAPI::~UsdTextTextStyleAPI()
{
}

/* static */
UsdTextTextStyleAPI
UsdTextTextStyleAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdTextTextStyleAPI();
    }
    return UsdTextTextStyleAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdTextTextStyleAPI::_GetSchemaKind() const
{
    return UsdTextTextStyleAPI::schemaKind;
}

/* static */
bool
UsdTextTextStyleAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdTextTextStyleAPI>(whyNot);
}

/* static */
UsdTextTextStyleAPI
UsdTextTextStyleAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdTextTextStyleAPI>()) {
        return UsdTextTextStyleAPI(prim);
    }
    return UsdTextTextStyleAPI();
}

/* static */
const TfType &
UsdTextTextStyleAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdTextTextStyleAPI>();
    return tfType;
}

/* static */
bool 
UsdTextTextStyleAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdTextTextStyleAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdTextTextStyleAPI::GetSchemaAttributeNames(bool includeInherited)
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
UsdTextTextStyleAPI::GetBindingRel() const
{
    return GetPrim().GetRelationship(UsdTextTokens->textStyleBinding);
}

UsdTextTextStyleAPI::TextStyleBinding::TextStyleBinding(
    const UsdRelationship &bindingRel, SdfPath const& textPrimPath) :
    _bindingRel(bindingRel)
{
    // Get the textstyle path.
    SdfPathVector targetPaths;
    _bindingRel.GetForwardedTargets(&targetPaths);
    if (targetPaths.size() == 1 &&
        targetPaths.front().IsPrimPath()) {
        _textStylePath = targetPaths.front();
    }

    // Add a binding to the cache.
    if (!_textStylePath.IsEmpty()) {
        UsdTextTextStyleAPI::AddBindToCache(_textStylePath, textPrimPath);
    }
    
}

UsdTextTextStyle
UsdTextTextStyleAPI::TextStyleBinding::GetTextStyle() const
{
    // Get the textstyle prim.
    if (!_textStylePath.IsEmpty()) {
        return UsdTextTextStyle(_bindingRel.GetStage()->GetPrimAtPath(
            _textStylePath));
    }
    return UsdTextTextStyle();
}

UsdTextTextStyleAPI::TextStyleBinding
UsdTextTextStyleAPI::GetTextStyleBinding(SdfPath const& primPath) const
{
    UsdRelationship bindingRel = GetBindingRel();
    return TextStyleBinding(bindingRel, primPath);
}

bool
UsdTextTextStyleAPI::CanContainPropertyName(const TfToken &name)
{
    return TfStringStartsWith(name, UsdTextTokens->textStyleBinding);
}

UsdRelationship
UsdTextTextStyleAPI::_CreateBindingRel() const
{
    return GetPrim().CreateRelationship(UsdTextTokens->textStyleBinding, /*custom*/ false);
}


bool
UsdTextTextStyleAPI::Bind(const UsdTextTextStyle &textStyle) const
{
    if (UsdRelationship bindingRel = _CreateBindingRel()) {
        return bindingRel.SetTargets({ textStyle.GetPath() });
    }

    return false;
}

UsdTextTextStyleAPI::TextStyleBindingCache UsdTextTextStyleAPI::_styleBindingCache;
PXR_NAMESPACE_CLOSE_SCOPE
