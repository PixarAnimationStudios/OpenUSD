//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef SDRGLSLFX_PARSER_PLUGIN_H
#define SDRGLSLFX_PARSER_PLUGIN_H

#include "pxr/pxr.h"
#include "pxr/usd/ndr/declare.h"
#include "pxr/usd/ndr/parserPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

// Forward declarations
class NdrNode;
struct NdrNodeDiscoveryResult;

/// \class SdrGlslfxParserPlugin
/// 
/// Parses shader definitions represented using Glslfx.
/// 
class SdrGlslfxParserPlugin: public NdrParserPlugin 
{
public: 
    SdrGlslfxParserPlugin() = default;

    ~SdrGlslfxParserPlugin() override = default;

    NdrNodeUniquePtr Parse(const NdrNodeDiscoveryResult &discoveryResult) 
        override;

    const NdrTokenVec &GetDiscoveryTypes() const override;

    const TfToken &GetSourceType() const override;    
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDRGLSLFX_PARSER_PLUGIN
