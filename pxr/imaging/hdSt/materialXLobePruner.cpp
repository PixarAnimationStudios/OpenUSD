//
// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "materialXLobePruner.h"

#include <pxr/usd/sdr/registry.h>
#include <pxr/usd/sdr/shaderProperty.h>

#include <MaterialXCore/Definition.h>
#include <MaterialXCore/Document.h>
#include <MaterialXCore/Exception.h>
#include <MaterialXCore/Interface.h>
#include <MaterialXCore/Node.h>
#include <MaterialXFormat/XmlIo.h>
#include <MaterialXGenGlsl/GlslShaderGenerator.h>

#include <algorithm>
#include <cstdlib>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE

namespace {
const auto kBasePbrNodes = std::set<std::string> {
    "oren_nayar_diffuse_bsdf", "compensating_oren_nayar_diffuse_bsdf",
    "burley_diffuse_bsdf",     "conductor_bsdf",
    "subsurface_bsdf",         "translucent_bsdf",
};

const auto kLayerPbrNodes = std::set<std::string> {
    "dielectric_bsdf",    "generalized_schlick_bsdf",       "sheen_bsdf",
    "dielectric_tf_bsdf", "generalized_schlick_tf_82_bsdf", "sheen_zeltner_bsdf"
};

// All the types that have a "multiply" node taking a float as input (FA nodes):
const auto kZeroMultiplyValueMap = std::map<std::string, std::string> {
    { "float", "0" },      { "color3", "0, 0, 0" },  { "color4", "0, 0, 0, 0" },
    { "vector2", "0, 0" }, { "vector3", "0, 0, 0" }, { "vector4", "0, 0, 0, 0" }
};

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,
    ((ND_PREFIX, "LPOPTIND_"))
    ((NG_PREFIX, "LPOPTING_"))
    ((DARK_BASE, "lobe_pruner_optimization_dark_base_bsdf"))
    ((DARK_LAYER, "lobe_pruner_optimization_dark_layer_bsdf"))
    ((ND_DARK_BASE, "ND_lobe_pruner_optimization_dark_base_bsdf"))
    ((ND_DARK_LAYER, "ND_lobe_pruner_optimization_dark_layer_bsdf"))
    ((IM_DARK_BASE, "IM_lobe_pruner_optimization_dark_base_bsdf_genglsl"))
    ((IM_DARK_LAYER, "IM_lobe_pruner_optimization_dark_layer_bsdf_genglsl"))
);

class LobePrunerImpl
{
    // Data layout:
    //    map< nodeDefName,
    //        {                             // NodeDefData
    //            nodeGraphName,
    //            map< attributeName,       // AttributeMap
    //                map< attributeValue,  // OptimizableValueMap
    //                     NodeSet
    //                >
    //            >
    //        }
    //    >
    using NodeSet = TfToken::HashSet;
    using OptimizableValueMap = std::map<float, NodeSet>;
    // We want attributes alphabetically sorted:
    using AttributeMap = std::map<TfToken, OptimizableValueMap>;
    struct NodeDefData
    {
        TfToken _nodeGraphName;
        AttributeMap    _attributeData;
    };

    // Also helps if we have a reverse connection map from source node to dest node:
    using Destinations = std::set<std::string>;
    using ReverseCnxMap = std::map<std::string, Destinations>;

public:
    LobePrunerImpl() = default;
    ~LobePrunerImpl() = default;
    LobePrunerImpl(const LobePrunerImpl&) = delete;
    LobePrunerImpl& operator=(const LobePrunerImpl&) = delete;
    LobePrunerImpl(LobePrunerImpl&&) = delete;
    LobePrunerImpl& operator=(LobePrunerImpl&&) = delete;

    static LobePrunerImpl& theLobePruner() {
        static LobePrunerImpl theOne;
        return theOne;
    }

    void Initialize(const MaterialX::DocumentPtr& library);
    const MaterialX::DocumentPtr& GetLibrary() const {return _library;}

    TfToken GetOptimizedNodeId(const HdMaterialNode2& node);

private:
    void _OptimizeLibrary(const MaterialX::DocumentPtr& library);
    bool _IsLobeInput(const mx::InputPtr& input, const mx::NodeDefPtr& nd) const;
    void _AddOptimizableValue(
        float                   value,
        const mx::InputPtr&     input,
        const mx::NodeGraphPtr& ng,
        const mx::NodeDefPtr&   nd);
    TfToken
         _EnsureLibraryHasOptimizedShader(const TfToken& nodeDefName, const std::string& flags);
    void _OptimizeZeroValue(
        mx::NodeGraphPtr&          optimizedNodeGraph,
        const OptimizableValueMap& optimizationMap,
        ReverseCnxMap&             reverseMap);
    void _OptimizeOneValue(
        mx::NodeGraphPtr&          optimizedNodeGraph,
        const OptimizableValueMap& optimizationMap,
        ReverseCnxMap&             reverseMap);
    void _AddDarkShaders();
    void _OptimizeMixNode(
        const std::string& promotedInputName,
        mx::NodePtr&       mixNode,
        mx::NodeGraphPtr&  optimizedNodeGraph,
        ReverseCnxMap&     reverseMap) const;
    void _OptimizeMultiplyNode(
        mx::NodePtr&      node,
        mx::NodeGraphPtr& optimizedNodeGraph,
        ReverseCnxMap&    reverseMap) const;
    void _OptimizePbrNode(
        mx::NodePtr&       node,
        const std::string& darkNodeName,
        const std::string& darkNodeDefName) const;
    void _UpdateReverseMap(ReverseCnxMap& reverseMap, const std::string& nameToRemove) const;
    std::unordered_map<TfToken, NodeDefData, TfToken::HashFunctor> _prunerData;
    mx::DocumentPtr                                                _library;
};


void LobePrunerImpl::Initialize(const mx::DocumentPtr& library)
{
    if (!library) {
        throw mx::Exception("Requires a library");
    }

    if (_library) {
        return;
    }

    _library = mx::createDocument();
    _library->importLibrary(library);

    _AddDarkShaders();

    // Browse all surface shaders and identify prunable lobes:
    for (const auto& nd : _library->getNodeDefs()) {
        const auto outputs = nd->getActiveOutputs();
        if (outputs.size() != 1 || outputs.front()->getType() != "surfaceshader") {
            continue;
        }

        const auto impl = nd->getImplementation(mx::GlslShaderGenerator::TARGET);
        if (!impl) {
            continue;
        }
        const auto ng = impl->isA<mx::NodeGraph>()
            ? impl->asA<mx::NodeGraph>()
            : _library->getNodeGraph(impl->asA<mx::Implementation>()->getNodeGraph());
        if (!ng) {
            continue;
        }

        for (const auto& node : ng->getNodes()) {
            if (node->getCategory() == "mix") {
                const auto nodeInput = node->getActiveInput("mix");
                // A mix node, especially one that mixes two BSDF subgraphs
                // is the best optimization point available in MaterialX.
                // If the value is zero, then we can promote the subgraph
                // connected to the bg input into the upstream node, and
                // if the value is 1 we can do the same for the fg subgraph.
                // The subgraph that was not promoted will be completely
                // ignored by the shader generator.
                if (nodeInput && _IsLobeInput(nodeInput, nd)) {
                    _AddOptimizableValue(0.0F, nodeInput, ng, nd);
                    _AddOptimizableValue(1.0F, nodeInput, ng, nd);
                }
            } else if (node->getCategory() == "multiply") {
                for (const auto& nodeInput : node->getActiveInputs()) {
                    // A multiply node can also be optimized because zeroing
                    // one input means the output will be zero as well and can
                    // be directly written into the upstream nodes. They rarely
                    // affect BSDF directly though, so are of limited value.
                    if (nodeInput && _IsLobeInput(nodeInput, nd)) {
                        _AddOptimizableValue(0.0F, nodeInput, ng, nd);
                    }
                }
            } else if (
                kBasePbrNodes.count(node->getCategory())
                || kLayerPbrNodes.count(node->getCategory())) {
                const auto nodeInput = node->getActiveInput("weight");
                // A PBR node with a weight of zero will always produce a black
                // result which will either be completely opaque in the base
                // layers, or fully transparent in the coat layers. This means
                // we can replace the node directly with one that generates these
                // return values. Also means that the whole subgraph below that
                // PBR node can be ignored, but, more importantly, it means that
                // we can skip including whole swaths of MaterialX library if any
                // PBR node category becomes fully unused.
                if (nodeInput && _IsLobeInput(nodeInput, nd)) {
                    _AddOptimizableValue(0.0F, nodeInput, ng, nd);
                }
            }
        }
    }

    // See if we can optimize deeper into NodeGraphs containing optimizable nodes.
    _OptimizeLibrary(_library);
}

bool LobePrunerImpl::_IsLobeInput(const mx::InputPtr& input, const mx::NodeDefPtr& nd) const
{
    if (!input->hasInterfaceName() || input->getType() != "float") {
        return false;
    }
    const auto& ndInput = nd->getActiveInput(input->getInterfaceName());
    if (!ndInput || !ndInput->hasAttribute("uimin") || !ndInput->hasAttribute("uimax")) {
        return false;
    }
    const auto minVal = std::stof(ndInput->getAttribute("uimin"));
    if (minVal != 0.0F) {
        return false;
    }
    const auto maxVal = std::stof(ndInput->getAttribute("uimax"));
    if (maxVal != 1.0F) {
        return false;
    }
    return true;
}

void LobePrunerImpl::_OptimizeLibrary(const MaterialX::DocumentPtr& library)
{
    if (!_library || _prunerData.empty()) {
        return;
    }

    std::set<std::string> allDefinedNodeGraphs;
    // Go thru all NodeGraphs found in the library that have an associated NodeDef:
    for (const auto& ng : library->getNodeGraphs()) {
        if (ng->hasNodeDefString()) {
            allDefinedNodeGraphs.insert(ng->getName());
        }
    }
    for (const auto& impl : library->getImplementations()) {
        if (impl->hasNodeGraph()) {
            allDefinedNodeGraphs.insert(impl->getNodeGraph());
        }
    }

    for (const auto& ngName : allDefinedNodeGraphs) {
        const auto ng = library->getNodeGraph(ngName);
        // Go thru all the nodes of that NodeGraph
        for (const auto& node : ng->getNodes()) {
            // Can this node be optimized?
            const auto& nd = node->getNodeDef();
            if (!nd) {
                continue;
            }

            const auto ndName = PXR_NS::TfToken(nd->getName());
            const auto ndIt = _prunerData.find(ndName);
            if (ndIt == _prunerData.end()) {
                continue;
            }

            // This NodeGraph contains an optimizable embedded surface shader node.
            std::string flags(ndIt->second._attributeData.size(), 'x');

            bool canOptimize = false;

            auto attrIt = ndIt->second._attributeData.cbegin();
            for (size_t i = 0; attrIt != ndIt->second._attributeData.cend(); ++attrIt, ++i) {
                const auto nodeinput = node->getActiveInput(attrIt->first);
                float      inputValue = 0.5F;
                if (nodeinput) {
                    // Can not optimize if connected in any way.
                    if (nodeinput->hasNodeName() || nodeinput->hasOutputString()
                        || nodeinput->hasInterfaceName()) {
                        continue;
                    }
                    inputValue = nodeinput->getValue()->asA<float>();
                } else {
                    const auto defInput = nd->getActiveInput(attrIt->first);
                    inputValue = defInput->getValue()->asA<float>();
                }

                for (const auto& optimizableValue : attrIt->second) {
                    if (optimizableValue.first == inputValue) {
                        if (inputValue == 0.0F) {
                            flags[i] = '0';
                        } else {
                            flags[i] = '1';
                        }
                        canOptimize = true;
                    }
                }
            }

            if (canOptimize) {
                const auto optimizedNodeId = _EnsureLibraryHasOptimizedShader(ndName, flags);
                const auto optimizedNodeDef = _library->getNodeDef(optimizedNodeId);
                // Replace the node with an optimized one:
                const auto nsPrefix = optimizedNodeDef->hasNamespace()
                    ? optimizedNodeDef->getNamespace() + ":"
                    : std::string {};

                node->setCategory(nsPrefix + optimizedNodeDef->getNodeString());
                if (node->hasNodeDefString()) {
                    node->setNodeDefString(optimizedNodeDef->getName());
                }
            }
        }
    }
}

void LobePrunerImpl::_AddOptimizableValue(
    float                   value,
    const mx::InputPtr&     input,
    const mx::NodeGraphPtr& ng,
    const mx::NodeDefPtr&   nd)
{
    const auto& nodeDefName = TfToken(nd->getName());
    if (!_prunerData.count(nodeDefName)) {
        _prunerData.emplace(
            nodeDefName, NodeDefData { TfToken(ng->getName()), AttributeMap {} });
    }
    auto& attrMap = _prunerData.find(nodeDefName)->second;

    const auto& interfaceName = TfToken(input->getInterfaceName());
    if (!attrMap._attributeData.count(interfaceName)) {
        attrMap._attributeData.emplace(interfaceName, OptimizableValueMap {});
    }
    auto& valueMap = attrMap._attributeData.find(interfaceName)->second;

    if (!valueMap.count(value)) {
        valueMap.emplace(value, NodeSet {});
    }

    valueMap.find(value)->second.insert(TfToken(input->getParent()->getName()));
}

TfToken LobePrunerImpl::GetOptimizedNodeId(const HdMaterialNode2& node)
{
    TfToken retVal;

    const auto ndIt = _prunerData.find(node.nodeTypeId);
    if (ndIt == _prunerData.end()) {
        return retVal;
    }

    const auto* nodeDef
        = SdrRegistry::GetInstance().GetShaderNodeByIdentifier(node.nodeTypeId);

    std::string flags(ndIt->second._attributeData.size(), 'x');

    bool canOptimize = false;

    auto attrIt = ndIt->second._attributeData.cbegin();
    for (size_t i = 0; attrIt != ndIt->second._attributeData.cend(); ++attrIt, ++i) {
        // Can not optimize if connected in any way.
        if (node.inputConnections.find(attrIt->first) != node.inputConnections.end()) {
            continue;
        }
        float      inputValue = 0.5F;
        const auto valueIt = node.parameters.find(attrIt->first);
        if (valueIt != node.parameters.end()) {
            inputValue = valueIt->second.UncheckedGet<float>();
        } else {
            const auto* defInput = nodeDef->GetShaderInput(attrIt->first);
            inputValue = defInput->GetDefaultValueAsSdfType().UncheckedGet<float>();
        }
        for (const auto& optimizableValue : attrIt->second) {
            if (optimizableValue.first == inputValue) {
                if (inputValue == 0.0F) {
                    flags[i] = '0';
                } else {
                    flags[i] = '1';
                }
                canOptimize = true;
            }
        }
    }

    if (canOptimize) {
        return _EnsureLibraryHasOptimizedShader(node.nodeTypeId, flags);
    }

    return retVal;
}

TfToken LobePrunerImpl::_EnsureLibraryHasOptimizedShader(
    const TfToken& nodeDefName,
    const std::string&     flags)
{
    const auto ndIt = _prunerData.find(nodeDefName);
    if (ndIt == _prunerData.end()) {
        return {};
    }

    const auto originalNodeDef = _library->getNodeDef(nodeDefName.GetString());
    const auto originalNodeGraph = _library->getNodeGraph(ndIt->second._nodeGraphName);
    const auto nsPrefix = originalNodeDef->hasNamespace()
        ? originalNodeDef->getNamespace() + mx::NAME_PREFIX_SEPARATOR
        : std::string {};
    auto optimizedNodeName = originalNodeDef->getNodeString() + "_" + flags;
    if (!nsPrefix.empty() && optimizedNodeName.rfind(nsPrefix, 0) == 0) {
        optimizedNodeName = optimizedNodeName.substr(nsPrefix.size());
    }
    const auto        optimizedNodeNameWithNS = nsPrefix + optimizedNodeName;
    const std::string optimizedNodeDefName
        = nsPrefix + _tokens->ND_PREFIX.GetString() + optimizedNodeName + "_surfaceshader";
    if (_library->getNodeDef(optimizedNodeDefName)) {
        // Already there
        return TfToken{optimizedNodeDefName};
    }

    auto optimizedNodeDef
        = _library->addNodeDef(optimizedNodeDefName, "surfaceshader", optimizedNodeName);
    optimizedNodeDef->copyContentFrom(originalNodeDef);
    optimizedNodeDef->setSourceUri("");
    optimizedNodeDef->setNodeString(optimizedNodeName);

    auto optimizedNodeGraph
        = _library->addNodeGraph(nsPrefix + _tokens->NG_PREFIX.GetString() + optimizedNodeName + "_surfaceshader");
    optimizedNodeGraph->copyContentFrom(originalNodeGraph);
    optimizedNodeGraph->setSourceUri("");
    optimizedNodeGraph->setNodeDefString(optimizedNodeDefName);

    ReverseCnxMap reverseMap;
    for (const auto& node : optimizedNodeGraph->getNodes()) {
        for (const auto& input : node->getActiveInputs()) {
            if (input->hasNodeName()) {
                const auto& sourceNodeName = input->getNodeName();
                if (!reverseMap.count(sourceNodeName)) {
                    reverseMap.emplace(sourceNodeName, Destinations {});
                }
                reverseMap.find(sourceNodeName)->second.insert(node->getName());
            }
        }
    }

    auto attrIt = ndIt->second._attributeData.cbegin();
    for (size_t i = 0; attrIt != ndIt->second._attributeData.cend(); ++attrIt, ++i) {
        switch (flags[i]) {
        case '0': _OptimizeZeroValue(optimizedNodeGraph, attrIt->second, reverseMap); break;
        case '1': _OptimizeOneValue(optimizedNodeGraph, attrIt->second, reverseMap); break;
        default: continue;
        }
    }
    return TfToken{optimizedNodeDefName};
}

void LobePrunerImpl::_OptimizeZeroValue(
    mx::NodeGraphPtr&          optimizedNodeGraph,
    const OptimizableValueMap& optimizationMap,
    ReverseCnxMap&             reverseMap)
{
    for (const auto& nodeName : optimizationMap.find(0.0F)->second) {
        auto node = optimizedNodeGraph->getNode(nodeName);
        if (!node) {
            continue;
        }
        if (node->getCategory() == "mix") {
            _OptimizeMixNode("bg", node, optimizedNodeGraph, reverseMap);
        } else if (node->getCategory() == "multiply") {
            _OptimizeMultiplyNode(node, optimizedNodeGraph, reverseMap);
        } else if (kBasePbrNodes.count(node->getCategory())) {
            _OptimizePbrNode(node, _tokens->DARK_BASE, _tokens->ND_DARK_BASE);
        } else if (kLayerPbrNodes.count(node->getCategory())) {
            _OptimizePbrNode(node, _tokens->DARK_LAYER, _tokens->ND_DARK_LAYER);
        }
    }
}

void LobePrunerImpl::_AddDarkShaders()
{

    if (_library->getNodeDef(_tokens->ND_DARK_BASE)) {
        return;
    }

    auto darkNodeDef = _library->addNodeDef(_tokens->ND_DARK_BASE, "BSDF", _tokens->DARK_BASE);
    darkNodeDef->setAttribute("bsdf", "R");
    darkNodeDef->setNodeGroup("pbr");
    darkNodeDef->setDocString("A completely dark base BSDF node.");

    auto darkImplementation = _library->addImplementation(_tokens->IM_DARK_BASE);
    darkImplementation->setNodeDef(darkNodeDef);

    darkNodeDef = _library->addNodeDef(_tokens->ND_DARK_LAYER, "BSDF", _tokens->DARK_LAYER);
    darkNodeDef->setNodeGroup("pbr");
    darkNodeDef->setDocString("A completely dark layer BSDF node.");

    darkImplementation = _library->addImplementation(_tokens->IM_DARK_LAYER);
    darkImplementation->setNodeDef(darkNodeDef);
}

void LobePrunerImpl::_OptimizeOneValue(
    mx::NodeGraphPtr&          optimizedNodeGraph,
    const OptimizableValueMap& optimizationMap,
    ReverseCnxMap&             reverseMap)
{
    for (const auto& nodeName : optimizationMap.find(1.0F)->second) {
        auto node = optimizedNodeGraph->getNode(nodeName);
        if (node && node->getCategory() == "mix") {
            _OptimizeMixNode("fg", node, optimizedNodeGraph, reverseMap);
        }
    }
}

void LobePrunerImpl::_OptimizeMixNode(
    const std::string& promotedInputName,
    mx::NodePtr&       mixNode,
    mx::NodeGraphPtr&  optimizedNodeGraph,
    ReverseCnxMap&     reverseMap) const
{
    auto bgInput = mixNode->getInput(promotedInputName);
    if (!bgInput) {
        return;
    }
    const auto nodesToUpdateIt = reverseMap.find(mixNode->getName());
    if (nodesToUpdateIt != reverseMap.end()) {
        for (const auto& destNodeName : nodesToUpdateIt->second) {
            auto destNode = optimizedNodeGraph->getNode(destNodeName);
            if (!destNode) {
                return;
            }
            for (auto input : destNode->getInputs()) {
                if (input->getNodeName() == mixNode->getName()) {
                    input->removeAttribute(mx::PortElement::NODE_NAME_ATTRIBUTE);
                    if (bgInput->hasNodeName()) {
                        input->setNodeName(bgInput->getNodeName());
                        const auto bgInputsToUpdateIt = reverseMap.find(bgInput->getNodeName());
                        if (bgInputsToUpdateIt != reverseMap.end()) {
                            bgInputsToUpdateIt->second.insert(destNodeName);
                        }
                    }
                    if (bgInput->hasInterfaceName()) {
                        input->setInterfaceName(bgInput->getInterfaceName());
                    }
                    if (bgInput->hasOutputString()) {
                        input->setOutputString(bgInput->getOutputString());
                    }
                    if (bgInput->hasValueString()) {
                        input->setValueString(bgInput->getValueString());
                    }
                }
            }
        }
    }
    optimizedNodeGraph->removeNode(mixNode->getName());
    _UpdateReverseMap(reverseMap, mixNode->getName());
}

void LobePrunerImpl::_OptimizeMultiplyNode(
    mx::NodePtr&      node,
    mx::NodeGraphPtr& optimizedNodeGraph,
    ReverseCnxMap&    reverseMap) const
{
    // Result will be a zero value of the type it requests:
    const auto nodesToUpdateIt = reverseMap.find(node->getName());
    if (nodesToUpdateIt != reverseMap.end()) {
        for (const auto& destNodeName : nodesToUpdateIt->second) {
            auto destNode = optimizedNodeGraph->getNode(destNodeName);
            for (auto input : destNode->getInputs()) {
                if (input->getNodeName() == node->getName()) {
                    input->removeAttribute(mx::PortElement::NODE_NAME_ATTRIBUTE);
                    const auto defaultValueIt = kZeroMultiplyValueMap.find(input->getType());
                    if (defaultValueIt != kZeroMultiplyValueMap.end()) {
                        input->setValueString(defaultValueIt->second);
                    }
                }
            }
        }
    }
    optimizedNodeGraph->removeNode(node->getName());
    _UpdateReverseMap(reverseMap, node->getName());
}

void LobePrunerImpl::_OptimizePbrNode(
    mx::NodePtr&       node,
    const std::string& darkNodeName,
    const std::string& darkNodeDefName) const
{
    // Prune all inputs.
    for (const auto& input : node->getInputs()) {
        node->removeInput(input->getName());
    }
    // Change node category:
    node->setCategory(darkNodeName);
    if (node->hasNodeDefString()) {
        node->setNodeDefString(darkNodeDefName);
    }
}

void LobePrunerImpl::_UpdateReverseMap(ReverseCnxMap& reverseMap, const std::string& nameToRemove)
    const
{
    // Need to remove anything leading to that deleted node:
    std::vector<std::string> emptyEntries;
    for (auto& mapEntry : reverseMap) {
        mapEntry.second.erase(nameToRemove);
        if (mapEntry.second.empty()) {
            emptyEntries.push_back(mapEntry.first);
        }
    }
    for (const auto& emptyEntry : emptyEntries) {
        reverseMap.erase(emptyEntry);
    }
}

}

// Initializes the LobePruner with a library
void HdSt_InitializeLobePruner(const MaterialX::DocumentPtr& library) {
    LobePrunerImpl::theLobePruner().Initialize(library);
}

// Fetch the LobePruner library containing optimized NodeGraphs
const MaterialX::DocumentPtr& HdSt_GetLobePrunerLibrary() {
    return LobePrunerImpl::theLobePruner().GetLibrary();
}

// Checks if a node is optimizable and if this is the case, create the optimized
// NodeDef and NodeGraph in the library and return the optimized node id. An optimized node id
// will be built from the name of the original category followed by a series of characters
// describing which attibutes were optimized:
//   - 'x' that attribute was not optimized (intermediate value or connected)
//   - '0' a zero value was optimized
//   - '1' a one value was optimized
TfToken HdSt_GetLobePrunedNodeId(const HdMaterialNode2& node) {
    return LobePrunerImpl::theLobePruner().GetOptimizedNodeId(node);
}

// Returns the implementation name of an optimized dark PBR node used to replace any base PBR
// node that has a weight of zero. It is the responsibility of the shadergen code to provide a
// working implementation.
TfToken HdSt_GetDarkBaseImplementationName() {
    return _tokens->IM_DARK_BASE;
}

// Returns the implementation name of an optimized dark PBR node used to replace any base PBR
// node that has a weight of zero. It is the responsibility of the shadergen code to provide a
// working implementation.
TfToken HdSt_GetDarkLayerImplementationName() {
    return _tokens->IM_DARK_LAYER;
}

mx::ShaderNodeImplPtr HdStDarkClosureNode::create() { return std::make_shared<HdStDarkClosureNode>(); }

void HdStDarkClosureNode::initialize(const mx::InterfaceElement& element, mx::GenContext& context)
{
    mx::ShaderNodeImpl::initialize(element, context);

    if (!element.isA<mx::Implementation>()) {
        throw mx::ExceptionShaderGenError(
            "Element '" + element.getName() + "' is not an Implementation element");
    }

    const auto& impl = static_cast<const mx::Implementation&>(element);
    _isBaseNode = impl.getName() == _tokens->IM_DARK_BASE;
}

void HdStDarkClosureNode::emitFunctionCall(
    const mx::ShaderNode& node,
    mx::GenContext&       context,
    mx::ShaderStage&      stage) const
{
    DEFINE_SHADER_STAGE(stage, mx::Stage::PIXEL)
    {
        const auto& shadergen = context.getShaderGenerator();

        // We want only the
        //    BSDF metal_bsdf_out = BSDF(vec3(0.0),vec3(1.0), 0.0, 0.0);
        // part, and nothing else.
        shadergen.emitLineBegin(stage);
        shadergen.emitOutput(node.getOutput(), true, true, context, stage);
        shadergen.emitLineEnd(stage);

        // For "base" nodes, like
        //   - OrenNayar (+mx39)
        //   - Burley
        //   - Conductor
        //   - Subsurface
        //   - Translucent
        //  we also want:
        //    metal_bsdf_out.throughput = vec3(0.0);
        if (_isBaseNode) {
            shadergen.emitLineBegin(stage);
            shadergen.emitOutput(node.getOutput(), false, false, context, stage);
            shadergen.emitString(".throughput = ", stage);
            shadergen.emitString(shadergen.getSyntax().getDefaultValue(mx::Type::COLOR3), stage);
            shadergen.emitLineEnd(stage);
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
