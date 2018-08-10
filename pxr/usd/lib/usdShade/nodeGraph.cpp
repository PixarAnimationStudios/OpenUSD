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
UsdSchemaType UsdShadeNodeGraph::_GetSchemaType() const {
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
UsdShadeNodeGraph::GetOutputs() const
{
    return UsdShadeConnectableAPI(GetPrim()).GetOutputs();
}

UsdShadeShader 
UsdShadeNodeGraph::ComputeOutputSource(
    const TfToken &outputName, 
    TfToken *sourceName, 
    UsdShadeAttributeType *sourceType) const
{
    UsdShadeOutput output = GetOutput(outputName); 
    if (!output)
        return UsdShadeShader();

    UsdShadeConnectableAPI source;
    if (output.GetConnectedSource(&source, sourceName, sourceType)) {
        // XXX: we're not doing anything to detect cycles here, which will lead
        // to an infinite loop.
        if (source.IsNodeGraph()) {
            source = UsdShadeNodeGraph(source).ComputeOutputSource(*sourceName,
                sourceName, sourceType);
        }
    }

    return source;
}

UsdShadeInput
UsdShadeNodeGraph::CreateInput(const TfToken& name,
                              const SdfValueTypeName& typeName) const
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

static std::string
_GetInterfaceAttributeRelPrefix(const TfToken& renderTarget)
{
    return renderTarget.IsEmpty() ? UsdShadeTokens->interfaceRecipientsOf:
            TfStringPrintf("%s:%s", renderTarget.GetText(),
                UsdShadeTokens->interfaceRecipientsOf.GetText());
}

std::vector<UsdShadeInput> 
UsdShadeNodeGraph::_GetInterfaceInputs(const TfToken &renderTarget) const
{
    if (renderTarget.IsEmpty() ||
        !UsdShadeUtils::ReadOldEncoding()) {
        return GetInterfaceInputs();
    }

    std::vector<UsdShadeInput> result;
    const std::string relPrefix = _GetInterfaceAttributeRelPrefix(renderTarget);
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
std::vector<UsdShadeInput>
_GetInterfaceAttributeRecipientInputs(
    const UsdAttribute &interfaceAttr,
    const TfToken &renderTarget,
    const TfTokenVector &propertyNames)
{
    UsdPrim prim = interfaceAttr.GetPrim();
    std::vector<UsdShadeInput> ret;
    std::string baseName = UsdShadeUtils::GetBaseNameAndType(
            interfaceAttr.GetName()).first.GetString();

    std::vector<UsdRelationship> interfaceRecipientsOfRels;
    if (!renderTarget.IsEmpty()) {
        TfToken relName(_GetInterfaceAttributeRelPrefix(renderTarget) 
                        + baseName);
        if (UsdRelationship rel = prim.GetRelationship(relName)) {
            interfaceRecipientsOfRels.push_back(rel);
        }
    } else {
        // Find "interfaceRecipientsOf:" relationships for all renderTargets.
        for (const TfToken &propName : propertyNames) {
            // If the relationship name contains "interfaceRecipientsOf:"
            // and if its basename matches the basename of the interface 
            // attribute, it must be relevant to this relationship.
            if (TfStringContains(propName, 
                UsdShadeTokens->interfaceRecipientsOf) &&
                TfStringEndsWith(propName,  std::string(":").append(
                    interfaceAttr.GetBaseName().GetString()))) {
                            
                // Ignore silently if it's not a valid relationship.
                if (UsdRelationship rel = prim.GetRelationship(propName)) {
                    interfaceRecipientsOfRels.push_back(rel);
                }
            }
        }
    }

    for (const UsdRelationship &rel : interfaceRecipientsOfRels) {
        std::vector<SdfPath> targets;
        rel.GetTargets(&targets);
        TF_FOR_ALL(targetIter, targets) {
            const SdfPath& targetPath = *targetIter;
            if (targetPath.IsPropertyPath()) {
                if (UsdPrim targetPrim = prim.GetStage()->GetPrimAtPath(
                            targetPath.GetPrimPath())) {
                    if (UsdAttribute attr = targetPrim.GetAttribute(
                                targetPath.GetNameToken())) {
                        ret.push_back(UsdShadeInput(attr));
                    }
                }
            }
        }
    }

    return ret;
}

static
UsdShadeNodeGraph::InterfaceInputConsumersMap 
_ComputeNonTransitiveInputConsumersMap(
    const UsdShadeNodeGraph &nodeGraph,
    const TfToken &renderTarget)
{
    UsdShadeNodeGraph::InterfaceInputConsumersMap result;

    // If we're reading old encoding, cache the vector of property names to 
    // avoid computing the entire vector once per node-graph input.
    TfTokenVector propertyNames;
    if (UsdShadeUtils::ReadOldEncoding()) {
        propertyNames = nodeGraph.GetPrim().GetAuthoredPropertyNames();
    }

    bool foundOldStyleInterfaceInputs = false;
    std::vector<UsdShadeInput> inputs = nodeGraph.GetInputs();
    for (const auto &input : inputs) {
        std::vector<UsdShadeInput> consumers;
        if (UsdShadeUtils::ReadOldEncoding()) {
            // If the interface input is an interface attribute, then get all 
            // consumer params using _GetInterfaceAttributeRecipientInputs.
            if (UsdShadeUtils::GetBaseNameAndType(input.GetAttr().GetName()).second
                    == UsdShadeAttributeType::InterfaceAttribute) {
                                
                const std::vector<UsdShadeInput> &recipients = 
                    _GetInterfaceAttributeRecipientInputs(input.GetAttr(), 
                        renderTarget, propertyNames);
                if (!recipients.empty()) {
                    foundOldStyleInterfaceInputs = true;
                    consumers = recipients;
                }
            }
        }
        result[input] = consumers;
    }

    // If we find old-style interface inputs on the material, then it's likely 
    // that the material and all its descendants have old-style encoding of 
    // shading networks. Hence, skip the downward traversal.
    // 
    // If authoring of bidirectional connections on old-style interface 
    // attributes (which is a feature we only use for testing) is enabled, 
    // then we can't skip the downward traversal.
    if (foundOldStyleInterfaceInputs && 
        !UsdShadeConnectableAPI::AreBidirectionalInterfaceConnectionsEnabled()) {
        return result;
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
