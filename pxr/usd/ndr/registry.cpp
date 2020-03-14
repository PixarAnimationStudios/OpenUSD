//
// Copyright 2018 Pixar
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

#include "pxr/pxr.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ndr/debugCodes.h"
#include "pxr/usd/ndr/discoveryPlugin.h"
#include "pxr/usd/ndr/node.h"
#include "pxr/usd/ndr/nodeDiscoveryResult.h"
#include "pxr/usd/ndr/property.h"
#include "pxr/usd/ndr/registry.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/envSetting.h"

#include <boost/functional/hash.hpp>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(
    PXR_NDR_SKIP_DISCOVERY_PLUGIN_DISCOVERY, 0,
    "The auto-discovery of discovery plugins in ndr can be skipped. "
    "This is used mostly for testing purposes.");

TF_DEFINE_ENV_SETTING(
    PXR_NDR_SKIP_PARSER_PLUGIN_DISCOVERY, 0,
    "The auto-discovery of parser plugins in ndr can be skipped. "
    "This is used mostly for testing purposes.");

// This function is used for property validation. It is written as a non-static
// freestanding function so that we can exercise it in a test without needing
// to expose it in the header file.  It is also written without using unique
// pointers for ease of python wrapping and testability.
NDR_API
bool
NdrRegistry_ValidateProperty(
    const NdrNodeConstPtr& node,
    const NdrPropertyConstPtr& property,
    std::string* errorMessage)
{
    const VtValue& defaultValue = property->GetDefaultValue();
    const SdfTypeIndicator sdfTypeIndicator = property->GetTypeAsSdfType();
    const SdfValueTypeName sdfType = sdfTypeIndicator.first;

    // We allow default values to be unspecified, but if they aren't empty, then
    // we want to error if the value's type is different from the specified type
    // for the property.
    if (!defaultValue.IsEmpty()) {
        if (defaultValue.GetType() != sdfType.GetType()) {

            if (errorMessage) {
                *errorMessage = TfStringPrintf(
                    "Default value type does not match specified type for "
                    "property.\n"
                    "Node identifier: %s\n"
                    "Source type: %s\n"
                    "Property name: %s.\n"
                    "Type from SdfType: %s.\n"
                    "Type from default value: %s.\n",
                    node->GetIdentifier().GetString().c_str(),
                    node->GetSourceType().GetString().c_str(),
                    property->GetName().GetString().c_str(),
                    sdfType.GetType().GetTypeName().c_str(),
                    defaultValue.GetType().GetTypeName().c_str());
            }

            return false;
        }
    }
    return true;
}

namespace {

// Helpers to allow template functions to treat discovery results and
// nodes equally.
template <typename T> struct _NdrObjectAccess { };
template <> struct _NdrObjectAccess<NdrNodeDiscoveryResult> {
    typedef NdrNodeDiscoveryResult Type;
    static const std::string& GetName(const Type& x) { return x.name; }
    static const TfToken& GetFamily(const Type& x) { return x.family; }
    static NdrVersion GetVersion(const Type& x) { return x.version; }
};
template <> struct _NdrObjectAccess<NdrNodeUniquePtr> {
    typedef NdrNodeUniquePtr Type;
    static const std::string& GetName(const Type& x) { return x->GetName(); }
    static const TfToken& GetFamily(const Type& x) { return x->GetFamily(); }
    static NdrVersion GetVersion(const Type& x) { return x->GetVersion(); }
};

template <typename T>
static
bool
_MatchesNameAndFilter(
    const T& object,
    const std::string& name,
    NdrVersionFilter filter)
{
    using Access = _NdrObjectAccess<T>;

    // Check the name.
    if (name != Access::GetName(object)) {
        return false;
    }

    // Check the filter.
    switch (filter) {
    case NdrVersionFilterDefaultOnly:
        if (!Access::GetVersion(object).IsDefault()) {
            return false;
        }
        break;

    default:
        break;
    }

    return true;
}

template <typename T>
static
bool
_MatchesFamilyAndFilter(
    const T& object,
    const TfToken& family,
    NdrVersionFilter filter)
{
    using Access = _NdrObjectAccess<T>;

    // Check the family.
    if (!family.IsEmpty() && family != Access::GetFamily(object)) {
        return false;
    }

    // Check the filter.
    switch (filter) {
    case NdrVersionFilterDefaultOnly:
        if (!Access::GetVersion(object).IsDefault()) {
            return false;
        }
        break;

    default:
        break;
    }

    return true;
}

static NdrIdentifier
_GetIdentifierForAsset(const SdfAssetPath &asset,
                       const NdrTokenMap &metadata,
                       const TfToken &subIdentifier,
                       const TfToken &sourceType)
{
    size_t h = 0;
    boost::hash_combine(h, asset);
    for (const auto &i : metadata) { 
        boost::hash_combine(h, i.first.GetString());
        boost::hash_combine(h, i.second);
    }

    return NdrIdentifier(TfStringPrintf(
        "%s<%s><%s>",
        std::to_string(h).c_str(),
        subIdentifier.GetText(),
        sourceType.GetText()));
}

static NdrIdentifier 
_GetIdentifierForSourceCode(const std::string &sourceCode, 
                            const NdrTokenMap &metadata) 
{
    size_t h = 0;
    boost::hash_combine(h, sourceCode);
    for (const auto &i : metadata) { 
        boost::hash_combine(h, i.first.GetString());
        boost::hash_combine(h, i.second);
    }
    return NdrIdentifier(std::to_string(h));
}

static bool
_ValidateProperty(
    const NdrNodeConstPtr& node,
    const NdrPropertyConstPtr& property)
{
    std::string errorMessage;
    if (!NdrRegistry_ValidateProperty(node, property, &errorMessage)) {
        // This warning may eventually want to be a runtime error and return
        // false to indicate an invalid node, but we didn't want to introduce
        // unexpected behaviors by introducing this error.
        TF_WARN(errorMessage);
    }
    return true;
}

static
bool
_ValidateNode(const NdrNodeUniquePtr &newNode, 
              const NdrNodeDiscoveryResult &dr)
{
    // Validate the node.                                                                                                                                                                                                                                                                                                                                                                                            
    if (!newNode) {
        TF_RUNTIME_ERROR("Parser for asset @%s@ of type %s returned null",
            dr.resolvedUri.c_str(), dr.discoveryType.GetText());
        return false;
    }
    
    // The node is invalid; continue without further error checking.
    // 
    // XXX -- WBN if these were just automatically copied and parser plugins
    //        didn't have to deal with them.
    if (newNode->IsValid() &&
        !(newNode->GetIdentifier() == dr.identifier &&
          newNode->GetName() == dr.name &&
          newNode->GetVersion() == dr.version &&
          newNode->GetFamily() == dr.family &&
          newNode->GetSourceType() == dr.sourceType)) {
        TF_RUNTIME_ERROR(
               "Parsed node %s:%s:%s:%s:%s doesn't match discovery result "
               "created for asset @%s@ - "
               "%s:%s:%s:%s:%s (identifier:version:name:family:source type); "
               "discarding.",
               NdrGetIdentifierString(newNode->GetIdentifier()).c_str(),
               newNode->GetVersion().GetString().c_str(),
               newNode->GetName().c_str(),
               newNode->GetFamily().GetText(),
               newNode->GetSourceType().GetText(),
               dr.resolvedUri.c_str(),
               NdrGetIdentifierString(dr.identifier).c_str(),
               dr.version.GetString().c_str(),
               dr.name.c_str(),
               dr.family.GetText(),
               dr.sourceType.GetText());
        return false;
    }

    // It is safe to get the raw pointer from the unique pointer here since
    // this raw pointer will not be passed beyond the scope of this function.
    NdrNodeConstPtr node = newNode.get();

    // Validate the node's properties.  Always validate each property even if
    // we have already found an invalid property because we want to report
    // errors on all properties.
    bool valid = true;
    for (const TfToken& inputName : newNode->GetInputNames()) {
        const NdrPropertyConstPtr& input = newNode->GetInput(inputName);
        valid &= _ValidateProperty(node, input);
    }

    for (const TfToken& outputName : newNode->GetOutputNames()) {
        const NdrPropertyConstPtr& output = newNode->GetOutput(outputName);
        valid &= _ValidateProperty(node, output);
    }

    return valid;
}

} // anonymous namespace

class NdrRegistry::_DiscoveryContext : public NdrDiscoveryPluginContext {
public:
    _DiscoveryContext(const NdrRegistry& registry) : _registry(registry) { }
    ~_DiscoveryContext() override = default;

    TfToken GetSourceType(const TfToken& discoveryType) const override
    {
        auto parser = _registry._GetParserForDiscoveryType(discoveryType);
        return parser ? parser->GetSourceType() : TfToken();
    }

private:
    const NdrRegistry& _registry;
};

NdrRegistry::NdrRegistry()
{
    TRACE_FUNCTION();
    _FindAndInstantiateParserPlugins();
    _FindAndInstantiateDiscoveryPlugins();
    _RunDiscoveryPlugins(_discoveryPlugins);
}

NdrRegistry::~NdrRegistry()
{
    // nothing yet
}

NdrRegistry&
NdrRegistry::GetInstance()
{
    return TfSingleton<NdrRegistry>::GetInstance();
}

void
NdrRegistry::SetExtraDiscoveryPlugins(DiscoveryPluginRefPtrVec plugins)
{
    {
        std::lock_guard<std::mutex> nmLock(_nodeMapMutex);

        // This policy was implemented in order to keep internal registry
        // operations simpler, and it "just makes sense" to have all plugins
        // run before asking for information from the registry.
        if (!_nodeMap.empty()) {
            TF_CODING_ERROR("SetExtraDiscoveryPlugins() cannot be called after"
                            " nodes have been parsed; ignoring.");
            return;
        }
    }

    _RunDiscoveryPlugins(plugins);

    _discoveryPlugins.insert(_discoveryPlugins.end(),
                             std::make_move_iterator(plugins.begin()),
                             std::make_move_iterator(plugins.end()));
}

void
NdrRegistry::SetExtraDiscoveryPlugins(const std::vector<TfType>& pluginTypes)
{
    // Validate the types and remove duplicates.
    std::set<TfType> discoveryPluginTypes;
    auto& discoveryPluginType = TfType::Find<NdrDiscoveryPlugin>();
    for (auto&& type: pluginTypes) {
        if (!TF_VERIFY(type.IsA(discoveryPluginType),
                       "Type %s is not a %s",
                       type.GetTypeName().c_str(),
                       discoveryPluginType.GetTypeName().c_str())) {
            return;
        }
        discoveryPluginTypes.insert(type);
    }

    // Instantiate any discovery plugins that were found
    DiscoveryPluginRefPtrVec discoveryPlugins;
    for (const TfType& discoveryPluginType : discoveryPluginTypes) {
        NdrDiscoveryPluginFactoryBase* pluginFactory =
            discoveryPluginType.GetFactory<NdrDiscoveryPluginFactoryBase>();

        if (TF_VERIFY(pluginFactory)) {
            discoveryPlugins.emplace_back(pluginFactory->New());
        }
    }

    // Add the discovery plugins.
    SetExtraDiscoveryPlugins(std::move(discoveryPlugins));
}

void
NdrRegistry::SetExtraParserPlugins(const std::vector<TfType>& pluginTypes)
{
    {
        std::lock_guard<std::mutex> nmLock(_nodeMapMutex);

        // This policy was implemented in order to keep internal registry
        // operations simpler, and it "just makes sense" to have all plugins
        // run before asking for information from the registry.
        if (!_nodeMap.empty()) {
            TF_CODING_ERROR("SetExtraParserPlugins() cannot be called after"
                            " nodes have been parsed; ignoring.");
            return;
        }
    }

    // Validate the types and remove duplicates.
    std::set<TfType> parserPluginTypes;
    auto& parserPluginType = TfType::Find<NdrParserPlugin>();
    for (auto&& type: pluginTypes) {
        if (!TF_VERIFY(type.IsA(parserPluginType),
                       "Type %s is not a %s",
                       type.GetTypeName().c_str(),
                       parserPluginType.GetTypeName().c_str())) {
            return;
        }
        parserPluginTypes.insert(type);
    }

    _InstantiateParserPlugins(parserPluginTypes);
}

NdrNodeConstPtr 
NdrRegistry::GetNodeFromAsset(const SdfAssetPath &asset,
                              const NdrTokenMap &metadata,
                              const TfToken &subIdentifier,
                              const TfToken &sourceType)
{
    // Ensure there is a parser plugin that can handle this asset.
    TfToken discoveryType(ArGetResolver().GetExtension(asset.GetAssetPath()));
    auto parserIt = _parserPluginMap.find(discoveryType);

    // Ensure that there is a parser registered corresponding to the 
    // discoveryType of the asset.
    if (parserIt == _parserPluginMap.end()) {
        TF_DEBUG(NDR_PARSING).Msg("Encountered a asset @%s@ of type [%s], but "
                                  "a parser for the type could not be found; "
                                  "ignoring.\n", asset.GetAssetPath().c_str(),
                                  discoveryType.GetText());
        return nullptr;
    }

    NdrIdentifier identifier =
        _GetIdentifierForAsset(asset, metadata, subIdentifier, sourceType);

    // Use given sourceType if there is one, else use sourceType from the parser
    // plugin.
    const TfToken &thisSourceType = (!sourceType.IsEmpty()) ? sourceType :
        parserIt->second->GetSourceType();
    NodeMapKey key{identifier, thisSourceType};

    // Return the existing node in the map if an entry for the constructed node 
    // key already exists. 
    std::unique_lock<std::mutex> nmLock(_nodeMapMutex);
    auto it = _nodeMap.find(key);
    if (it != _nodeMap.end()) {
        // Get the raw ptr from the unique_ptr
        return it->second.get();
    }

    // Ensure the map is not locked at this point. The parse is the bulk of the
    // operation, and concurrency is the most valuable here.
    nmLock.unlock();

    // Construct a NdrNodeDiscoveryResult object to pass into the parser 
    // plugin's Parse() method.
    // XXX: Should we try resolving the assetPath if the resolved path is empty.
    std::string resolvedUri = asset.GetResolvedPath().empty() ? 
        asset.GetAssetPath() : asset.GetResolvedPath();

    NdrNodeDiscoveryResult dr(identifier,
                              NdrVersion(), /* use an invalid version */
                              /* name */ identifier, 
                              /*family*/ TfToken(), 
                              discoveryType, 
                              /* sourceType */ thisSourceType,
                              /* uri */ asset.GetAssetPath(),
                              resolvedUri, 
                              /* sourceCode */ "",
                              metadata,
                              /* blindData */ "",
                              /* subIdentifier */ subIdentifier);

    NdrNodeUniquePtr newNode = parserIt->second->Parse(dr);

    if (!_ValidateNode(newNode, dr)) {
        return nullptr;
    }

    // Move the discovery result into _discoveryResults so the node can be found
    // in the Get*() methods
    {
        std::lock_guard<std::mutex> drLock(_discoveryResultMutex);
        _discoveryResults.emplace_back(std::move(dr));
    }

    nmLock.lock();

    NodeMap::const_iterator result =
        _nodeMap.emplace(std::move(key), std::move(newNode));

    // Get the unique_ptr from the iterator, then get its raw ptr
    return result->second.get();
}

NdrNodeConstPtr 
NdrRegistry::GetNodeFromSourceCode(const std::string &sourceCode,
                                   const TfToken &sourceType,
                                   const NdrTokenMap &metadata)
{
    // Ensure that there is a parser registered corresponding to the 
    // given sourceType.
    NdrParserPlugin *parserForSourceType = nullptr;
    for (const auto &parserIt : _parserPlugins) {
        if (parserIt->GetSourceType() == sourceType) {
            parserForSourceType = parserIt.get();
        }
    }

    if (!parserForSourceType) {
        // XXX: Should we try looking for sourceType in _parserPluginMap, 
        // in case it corresponds to a discovery type in Ndr?
       
        TF_DEBUG(NDR_PARSING).Msg("Encountered source code of type [%s], but "
                                  "a parser for the type could not be found; "
                                  "ignoring.\n", sourceType.GetText());
        return nullptr;
    }

    NdrIdentifier identifier = _GetIdentifierForSourceCode(sourceCode, 
            metadata);
    NodeMapKey key{identifier, sourceType};

    // Return the existing node in the map if an entry for the constructed node 
    // key already exists. 
    std::unique_lock<std::mutex> nmLock(_nodeMapMutex);
    auto it = _nodeMap.find(key);
    if (it != _nodeMap.end()) {
        // Get the raw ptr from the unique_ptr
        return it->second.get();
    }

    // Ensure the map is not locked at this point. The parse is the bulk of the
    // operation, and concurrency is the most valuable here.
    nmLock.unlock();

    NdrNodeDiscoveryResult dr(identifier, 
                              NdrVersion(), /* use an invalid version */
                              /* name */ identifier, 
                              /*family*/ TfToken(), 
                              // XXX: Setting discoveryType also to sourceType.
                              // Do ParserPlugins rely on it? If yes, should they?
                              /* discoveryType */ sourceType, 
                              sourceType, 
                              /* uri */ "",
                              /* resolvedUri */ "",
                               sourceCode,
                               metadata);

    NdrNodeUniquePtr newNode = parserForSourceType->Parse(dr);
    if (!newNode) {
        TF_RUNTIME_ERROR("Could not create node for the given source code of "
            "source type '%s'.", sourceType.GetText());
        return nullptr;
    }

    // Move the discovery result into _discoveryResults so the node can be found
    // in the Get*() methods
    {
        std::lock_guard<std::mutex> drLock(_discoveryResultMutex);
        _discoveryResults.emplace_back(std::move(dr));
    }

    nmLock.lock();

    NodeMap::const_iterator result =
        _nodeMap.emplace(std::move(key), std::move(newNode));

    // Get the unique_ptr from the iterator, then get its raw ptr
    return result->second.get();
}

NdrStringVec
NdrRegistry::GetSearchURIs() const
{
    NdrStringVec searchURIs;

    for (const NdrDiscoveryPluginRefPtr& dp : _discoveryPlugins) {
        NdrStringVec uris = dp->GetSearchURIs();

        searchURIs.insert(searchURIs.end(),
                          std::make_move_iterator(uris.begin()),
                          std::make_move_iterator(uris.end()));
    }

    return searchURIs;
}

NdrIdentifierVec
NdrRegistry::GetNodeIdentifiers(
    const TfToken& family, NdrVersionFilter filter) const
{
    //
    // This should not trigger a parse because node names come directly from
    // the discovery process.
    //

    std::lock_guard<std::mutex> drLock(_discoveryResultMutex);

    NdrIdentifierVec result;
    result.reserve(_discoveryResults.size());

    NdrIdentifierSet visited;
    for (const NdrNodeDiscoveryResult& dr : _discoveryResults) {
        if (_MatchesFamilyAndFilter(dr, family, filter)) {
            // Avoid duplicates.
            if (visited.insert(dr.identifier).second) {
                result.push_back(dr.identifier);
            }
        }
    }

    return result;
}

NdrStringVec
NdrRegistry::GetNodeNames(const TfToken& family) const
{
    //
    // This should not trigger a parse because node names come directly from
    // the discovery process.
    //

    std::lock_guard<std::mutex> drLock(_discoveryResultMutex);

    NdrStringVec nodeNames;
    nodeNames.reserve(_discoveryResults.size());

    NdrStringSet visited;
    for (const NdrNodeDiscoveryResult& dr : _discoveryResults) {
        if (family.IsEmpty() || dr.family == family) {
            // Avoid duplicates.
            if (visited.insert(dr.name).second) {
                nodeNames.push_back(dr.name);
            }
        }
    }

    return nodeNames;
}

NdrNodeConstPtr
NdrRegistry::GetNodeByIdentifier(
    const NdrIdentifier& identifier, const NdrTokenVec& typePriority)
{
    return _GetNodeByTypePriority(GetNodesByIdentifier(identifier),
                                  typePriority);
}

NdrNodeConstPtr
NdrRegistry::GetNodeByIdentifierAndType(
    const NdrIdentifier& identifier, const TfToken& nodeType)
{
    return NdrRegistry::GetNodeByIdentifier(identifier,NdrTokenVec({nodeType}));
}

NdrNodeConstPtr
NdrRegistry::GetNodeByName(
    const std::string& name,
    const NdrTokenVec& typePriority,
    NdrVersionFilter filter)
{
    return _GetNodeByTypePriority(GetNodesByName(name, filter), typePriority);
}

NdrNodeConstPtr
NdrRegistry::GetNodeByNameAndType(
    const std::string& name, const TfToken& nodeType, NdrVersionFilter filter)
{
    return NdrRegistry::GetNodeByName(name, NdrTokenVec({nodeType}), filter);
}

NdrNodeConstPtr
NdrRegistry::_GetNodeByTypePriority(
    const NdrNodeConstPtrVec& nodes,
    const NdrTokenVec& typePriority)
{
    // If the type priority specifier is empty, pick the first node that matches
    // the name
    if (typePriority.empty() && !nodes.empty()) {
        return nodes.front();
    }

    // Although this is a doubly-nested loop, the number of types in the
    // priority list should be small as should the number of nodes.
    for (const TfToken& nodeType : typePriority) {
        for (const NdrNodeConstPtr& node : nodes) {
            if (node->GetSourceType() == nodeType) {
                return node;
            }
        }
    }

    return nullptr;
}

NdrNodeConstPtr
NdrRegistry::GetNodeByURI(const std::string& uri)
{
    NdrNodeConstPtrVec parsedNodes = _ParseNodesMatchingPredicate(
        [&uri](const NdrNodeDiscoveryResult& dr) {
            return dr.uri == uri;
        },
        true // onlyParseFirstMatch
    );

    if (!parsedNodes.empty()) {
        return parsedNodes[0];
    }

    return nullptr;
}

NdrNodeConstPtrVec
NdrRegistry::GetNodesByIdentifier(const NdrIdentifier& identifier)
{
    return _ParseNodesMatchingPredicate(
        [&identifier](const NdrNodeDiscoveryResult& dr) {
            return dr.identifier == identifier;
        },
        false // onlyParseFirstMatch
    );
}

NdrNodeConstPtrVec
NdrRegistry::GetNodesByName(const std::string& name, NdrVersionFilter filter)
{
    return _ParseNodesMatchingPredicate(
        [&name, filter](const NdrNodeDiscoveryResult& dr) {
            return _MatchesNameAndFilter(dr, name, filter);
        },
        false // onlyParseFirstMatch
    );
}

NdrNodeConstPtrVec
NdrRegistry::GetNodesByFamily(const TfToken& family, NdrVersionFilter filter)
{
    // Locking the discovery results for the entire duration of the parse is a
    // bit heavy-handed, but it needs to be 100% guaranteed that the results are
    // not modified while they are being iterated over.
    std::lock_guard<std::mutex> drLock(_discoveryResultMutex);

    // This method does a multi-threaded "bulk parse" of all discovered nodes
    // (or a partial parse if a family is specified). It's possible that another
    // node access method (potentially triggering a parse) could be called in
    // another thread during bulk parse. In that scenario, the worst that should
    // happen is that one of the parses (either from the other method, or this
    // bulk parse) is discarded in favor of the other parse result
    // (_InsertNodeIntoCache() will guard against nodes of the same name and
    // type from being cached).
    {
        std::lock_guard<std::mutex> nmLock(_nodeMapMutex);

        // Skip parsing if a parse was already completed for all nodes
        if (_nodeMap.size() == _discoveryResults.size()) {
            return _GetNodeMapAsNodePtrVec(family, filter);
        }
    }

    // Do the parsing
    WorkParallelForN(_discoveryResults.size(),
        [&](size_t begin, size_t end) {
            for (size_t i = begin; i < end; ++i) {
                const NdrNodeDiscoveryResult& dr = _discoveryResults.at(i);
                if (_MatchesFamilyAndFilter(dr, family, filter)) {
                    _InsertNodeIntoCache(dr);
                }
            }
        }
    );

    // Expose the concurrent map as a normal vector to the outside world
    return _GetNodeMapAsNodePtrVec(family, filter);
}

NdrTokenVec
NdrRegistry::GetAllNodeSourceTypes() const 
{
    // We're using the _discoveryResultMutex because we populate the
    // _availableSourceTypes while creating the _discoveryResults.
    //
    // We also have to return the source types by value instead of by const
    // reference because we don't want a client holding onto the reference
    // to read from it when _RunDiscoveryPlugins could potentially be running
    // and modifying _availableSourceTypes
    std::lock_guard<std::mutex> drLock(_discoveryResultMutex);
    return _availableSourceTypes;
}

NdrNodeConstPtrVec
NdrRegistry::_ParseNodesMatchingPredicate(
    std::function<bool(const NdrNodeDiscoveryResult&)> shouldParsePredicate,
    bool onlyParseFirstMatch)
{
    std::lock_guard<std::mutex> drLock(_discoveryResultMutex);
    NdrNodeConstPtrVec parsedNodes;

    for (const NdrNodeDiscoveryResult& dr : _discoveryResults) {
        if (!shouldParsePredicate(dr)) {
            continue;
        }

        NdrNodeConstPtr parsedNode = _InsertNodeIntoCache(dr);

        if (parsedNode) {
            parsedNodes.emplace_back(std::move(parsedNode));
        }

        if (onlyParseFirstMatch) {
            break;
        }
    }

    return parsedNodes;
}

void
NdrRegistry::_FindAndInstantiateDiscoveryPlugins()
{
    // The auto-discovery of discovery plugins can be skipped. This is mostly
    // for testing purposes.
    if (TfGetEnvSetting(PXR_NDR_SKIP_DISCOVERY_PLUGIN_DISCOVERY)) {
        return;
    }

    // Find all of the available discovery plugins
    std::set<TfType> discoveryPluginTypes;
    PlugRegistry::GetInstance().GetAllDerivedTypes<NdrDiscoveryPlugin>(
        &discoveryPluginTypes);

    // Instantiate any discovery plugins that were found
    for (const TfType& discoveryPluginType : discoveryPluginTypes) {
        TF_DEBUG(NDR_DISCOVERY).Msg(
            "Found NdrDiscoveryPlugin '%s'\n", 
            discoveryPluginType.GetTypeName().c_str());

        NdrDiscoveryPluginFactoryBase* pluginFactory =
            discoveryPluginType.GetFactory<NdrDiscoveryPluginFactoryBase>();

        if (TF_VERIFY(pluginFactory)) {
            _discoveryPlugins.emplace_back(pluginFactory->New());
        }
    }
}

void
NdrRegistry::_FindAndInstantiateParserPlugins()
{
    // The auto-discovery of parser plugins can be skipped. This is mostly
    // for testing purposes.
    if (TfGetEnvSetting(PXR_NDR_SKIP_PARSER_PLUGIN_DISCOVERY)) {
        return;
    }

    // Find all of the available parser plugins
    std::set<TfType> parserPluginTypes;
    PlugRegistry::GetInstance().GetAllDerivedTypes<NdrParserPlugin>(
        &parserPluginTypes);

    _InstantiateParserPlugins(parserPluginTypes);
}

void
NdrRegistry::_InstantiateParserPlugins(
    const std::set<TfType>& parserPluginTypes)
{
    // Instantiate any parser plugins that were found
    for (const TfType& parserPluginType : parserPluginTypes) {
        TF_DEBUG(NDR_DISCOVERY).Msg(
            "Found NdrParserPlugin '%s' for discovery types:\n", 
            parserPluginType.GetTypeName().c_str());

        NdrParserPluginFactoryBase* pluginFactory =
            parserPluginType.GetFactory<NdrParserPluginFactoryBase>();

        if (!TF_VERIFY(pluginFactory)) {
            continue;
        }

        NdrParserPlugin* parserPlugin = pluginFactory->New();
        _parserPlugins.emplace_back(parserPlugin);

        for (const TfToken& discoveryType : parserPlugin->GetDiscoveryTypes()) {
            TF_DEBUG(NDR_DISCOVERY).Msg("  - %s\n", discoveryType.GetText());

            auto i = _parserPluginMap.insert({discoveryType, parserPlugin});
            if (!i.second){
                const TfType otherType = TfType::Find(*i.first->second);
                TF_CODING_ERROR("Plugin type %s claims discovery type '%s' "
                                "but that's already claimed by type %s",
                                parserPluginType.GetTypeName().c_str(),
                                discoveryType.GetText(),
                                otherType.GetTypeName().c_str());
            }
        }
    }
}

void
NdrRegistry::_RunDiscoveryPlugins(const DiscoveryPluginRefPtrVec& discoveryPlugins)
{
    std::lock_guard<std::mutex> drLock(_discoveryResultMutex);

    for (const NdrDiscoveryPluginRefPtr& dp : discoveryPlugins) {
        NdrNodeDiscoveryResultVec results =
            dp->DiscoverNodes(_DiscoveryContext(*this));

        for (const NdrNodeDiscoveryResult& result : results) {
            if (!result.sourceType.IsEmpty()) {
                // Populate the source types that the registry knows about from
                // the source types we discover
                NdrTokenVec::iterator it = std::lower_bound(
                    _availableSourceTypes.begin(),
                    _availableSourceTypes.end(),
                    result.sourceType);
                if (it == _availableSourceTypes.end() ||
                    result.sourceType != *it) {
                    // The vector will be sorted because we always insert the
                    // current result's source type before the first item in the
                    // vector that does not compare less than the current source
                    // type.  We don't insert the source type if the iterator
                    // we get back is pointing to a source type that is the
                    // same, thus avoiding duplicates.
                    _availableSourceTypes.insert(it, result.sourceType);
                }
            }
        }

        _discoveryResults.insert(_discoveryResults.end(),
                                 std::make_move_iterator(results.begin()),
                                 std::make_move_iterator(results.end()));
    }
}

NdrNodeConstPtr
NdrRegistry::_InsertNodeIntoCache(const NdrNodeDiscoveryResult& dr)
{
    // Return an existing node in the map if the new node matches the
    // identifier AND source type of a node in the map.
    std::unique_lock<std::mutex> nmLock(_nodeMapMutex);
    NodeMapKey key{dr.identifier, dr.sourceType};
    auto it = _nodeMap.find(key);
    if (it != _nodeMap.end()) {
        // Get the raw ptr from the unique_ptr
        return it->second.get();
    }

    // Ensure the map is not locked at this point. The parse is the bulk of the
    // operation, and concurrency is the most valuable here.
    nmLock.unlock();

    // Ensure there is a parser plugin that can handle this node
    auto i = _parserPluginMap.find(dr.discoveryType);
    if (i == _parserPluginMap.end()) {
        TF_DEBUG(NDR_PARSING).Msg("Encountered a node of type [%s], "
                                  "with name [%s], but a parser for that type "
                                  "could not be found; ignoring.\n", 
                                  dr.discoveryType.GetText(),  dr.name.c_str());
        return nullptr;
    }

    NdrNodeUniquePtr newNode = i->second->Parse(dr);

    // Validate the node.
    if (!_ValidateNode(newNode, dr)) {
        return nullptr;
    }
    
    nmLock.lock();

    NodeMap::const_iterator result =
        _nodeMap.emplace(std::move(key), std::move(newNode));

    // Get the unique_ptr from the iterator, then get its raw ptr
    return result->second.get();
}

NdrNodeConstPtrVec
NdrRegistry::_GetNodeMapAsNodePtrVec(
    const TfToken& family, NdrVersionFilter filter) const
{
    NdrNodeConstPtrVec _nodeVec;

    for (const auto& nodePair : _nodeMap) {
        if (_MatchesFamilyAndFilter(nodePair.second, family, filter)) {
            _nodeVec.emplace_back(nodePair.second.get());
        }
    }

    return _nodeVec;
}

NdrParserPlugin*
NdrRegistry::_GetParserForDiscoveryType(const TfToken& discoveryType) const
{
    auto i = _parserPluginMap.find(discoveryType);
    return i == _parserPluginMap.end() ? nullptr : i->second;
}

PXR_NAMESPACE_CLOSE_SCOPE
