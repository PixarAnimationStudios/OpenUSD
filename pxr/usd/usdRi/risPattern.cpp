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
#include "pxr/usd/usdRi/risPattern.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdRiRisPattern,
        TfType::Bases< UsdRiRisObject > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("RisPattern")
    // to find TfType<UsdRiRisPattern>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdRiRisPattern>("RisPattern");
}

/* virtual */
UsdRiRisPattern::~UsdRiRisPattern()
{
}

/* static */
UsdRiRisPattern
UsdRiRisPattern::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiRisPattern();
    }
    return UsdRiRisPattern(stage->GetPrimAtPath(path));
}

/* static */
UsdRiRisPattern
UsdRiRisPattern::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("RisPattern");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiRisPattern();
    }
    return UsdRiRisPattern(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaType UsdRiRisPattern::_GetSchemaType() const {
    return UsdRiRisPattern::schemaType;
}

/* static */
const TfType &
UsdRiRisPattern::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdRiRisPattern>();
    return tfType;
}

/* static */
bool 
UsdRiRisPattern::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdRiRisPattern::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdRiRisPattern::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdRiRisObject::GetSchemaAttributeNames(true);

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
