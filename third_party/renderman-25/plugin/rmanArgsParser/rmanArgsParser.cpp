//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/arch/fileSystem.h"
#include "pxr/base/arch/library.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/gf/vec4f.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/array.h"
#include "pxr/usd/ar/ar.h"
#include "pxr/usd/ar/asset.h"
#include "pxr/usd/ar/resolvedPath.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ndr/debugCodes.h"
#include "pxr/usd/ndr/nodeDiscoveryResult.h"
#include "pxr/usd/sdf/assetPath.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdr/shaderMetadataHelpers.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "rmanArgsParser/rmanArgsParser.h"
#include "rmanArgsParser/pugixml/pugixml.hpp"

using namespace pugi;

PXR_NAMESPACE_OPEN_SCOPE

using ShaderMetadataHelpers::CreateStringFromStringVec;
using ShaderMetadataHelpers::IsPropertyAnAssetIdentifier;
using ShaderMetadataHelpers::IsPropertyATerminal;
using ShaderMetadataHelpers::IsTruthy;
using ShaderMetadataHelpers::OptionVecVal;

NDR_REGISTER_PARSER_PLUGIN(RmanArgsParserPlugin)

namespace {
    // Pre-constructed xml char strings to make things easier to read
    const char* nameStr = "name";
    const char* paramStr = "param";
    const char* outputStr = "output";
    const char* helpStr = "help";
    const char* hintdictStr = "hintdict";
    const char* hintlistStr = "hintlist";
    const char* optionsStr = "options";
    const char* valueStr = "value";
    const char* tagStr = "tag";
    const char* tagsStr = "tags";
    const char* pageStr = "page";
    const char* primvarsStr = "primvars";
    const char* departmentsStr = "departments";
    const char* shaderTypeStr = "shaderType";
    const char* typeTagStr = "typeTag";
    const char* usdSchemaDefStr = "usdSchemaDef";
    const char* apiSchemaAutoApplyToStr = "apiSchemaAutoApplyTo";
    const char* apiSchemaCanOnlyApplyToStr = "apiSchemaCanOnlyApplyTo";
    const char* apiSchemasForAttrPruningStr = "apiSchemasForAttrPruning";
    const char* sdrGlobalConfigStr = "sdrGlobalConfig";

    // Helper to make comparisons of `const char*` easier to read; there are
    // lots of these comparisons
    inline bool EQUALS(const char* x, const char* y) {
        return strcmp(x, y) == 0;
    }
}

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((discoveryType, "args"))
    ((sourceType, "RmanCpp"))
    ((bxdfType, "bxdf"))
);

// XML attribute names (as they come from the args file). Many attributes are
// named exactly like the metadata on the node/property, and are not included
// here because the node and property classes have their own tokens for these.
TF_DEFINE_PRIVATE_TOKENS(
    _xmlAttributeNames,

    ((nameAttr, "name"))
    ((typeAttr, "type"))
    ((arraySizeAttr, "arraySize"))
    ((defaultAttr, "default"))
    ((inputAttr, "input"))
    ((tagAttr, "tag"))
    ((vstructmemberAttr, "vstructmember"))
    ((sdrDefinitionNameAttr, "sdrDefinitionName"))
);

// Data that represents an SdrShaderNode before it is turned into one. The
// args file parsing happens recursively, and this is used to pass around a
// shader node being incrementally constructed.
struct SdrShaderRepresentation
{
    SdrShaderRepresentation(const NdrNodeDiscoveryResult& discoveryResult)
        : hasSdrDefinitionNameProperty(false), name(discoveryResult.name) {}

    bool hasSdrDefinitionNameProperty;
    std::string name;
    std::string helpText;
    TfToken sdrDefinitionNameFallbackPrefix;
    NdrStringVec primvars;
    NdrStringVec departments;
    NdrStringVec pages;
    NdrPropertyUniquePtrVec properties;
    NdrTokenMap metadata;

    // This is the type that the shader declares itself as; this is NOT the
    // source type
    TfToken type = SdrPropertyTypes->Unknown;
};

// Gets a common type + array size (if array) from the property attributes
static std::tuple<TfToken, size_t> _GetTypeName(
    const NdrTokenMap& attributes);

// Gets a VtValue instance from a string value and a type, array, metadata
// hint
static VtValue _GetVtValue(
    const std::string& stringValue,
    TfToken& type,
    size_t arraySize,
    const NdrTokenMap& metadata);

// Emits a deprecation warning to the log
static void _OutputDeprecationWarning(
    const TfToken& attrName, const SdrShaderRepresentation& shaderRep,
    const TfToken& propName);

// Parses the given child's data as a preparation to be handed off to
// `_CreateProperty` to actually be created
static SdrShaderPropertyUniquePtr _ParseChildElem(
    const SdrShaderRepresentation& shaderRep, bool isOutput,
    pugi::xml_node childElement, const std::string& parentPage);

// Creates a SdrShaderProperty with the given inputs
static SdrShaderPropertyUniquePtr _CreateProperty(
    const SdrShaderRepresentation& entry, bool isOutput,
    NdrTokenMap& attributes, NdrStringVec& validConnectionTypes,
    NdrOptionVec& options);

// Injects any metadata that is generated by the parser. This type of
// metadata provides additional hints to the property.
static void _InjectParserMetadata(NdrTokenMap& metadata,
    const TfToken& typeName);

// Recursively builds up the shaderRep as the shader is parsed
static void _Parse(SdrShaderRepresentation& shaderRep, pugi::xml_node parent,
    const std::string& parentPage);

// For each child under the specified parent, get the value of the
// specified attribute
static NdrStringVec _GetAttributeValuesFromChildren(
    pugi::xml_node parent, const char* attrName);

// Helper methods to get a value out of the attr map (or the default value
// if the key doesn't exist on the map)
static TfToken _Get(const NdrTokenMap& map, const TfToken& key,
    TfToken defaultValue);

static int _Get(const NdrTokenMap& map, const TfToken& key,
    int defaultValue);

static std::string _GetDsoPathFromArgsPath(const std::string &argsPath);

RmanArgsParserPlugin::RmanArgsParserPlugin()
{
    // Nothing yet
}

RmanArgsParserPlugin::~RmanArgsParserPlugin()
{
    // Nothing yet
}

const NdrTokenVec& 
RmanArgsParserPlugin::GetDiscoveryTypes() const 
{
    static const NdrTokenVec discoveryTypes = {_tokens->discoveryType};
    return discoveryTypes;
}

const TfToken& 
RmanArgsParserPlugin::GetSourceType() const
{
    return _tokens->sourceType;
}

static
bool
_LoadXmlDoc(const NdrNodeDiscoveryResult& discoveryResult,
            xml_document *doc)
{
    if (!discoveryResult.resolvedUri.empty()) {
        std::shared_ptr<const char> buffer;
        std::shared_ptr<ArAsset> asset = ArGetResolver().OpenAsset(
            ArResolvedPath(discoveryResult.resolvedUri));
        if (asset) {
            buffer = asset->GetBuffer();
        }

        if (!buffer) {
            TF_WARN("Could not open the args file at URI [%s] (%s). "
                    "An invalid Sdr node definition will be created.",
                    discoveryResult.uri.c_str(),
                    discoveryResult.resolvedUri.c_str());
                
            return false;
        }
            
        const xml_parse_result result =
            doc->load_buffer(buffer.get(), asset->GetSize());

        if (!result) {
            TF_WARN("Could not parse args file at URI [%s] because the file "
                    "could not be opened or was malformed. An invalid Sdr node "
                    "definition will be created. (Error: %s)",
                    discoveryResult.uri.c_str(),
                    result.description());

            return false; 
        }

    } else if (!discoveryResult.sourceCode.empty()) {
        const xml_parse_result result = 
            doc->load_string(discoveryResult.sourceCode.c_str());

        if (!result) {
            TF_WARN("Could not parse given source code for node with identifier"
                    "'%s' because it was malformed. An invalid Sdr node "
                    "definition will be created. (Error: %s)", 
                    discoveryResult.identifier.GetText(),
                    result.description());
            return false;
        }

    } else {
        TF_WARN("Invalid NdrNodeDiscoveryResult with identifier '%s': both "
            "resolvedUri and sourceCode fields are empty.", 
            discoveryResult.identifier.GetText());
        return false;
    }

    return true;
}

// Typically we can use the shaderType as the SdrShaderNode's context, but
// for these filter types the rman all lower case convention doesn't match
// our standard camel-case convention. We map these shader types to the correct
// Sdr context token here.
static TfToken 
_GetSdrContextFromShaderType(const TfToken &shaderType)
{
    static const std::unordered_map<TfToken, TfToken, TfHash> contextMapping({
        { TfToken("displayfilter"), SdrNodeContext->DisplayFilter },
        { TfToken("lightfilter"), SdrNodeContext->LightFilter },
        { TfToken("pixelfilter"), SdrNodeContext->PixelFilter },
        { TfToken("samplefilter"), SdrNodeContext->SampleFilter }
    });

    const auto it = contextMapping.find(shaderType);
    return it == contextMapping.end() ? shaderType : it->second;
}

NdrNodeUniquePtr
RmanArgsParserPlugin::Parse(const NdrNodeDiscoveryResult& discoveryResult)
{
    xml_document doc;

    if (!_LoadXmlDoc(discoveryResult, &doc)) {
        return NdrParserPlugin::GetInvalidNode(discoveryResult);
    }

    xml_node rootElem = doc.first_child();
    SdrShaderRepresentation shaderRepresentation(discoveryResult);

    //
    // Parse the node
    //
    _Parse(shaderRepresentation, rootElem, /* page = */ "");

    NdrTokenMap metadata = discoveryResult.metadata;
    metadata.insert(shaderRepresentation.metadata.begin(), 
            shaderRepresentation.metadata.end());

    if (!shaderRepresentation.departments.empty()) {
        metadata[SdrNodeMetadata->Departments] = 
            CreateStringFromStringVec(shaderRepresentation.departments);
    }

    if (!shaderRepresentation.pages.empty()) {
        metadata[SdrNodeMetadata->Pages] = 
            CreateStringFromStringVec(shaderRepresentation.pages);
    }

    if (!shaderRepresentation.primvars.empty()) {
        metadata[SdrNodeMetadata->Primvars] = 
            CreateStringFromStringVec(shaderRepresentation.primvars);
    }

    if (!shaderRepresentation.helpText.empty()) {
        metadata[SdrNodeMetadata->Help] = shaderRepresentation.helpText;
    }

    return SdrShaderNodeUniquePtr(
        new SdrShaderNode(
            discoveryResult.identifier,
            discoveryResult.version,
            shaderRepresentation.name,
            discoveryResult.family,
            _GetSdrContextFromShaderType(shaderRepresentation.type),
            _tokens->sourceType,
            discoveryResult.resolvedUri,
            _GetDsoPathFromArgsPath(discoveryResult.resolvedUri),
            std::move(shaderRepresentation.properties),
            metadata,
            discoveryResult.sourceCode)
    );
}

static std::string
_GetDsoPathFromArgsPath(const std::string &argsPath)
{
    // We assume:
    // - both the args file at argsPath and the .so it describes are 
    //   filesystem accessible
    // -  Given: /path/to/plugins/Args/somePlugin.args ,
    //    we will locate its dso as:
    //    /path/to/plugins/somePlugin.so
    
    const std::string argsExt(".args");
    const std::string dsoExt(ARCH_PLUGIN_SUFFIX);
    
    std::vector<std::string> pathElts = TfStringSplit(TfNormPath(argsPath), "/");

    if (pathElts.size() < 3 || 
        !TfStringEndsWith(argsPath, argsExt) ||
        pathElts[pathElts.size()-2] != "Args") {

        if (pathElts.size() > 1 && pathElts[pathElts.size()-2] != "Args") {
            TF_DEBUG(NDR_PARSING).Msg("Args file being parsed does not "
                "live in \"Args\" named parent directory. No "
                "implementation will be set for the sdr node.\n");
            return std::string();
        }

        TF_WARN("Unexpected path for RenderMan args file: %s - "
                "expected a form like /path/to/plugins/Args/somePlugin.args",
                argsPath.c_str());
        return std::string();
    }
    
    std::string  pluginFileName = TfStringReplace(pathElts.back(),
                                                  argsExt,
                                                  dsoExt);
    pathElts.pop_back();
    pathElts.back() = pluginFileName;
    
    return TfStringJoin(pathElts, ARCH_PATH_SEP);
}

static SdrShaderPropertyUniquePtr
_ParseChildElem(
    const SdrShaderRepresentation& shaderRep, bool isOutput,
    xml_node childElement, const std::string& parentPage)
{
    // The bits of data that will later be turned into the shader property
    NdrTokenMap attributes;
    NdrOptionVec options;
    NdrStringVec validConnectionTypes;

    xml_attribute attribute = childElement.first_attribute();

    // Extract all XML properties that exist on this element into the
    // attributes map. This general collection of attributes will be translated
    // into data on the SdrShaderProperty at the end of the parse process.
    // -------------------------------------------------------------------------
    while (attribute) {
        attributes.emplace(
            TfToken(attribute.name()),
            attribute.value()
        );

        attribute = attribute.next_attribute();
    }

    // If page wasn't found in the attributes, use the parent page that was
    // found via a <page> element
    if (!parentPage.empty()) {
        attributes.insert({
            SdrPropertyMetadata->Page,
            parentPage
        });
    }

    // The properties in the element have been extracted. The next step is
    // iterating over all of the sub elements to extract more attributes
    // and/or data (some values need to be extracted into specialized variables
    // because they are not strings).
    // -------------------------------------------------------------------------
    xml_node attrChild = childElement.first_child();
    while (attrChild) {
        // Help text
        // -------------------
        if (EQUALS(helpStr, attrChild.name())) {
            // The help element's value might contain HTML, and the HTML should
            // be included in the value of the help text. Getting the element's
            // value will cut off anything after the first HTML tag, so instead
            // capture the raw value of the element via "print". "print" has the
            // downside that the <help> and </help> tags are included in the
            // value, so those need to be manually removed. This is a bit of a
            // sloppy solution, but getting the raw value of the element with
            // the HTML intact seems to be quite difficult with pugixml. Note
            // that the "format_no_escapes" option is given so that pugixml does
            // not change, for example, ">" into "&gt;".
            std::ostringstream helpStream;
            attrChild.print(helpStream, /*indent=*/"\t",
                pugi::format_default | pugi::format_no_escapes);
            std::string helpText = TfStringTrim(helpStream.str());

            // Not using TfStringReplace() here -- which replaces all
            // occurrences -- since it _is_ possible that someone decides to
            // include a <help> tag in the help text itself
            if (TfStringStartsWith(helpText, "<help>")) {
                helpText = helpText.substr(6);
            }

            if (TfStringEndsWith(helpText, "</help>")) {
                helpText = helpText.substr(0, helpText.size() - 7);
            }

            attributes[TfToken(helpStr)] = helpText.c_str();
        }

        // Hint dictionary
        // -------------------
        else if (EQUALS(hintdictStr, attrChild.name())) {
            xml_attribute nameAttr = attrChild.attribute(nameStr);

            if (EQUALS(optionsStr, nameAttr.value())) {
                xml_node optChild = attrChild.first_child();

                while (optChild) {
                    const TfToken name(optChild.attribute(nameStr).value());
                    const TfToken value(optChild.attribute(valueStr).value());
                    options.emplace_back(std::make_pair(name, value));

                    optChild = optChild.next_sibling();
                }
            }
        }

        // Hint list
        // -------------------
        else if (EQUALS(hintlistStr, attrChild.name())) {
            xml_attribute nameAttr = attrChild.attribute(nameStr);

            if (EQUALS(optionsStr, nameAttr.value())) {
                xml_node optChild = attrChild.first_child();

                while (optChild) {
                    const TfToken value(optChild.attribute(valueStr).value());
                    options.emplace_back(std::make_pair(value, TfToken()));

                    optChild = optChild.next_sibling();
                }
            }
        }

        // Tags
        // -------------------
        else if (EQUALS(tagsStr, attrChild.name())) {
            NdrStringVec connTypes =
                _GetAttributeValuesFromChildren(attrChild, "value");

            for (const auto& connType : connTypes) {
                validConnectionTypes.push_back(connType);
            }
        }

        attrChild = attrChild.next_sibling();
    } // end while


    // Conform connection types into the standard string-based format that can
    // be consumed by the shader node
    // -------------------------------------------------------------------------
    bool hasTagAttr = attributes.count(_xmlAttributeNames->tagAttr);
    if (validConnectionTypes.size() || hasTagAttr) {
        // Merge the tag attr into valid connection types
        if (hasTagAttr) {
            validConnectionTypes.push_back(
                attributes.at(_xmlAttributeNames->tagAttr));
        }

        attributes.emplace(
            SdrPropertyMetadata->ValidConnectionTypes,
            CreateStringFromStringVec(validConnectionTypes)
        );
    }


    // Manipulate the attributes as needed
    // -------------------------------------------------------------------------
    if (attributes.count(SdrPropertyMetadata->Options)) {
        // Extract any options that were specified as attributes into the
        // options vector, and remove from the attributes

        NdrOptionVec opts = OptionVecVal(
            attributes.at(SdrPropertyMetadata->Options)
        );

        for (const auto& opt : opts) {
            options.push_back(opt);
        }

        attributes.erase(SdrPropertyMetadata->Options);
    }


    // Sub elements have been processed. If a type doesn't exist at this point,
    // make a last-ditch effort to determine what it is.
    // -------------------------------------------------------------------------
    if (attributes.count(_xmlAttributeNames->typeAttr) == 0) {
        // Try to infer it from the valid connection types
        if (validConnectionTypes.size() > 0) {
            // Use the first valid type only
            attributes.emplace(
                _xmlAttributeNames->typeAttr,
                validConnectionTypes[0]
            );
        }
    }

    return _CreateProperty(
        shaderRep, isOutput, attributes, validConnectionTypes, options
    );
}

static void
_Parse(
    SdrShaderRepresentation& shaderRep, xml_node parent,
    const std::string& parentPage)
{
    xml_node childElement = parent.first_child();

    // Iterate over all children elements
    // -------------------------------------------------------------------------
    while (childElement) {
        bool isInput = EQUALS(paramStr, childElement.name());
        bool isOutput = EQUALS(outputStr, childElement.name());

        // Handle input/output elements first. They can have sub-elements that
        // must be accounted for as well. Inputs and outputs at this level do
        // not belong to a page.
        // <param> and <output>
        // ---------------------------------------------------------------------
        if (isInput || isOutput) {
            shaderRep.properties.emplace_back(
                _ParseChildElem(shaderRep, isOutput, childElement, parentPage)
            );
            // Query the emplaced property to see if ImplementationName metadata
            // is added, which would imply sdrDefinitionName was set for this
            // property, if so save the information in shaderRep. 
            const NdrTokenMap& propMetadata = 
                shaderRep.properties.back()->GetMetadata();
            if (propMetadata.find(SdrPropertyMetadata->ImplementationName) != 
                    propMetadata.end()) {
                // We do this here so as to maintain constness for shaderRep in
                // _ParseChildElem
                shaderRep.hasSdrDefinitionNameProperty = true;
            }
        }

        // SdrGlobalConfig
        // <sdrGlobalConfig> with named sdr node metadata, example
        // <sdrDefinitionNameFallbackPrefix>, etc
        // Note that its assumed that such config will be placed in the 
        // beginning of an args file. And if this element is found after any
        // parameter which specified an sdrDefinitionName then we will issue an
        // error, reporting malformed args file.
        else if (EQUALS(sdrGlobalConfigStr, childElement.name())) {
            if (shaderRep.hasSdrDefinitionNameProperty) {
                TF_CODING_ERROR("Malformed args file: "
                        "sdrDefinitionNameFallbackPrefix needs to be specified"
                        "before any parameter's sdrDefinitionName is "
                        "specified/parsed.");
                return;
            }
            xml_node attrChild = childElement.first_child();
            while (attrChild) {
                xml_attribute attrValue = attrChild.attribute(valueStr);
                shaderRep.metadata.emplace(TfToken(attrChild.name()),
                        attrValue.value());
                if (EQUALS(SdrNodeMetadata->SdrDefinitionNameFallbackPrefix
                            .GetText(), attrChild.name())) {
                    // Cache sdrDefinitionNameFallbackPrefix token, instead of
                    // creating a token from sdrNodeMetafdata everytime
                    // sdrDefinitionNameFallbackPrefix gets used.
                    shaderRep.sdrDefinitionNameFallbackPrefix = 
                        TfToken(attrValue.value());
                }
                attrChild = attrChild.next_sibling();
            }
        }

        // Page
        // <page name="...">
        // Pages have inputs (<param> elements) as children; pages can also
        // have more <page> elements as children
        // ---------------------------------------------------------------------
        else if (EQUALS(pageStr, childElement.name())) {
            const std::string pageName(childElement.attribute(nameStr).value());

            if (parentPage.empty()) {
                _Parse(shaderRep, childElement, pageName);
            } else {
                _Parse(shaderRep, childElement, 
                        parentPage + 
                        SdrPropertyTokens->PageDelimiter.GetString() + pageName);
            }
        }

        // Help
        // <help>
        // ---------------------------------------------------------------------
        else if (EQUALS(helpStr, childElement.name())) {
            const char* helpText = childElement.child_value();
            shaderRep.helpText = helpText;
        }

        // Primvars
        // <primvars> with <primvar name="..."> children
        // ---------------------------------------------------------------------
        else if (EQUALS(primvarsStr, childElement.name())) {
            shaderRep.primvars =
                _GetAttributeValuesFromChildren(childElement, "name");
        }

        // Departments
        // <departments> with <department name="..."> children
        // ---------------------------------------------------------------------
        else if (EQUALS(departmentsStr, childElement.name())) {
            shaderRep.departments =
                _GetAttributeValuesFromChildren(childElement, "name");
        }

        // Shader type
        // <shaderType name="..."> OR
        // <shaderType> with <tag value="..."> children
        // ---------------------------------------------------------------------
        else if (EQUALS(shaderTypeStr, childElement.name())) {
            xml_attribute nameAttr = childElement.attribute(nameStr);

            if (nameAttr) {
                shaderRep.type = TfToken(nameAttr.value());
            } else {
                xml_node attrChild = childElement.first_child();

                if (attrChild && EQUALS(tagStr, attrChild.name())) {
                    nameAttr = attrChild.attribute(valueStr);
                    shaderRep.type = TfToken(nameAttr.value());
                }
            }
        }

        // Type tag
        // <typeTag> with <tag value="..."> children
        // ---------------------------------------------------------------------
        else if (EQUALS(typeTagStr, childElement.name())) {
            xml_node attrChild = childElement.first_child();

            if (attrChild && EQUALS(tagStr, attrChild.name())) {
                xml_attribute attr = attrChild.attribute(valueStr);
                shaderRep.type = TfToken(attr.value());

                TF_DEBUG(NDR_PARSING).Msg(
                    "Deprecated 'typeTag' on shader [%s]", 
                    shaderRep.name.c_str());
            }

        }


        // UsdSchemaDef
        // <usdSchemaDef> with named metadata, example
        // <schemaName>, <schemaKind>, <schemaBase>, <apiSchemaAutoApplyTo>
        // <apiSchemaAutoApplyTo> is a list of autoApplyTo names (schemas).
        // <apiSchemaCanOnlyApplyToStr> is a list of autoApplyTo names (schemas).
        // <apiSchemasForAttrPruning> is a list of apiSchema names (schemas).
        else if (EQUALS(usdSchemaDefStr, childElement.name())) {
            xml_node attrChild = childElement.first_child();
            while(attrChild) {
                if (EQUALS(apiSchemaAutoApplyToStr, attrChild.name())) {
                    NdrStringVec apiSchemas = 
                        _GetAttributeValuesFromChildren(attrChild, "name");
                    shaderRep.metadata.emplace(
                            TfToken(apiSchemaAutoApplyToStr),
                            CreateStringFromStringVec(apiSchemas));
                } else if (
                        EQUALS(apiSchemaCanOnlyApplyToStr, attrChild.name())) {
                    NdrStringVec apiSchemas =
                        _GetAttributeValuesFromChildren(attrChild, "name");
                    shaderRep.metadata.emplace(
                            TfToken(apiSchemaCanOnlyApplyToStr),
                            CreateStringFromStringVec(apiSchemas));
                } else if (
                        EQUALS(apiSchemasForAttrPruningStr, attrChild.name())) {
                    NdrStringVec apiSchemas =
                        _GetAttributeValuesFromChildren(attrChild, "name");
                    shaderRep.metadata.emplace(
                            TfToken(apiSchemasForAttrPruningStr),
                            CreateStringFromStringVec(apiSchemas));
                } else {
                    xml_attribute attrValue = attrChild.attribute(valueStr);
                    shaderRep.metadata.emplace(TfToken(attrChild.name()),
                            attrValue.value());
                }
                attrChild = attrChild.next_sibling();
            }
        }

        childElement = childElement.next_sibling();
    }
}

static std::tuple<TfToken, size_t>
_GetTypeName(
    const NdrTokenMap& attributes)
{
    // Determine arraySize
    // -------------------------------------------------------------------------
    size_t arraySize = _Get(attributes, _xmlAttributeNames->arraySizeAttr, 0);

    // Determine type
    // -------------------------------------------------------------------------
    TfToken typeName =
        _Get(attributes, _xmlAttributeNames->typeAttr, TfToken());

    // 'bxdf' typed attributes are cast to the terminal type of the Sdr library
    if (typeName == _tokens->bxdfType) {
        typeName = SdrPropertyTypes->Terminal;
    }
    // If the attributes indicates the property is a terminal, then the property
    // should be SdrPropertyTypes->Terminal
    else if (IsPropertyATerminal(attributes)) {
        typeName = SdrPropertyTypes->Terminal;
    }

    return std::make_tuple(typeName, arraySize);
}

static VtValue
_GetVtValue(
    const std::string& stringValue,
    TfToken& type,
    size_t arraySize,
    const NdrTokenMap& metadata)
{
    // Determine array-ness
    // -------------------------------------------------------------------------
    int isDynamicArray =
        IsTruthy(SdrPropertyMetadata->IsDynamicArray, metadata);
    bool isArray = (arraySize > 0) || isDynamicArray;

    // INT and INT ARRAY
    // -------------------------------------------------------------------------
    if (type == SdrPropertyTypes->Int) {
        if (!isArray) {
            // If the conversion fails, we get zero
            return VtValue(atoi(stringValue.c_str()));
        } else {
            NdrStringVec parts = TfStringTokenize(stringValue, " ,");
            int numValues = parts.size();
            VtIntArray ints(numValues);

            for (int i = 0; i < numValues; ++i) {
                ints[i] = atoi(parts[i].c_str());
            }

            return VtValue::Take(ints);
        }
    }

    // STRING and STRING ARRAY
    // -------------------------------------------------------------------------
    else if (type == SdrPropertyTypes->String) {
        // Handle non-array
        if (!isArray) {
            return VtValue(stringValue);
        } else {
            // Handle array
            VtStringArray array;
            std::vector<std::string> tokens =
                TfStringTokenize(stringValue, " ,");
            array.reserve(tokens.size());

            for (const std::string& token : tokens) {
                array.push_back(token);
            }

            return VtValue::Take(array);
        }
    }

    // FLOAT and FLOAT ARRAY
    // -------------------------------------------------------------------------
    else if (type == SdrPropertyTypes->Float) {
        if (!isArray) {
            // If the conversion fails, we get zero
            return VtValue(static_cast<float>(atof(stringValue.c_str())));
        } else {
            NdrStringVec parts = TfStringTokenize(stringValue, " ,");
            int numValues = parts.size();

            VtFloatArray floats(numValues);
            for (int i = 0; i < numValues; ++i) {
                floats[i] = static_cast<float>(atof(parts[i].c_str()));
            }

            return VtValue::Take(floats);
        }
    }

    // VECTOR TYPES and VECTOR TYPE ARRAYS
    // -------------------------------------------------------------------------
    else if (type == SdrPropertyTypes->Color  ||
             type == SdrPropertyTypes->Point  ||
             type == SdrPropertyTypes->Normal ||
             type == SdrPropertyTypes->Vector) {

        NdrStringVec parts = TfStringTokenize(stringValue, " ,");

        if (!isArray) {
            if (parts.size() == 3) {
                return VtValue(
                    GfVec3f(atof(parts[0].c_str()),
                            atof(parts[1].c_str()),
                            atof(parts[2].c_str()))
                );
            } else {
                TF_DEBUG(NDR_PARSING).Msg(
                    "float3 default value [%s] has %zd values; should "
                    "have three.", stringValue.c_str(), parts.size());

                return VtValue(GfVec3f(0.0, 0.0, 0.0));
            }
        } else if (isArray && parts.size() % 3 == 0) {
            int numElements = parts.size() / 3;
            VtVec3fArray array(numElements);

            for (int i = 0; i < numElements; ++i) {
                array[i] = GfVec3f(atof(parts[3*i + 0].c_str()),
                                   atof(parts[3*i + 1].c_str()),
                                   atof(parts[3*i + 2].c_str()));
            }

            return VtValue::Take(array);
        }
    }

    // MATRIX
    // -------------------------------------------------------------------------
    else if (type == SdrPropertyTypes->Matrix) {
        NdrStringVec parts = TfStringTokenize(stringValue, " ,");

        // XXX no matrix array support
        if (!isArray && parts.size() == 16) {
            GfMatrix4d mat;
            double* values = mat.GetArray();

            for (int i = 0; i < 16; ++i) {
                values[i] = atof(parts[i].c_str());
            }

            return VtValue::Take(mat);
        }

    }

    // STRUCT, TERMINAL, VSTRUCT
    // -------------------------------------------------------------------------
    else if (type == SdrPropertyTypes->Struct ||
             type == SdrPropertyTypes->Terminal ||
             type == SdrPropertyTypes->Vstruct) {
        // We return an empty VtValue for Struct, Terminal, and Vstruct
        // properties because their value may rely on being computed within the
        // renderer, or we might not have a reasonable way to represent their
        // value within Sdr
        return VtValue();
    }

    // Didn't find a supported type
    return VtValue();
}

static void
_OutputDeprecationWarning(
    const TfToken& attrName, const SdrShaderRepresentation& shaderRep,
    const TfToken& propName)
{
    TF_DEBUG(NDR_PARSING).Msg(
        "Deprecated '%s' attribute on shader [%s] on property [%s]",
        attrName.GetText(), shaderRep.name.c_str(), propName.GetText());
}

static SdrShaderPropertyUniquePtr
_CreateProperty(
    const SdrShaderRepresentation& shaderRep, bool isOutput,
    NdrTokenMap& attributes, NdrStringVec& validConnectionTypes,
    NdrOptionVec& options)
{
    TfToken propName =
        _Get(attributes,
             _xmlAttributeNames->nameAttr,
             TfToken("NAME UNSPECIFIED"));
    TfToken definitionName;

    // Get type name, and determine the size of the array (if an array)
    TfToken typeName;
    size_t arraySize;
    std::tie(typeName, arraySize) = _GetTypeName(attributes);

    if (typeName.IsEmpty()) {
        typeName = SdrPropertyTypes->Unknown;

        TF_DEBUG(NDR_PARSING).Msg(
            "Property [%s] doesn't have a valid type. "
            "Neither an explicit type nor a validConnectionType was specified.",
            propName.GetText());
    } else {
        if (isOutput) {
            _OutputDeprecationWarning(
                _xmlAttributeNames->typeAttr, shaderRep, propName);
        }
    }
    
    // The 'tag' attr is deprecated
    // -------------------------------------------------------------------------
    if (attributes.count(_xmlAttributeNames->tagAttr)) {
        _OutputDeprecationWarning(
            _xmlAttributeNames->tagAttr, shaderRep, propName);

        // Rename to 'validConnectionTypes'
        attributes.insert({
            SdrPropertyMetadata->ValidConnectionTypes,
            attributes.at(_xmlAttributeNames->tagAttr)
        });
        attributes.erase(_xmlAttributeNames->tagAttr);
    }


    // More deprecation warnings
    // -------------------------------------------------------------------------
    if (attributes.count(_xmlAttributeNames->inputAttr)) {
        // Just output a warning here; it will be inserted into the hints map
        // later on
        _OutputDeprecationWarning(
            _xmlAttributeNames->inputAttr, shaderRep, propName);
    }


    // Handle vstruct information
    // -------------------------------------------------------------------------
    if (attributes.count(_xmlAttributeNames->vstructmemberAttr)) {
        std::string vstructMember =
            attributes.at(_xmlAttributeNames->vstructmemberAttr);

        if (!vstructMember.empty()) {
            // Find the dot that splits struct from member name
            size_t dotPos = vstructMember.find('.');

            if (dotPos != std::string::npos) {
                // Add member of to attributes
                attributes.insert({
                    SdrPropertyMetadata->VstructMemberOf,
                    vstructMember.substr(0, dotPos)
                });

                // Add member name to attributes
                attributes.insert({
                    SdrPropertyMetadata->VstructMemberName,
                    vstructMember.substr(dotPos + 1)
                });
            } else {
                TF_DEBUG(NDR_PARSING).Msg(
                    "Bad virtual structure member in %s.%s:%s",
                    shaderRep.name.c_str(), propName.GetText(),
                    vstructMember.c_str());
            }
        }
    }

    // Handle definitionName, which requires changing propName
    // -------------------------------------------------------------------------
    if (attributes.count(_xmlAttributeNames->sdrDefinitionNameAttr)) {
        TfToken definitionName =
            TfToken(attributes.at(_xmlAttributeNames->sdrDefinitionNameAttr));
        
        attributes[SdrPropertyMetadata->ImplementationName] = propName;
        propName = definitionName;
        attributes.erase(_xmlAttributeNames->sdrDefinitionNameAttr);
    } else if (!shaderRep.sdrDefinitionNameFallbackPrefix.IsEmpty()) {
        // Args files author should have placed such sdr node metadata before
        // any sdr shader parameter. Hence we should have this metadata parsed
        // and available to be used here.
        TfToken definitionName = TfToken(SdfPath::JoinIdentifier(
                    shaderRep.sdrDefinitionNameFallbackPrefix, propName));
        attributes[SdrPropertyMetadata->ImplementationName] = propName;
        propName = definitionName;
    }
 

    // Put any uncategorized attributes into hints
    // -------------------------------------------------------------------------
    NdrTokenMap hints;
    for (const auto& pair : attributes) {
        const TfToken attrName = pair.first;
        const std::string attrValue = pair.second;

        if (std::find(SdrPropertyMetadata->allTokens.begin(),
                      SdrPropertyMetadata->allTokens.end(),
                      attrName) != SdrPropertyMetadata->allTokens.end()){
            continue;
        }

        if (std::find(_xmlAttributeNames->allTokens.begin(),
                      _xmlAttributeNames->allTokens.end(),
                      attrName) != _xmlAttributeNames->allTokens.end()){
            continue;
        }

        // Attribute hasn't been handled yet, so put it into the hints dict
        hints.insert({attrName, attrValue});
    }

    // Inject any parser-specific metadata into the metadata map
    _InjectParserMetadata(attributes, typeName);

    // Determine the default value; leave empty if a default isn't found
    // -------------------------------------------------------------------------
    const VtValue defaultValue =
        attributes.count(_xmlAttributeNames->defaultAttr)
        ? _GetVtValue(attributes.at(_xmlAttributeNames->defaultAttr),
                      typeName,
                      arraySize,
                      attributes)
        : VtValue();

    return SdrShaderPropertyUniquePtr(
        new SdrShaderProperty(
            propName,
            typeName,
            defaultValue,
            isOutput,
            arraySize,
            attributes,
            hints,
            options
        )
    );
}

static void
_InjectParserMetadata(NdrTokenMap& metadata,
                                           const TfToken& typeName)
{
    if (typeName == SdrPropertyTypes->String) {
        if (IsPropertyAnAssetIdentifier(metadata)) {
            metadata[SdrPropertyMetadata->IsAssetIdentifier] = "";
        }
    }
}

static NdrStringVec
_GetAttributeValuesFromChildren(
    xml_node parent, const char* attrName)
{
    xml_node child = parent.first_child();
    NdrStringVec childAttrValues;

    // Iterate over all children and get the attr value
    while (child) {
        xml_attribute attr = child.attribute(attrName);
        childAttrValues.emplace_back(attr.value());
        child = child.next_sibling();
    }

    return childAttrValues;
}

static TfToken
_Get(
    const NdrTokenMap& map, const TfToken& key, TfToken defaultValue)
{
    typename NdrTokenMap::const_iterator it = map.find(key);

    if (it != map.end()) {
        return TfToken(it->second);
    }

    return defaultValue;
}

static int
_Get(
    const NdrTokenMap& map, const TfToken& key, int defaultValue)
{
    typename NdrTokenMap::const_iterator it = map.find(key);

    if (it != map.end()) {
        int value = defaultValue;

        try {
            value = stoi(it->second);
        } catch (...) {
            TF_DEBUG(NDR_PARSING).Msg(
                "Attribute [%s] with string value [%s] "
                "couldn't be converted to int.", 
                key.GetText(), it->second.c_str());
        }

        return value;
    }

    return defaultValue;
}

PXR_NAMESPACE_CLOSE_SCOPE
