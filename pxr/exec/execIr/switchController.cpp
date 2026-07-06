//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/execIr/switchController.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<ExecIrSwitchController,
        TfType::Bases< ExecIrController > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("IrSwitchController")
    // to find TfType<ExecIrSwitchController>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, ExecIrSwitchController>("IrSwitchController");
}

/* virtual */
ExecIrSwitchController::~ExecIrSwitchController()
{
}

/* static */
ExecIrSwitchController
ExecIrSwitchController::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return ExecIrSwitchController();
    }
    return ExecIrSwitchController(stage->GetPrimAtPath(path));
}

/* static */
ExecIrSwitchController
ExecIrSwitchController::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("IrSwitchController");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return ExecIrSwitchController();
    }
    return ExecIrSwitchController(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind ExecIrSwitchController::_GetSchemaKind() const
{
    return ExecIrSwitchController::schemaKind;
}

/* static */
const TfType &
ExecIrSwitchController::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<ExecIrSwitchController>();
    return tfType;
}

/* static */
bool 
ExecIrSwitchController::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
ExecIrSwitchController::_GetTfType() const
{
    return _GetStaticTfType();
}

UsdAttribute
ExecIrSwitchController::GetRig1SpaceAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->rig1Space);
}

UsdAttribute
ExecIrSwitchController::CreateRig1SpaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->rig1Space,
                       SdfValueTypeNames->Matrix4d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrSwitchController::GetRig2SpaceAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->rig2Space);
}

UsdAttribute
ExecIrSwitchController::CreateRig2SpaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->rig2Space,
                       SdfValueTypeNames->Matrix4d,
                       /* custom = */ false,
                       SdfVariabilityVarying,
                       defaultValue,
                       writeSparsely);
}

UsdAttribute
ExecIrSwitchController::GetOutSpaceAttr() const
{
    return GetPrim().GetAttribute(ExecIrTokens->outSpace);
}

UsdAttribute
ExecIrSwitchController::CreateOutSpaceAttr(VtValue const &defaultValue, bool writeSparsely) const
{
    return UsdSchemaBase::_CreateAttr(ExecIrTokens->outSpace,
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
ExecIrSwitchController::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        ExecIrTokens->rig1Space,
        ExecIrTokens->rig2Space,
        ExecIrTokens->outSpace,
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
