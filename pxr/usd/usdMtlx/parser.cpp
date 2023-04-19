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
#include "pxr/usd/usdMtlx/utils.h"
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

    (colorspace)
    (defaultgeomprop)
    (defaultinput)
    (doc)
    ((enum_, "enum"))
    (enumvalues)
    (nodecategory)
    (nodegroup)
    (target)
    (uifolder)
    (uimax)
    (uimin)
    (uiname)
    (uisoftmax)
    (uisoftmin)
    (uistep)
    (unit)
    (unittype)
    (UV0)
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
        return UsdUtilsGetPrimaryUVSetName();
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
                     bool isOutput, NdrStringVec *primvars, 
                     bool addedTexcoordPrimvar=false);

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

static
void
ParseMetadata(
    NdrTokenMap& metadata,
    const TfToken& key,
    const mx::ConstElementPtr& element,
    const std::string& attribute)
{
    const auto& value = element->getAttribute(attribute);
    if (!value.empty()) {
        metadata.emplace(key, value);
    }
}

static
void
ParseMetadata(
    NdrTokenMap& metadata,
    const TfToken& key,
    const mx::ConstElementPtr& element)
{
    const auto& value = element->getAttribute(key);
    if (!value.empty()) {
        metadata.emplace(key, value);
    }
}

static
void
ParseOptions(
    NdrOptionVec& options,
    const mx::ConstElementPtr& element
)
{
    const auto& enumLabels = element->getAttribute(_tokens->enum_);
    if (enumLabels.empty()) {
        return;
    }

    const auto& enumValues = element->getAttribute(_tokens->enumvalues);
    std::vector<std::string> allLabels = UsdMtlxSplitStringArray(enumLabels);
    std::vector<std::string> allValues = UsdMtlxSplitStringArray(enumValues);

    if (!allValues.empty() && allValues.size() != allLabels.size()) {
        // An array of vector2 values will produce twice the expected number of
        // elements. We can fix that by regrouping them.
        if (allValues.size() > allLabels.size() &&
            allValues.size() % allLabels.size() == 0) {

            size_t stride = allValues.size() / allLabels.size();
            std::vector<std::string> rebuiltValues;
            std::string currentValue;
            for (size_t i = 0; i < allValues.size(); ++i) {
                if (i % stride != 0) {
                    currentValue += mx::ARRAY_PREFERRED_SEPARATOR;
                }
                currentValue += allValues[i];
                if ((i+1) % stride == 0) {
                    rebuiltValues.push_back(currentValue);
                    currentValue = "";
                }
            }
            allValues.swap(rebuiltValues);
        } else {
            // Can not reconcile the size difference:
            allValues.clear();
        }
    }

    auto itLabels = allLabels.cbegin();
    auto itValues = allValues.cbegin();
    while (itLabels != allLabels.cend()) {
        TfToken value;
        if (itValues != allValues.cend()) {
            value = TfToken(*itValues++);
        }
        options.emplace_back(TfToken(*itLabels++), value);
    }
}

void
ShaderBuilder::AddProperty(
    const mx::ConstTypedElementPtr& element,
    bool isOutput, NdrStringVec *primvars, bool addedTexcoordPrimvar)
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
            // Do not use GetAsToken for comparison as recommended in the API
            if (converted.valueTypeName == SdfValueTypeNames->Bool) {
                 defaultValue = UsdMtlxGetUsdValue(element, isOutput);
                 metadata.emplace(SdrPropertyMetadata->SdrUsdDefinitionType,
                           converted.valueTypeName.GetType().GetTypeName());
            }
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
    if (isOutput) {
        const auto& defaultinput = element->getAttribute(_tokens->defaultinput);
        if (!defaultinput.empty()) {
            metadata.emplace(SdrPropertyMetadata->DefaultInput, defaultinput);
        }
    }

    // Record the targets on inputs.
    if (!isOutput) {
        const auto& target = element->getAttribute(_tokens->target);
        if (!target.empty()) {
            metadata.emplace(SdrPropertyMetadata->Target, target);
        }
    }

    // Record the colorspace on inputs and outputs.
    if (isOutput || element->isA<mx::Input>()) {
        const auto& colorspace = element->getAttribute(_tokens->colorspace);
        if (!colorspace.empty() &&
                colorspace != element->getParent()->getActiveColorSpace()) {
            metadata.emplace(SdrPropertyMetadata->Colorspace, colorspace);
        }
    }

    // Get the property name.
    auto name = element->getName();

    // Record builtin primvar references for this node's inputs.
    if (!isOutput && primvars != nullptr) {

        // If an input has "defaultgeomprop", that means it reads from the
        // primvar specified unless connected. We mark these in Sdr as
        // always-required primvars; note that this means we might overestimate
        // which primvars are referenced in a material.
        const auto& defaultgeomprop = element->getAttribute(_tokens->defaultgeomprop);
        if (!defaultgeomprop.empty()) {
            // Note: MaterialX uses a default texcoord of "UV0", which we
            // inline replace with the configured default.
            if (defaultgeomprop == _tokens->UV0) {
                if (!addedTexcoordPrimvar) {
                    primvars->push_back(_GetPrimaryUvSetName());
                }
            } else {
                primvars->push_back(defaultgeomprop);
            }
        }
    }

    // MaterialX doesn't name the output of a nodedef unless it has
    // multiple outputs.  The default name would be the name of the
    // nodedef itself, which seems wrong.  We pick a different name.
    if (auto nodeDef = element->asA<mx::NodeDef>()) {
        name = UsdMtlxTokens->DefaultOutputName;
    }

    // Remap property name.
    auto j = _propertyNameRemapping.find(name);
    if (j != _propertyNameRemapping.end()) {
        metadata[SdrPropertyMetadata->ImplementationName] = j->second;
    }

    if (!isOutput) {
        ParseMetadata(metadata, SdrPropertyMetadata->Label, element, _tokens->uiname);
        ParseMetadata(metadata, SdrPropertyMetadata->Help, element, _tokens->doc);
        ParseMetadata(metadata, SdrPropertyMetadata->Page, element, _tokens->uifolder);

        ParseMetadata(metadata, _tokens->uimin, element);
        ParseMetadata(metadata, _tokens->uimax, element);
        ParseMetadata(metadata, _tokens->uisoftmin, element);
        ParseMetadata(metadata, _tokens->uisoftmax, element);
        ParseMetadata(metadata, _tokens->uistep, element);
        ParseMetadata(metadata, _tokens->unit, element);
        ParseMetadata(metadata, _tokens->unittype, element);
        ParseMetadata(metadata, _tokens->defaultgeomprop, element);

        if (!metadata.count(SdrPropertyMetadata->Help) &&
             metadata.count(_tokens->unit)) {
            // The unit can be helpful if there is no documentation.
            metadata.emplace(SdrPropertyMetadata->Help,
                             TfStringPrintf("Unit is %s.",
                                            metadata[_tokens->unit].c_str()));
        }

        for (const auto& pair : metadata) {
            const TfToken attrName = pair.first;
            const std::string attrValue = pair.second;

            const auto& allTokens = SdrPropertyMetadata->allTokens;
            if (std::find(allTokens.begin(), allTokens.end(), attrName) !=
                allTokens.end()) {
                continue;
            }

            // Attribute hasn't been handled yet, so put it into the hints dict.
            hints.insert({attrName, attrValue});
        }

        ParseOptions(options, element);
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
        // Change the 'texture2d' role for stdlib MaterialX Texture nodes
        // to 'texture' for Sdr.
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
    ParseMetadata(builder, SdrNodeMetadata->Category, nodeDef, _tokens->nodecategory);
    ParseMetadata(builder, SdrNodeMetadata->Help, nodeDef, _tokens->doc);
    ParseMetadata(builder, SdrNodeMetadata->Target, nodeDef, _tokens->target);
    ParseMetadata(builder, SdrNodeMetadata->Role, nodeDef, _tokens->nodegroup);

    // XXX -- version

    NdrStringVec primvars;

    // If the nodeDef name starts with ND_geompropvalue, it's a primvar reader
    // node and we want to add $geomprop to the list of referenced primvars.
    if (TfStringStartsWith(nodeDef->getName(), "ND_geompropvalue")) {
        primvars.push_back("$geomprop");
    }
    // If the nodeDef name is ND_texcoord_vector2, it is using texture 
    // coordinates and we want to add the default texturecoordinate name 
    // to the list of referenced primvars.
    if (nodeDef->getName() == "ND_texcoord_vector2") {
        primvars.push_back(_GetPrimaryUvSetName());
    }
    // For custom nodes that use textures or texcoords, look through the
    // implementation nodegraph to find the texcoord, geompropvalue, 
    // or stdlib image/tiledimage node and add the appropriate primvar to  
    // the list of referenced primvars.
    bool addedTexcoordPrimvar = false;
    const mx::InterfaceElementPtr& impl = nodeDef->getImplementation();
    if (impl && impl->isA<mx::NodeGraph>()) {
        // Add primvar name for geompropvalue nodes.
        // XXX Using '$geomprop' here does not get replaced with the 
        // appropriate primvar name. 
        const mx::NodeGraphPtr ng = impl->asA<mx::NodeGraph>();
        for (const mx::NodePtr& geompropNode : ng->getNodes("geompropvalue")) {
            if (const mx::InputPtr& input = geompropNode->getInput("geomprop")) {
                primvars.push_back(input->getValueString());

                // Assume a texture coordinate primvar if of vector2 type.
                if (geompropNode->getType() == "vector2") {
                    addedTexcoordPrimvar = true;
                }
            }
        }
        // Add the default texturecoordinate name for texcoord nodes.
        if (ng->getNodes("texcoord").size() != 0) {
            primvars.push_back(_GetPrimaryUvSetName());
            addedTexcoordPrimvar = true;
        }
        // Add the default texture coordinate name with an image/tiledimage 
        // node if we have not yet added a texcoordPrimvar name. 
        if (!addedTexcoordPrimvar) {
            if (ng->getNodes("tiledimage").size() != 0 ||
                ng->getNodes("image").size() != 0) {
                primvars.push_back(_GetPrimaryUvSetName());
                addedTexcoordPrimvar = true;
            }
        }
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
            if (name == _tokens->UV0) {
                name = _GetPrimaryUvSetName();
            }
        }
        primvars.insert(primvars.end(), split.begin(), split.end());
    }

    // Properties
    for (const auto& mtlxInput: nodeDef->getActiveInputs()) {
        builder->AddProperty(mtlxInput, false, &primvars, addedTexcoordPrimvar);
    }

    for (const auto& mtlxOutput: nodeDef->getActiveOutputs()) {
        builder->AddProperty(mtlxOutput, true, nullptr);
    }

    builder->metadata[SdrNodeMetadata->Primvars] =
        TfStringJoin(primvars.begin(), primvars.end(), "|");
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
UsdMtlxParserPlugin::Parse(const NdrNodeDiscoveryResult& discoveryResult)
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
        document = UsdMtlxGetDocumentFromString(discoveryResult.sourceCode);
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

    auto nodeDef = document->getNodeDef(discoveryResult.identifier);
    if (!nodeDef) {
        TF_WARN("Invalid MaterialX NodeDef; unknown node name ' %s '",
            discoveryResult.identifier.GetText());
        return GetInvalidNode(discoveryResult);
    }

    ShaderBuilder builder(discoveryResult);
    ParseElement(&builder, nodeDef);

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
