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

/* virtual */
UsdSchemaKind UsdShadeNodeGraph::_GetSchemaKind() const {
    return UsdShadeNodeGraph::schemaKind;
}

/* virtual */
UsdSchemaKind UsdShadeNodeGraph::_GetSchemaType() const {
    return UsdShadeNodeGraph::schemaType;
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
#include "pxr/usd/usdShade/utils.h"

PXR_NAMESPACE_OPEN_SCOPE

UsdShadeNodeGraph::UsdShadeNodeGraph(const UsdShadeConnectableAPI &connectable)
    : UsdShadeNodeGraph(connectable.GetPrim())
{
}

UsdShadeConnectableAPI 
UsdShadeNodeGraph::ConnectableAPI() const
{
    return UsdShadeConnectableAPI(GetPrim());
}

UsdShadeOutput
UsdShadeNodeGraph::CreateOutput(const TfToken& name,
                             const SdfValueTypeName& typeName) const
{
    return UsdShadeConnectableAPI(GetPrim()).CreateOutput(name, typeName);
}

UsdShadeOutput
UsdShadeNodeGraph::GetOutput(const TfToken &name) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetOutput(name);
}

std::vector<UsdShadeOutput>
UsdShadeNodeGraph::GetOutputs(bool onlyAuthored) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetOutputs(onlyAuthored);
}

UsdShadeShader
UsdShadeNodeGraph::ComputeOutputSource(
    const TfToken &outputName,
    TfToken *sourceName,
    UsdShadeAttributeType *sourceType) const
{
    // Check that we have a legit output
    UsdShadeOutput output = GetOutput(outputName);
    if (!output) {
        return UsdShadeShader();
    }

    UsdShadeAttributeVector valueAttrs =
        UsdShadeUtils::GetValueProducingAttributes(output);

    if (valueAttrs.empty()) {
        return UsdShadeShader();
    }

    if (valueAttrs.size() > 1) {
        TF_WARN("Found multiple upstream attributes for output %s on NodeGraph "
                "%s. ComputeOutputSource will only report the first upsteam "
                "UsdShadeShader. Please use GetValueProducingAttributes to "
                "retrieve all.", outputName.GetText(), GetPath().GetText());
    }

    UsdAttribute attr = valueAttrs[0];
    std::tie(*sourceName, *sourceType) =
        UsdShadeUtils::GetBaseNameAndType(attr.GetName());

    UsdShadeShader shader(attr.GetPrim());

    if (*sourceType != UsdShadeAttributeType::Output || !shader) {
        return UsdShadeShader();
    }

    return shader;
}

UsdShadeInput
UsdShadeNodeGraph::CreateInput(const TfToken& name,
                              const SdfValueTypeName& typeName) const
{
    return UsdShadeConnectableAPI(GetPrim()).CreateInput(name, typeName);
}

UsdShadeInput
UsdShadeNodeGraph::GetInput(const TfToken &name) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetInput(name);
}

std::vector<UsdShadeInput>
UsdShadeNodeGraph::GetInputs(bool onlyAuthored) const
{
    return UsdShadeConnectableAPI(GetPrim()).GetInputs(onlyAuthored);
}

std::vector<UsdShadeInput> 
UsdShadeNodeGraph::GetInterfaceInputs() const
{
    return GetInputs();
}

static bool 
_IsValidInput(UsdShadeConnectableAPI const &source, 
              UsdShadeAttributeType const sourceType) 
{
    return (sourceType == UsdShadeAttributeType::Input);
}

static
UsdShadeNodeGraph::InterfaceInputConsumersMap 
_ComputeNonTransitiveInputConsumersMap(
    const UsdShadeNodeGraph &nodeGraph)
{
    UsdShadeNodeGraph::InterfaceInputConsumersMap result;

    for (const auto& input : nodeGraph.GetInputs()) {
        result[input] = {};
    }

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
    UsdShadeNodeGraph::NodeGraphInputConsumersMap *nodeGraphInputConsumers)
{
    for (const auto &inputAndConsumers : inputConsumersMap) {
        const std::vector<UsdShadeInput> &consumers = inputAndConsumers.second;
        for (const UsdShadeInput &consumer: consumers) {
            UsdShadeConnectableAPI connectable(consumer.GetAttr().GetPrim());
            if (connectable.GetPrim().IsA<UsdShadeNodeGraph>()) {
                if (!nodeGraphInputConsumers->count(connectable)) {

                    const auto &irMap = _ComputeNonTransitiveInputConsumersMap(
                        UsdShadeNodeGraph(connectable));
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
    InterfaceInputConsumersMap result = 
        _ComputeNonTransitiveInputConsumersMap(*this);

    if (!computeTransitiveConsumers)
        return result;

    // Collect all node-graphs for which we must compute the input-consumers map.
    NodeGraphInputConsumersMap nodeGraphInputConsumers;
    _RecursiveComputeNodeGraphInterfaceInputConsumers(result, 
                                                      &nodeGraphInputConsumers);

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

bool
UsdShadeNodeGraph::ConnectableAPIBehavior::CanConnectOutputToSource(
    const UsdShadeOutput &output,
    const UsdAttribute &source,
    std::string *reason)
{
    return UsdShadeConnectableAPIBehavior::_CanConnectOutputToSource(
            output, source, reason);
}

bool
UsdShadeNodeGraph::ConnectableAPIBehavior::IsContainer() const
{
    // NodeGraph does act as a namespace container for connected nodes
    return true;
}

TF_REGISTRY_FUNCTION(UsdShadeConnectableAPI)
{
    UsdShadeRegisterConnectableAPIBehavior<
        UsdShadeNodeGraph,
        UsdShadeNodeGraph::ConnectableAPIBehavior>();
}

PXR_NAMESPACE_CLOSE_SCOPE
