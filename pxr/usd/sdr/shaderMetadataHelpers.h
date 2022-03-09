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

#ifndef PXR_USD_SDR_SHADER_METADATA_HELPERS_H
#define PXR_USD_SDR_SHADER_METADATA_HELPERS_H

/// \file sdr/shaderMetadataHelpers.h

#include "pxr/pxr.h"
#include "pxr/usd/sdr/api.h"
#include "pxr/base/tf/token.h"
#include "pxr/usd/sdr/declare.h"

#include <limits>

PXR_NAMESPACE_OPEN_SCOPE

/// \namespace ShaderMetadataHelpers
///
/// Various utilities for parsing metadata contained within shaders.
///
namespace ShaderMetadataHelpers
{
    /// Determines if the given metadatum in the metadata dictionary has a
    /// truthy value. All values are considered to be true except the following
    /// (case-insensitive): '0', 'false', and 'f'. The absence of `key`
    /// in the metadata also evaluates to false.
    SDR_API
    bool
    IsTruthy(const TfToken& key, const NdrTokenMap& metadata);

    /// Extracts the string value from the given metadatum if it exists,
    /// otherwise returns \p defaultValue.
    SDR_API
    std::string
    StringVal(const TfToken& key, const NdrTokenMap& metadata,
              const std::string& defaultValue = std::string());

    /// Extracts the tokenized value from the given metadatum if it exists,
    /// otherwise returns \p defaultValue.
    SDR_API
    TfToken
    TokenVal(const TfToken& key, const NdrTokenMap& metadata,
             const TfToken& defaultValue = TfToken());

    /// Extracts the int value from the given metadatum if it exists and is a
    /// valid integer value, otherwise returns \p default value.
    SDR_API
    int
    IntVal(const TfToken& key, const NdrTokenMap& metadata,
           int defaultValue = std::numeric_limits<int>::max());

    /// Extracts a vector of strings from the given metadatum. An empty vector
    /// is returned if the metadatum does not exist.
    SDR_API
    NdrStringVec
    StringVecVal(const TfToken& key, const NdrTokenMap& metadata);

    /// Extracts a vector of tokenized values from the given metadatum. An empty
    /// vector is returned if the metadatum does not exist.
    SDR_API
    NdrTokenVec
    TokenVecVal(const TfToken& key, const NdrTokenMap& metadata);

    /// Extracts an "options" vector from the given string.
    SDR_API
    NdrOptionVec
    OptionVecVal(const std::string& optionStr);

    /// Serializes a vector of strings into a string using the pipe character
    /// as the delimiter.
    SDR_API
    std::string
    CreateStringFromStringVec(const NdrStringVec& stringVec);

    /// Determines if the specified property metadata has a widget that
    /// indicates the property is an asset identifier.
    SDR_API
    bool
    IsPropertyAnAssetIdentifier(const NdrTokenMap& metadata);

    /// Determines if the specified property metadata has a 'renderType' that
    /// indicates the property should be a SdrPropertyTypes->Terminal
    SDR_API
    bool
    IsPropertyATerminal(const NdrTokenMap& metadata);

    /// Gets the "role" from metadata if one is provided. Only returns a value
    // if it's a valid role as defined by SdrPropertyRole tokens.
    SDR_API
    TfToken
    GetRoleFromMetadata(const NdrTokenMap& metadata);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_SDR_SHADER_METADATA_HELPERS_H
