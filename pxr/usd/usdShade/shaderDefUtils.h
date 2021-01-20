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
#ifndef PXR_USD_USD_SHADE_SHADER_DEF_UTILS_H
#define PXR_USD_USD_SHADE_SHADER_DEF_UTILS_H

#include "pxr/pxr.h"
#include "pxr/usd/usdShade/api.h"
#include "pxr/usd/usdShade/connectableAPI.h"

#include "pxr/usd/ndr/nodeDiscoveryResult.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

class UsdShadeShader;

/// \class UsdShadeShaderDefUtils
///
/// This class contains a set of utility functions used for populating the 
/// shader registry with shaders definitions specified using UsdShade schemas.
///
class UsdShadeShaderDefUtils {
public:
    /// Given a shader's \p identifier token, computes the corresponding 
    /// SdrShaderNode's family name, implementation name and shader version 
    /// (as NdrVersion).
    /// 
    /// * \p familyName is the prefix of \p identifier up to and not 
    /// including the first underscore. 
    /// * \p version is the suffix of \p identifier comprised of one or 
    /// two integers representing the major and minor version numbers.
    /// * \p implementationName is the string we get by joining 
    /// <i>familyName</i> with everything that's in between <i>familyName</i> 
    /// and <i>version</i> with an underscore.
    /// 
    /// Returns true if \p identifier is valid and was successfully split 
    /// into the different components. 
    /// 
    /// \note The python version of this function returns a tuple containing
    /// (famiyName, implementationName, version).
    USDSHADE_API
    static bool SplitShaderIdentifier(const TfToken &identifier, 
                TfToken *familyName,
                TfToken *implementationName,
                NdrVersion *version);

    /// Returns the list of NdrNodeDiscoveryResult objects that must be added 
    /// to the shader registry for the given shader \p shaderDef, assuming it 
    /// is found in a shader definition file found by an Ndr discovery plugin. 
    /// 
    /// To enable the shaderDef parser to find and parse this shader, 
    /// \p sourceUri should have the resolved path to the usd file containing 
    /// this shader prim.
    USDSHADE_API
    static NdrNodeDiscoveryResultVec GetNodeDiscoveryResults(
        const UsdShadeShader &shaderDef,
        const std::string &sourceUri);

    /// Gets all input and output properties of the given \p shaderDef and 
    /// translates them into NdrProperties that can be used as the properties
    /// for an SdrShaderNode.
    USDSHADE_API
    static NdrPropertyUniquePtrVec GetShaderProperties(
        const UsdShadeConnectableAPI &shaderDef);

    /// Collects all the names of valid primvar inputs of the given \p metadata
    /// and the given \p shaderDef and returns the string used to represent
    /// them in SdrShaderNode metadata.
    USDSHADE_API
    static std::string GetPrimvarNamesMetadataString(
        const NdrTokenMap metadata,
        const UsdShadeConnectableAPI &shaderDef);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
