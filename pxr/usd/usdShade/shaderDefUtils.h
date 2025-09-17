//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_SHADE_SHADER_DEF_UTILS_H
#define PXR_USD_USD_SHADE_SHADER_DEF_UTILS_H

#include "pxr/pxr.h"
#include "pxr/usd/usdShade/api.h"
#include "pxr/usd/usdShade/connectableAPI.h"

#include "pxr/usd/sdr/shaderProperty.h"
#include "pxr/usd/sdr/shaderNodeDiscoveryResult.h"

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
    /// Returns the list of SdrShaderNodeDiscoveryResult objects that must be 
    /// added to the shader registry for the given shader \p shaderDef, 
    /// assuming it is found in a shader definition file found by an Sdr 
    /// discovery plugin. 
    /// 
    /// To enable the shaderDef parser to find and parse this shader, 
    /// \p sourceUri should have the resolved path to the usd file containing 
    /// this shader prim.
    USDSHADE_API
    static SdrShaderNodeDiscoveryResultVec GetDiscoveryResults(
        const UsdShadeShader &shaderDef,
        const std::string &sourceUri);

    /// Gets all input and output properties of the given \p shaderDef and 
    /// translates them into SdrShaderProperties that can be used as the
    /// properties for an SdrShaderNode.
    USDSHADE_API
    static SdrShaderPropertyUniquePtrVec GetProperties(
        const UsdShadeConnectableAPI &shaderDef);

    /// Collects all the names of valid primvar inputs of the given \p metadata
    /// and the given \p shaderDef and returns the string used to represent
    /// them in SdrShaderNode metadata.
    USDSHADE_API
    static std::string GetPrimvarNamesMetadataString(
        const SdrTokenMap metadata,
        const UsdShadeConnectableAPI &shaderDef);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
