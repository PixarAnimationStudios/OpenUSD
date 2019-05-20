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
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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
    const std::string& uri,
    NdrPropertyUniquePtrVec&& properties,
    const NdrTokenMap& metadata,
    const std::string &sourceCode)
    : _identifier(identifier),
      _version(version),
      _name(name),
      _family(family),
      _context(context),
      _sourceType(sourceType),
      _uri(uri),
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
        "%s (context: '%s', version: '%s', family: '%s'); URI: '%s'",
        NdrGetIdentifierString(_identifier).c_str(), _context.GetText(),
        _version.GetString().c_str(), _family.GetText(), _uri.c_str()
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
