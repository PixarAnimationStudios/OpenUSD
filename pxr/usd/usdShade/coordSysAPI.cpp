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
#include "pxr/usd/usdShade/coordSysAPI.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"
#include "pxr/usd/usd/tokens.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdShadeCoordSysAPI,
        TfType::Bases< UsdAPISchemaBase > >();
    
}

TF_DEFINE_PRIVATE_TOKENS(
    _schemaTokens,
    (CoordSysAPI)
);

/* virtual */
UsdShadeCoordSysAPI::~UsdShadeCoordSysAPI()
{
}

/* static */
UsdShadeCoordSysAPI
UsdShadeCoordSysAPI::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdShadeCoordSysAPI();
    }
    return UsdShadeCoordSysAPI(stage->GetPrimAtPath(path));
}


/* virtual */
UsdSchemaKind UsdShadeCoordSysAPI::_GetSchemaKind() const {
    return UsdShadeCoordSysAPI::schemaKind;
}

/* virtual */
UsdSchemaKind UsdShadeCoordSysAPI::_GetSchemaType() const {
    return UsdShadeCoordSysAPI::schemaType;
}

/* static */
const TfType &
UsdShadeCoordSysAPI::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdShadeCoordSysAPI>();
    return tfType;
}

/* static */
bool 
UsdShadeCoordSysAPI::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdShadeCoordSysAPI::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdShadeCoordSysAPI::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdAPISchemaBase::GetSchemaAttributeNames(true);

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

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (coordSys)
);

std::vector<UsdShadeCoordSysAPI::Binding> 
UsdShadeCoordSysAPI::GetLocalBindings() const
{
    std::vector<Binding> result;
    SdfPathVector targets;
    for (UsdProperty prop:
         GetPrim().GetAuthoredPropertiesInNamespace(_tokens->coordSys)) {
        if (UsdRelationship rel = prop.As<UsdRelationship>()) {
            targets.clear();
            if (rel.GetForwardedTargets(&targets) && !targets.empty()) {
                Binding b = {rel.GetBaseName(), rel.GetPath(), targets.front()};
                result.push_back(b);
            }
        }
    }
    return result;
}

std::vector<UsdShadeCoordSysAPI::Binding> 
UsdShadeCoordSysAPI::FindBindingsWithInheritance() const
{
    std::vector<Binding> result;
    SdfPathVector targets;
    for (UsdPrim prim = GetPrim(); prim; prim = prim.GetParent()) {
        SdfPathVector targets;
        for (UsdProperty prop:
             prim.GetAuthoredPropertiesInNamespace(_tokens->coordSys)) {
            if (UsdRelationship rel = prop.As<UsdRelationship>()) {
                // Check if name is already bound; skip if bound.
                bool nameIsAlreadyBound = false;
                for (Binding const& existing: result) {
                    if (existing.name == rel.GetBaseName()) {
                        nameIsAlreadyBound = true;
                        break;
                    }
                }
                if (!nameIsAlreadyBound) {
                    targets.clear();
                    if (rel.GetForwardedTargets(&targets) && !targets.empty()) {
                        Binding b = {rel.GetBaseName(), rel.GetPath(),
                            targets.front()};
                        result.push_back(b);
                    }
                }
            }
        }
    }
    return result;
}

bool
UsdShadeCoordSysAPI::HasLocalBindings() const
{
    for (UsdProperty prop:
         GetPrim().GetAuthoredPropertiesInNamespace(_tokens->coordSys)) {
        if (UsdRelationship rel = prop.As<UsdRelationship>()) {
            if (rel.HasAuthoredTargets()) {
                return true;
            }
        }
    }
    return false;
}

bool 
UsdShadeCoordSysAPI::Bind(const TfToken &name, const SdfPath &path) const
{
    TfToken relName = GetCoordSysRelationshipName(name);
    if (UsdRelationship rel = GetPrim().CreateRelationship(relName)) {
        return rel.SetTargets(SdfPathVector(1, path));
    }
    return false;
}

bool 
UsdShadeCoordSysAPI::ClearBinding(const TfToken &name, bool removeSpec) const
{
    TfToken relName = GetCoordSysRelationshipName(name);
    if (UsdRelationship rel = GetPrim().GetRelationship(relName)) {
        return rel.ClearTargets(removeSpec);
    }
    return false;
}

bool 
UsdShadeCoordSysAPI::BlockBinding(const TfToken &name) const
{
    TfToken relName = GetCoordSysRelationshipName(name);
    if (UsdRelationship rel = GetPrim().CreateRelationship(relName)) {
        return rel.SetTargets({});
    }
    return false;
}

TfToken
UsdShadeCoordSysAPI::GetCoordSysRelationshipName(const std::string &name)
{
    return TfToken(_tokens->coordSys.GetString() + ":" + name);
}

/* static */
bool
UsdShadeCoordSysAPI::CanContainPropertyName(const TfToken &name)
{
    return TfStringStartsWith(name, UsdShadeTokens->coordSys);
}

PXR_NAMESPACE_CLOSE_SCOPE
