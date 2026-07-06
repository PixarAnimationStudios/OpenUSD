//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/execIr/fkController.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<ExecIrFkController,
        TfType::Bases< ExecIrController > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("IrFkController")
    // to find TfType<ExecIrFkController>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, ExecIrFkController>("IrFkController");
}

/* virtual */
ExecIrFkController::~ExecIrFkController()
{
}

/* static */
ExecIrFkController
ExecIrFkController::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return ExecIrFkController();
    }
    return ExecIrFkController(stage->GetPrimAtPath(path));
}

/* static */
ExecIrFkController
ExecIrFkController::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("IrFkController");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return ExecIrFkController();
    }
    return ExecIrFkController(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind ExecIrFkController::_GetSchemaKind() const
{
    return ExecIrFkController::schemaKind;
}

/* static */
const TfType &
ExecIrFkController::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<ExecIrFkController>();
    return tfType;
}

/* static */
bool 
ExecIrFkController::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
ExecIrFkController::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
ExecIrFkController::GetParentInSpaceAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->parentInSpace);
}

UsdAttribute
ExecIrFkController::CreateParentInSpaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->parentInSpace,
                       SdfValueTypeNames->Matrix4d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrFkController::GetParentInDefaultSpaceAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->parentInDefaultSpace);
}

UsdAttribute
ExecIrFkController::CreateParentInDefaultSpaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->parentInDefaultSpace,
                       SdfValueTypeNames->Matrix4d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrFkController::GetOutSpaceAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->outSpace);
}

UsdAttribute
ExecIrFkController::CreateOutSpaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->outSpace,
                       SdfValueTypeNames->Matrix4d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrFkController::GetOutDefaultSpaceAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->outDefaultSpace);
}

UsdAttribute
ExecIrFkController::CreateOutDefaultSpaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->outDefaultSpace,
                       SdfValueTypeNames->Matrix4d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrFkController::GetInDefaultSpaceAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->inDefaultSpace);
}

UsdAttribute
ExecIrFkController::CreateInDefaultSpaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->inDefaultSpace,
                       SdfValueTypeNames->Matrix4d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrFkController::GetInTxAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->inTx);
}

UsdAttribute
ExecIrFkController::CreateInTxAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->inTx,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrFkController::GetInTyAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->inTy);
}

UsdAttribute
ExecIrFkController::CreateInTyAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->inTy,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrFkController::GetInTzAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->inTz);
}

UsdAttribute
ExecIrFkController::CreateInTzAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->inTz,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrFkController::GetInRxAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->inRx);
}

UsdAttribute
ExecIrFkController::CreateInRxAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->inRx,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrFkController::GetInRyAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->inRy);
}

UsdAttribute
ExecIrFkController::CreateInRyAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->inRy,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrFkController::GetInRzAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->inRz);
}

UsdAttribute
ExecIrFkController::CreateInRzAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->inRz,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrFkController::GetInRspinAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->inRspin);
}

UsdAttribute
ExecIrFkController::CreateInRspinAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->inRspin,
                       SdfValueTypeNames->Double,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrFkController::GetInRotationOrderAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->inRotationOrder);
}

UsdAttribute
ExecIrFkController::CreateInRotationOrderAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->inRotationOrder,
                       SdfValueTypeNames->Token,
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
ExecIrFkController::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        ExecIrTokens->parentInSpace,
        ExecIrTokens->parentInDefaultSpace,
        ExecIrTokens->outSpace,
        ExecIrTokens->outDefaultSpace,
        ExecIrTokens->inDefaultSpace,
        ExecIrTokens->inTx,
        ExecIrTokens->inTy,
        ExecIrTokens->inTz,
        ExecIrTokens->inRx,
        ExecIrTokens->inRy,
        ExecIrTokens->inRz,
        ExecIrTokens->inRspin,
        ExecIrTokens->inRotationOrder,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            ExecIrController::GetSchemaAttributeNames(true),
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
