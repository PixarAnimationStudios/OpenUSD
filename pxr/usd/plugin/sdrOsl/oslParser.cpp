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

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/matrix4d.h"
#include "pxr/base/tf/staticTokens.h"
#include "pxr/base/tf/weakPtr.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/array.h"
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/ndr/debugCodes.h"
#include "pxr/usd/ndr/nodeDiscoveryResult.h"
#include "pxr/usd/sdr/shaderMetadataHelpers.h"
#include "pxr/usd/sdr/shaderNode.h"
#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/sdrOsl/oslParser.h"

#include <tuple>

PXR_NAMESPACE_OPEN_SCOPE

using ShaderMetadataHelpers::IsPropertyAnAssetIdentifier;
using ShaderMetadataHelpers::IsTruthy;
using ShaderMetadataHelpers::OptionVecVal;

NDR_REGISTER_PARSER_PLUGIN(SdrOslParserPlugin)

TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    ((arraySize, "arraySize"))
    ((vstructMember, "vstructmember"))

    // Discovery and source type
    ((discoveryType, "oso"))
    ((sourceType, "OSL"))
);

// Static member initialization
const NdrTokenVec& SdrOslParserPlugin::DiscoveryTypes = {_tokens->discoveryType};
const TfToken& SdrOslParserPlugin::SourceType = _tokens->sourceType;

SdrOslParserPlugin::SdrOslParserPlugin()
{
    // Nothing yet
}

SdrOslParserPlugin::~SdrOslParserPlugin()
{
    // Nothing yet
}

NdrNodeUniquePtr
SdrOslParserPlugin::Parse(const NdrNodeDiscoveryResult& discoveryResult)
{
    // Each call to `Parse` should have its own reference to an OSL query to
    // prevent multi-threading issues
    OSL::OSLQuery oslQuery;

    bool parseSuccessful = true;

    if (!discoveryResult.uri.empty()) {
        // Get the resolved URI to a location that it can be read by the OSL parser    
        bool localFetchSuccessful = ArGetResolver().FetchToLocalResolvedPath(
            discoveryResult.uri,
            discoveryResult.resolvedUri
        );

        if (!localFetchSuccessful) {
            TF_WARN("Could not localize the OSL at URI [%s] into a local path. "
                    "An invalid Sdr node definition will be created.", 
                    discoveryResult.uri.c_str());

            return NdrParserPlugin::GetInvalidNode(discoveryResult);
        }

       // Attempt to parse the node
        parseSuccessful = oslQuery.open(discoveryResult.resolvedUri);

    } else if (!discoveryResult.sourceCode.empty()) {
#if OSL_LIBRARY_VERSION_CODE < 10701
        TF_WARN("Support for parsing OSL from an in-memory string is only "
            "available in OSL version 1.7.1 or newer.");
#else
        parseSuccessful = oslQuery.open_bytecode(discoveryResult.sourceCode);
#endif

    } else {
        TF_WARN("Invalid NdrNodeDiscoveryResult with identifier %s: both uri "
            "and sourceCode are empty.", discoveryResult.identifier.GetText());
        return NdrParserPlugin::GetInvalidNode(discoveryResult);
    }

    std::string errors = oslQuery.geterror();
    if (!parseSuccessful || !errors.empty()) {
        TF_WARN("Could not parse OSL shader at URI [%s]. An invalid Sdr node "
                "definition will be created. %s%s",
                discoveryResult.uri.c_str(),
                (errors.empty() ? "" : "Errors from OSL parser: "),
                (errors.empty() ? "" : TfStringReplace(errors, "\n", "; ").c_str()));

        return NdrParserPlugin::GetInvalidNode(discoveryResult);
    }

    return NdrNodeUniquePtr(
        new SdrShaderNode(
            discoveryResult.identifier,
            discoveryResult.version,
            discoveryResult.name,
            discoveryResult.family,
            _tokens->sourceType,
            _tokens->sourceType,    // OSL shaders don't declare different types
                                    // so use the same type as the source type
            discoveryResult.uri,
            _getNodeProperties(oslQuery, discoveryResult),
            _getNodeMetadata(oslQuery, discoveryResult.metadata),
            discoveryResult.sourceCode
        )
    );
}

NdrPropertyUniquePtrVec
SdrOslParserPlugin::_getNodeProperties(
    const OSL::OSLQuery &query, const NdrNodeDiscoveryResult& discoveryResult) const
{
    NdrPropertyUniquePtrVec properties;
    const size_t nParams = query.nparams();

    for (size_t i = 0; i < nParams; ++i) {
        const OslParameter* param = query.getparam(i);
        const std::string propName = param->name.string();

        // Struct members are not supported
        if (propName.find('.') != std::string::npos) {
            continue;
        }

        // Get type name, and determine the size of the array (if an array)
        TfToken typeName;
        size_t arraySize;
        std::tie(typeName, arraySize) = _getTypeName(param);

        // Extract metadata
        NdrTokenMap metadata = _getPropertyMetadata(param, discoveryResult);
        _injectParserMetadata(metadata, typeName);

        // Non-standard properties in the metadata are considered hints
        NdrTokenMap hints;
        for (const auto& meta : metadata) {
            if ((meta.first == SdrPropertyMetadata->Connectable)       ||
                (meta.first == SdrPropertyMetadata->Page)              ||
                (meta.first == SdrPropertyMetadata->Help)              ||
                (meta.first == SdrPropertyMetadata->Label)             ||
                (meta.first == SdrPropertyMetadata->IsDynamicArray)    ||
                (meta.first == SdrPropertyMetadata->Options)           ||
                (meta.first == SdrPropertyMetadata->VstructMemberName) ||
                (meta.first == SdrPropertyMetadata->VstructMemberOf)) {
                continue;
            }

            // The metadata sometimes incorrectly specifies array size; this
            // value is not respected
            if (meta.first == _tokens->arraySize) {
                TF_DEBUG(NDR_PARSING).Msg(
                    "Ignoring bad 'arraySize' attribute on property [%s] "
                    "on OSL shader [%s]",
                    propName.c_str(), discoveryResult.name.c_str());
                continue;
            }

            hints.insert(meta);
        }

        // Extract options
        NdrOptionVec options;
        if (metadata.count(SdrPropertyMetadata->Options)) {
            options = OptionVecVal(metadata.at(SdrPropertyMetadata->Options));
        }

        // Determine array-ness
        bool isDynamicArray =
            IsTruthy(SdrPropertyMetadata->IsDynamicArray, metadata);
        bool isArray = (arraySize > 0) || isDynamicArray;

        properties.emplace_back(
            SdrShaderPropertyUniquePtr(
                new SdrShaderProperty(
                    TfToken(propName),
                    typeName,
                    _getDefaultValue(*param, typeName, isArray),
                    param->isoutput,
                    arraySize,
                    metadata,
                    hints,
                    options
                )
            )
        );
    }

    return properties;
}

NdrTokenMap
SdrOslParserPlugin::_getPropertyMetadata(const OslParameter* param,
    const NdrNodeDiscoveryResult& discoveryResult) const
{
    NdrTokenMap metadata;

    for (const OslParameter& metaParam : param->metadata) {
        TfToken entryName = TfToken(metaParam.name.string());

        // Vstruct metadata needs to be specially parsed; otherwise, just stuff
        // the value into the map
        if (entryName == _tokens->vstructMember) {
            std::string vstruct = _getParamAsString(metaParam);

            if (!vstruct.empty()) {
                // A dot splits struct from member name
                size_t dotPos = vstruct.find('.');

                if (dotPos != std::string::npos) {
                    metadata[SdrPropertyMetadata->VstructMemberOf] =
                        vstruct.substr(0, dotPos);

                    metadata[SdrPropertyMetadata->VstructMemberName] =
                        vstruct.substr(dotPos + 1);
                } else {
                TF_DEBUG(NDR_PARSING).Msg(
                    "Bad virtual structure member in %s.%s:%s",
                    discoveryResult.name.c_str(), param->name.string().c_str(),
                    vstruct.c_str());
                }
            }
        } else {
            metadata[entryName] = _getParamAsString(metaParam);
        }
    }

    return metadata;
}

void
SdrOslParserPlugin::_injectParserMetadata(NdrTokenMap& metadata,
                                          const TfToken& typeName) const
{
    if (typeName == SdrPropertyTypes->String) {
        if (IsPropertyAnAssetIdentifier(metadata)) {
            metadata[SdrPropertyMetadata->IsAssetIdentifier] = "";
        }
    }
}

NdrTokenMap
SdrOslParserPlugin::_getNodeMetadata(
    const OSL::OSLQuery &query,
    const NdrTokenMap &baseMetadata) const
{
    NdrTokenMap nodeMetadata = baseMetadata;

    // Convert the OSL metadata to a dict. Each entry in the metadata is stored
    // as an OslParameter.
    for (const OslParameter& metaParam : query.metadata()) {
        TfToken entryName = TfToken(metaParam.name.string());

        nodeMetadata[entryName] = _getParamAsString(metaParam);
    }

    return nodeMetadata;
}

std::string
SdrOslParserPlugin::_getParamAsString(const OslParameter& param) const
{
    if (param.sdefault.size() == 1) {
        return param.sdefault[0].string();
    } else if (param.idefault.size() == 1) {
        return std::to_string(param.idefault[0]);
    } else if (param.fdefault.size() == 1) {
        return std::to_string(param.fdefault[0]);
    }

    return std::string();
}

std::tuple<TfToken, size_t>
SdrOslParserPlugin::_getTypeName(const OslParameter* param) const
{
    // Exit early if this param is known to be a struct
    if (param->isstruct) {
        return std::make_tuple(SdrPropertyTypes->Struct, /* array size = */ 0);
    }

    // Otherwise, continue on to determine the type (and possibly array size)
    std::string typeName = std::string(param->type.c_str());
    size_t arraySize = 0;
    size_t openingBracket = typeName.find('[');

    if (openingBracket != std::string::npos) {
        // stoi will stop at the first non-number char, usually ']'
        arraySize = std::stoi(typeName.substr(openingBracket + 1));

        // grab the part before the first bracket and turn it into the typeName
        typeName = typeName.substr(0, openingBracket);
    }

    return std::make_tuple(TfToken(typeName), arraySize);
}

VtValue
SdrOslParserPlugin::_getDefaultValue(
    const SdrOslParserPlugin::OslParameter& param,
    const std::string& oslType,
    bool isArray) const
{
    // INT and INT ARRAY
    // -------------------------------------------------------------------------
    if (oslType == SdrPropertyTypes->Int) {
        if (!isArray && param.idefault.size() == 1) {
            return VtValue(param.idefault[0]);
        }

        VtIntArray array;
        array.assign(param.idefault.begin(), param.idefault.end());

        return VtValue::Take(array);
    }

    // STRING and STRING ARRAY
    // -------------------------------------------------------------------------
    else if (oslType == SdrPropertyTypes->String) {
        if (!isArray && param.sdefault.size() == 1) {
            return VtValue(param.sdefault[0].string());
        }

        VtStringArray array;
        array.assign(param.sdefault.begin(), param.sdefault.end());

        return VtValue::Take(array);
    }

    // FLOAT and FLOAT ARRAY
    // -------------------------------------------------------------------------
    else if (oslType == SdrPropertyTypes->Float) {
        if (!isArray && param.fdefault.size() == 1) {
            return VtValue(param.fdefault[0]);
        }

        VtFloatArray array;
        array.assign(param.fdefault.begin(), param.fdefault.end());

        return VtValue::Take(array);
    }

    // VECTOR TYPES and VECTOR TYPE ARRAYS
    // -------------------------------------------------------------------------
    else if (oslType == SdrPropertyTypes->Color  ||
             oslType == SdrPropertyTypes->Point  ||
             oslType == SdrPropertyTypes->Normal ||
             oslType == SdrPropertyTypes->Vector) {
        if (!isArray && param.fdefault.size() == 3) {
            return VtValue(
                GfVec3d(param.fdefault[0],
                        param.fdefault[1],
                        param.fdefault[2])
            );
        } else if (isArray && param.fdefault.size() % 3 == 0) {
            int numElements = param.fdefault.size() / 3;
            VtVec3dArray array(numElements);

            for (int i = 0; i < numElements; ++i) {
                array[i] = GfVec3d(param.fdefault[3*i + 0],
                                   param.fdefault[3*i + 1],
                                   param.fdefault[3*i + 2]);
            }

            return VtValue::Take(array);
        }
    }

    // MATRIX
    // -------------------------------------------------------------------------
    else if (oslType == SdrPropertyTypes->Matrix) {
        // XXX: No matrix array support
        if (!isArray && param.fdefault.size() == 16) {
            GfMatrix4d mat;
            double* values = mat.GetArray();

            for (int i = 0; i < 16; ++i) {
                values[i] = static_cast<double>(param.fdefault[i]);
            }

            return VtValue::Take(mat);
        }
    }

    // Didn't find a supported type
    return VtValue();
}

PXR_NAMESPACE_CLOSE_SCOPE
