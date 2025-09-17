//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_USD_SHADE_SHADER_DEF_PARSER_H
#define PXR_USD_USD_SHADE_SHADER_DEF_PARSER_H

#include "pxr/usd/usdShade/api.h"

#include "pxr/usd/sdr/declare.h"
#include "pxr/usd/sdr/discoveryPlugin.h"
#include "pxr/usd/sdr/parserPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdShadeShaderDefParserPlugin
/// 
/// Parses shader definitions represented using USD scene description using the 
/// schemas provided by UsdShade.
/// 
class UsdShadeShaderDefParserPlugin : public SdrParserPlugin 
{
public: 
    USDSHADE_API
    UsdShadeShaderDefParserPlugin() = default;

    USDSHADE_API
    ~UsdShadeShaderDefParserPlugin() override = default;

    USDSHADE_API
    SdrShaderNodeUniquePtr ParseShaderNode(
        const SdrShaderNodeDiscoveryResult &discoveryResult) override;

    USDSHADE_API
    const SdrTokenVec &GetDiscoveryTypes() const override;

    USDSHADE_API
    const TfToken &GetSourceType() const override;    
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_USD_SHADE_SHADER_DEF_PARSER_H
