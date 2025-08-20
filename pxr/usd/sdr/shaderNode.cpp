//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/usd/sdr/debugCodes.h"
#include "pxr/usd/sdf/valueTypeName.h"
#include "pxr/usd/sdr/shaderMetadataHelpers.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"

#include <algorithm>

PXR_NAMESPACE_OPEN_SCOPE

using ShaderMetadataHelpers::StringVal;
using ShaderMetadataHelpers::StringVecVal;
using ShaderMetadataHelpers::TokenVal;
using ShaderMetadataHelpers::TokenVecVal;
using ShaderMetadataHelpers::IntVal;

TF_DEFINE_PUBLIC_TOKENS(SdrNodeMetadata, SDR_NODE_METADATA_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(SdrNodeContext, SDR_NODE_CONTEXT_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(SdrNodeRole, SDR_NODE_ROLE_TOKENS);

SdrShaderNode::SdrShaderNode(
    const SdrIdentifier& identifier,
    const SdrVersion& version,
    const std::string& name,
    const TfToken& family,
    const TfToken& context,
    const TfToken& sourceType,
    const std::string& definitionURI,
    const std::string& implementationURI,
    SdrShaderPropertyUniquePtrVec&& properties,
    const SdrTokenMap& metadata,
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

    // Build a map of input/output name -> SdrShaderProperty.
    // This could also be done lazily if needed.
    size_t numProperties = _properties.size();
    for (size_t i = 0; i < numProperties; i++) {
        SdrShaderPropertyConstPtr property = _properties[i].get();
        const TfToken& propertyName = property->GetName();

        if (property->IsOutput()) {
            _outputNames.push_back(propertyName);
            _outputs.insert({propertyName, property});
        } else {
            _inputNames.push_back(propertyName);
            _inputs.insert({propertyName, property});
        }
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
    // See if this shader node has been tagged with an explict USD encoding
    // version, which affects how properties manifest in USD files. We propagate
    // this metadatum to the individual properties, since the encoding is
    // controlled there in the GetTypeAsSdfType method.
    static const int DEFAULT_ENCODING = -1;
    int usdEncodingVersion =
        IntVal(SdrNodeMetadata->SdrUsdEncodingVersion, _metadata,
               DEFAULT_ENCODING);

    const SdrTokenVec vsNames = GetAllVstructNames();

    for (const SdrShaderPropertyUniquePtr& property : _properties) {
        SdrShaderPropertyConstPtr constShaderProperty = property.get();
        // This function, and only this function, has special permission (is a
        // friend function of SdrProperty) to call private methods and so we
        // need a non-const pointer.
        SdrShaderProperty* shaderProperty =
            const_cast<SdrShaderProperty*>(constShaderProperty);

        if (usdEncodingVersion != DEFAULT_ENCODING) {
            shaderProperty->_SetUsdEncodingVersion(usdEncodingVersion);
        }

        bool isVStruct = std::find(vsNames.begin(), vsNames.end(),
                                   shaderProperty->GetName()) != vsNames.end();
        if (isVStruct) {
            shaderProperty->_ConvertToVStruct();
        }

        // There must not be any further modifications of this property after
        // this method has been called.
        shaderProperty->_FinalizeProperty();
    }
}

SdrShaderNode::~SdrShaderNode()
{
    // nothing yet
}

std::string
SdrShaderNode::GetInfoString() const
{
    return TfStringPrintf(
        "%s (context: '%s', version: '%s', family: '%s'); definition URI: '%s';"
        " implementation URI: '%s'",
        SdrGetIdentifierString(_identifier).c_str(), _context.GetText(),
        _version.GetString().c_str(), _family.GetText(), 
        _definitionURI.c_str(), _implementationURI.c_str()
    );
}

const SdrTokenVec&
SdrShaderNode::GetShaderInputNames() const {
    return _inputNames;
}

const SdrTokenVec&
SdrShaderNode::GetShaderOutputNames() const {
    return _outputNames;
}

SdrShaderPropertyConstPtr
SdrShaderNode::GetShaderInput(const TfToken& inputName) const
{
    SdrShaderPropertyMap::const_iterator it = _inputs.find(inputName);

    if (it != _inputs.end()) {
        return it->second;
    }

    return nullptr;
}

SdrShaderPropertyConstPtr
SdrShaderNode::GetShaderOutput(const TfToken& outputName) const
{
    SdrShaderPropertyMap::const_iterator it = _outputs.find(outputName);

    if (it != _outputs.end()) {
        return it->second;
    }

    return nullptr;
}

SdrTokenVec
SdrShaderNode::GetAssetIdentifierInputNames() const
{
    SdrTokenVec result;
    for (const auto &inputName : GetShaderInputNames()) {
        if (auto input = GetShaderInput(inputName)) {
            if (input->IsAssetIdentifier()) {
                result.push_back(input->GetName());
            }
        }
    }
    return result;
}

SdrShaderPropertyConstPtr 
SdrShaderNode::GetDefaultInput() const
{
    std::vector<SdrShaderPropertyConstPtr> result;
    for (const auto &inputName : GetShaderInputNames()) {
        if (auto input = GetShaderInput(inputName)) {
            if (input->IsDefaultInput()) {
                return input;
            }
        }
    }
    return nullptr;
}

const SdrTokenMap&
SdrShaderNode::GetMetadata() const
{
    return _metadata;
}

std::string
SdrShaderNode::GetHelp() const
{
    return StringVal(SdrNodeMetadata->Help, _metadata);
}

std::string
SdrShaderNode::GetImplementationName() const
{
    return StringVal(SdrNodeMetadata->ImplementationName, _metadata, GetName());
}

std::string
SdrShaderNode::GetRole() const
{
    return StringVal(SdrNodeMetadata->Role, _metadata, GetName());
}

SdrTokenVec
SdrShaderNode::GetPropertyNamesForPage(const std::string& pageName) const
{
    SdrTokenVec propertyNames;

    for (const SdrShaderPropertyUniquePtr& property : _properties) {
        if (property->GetPage() == pageName) {
            propertyNames.push_back(property->GetName());
        }
    }

    return propertyNames;
}

SdrTokenVec
SdrShaderNode::GetAllVstructNames() const
{
    std::unordered_set<std::string> vstructs;

    auto hasVstructMetadata = [] (const SdrShaderPropertyConstPtr& property) {
        const SdrTokenMap& metadata = property->GetMetadata();
        const auto t = metadata.find(SdrPropertyMetadata->Tag);
        return (t != metadata.end() && t->second == "vstruct");
    };

    for (const auto& input : _inputs) {

        if (hasVstructMetadata(input.second)) {
            vstructs.insert(input.first);
            continue;
        }

        if (!input.second->IsVStructMember()) {
            continue;
        }

        const TfToken& head = input.second->GetVStructMemberOf();

        if (_inputs.count(head)) {
            vstructs.insert(head);
        }
    }

    for (const auto& output : _outputs) {

        if (hasVstructMetadata(output.second)) {
            vstructs.insert(output.first);
            continue;
        }

        if (!output.second->IsVStructMember()) {
            continue;
        }

        const TfToken& head = output.second->GetVStructMemberOf();

        if (_outputs.count(head)) {
            vstructs.insert(head);
        }
    }

    // Transform the set into a vector
    return SdrTokenVec(vstructs.begin(), vstructs.end());
}

/* static */
SdrShaderNode::ComplianceResults
SdrShaderNode::CheckPropertyCompliance(
    const std::vector<SdrShaderNodeConstPtr> &shaderNodes)
{
    std::unordered_map<TfToken, SdrShaderPropertyConstPtr, TfToken::HashFunctor>
        propertyMap;
    SdrShaderNode::ComplianceResults result;
    for (SdrShaderNodeConstPtr shaderNode : shaderNodes) {
        for (const TfToken &propName : shaderNode->GetShaderInputNames()) {
            if (SdrShaderPropertyConstPtr sdrProp = 
                 shaderNode->GetShaderInput(propName)) {
                auto propIt = propertyMap.find(propName);
                if (propIt == propertyMap.end()) {
                    // insert property
                    propertyMap.emplace(propName, sdrProp);
                } else {
                    // property already found, lets check for compliance
                    if (propIt->second->GetTypeAsSdfType() != 
                            sdrProp->GetTypeAsSdfType() ||
                        propIt->second->GetDefaultValue() !=
                            sdrProp->GetDefaultValue() ||
                        propIt->second->GetDefaultValueAsSdfType() !=
                            sdrProp->GetDefaultValueAsSdfType()) {
                        auto resultIt = result.find(propName);
                        if (resultIt == result.end()) {
                            result.emplace(propName, 
                                           std::vector<SdrIdentifier>{
                                               shaderNode->GetIdentifier()});
                        } else {
                            resultIt->second.push_back(
                                shaderNode->GetIdentifier());
                        }
                    }
                }
            }
        }
    }
    return result;
}

void
SdrShaderNode::_InitializePrimvars()
{
    SdrTokenVec primvars;
    SdrTokenVec primvarNamingProperties;

    // The "raw" list of primvars contains both ordinary primvars, and the names
    // of properties whose values contain additional primvar names
    const SdrStringVec rawPrimvars =
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
                TF_DEBUG(SDR_PARSING).Msg(
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

SdrTokenVec
SdrShaderNode::_ComputePages() const
{
    SdrTokenVec pages;

    for (const SdrShaderPropertyUniquePtr& property : _properties) {
        const TfToken& page = property->GetPage();

        // Exclude duplicate pages
        if (std::find(pages.begin(), pages.end(), page) != pages.end()) {
            continue;
        }

        pages.emplace_back(std::move(page));
    }

    return pages;
}

PXR_NAMESPACE_CLOSE_SCOPE
