//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/instantiateSingleton.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/withScopedParallelism.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/sdr/debugCodes.h"
#include "pxr/usd/sdr/discoveryPlugin.h"
#include "pxr/usd/sdr/registry.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderNodeDiscoveryResult.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/tf/diagnostic.h"

#include "pxr/base/plug/registry.h"
#include "pxr/base/tf/envSetting.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(
    PXR_SDR_SKIP_DISCOVERY_PLUGIN_DISCOVERY, 0,
    "The auto-discovery of discovery plugins in sdr can be skipped. "
    "This is used mostly for testing purposes.");

TF_DEFINE_ENV_SETTING(
    PXR_SDR_SKIP_PARSER_PLUGIN_DISCOVERY, 0,
    "The auto-discovery of parser plugins in sdr can be skipped. "
    "This is used mostly for testing purposes.");

TF_DEFINE_ENV_SETTING(
    PXR_SDR_DISABLE_PLUGINS, "",
    "Comma separated list of Sdr plugins to disable.  Note that disabling "
    "plugins may cause shaders in your scenes to malfunction.");

TF_INSTANTIATE_SINGLETON(SdrRegistry);

// This function is used for property validation. It explictly is validating 
// that the sdfType and sdfTypeDefaultValue have the same type. Note how it is
// calling the methods GetTypeAsSdfType() and GetDefaultValueAsSdfType() 
// explicitly as opposed to GetType() and GetDefaultValue().
//
// This function is written as a non-static freestanding function so that we can
// exercise it in a test without needing to expose it in the header file.  It is
// also written without using unique pointers for ease of python wrapping and 
// testability.
SDR_API
bool
SdrRegistry_ValidateProperty(
    const SdrShaderNodeConstPtr& node,
    const SdrShaderPropertyConstPtr& property,
    std::string* errorMessage)
{
    const VtValue& defaultValue = property->GetDefaultValueAsSdfType();
    const SdrSdfTypeIndicator sdfTypeIndicator = property->GetTypeAsSdfType();
    const SdfValueTypeName sdfType = sdfTypeIndicator.GetSdfType();

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
template <typename T> struct _SdrObjectAccess { };
template <> struct _SdrObjectAccess<SdrShaderNodeDiscoveryResult> {
    typedef SdrShaderNodeDiscoveryResult Type;
    static const std::string& GetName(const Type& x) { return x.name; }
    static const TfToken& GetFamily(const Type& x) { return x.family; }
    static SdrVersion GetShaderVersion(const Type& x) { return x.version; }
};
template <> struct _SdrObjectAccess<SdrShaderNodeUniquePtr> {
    typedef SdrShaderNodeUniquePtr Type;
    static const std::string& GetName(const Type& x) { return x->GetName(); }
    static const TfToken& GetFamily(const Type& x) { return x->GetFamily(); }
    static SdrVersion GetShaderVersion(const Type& x) { return x->GetShaderVersion(); }
};

template <typename T>
static
bool
_MatchesFamilyAndFilter(
    const T& object,
    const TfToken& family,
    SdrVersionFilter filter)
{
    using Access = _SdrObjectAccess<T>;

    // Check the family.
    if (!family.IsEmpty() && family != Access::GetFamily(object)) {
        return false;
    }

    // Check the filter.
    switch (filter) {
    case SdrVersionFilterDefaultOnly:
        if (!Access::GetShaderVersion(object).IsDefault()) {
            return false;
        }
        break;

    default:
        break;
    }

    return true;
}

static SdrIdentifier
_GetIdentifierForAsset(const SdfAssetPath &asset,
                       const SdrTokenMap &metadata,
                       const TfToken &subIdentifier,
                       const TfToken &sourceType)
{
    size_t h = TfHash()(asset);
    for (const auto &i : metadata) {
        h = TfHash::Combine(h, i.first.GetString(), i.second);
    }

    return SdrIdentifier(TfStringPrintf(
        "%s<%s><%s>",
        std::to_string(h).c_str(),
        subIdentifier.GetText(),
        sourceType.GetText()));
}

static SdrIdentifier 
_GetIdentifierForSourceCode(const std::string &sourceCode, 
                            const SdrTokenMap &metadata) 
{
    size_t h = TfHash()(sourceCode);
    for (const auto &i : metadata) {
        h = TfHash::Combine(h, i.first.GetString(), i.second);
    }
    return SdrIdentifier(std::to_string(h));
}

static bool
_ValidateProperty(
    const SdrShaderNodeConstPtr& node,
    const SdrShaderPropertyConstPtr& property)
{
    std::string errorMessage;
    if (!SdrRegistry_ValidateProperty(node, property, &errorMessage)) {
        // This warning may eventually want to be a runtime error and return
        // false to indicate an invalid node, but we didn't want to introduce
        // unexpected behaviors by introducing this error.
        TF_WARN(errorMessage);
    }
    return true;
}

static
bool
_ValidateNode(const SdrShaderNodeUniquePtr &newNode, 
              const SdrShaderNodeDiscoveryResult &dr)
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
          newNode->GetShaderVersion() == dr.version &&
          newNode->GetFamily() == dr.family &&
          newNode->GetSourceType() == dr.sourceType)) {
        TF_RUNTIME_ERROR(
               "Parsed node %s:%s:%s:%s:%s doesn't match discovery result "
               "created for asset @%s@ - "
               "%s:%s:%s:%s:%s (identifier:version:name:family:source type); "
               "discarding.",
               SdrGetIdentifierString(newNode->GetIdentifier()).c_str(),
               newNode->GetShaderVersion().GetString().c_str(),
               newNode->GetName().c_str(),
               newNode->GetFamily().GetText(),
               newNode->GetSourceType().GetText(),
               dr.resolvedUri.c_str(),
               SdrGetIdentifierString(dr.identifier).c_str(),
               dr.version.GetString().c_str(),
               dr.name.c_str(),
               dr.family.GetText(),
               dr.sourceType.GetText());
        return false;
    }

    // It is safe to get the raw pointer from the unique pointer here since
    // this raw pointer will not be passed beyond the scope of this function.
    SdrShaderNodeConstPtr node = newNode.get();

    // Validate the node's properties.  Always validate each property even if
    // we have already found an invalid property because we want to report
    // errors on all properties.
    bool valid = true;
    for (const TfToken& inputName : newNode->GetShaderInputNames()) {
        const SdrShaderPropertyConstPtr& input =
            newNode->GetShaderInput(inputName);
        valid &= _ValidateProperty(node, input);
    }

    for (const TfToken& outputName : newNode->GetShaderOutputNames()) {
        const SdrShaderPropertyConstPtr& output =
            newNode->GetShaderOutput(outputName);
        valid &= _ValidateProperty(node, output);
    }

    return valid;
}

} // anonymous namespace

class SdrRegistry::_DiscoveryContext : public SdrDiscoveryPluginContext {
public:
    _DiscoveryContext(const SdrRegistry& registry) : _registry(registry) { }
    ~_DiscoveryContext() override = default;

    TfToken GetSourceType(const TfToken& discoveryType) const override
    {
        auto parser = _registry._GetParserForDiscoveryType(discoveryType);
        return parser ? parser->GetSourceType() : TfToken();
    }

private:
    const SdrRegistry& _registry;
};

SdrRegistry::SdrRegistry()
{
    TRACE_FUNCTION();
    _FindAndInstantiateParserPlugins();
    _FindAndInstantiateDiscoveryPlugins();
    _RunDiscoveryPlugins(_discoveryPlugins);
}

SdrRegistry::~SdrRegistry()
{
}

SdrRegistry&
SdrRegistry::GetInstance()
{
    return TfSingleton<SdrRegistry>::GetInstance();
}

void
SdrRegistry::SetExtraDiscoveryPlugins(DiscoveryPluginRefPtrVec plugins)
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
SdrRegistry::SetExtraDiscoveryPlugins(const std::vector<TfType>& pluginTypes)
{
    // Validate the types and remove duplicates.
    std::set<TfType> discoveryPluginTypes;
    auto& discoveryPluginType = TfType::Find<SdrDiscoveryPlugin>();
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
        SdrDiscoveryPluginFactoryBase* pluginFactory =
            discoveryPluginType.GetFactory<SdrDiscoveryPluginFactoryBase>();

        if (TF_VERIFY(pluginFactory)) {
            discoveryPlugins.emplace_back(pluginFactory->New());
        }
    }

    // Add the discovery plugins.
    SetExtraDiscoveryPlugins(std::move(discoveryPlugins));
}

void
SdrRegistry::AddDiscoveryResult(SdrShaderNodeDiscoveryResult&& discoveryResult)
{
    std::lock_guard<std::mutex> drLock(_discoveryResultMutex);
    _AddDiscoveryResultNoLock(std::move(discoveryResult));
}

void
SdrRegistry::AddDiscoveryResult(
    const SdrShaderNodeDiscoveryResult& discoveryResult)
{
    // Explicitly create a copy, otherwise this method will recurse
    // into itself.
    SdrShaderNodeDiscoveryResult result = discoveryResult;
    AddDiscoveryResult(std::move(result));
}

void
SdrRegistry::SetExtraParserPlugins(const std::vector<TfType>& pluginTypes)
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
    auto& parserPluginType = TfType::Find<SdrParserPlugin>();
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

SdrStringVec
SdrRegistry::GetSearchURIs() const
{
    SdrStringVec searchURIs;

    for (const SdrDiscoveryPluginRefPtr& dp : _discoveryPlugins) {
        SdrStringVec uris = dp->GetSearchURIs();

        searchURIs.insert(searchURIs.end(),
                          std::make_move_iterator(uris.begin()),
                          std::make_move_iterator(uris.end()));
    }

    return searchURIs;
}

SdrIdentifierVec
SdrRegistry::GetShaderNodeIdentifiers(
    const TfToken& family,
    SdrVersionFilter filter) const
{
    //
    // This should not trigger a parse because node names come directly from
    // the discovery process.
    //

    std::lock_guard<std::mutex> drLock(_discoveryResultMutex);

    SdrIdentifierVec result;
    result.reserve(_discoveryResultsByIdentifier.size());

    for (const auto& it : _discoveryResultsByIdentifier) {
        const SdrShaderNodeDiscoveryResult& dr = it.second;
        if (_MatchesFamilyAndFilter(dr, family, filter)) {
            // Since the discovery results are keyed by identifier in a
            // multimap, any duplicate idenitfiers will show up in order so we
            // only have to check the last identifier we added to avoid
            // duplicates. 
            if (result.empty() || result.back() != dr.identifier) {
                result.push_back(dr.identifier);
            }
        }
    }

    return result;
}

SdrStringVec
SdrRegistry::GetShaderNodeNames(const TfToken& family) const
{
    //
    // This should not trigger a parse because node names come directly from
    // the discovery process.
    //

    std::lock_guard<std::mutex> drLock(_discoveryResultMutex);

    SdrStringVec nodeNames;
    nodeNames.reserve(_discoveryResultPtrsByName.size());

    for (const auto& it : _discoveryResultPtrsByName) {
        const SdrShaderNodeDiscoveryResult& dr = *(it.second);
        if (family.IsEmpty() || dr.family == family) {
            // Since the discovery results are keyed by name in a multimap, any
            // duplicate names will show up in order so we only have to check
            // the last name we added to avoid duplicates. 
            if (nodeNames.empty() || nodeNames.back() != dr.name) {
                nodeNames.push_back(dr.name);
            }
        }
    }

    return nodeNames;
}


SdrShaderNodeConstPtr
SdrRegistry::GetShaderNodeByIdentifier(
    const SdrIdentifier& identifier, const SdrTokenVec& typePriority)
{
    TRACE_FUNCTION();
    std::lock_guard<std::mutex> drLock(_discoveryResultMutex);

    // There can be multiple discovery results for different source types for a
    // single identifier so get the range of results for the identifier. 
    const _DiscoveryResultsByIdentifierRange range = 
        _discoveryResultsByIdentifier.equal_range(identifier);
    if (range.first == range.second) {
        return nullptr;
    }

    if (typePriority.empty()) {
        // If the type priority specifier is empty, pick the first valid node
        // that matches the identifier regardless of source type.
        for (auto it = range.first; it != range.second; ++it) {
            if (SdrShaderNodeConstPtr node =
                    _FindOrParseNodeInCache(it->second)) {
                return node;
            }
        }
    } else {
        // Otherwise we attempt to get a node for matching the identifier for 
        // each source type in priority order.
        for (const TfToken& sourceType : typePriority) {
            SdrShaderNodeConstPtr node =
                _GetNodeInIdentifierRangeWithSourceType(range, sourceType);
            if (node) {
                return node;
            }
        }
    }

    return nullptr;
}

SdrShaderNodeConstPtr
SdrRegistry::GetShaderNodeByIdentifierAndType(
    const SdrIdentifier& identifier, const TfToken& nodeType)
{
    TRACE_FUNCTION();
    std::lock_guard<std::mutex> drLock(_discoveryResultMutex);

    // There can be multiple discovery results for different source types for a
    // single identifier so get the range of results for the identifier. 
    const _DiscoveryResultsByIdentifierRange range = 
        _discoveryResultsByIdentifier.equal_range(identifier);
    if (range.first == range.second) {
        return nullptr;
    }
    return _GetNodeInIdentifierRangeWithSourceType(range, nodeType);
}

SdrShaderNodeConstPtr 
SdrRegistry::GetShaderNodeFromAsset(
    const SdfAssetPath &shaderAsset,
    const SdrTokenMap &metadata,
    const TfToken &subIdentifier,
    const TfToken &sourceType)
{
    // Ensure there is a parser plugin that can handle this asset.
    TfToken discoveryType(ArGetResolver().GetExtension(
        shaderAsset.GetAssetPath()));
    auto parserIt = _parserPluginMap.find(discoveryType);

    // Ensure that there is a parser registered corresponding to the 
    // discoveryType of the asset.
    if (parserIt == _parserPluginMap.end()) {
        TF_DEBUG(SDR_PARSING).Msg("Encountered a asset @%s@ of type [%s], but "
                                  "a parser for the type could not be found; "
                                  "ignoring.\n",
                                  shaderAsset.GetAssetPath().c_str(),
                                  discoveryType.GetText());
        return nullptr;
    }

    SdrIdentifier identifier =
        _GetIdentifierForAsset(shaderAsset, metadata, subIdentifier, sourceType);

    // Use given sourceType if there is one, else use sourceType from the parser
    // plugin.
    const TfToken &thisSourceType = (!sourceType.IsEmpty()) ? sourceType :
        parserIt->second->GetSourceType();
    _ShaderNodeMapKey key{identifier, thisSourceType};

    // Return the existing node in the map if an entry for the identifier and
    // sourceType already exists. Note that the existing node may not yet be 
    // parsed, so this will parse and return the node if it should exist 
    // already.
    if (SdrShaderNodeConstPtr node = 
            GetShaderNodeByIdentifierAndType(identifier, sourceType)) {
        return node;
    }

    // Construct a SdrShaderNodeDiscoveryResult object to pass into the parser 
    // plugin's ParseShaderNode() method.
    // XXX: Should we try resolving the assetPath if the resolved path is empty.
    std::string resolvedUri = shaderAsset.GetResolvedPath().empty() ? 
        shaderAsset.GetAssetPath() : shaderAsset.GetResolvedPath();

    SdrShaderNodeDiscoveryResult dr(identifier,
                                    SdrVersion(), /* use an invalid version */
                                    /* name */ TfGetBaseName(resolvedUri),
                                    /*family*/ TfToken(), 
                                    discoveryType, 
                                    /* sourceType */ thisSourceType,
                                    /* uri */ shaderAsset.GetAssetPath(),
                                    resolvedUri, 
                                    /* sourceCode */ "",
                                    metadata,
                                    /* blindData */ "",
                                    /* subIdentifier */ subIdentifier);

    return _ParseNodeFromAssetOrSourceCode(*(parserIt->second), std::move(dr));
}

SdrShaderNodeConstPtr 
SdrRegistry::GetShaderNodeFromSourceCode(
    const std::string &sourceCode,
    const TfToken &sourceType,
    const SdrTokenMap &metadata)
{
    // Ensure that there is a parser registered corresponding to the 
    // given sourceType.
    SdrParserPlugin *parserForSourceType = nullptr;
    for (const auto &parserIt : _parserPlugins) {
        if (parserIt->GetSourceType() == sourceType) {
            parserForSourceType = parserIt.get();
        }
    }

    if (!parserForSourceType) {
        // XXX: Should we try looking for sourceType in _parserPluginMap, 
        // in case it corresponds to a discovery type in Sdr?
       
        TF_DEBUG(SDR_PARSING).Msg("Encountered source code of type [%s], but "
                                  "a parser for the type could not be found; "
                                  "ignoring.\n", sourceType.GetText());
        return nullptr;
    }

    SdrIdentifier identifier = _GetIdentifierForSourceCode(sourceCode, 
            metadata);

    // Return the existing node in the map if an entry for the identifier and
    // sourceType already exists. Note that the existing node may not yet be 
    // parsed, so this will parse and return the node if it should exist 
    // already.
    if (SdrShaderNodeConstPtr node = 
            GetShaderNodeByIdentifierAndType(identifier, sourceType)) {
        return node;
    }

    SdrShaderNodeDiscoveryResult dr(identifier, 
                                    SdrVersion(), /* use an invalid version */
                                    /* name */ identifier, 
                                    /*family*/ TfToken(), 
                                    // XXX: Setting discoveryType also to
                                    // sourceType. Do ParserPlugins rely on it?
                                    // If yes, should they?
                                    /* discoveryType */ sourceType, 
                                    sourceType, 
                                    /* uri */ "",
                                    /* resolvedUri */ "",
                                    sourceCode,
                                    metadata);

    SdrShaderNodeConstPtr node = 
        _ParseNodeFromAssetOrSourceCode(*parserForSourceType, std::move(dr));
    if (!node) {
        TF_RUNTIME_ERROR("Could not create node for the given source code of "
            "source type '%s'.", sourceType.GetText());
        return nullptr;
    }
    return node;
}

SdrShaderNodeConstPtr
SdrRegistry::GetShaderNodeByName(
    const std::string& name, const SdrTokenVec& typePriority,
    SdrVersionFilter filter)
{
    TRACE_FUNCTION();
    std::lock_guard<std::mutex> drLock(_discoveryResultMutex);

    // There can be multiple discovery results with the same name so get the 
    // range of results with the given name. 
    const _DiscoveryResultPtrsByNameRange range = 
        _discoveryResultPtrsByName.equal_range(name);
    if (range.first == range.second) {
        return nullptr;
    }

    // If the type priority specifier is empty, pick the first node that
    // matches the name
    if (typePriority.empty()) {
        // If the type priority specifier is empty, pick the first valid node
        // that passes the version filter regardless of source type.
        for (auto it = range.first; it != range.second; ++ it) {
            const SdrShaderNodeDiscoveryResult &dr = *(it->second);
            if (!_MatchesFamilyAndFilter(dr, TfToken(), filter)) {
                continue;
            }
            if (SdrShaderNodeConstPtr node = _FindOrParseNodeInCache(dr)) {
                return node;
            }
        }
    } else {
        // Otherwise we attempt to get a node that passes the version filter
        // for each source type in priority order.
        for (const TfToken& sourceType : typePriority) {
            SdrShaderNodeConstPtr node = _GetNodeInNameRangeWithSourceType(
                range, sourceType, filter);
            if (node) {
                return node;
            }
        }
    }

    return nullptr;
}

SdrShaderNodeConstPtr
SdrRegistry::GetShaderNodeByNameAndType(
    const std::string& name, const TfToken& nodeType,
    SdrVersionFilter filter)
{
    TRACE_FUNCTION();
    std::lock_guard<std::mutex> drLock(_discoveryResultMutex);

    // There can be multiple discovery results with the same name so get the 
    // range of results with the given name. 
    const _DiscoveryResultPtrsByNameRange range = 
        _discoveryResultPtrsByName.equal_range(name);
    if (range.first == range.second) {
        return nullptr;
    }
    
    return _GetNodeInNameRangeWithSourceType(range, nodeType, filter);
}

SdrShaderNodePtrVec
SdrRegistry::GetShaderNodesByIdentifier(const SdrIdentifier& identifier)
{
    TRACE_FUNCTION();
    std::lock_guard<std::mutex> drLock(_discoveryResultMutex);
    SdrShaderNodeConstPtrVec parsedNodes;

    const _DiscoveryResultsByIdentifierRange range = 
        _discoveryResultsByIdentifier.equal_range(identifier);
    if (range.first == range.second) {
        return parsedNodes;
    }

    for (auto it = range.first; it != range.second; ++it) {
        if (SdrShaderNodeConstPtr node =
                _FindOrParseNodeInCache(it->second)) {
            parsedNodes.push_back(node);
        }
    }

    return parsedNodes;
}

SdrShaderNodePtrVec
SdrRegistry::GetShaderNodesByName(
    const std::string& name,
    SdrVersionFilter filter)
{
    TRACE_FUNCTION();
    std::lock_guard<std::mutex> drLock(_discoveryResultMutex);
    SdrShaderNodeConstPtrVec parsedNodes;

    const _DiscoveryResultPtrsByNameRange range = 
        _discoveryResultPtrsByName.equal_range(name);
    if (range.first == range.second) {
        return parsedNodes;
    }

    for (auto it = range.first; it != range.second; ++ it) {
        const SdrShaderNodeDiscoveryResult &dr = *(it->second);
        if (!_MatchesFamilyAndFilter(dr, TfToken(), filter)) {
            continue;
        }
        if (SdrShaderNodeConstPtr node =
                _FindOrParseNodeInCache(dr)) {
            parsedNodes.push_back(node);
        }
    }

    return parsedNodes;
}

SdrShaderNodePtrVec
SdrRegistry::GetShaderNodesByFamily(
    const TfToken& family,
    SdrVersionFilter filter)
{
    // Locking the discovery results for the entire duration of the parse is a
    // bit heavy-handed, but it needs to be 100% guaranteed that the results
    // atr not modified while they are being iterated over.
    std::lock_guard<std::mutex> drLock(_discoveryResultMutex);

    // The node map needs to be locked too while we generate a vector from its
    // contents.
    std::unique_lock<std::mutex> nmLock(_nodeMapMutex);

    // This method does a multi-threaded "bulk parse" of all discovered nodes
    // (or a partial parse if a family is specified). It's possible that another
    // node access method (potentially triggering a parse) could be called in
    // another thread during bulk parse. In that scenario, the worst that should
    // happen is that one of the parses (either from the other method, or this
    // bulk parse) is discarded in favor of the other parse result
    // (_FindOrParseNodeInCache() will guard against nodes of the same name and
    // type from being cached).

    // Skip parsing if a parse was already completed for all nodes    
    if (_nodeMap.size() != _discoveryResultsByIdentifier.size()) {
        // We unlock the node map so we can parse and insert nodes in parallel.
        nmLock.unlock();

        // Do the parsing. We need to release the Python GIL here to avoid
        // deadlocks since the code running in the worker threads may call into
        // Python and try to take the GIL when loading plugins. We also need
        // to use scoped parallelism to ensure we don't pick up other tasks
        // during the call to WorkParallelForN that may reenter this function
        // and also deadlock.
        TF_PY_ALLOW_THREADS_IN_SCOPE();

        WorkWithScopedParallelism([&]() {
            WorkParallelForEach(_discoveryResultsByIdentifier.begin(),
                                _discoveryResultsByIdentifier.end(),
                [&](const _DiscoveryResultsByIdentifier::value_type &val) {
                    if (_MatchesFamilyAndFilter(val.second, family, filter)) {
                        _FindOrParseNodeInCache(val.second);
                    }
                });
            }
        );

        nmLock.lock();
    }

    // Expose the concurrent map as a normal vector to the outside world
    SdrShaderNodeConstPtrVec nodeVec;
    nodeVec.reserve(_nodeMap.size());
    for (const auto& nodePair : _nodeMap) {
        if (_MatchesFamilyAndFilter(nodePair.second, family, filter)) {
            nodeVec.push_back(nodePair.second.get());
        }
    }
    return nodeVec;
}

SdrTokenVec
SdrRegistry::GetAllShaderNodeSourceTypes() const
{
    // We're using the _discoveryResultMutex because we populate/udpate the
    // _allSourceTypes in tandem with the population of the discovery results
    // structures.
    //
    // We also have to return the source types by value instead of by const
    // reference because we don't want a client holding onto the reference
    // to read from it when _RunDiscoveryPlugins could potentially be running
    // and modifying _allSourceTypes
    std::lock_guard<std::mutex> drLock(_discoveryResultMutex);
    return SdrTokenVec(_allSourceTypes.begin(), _allSourceTypes.end());
}


void
SdrRegistry::_FindAndInstantiateDiscoveryPlugins()
{
    // The auto-discovery of discovery plugins can be skipped. This is mostly
    // for testing purposes.
    if (TfGetEnvSetting(PXR_SDR_SKIP_DISCOVERY_PLUGIN_DISCOVERY)) {
        return;
    }

    // Find all of the available discovery plugins
    std::set<TfType> discoveryPluginTypes;
    PlugRegistry::GetInstance().GetAllDerivedTypes<SdrDiscoveryPlugin>(
        &discoveryPluginTypes);

    // Allow plugins to be disabled.
    const std::string disabledPluginsStr =
        TfGetEnvSetting(PXR_SDR_DISABLE_PLUGINS);
    std::set<std::string> disabledPlugins =
        TfStringTokenizeToSet(disabledPluginsStr, ",");

    // Instantiate any discovery plugins that were found
    for (const TfType& discoveryPluginType : discoveryPluginTypes) {
        const std::string& pluginName = discoveryPluginType.GetTypeName();
        if (disabledPlugins.find(pluginName) != disabledPlugins.end()) {
            TF_DEBUG(SDR_DISCOVERY).Msg(
                "[PXR_SDR_DISABLE_PLUGINS] Disabled SdrDiscoveryPlugin '%s'\n",
                pluginName.c_str());
            continue;
        }

        TF_DEBUG(SDR_DISCOVERY).Msg(
            "Found SdrDiscoveryPlugin '%s'\n", 
            discoveryPluginType.GetTypeName().c_str());

        SdrDiscoveryPluginFactoryBase* pluginFactory =
            discoveryPluginType.GetFactory<SdrDiscoveryPluginFactoryBase>();

        if (TF_VERIFY(pluginFactory)) {
            _discoveryPlugins.emplace_back(pluginFactory->New());
        }
    }
}

void
SdrRegistry::_FindAndInstantiateParserPlugins()
{
    // The auto-discovery of parser plugins can be skipped. This is mostly
    // for testing purposes.
    if (TfGetEnvSetting(PXR_SDR_SKIP_PARSER_PLUGIN_DISCOVERY)) {
        return;
    }

    // Find all of the available parser plugins
    std::set<TfType> parserPluginTypes;
    PlugRegistry::GetInstance().GetAllDerivedTypes<SdrParserPlugin>(
        &parserPluginTypes);

    _InstantiateParserPlugins(parserPluginTypes);
}

void
SdrRegistry::_InstantiateParserPlugins(
    const std::set<TfType>& parserPluginTypes)
{
    // Allow plugins to be disabled.
    const std::string disabledPluginsStr = TfGetEnvSetting(PXR_SDR_DISABLE_PLUGINS);
    const std::set<std::string> disabledPlugins = TfStringTokenizeToSet(disabledPluginsStr, ",");

    // Ensure this list is in a consistent order to ensure stable behavior.
    // TfType's operator< is not stable across runs, so we sort based on
    // typename instead.
    std::vector<TfType> orderedPluginTypes {parserPluginTypes.begin(), parserPluginTypes.end()};
    std::sort(orderedPluginTypes.begin(), orderedPluginTypes.end(),
        [](const TfType& a, const TfType& b) {
            return a.GetTypeName() < b.GetTypeName();
        });

    // Instantiate any parser plugins that were found
    for (const TfType& parserPluginType : orderedPluginTypes) {
        const std::string& pluginName = parserPluginType.GetTypeName();
        if (disabledPlugins.find(pluginName) != disabledPlugins.end()) {
            TF_DEBUG(SDR_DISCOVERY).Msg(
                "[PXR_SDR_DISABLE_PLUGINS] Disabled SdrParserPlugin '%s'\n",
                pluginName.c_str());
            continue;
        }
        TF_DEBUG(SDR_DISCOVERY).Msg(
            "Found SdrParserPlugin '%s' for discovery types:\n", 
            parserPluginType.GetTypeName().c_str());

        SdrParserPluginFactoryBase* pluginFactory =
            parserPluginType.GetFactory<SdrParserPluginFactoryBase>();

        if (!TF_VERIFY(pluginFactory)) {
            continue;
        }

        SdrParserPlugin* parserPlugin = pluginFactory->New();
        _parserPlugins.emplace_back(parserPlugin);

        for (const TfToken& discoveryType : parserPlugin->GetDiscoveryTypes()) {
            TF_DEBUG(SDR_DISCOVERY).Msg("  - %s\n", discoveryType.GetText());

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
SdrRegistry::_RunDiscoveryPlugins(const DiscoveryPluginRefPtrVec& discoveryPlugins)
{
    size_t num_plugins = discoveryPlugins.size();
    std::vector<SdrShaderNodeDiscoveryResultVec> results_vec(num_plugins);

    // Discover nodes in parallel. Following the pattern in GetNodesByFamily,
    // pre-emptively release the Python GIL here to avoid
    // deadlocks since the code running in the worker threads may call into
    // Python and try to take the GIL when discovering nodes. We also need
    // to use scoped parallelism to ensure we don't pick up other tasks
    // during the call to WorkParallelForN that may reenter this function
    // and also deadlock.
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    WorkWithScopedParallelism([&]() {
        WorkParallelForN(
            num_plugins,
            [&](size_t start, size_t end) {
                for (size_t i = start; i < end; i++) {
                    results_vec[i] = discoveryPlugins[i]->DiscoverShaderNodes(
                        _DiscoveryContext(*this));
                }
            });
        }
    );

    std::lock_guard<std::mutex> drLock(_discoveryResultMutex);
    for (SdrShaderNodeDiscoveryResultVec &results : results_vec) {
        for (SdrShaderNodeDiscoveryResult &dr : results) {
            _AddDiscoveryResultNoLock(std::move(dr));
        }
    }
}

void 
SdrRegistry::_AddDiscoveryResultNoLock(SdrShaderNodeDiscoveryResult&& drMoved)
{
    // The "by identifier" map holds discovery result itself
    const auto it = _discoveryResultsByIdentifier.emplace(
        drMoved.identifier, std::move(drMoved));

    const SdrShaderNodeDiscoveryResult &dr = it->second;
    // The "by name" map holds a pointer to each discovery result in the 
    // "by identifier" map.
    _discoveryResultPtrsByName.emplace(dr.name, &dr);
    // All possible source types are determined by all available discoveries.
    _allSourceTypes.insert(dr.sourceType);

}

SdrShaderNodeConstPtr 
SdrRegistry::_ParseNodeFromAssetOrSourceCode(
    SdrParserPlugin &parser, SdrShaderNodeDiscoveryResult &&dr)
{
    SdrShaderNodeUniquePtr newNode = parser.ParseShaderNode(dr);

    if (!_ValidateNode(newNode, dr)) {
        return nullptr;
    }

    // Create the node map key before we move the discovery result.
    _ShaderNodeMapKey key{dr.identifier, dr.sourceType};

    // Move the discovery result into _discoveryResults so the node can be found
    // in the Get*() methods. Note that we keep this locked while caching the
    // node itself so that in the extraordinarily unlikely case that another
    // thread tries to add a node with the same identifier and sourceType 
    // through this code path, that THIS node is the one that ends up cached.
    std::lock_guard<std::mutex> drLock(_discoveryResultMutex);
    _AddDiscoveryResultNoLock(std::move(dr));

    return _InsertNodeInCache(std::move(key), std::move(newNode));
}

SdrShaderNodeConstPtr 
SdrRegistry::_GetNodeInIdentifierRangeWithSourceType(
    _DiscoveryResultsByIdentifierRange range, const TfToken& sourceType)
{
    // Return the first node that we can successfully find or parse with the
    // given source type. We expect there to be at most a few (and frequently 
    // just one) source types for a particular identifier so there should be 
    // little impact from this linear search.
    for (auto it = range.first; it != range.second; ++ it) {
        const SdrShaderNodeDiscoveryResult &dr = it->second;
        if (dr.sourceType != sourceType) {
            continue;
        }
        if (SdrShaderNodeConstPtr node = _FindOrParseNodeInCache(dr)) {
            return node;
        }
    }
    return nullptr;
}

SdrShaderNodeConstPtr 
SdrRegistry::_GetNodeInNameRangeWithSourceType(
    _DiscoveryResultPtrsByNameRange range, const TfToken& sourceType,
    SdrVersionFilter filter)
{
    for (auto it = range.first; it != range.second; ++ it) {
        const SdrShaderNodeDiscoveryResult &dr = *(it->second);
        if (dr.sourceType != sourceType) {
            continue;
        }
        if (!_MatchesFamilyAndFilter(dr, TfToken(), filter)) {
            continue;
        }
        if (SdrShaderNodeConstPtr node = _FindOrParseNodeInCache(dr)) {
            return node;
        }
    }
    return nullptr;
}

SdrShaderNodeConstPtr 
SdrRegistry::_FindNodeInCache(const _ShaderNodeMapKey &key) const
{
    // Return an existing node in the node map if there's one that matches the
    // node unique key (identifier AND source type).
    std::lock_guard<std::mutex> nmLock(_nodeMapMutex);
    auto it = _nodeMap.find(key);
    if (it != _nodeMap.end()) {
        // Get the raw ptr from the unique_ptr
        return it->second.get();
    }
    return nullptr;
}

SdrShaderNodeConstPtr 
SdrRegistry::_InsertNodeInCache(
    _ShaderNodeMapKey &&key, SdrShaderNodeUniquePtr &&node)
{
    std::lock_guard<std::mutex> nmLock(_nodeMapMutex);
    const auto result = _nodeMap.emplace(std::move(key), std::move(node));

    // Get the unique_ptr from the iterator, then get its raw ptr
    return result.first->second.get();
}

SdrShaderNodeConstPtr
SdrRegistry::_FindOrParseNodeInCache(const SdrShaderNodeDiscoveryResult& dr)
{
    // Return an existing node in the map if it already exists.
    _ShaderNodeMapKey key{dr.identifier, dr.sourceType};
    if (SdrShaderNodeConstPtr node = _FindNodeInCache(key)) {
        return node;
    }

    // Ensure there is a parser plugin that can handle this node
    auto i = _parserPluginMap.find(dr.discoveryType);
    if (i == _parserPluginMap.end()) {
        TF_DEBUG(SDR_PARSING).Msg("Encountered a node of type [%s], "
                                  "with name [%s], but a parser for that type "
                                  "could not be found; ignoring.\n", 
                                  dr.discoveryType.GetText(),  dr.name.c_str());
        return nullptr;
    }

    // Parse and validate the node. _ValidateNode handles posting warnings and
    // runtime errors itself.
    SdrShaderNodeUniquePtr newNode = i->second->ParseShaderNode(dr);
    if (!_ValidateNode(newNode, dr)) {
        return nullptr;
    }
    
    // Cache the node and return the cached node.
    return _InsertNodeInCache(std::move(key), std::move(newNode));
}

SdrParserPlugin*
SdrRegistry::_GetParserForDiscoveryType(const TfToken& discoveryType) const
{
    auto i = _parserPluginMap.find(discoveryType);
    return i == _parserPluginMap.end() ? nullptr : i->second;
}

PXR_NAMESPACE_CLOSE_SCOPE
