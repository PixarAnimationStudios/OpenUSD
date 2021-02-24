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
#include "pxr/usd/usdLux/lightFilter.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdLuxLightFilter,
        TfType::Bases< UsdGeomXformable > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("LightFilter")
    // to find TfType<UsdLuxLightFilter>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdLuxLightFilter>("LightFilter");
}

/* virtual */
UsdLuxLightFilter::~UsdLuxLightFilter()
{
}

/* static */
UsdLuxLightFilter
UsdLuxLightFilter::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxLightFilter();
    }
    return UsdLuxLightFilter(stage->GetPrimAtPath(path));
}

/* static */
UsdLuxLightFilter
UsdLuxLightFilter::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("LightFilter");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdLuxLightFilter();
    }
    return UsdLuxLightFilter(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdLuxLightFilter::_GetSchemaKind() const {
    return UsdLuxLightFilter::schemaKind;
}

/* virtual */
UsdSchemaKind UsdLuxLightFilter::_GetSchemaType() const {
    return UsdLuxLightFilter::schemaType;
}

/* static */
const TfType &
UsdLuxLightFilter::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdLuxLightFilter>();
    return tfType;
}

/* static */
bool 
UsdLuxLightFilter::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdLuxLightFilter::_GetTfType() const
{
    return _GetStaticTfType();
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
UsdLuxLightFilter::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames = {
        UsdLuxTokens->collectionFilterLinkIncludeRoot,
    };
    static TfTokenVector allNames =
        _ConcatenateAttributeNames(
            UsdGeomXformable::GetSchemaAttributeNames(true),
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

#include "pxr/usd/usdShade/connectableAPI.h"
#include "pxr/usd/usdShade/connectableAPIBehavior.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdLuxLightFilter_ConnectableAPIBehavior : 
    public UsdShadeConnectableAPIBehavior
{
    bool
    CanConnectInputToSource(const UsdShadeInput &input,
                            const UsdAttribute &source,
                            std::string *reason) override
    {     
        return _CanConnectInputToSource(input, source, reason, 
                ConnectableNodeTypes::DerivedContainerNodes);
    }

    bool IsContainer() const
    {
        return true;
    }

    // Note that LightFilter's outputs are not connectable (different from
    // UsdShadeNodeGraph default behavior) as there are no known use-case for 
    // these right now.

};


TF_REGISTRY_FUNCTION(UsdShadeConnectableAPI)
{
    // UsdLuxLightFilter prims are connectable, with special behavior requiring 
    // connection source to be encapsulated under the light.
    UsdShadeRegisterConnectableAPIBehavior<
        UsdLuxLightFilter, UsdLuxLightFilter_ConnectableAPIBehavior>();
}

UsdLuxLightFilter::UsdLuxLightFilter(const UsdShadeConnectableAPI &connectable)
    : UsdLuxLightFilter(connectable.GetPrim())
{
}

UsdShadeConnectableAPI 
UsdLuxLightFilter::ConnectableAPI() const
{
    return UsdShadeConnectableAPI(GetPrim());
}

UsdShadeOutput
UsdLuxLightFilter::CreateOutput(const TfToken& name,
                                const SdfValueTypeName& typeName)
{
    return UsdShadeConnectableAPI(GetPrim()).CreateOutput(name, typeName);
}

UsdShadeOutput
UsdLuxLightFilter::GetOutput(const TfToken &name) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetOutput(name);
}

std::vector<UsdShadeOutput>
UsdLuxLightFilter::GetOutputs(bool onlyAuthored) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetOutputs(onlyAuthored);
}

UsdShadeInput
UsdLuxLightFilter::CreateInput(const TfToken& name,
                               const SdfValueTypeName& typeName)
{
    return UsdShadeConnectableAPI(GetPrim()).CreateInput(name, typeName);
}

UsdShadeInput
UsdLuxLightFilter::GetInput(const TfToken &name) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetInput(name);
}

std::vector<UsdShadeInput>
UsdLuxLightFilter::GetInputs(bool onlyAuthored) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetInputs(onlyAuthored);
}

UsdCollectionAPI
UsdLuxLightFilter::GetFilterLinkCollectionAPI() const
{
    return UsdCollectionAPI(GetPrim(), UsdLuxTokens->filterLink);
}

PXR_NAMESPACE_CLOSE_SCOPE
