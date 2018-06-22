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
#include "pxr/usd/usdRi/pxrIntMultLightFilter.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdRiPxrIntMultLightFilter,
        TfType::Bases< UsdLuxLightFilter > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("PxrIntMultLightFilter")
    // to find TfType<UsdRiPxrIntMultLightFilter>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdRiPxrIntMultLightFilter>("PxrIntMultLightFilter");
}

/* virtual */
UsdRiPxrIntMultLightFilter::~UsdRiPxrIntMultLightFilter()
{
}

/* static */
UsdRiPxrIntMultLightFilter
UsdRiPxrIntMultLightFilter::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiPxrIntMultLightFilter();
    }
    return UsdRiPxrIntMultLightFilter(stage->GetPrimAtPath(path));
}

/* static */
UsdRiPxrIntMultLightFilter
UsdRiPxrIntMultLightFilter::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("PxrIntMultLightFilter");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdRiPxrIntMultLightFilter();
    }
    return UsdRiPxrIntMultLightFilter(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaType UsdRiPxrIntMultLightFilter::_GetSchemaType() const {
    return UsdRiPxrIntMultLightFilter::schemaType;
}

/* static */
const TfType &
UsdRiPxrIntMultLightFilter::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdRiPxrIntMultLightFilter>();
    return tfType;
}

/* static */
bool 
UsdRiPxrIntMultLightFilter::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdRiPxrIntMultLightFilter::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdRiPxrIntMultLightFilter::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdLuxLightFilter::GetSchemaAttributeNames(true);

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
