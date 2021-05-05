//
// Copyright 2016 Pixar
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
#ifndef PXR_USD_USD_SHADE_UTILS_H
#define PXR_USD_USD_SHADE_UTILS_H

#include "pxr/pxr.h"
#include "pxr/usd/usdShade/api.h"
#include "pxr/usd/usdShade/types.h"
#include "pxr/usd/usd/attribute.h"

#include "pxr/base/tf/smallVector.h"
#include "pxr/base/tf/token.h"

#include <string>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

class UsdShadeInput;
class UsdShadeOutput;

/// \class UsdShadeUtils
///
/// This class contains a set of utility functions used when authoring and 
/// querying shading networks.
///
class UsdShadeUtils {
public:
    /// Returns the namespace prefix of the USD attribute associated with the
    /// given shading attribute type.
    USDSHADE_API
    static std::string GetPrefixForAttributeType(
        UsdShadeAttributeType sourceType);

    /// Given the full name of a shading attribute, returns it's base name and
    /// shading attribute type.
    USDSHADE_API
    static std::pair<TfToken, UsdShadeAttributeType> 
        GetBaseNameAndType(const TfToken &fullName);

    /// Given the full name of a shading attribute, returns its shading
    /// attribute type.
    USDSHADE_API
    static UsdShadeAttributeType GetType(const TfToken &fullName);

    /// Returns the full shading attribute name given the basename and the
    /// shading attribute type. \p baseName is the name of the input or output
    /// on the shading node. \p type is the \ref UsdShadeAttributeType of the
    /// shading attribute.
    USDSHADE_API
    static TfToken GetFullName(const TfToken &baseName, 
                               const UsdShadeAttributeType type);

    /// \brief Find what is connected to an Input or Output recursively
    ///
    /// GetValueProducingAttributes implements the UsdShade connectivity rules
    /// described in \ref UsdShadeAttributeResolution .
    ///
    /// When tracing connections within networks that contain containers like
    /// UsdShadeNodeGraph nodes, the actual output(s) or value(s) at the end of
    /// an input or output might be multiple connections removed. The methods
    /// below resolves this across multiple physical connections.
    ///
    /// An UsdShadeInput is getting its value from one of these sources:
    /// - If the input is not connected the UsdAttribute for this input is
    /// returned, but only if it has an authored value. The input attribute
    /// itself carries the value for this input.
    /// - If the input is connected we follow the connection(s) until we reach
    /// a valid output of a UsdShadeShader node or if we reach a valid
    /// UsdShadeInput attribute of a UsdShadeNodeGraph or UsdShadeMaterial that
    /// has an authored value.
    ///
    /// An UsdShadeOutput on a container can get its value from the same
    /// type of sources as a UsdShadeInput on either a UsdShadeShader or
    /// UsdShadeNodeGraph. Outputs on non-containers (UsdShadeShaders) cannot be
    /// connected.
    ///
    /// This function returns a vector of UsdAttributes. The vector is empty if
    /// no valid attribute was found. The type of each attribute can be
    /// determined with the \p UsdShadeUtils::GetType function.
    ///
    /// If \p shaderOutputsOnly is true, it will only report attributes that are
    /// outputs of non-containers (UsdShadeShaders). This is a bit faster and
    /// what is need when determining the connections for Material terminals.
    ///
    /// \note This will return the last attribute along the connection chain
    /// that has an authored value, which might not be the last attribute in the
    /// chain itself.
    /// \note When the network contains multi-connections, this function can
    /// return multiple attributes for a single input or output. The list of
    /// attributes is build by a depth-first search, following the underlying
    /// connection paths in order. The list can contain both UsdShadeOutput and
    /// UsdShadeInput attributes. It is up to the caller to decide how to
    /// process such a mixture.
    USDSHADE_API
    static UsdShadeAttributeVector GetValueProducingAttributes(
        UsdShadeInput const &input,
        bool shaderOutputsOnly = false);
    /// \overload
    USDSHADE_API
    static UsdShadeAttributeVector GetValueProducingAttributes(
        UsdShadeOutput const &output,
        bool shaderOutputsOnly = false);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
