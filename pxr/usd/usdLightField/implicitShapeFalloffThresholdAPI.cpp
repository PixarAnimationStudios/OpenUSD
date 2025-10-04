//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdLightField/implicitShapeFalloffThresholdAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLightFieldImplicitShapeFalloffThresholdAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

/* virtual */
UsdLightFieldImplicitShapeFalloffThresholdAPI::~UsdLightFieldImplicitShapeFalloffThresholdAPI()
{
}

/* static */
UsdLightFieldImplicitShapeFalloffThresholdAPI
UsdLightFieldImplicitShapeFalloffThresholdAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLightFieldImplicitShapeFalloffThresholdAPI();
    }
    return UsdLightFieldImplicitShapeFalloffThresholdAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdLightFieldImplicitShapeFalloffThresholdAPI::_GetSchemaKind() const
{
    return UsdLightFieldImplicitShapeFalloffThresholdAPI::schemaKind;
}

/* static */
bool
UsdLightFieldImplicitShapeFalloffThresholdAPI::CanApply(
    const UsdPrim &prim, std::string *whyNot)
{
    return prim.CanApplyAPI<UsdLightFieldImplicitShapeFalloffThresholdAPI>(whyNot);
}

/* static */
UsdLightFieldImplicitShapeFalloffThresholdAPI
UsdLightFieldImplicitShapeFalloffThresholdAPI::Apply(const UsdPrim &prim)
{
    if (prim.ApplyAPI<UsdLightFieldImplicitShapeFalloffThresholdAPI>()) {
        return UsdLightFieldImplicitShapeFalloffThresholdAPI(prim);
    }
    return UsdLightFieldImplicitShapeFalloffThresholdAPI();
}

/* static */
const TfType &
UsdLightFieldImplicitShapeFalloffThresholdAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLightFieldImplicitShapeFalloffThresholdAPI>();
    return tfType;
}

/* static */
bool 
UsdLightFieldImplicitShapeFalloffThresholdAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLightFieldImplicitShapeFalloffThresholdAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdLightFieldImplicitShapeFalloffThresholdAPI::GetKernelFalloffImplicitShapeFalloffThresholdAttr() const
{
    return GetPrim().GetAttribute(UsdLightFieldTokens->kernelFalloffImplicitShapeFalloffThreshold);
}

UsdAttribute
UsdLightFieldImplicitShapeFalloffThresholdAPI::CreateKernelFalloffImplicitShapeFalloffThresholdAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdLightFieldTokens->kernelFalloffImplicitShapeFalloffThreshold,
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
UsdLightFieldImplicitShapeFalloffThresholdAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLightFieldTokens->kernelFalloffImplicitShapeFalloffThreshold,
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
