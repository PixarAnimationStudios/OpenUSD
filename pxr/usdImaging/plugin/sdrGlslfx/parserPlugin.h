//
// Copyright 2019 Pixar
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
