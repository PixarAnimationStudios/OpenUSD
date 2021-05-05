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
#include "pxr/pxr.h"
#include "pxr/usd/plugin/usdMtlx/utils.h"
#include "pxr/usd/ndr/debugCodes.h"
#include "pxr/usd/ndr/node.h"
#include "pxr/usd/ndr/nodeDiscoveryResult.h"
#include "pxr/usd/ndr/parserPlugin.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/usdUtils/pipeline.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"

namespace mx = MaterialX;

PXR_NAMESPACE_OPEN_SCOPE

namespace {

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((discoveryType, "mtlx"))
    ((sourceType, ""))
);

// This environment variable lets users override the name of the primary
// UV set that MaterialX should look for.  If it's empty, it uses the USD
// default, "st".
TF_DEFINE_ENV_SETTING(USDMTLX_PRIMARY_UV_NAME, "",
    "The name usdMtlx should use to reference the primary UV set.");

static const std::string _GetPrimaryUvSetName()
{
    static const std::string env = TfGetEnvSetting(USDMTLX_PRIMARY_UV_NAME);
    if (env.empty()) {
        return UsdUtilsGetPrimaryUVSetName().GetString();
    }
    return env;
}

// A builder for shader nodes.  We find it convenient to build the
// arguments to SdrShaderNode across multiple functions.  This type
// holds the arguments.
struct ShaderBuilder {
public:
    ShaderBuilder(const NdrNodeDiscoveryResult& discoveryResult)
        : discoveryResult(discoveryResult)
        , valid(true) 
        , metadata(discoveryResult.metadata) { }

    void SetInvalid()              { valid = false; }
    explicit operator bool() const { return valid; }
    bool operator !() const        { return !valid; }

    NdrNodeUniquePtr Build()
    {
        if (!*this) {
            return NdrParserPlugin::GetInvalidNode(discoveryResult);
        }

        return NdrNodeUniquePtr(
                new SdrShaderNode(discoveryResult.identifier,
                                  discoveryResult.version,
                                  discoveryResult.name,
                                  discoveryResult.family,
                                  context,
                                  discoveryResult.sourceType,
                                  definitionURI,
                                  implementationURI,
                                  std::move(properties),
                                  std::move(metadata)));
    }

    void AddPropertyNameRemapping(const std::string& from,
                                  const std::string& to)
    {
        if (from != to) {
            _propertyNameRemapping[from] = to;
        }
    }

    void AddProperty(const mx::ConstTypedElementPtr& element,
                     bool isOutput, NdrStringVec *primvars);

public:
    const NdrNodeDiscoveryResult& discoveryResult;
    bool valid;

    std::string definitionURI;
    std::string implementationURI;
    TfToken context;
    NdrPropertyUniquePtrVec properties;
    NdrTokenMap metadata;

private:
    std::map<std::string, std::string> _propertyNameRemapping;
};

void
ShaderBuilder::AddProperty(
    const mx::ConstTypedElementPtr& element,
    bool isOutput, NdrStringVec *primvars)
{
    TfToken type;
    NdrTokenMap metadata;
    NdrTokenMap hints;
    NdrOptionVec options;
    VtValue defaultValue;

    const auto& mtlxType = element->getType();
    const auto converted = UsdMtlxGetUsdType(mtlxType);
    if (converted.shaderPropertyType.IsEmpty()) {
        // Not found.  If an Sdf type exists use that.
        if (converted.valueTypeName) {
            type = converted.valueTypeName.GetAsToken();
        }
        else {
            type = TfToken(mtlxType);

            // This could be a custom type.  Check the document.
            if (!element->getDocument()->getTypeDef(mtlxType)) {
                TF_WARN("MaterialX unrecognized type %s on %s",
                        mtlxType.c_str(),
                        element->getNamePath().c_str());
            }
        }
    }
    else {
        // Get the sdr type.
        type = converted.shaderPropertyType;
        if (converted.valueTypeName.IsArray() && converted.arraySize == 0) {
            metadata.emplace(SdrPropertyMetadata->IsDynamicArray, "");
        }

        // Check for an asset type.
        if (converted.valueTypeName == SdfValueTypeNames->Asset) {
            metadata.emplace(SdrPropertyMetadata->IsAssetIdentifier, "");
        }

        // If this is a MaterialX parameter or input then get the value,
        // otherwise it's an output or nodedef and get the default.
        defaultValue = UsdMtlxGetUsdValue(element, isOutput);
    }

    // If this is an output then save the defaultinput, if any.
    static const std::string defaultinputName("defaultinput");
    if (isOutput) {
        const auto& defaultinput = element->getAttribute(defaultinputName);
        if (!defaultinput.empty()) {
            metadata.emplace(SdrPropertyMetadata->DefaultInput, defaultinput);
        }
    }

    // Record the targets on inputs.
    static const std::string targetName("target");
    if (!isOutput) {
        const auto& target = element->getAttribute(targetName);
        if (!target.empty()) {
            metadata.emplace(SdrPropertyMetadata->Target, target);
        }
    }

    // Record the colorspace on inputs and outputs.
    static const std::string colorspaceName("colorspace");
    if (isOutput || element->isA<mx::Input>()) {
        const auto& colorspace = element->getAttribute(colorspaceName);
        if (!colorspace.empty() &&
                colorspace != element->getParent()->getActiveColorSpace()) {
            metadata.emplace(SdrPropertyMetadata->Colorspace, colorspace);
        }
    }

    // Get the property name.
    auto name = element->getName();

    // Record builtin primvar references for this node's inputs.
    static const std::string defaultgeompropName("defaultgeomprop");
    if (!isOutput && primvars != nullptr) {

        // If an input has "defaultgeomprop", that means it reads from the
        // primvar specified unless connected. We mark these in Sdr as
        // always-required primvars; note that this means we might overestimate
        // which primvars are referenced in a material.
        const auto& defaultgeomprop = element->getAttribute(defaultgeompropName);
        if (!defaultgeomprop.empty()) {
            // Note: MaterialX uses a default texcoord of "UV0", which we
            // inline replace with the configured default.
            if (defaultgeomprop == "UV0") {
                primvars->push_back(_GetPrimaryUvSetName());
            } else {
                primvars->push_back(defaultgeomprop);
            }
        }
    }

    // MaterialX doesn't name the output of a nodedef unless it has
    // multiple outputs.  The default name would be the name of the
    // nodedef itself, which seems wrong.  We pick a different name.
    if (auto nodeDef = element->asA<mx::NodeDef>()) {
        name = UsdMtlxTokens->DefaultOutputName.GetString();
    }

    // Remap property name.
    auto j = _propertyNameRemapping.find(name);
    if (j != _propertyNameRemapping.end()) {
        metadata[SdrPropertyMetadata->ImplementationName] = j->second;
    }

    // Add the property.
    properties.push_back(
        SdrShaderPropertyUniquePtr(
            new SdrShaderProperty(TfToken(name),
                                  type,
                                  defaultValue,
                                  isOutput,
                                  converted.arraySize,
                                  metadata,
                                  hints,
                                  options)));
}

static
void
ParseMetadata(
    ShaderBuilder* builder,
    const TfToken& key,
    const mx::ConstElementPtr& element,
    const std::string& attribute)
{
    const auto& value = element->getAttribute(attribute);
    if (!value.empty()) {
        // Change the MaterialX Texture node role from 'texture2d' to 'texture' 
        if (key == SdrNodeMetadata->Role && value == "texture2d") {
            builder->metadata[key] = "texture";
        }
        else {
            builder->metadata[key] = value;
        }
    }
}

static
TfToken
GetContext(const mx::ConstDocumentPtr& doc, const std::string& type)
{
    if (doc) {
        if (auto mtlxTypeDef = doc->getTypeDef(type)) {
            // Use the context if the type has "shader" semantic.
            if (mtlxTypeDef->getAttribute("semantic") == "shader") {
                return TfToken(mtlxTypeDef->getAttribute("context"));
            }
        }
    }
    return TfToken();
}

static
void
ParseElement(ShaderBuilder* builder, const mx::ConstNodeDefPtr& nodeDef)
{
    if (!TF_VERIFY(nodeDef)) {
        return;
    }

    const auto& type = nodeDef->getType();

    // Get the context.
    TfToken context = GetContext(nodeDef->getDocument(), type);
    if (context.IsEmpty()) {
        // Fallback to standard typedefs.
        context = GetContext(UsdMtlxGetDocument(""), type);
    }
    if (context.IsEmpty()) {
        context = SdrNodeContext->Pattern;
    }

    // Build the basic shader node info. We are filling in implementationURI
    // as a placeholder - it should get set to a more acccurate value by caller.
    builder->context           = context;
    builder->definitionURI     = UsdMtlxGetSourceURI(nodeDef);
    builder->implementationURI = builder->definitionURI;

    // Metadata
    builder->metadata[SdrNodeMetadata->Label] = nodeDef->getNodeString();
    ParseMetadata(builder, SdrNodeMetadata->Category, nodeDef, "nodecategory");
    ParseMetadata(builder, SdrNodeMetadata->Help, nodeDef, "doc");
    ParseMetadata(builder, SdrNodeMetadata->Target, nodeDef, "target");
    ParseMetadata(builder, SdrNodeMetadata->Role, nodeDef, "nodegroup");

    // XXX -- version

    NdrStringVec primvars;

    // If the nodeDef name starts with ND_geompropvalue, it's a primvar reader
    // node and we want to add $geomprop to the list of referenced primvars.
    if (TfStringStartsWith(nodeDef->getName(), "ND_geompropvalue")) {
        primvars.push_back("$geomprop");
    }

    // Also check internalgeomprops.
    static const std::string internalgeompropsName("internalgeomprops");
    const auto& internalgeomprops = nodeDef->getAttribute(internalgeompropsName);
    if (!internalgeomprops.empty()) {
        std::vector<std::string> split =
            UsdMtlxSplitStringArray(internalgeomprops);
        // Note: MaterialX uses a default texcoord of "UV0", which we
        // inline replace with the configured default.
        for (auto& name : split) {
            if (name == "UV0") {
                name = _GetPrimaryUvSetName();
            }
        }
        primvars.insert(primvars.end(), split.begin(), split.end());
    }

    // Properties
    for (const auto& mtlxInput: nodeDef->getInputs()) {
        builder->AddProperty(mtlxInput, false, &primvars);
    }
    for (const auto& mtlxOutput: nodeDef->getOutputs()) {
        builder->AddProperty(mtlxOutput, true, nullptr);
    }

    builder->metadata[SdrNodeMetadata->Primvars] =
        TfStringJoin(primvars.begin(), primvars.end(), "|");
}

static
void
ParseElement(
    ShaderBuilder* builder,
    const mx::ConstNodeGraphPtr& nodeGraph,
    const NdrNodeDiscoveryResult& discoveryResult)
{
    ParseElement(builder, nodeGraph->getNodeDef());
    if (*builder) {
        // XXX -- Node graphs not supported yet.
    }
}

static
void
ParseElement(
    ShaderBuilder* builder,
    const mx::ConstImplementationPtr& impl,
    const NdrNodeDiscoveryResult& discoveryResult)
{
    // Name remapping.
    for (const auto& mtlxInput: impl->getInputs()) {
        builder->AddPropertyNameRemapping(
            mtlxInput->getName(),
            mtlxInput->getAttribute("implname"));
    }

    ParseElement(builder, impl->getNodeDef());
    if (!*builder) {
        return;
    }

    // Get the implementation file.  Note we're not doing proper Ar asset
    // localization here yet.
    auto filename = impl->getFile();
    if (filename.empty()) {
        builder->SetInvalid();
        return;
    }

    if (TfIsRelativePath(filename)) {
        // The path is relative to some library path but we don't know which.
        // We'll just check them all until we find an existing file.
        // XXX -- Since we're likely to do this with every implementation
        //        element we should consider some kind of cache so we don't
        //        keep hitting the filesystem.
        // XXX -- A future version of the asset resolver that has protocols
        //        would make it easy for clients to resolve a relative path.
        //        We should switch to that when available.
        for (const auto& dir: UsdMtlxStandardLibraryPaths()) {
            const auto path = TfStringCatPaths(dir, filename);
            if (TfIsFile(path, true)) {
                filename = path;
                break;
            }
        }
        if (TfIsRelativePath(filename)) {
            TF_DEBUG(NDR_PARSING).Msg("MaterialX implementation %s could "
                "not be found", filename.c_str());
            builder->SetInvalid();
            return;
        }
    }
    builder->implementationURI = filename;

    // Function
    const auto& function = impl->getFunction();
    if (!function.empty()) {
        builder->metadata[SdrNodeMetadata->ImplementationName] = function;
    }
}

} // anonymous namespace

/// Parses nodes in MaterialX files.
class UsdMtlxParserPlugin : public NdrParserPlugin {
public:
    UsdMtlxParserPlugin() = default;
    ~UsdMtlxParserPlugin() override = default;

    NdrNodeUniquePtr Parse(
        const NdrNodeDiscoveryResult& discoveryResult) override;
    const NdrTokenVec& GetDiscoveryTypes() const override;
    const TfToken& GetSourceType() const override;
};

NdrNodeUniquePtr
UsdMtlxParserPlugin::Parse(
    const NdrNodeDiscoveryResult& discoveryResult)
{
    MaterialX::ConstDocumentPtr document = nullptr;
    // Get the MaterialX document.
    if (!discoveryResult.resolvedUri.empty()) {
        document =
            UsdMtlxGetDocument(discoveryResult.resolvedUri == "mtlx"
                            ? "" : discoveryResult.resolvedUri);
        if (!TF_VERIFY(document)) {
            return GetInvalidNode(discoveryResult);
        }
    } else if (!discoveryResult.sourceCode.empty()) {
        document = UsdMtlxGetDocumentFromString(
            discoveryResult.sourceCode);
        if (!document) {
            TF_WARN("Invalid mtlx source code.");
            return GetInvalidNode(discoveryResult);
        }
    } else {
        TF_WARN("Invalid NdrNodeDiscoveryResult for identifier '%s': both "
            "resolvedUri and sourceCode fields are empty.", 
            discoveryResult.identifier.GetText());
        return GetInvalidNode(discoveryResult);
    }

    // Get the element.
    if (discoveryResult.blindData.empty()) {
        TF_WARN("Invalid MaterialX blindData; should have node name");
        return GetInvalidNode(discoveryResult);
    }

    auto element = document->getChild(discoveryResult.blindData);
    if (!element) {
        TF_WARN("Invalid MaterialX blindData; unknown node name ' %s '",
            discoveryResult.blindData.c_str());
        return GetInvalidNode(discoveryResult);
    }

    // Handle nodegraphs and implementations differently.
    ShaderBuilder builder(discoveryResult);
    if (auto nodeGraph = element->asA<mx::NodeGraph>()) {
        ParseElement(&builder, nodeGraph, discoveryResult);
    }
    else if (auto impl = element->asA<mx::Implementation>()) {
        ParseElement(&builder, impl, discoveryResult);
    }
    else {
        TF_VERIFY(false,
                  "MaterialX node '%s' isn't a nodegraph or implementation",
                  element->getNamePath().c_str());
    }

    return builder.Build();
}

const NdrTokenVec&
UsdMtlxParserPlugin::GetDiscoveryTypes() const
{
    static const NdrTokenVec discoveryTypes = {
        _tokens->discoveryType
    };
    return discoveryTypes;
}

const TfToken&
UsdMtlxParserPlugin::GetSourceType() const
{
    return _tokens->sourceType;
}

NDR_REGISTER_PARSER_PLUGIN(UsdMtlxParserPlugin)

PXR_NAMESPACE_CLOSE_SCOPE
