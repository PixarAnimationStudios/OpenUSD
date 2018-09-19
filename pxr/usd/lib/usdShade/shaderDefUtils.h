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
#ifndef USD_SHADERDEF_UTILS_H
#define USD_SHADERDEF_UTILS_H

#include "pxr/pxr.h"
#include "pxr/usd/usdShade/api.h"

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
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
