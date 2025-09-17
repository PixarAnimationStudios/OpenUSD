//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/usd/usd/apiSchemaBase.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdAPISchemaBase,
        TfType::Bases< UsdSchemaBase > >();
    
}

/* virtual */
UsdAPISchemaBase::~UsdAPISchemaBase()
{
}


/* virtual */
UsdSchemaKind UsdAPISchemaBase::_GetSchemaKind() const
{
    return UsdAPISchemaBase::schemaKind;
}

/* static */
const TfType &
UsdAPISchemaBase::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdAPISchemaBase>();
    return tfType;
}

/* static */
bool 
UsdAPISchemaBase::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdAPISchemaBase::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdAPISchemaBase::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdSchemaBase::GetSchemaAttributeNames(true);

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

#include "pxr/usd/usd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

/* static */
TfTokenVector
UsdAPISchemaBase::_GetMultipleApplyInstanceNames(const UsdPrim &prim,
                                                 const TfType &schemaType)
{
    TfTokenVector instanceNames;

    auto appliedSchemas = prim.GetAppliedSchemas();
    if (appliedSchemas.empty()) {
        return instanceNames;
    }
    
    TfToken schemaTypeName = UsdSchemaRegistry::GetAPISchemaTypeName(schemaType);

    for (const auto &appliedSchema : appliedSchemas) {
        std::pair<TfToken, TfToken> typeNameAndInstance =
                UsdSchemaRegistry::GetTypeNameAndInstance(appliedSchema);
        if (typeNameAndInstance.first == schemaTypeName) {
            instanceNames.emplace_back(typeNameAndInstance.second);
        }
    }
    
    return instanceNames;
}

/* virtual */
bool 
UsdAPISchemaBase::_IsCompatible() const
{
    if (!UsdSchemaBase::_IsCompatible())
        return false;

    // This virtual function call tells us whether we're an applied 
    // API schema. For applied API schemas, we'd like to check whether 
    // the API schema has been applied properly on the prim.
    if (IsAppliedAPISchema()) {
        if (IsMultipleApplyAPISchema()) {
            if (_instanceName.IsEmpty() ||
                !GetPrim().HasAPI(_GetTfType(), _instanceName)) {
                return false;
            }
        } else {
            if (!GetPrim().HasAPI(_GetTfType())) {
                return false;
            }
        }
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
