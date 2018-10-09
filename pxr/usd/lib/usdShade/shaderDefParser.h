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
#ifndef USDSHADE_SRDPARSERPLUGIN_H
#define USDSHADE_SRDPARSERPLUGIN_H

#include "pxr/usd/usdShade/api.h"

#include "pxr/usd/ndr/declare.h"
#include "pxr/usd/ndr/discoveryPlugin.h"
#include "pxr/usd/ndr/parserPlugin.h"

PXR_NAMESPACE_OPEN_SCOPE

class UsdStageCache;

/// \class UsdShadeShaderDefParserPlugin
/// 
/// Parses shader definitions represented using USD scene description using the 
/// schemas provided by UsdShade.
/// 
class UsdShadeShaderDefParserPlugin : public NdrParserPlugin 
{
public: 
    USDSHADE_API
    UsdShadeShaderDefParserPlugin() = default;

    USDSHADE_API
    ~UsdShadeShaderDefParserPlugin() override = default;

    USDSHADE_API
    NdrNodeUniquePtr Parse(const NdrNodeDiscoveryResult &discoveryResult) 
        override;

    USDSHADE_API
    const NdrTokenVec &GetDiscoveryTypes() const override;

    USDSHADE_API
    const TfToken &GetSourceType() const override;    

private:
    static UsdStageCache _cache;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDSHADE_SRDPARSERPLUGIN_H
