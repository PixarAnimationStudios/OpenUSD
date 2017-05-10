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
#include "pxr/usd/usdShade/nodeGraph.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdShadeNodeGraph,
        TfType::Bases< UsdTyped > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("NodeGraph")
    // to find TfType<UsdShadeNodeGraph>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdShadeNodeGraph>("NodeGraph");
}

/* virtual */
UsdShadeNodeGraph::~UsdShadeNodeGraph()
{
}

/* static */
UsdShadeNodeGraph
UsdShadeNodeGraph::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdShadeNodeGraph();
    }
    return UsdShadeNodeGraph(stage->GetPrimAtPath(path));
}

/* static */
UsdShadeNodeGraph
UsdShadeNodeGraph::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("NodeGraph");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdShadeNodeGraph();
    }
    return UsdShadeNodeGraph(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* static */
const TfType &
UsdShadeNodeGraph::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdShadeNodeGraph>();
    return tfType;
}

/* static */
bool 
UsdShadeNodeGraph::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdShadeNodeGraph::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdShadeNodeGraph::GetSchemaAttributeNames(bool includeInherited)
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

#include "pxr/usd/usd/primRange.h"
#include "pxr/usd/usdShade/connectableAPI.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdShadeNodeGraph::operator UsdShadeConnectableAPI () const {
    return UsdShadeConnectableAPI(GetPrim());
}

UsdShadeConnectableAPI 
UsdShadeNodeGraph::ConnectableAPI() const
{
    return UsdShadeConnectableAPI(GetPrim());
}

UsdShadeInterfaceAttribute
UsdShadeNodeGraph::CreateInterfaceAttribute(
        const TfToken& interfaceAttrName,
        const SdfValueTypeName& typeName)
{
    return UsdShadeInterfaceAttribute(
            GetPrim(),
            interfaceAttrName,
            typeName);
}

UsdShadeInterfaceAttribute
UsdShadeNodeGraph::GetInterfaceAttribute(
        const TfToken& interfaceAttrName) const
{
    return UsdShadeInterfaceAttribute(
            GetPrim().GetAttribute(
                UsdShadeInterfaceAttribute::_GetName(interfaceAttrName)));
}

std::vector<UsdShadeInterfaceAttribute> 
UsdShadeNodeGraph::GetInterfaceAttributes(
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
UsdShadeNodeGraph::CreateOutput(const TfToken& name,
                             const SdfValueTypeName& typeName)
{
    return UsdShadeConnectableAPI(GetPrim()).CreateOutput(name, typeName);
}

UsdShadeOutput
UsdShadeNodeGraph::GetOutput(const TfToken &name) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetOutput(name);
}

std::vector<UsdShadeOutput>
UsdShadeNodeGraph::GetOutputs() const
{
    return UsdShadeConnectableAPI(GetPrim()).GetOutputs();
}

UsdShadeInput
UsdShadeNodeGraph::CreateInput(const TfToken& name,
                              const SdfValueTypeName& typeName)
{
    TfToken inputName = name;
    if (!UsdShadeUtils::WriteNewEncoding()) {
        inputName = TfToken(UsdShadeTokens->interface_.GetString() + 
                            name.GetString());
    }
    return UsdShadeConnectableAPI(GetPrim()).CreateInput(inputName, typeName);
}

UsdShadeInput
UsdShadeNodeGraph::GetInput(const TfToken &name) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetInput(name);
}

std::vector<UsdShadeInput>
UsdShadeNodeGraph::GetInputs() const
{
    return UsdShadeConnectableAPI(GetPrim()).GetInputs();
}

std::vector<UsdShadeInput> 
UsdShadeNodeGraph::GetInterfaceInputs() const
{
    return GetInputs();
}

std::vector<UsdShadeInput> 
UsdShadeNodeGraph::_GetInterfaceInputs(const TfToken &renderTarget) const
{
    if (renderTarget.IsEmpty() ||
        !UsdShadeUtils::ReadOldEncoding()) {
        return GetInterfaceInputs();
    }

    std::vector<UsdShadeInput> result;
    const std::string relPrefix = 
        UsdShadeInterfaceAttribute::_GetInterfaceAttributeRelPrefix(renderTarget);
    std::vector<UsdRelationship> rels = GetPrim().GetRelationships();
    TF_FOR_ALL(relIter, rels) {
        UsdRelationship rel = *relIter;
        std::string relName = rel.GetName().GetString();
        if (TfStringStartsWith(relName, relPrefix)) {
            TfToken interfaceAttrName(relName.substr(relPrefix.size()));
            UsdShadeInput interfaceInput = GetInput(interfaceAttrName);
            if (interfaceInput.GetAttr()) {
                result.push_back(interfaceInput);
            }
        }
    }

    return result;
}

static bool 
_IsValidInput(UsdShadeConnectableAPI const &source, 
              UsdShadeAttributeType const sourceType) 
{
    return (sourceType == UsdShadeAttributeType::Input) || 
           (UsdShadeUtils::ReadOldEncoding() && 
            ((source.IsNodeGraph() && 
              sourceType == UsdShadeAttributeType::InterfaceAttribute) 
             || 
             (source.IsShader() && 
              sourceType == UsdShadeAttributeType::Parameter)));
}


static
UsdShadeNodeGraph::InterfaceInputConsumersMap 
_ComputeNonTransitiveInputConsumersMap(
    const UsdShadeNodeGraph &nodeGraph,
    const TfToken &renderTarget)
{
    UsdShadeNodeGraph::InterfaceInputConsumersMap result;

    bool foundOldStyleInterfaceInputs = false;
    std::vector<UsdShadeInput> inputs = nodeGraph.GetInputs();
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
                    interfaceAttr.GetRecipientParameters(renderTarget);
                for (const auto &param: consumerParams) {
                    foundOldStyleInterfaceInputs = true;
                    consumers.push_back(UsdShadeInput(param.GetAttr()));
                }
            }
        }
        result[input] = consumers;
    }

    // If we find old-style interface inputs on the material, then it's likely 
    // that the material and all its descendants have old-style encoding of 
    // shading networks. Hence, skip the downward traversal.
    if (foundOldStyleInterfaceInputs)
        return result;

    // XXX: This traversal isn't instancing aware. We must update this 
    // once we have instancing aware USD objects. See http://bug/126053
    for (UsdPrim prim: nodeGraph.GetPrim().GetDescendants()) {

        UsdShadeConnectableAPI connectable(prim);
        if (!connectable)
            continue;

        std::vector<UsdShadeInput> internalInputs = connectable.GetInputs();
        for (const auto &internalInput: internalInputs) {
            UsdShadeConnectableAPI source;
            TfToken sourceName;
            UsdShadeAttributeType sourceType;
            if (UsdShadeConnectableAPI::GetConnectedSource(internalInput,
                    &source, &sourceName, &sourceType)) {
                if (source.GetPrim() == nodeGraph.GetPrim() && 
                    _IsValidInput(source, sourceType))
                {
                    result[nodeGraph.GetInput(sourceName)].push_back(
                        internalInput);
                }
            }
        }
    }

    return result;
}

static 
void
_RecursiveComputeNodeGraphInterfaceInputConsumers(
    const UsdShadeNodeGraph::InterfaceInputConsumersMap &inputConsumersMap,
    UsdShadeNodeGraph::NodeGraphInputConsumersMap *nodeGraphInputConsumers,
    const TfToken &renderTarget) 
{
    for (const auto &inputAndConsumers : inputConsumersMap) {
        const std::vector<UsdShadeInput> &consumers = inputAndConsumers.second;
        for (const UsdShadeInput &consumer: consumers) {
            UsdShadeConnectableAPI connectable(consumer.GetAttr().GetPrim());
            if (connectable.IsNodeGraph()) {
                if (!nodeGraphInputConsumers->count(connectable)) {

                    const auto &irMap = _ComputeNonTransitiveInputConsumersMap(
                        UsdShadeNodeGraph(connectable), renderTarget);
                    (*nodeGraphInputConsumers)[connectable] = irMap;
                    
                    _RecursiveComputeNodeGraphInterfaceInputConsumers(irMap, 
                        nodeGraphInputConsumers, renderTarget);
                }
            }
        }
    }
}

static 
void
_ResolveConsumers(const UsdShadeInput &consumer, 
                   const UsdShadeNodeGraph::NodeGraphInputConsumersMap 
                        &nodeGraphInputConsumers,
                   std::vector<UsdShadeInput> *resolvedConsumers) 
{
    UsdShadeNodeGraph consumerNodeGraph(consumer.GetAttr().GetPrim());
    if (!consumerNodeGraph) {
        resolvedConsumers->push_back(consumer);
        return;
    }

    const auto &nodeGraphIt = nodeGraphInputConsumers.find(consumerNodeGraph);
    if (nodeGraphIt != nodeGraphInputConsumers.end()) {
        const UsdShadeNodeGraph::InterfaceInputConsumersMap &inputConsumers = 
            nodeGraphIt->second;

        const auto &inputIt = inputConsumers.find(consumer);
        if (inputIt != inputConsumers.end()) {
            const auto &consumers = inputIt->second;
            if (!consumers.empty()) {
                for (const auto &nestedConsumer : consumers) {
                    _ResolveConsumers(nestedConsumer, nodeGraphInputConsumers, 
                                    resolvedConsumers);
                }
            } else {
                // If the node-graph input has no consumers, then add it to 
                // the list of resolved consumers.
                resolvedConsumers->push_back(consumer);
            }
        }
    } else {
        resolvedConsumers->push_back(consumer);
    }
}

UsdShadeNodeGraph::InterfaceInputConsumersMap 
UsdShadeNodeGraph::ComputeInterfaceInputConsumersMap(
    bool computeTransitiveConsumers) const
{
    return _ComputeInterfaceInputConsumersMap(computeTransitiveConsumers, TfToken());
}

UsdShadeNodeGraph::InterfaceInputConsumersMap 
UsdShadeNodeGraph::_ComputeInterfaceInputConsumersMap(
    bool computeTransitiveConsumers,
    const TfToken &renderTarget) const
{
    InterfaceInputConsumersMap result = 
        _ComputeNonTransitiveInputConsumersMap(*this, renderTarget);

    if (!computeTransitiveConsumers)
        return result;

    // Collect all node-graphs for which we must compute the input-consumers map.
    NodeGraphInputConsumersMap nodeGraphInputConsumers;
    _RecursiveComputeNodeGraphInterfaceInputConsumers(result, 
                                                      &nodeGraphInputConsumers,
                                                      renderTarget);

    // If the are no consumers belonging to node-graphs, we're done.
    if (nodeGraphInputConsumers.empty())
        return result;

    InterfaceInputConsumersMap resolved;
    for (const auto &inputAndConsumers : result) {
        const std::vector<UsdShadeInput> &consumers = inputAndConsumers.second;

        std::vector<UsdShadeInput> resolvedConsumers;
        for (const UsdShadeInput &consumer: consumers) {
            std::vector<UsdShadeInput> nestedConsumers;
            _ResolveConsumers(consumer, nodeGraphInputConsumers, 
                              &nestedConsumers);

            resolvedConsumers.insert(resolvedConsumers.end(), 
                nestedConsumers.begin(), nestedConsumers.end());
        }

        resolved[inputAndConsumers.first] = resolvedConsumers;
    }

    return resolved;
}

PXR_NAMESPACE_CLOSE_SCOPE
