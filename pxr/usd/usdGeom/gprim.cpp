//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usdGeom/gprim.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdGeomGprim,
        TfType::Bases< UsdGeomBoundable > >();
    
}

/* virtual */
UsdGeomGprim::~UsdGeomGprim()
{
}

/* static */
UsdGeomGprim
UsdGeomGprim::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdGeomGprim();
    }
    return UsdGeomGprim(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdGeomGprim::_GetSchemaKind() const
{
    return UsdGeomGprim::schemaKind;
}

/* static */
const TfType &
UsdGeomGprim::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdGeomGprim>();
    return tfType;
}

/* static */
bool 
UsdGeomGprim::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdGeomGprim::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
UsdGeomGprim::GetDisplayColorAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->primvarsDisplayColor);
}

UsdAttribute
UsdGeomGprim::CreateDisplayColorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->primvarsDisplayColor,
                       SdfValueTypeNames->Color3fArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomGprim::GetDisplayOpacityAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->primvarsDisplayOpacity);
}

UsdAttribute
UsdGeomGprim::CreateDisplayOpacityAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->primvarsDisplayOpacity,
                       SdfValueTypeNames->FloatArray,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomGprim::GetDoubleSidedAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->doubleSided);
}

UsdAttribute
UsdGeomGprim::CreateDoubleSidedAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->doubleSided,
                       SdfValueTypeNames->Bool,
                       /* custom = */ false,
                       SdfVariabilityUniform,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
UsdGeomGprim::GetOrientationAttr() const
{
    return GetPrim().GetAttribute(UsdGeomTokens->orientation);
}

UsdAttribute
UsdGeomGprim::CreateOrientationAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(UsdGeomTokens->orientation,
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
UsdGeomGprim::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdGeomTokens->primvarsDisplayColor,
        UsdGeomTokens->primvarsDisplayOpacity,
        UsdGeomTokens->doubleSided,
        UsdGeomTokens->orientation,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomBoundable::GetSchemaAttributeNames(true),
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

#include "pxr/usd/usdGeom/primvarsAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdGeomPrimvar
UsdGeomGprim::GetDisplayColorPrimvar() const
{
    return UsdGeomPrimvar(GetDisplayColorAttr());
}

UsdGeomPrimvar
UsdGeomGprim::CreateDisplayColorPrimvar(const TfToken& interpolation,
                                        int elementSize) const
{
    return UsdGeomPrimvarsAPI(GetPrim()).CreatePrimvar(
        UsdGeomTokens->primvarsDisplayColor,
        SdfValueTypeNames->Color3fArray,
        interpolation,
        elementSize);
}

UsdGeomPrimvar
UsdGeomGprim::GetDisplayOpacityPrimvar() const
{
    return UsdGeomPrimvar(GetDisplayOpacityAttr());
}

UsdGeomPrimvar
UsdGeomGprim::CreateDisplayOpacityPrimvar(const TfToken& interpolation,
                                          int elementSize) const
{
    return UsdGeomPrimvarsAPI(GetPrim()).CreatePrimvar(
        UsdGeomTokens->primvarsDisplayOpacity,
        SdfValueTypeNames->FloatArray,
        interpolation,
        elementSize);
}

PXR_NAMESPACE_CLOSE_SCOPE
