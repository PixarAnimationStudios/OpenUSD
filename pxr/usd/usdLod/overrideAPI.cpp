//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLod/overrideAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLodOverrideAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLodOverrideAPI::~UsdLodOverrideAPI()
{
}

/* static */
UsdLodOverrideAPI
UsdLodOverrideAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLodOverrideAPI();
    }
    return UsdLodOverrideAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLodOverrideAPI::_GetSchemaKind() const
{
    return UsdLodOverrideAPI::schemaKind;
}

/* static */
bool
UsdLodOverrideAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLodOverrideAPI>(whyNot);
}

/* static */
UsdLodOverrideAPI
UsdLodOverrideAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLodOverrideAPI>()) {
        return UsdLodOverrideAPI(prim);
    }
    return UsdLodOverrideAPI();
}

/* static */
const TfType &
UsdLodOverrideAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLodOverrideAPI>();
    return tfType;
}

/* static */
bool 
UsdLodOverrideAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLodOverrideAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLodOverrideAPI::GetLodOverrideModeAttr() const
{
    return GetPrim().GetAttribute(UsdLodTokens->lodOverrideMode);
}

UsdAttribute
UsdLodOverrideAPI::CreateLodOverrideModeAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLodTokens->lodOverrideMode,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdLodOverrideAPI::GetLodOverrideIndexAttr() const
{
    return GetPrim().GetAttribute(UsdLodTokens->lodOverrideIndex);
}

UsdAttribute
UsdLodOverrideAPI::CreateLodOverrideIndexAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLodTokens->lodOverrideIndex,
                       SdfValueTypeNames->Float,
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
UsdLodOverrideAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLodTokens->lodOverrideMode,
        UsdLodTokens->lodOverrideIndex,
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

PXR_NAMESPACE_OPEN_SCOPE

TfToken
UsdLodOverrideAPI::ComputeLODOverride(
    float* overrideIndex,
    UsdTimeCode time /* = UsdTimeCode::Default() */) const
{
    TfToken mode = UsdLodTokens->inherited;

    UsdPrim prim = GetPrim();
    if (!prim) {
        return mode;
    }

    UsdPrim pseudoRoot = prim.GetStage()->GetPseudoRoot();

    while (prim != pseudoRoot){
        UsdLodOverrideAPI overrideAPI = UsdLodOverrideAPI(prim);
        if (overrideAPI) {
            UsdAttribute modeAttr = overrideAPI.GetLodOverrideModeAttr();

            if (modeAttr.HasAuthoredValue() && 
                modeAttr.Get(&mode, time) &&
                mode != UsdLodTokens->inherited)
            {
                if (mode == UsdLodTokens->indexedLOD) {
                    UsdAttribute indexAttr =
                        overrideAPI.GetLodOverrideIndexAttr();
                    if (!indexAttr.Get(overrideIndex, time)) {
                        // Get failed, overrideIndex is unchanged.
                        TF_WARN("Unable to get float value for %s, treating"
                                " '%s' override mode as %s at %s.",
                                indexAttr.GetPath().GetText(),
                                UsdLodTokens->indexedLOD.GetString().c_str(),
                                UsdLodTokens->inherited.GetString().c_str(),
                                modeAttr.GetPath().GetText());

                        mode = UsdLodTokens->inherited;
                        goto next;
                    }
                }
                
                return mode;
            }
        }

    next:
        prim = prim.GetParent();
    }

    return UsdLodTokens->noOverride;
}

PXR_NAMESPACE_CLOSE_SCOPE
