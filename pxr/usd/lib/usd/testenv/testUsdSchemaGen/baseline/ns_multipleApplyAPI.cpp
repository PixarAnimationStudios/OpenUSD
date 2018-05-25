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
#include "pxr/usd/usdContrived/multipleApplyAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

namespace foo {

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdContrivedMultipleApplyAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (MultipleApplyAPI)
);

/* virtual */
UsdContrivedMultipleApplyAPI::~UsdContrivedMultipleApplyAPI()
{
}

/* static */
UsdContrivedMultipleApplyAPI
UsdContrivedMultipleApplyAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdContrivedMultipleApplyAPI();
    }
    return UsdContrivedMultipleApplyAPI(stage->GetPrimAtPath(path));
}

/*virtual*/
bool 
UsdContrivedMultipleApplyAPI::_IsAppliedAPISchema() const 
{
    return true;
}

/*virtual*/
bool 
UsdContrivedMultipleApplyAPI::_IsMultipleApplyAPISchema() const 
{
    return true;
}

/* static */
UsdContrivedMultipleApplyAPI
UsdContrivedMultipleApplyAPI::_Apply(const UsdPrim &prim, const TfToken &name)
{
    return UsdAPISchemaBase::_MultipleApplyAPISchema<UsdContrivedMultipleApplyAPI>(
            prim, _schemaTokens->MultipleApplyAPI, name);
}

/* static */
const TfType &
UsdContrivedMultipleApplyAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdContrivedMultipleApplyAPI>();
    return tfType;
}

/* static */
bool 
UsdContrivedMultipleApplyAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdContrivedMultipleApplyAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdContrivedMultipleApplyAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdAPISchemaBase::GetSchemaAttributeNames(true);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

}

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'namespace foo {', '}'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--
