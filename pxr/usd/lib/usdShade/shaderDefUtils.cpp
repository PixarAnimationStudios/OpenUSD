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
#include "pxr/usd/ar/resolver.h"
#include "pxr/usd/usdShade/shaderDefUtils.h"
#include "pxr/usd/usdShade/shader.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_DEFINE_PRIVATE_TOKENS(
    _tokens,

    // sdrMetadata keys used in UsdShade-based shader definitions that are 
    // added to SdrRegistry.
    (family)
    (version)
);

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

    // We can't use GetShaderId() here since the implmentationSource of the 
    // shader is sourceAsset.
    TfToken shaderId; 
    shaderDef.GetIdAttr().Get(&shaderId);

    const UsdPrim shaderDefPrim = shaderDef.GetPrim();

    // Get shader family from sdrMetadata.
    TfToken family(shaderDef.GetSdrMetadataByKey(_tokens->family));
    // Use the prim name if family info isn't available in sdrMetadata.
    if (family.IsEmpty()) {
        family = shaderDefPrim.GetName();
    }

    // Get shader version from sdrMetadata.
    // XXX: We could also get this info from the name of the prim, 
    // if we enforce the restriction that the version should be a part of the 
    // prim's name.
    std::string version = shaderDef.GetSdrMetadataByKey(_tokens->version);

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
                    /* identifier */ shaderDefPrim.GetName(), 
                    version.empty() ? NdrVersion().GetAsDefault() 
                                    : NdrVersion(version),  
                    /* name */ shaderId.GetString(),
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

