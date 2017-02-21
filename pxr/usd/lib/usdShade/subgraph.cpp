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

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdShadeSubgraph,
        TfType::Bases< UsdTyped > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("Subgraph")
    // to find TfType<UsdShadeSubgraph>, which is how IsA queries are
    // answered.
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
    if (!stage) {
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
    if (!stage) {
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

PXR_NAMESPACE_CLOSE_SCOPE

// ===================================================================== //
// Feel free to add custom code below this line. It will be preserved by
// the code generator.
//
// Just remember to wrap code in the appropriate delimiters:
// 'PXR_NAMESPACE_OPEN_SCOPE', 'PXR_NAMESPACE_CLOSE_SCOPE'.
// ===================================================================== //
// --(BEGIN CUSTOM CODE)--

#include "pxr/usd/usd/treeIterator.h"
#include "pxr/usd/usdShade/connectableAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdShadeSubgraph::operator UsdShadeConnectableAPI () const {
    return UsdShadeConnectableAPI(GetPrim());
}

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

UsdShadeOutput
UsdShadeSubgraph::CreateOutput(const TfToken& name,
                             const SdfValueTypeName& typeName)
{
    return UsdShadeConnectableAPI(GetPrim()).CreateOutput(name, typeName);
}

UsdShadeOutput
UsdShadeSubgraph::GetOutput(const TfToken &name) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetOutput(name);
}

std::vector<UsdShadeOutput>
UsdShadeSubgraph::GetOutputs() const
{
    return UsdShadeConnectableAPI(GetPrim()).GetOutputs();
}

UsdShadeInput
UsdShadeSubgraph::CreateInput(const TfToken& name,
                              const SdfValueTypeName& typeName)
{
    TfToken inputName = name;
    if (not UsdShadeUtils::WriteNewEncoding()) {
        inputName = TfToken(UsdShadeTokens->interface.GetString() + 
                            name.GetString());
    }
    return UsdShadeConnectableAPI(GetPrim()).CreateInput(inputName, typeName);
}

UsdShadeInput
UsdShadeSubgraph::GetInput(const TfToken &name) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetInput(name);
}

std::vector<UsdShadeInput>
UsdShadeSubgraph::GetInputs() const
{
    return UsdShadeConnectableAPI(GetPrim()).GetInputs();
}

std::vector<UsdShadeInput> 
UsdShadeSubgraph::GetInterfaceInputs() const
{
    return GetInputs();
}

static bool 
_IsValidInput(UsdShadeConnectableAPI const &source, 
              UsdShadeAttributeType const sourceType) 
{
    return (sourceType == UsdShadeAttributeType::Input) or 
           (UsdShadeUtils::ReadOldEncoding() and 
            ((source.IsSubgraph() and 
              sourceType == UsdShadeAttributeType::InterfaceAttribute) 
             or 
             (source.IsShader() and 
              sourceType == UsdShadeAttributeType::Parameter)));
}

static 
UsdShadeSubgraph::InterfaceInputConsumersMap 
_ComputeNonTransitiveInputConsumersMap(const UsdShadeSubgraph &subgraph)
{
    UsdShadeSubgraph::InterfaceInputConsumersMap result;

    std::vector<UsdShadeInput> inputs = subgraph.GetInputs();
    for (const auto &input : inputs) {
        std::vector<UsdShadeInput> consumers;
        if (UsdShadeUtils::ReadOldEncoding()) {
            // If the input is an interface attribute, then get all consumer 
            // params using available API on UsdShadeInterfaceAttribute.
            if (UsdShadeUtils::GetBaseNameAndType(input.GetAttr().GetName()).second
                    == UsdShadeAttributeType::InterfaceAttribute) {
                UsdShadeInterfaceAttribute interfaceAttr(input.GetAttr());
                // How do we get all driven params of all render targets?!
                std::vector<UsdShadeParameter> consumerParams = 
                    interfaceAttr.GetRecipientParameters(TfToken());
                for (const auto &param: consumerParams) {
                    consumers.push_back(UsdShadeInput(param.GetAttr()));
                }
            }
        }
        result[input] = consumers;
    }

    UsdTreeIterator iter(subgraph.GetPrim());
    // Skip the subgraph root in the traversal.
    ++iter;

    // XXX: This traversal isn't instancing aware. We must update this 
    // once we have instancing aware USD objects. See http://bug/126053
    for ( ; iter ; ++iter) {
        const UsdPrim &prim = *iter;

        UsdShadeConnectableAPI connectable(prim);
        if (not connectable)
            continue;

        std::vector<UsdShadeInput> internalInputs = connectable.GetInputs();
        for (const auto &internalInput: internalInputs) {
            UsdShadeConnectableAPI source;
            TfToken sourceName;
            UsdShadeAttributeType sourceType;
            if (UsdShadeConnectableAPI::GetConnectedSource(internalInput,
                    &source, &sourceName, &sourceType)) {
                if (source.GetPrim() == subgraph.GetPrim() and 
                    _IsValidInput(source, sourceType))
                {
                    result[subgraph.GetInput(sourceName)].push_back(
                        internalInput);
                }
            }
        }
    }

    return result;
}

static 
void
_RecursiveComputeSubgraphInterfaceInputConsumers(
    const UsdShadeSubgraph::InterfaceInputConsumersMap &inputConsumersMap,
    UsdShadeSubgraph::SubgraphInputConsumersMap *subgraphInputConsumers) 
{
    for (const auto &inputAndConsumers : inputConsumersMap) {
        const std::vector<UsdShadeInput> &consumers = inputAndConsumers.second;
        for (const UsdShadeInput &consumer: consumers) {
            UsdShadeConnectableAPI connectable(consumer.GetAttr().GetPrim());
            if (connectable.IsSubgraph()) {
                if (not subgraphInputConsumers->count(connectable)) {

                    const auto &irMap = _ComputeNonTransitiveInputConsumersMap(
                        UsdShadeSubgraph(connectable));
                    (*subgraphInputConsumers)[connectable] = irMap;
                    
                    _RecursiveComputeSubgraphInterfaceInputConsumers(irMap, 
                        subgraphInputConsumers);
                }
            }
        }
    }
}

static 
void
_ResolveConsumers(const UsdShadeInput &consumer, 
                   const UsdShadeSubgraph::SubgraphInputConsumersMap 
                        &subgraphInputConsumers,
                   std::vector<UsdShadeInput> *resolvedConsumers) 
{
    UsdShadeSubgraph consumerSubgraph(consumer.GetAttr().GetPrim());
    if (not consumerSubgraph) {
        resolvedConsumers->push_back(consumer);
        return;
    }

    const auto &subgraphIt = subgraphInputConsumers.find(consumerSubgraph);
    if (subgraphIt != subgraphInputConsumers.end()) {
        const UsdShadeSubgraph::InterfaceInputConsumersMap &inputConsumers = 
            subgraphIt->second;

        const auto &inputIt = inputConsumers.find(consumer);
        if (inputIt != inputConsumers.end()) {
            const auto &consumers = inputIt->second;
            if (not consumers.empty()) {
                for (const auto &nestedConsumer : consumers) {
                    _ResolveConsumers(nestedConsumer, subgraphInputConsumers, 
                                    resolvedConsumers);
                }
            } else {
                // If the subgraph input has no consumers, then add it to 
                // the list of resolved consumers.
                resolvedConsumers->push_back(consumer);
            }
        }
    } else {
        resolvedConsumers->push_back(consumer);
    }
}

UsdShadeSubgraph::InterfaceInputConsumersMap 
UsdShadeSubgraph::ComputeInterfaceInputConsumersMap(
    bool computeTransitiveConsumers) const
{
    InterfaceInputConsumersMap result = 
        _ComputeNonTransitiveInputConsumersMap(*this);

    if (not computeTransitiveConsumers)
        return result;

    // Collect all subgraphs for which we must compute the input-consumers map.
    SubgraphInputConsumersMap subgraphInputConsumers;
    _RecursiveComputeSubgraphInterfaceInputConsumers(result, 
                                                     &subgraphInputConsumers);

    // If the are no consumers belonging to subgraphs, we're done.
    if (subgraphInputConsumers.empty())
        return result;

    InterfaceInputConsumersMap resolved;
    for (const auto &inputAndConsumers : result) {
        const std::vector<UsdShadeInput> &consumers = inputAndConsumers.second;

        std::vector<UsdShadeInput> resolvedConsumers;
        for (const UsdShadeInput &consumer: consumers) {
            std::vector<UsdShadeInput> nestedConsumers;
            _ResolveConsumers(consumer, subgraphInputConsumers, 
                              &nestedConsumers);

            resolvedConsumers.insert(resolvedConsumers.end(), 
                nestedConsumers.begin(), nestedConsumers.end());
        }

        resolved[inputAndConsumers.first] = resolvedConsumers;
    }

    return resolved;
}

PXR_NAMESPACE_CLOSE_SCOPE
