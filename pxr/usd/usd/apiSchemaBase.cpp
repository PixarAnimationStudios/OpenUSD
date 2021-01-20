//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
UsdSchemaKind UsdAPISchemaBase::_GetSchemaKind() const {
    return UsdAPISchemaBase::schemaKind;
}

/* virtual */
UsdSchemaKind UsdAPISchemaBase::_GetSchemaType() const {
    return UsdAPISchemaBase::schemaType;
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


/* virtual */
bool 
UsdAPISchemaBase::_IsCompatible() const
{
    if (!UsdSchemaBase::_IsCompatible())
        return false;

    // This virtual function call tells us whether we're an applied 
    // API schema. For applied API schemas, we'd like to check whether 
    // the API schema has been applied properly on the prim.
    if (IsAppliedAPISchema() && 
        ! GetPrim()._HasAPI(_GetTfType(), /*validateSchemaType*/ false, 
                            _instanceName)) {
        return false;
    }

    if (IsMultipleApplyAPISchema() && _instanceName.IsEmpty()) {
        return false;
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
