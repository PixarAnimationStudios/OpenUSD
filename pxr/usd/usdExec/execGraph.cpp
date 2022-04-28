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
#include "pxr/usd/usdExec/execGraph.h"
#include "pxr/usd/usd/schemaRegistry.h"
#include "pxr/usd/usd/typed.h"

#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/assetPath.h"

PXR_NAMESPACE_OPEN_SCOPE

// Register the schema with the TfType system.
TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdExecGraph,
        TfType::Bases< UsdTyped > >();
    
    // Register the usd prim typename as an alias under UsdSchemaBase. This
    // enables one to call
    // TfType::Find<UsdSchemaBase>().FindDerivedByName("ExecGraph")
    // to find TfType<UsdExecGraph>, which is how IsA queries are
    // answered.
    TfType::AddAlias<UsdSchemaBase, UsdExecGraph>("ExecGraph");
}

/* virtual */
UsdExecGraph::~UsdExecGraph()
{
}

/* static */
UsdExecGraph
UsdExecGraph::Get(const UsdStagePtr &stage, const SdfPath &path)
{
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdExecGraph();
    }
    return UsdExecGraph(stage->GetPrimAtPath(path));
}

/* static */
UsdExecGraph
UsdExecGraph::Define(
    const UsdStagePtr &stage, const SdfPath &path)
{
    static TfToken usdPrimTypeName("ExecGraph");
    if (!stage) {
        TF_CODING_ERROR("Invalid stage");
        return UsdExecGraph();
    }
    return UsdExecGraph(
        stage->DefinePrim(path, usdPrimTypeName));
}

/* virtual */
UsdSchemaKind UsdExecGraph::_GetSchemaKind() const
{
    return UsdExecGraph::schemaKind;
}

/* static */
const TfType &
UsdExecGraph::_GetStaticTfType()
{
    static TfType tfType = TfType::Find<UsdExecGraph>();
    return tfType;
}

/* static */
bool 
UsdExecGraph::_IsTypedSchema()
{
    static bool isTyped = _GetStaticTfType().IsA<UsdTyped>();
    return isTyped;
}

/* virtual */
const TfType &
UsdExecGraph::_GetTfType() const
{
    return _GetStaticTfType();
}

/*static*/
const TfTokenVector&
UsdExecGraph::GetSchemaAttributeNames(bool includeInherited)
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
#include "pxr/usd/usdExec/execConnectableAPI.h"
#include "pxr/usd/usdExec/execUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdExecGraph::UsdExecGraph(const UsdExecConnectableAPI &connectable)
    : UsdExecGraph(connectable.GetPrim())
{
}

UsdExecConnectableAPI 
UsdExecGraph::ConnectableAPI() const
{
    return UsdExecConnectableAPI(GetPrim());
}

UsdExecOutput
UsdExecGraph::CreateOutput(const TfToken& name,
                             const SdfValueTypeName& typeName) const
{
    return UsdExecConnectableAPI(GetPrim()).CreateOutput(name, typeName);
}

UsdExecOutput
UsdExecGraph::GetOutput(const TfToken &name) const
{
    return UsdExecConnectableAPI(GetPrim()).GetOutput(name);
}

std::vector<UsdExecOutput>
UsdExecGraph::GetOutputs(bool onlyAuthored) const
{
    return UsdExecConnectableAPI(GetPrim()).GetOutputs(onlyAuthored);
}

UsdExecNode
UsdExecGraph::ComputeOutputSource(
    const TfToken &outputName,
    TfToken *sourceName,
    UsdExecAttributeType *sourceType) const
{
    // Check that we have a legit output
    UsdExecOutput output = GetOutput(outputName);
    if (!output) {
        return UsdExecNode();
    }

    UsdExecAttributeVector valueAttrs =
        UsdExecUtils::GetValueProducingAttributes(output);

    if (valueAttrs.empty()) {
        return UsdExecNode();
    }

    if (valueAttrs.size() > 1) {
        TF_WARN("Found multiple upstream attributes for output %s on NodeGraph "
                "%s. ComputeOutputSource will only report the first upsteam "
                "UsdExecNode. Please use GetValueProducingAttributes to "
                "retrieve all.", outputName.GetText(), GetPath().GetText());
    }

    UsdAttribute attr = valueAttrs[0];
    std::tie(*sourceName, *sourceType) =
        UsdExecUtils::GetBaseNameAndType(attr.GetName());

    UsdExecNode node(attr.GetPrim());

    if (*sourceType != UsdExecAttributeType::Output || !node) {
        return UsdExecNode();
    }

    return node;
}

UsdExecInput
UsdExecGraph::CreateInput(const TfToken& name,
                              const SdfValueTypeName& typeName) const
{
    return UsdExecConnectableAPI(GetPrim()).CreateInput(name, typeName);
}

UsdExecInput
UsdExecGraph::GetInput(const TfToken &name) const
{
    return UsdExecConnectableAPI(GetPrim()).GetInput(name);
}

std::vector<UsdExecInput>
UsdExecGraph::GetInputs(bool onlyAuthored) const
{
    return UsdExecConnectableAPI(GetPrim()).GetInputs(onlyAuthored);
}

std::vector<UsdExecInput> 
UsdExecGraph::GetInterfaceInputs() const
{
    return GetInputs();
}

static bool 
_IsValidInput(UsdExecConnectableAPI const &source, 
              UsdExecAttributeType const sourceType) 
{
    return (sourceType == UsdExecAttributeType::Input);
}

static
UsdExecGraph::ExecInterfaceInputConsumersMap 
_ComputeNonTransitiveInputConsumersMap(
    const UsdExecGraph &nodeGraph)
{
    UsdExecGraph::ExecInterfaceInputConsumersMap result;

    for (const auto& input : nodeGraph.GetInputs()) {
        result[input] = {};
    }

    // XXX: This traversal isn't instancing aware. We must update this 
    // once we have instancing aware USD objects. See http://bug/126053
    for (UsdPrim prim: nodeGraph.GetPrim().GetDescendants()) {

        UsdExecConnectableAPI connectable(prim);
        if (!connectable)
            continue;

        std::vector<UsdExecInput> internalInputs = connectable.GetInputs();
        for (const auto &internalInput: internalInputs) {
            UsdExecConnectableAPI source;
            TfToken sourceName;
            UsdExecAttributeType sourceType;
            if (UsdExecConnectableAPI::GetConnectedSource(internalInput,
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
    const UsdExecGraph::ExecInterfaceInputConsumersMap &inputConsumersMap,
    UsdExecGraph::ExecGraphInputConsumersMap *nodeGraphInputConsumers)
{
    for (const auto &inputAndConsumers : inputConsumersMap) {
        const std::vector<UsdExecInput> &consumers = inputAndConsumers.second;
        for (const UsdExecInput &consumer: consumers) {
            UsdExecConnectableAPI connectable(consumer.GetAttr().GetPrim());
            if (connectable.GetPrim().IsA<UsdExecGraph>()) {
                if (!nodeGraphInputConsumers->count(connectable)) {

                    const auto &irMap = _ComputeNonTransitiveInputConsumersMap(
                        UsdExecGraph(connectable));
                    (*nodeGraphInputConsumers)[connectable] = irMap;
                    
                    _RecursiveComputeNodeGraphInterfaceInputConsumers(irMap, 
                        nodeGraphInputConsumers);
                }
            }
        }
    }
}

static 
void
_ResolveConsumers(const UsdExecInput &consumer, 
                   const UsdExecGraph::ExecGraphInputConsumersMap 
                        &nodeGraphInputConsumers,
                   std::vector<UsdExecInput> *resolvedConsumers) 
{
    UsdExecGraph consumerNodeGraph(consumer.GetAttr().GetPrim());
    if (!consumerNodeGraph) {
        resolvedConsumers->push_back(consumer);
        return;
    }

    const auto &nodeGraphIt = nodeGraphInputConsumers.find(consumerNodeGraph);
    if (nodeGraphIt != nodeGraphInputConsumers.end()) {
        const UsdExecGraph::ExecInterfaceInputConsumersMap &inputConsumers = 
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

UsdExecGraph::ExecInterfaceInputConsumersMap 
UsdExecGraph::ComputeExecInterfaceInputConsumersMap(
    bool computeTransitiveConsumers) const
{
    ExecInterfaceInputConsumersMap result = 
        _ComputeNonTransitiveInputConsumersMap(*this);

    if (!computeTransitiveConsumers)
        return result;

    // Collect all node-graphs for which we must compute the input-consumers map.
    ExecGraphInputConsumersMap nodeGraphInputConsumers;
    _RecursiveComputeNodeGraphInterfaceInputConsumers(result, 
                                                      &nodeGraphInputConsumers);

    // If the are no consumers belonging to node-graphs, we're done.
    if (nodeGraphInputConsumers.empty())
        return result;

    ExecInterfaceInputConsumersMap resolved;
    for (const auto &inputAndConsumers : result) {
        const std::vector<UsdExecInput> &consumers = inputAndConsumers.second;

        std::vector<UsdExecInput> resolvedConsumers;
        for (const UsdExecInput &consumer: consumers) {
            std::vector<UsdExecInput> nestedConsumers;
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
