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

#ifndef PXR_USD_NDR_NODE_H
#define PXR_USD_NDR_NODE_H

/// \file ndr/node.h

#include "pxr/pxr.h"
#include "pxr/usd/ndr/api.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/ndr/declare.h"
#include "pxr/usd/ndr/nodeDiscoveryResult.h"
#include "pxr/usd/ndr/property.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class NdrNode
///
/// Represents an abstract node. Describes information like the name of the
/// node, what its inputs and outputs are, and any associated metadata.
///
/// In almost all cases, this class will not be used directly. More specialized
/// nodes can be created that derive from `NdrNode`; those specialized nodes can
/// add their own domain-specific data and methods.
///
class NdrNode
{
public:
    /// Constructor.
    NDR_API
    NdrNode(const NdrIdentifier& identifier,
            const NdrVersion& version,
            const std::string& name,
            const TfToken& family,
            const TfToken& context,
            const TfToken& sourceType,
            const std::string& definitionURI,
            const std::string& implementationURI,
            NdrPropertyUniquePtrVec&& properties,
            const NdrTokenMap& metadata = NdrTokenMap(),
            const std::string &sourceCode = std::string());

    /// Destructor.
    NDR_API
    virtual ~NdrNode();

    /// \name The Basics
    /// @{

    /// Return the identifier of the node.
    const NdrIdentifier& GetIdentifier() const { return _identifier; }

    /// Return the version of the node
    NdrVersion GetVersion() const { return _version; }

    /// Gets the name of the node.
    const std::string& GetName() const { return _name; }

    /// Gets the name of the family that the node belongs to. An empty token
    /// will be returned if the node does not belong to a family.
    const TfToken& GetFamily() const { return _family; }

    /// Gets the context of the node.
    ///
    /// The context is the context that the node declares itself as having (or,
    /// if a particular node does not declare a context, it will be assigned a
    /// default context by the parser).
    ///
    /// As a concrete example from the `Sdr` library, a shader with a specific
    /// source type may perform different duties vs. another shader with the
    /// same source type. For example, one shader with a source type of
    /// `SdrArgsParser::SourceType` may declare itself as having a context of
    /// 'pattern', while another shader of the same source type may say it is
    /// used for lighting, and thus has a context of 'light'.
    const TfToken& GetContext() const { return _context; }

    /// Gets the type of source that this node originated from.
    ///
    /// Note that this is distinct from `GetContext()`, which is the type that
    /// the node declares itself as having.
    ///
    /// As a concrete example from the `Sdr` library, several shader parsers
    /// exist and operate on different types of shaders. In this scenario, each
    /// distinct type of shader (OSL, Args, etc) is considered a different
    /// _source_, even though they are all shaders. In addition, the shaders
    /// under each source type may declare themselves as having a specific
    /// context (shaders can serve different roles). See `GetContext()` for
    /// more information on this.
    const TfToken& GetSourceType() const { return _sourceType; }

    /// Gets the URI to the resource that provided this node's
    /// definition. Could be a path to a file, or some other resource
    /// identifier. This URI should be fully resolved.
    ///
    /// \sa NdrNode::GetResolvedImplementationURI()
    const std::string& GetResolvedDefinitionURI() const { return _definitionURI; }

    /// Gets the URI to the resource that provides this node's
    /// implementation. Could be a path to a file, or some other resource
    /// identifier. This URI should be fully resolved.
    ///
    /// \sa NdrNode::GetResolvedDefinitionURI()
    const std::string& GetResolvedImplementationURI() const { return _implementationURI; }

    /// Returns  the source code for this node. This will be empty for most 
    /// nodes. It will be non-empty only for the nodes that are constructed 
    /// using \ref NdrRegistry::GetNodeFromSourceCode(), in which case, the 
    /// source code has not been parsed (or even compiled) yet. 
    /// 
    /// An unparsed node with non-empty source-code but no properties is 
    /// considered to be invalid. Once the node is parsed and the relevant 
    /// properties and metadata are extracted from the source code, the node 
    /// becomes valid.
    /// 
    /// \sa NdrNode::IsValid
    const std::string &GetSourceCode() const { return _sourceCode; }

    /// Whether or not this node is valid. A node that is valid indicates that
    /// the parser plugin was able to successfully parse the contents of this
    /// node.
    ///
    /// Note that if a node is not valid, some data like its name, URI, source 
    /// code etc. could still be available (data that was obtained during the 
    /// discovery process). However, other data that must be gathered from the 
    /// parsing process will NOT be available (eg, inputs and outputs).
    NDR_API
    virtual bool IsValid() const { return _isValid; }

    /// Gets a string with basic information about this node. Helpful for
    /// things like adding this node to a log.
    NDR_API
    virtual std::string GetInfoString() const;

    /// @}


    /// \name Inputs and Outputs
    /// An input or output is also generically referred to as a "property".
    /// @{

    /// Get an ordered list of all the input names on this node.
    NDR_API
    const NdrTokenVec& GetInputNames() const;

    /// Get an ordered list of all the output names on this node.
    NDR_API
    const NdrTokenVec& GetOutputNames() const;

    /// Get an input property by name. `nullptr` is returned if an input with
    /// the given name does not exist.
    NDR_API
    NdrPropertyConstPtr GetInput(const TfToken& inputName) const;

    /// Get an output property by name. `nullptr` is returned if an output with
    /// the given name does not exist.
    NDR_API
    NdrPropertyConstPtr GetOutput(const TfToken& outputName) const;

    /// @}


    /// \name Metadata
    /// The metadata returned here is a direct result of what the parser plugin
    /// is able to determine about the node. See the documentation for a
    /// specific parser plugin to get help on what the parser is looking for to
    /// populate these values.
    /// @{

    /// All metadata that came from the parse process. Specialized nodes may
    /// isolate values in the metadata (with possible manipulations and/or
    /// additional parsing) and expose those values in their API.
    NDR_API
    const NdrTokenMap& GetMetadata() const;

    /// @}

protected:
    NdrNode& operator=(const NdrNode&) = delete;

    bool _isValid;
    NdrIdentifier _identifier;
    NdrVersion _version;
    std::string _name;
    TfToken _family;
    TfToken _context;
    TfToken _sourceType;
    std::string _definitionURI;
    std::string _implementationURI;
    NdrPropertyUniquePtrVec _properties;
    NdrTokenMap _metadata;
    std::string _sourceCode;

    NdrPropertyPtrMap _inputs;
    NdrTokenVec _inputNames;
    NdrPropertyPtrMap _outputs;
    NdrTokenVec _outputNames;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_NDR_NODE_H
