//
// Copyright 2022 benmalartre
//

#include "pxr/pxr.h"
#include "pxr/base/tf/refPtr.h"
#include "pxr/usd/ndr/debugCodes.h"
#include "pxr/usd/exec/execMetadataHelpers.h"
#include "pxr/usd/exec/execNode.h"
#include "pxr/usd/exec/execProperty.h"

#include <unordered_set>

PXR_NAMESPACE_OPEN_SCOPE

using ExecNodeMetadataHelpers::StringVal;
using ExecNodeMetadataHelpers::StringVecVal;
using ExecNodeMetadataHelpers::TokenVal;
using ExecNodeMetadataHelpers::TokenVecVal;
using ExecNodeMetadataHelpers::IntVal;

TF_DEFINE_PUBLIC_TOKENS(ExecNodeMetadata, EXEC_NODE_METADATA_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(ExecNodeContext, EXEC_NODE_CONTEXT_TOKENS);
TF_DEFINE_PUBLIC_TOKENS(ExecNodeRole, EXEC_NODE_ROLE_TOKENS);

ExecNode::ExecNode(
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
    : NdrNode(identifier, version, name, family,
              context, sourceType, definitionURI, implementationURI,
              std::move(properties), metadata, sourceCode)
{
    // Cast inputs to exec inputs
    for (const auto& input : _inputs) {
        _execInputs[input.first] =
            dynamic_cast<ExecPropertyConstPtr>(input.second);
    }

    // ... and the same for outputs
    for (const auto& output : _outputs) {
        _execOutputs[output.first] =
            dynamic_cast<ExecPropertyConstPtr>(output.second);
    }

    _InitializePrimvars();
    _PostProcessProperties();

    // Tokenize metadata
    _label = TokenVal(ExecNodeMetadata->Label, _metadata);
    _category = TokenVal(ExecNodeMetadata->Category, _metadata);
    _departments = TokenVecVal(ExecNodeMetadata->Departments, _metadata);
    _pages = _ComputePages();
}

void
ExecNode::_PostProcessProperties()
{
    
}

ExecPropertyConstPtr
ExecNode::GetExecInput(const TfToken& inputName) const
{
    return dynamic_cast<ExecPropertyConstPtr>(
        NdrNode::GetInput(inputName)
    );
}

ExecPropertyConstPtr
ExecNode::GetExecOutput(const TfToken& outputName) const
{
    return dynamic_cast<ExecPropertyConstPtr>(
        NdrNode::GetOutput(outputName)
    );
}

NdrTokenVec
ExecNode::GetAssetIdentifierInputNames() const
{
    NdrTokenVec result;
    for (const auto &inputName : GetInputNames()) {
        if (auto input = GetExecInput(inputName)) {
            if (input->IsAssetIdentifier()) {
                result.push_back(input->GetName());
            }
        }
    }
    return result;
}

ExecPropertyConstPtr 
ExecNode::GetDefaultInput() const
{
    std::vector<ExecPropertyConstPtr> result;
    for (const auto &inputName : GetInputNames()) {
        if (auto input = GetExecInput(inputName)) {
            if (input->IsDefaultInput()) {
                return input;
            }
        }
    }
    return nullptr;
}

std::string
ExecNode::GetHelp() const
{
    return StringVal(ExecNodeMetadata->Help, _metadata);
}

std::string
ExecNode::GetImplementationName() const
{
    return StringVal(ExecNodeMetadata->ImplementationName, _metadata, GetName());
}

std::string
ExecNode::GetRole() const
{
    return StringVal(ExecNodeMetadata->Role, _metadata, GetName());
}

NdrTokenVec
ExecNode::GetPropertyNamesForPage(const std::string& pageName) const
{
    NdrTokenVec propertyNames;

    for (const NdrPropertyUniquePtr& property : _properties) {
        const ExecPropertyConstPtr execProperty =
            dynamic_cast<const ExecPropertyConstPtr>(property.get());

        if (execProperty->GetPage() == pageName) {
            propertyNames.push_back(execProperty->GetName());
        }
    }

    return propertyNames;
}

void
ExecNode::_InitializePrimvars()
{
    NdrTokenVec primvars;
    NdrTokenVec primvarNamingProperties;

    // The "raw" list of primvars contains both ordinary primvars, and the names
    // of properties whose values contain additional primvar names
    const NdrStringVec rawPrimvars =
        StringVecVal(ExecNodeMetadata->Primvars, _metadata);

    for (const std::string& primvar : rawPrimvars) {
        if (TfStringStartsWith(primvar, "$")) {
            const std::string propertyName = TfStringTrimLeft(primvar, "$");
            const ExecPropertyConstPtr input =
                GetExecInput(TfToken(propertyName));

            if (input && (input->GetType() == ExecPropertyTypes->String)) {
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
ExecNode::_ComputePages() const
{
    NdrTokenVec pages;

    for (const NdrPropertyUniquePtr& property : _properties) {
        auto execProperty = static_cast<ExecPropertyPtr>(property.get());
        const TfToken& page = execProperty->GetPage();

        // Exclude duplicate pages
        if (std::find(pages.begin(), pages.end(), page) != pages.end()) {
            continue;
        }

        pages.emplace_back(std::move(page));
    }

    return pages;
}

PXR_NAMESPACE_CLOSE_SCOPE
