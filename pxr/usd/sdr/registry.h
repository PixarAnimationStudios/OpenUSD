//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_SDR_REGISTRY_H
#define PXR_USD_SDR_REGISTRY_H

/// \file sdr/registry.h
///
/// \note
/// All Ndr objects are deprecated in favor of the corresponding Sdr objects
/// in this file. All existing pxr/usd/ndr implementations will be moved to
/// pxr/usd/sdr.

#include "pxr/pxr.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/usd/ndr/registry.h"
#include "pxr/usd/sdr/api.h"
#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/sdr/discoveryPlugin.h"
#include "pxr/usd/sdr/parserPlugin.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderNodeDiscoveryResult.h"
#include "pxr/usd/sdf/assetPath.h"
#include <map>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

/// \class SdrRegistry
///
/// The registry provides access to shader node information.
/// "Discovery Plugins" are responsible for finding the nodes that should
/// be included in the registry.
///
/// Discovery plugins are found through the plugin system. If additional
/// discovery plugins need to be specified, a client can pass them to
/// `SetExtraDiscoveryPlugins()`.
///
/// When the registry is first told about the discovery plugins, the plugins
/// will be asked to discover nodes. These plugins will generate
/// `SdrShaderNodeDiscoveryResult` instances, which only contain basic
/// metadata. Once the client asks for information that would require the
/// node's contents to be parsed (eg, what its inputs and outputs are), the
/// registry will begin the parsing process on an as-needed basis. See
/// `SdrShaderNodeDiscoveryResult` for the information that can be retrieved
/// without triggering a parse.
///
/// Some methods in this library may allow for a "family" to be provided. A
/// family is simply a generic grouping which is optional.
///
class SdrRegistry : public NdrRegistry
{
public:
    using DiscoveryPluginRefPtrVec = SdrDiscoveryPluginRefPtrVector;

    /// Get the single `SdrRegistry` instance.
    SDR_API
    static SdrRegistry& GetInstance();

    /// Allows the client to explicitly set additional discovery results that
    /// would otherwise NOT be found through the plugin system. For example
    /// to support lazily-loaded plugins which cannot be easily discovered
    /// in advance.
    ///
    /// This method will not immediately spawn a parse call which will be
    /// deferred until a GetShaderNode*() method is called.
    SDR_API
    void AddDiscoveryResult(SdrShaderNodeDiscoveryResult&& discoveryResult);

    /// Copy version of the method above.
    /// For performance reasons, one should prefer to use the rvalue reference
    /// form.
    /// \overload
    SDR_API
    void AddDiscoveryResult(
        const SdrShaderNodeDiscoveryResult& discoveryResult);

    using NdrRegistry::AddDiscoveryResult;

    /// Get identifiers of all the shader nodes that the registry is aware of.
    ///
    /// This will not run the parsing plugins on the nodes that have been
    /// discovered, so this method is relatively quick. Optionally, a "family"
    /// name can be specified to only get the identifiers of nodes that belong
    /// to that family and a filter can be specified to get just the default
    /// version (the default) or all versions of the node.
    SDR_API
    SdrIdentifierVec
    GetShaderNodeIdentifiers(const TfToken& family = TfToken(),
                             SdrVersionFilter filter =
                             SdrVersionFilterDefaultOnly) const;

    /// Get the names of all the shader nodes that the registry is aware of.
    ///
    /// This will not run the parsing plugins on the nodes that have been
    /// discovered, so this method is relatively quick. Optionally, a "family"
    /// name can be specified to only get the names of nodes that belong to
    /// that family.
    SDR_API
    SdrStringVec GetShaderNodeNames(const TfToken& family = TfToken()) const;

    /// Get the shader node with the specified \p identifier, and an optional
    /// \p sourceTypePriority list specifying the set of node SOURCE types (see
    /// `SdrShaderNode::GetSourceType()`) that should be searched.
    ///
    /// If no sourceTypePriority is specified, the first encountered node with 
    /// the specified identifier will be returned (first is arbitrary) if found.
    /// 
    /// If a sourceTypePriority list is specified, then this will iterate 
    /// through each source type and try to find a node matching by identifier.
    /// This is equivalent to calling
    /// SdrRegistry::GetShaderNodeByIdentifierAndType for each source type
    /// until a node is found.
    /// 
    /// Nodes of the same identifier but different source type can exist
    /// in the registry. If a node 'Foo' with source types 'abc' and 'xyz'
    /// exist in the registry, and you want to make sure the 'abc' version
    /// is fetched before the 'xyz' version, the priority list would be
    /// specified as ['abc', 'xyz']. If the 'abc' version did not exist in
    /// the registry, then the 'xyz' version would be returned.
    ///
    /// Returns `nullptr` if a node matching the arguments can't be found.
    SDR_API
    SdrShaderNodeConstPtr GetShaderNodeByIdentifier(
        const SdrIdentifier& identifier,
        const SdrTokenVec& typePriority = SdrTokenVec());

    /// Get the shader node with the specified \p identifier and \p sourceType. 
    /// If there is no matching node for the sourceType, nullptr is returned.
    SDR_API
    SdrShaderNodeConstPtr GetShaderNodeByIdentifierAndType(
        const SdrIdentifier& identifier,
        const TfToken& nodeType);

    /// Get the shader node with the specified name.
    ///
    /// \deprecated
    /// Deprecated in favor of GetShaderNodeByName(..., SdrVersionFilter filter)
    SDR_API
    SdrShaderNodeConstPtr GetShaderNodeByName(
        const std::string& name,
        const NdrTokenVec& typePriority,
        NdrVersionFilter filter);

    /// Get the shader node with the specified name.  An optional priority list
    /// specifies the set of node SOURCE types
    /// (\sa SdrShaderNode::GetSourceType()) that should be searched and in what
    /// order.
    ///
    /// Optionally, a filter can be specified to consider just the default
    /// versions of nodes matching \p name (the default) or all versions
    /// of the nodes.
    ///
    /// \sa GetShaderNodeByIdentifier().
    SDR_API
    SdrShaderNodeConstPtr GetShaderNodeByName(
        const std::string& name,
        const SdrTokenVec& typePriority = SdrTokenVec(),
        SdrVersionFilter filter = SdrVersionFilterDefaultOnly);

    /// A convenience wrapper around \c GetShaderNodeByName().
    ///
    /// \deprecated
    /// Deprecated in favor of GetShaderNodeByNameAndType(..., SdrVersionFilter filter)
    SDR_API
    SdrShaderNodeConstPtr GetShaderNodeByNameAndType(
        const std::string& name,
        const TfToken& nodeType,
        NdrVersionFilter filter);

    /// A convenience wrapper around \c GetShaderNodeByName(). Instead of
    /// providing a priority list, an exact type is specified, and
    /// `nullptr` is returned if a node with the exact identifier and
    /// type does not exist.
    ///
    /// Optionally, a filter can be specified to consider just the default
    /// versions of nodes matching \p name (the default) or all versions
    /// of the nodes.
    SDR_API
    SdrShaderNodeConstPtr GetShaderNodeByNameAndType(
        const std::string& name,
        const TfToken& nodeType,
        SdrVersionFilter filter = SdrVersionFilterDefaultOnly);

    /// Parses the given \p asset, constructs a SdrShaderNode from it and adds it to
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
    SDR_API
    SdrShaderNodeConstPtr GetShaderNodeFromAsset(
        const SdfAssetPath &shaderAsset,
        const SdrTokenMap &metadata=SdrTokenMap(),
        const TfToken &subIdentifier=TfToken(),
        const TfToken &sourceType=TfToken());

    /// Parses the given \p sourceCode string, constructs a SdrShaderNode from
    /// it and adds it to the registry. The parser to be used is determined
    /// by the specified \p sourceType.
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
    SDR_API
    SdrShaderNodeConstPtr GetShaderNodeFromSourceCode(
        const std::string &sourceCode,
        const TfToken &sourceType,
        const SdrTokenMap &metadata=SdrTokenMap());

    /// Get all shader nodes matching the given identifier (multiple nodes of
    /// the same identifier, but different source types, may exist). If no
    /// nodes match the identifier, an empty vector is returned.
    SDR_API
    SdrShaderNodePtrVec GetShaderNodesByIdentifier(const SdrIdentifier& identifier);

    /// Get all shader nodes matching the given name.
    ///
    /// \deprecated
    /// Deprecated in favor of GetShaderNodesByName(..., SdrVersionFilter filter)
    SDR_API
    SdrShaderNodePtrVec GetShaderNodesByName(
        const std::string& name,
        NdrVersionFilter filter);

    /// Get all shader nodes matching the given name. Only nodes matching the
    /// specified name will be parsed. Optionally, a filter can be specified
    /// to get just the default version (the default) or all versions of the
    /// node.  If no nodes match an empty vector is returned.
    SDR_API
    SdrShaderNodePtrVec GetShaderNodesByName(
        const std::string& name,
        SdrVersionFilter filter = SdrVersionFilterDefaultOnly);

    /// Get all shader nodes, optionally restricted to the nodes
    /// that fall under a specified family and/or the default version.
    ///
    /// \deprecated
    /// Deprecated in favor of GetShaderNodesByFamily(..., SdrVersionFilter filter)
    SDR_API
    SdrShaderNodePtrVec GetShaderNodesByFamily(
        const TfToken& family,
        NdrVersionFilter filter);

    /// Get all shader nodes, optionally restricted to the nodes
    /// that fall under a specified family and/or the default version.
    ///
    /// Note that this will parse \em all nodes that the registry is aware of
    /// (unless a family is specified), so this may take some time to run
    /// the first time it is called.
    SDR_API
    SdrShaderNodePtrVec GetShaderNodesByFamily(
        const TfToken& family = TfToken(),
        SdrVersionFilter filter = SdrVersionFilterDefaultOnly);

    /// Get a sorted list of all shader node source types that may be present
    /// on the nodes in the registry.
    ///
    /// Source types originate from the discovery process, but there is no
    /// guarantee that the discovered source types will also have a registered
    /// parser plugin.  The actual supported source types here depend on the
    /// parsers that are available.  Also note that some parser plugins may not
    /// advertise a source type.
    ///
    /// See the documentation for `SdrParserPlugin` and
    /// `SdrShaderNode::GetSourceType()` for more information.
    SDR_API
    SdrTokenVec GetAllShaderNodeSourceTypes() const;

protected:
    // Allow TF to construct the class
    friend class TfSingleton<SdrRegistry>;

    SDR_API
    SdrRegistry();

    SDR_API
    ~SdrRegistry();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDR_REGISTRY_H
