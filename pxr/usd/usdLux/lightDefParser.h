//
// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
/// the UsdLux intrinsic concrete light types.
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

    // Mapping of shaderId to Typenames for API schemas which we want to have a
    // sdr representation like concrete UsdLux light types.
    using ShaderIdToAPITypeNameMap = 
        std::unordered_map<TfToken, TfToken, TfToken::HashFunctor>;
    static const ShaderIdToAPITypeNameMap& _GetShaderIdToAPITypeNameMap();
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_LUX_LIGHT_DEF_PARSER_H
