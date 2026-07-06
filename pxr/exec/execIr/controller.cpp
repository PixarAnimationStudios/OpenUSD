//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/execIr/controller.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<ExecIrController,
        TfType::Bases< UsdTyped > >();
    
}

/* virtual */
ExecIrController::~ExecIrController()
{
}

/* static */
ExecIrController
ExecIrController::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return ExecIrController();
    }
    return ExecIrController(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind ExecIrController::_GetSchemaKind() const
{
    return ExecIrController::schemaKind;
}

/* static */
const TfType &
ExecIrController::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<ExecIrController>();
    return tfType;
}

/* static */
bool 
ExecIrController::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
ExecIrController::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
ExecIrController::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdTyped::GetSchemaAttributeNames(true);

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
