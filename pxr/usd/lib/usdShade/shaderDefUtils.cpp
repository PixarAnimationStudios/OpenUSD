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

#include "pxr/base/tf/stringUtils.h"

#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/usdShade/shaderDefUtils.h"
#include "pxr/usd/usdShade/shader.h"

#include <cctype>

PXR_NAMESPACE_OPEN_SCOPE

static bool _IsNumber(const std::string& s)
{
    return !s.empty() && 
        std::find_if(s.begin(), s.end(), 
                     [](unsigned char c) { return !std::isdigit(c); })
        == s.end();
}

/* static */ 
bool
UsdShadeShaderDefUtils::SplitShaderIdentifier(
    const TfToken &identifier, 
    TfToken *familyName,
    TfToken *implementationName,
    NdrVersion *version)
{
   std::vector<std::string> tokens = TfStringTokenize(identifier.GetString(), 
        "_");

    if (tokens.empty()) {
        return false;
    }

    *familyName = TfToken(tokens[0]);

    if (tokens.size() == 1) {
        *familyName = identifier;
        *implementationName = identifier;
        *version = NdrVersion();
    } else if (tokens.size() == 2) {
        if (_IsNumber(tokens[tokens.size()-1])) {
            int major = std::stoi(*tokens.rbegin());
            *version = NdrVersion(major);
            *implementationName = *familyName;
        } else {
            *version = NdrVersion();
            *implementationName = identifier;
        }
    } else if (tokens.size() > 2) {
        bool lastTokenIsNumber = _IsNumber(tokens[tokens.size()-1]);
        bool penultimateTokenIsNumber = _IsNumber(tokens[tokens.size()-2]);

        if (penultimateTokenIsNumber && !lastTokenIsNumber) {
            TF_WARN("Invalid shader identifier '%s'.", identifier.GetText()); 
            return false;
        }

        if (lastTokenIsNumber && penultimateTokenIsNumber) {
            *version = NdrVersion(std::stoi(tokens[tokens.size()-2]), 
                                  std::stoi(tokens[tokens.size()-1]));
            *implementationName = TfToken(TfStringJoin(tokens.begin(), 
                tokens.begin() + (tokens.size() - 2), "_"));
        } else if (lastTokenIsNumber) {
            *version = NdrVersion(std::stoi(tokens[tokens.size()-1]));
            *implementationName  = TfToken(TfStringJoin(tokens.begin(), 
                tokens.begin() + (tokens.size() - 1), "_"));
        } else {
            // No version information is available. 
            *implementationName = identifier;
            *version = NdrVersion();
        }
    }

    return true;
}

/* static */
NdrNodeDiscoveryResultVec 
UsdShadeShaderDefUtils::GetNodeDiscoveryResults(
    const UsdShadeShader &shaderDef,
    const std::string &sourceUri)
{
    NdrNodeDiscoveryResultVec result;

    // Implementation source must be sourceAsset for the shader to represent 
    // nodes in Sdr.
    if (shaderDef.GetImplementationSource() != UsdShadeTokens->sourceAsset)
        return result;

    const UsdPrim shaderDefPrim = shaderDef.GetPrim();
    const TfToken &identifier = shaderDefPrim.GetName();

    // Get the family name, shader name and version information from the 
    // identifier.
    TfToken family;
    TfToken name; 
    NdrVersion version; 
    if (!SplitShaderIdentifier(shaderDefPrim.GetName(), 
                &family, &name, &version)) {
        // A warning has already been issued by SplitShaderIdentifier.
        return result;
    }
    
    static const std::string infoNamespace("info:");
    static const std::string baseSourceAsset(":sourceAsset");

    // This vector will contain all the info:*:sourceAsset properties.
    std::vector<UsdProperty> sourceAssetProperties = 
        shaderDefPrim.GetAuthoredProperties(
        [](const TfToken &propertyName) {
            const std::string &propertyNameStr = propertyName.GetString();
            return TfStringStartsWith(propertyNameStr, infoNamespace) &&
                    TfStringEndsWith(propertyNameStr, baseSourceAsset);
        });

    const TfToken discoveryType(ArGetResolver().GetExtension(sourceUri));

    for (auto &prop : sourceAssetProperties) {
        UsdAttribute attr = prop.As<UsdAttribute>();
        if (!attr) {
            continue;
        }

        SdfAssetPath sourceAssetPath;
        if (attr.Get(&sourceAssetPath) && 
            !sourceAssetPath.GetAssetPath().empty()) {
                
            auto nameTokens = 
                    SdfPath::TokenizeIdentifierAsTokens(attr.GetName());
            if (nameTokens.size() != 3) {
                continue;
            }

            std::string resolvedUri = ArGetResolver().Resolve(
                    sourceAssetPath.GetAssetPath());

            // Create a discoveryResult only if the referenced sourceAsset
            // can be resolved. 
            // XXX: Should we do this regardless and expect the parser to be 
            // able to resolve the unresolved asset path?
            if (!resolvedUri.empty()) {
                const TfToken &sourceType = nameTokens[1];

                // Use the prim name as the identifier since it is 
                // guaranteed to be unique in the file. 
                // Use the shader id as the name of the shader.
                result.emplace_back(
                    identifier,
                    version.GetAsDefault(),
                    name,
                    family, 
                    discoveryType,
                    sourceType,
                    /* uri */ sourceUri, 
                    /* resolvedUri */ sourceUri);
            } else {
                TF_WARN("Unable to resolve info:sourceAsset <%s> with value "
                    "@%s@.", attr.GetPath().GetText(), 
                    sourceAssetPath.GetAssetPath().c_str());
            }
        }
    }

    return result;
}

PXR_NAMESPACE_CLOSE_SCOPE

