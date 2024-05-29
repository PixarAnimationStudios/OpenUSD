//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/ndr/node.h"
#include "pxr/usd/ndr/nodeDiscoveryResult.h"
#include "pxr/usd/ndr/property.h"

PXR_NAMESPACE_OPEN_SCOPE

NdrNode::NdrNode(
    const NdrIdentifier& identifier,
    const NdrVersion& version,
    const std::string& name,
    const TfToken& family,
    const TfToken& context,
    const TfToken& sourceType,
    const std::string& definitionURI,
    const std::string& implementationURI,
    NdrPropertyUniquePtrVec&& properties,
    const NdrTokenMap& metadata,
    const std::string &sourceCode)
    : _identifier(identifier),
      _version(version),
      _name(name),
      _family(family),
      _context(context),
      _sourceType(sourceType),
      _definitionURI(definitionURI),
      _implementationURI(implementationURI),
      _properties(std::move(properties)),
      _metadata(metadata),
      _sourceCode(sourceCode)
{
    // If the properties are not empty, that signifies that the node was parsed
    // successfully, and thus the node is valid.
    _isValid = !_properties.empty();

    // Build a map of input/output name -> NdrProperty.
    // This could also be done lazily if needed.
    size_t numProperties = _properties.size();
    for (size_t i = 0; i < numProperties; i++) {
        NdrPropertyConstPtr property = _properties[i].get();
        const TfToken& propertyName = property->GetName();

        if (property->IsOutput()) {
            _outputNames.push_back(propertyName);
            _outputs.insert({propertyName, property});
        } else {
            _inputNames.push_back(propertyName);
            _inputs.insert({propertyName, property});
        }
    }
}

NdrNode::~NdrNode()
{
    // nothing yet
}

std::string
NdrNode::GetInfoString() const
{
    return TfStringPrintf(
        "%s (context: '%s', version: '%s', family: '%s'); definition URI: '%s';"
        " implementation URI: '%s'",
        NdrGetIdentifierString(_identifier).c_str(), _context.GetText(),
        _version.GetString().c_str(), _family.GetText(), 
        _definitionURI.c_str(), _implementationURI.c_str()
    );
}

const NdrTokenVec&
NdrNode::GetInputNames() const
{
    return _inputNames;
}

const NdrTokenVec&
NdrNode::GetOutputNames() const
{
    return _outputNames;
}

NdrPropertyConstPtr
NdrNode::GetInput(const TfToken& inputName) const
{
    NdrPropertyPtrMap::const_iterator it = _inputs.find(inputName);

    if (it != _inputs.end()) {
        return it->second;
    }

    return nullptr;
}

NdrPropertyConstPtr
NdrNode::GetOutput(const TfToken& outputName) const
{
    NdrPropertyPtrMap::const_iterator it = _outputs.find(outputName);

    if (it != _outputs.end()) {
        return it->second;
    }

    return nullptr;
}

const NdrTokenMap&
NdrNode::GetMetadata() const
{
    return _metadata;
}

PXR_NAMESPACE_CLOSE_SCOPE
