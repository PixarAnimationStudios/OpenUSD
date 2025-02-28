//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_NDR_NODE_DISCOVERY_RESULT_H
#define PXR_USD_NDR_NODE_DISCOVERY_RESULT_H

#include "pxr/usd/ndr/declare.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Represents the raw data of a node, and some other bits of metadata, that
/// were determined via a `NdrDiscoveryPlugin`.
///
/// \deprecated
/// Deprecated in favor of SdrShaderNodeDiscoveryResult
struct NdrNodeDiscoveryResult {
    /// Constructor.
    NdrNodeDiscoveryResult(
        const NdrIdentifier& identifier,
        const NdrVersion& version,
        const std::string& name,
        const TfToken& family,
        const TfToken& discoveryType,
        const TfToken& sourceType,
        const std::string& uri,
        const std::string& resolvedUri,
        const std::string &sourceCode=std::string(),
        const NdrTokenMap &metadata=NdrTokenMap(),
        const std::string& blindData=std::string(),
        const TfToken& subIdentifier=TfToken()
    ) : identifier(identifier),
        version(version),
        name(name),
        family(family),
        discoveryType(discoveryType),
        sourceType(sourceType),
        uri(uri),
        resolvedUri(resolvedUri),
        sourceCode(sourceCode),
        metadata(metadata),
        blindData(blindData),
        subIdentifier(subIdentifier)
    { }

    /// The node's identifier.
    ///
    /// How the node is identified. In many cases this will be the
    /// name of the file or resource that this node originated from.
    /// E.g. "mix_float_2_1".  The identifier must be unique for a
    /// given sourceType.
    NdrIdentifier identifier;

    /// The node's version.  This may or may not be embedded in the
    /// identifier, it's up to implementations.  E.g a node with
    /// identifier "mix_float_2_1" might have version 2.1.
    NdrVersion version;

    /// The node's name.
    ///
    /// A version independent identifier for the node type.  This will
    /// often embed type parameterization but should not embed the
    /// version.  E.g a node with identifier "mix_float_2_1" might have
    /// name "mix_float".
    std::string name;

    /// The node's family.
    ///
    /// A node's family is an optional piece of metadata that specifies a
    /// generic grouping of nodes.  E.g a node with identifier
    /// "mix_float_2_1" might have family "mix".
    TfToken family;

    /// The node's discovery type.
    ///
    /// The type could be the file extension, or some other type of metadata
    /// that can signify a type prior to parsing. See the documentation for
    /// `NdrParserPlugin` and `NdrParserPlugin::DiscoveryTypes` for more
    /// information on how this value is used.
    TfToken discoveryType;

    /// The node's source type.
    ///
    /// This type is unique to the parsing plugin
    /// (`NdrParserPlugin::SourceType`), and determines the source of the node.
    /// See `NdrNode::GetSourceType()` for more information.
    TfToken sourceType;

    /// The node's origin.
    ///
    /// This may be a filesystem path, a URL pointing to a resource in the
    /// cloud, or some other type of resource identifier.
    std::string uri;

    /// The node's fully-resolved URI.
    ///
    /// For example, this might be an absolute path when the original URI was
    /// a relative path. In most cases, this is the path that `Ar`'s
    /// `Resolve()` returns. In any case, this path should be locally
    /// accessible.
    std::string resolvedUri;

    /// The node's entire source code.
    ///  
    /// The source code is parsed (if non-empty) by parser plugins when the 
    /// resolvedUri value is empty.
    std::string sourceCode;

    /// The node's metadata collected during the discovery process.
    /// 
    /// Additional metadata may be present in the node's source, in the asset
    /// pointed to by resolvedUri or in sourceCode (if resolvedUri is empty).
    /// In general, parsers should override this data with metadata from the 
    /// shader source. 
    NdrTokenMap metadata;

    /// An optional detail for the parser plugin.  The parser plugin
    /// defines the meaning of this data so the discovery plugin must
    /// be written to match.
    std::string blindData;

    /// The subIdentifier is associated with a particular asset and refers to a
    /// specific definition within the asset.  The asset is the one referred to
    /// by `NdrRegistry::GetNodeFromAsset()`.  The subIdentifier is not needed
    /// for all cases where the node definition is not associated with an asset.
    /// Even if the node definition is associated with an asset, the
    /// subIdentifier is only needed if the asset specifies multiple definitions
    /// rather than a single definition.
    TfToken subIdentifier;
};

typedef std::vector<NdrNodeDiscoveryResult> NdrNodeDiscoveryResultVec;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_NDR_NODE_DISCOVERY_RESULT_H
