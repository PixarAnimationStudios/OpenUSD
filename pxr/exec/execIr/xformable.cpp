//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/execIr/xformable.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<ExecIrXformable,
        TfType::Bases< UsdTyped > >();
    
}

/* virtual */
ExecIrXformable::~ExecIrXformable()
{
}

/* static */
ExecIrXformable
ExecIrXformable::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return ExecIrXformable();
    }
    return ExecIrXformable(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind ExecIrXformable::_GetSchemaKind() const
{
    return ExecIrXformable::schemaKind;
}

/* static */
const TfType &
ExecIrXformable::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<ExecIrXformable>();
    return tfType;
}

/* static */
bool 
ExecIrXformable::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
ExecIrXformable::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
ExecIrXformable::GetRestTxAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->restTx);
}

UsdAttribute
ExecIrXformable::CreateRestTxAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->restTx,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetRestTyAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->restTy);
}

UsdAttribute
ExecIrXformable::CreateRestTyAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->restTy,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetRestTzAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->restTz);
}

UsdAttribute
ExecIrXformable::CreateRestTzAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->restTz,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetRestRxAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->restRx);
}

UsdAttribute
ExecIrXformable::CreateRestRxAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->restRx,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetRestRyAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->restRy);
}

UsdAttribute
ExecIrXformable::CreateRestRyAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->restRy,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetRestRzAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->restRz);
}

UsdAttribute
ExecIrXformable::CreateRestRzAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->restRz,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetRestSpaceAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->restSpace);
}

UsdAttribute
ExecIrXformable::CreateRestSpaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->restSpace,
                       SdfValueTypeNames->Matrix4d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetDefaultTxAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->defaultTx);
}

UsdAttribute
ExecIrXformable::CreateDefaultTxAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->defaultTx,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetDefaultTyAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->defaultTy);
}

UsdAttribute
ExecIrXformable::CreateDefaultTyAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->defaultTy,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetDefaultTzAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->defaultTz);
}

UsdAttribute
ExecIrXformable::CreateDefaultTzAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->defaultTz,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetDefaultRxAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->defaultRx);
}

UsdAttribute
ExecIrXformable::CreateDefaultRxAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->defaultRx,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetDefaultRyAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->defaultRy);
}

UsdAttribute
ExecIrXformable::CreateDefaultRyAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->defaultRy,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetDefaultRzAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->defaultRz);
}

UsdAttribute
ExecIrXformable::CreateDefaultRzAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->defaultRz,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetDefaultSpaceAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->defaultSpace);
}

UsdAttribute
ExecIrXformable::CreateDefaultSpaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->defaultSpace,
                       SdfValueTypeNames->Matrix4d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetPosedSpaceAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->posedSpace);
}

UsdAttribute
ExecIrXformable::CreatePosedSpaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->posedSpace,
                       SdfValueTypeNames->Matrix4d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetPosedDefaultSpaceAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->posedDefaultSpace);
}

UsdAttribute
ExecIrXformable::CreatePosedDefaultSpaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->posedDefaultSpace,
                       SdfValueTypeNames->Matrix4d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetAvarsTxAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->avarsTx);
}

UsdAttribute
ExecIrXformable::CreateAvarsTxAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->avarsTx,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetAvarsTyAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->avarsTy);
}

UsdAttribute
ExecIrXformable::CreateAvarsTyAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->avarsTy,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetAvarsTzAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->avarsTz);
}

UsdAttribute
ExecIrXformable::CreateAvarsTzAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->avarsTz,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetAvarsRxAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->avarsRx);
}

UsdAttribute
ExecIrXformable::CreateAvarsRxAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->avarsRx,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetAvarsRyAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->avarsRy);
}

UsdAttribute
ExecIrXformable::CreateAvarsRyAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->avarsRy,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetAvarsRzAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->avarsRz);
}

UsdAttribute
ExecIrXformable::CreateAvarsRzAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->avarsRz,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetAvarsRspinAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->avarsRspin);
}

UsdAttribute
ExecIrXformable::CreateAvarsRspinAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->avarsRspin,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetAvarsRotationOrderAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->avarsRotationOrder);
}

UsdAttribute
ExecIrXformable::CreateAvarsRotationOrderAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->avarsRotationOrder,
                       SdfValueTypeNames->Token,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetAvarsDefaultSpaceAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->avarsDefaultSpace);
}

UsdAttribute
ExecIrXformable::CreateAvarsDefaultSpaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->avarsDefaultSpace,
                       SdfValueTypeNames->Matrix4d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetAvarsUnitScaleFactorAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->avarsUnitScaleFactor);
}

UsdAttribute
ExecIrXformable::CreateAvarsUnitScaleFactorAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->avarsUnitScaleFactor,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetParentSpaceAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->parentSpace);
}

UsdAttribute
ExecIrXformable::CreateParentSpaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->parentSpace,
                       SdfValueTypeNames->Matrix4d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrXformable::GetParentDefaultSpaceAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->parentDefaultSpace);
}

UsdAttribute
ExecIrXformable::CreateParentDefaultSpaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->parentDefaultSpace,
                       SdfValueTypeNames->Matrix4d,
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
ExecIrXformable::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        ExecIrTokens->restTx,
        ExecIrTokens->restTy,
        ExecIrTokens->restTz,
        ExecIrTokens->restRx,
        ExecIrTokens->restRy,
        ExecIrTokens->restRz,
        ExecIrTokens->restSpace,
        ExecIrTokens->defaultTx,
        ExecIrTokens->defaultTy,
        ExecIrTokens->defaultTz,
        ExecIrTokens->defaultRx,
        ExecIrTokens->defaultRy,
        ExecIrTokens->defaultRz,
        ExecIrTokens->defaultSpace,
        ExecIrTokens->posedSpace,
        ExecIrTokens->posedDefaultSpace,
        ExecIrTokens->avarsTx,
        ExecIrTokens->avarsTy,
        ExecIrTokens->avarsTz,
        ExecIrTokens->avarsRx,
        ExecIrTokens->avarsRy,
        ExecIrTokens->avarsRz,
        ExecIrTokens->avarsRspin,
        ExecIrTokens->avarsRotationOrder,
        ExecIrTokens->avarsDefaultSpace,
        ExecIrTokens->avarsUnitScaleFactor,
        ExecIrTokens->parentSpace,
        ExecIrTokens->parentDefaultSpace,
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
