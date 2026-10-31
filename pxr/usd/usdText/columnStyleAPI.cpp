//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdText/columnStyleAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdTextColumnStyleAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdTextColumnStyleAPI::~UsdTextColumnStyleAPI()
{
}

/* static */
UsdTextColumnStyleAPI
UsdTextColumnStyleAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdTextColumnStyleAPI();
    }
    return UsdTextColumnStyleAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdTextColumnStyleAPI::_GetSchemaKind() const
{
    return UsdTextColumnStyleAPI::schemaKind;
}

/* static */
bool
UsdTextColumnStyleAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdTextColumnStyleAPI>(whyNot);
}

/* static */
UsdTextColumnStyleAPI
UsdTextColumnStyleAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdTextColumnStyleAPI>()) {
        return UsdTextColumnStyleAPI(prim);
    }
    return UsdTextColumnStyleAPI();
}

/* static */
const TfType &
UsdTextColumnStyleAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdTextColumnStyleAPI>();
    return tfType;
}

/* static */
bool 
UsdTextColumnStyleAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdTextColumnStyleAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdTextColumnStyleAPI::GetSchemaAttributeNames(bool includeInherited)
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
UsdTextColumnStyleAPI::GetBindingRel() const
{
    return GetPrim().GetRelationship(UsdTextTokens->columnStyleBinding);
}

UsdTextColumnStyleAPI::ColumnStyleBinding::ColumnStyleBinding(
    const UsdRelationship &bindingRel, SdfPath const& textPrimPath) :
    _bindingRel(bindingRel)
{
    // Get the columnstyle path. One text prim can bind several column styles.
    SdfPathVector targetPaths;
    _bindingRel.GetForwardedTargets(&targetPaths);
    if (targetPaths.size() >= 1 &&
        targetPaths.front().IsPrimPath()) {
        for (auto path : targetPaths)
            _columnStylePaths.push_back(path);
    }

    // Add a binding to the cache.
    if (!_columnStylePaths.empty()) {
        for (auto path : _columnStylePaths)
            UsdTextColumnStyleAPI::AddBindToCache(path, textPrimPath);
    }

}

std::vector<UsdTextColumnStyle>
UsdTextColumnStyleAPI::ColumnStyleBinding::GetColumnStyles() const
{
    std::vector<UsdTextColumnStyle> columnStyles;
    // Get the columnstyle prim.
    if (!_columnStylePaths.empty()) {
        for(auto path : _columnStylePaths)
            columnStyles.push_back(UsdTextColumnStyle(_bindingRel.GetStage()->GetPrimAtPath(
                path)));
    }
    return columnStyles;
}

UsdTextColumnStyleAPI::ColumnStyleBinding
UsdTextColumnStyleAPI::GetColumnStyleBinding(SdfPath const& primPath) const
{
    UsdRelationship bindingRel = GetBindingRel();
    return ColumnStyleBinding(bindingRel, primPath);
}

UsdRelationship
UsdTextColumnStyleAPI::_CreateBindingRel() const
{
    return GetPrim().CreateRelationship(UsdTextTokens->columnStyleBinding, /*custom*/ false);
}

bool
UsdTextColumnStyleAPI::Bind(const std::vector<UsdTextColumnStyle> columnStyles) const
{
    if (UsdRelationship bindingRel = _CreateBindingRel())
    {
        SdfPathVector targetPaths;
        for (auto columnStyle:columnStyles)
            targetPaths.push_back(columnStyle.GetPath());

        return bindingRel.SetTargets(targetPaths);
    }

    return false;
}

bool
UsdTextColumnStyleAPI::CanContainPropertyName(const TfToken &name)
{
    return TfStringStartsWith(name, UsdTextTokens->columnStyleBinding);
}

UsdTextColumnStyleAPI::ColumnStyleBindingCache UsdTextColumnStyleAPI::_styleBindingCache;

PXR_NAMESPACE_CLOSE_SCOPE
