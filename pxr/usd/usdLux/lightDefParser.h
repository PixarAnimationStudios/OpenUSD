//
// Copyright 2020 Pixar
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
#ifndef PXR_USD_USD_LUX_LIGHT_DEF_PARSER_H
#define PXR_USD_USD_LUX_LIGHT_DEF_PARSER_H

#include "pxr/pxr.h"
#include "pxr/usd/usdLux/api.h"

#include "pxr/usd/ndr/declare.h"
#include "pxr/usd/ndr/parserPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdLux_LightDefParserPlugin
/// 
/// Parses shader definitions from the registered prim definitions for 
/// UsdLuxLight and UsdLuxLightFilter derived schema classes.
/// 
class UsdLux_LightDefParserPlugin : public NdrParserPlugin 
{
public: 
    USDLUX_API
    UsdLux_LightDefParserPlugin() = default;

    USDLUX_API
    ~UsdLux_LightDefParserPlugin() override = default;

    USDLUX_API
    NdrNodeUniquePtr Parse(const NdrNodeDiscoveryResult &discoveryResult) 
        override;

    USDLUX_API
    const NdrTokenVec &GetDiscoveryTypes() const override;

    USDLUX_API
    const TfToken &GetSourceType() const override;

private:
    // The discovery plugin needs to match the source type and discovery types
    // that instances of this parser returns when discovering nodes.
    friend class UsdLux_DiscoveryPlugin;
    static const TfToken &_GetSourceType();
    static const TfToken &_GetDiscoveryType();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_LUX_LIGHT_DEF_PARSER_H
