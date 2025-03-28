//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_SDR_NODE_DISCOVERY_RESULT_H
#define PXR_USD_SDR_NODE_DISCOVERY_RESULT_H

/// \file sdr/shaderNodeDiscoveryResult.h
///
/// \note
/// All Ndr objects are deprecated in favor of the corresponding Sdr objects
/// in this file. All existing pxr/usd/ndr implementations will be moved to
/// pxr/usd/sdr.

#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/ndr/nodeDiscoveryResult.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Represents the raw data of a node, and some other bits of metadata, that
/// were determined via a `SdrDiscoveryPlugin`.
struct SdrShaderNodeDiscoveryResult {
    /// Constructor.
    SdrShaderNodeDiscoveryResult(
        const SdrIdentifier& identifier,
        const SdrVersion& version,
        const std::string& name,
        const TfToken& family,
        const TfToken& discoveryType,
        const TfToken& sourceType,
        const std::string& uri,
        const std::string& resolvedUri,
        const std::string &sourceCode=std::string(),
        const SdrTokenMap &metadata=SdrTokenMap(),
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
    SdrIdentifier identifier;

    /// The node's version.  This may or may not be embedded in the
    /// identifier, it's up to implementations.  E.g a node with
    /// identifier "mix_float_2_1" might have version 2.1.
    SdrVersion version;

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
    /// `SdrParserPlugin` and `SdrParserPlugin::DiscoveryTypes` for more
    /// information on how this value is used.
    TfToken discoveryType;

    /// The node's source type.
    ///
    /// This type is unique to the parsing plugin
    /// (`SdrParserPlugin::SourceType`), and determines the source of the node.
    /// See `SdrShaderNode::GetSourceType()` for more information.
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
    SdrTokenMap metadata;

    /// An optional detail for the parser plugin.  The parser plugin
    /// defines the meaning of this data so the discovery plugin must
    /// be written to match.
    std::string blindData;

    /// The subIdentifier is associated with a particular asset and refers to a
    /// specific definition within the asset.  The asset is the one referred to
    /// by `SdrRegistry::GetNodeFromAsset()`.  The subIdentifier is not needed
    /// for all cases where the node definition is not associated with an asset.
    /// Even if the node definition is associated with an asset, the
    /// subIdentifier is only needed if the asset specifies multiple definitions
    /// rather than a single definition.
    TfToken subIdentifier;

    /// Helper function to translate SdrShaderNodeDiscoveryResult to
    /// NdrNodeDiscoveryResult values.
    ///
    /// \deprecated
    /// This function is deprecated and will be removed with the removal of
    /// the Ndr library.
    NdrNodeDiscoveryResult ToNdrNodeDiscoveryResult() const {
        return NdrNodeDiscoveryResult(
            identifier,
            SdrToNdrVersion(version),
            name,
            family,
            discoveryType,
            sourceType,
            uri,
            resolvedUri,
            sourceCode,
            metadata,
            blindData,
            subIdentifier
        );
    }

    /// Helper function to translate NdrNodeDiscoveryResult to
    /// SdrShaderNodeDiscoveryResult values.
    ///
    /// \deprecated
    /// This function is deprecated and will be removed with the removal of
    /// the Ndr library.
    static SdrShaderNodeDiscoveryResult FromNdrNodeDiscoveryResult(
        const NdrNodeDiscoveryResult& result
    ) {
        return SdrShaderNodeDiscoveryResult(
            result.identifier,
            NdrToSdrVersion(result.version),
            result.name,
            result.family,
            result.discoveryType,
            result.sourceType,
            result.uri,
            result.resolvedUri,
            result.sourceCode,
            result.metadata,
            result.blindData,
            result.subIdentifier
        );
    }
};

typedef std::vector<SdrShaderNodeDiscoveryResult> SdrShaderNodeDiscoveryResultVec;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDR_NODE_DISCOVERY_RESULT_H
