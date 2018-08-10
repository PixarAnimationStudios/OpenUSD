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
#include "pxr/base/tf/refPtr.h"
#include "pxr/usd/ndr/debugCodes.h"
#include "pxr/usd/sdr/shaderMetadataHelpers.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"

#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

using ShaderMetadataHelpers::StringVal;
using ShaderMetadataHelpers::StringVecVal;
using ShaderMetadataHelpers::TokenVal;
using ShaderMetadataHelpers::TokenVecVal;

TF_DEFINE_PUBLIC_TOKENS(SdrNodeMetadata, SDR_NODE_METADATA_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(SdrNodeContext, SDR_NODE_CONTEXT_TOKENS);

SdrShaderNode::SdrShaderNode(
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
    : NdrNode(identifier, version, name, family,
              context, sourceType, uri, std::move(properties), 
              metadata, sourceCode)
{
    // Cast inputs to shader inputs
    for (const auto& input : _inputs) {
        _shaderInputs[input.first] =
            dynamic_cast<SdrShaderPropertyConstPtr>(input.second);
    }

    // ... and the same for outputs
    for (const auto& output : _outputs) {
        _shaderOutputs[output.first] =
            dynamic_cast<SdrShaderPropertyConstPtr>(output.second);
    }

    _InitializePrimvars();
    _PostProcessProperties();

    // Tokenize metadata
    _label = TokenVal(SdrNodeMetadata->Label, _metadata);
    _category = TokenVal(SdrNodeMetadata->Category, _metadata);
    _departments = TokenVecVal(SdrNodeMetadata->Departments, _metadata);
    _pages = _ComputePages();
}

void
SdrShaderNode::_PostProcessProperties()
{
    const NdrTokenVec vsNames = GetAllVstructNames();

    // Declare the input type to be vstruct if it's a vstruct head
    for (const TfToken& inputName : _inputNames) {
        NdrTokenVec::const_iterator it =
            std::find(vsNames.begin(), vsNames.end(), inputName);

        if (it != vsNames.end()) {
            SdrShaderPropertyConstPtr input = _shaderInputs.at(inputName);

            const_cast<SdrShaderProperty*>(input)->_type =
                SdrPropertyTypes->Vstruct;
        }
    }

    // Declare the output type to be vstruct if it's a vstruct head
    for (const TfToken& outputName : _outputNames) {
        NdrTokenVec::const_iterator it =
            std::find(vsNames.begin(), vsNames.end(), outputName);

        if (it != vsNames.end()) {
            SdrShaderPropertyConstPtr output = _shaderOutputs.at(outputName);

            const_cast<SdrShaderProperty*>(output)->_type =
                SdrPropertyTypes->Vstruct;
        }
    }
}

SdrShaderPropertyConstPtr
SdrShaderNode::GetShaderInput(const TfToken& inputName) const
{
    return dynamic_cast<SdrShaderPropertyConstPtr>(
        NdrNode::GetInput(inputName)
    );
}

SdrShaderPropertyConstPtr
SdrShaderNode::GetShaderOutput(const TfToken& outputName) const
{
    return dynamic_cast<SdrShaderPropertyConstPtr>(
        NdrNode::GetOutput(outputName)
    );
}

const std::string&
SdrShaderNode::GetHelp() const
{
    return StringVal(SdrNodeMetadata->Help, _metadata);
}

const std::string&
SdrShaderNode::GetImplementationName() const
{
    return StringVal(SdrNodeMetadata->ImplementationName, _metadata, GetName());
}

NdrTokenVec
SdrShaderNode::GetPropertyNamesForPage(const std::string& pageName) const
{
    NdrTokenVec propertyNames;

    for (const NdrPropertyUniquePtr& property : _properties) {
        const SdrShaderPropertyConstPtr shaderProperty =
            dynamic_cast<const SdrShaderPropertyConstPtr>(property.get());

        if (shaderProperty->GetPage() == pageName) {
            propertyNames.push_back(shaderProperty->GetName());
        }
    }

    return propertyNames;
}

NdrTokenVec
SdrShaderNode::GetAllVstructNames() const
{
    std::unordered_set<std::string> vstructs;

    for (const auto& input : _shaderInputs) {
        if (!input.second->IsVStructMember()) {
            continue;
        }

        const TfToken& head = input.second->GetVStructMemberOf();

        if (_shaderInputs.count(head)) {
            vstructs.insert(head);
        }
    }

    for (const auto& output : _shaderOutputs) {
        if (!output.second->IsVStructMember()) {
            continue;
        }

        const TfToken& head = output.second->GetVStructMemberOf();

        if (_shaderOutputs.count(head)) {
            vstructs.insert(head);
        }
    }

    // Transform the set into a vector
    return NdrTokenVec(vstructs.begin(), vstructs.end());
}

void
SdrShaderNode::_InitializePrimvars()
{
    NdrTokenVec primvars;
    NdrTokenVec primvarNamingProperties;

    // The "raw" list of primvars contains both ordinary primvars, and the names
    // of properties whose values contain additional primvar names
    const NdrStringVec rawPrimvars =
        StringVecVal(SdrNodeMetadata->Primvars, _metadata);

    for (const std::string& primvar : rawPrimvars) {
        if (TfStringStartsWith(primvar, "$")) {
            const std::string propertyName = TfStringTrimLeft(primvar, "$");
            const SdrShaderPropertyConstPtr input =
                GetShaderInput(TfToken(propertyName));

            if (input && (input->GetType() == SdrPropertyTypes->String)) {
                primvarNamingProperties.emplace_back(
                    TfToken(std::move(propertyName))
                );
            } else {
                TF_DEBUG(NDR_PARSING).Msg(
                    "Found a node [%s] whose metadata "
                    "indicates a primvar naming property [%s] "
                    "but the property's type is not string; ignoring.",  
                    GetName().c_str(), primvar.c_str());
            }
        } else {
            primvars.emplace_back(TfToken(primvar));
        }
    }

    _primvars = primvars;
    _primvarNamingProperties = primvarNamingProperties;
}

NdrTokenVec
SdrShaderNode::_ComputePages() const
{
    NdrTokenVec pages;

    for (const SdrPropertyMap::value_type& input : _shaderInputs) {
        const TfToken page = TfToken(input.second->GetPage());

        // Exclude duplicate pages
        if (std::find(pages.begin(), pages.end(), page) != pages.end()) {
            continue;
        }

        pages.emplace_back(std::move(page));
    }

    for (const SdrPropertyMap::value_type& output : _shaderOutputs) {
        const TfToken page = TfToken(output.second->GetPage());

        // Exclude duplicate pages
        if (std::find(pages.begin(), pages.end(), page) != pages.end()) {
            continue;
        }

        pages.emplace_back(std::move(page));
    }

    return pages;
}

PXR_NAMESPACE_CLOSE_SCOPE
