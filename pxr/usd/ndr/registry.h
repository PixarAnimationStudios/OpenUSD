//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_NDR_REGISTRY_H
#define PXR_USD_NDR_REGISTRY_H

/// \file ndr/registry.h
///
/// \deprecated
/// All Ndr objects are deprecated in favor of the corresponding Sdr objects
/// in sdr/registry.h

#include "pxr/pxr.h"
#include "pxr/usd/ndr/api.h"
#include "pxr/base/tf/weakBase.h"
#include "pxr/usd/ndr/declare.h"
#include "pxr/usd/ndr/discoveryPlugin.h"
#include "pxr/usd/ndr/node.h"
#include "pxr/usd/ndr/nodeDiscoveryResult.h"
#include "pxr/usd/ndr/parserPlugin.h"
#include "pxr/usd/sdf/assetPath.h"
#include <map>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

/// \class NdrRegistry
///
/// The registry provides access to node information. "Discovery Plugins" are
/// responsible for finding the nodes that should be included in the registry.
///
/// Discovery plugins are found through the plugin system. If additional
/// discovery plugins need to be specified, a client can pass them to
/// `SetExtraDiscoveryPlugins()`.
///
/// When the registry is first told about the discovery plugins, the plugins
/// will be asked to discover nodes. These plugins will generate
/// `NdrNodeDiscoveryResult` instances, which only contain basic metadata. Once
/// the client asks for information that would require the node's contents to
/// be parsed (eg, what its inputs and outputs are), the registry will begin the
/// parsing process on an as-needed basis. See `NdrNodeDiscoveryResult` for the
/// information that can be retrieved without triggering a parse.
///
/// Some methods in this library may allow for a "family" to be provided. A
/// family is simply a generic grouping which is optional.
///
/// \deprecated
/// Deprecated in favor of SdrRegistry
class NdrRegistry : public TfWeakBase
{
public:
    using DiscoveryPluginRefPtrVec = NdrDiscoveryPluginRefPtrVector;

    /// Allows the client to set any additional discovery plugins that would
    /// otherwise NOT be found through the plugin system. Runs the discovery
    /// process for the specified plugins immediately.
    ///
    /// Note that this method cannot be called after any nodes in the registry
    /// have been parsed (eg, through GetNode*()), otherwise an error will
    /// result.
    NDR_API
    void SetExtraDiscoveryPlugins(DiscoveryPluginRefPtrVec plugins);

    /// Allows the client to set any additional discovery plugins that would
    /// otherwise NOT be found through the plugin system. Runs the discovery
    /// process for the specified plugins immediately.
    ///
    /// Note that this method cannot be called after any nodes in the registry
    /// have been parsed (eg, through GetNode*()), otherwise an error will
    /// result.
    NDR_API
    void SetExtraDiscoveryPlugins(const std::vector<TfType>& pluginTypes);

    /// Allows the client to explicitly set additional discovery results that
    /// would otherwise NOT be found through the plugin system. For example
    /// to support lazily-loaded plugins which cannot be easily discovered
    /// in advance.
    ///
    /// This method will not immediately spawn a parse call which will be
    /// deferred until a GetNode*() method is called.
    ///
    /// \deprecated
    /// Deprecated in favor of
    /// SdrRegistry::AddDiscoveryResult(SdrShaderNodeDiscoveryResult&&)
    NDR_API
    void AddDiscoveryResult(NdrNodeDiscoveryResult&& discoveryResult);

    /// Copy version of the method above.
    /// For performance reasons, one should prefer to use the rvalue reference
    /// form.
    /// \overload
    ///
    /// \deprecated
    /// Deprecated in favor of
    /// SdrRegistry::AddDiscoveryResult(const SdrShaderNodeDiscoveryResult&)
    NDR_API
    void AddDiscoveryResult(const NdrNodeDiscoveryResult& discoveryResult);

    /// Allows the client to set any additional parser plugins that would
    /// otherwise NOT be found through the plugin system.
    ///
    /// Note that this method cannot be called after any nodes in the registry
    /// have been parsed (eg, through GetNode*()), otherwise an error will
    /// result.
    NDR_API
    void SetExtraParserPlugins(const std::vector<TfType>& pluginTypes);

    /// Parses the given \p asset, constructs a NdrNode from it and adds it to
    /// the registry.
    /// 
    /// Nodes created from an asset using this API can be looked up by the 
    /// unique identifier and sourceType of the returned node, or by URI, 
    /// which will be set to the unresolved asset path value.
    /// 
    /// \p metadata contains additional metadata needed for parsing and 
    /// compiling the source code in the file pointed to by \p asset correctly.
    /// This metadata supplements the metadata available in the asset and 
    /// overrides it in cases where there are key collisions.
    ///
    /// \p subidentifier is optional, and it would be used to indicate a
    /// particular definition in the asset file if the asset contains multiple
    /// node definitions.
    ///
    /// \p sourceType is optional, and it is only needed to indicate a
    /// particular type if the asset file is capable of representing a node
    /// definition of multiple source types.
    ///
    /// Returns a valid node if the asset is parsed successfully using one 
    /// of the registered parser plugins.
    ///
    /// \deprecated
    /// Deprecated in favor of SdrRegistry::GetShaderNodeFromAsset
    NDR_API
    NdrNodeConstPtr GetNodeFromAsset(const SdfAssetPath &asset,
                                     const NdrTokenMap &metadata,
                                     const TfToken &subIdentifier=TfToken(),
                                     const TfToken &sourceType=TfToken());

    /// Parses the given \p sourceCode string, constructs a NdrNode from it and 
    /// adds it to the registry. The parser to be used is determined by the 
    /// specified \p sourceType.
    /// 
    /// Nodes created from source code using this API can be looked up by the 
    /// unique identifier and sourceType of the returned node.
    /// 
    /// \p metadata contains additional metadata needed for parsing and 
    /// compiling the source code correctly. This metadata supplements the 
    /// metadata available in \p sourceCode and overrides it cases where there 
    /// are key collisions.
    /// 
    /// Returns a valid node if the given source code is parsed successfully 
    /// using the parser plugins that is registered for the specified 
    /// \p sourceType.
    ///
    /// \deprecated
    /// Deprecated in favor of SdrRegistry::GetShaderNodeFromSourceCode
    NDR_API
    NdrNodeConstPtr GetNodeFromSourceCode(const std::string &sourceCode,
                                          const TfToken &sourceType,
                                          const NdrTokenMap &metadata);

    /// Get the locations where the registry is searching for nodes.
    ///
    /// Depending on which discovery plugins were used, this may include
    /// non-filesystem paths.
    NDR_API
    NdrStringVec GetSearchURIs() const;

    /// Get the identifiers of all the nodes that the registry is aware of.
    ///
    /// This will not run the parsing plugins on the nodes that have been
    /// discovered, so this method is relatively quick. Optionally, a "family"
    /// name can be specified to only get the identifiers of nodes that belong
    /// to that family and a filter can be specified to get just the default
    /// version (the default) or all versions of the node.
    ///
    /// \deprecated
    /// Deprecated in favor of SdrRegistry::GetShaderNodeIdentifiers
    NDR_API
    NdrIdentifierVec
    GetNodeIdentifiers(const TfToken& family = TfToken(),
                       NdrVersionFilter filter =
                           NdrVersionFilterDefaultOnly) const;

    /// Get the names of all the nodes that the registry is aware of.
    ///
    /// This will not run the parsing plugins on the nodes that have been
    /// discovered, so this method is relatively quick. Optionally, a "family"
    /// name can be specified to only get the names of nodes that belong to
    /// that family.
    ///
    /// \deprecated
    /// Deprecated in favor of SdrRegistry::GetShaderNodeNames
    NDR_API
    NdrStringVec GetNodeNames(const TfToken& family = TfToken()) const;

    /// Get the node with the specified \p identifier, and an optional
    /// \p sourceTypePriority list specifying the set of node SOURCE types (see
    /// `NdrNode::GetSourceType()`) that should be searched.
    ///
    /// If no sourceTypePriority is specified, the first encountered node with 
    /// the specified identifier will be returned (first is arbitrary) if found.
    /// 
    /// If a sourceTypePriority list is specified, then this will iterate 
    /// through each source type and try to find a node matching by identifier.
    /// This is equivalent to calling NdrRegistry::GetNodeByIdentifierAndType 
    /// for each source type until a node is found.
    /// 
    /// Nodes of the same identifier but different source type can exist
    /// in the registry. If a node 'Foo' with source types 'abc' and 'xyz'
    /// exist in the registry, and you want to make sure the 'abc' version
    /// is fetched before the 'xyz' version, the priority list would be
    /// specified as ['abc', 'xyz']. If the 'abc' version did not exist in
    /// the registry, then the 'xyz' version would be returned.
    ///
    /// Returns `nullptr` if a node matching the arguments can't be found.
    ///
    /// \deprecated
    /// Deprecated in favor of SdrRegistry::GetShaderNodeByIdentifier
    NDR_API
    NdrNodeConstPtr GetNodeByIdentifier(const NdrIdentifier& identifier,
                        const NdrTokenVec& sourceTypePriority = NdrTokenVec());

    /// Get the node with the specified \p identifier and \p sourceType. 
    /// If there is no matching node for the sourceType, nullptr is returned.
    ///
    /// \deprecated
    /// Deprecated in favor of SdrRegistry::GetShaderNodeByIdentifierAndType
    NDR_API
    NdrNodeConstPtr GetNodeByIdentifierAndType(const NdrIdentifier& identifier,
                                               const TfToken& sourceType);

    /// Get the node with the specified name.  An optional priority list
    /// specifies the set of node SOURCE types (\sa NdrNode::GetSourceType())
    /// that should be searched and in what order.
    ///
    /// Optionally, a filter can be specified to consider just the default
    /// versions of nodes matching \p name (the default) or all versions
    /// of the nodes.
    ///
    /// \sa GetNodeByIdentifier().
    ///
    /// \deprecated
    /// Deprecated in favor of SdrRegistry::GetShaderNodeByName
    NDR_API
    NdrNodeConstPtr GetNodeByName(const std::string& name,
                        const NdrTokenVec& sourceTypePriority = NdrTokenVec(),
                        NdrVersionFilter filter = NdrVersionFilterDefaultOnly);

    /// A convenience wrapper around \c GetNodeByName(). Instead of
    /// providing a priority list, an exact type is specified, and
    /// `nullptr` is returned if a node with the exact identifier and
    /// type does not exist.
    ///
    /// Optionally, a filter can be specified to consider just the default
    /// versions of nodes matching \p name (the default) or all versions
    /// of the nodes.
    ///
    /// \deprecated
    /// Deprecated in favor of SdrRegistry::GetShaderNodeByNameAndType
    NDR_API
    NdrNodeConstPtr GetNodeByNameAndType(const std::string& name,
                                         const TfToken& sourceType,
                                         NdrVersionFilter filter =
                                             NdrVersionFilterDefaultOnly);

    /// Get all nodes matching the specified identifier (multiple nodes of
    /// the same identifier, but different source types, may exist). If no nodes
    /// match the identifier, an empty vector is returned.
    ///
    /// \deprecated
    /// Deprecated in favor of SdrRegistry::GetShaderNodesByIdentifier
    NDR_API
    NdrNodeConstPtrVec GetNodesByIdentifier(const NdrIdentifier& identifier);

    /// Get all nodes matching the specified name. Only nodes matching the
    /// specified name will be parsed. Optionally, a filter can be specified
    /// to get just the default version (the default) or all versions of the
    /// node.  If no nodes match an empty vector is returned.
    ///
    /// \deprecated
    /// Deprecated in favor of SdrRegistry::GetShaderNodesByName
    NDR_API
    NdrNodeConstPtrVec GetNodesByName(const std::string& name,
                                      NdrVersionFilter filter =
                                          NdrVersionFilterDefaultOnly);

    /// Get all nodes from the registry, optionally restricted to the nodes
    /// that fall under a specified family and/or the default version.
    ///
    /// Note that this will parse \em all nodes that the registry is aware of
    /// (unless a family is specified), so this may take some time to run
    /// the first time it is called.
    ///
    /// \deprecated
    /// Deprecated in favor of SdrRegistry::GetShaderNodesByFamily
    NDR_API
    NdrNodeConstPtrVec GetNodesByFamily(const TfToken& family = TfToken(),
                                        NdrVersionFilter filter =
                                            NdrVersionFilterDefaultOnly);

    /// Get a sorted list of all node source types that may be present on the
    /// nodes in the registry.
    ///
    /// Source types originate from the discovery process, but there is no
    /// guarantee that the discovered source types will also have a registered
    /// parser plugin.  The actual supported source types here depend on the
    /// parsers that are available.  Also note that some parser plugins may not
    /// advertise a source type.
    ///
    /// See the documentation for `NdrParserPlugin` and
    /// `NdrNode::GetSourceType()` for more information.
    ///
    /// \deprecated
    /// Deprecated in favor of SdrRegistry::GetAllShaderNodeSourceTypes
    NDR_API
    NdrTokenVec GetAllNodeSourceTypes() const;

protected:
    NdrRegistry(const NdrRegistry&) = delete;
    NdrRegistry& operator=(const NdrRegistry&) = delete;

    NDR_API
    NdrRegistry();

    NDR_API
    ~NdrRegistry();

private:
    class _DiscoveryContext;
    friend class _DiscoveryContext;

    using _TypeToParserPluginMap = 
        std::unordered_map<TfToken, NdrParserPlugin*, TfToken::HashFunctor>;

    // Node cache data structure, stored NdrNodes keyed by identifier and source
    // type.
    using _NodeMapKey = std::pair<NdrIdentifier, TfToken>;
    using _NodeMap = std::unordered_map<_NodeMapKey, NdrNodeUniquePtr, TfHash>;

    // Discovery results data structure, NdrNodeDiscoveryResults multimap keyed
    // by identifier
    using _DiscoveryResultsByIdentifier = std::unordered_multimap<
        TfToken, NdrNodeDiscoveryResult, TfHash>;
    using _DiscoveryResultsByIdentifierRange = 
        std::pair<_DiscoveryResultsByIdentifier::const_iterator,
                  _DiscoveryResultsByIdentifier::const_iterator>;

    // Discovery results data structure: a multimap of raw pointers to 
    // NdrNodeDiscoveryResults (i.e. pointers to the discovery results stored 
    // in a _DiscoveryResultsByIdentifier) keyed by name.
    using _DiscoveryResultPtrsByName = std::unordered_multimap<
        std::string, const NdrNodeDiscoveryResult *, TfHash>;
    using _DiscoveryResultPtrsByNameRange = 
        std::pair<_DiscoveryResultPtrsByName::const_iterator,
                  _DiscoveryResultPtrsByName::const_iterator>;

    // The discovery result data structures are not concurrent and must be kept
    // in sync, thus they need some locking infrastructure.
    mutable std::mutex _discoveryResultMutex;

    // The node map is not a concurrent data structure, thus it needs some
    // locking infrastructure.
    mutable std::mutex _nodeMapMutex;

    // Runs each discovery plugin provided and adds the results to the
    // internal discovery result maps
    void _RunDiscoveryPlugins(const DiscoveryPluginRefPtrVec& discoveryPlugins);

    // Takes the discovery and puts in the maps that hold the discovery results,
    // keeping them in sync.
    void _AddDiscoveryResultNoLock(NdrNodeDiscoveryResult&& dr);

    // Finds and instantiates the discovery plugins
    void _FindAndInstantiateDiscoveryPlugins();

    // Finds and instantiates the parser plugins
    void _FindAndInstantiateParserPlugins();

    // Instantiates the specified parser plugins and adds them to
    // the registry.
    void _InstantiateParserPlugins(const std::set<TfType>& parserPluginTypes);

    // Parses the node for the discovery result if adding it to the node map if
    // able and adds the discovery result to the discovery result maps. Intended
    // for the GetNodeFromAsset and GetNodeFromSourceCode APIs which can add
    // nodes that don't already appear in the discovery results.
    NdrNodeConstPtr _ParseNodeFromAssetOrSourceCode(
        NdrParserPlugin &parser, NdrNodeDiscoveryResult &&dr);

    // Implementation helper for getting the first node of the given sourceType 
    // in the range of node discovery results for a paricular identifier.
    NdrNodeConstPtr _GetNodeInIdentifierRangeWithSourceType(
        _DiscoveryResultsByIdentifierRange range, const TfToken& sourceType);

    // Implementation helper for getting the first node of the given sourceType 
    // and matching the given version filter in the range of node discovery 
    // results for a paricular name.
    NdrNodeConstPtr _GetNodeInNameRangeWithSourceType(
        _DiscoveryResultPtrsByNameRange range, const TfToken& sourceType,
        NdrVersionFilter filter);

    // Thread-safe find of a node in the cache by key.
    NdrNodeConstPtr _FindNodeInCache(const _NodeMapKey &key) const;

    // Thread-safe insertion of a node into the cache with a given key. If a 
    // node with the same key already exists in the cache, the pointer to the
    // existing node will be returned, otherwise the pointer to pointer to the
    // inserted node is returned.
    NdrNodeConstPtr _InsertNodeInCache(
        _NodeMapKey &&key, NdrNodeUniquePtr &&node);

    // Finds an existing node in the node cache for the discovery result if one
    // exists. Otherwise it parses the new node, inserts it into the cache, and
    // returns it. If there was an error parsing or validating the node, 
    // `nullptr` will be returned.
    NdrNodeConstPtr _FindOrParseNodeInCache(const NdrNodeDiscoveryResult& dr);

    // Return the parser plugin for a discovery type. Returns null if no parser 
    // plugin has that discovery type.
    NdrParserPlugin*
    _GetParserForDiscoveryType(const TfToken& discoveryType) const;

    // The discovery plugins that were found through libplug and/or provided by
    // the client
    DiscoveryPluginRefPtrVec _discoveryPlugins;

    // The parser plugins that have been discovered via the plugin system. Maps
    // a discovery result's "discovery type" to a specific parser.
    _TypeToParserPluginMap _parserPluginMap;

    // The parser plugins.  This has ownership of the plugin objects.
    std::vector<std::unique_ptr<NdrParserPlugin>> _parserPlugins;

    // The preliminary discovery results prior to parsing. These are stored 
    // in a multimap by identifier and a multimap by name. If accessing or
    // mutating, _discoveryResultMutex should be used.
    _DiscoveryResultsByIdentifier _discoveryResultsByIdentifier;
    _DiscoveryResultPtrsByName _discoveryResultPtrsByName;

    // Set of all possible source types as determined by the existing discovery
    // results. Populated along with the discovery result multimaps. If 
    // accessing or mutating, _discoveryResultMutex should be used.
    TfToken::Set _allSourceTypes;

    // Maps a node's identifier and source type to a node instance. If accessing
    // or mutating, _nodeMapMutex should be used.
    _NodeMap _nodeMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_NDR_REGISTRY_H
