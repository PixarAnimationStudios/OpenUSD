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
#include "pxr/usd/usdShade/subgraph.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdShadeSubgraph,
        TfType::Bases< UsdTyped > >();
    
    // Register the usd prim typename to associate it with the TfType, under
    // UsdSchemaBase. This enables one to call TfType::FindByName("Subgraph") to find
    // TfType<UsdShadeSubgraph>, which is how IsA queries are answered.
    TfType::AddAlias<UsdSchemaBase, UsdShadeSubgraph>("Subgraph");
}

/* virtual */
UsdShadeSubgraph::~UsdShadeSubgraph()
{
}

/* static */
UsdShadeSubgraph
UsdShadeSubgraph::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (not stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdShadeSubgraph();
    }
    return UsdShadeSubgraph(stage->GetPrimAtPath(path));
}

/* static */
UsdShadeSubgraph
UsdShadeSubgraph::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("Subgraph");
    if (not stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdShadeSubgraph();
    }
    return UsdShadeSubgraph(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* static */
const TfType &
UsdShadeSubgraph::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdShadeSubgraph>();
    return tfType;
}

/* static */
bool 
UsdShadeSubgraph::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdShadeSubgraph::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdShadeSubgraph::GetSchemaAttributeNames(bool includeInherited)
{
    static TfTokenVector localNames;
    static TfTokenVector allNames =
        UsdTyped::GetSchemaAttributeNames(true);

    if (includeInherited)
        return allNames;
    else
        return localNames;
}

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    (subgraph)
    ((TerminalNamespaceName, "terminal"))
    ((TerminalNamespacePrefix, "terminal:"))
);


UsdShadeInterfaceAttribute
UsdShadeSubgraph::CreateInterfaceAttribute(
        const TfToken& interfaceAttrName,
        const SdfValueTypeName& typeName)
{
    return UsdShadeInterfaceAttribute(
            GetPrim(),
            interfaceAttrName,
            typeName);
}

UsdShadeInterfaceAttribute
UsdShadeSubgraph::GetInterfaceAttribute(
        const TfToken& interfaceAttrName) const
{
    return UsdShadeInterfaceAttribute(
            GetPrim().GetAttribute(
                UsdShadeInterfaceAttribute::_GetName(interfaceAttrName)));
}

std::vector<UsdShadeInterfaceAttribute> 
UsdShadeSubgraph::GetInterfaceAttributes(
        const TfToken& renderTarget) const
{
    std::vector<UsdShadeInterfaceAttribute> ret;

    if (renderTarget.IsEmpty()) {
        std::vector<UsdAttribute> attrs = GetPrim().GetAttributes();
        TF_FOR_ALL(attrIter, attrs) {
            UsdAttribute attr = *attrIter;
            if (UsdShadeInterfaceAttribute interfaceAttr = 
                    UsdShadeInterfaceAttribute(attr)) {
                ret.push_back(interfaceAttr);
            }
        }
    }
    else {
        const std::string relPrefix = 
            UsdShadeInterfaceAttribute::_GetInterfaceAttributeRelPrefix(renderTarget);
        std::vector<UsdRelationship> rels = GetPrim().GetRelationships();
        TF_FOR_ALL(relIter, rels) {
            UsdRelationship rel = *relIter;
            std::string relName = rel.GetName().GetString();
            if (TfStringStartsWith(relName, relPrefix)) {
                TfToken interfaceAttrName(relName.substr(relPrefix.size()));
                if (UsdShadeInterfaceAttribute interfaceAttr = 
                        GetInterfaceAttribute(interfaceAttrName)) {
                    ret.push_back(interfaceAttr);
                }
            }
        }
    }

    return ret;
}


static TfToken 
_GetTerminalRelName(const TfToken& name)
{
    return TfToken(_tokens->TerminalNamespacePrefix.GetString() + name.GetString());
}

UsdRelationship 
UsdShadeSubgraph::CreateTerminal(
    const TfToken& terminalName,
    const SdfPath& targetPath) const
{
    if (not targetPath.IsPropertyPath()) {
        TF_CODING_ERROR("A terminal needs to be pointing to a property");
        return UsdRelationship();
    }
    const UsdPrim& prim = GetPrim();
    const TfToken& relName = _GetTerminalRelName(terminalName);
    UsdRelationship rel = prim.GetRelationship(relName);
    if (!rel) {
        rel = prim.CreateRelationship(relName, /* custom = */ false);
    }

    SdfPathVector  target(1, targetPath);
    rel.SetTargets(target);
    return rel;
}

UsdRelationship
UsdShadeSubgraph::GetTerminal(
    const TfToken& terminalName) const
{
    const UsdPrim& prim = GetPrim();
    const TfToken& relName = _GetTerminalRelName(terminalName);
    return prim.GetRelationship(relName);
}

UsdRelationshipVector
UsdShadeSubgraph::GetTerminals() const
{
    UsdRelationshipVector terminals;

    const UsdPrim& prim = GetPrim();
    const std::vector<UsdProperty>& terminalNamespaceProperties =
        prim.GetPropertiesInNamespace(_tokens->TerminalNamespaceName.GetString());

    for (const UsdProperty& property : terminalNamespaceProperties) {
        if (const UsdRelationship& relationship = property.As<UsdRelationship>()) {
            terminals.push_back(relationship);
        }
    }

    return terminals;
}
