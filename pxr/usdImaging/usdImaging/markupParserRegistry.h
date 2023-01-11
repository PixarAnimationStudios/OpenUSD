//
// Copyright 2023 Pixar
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
#ifndef PXR_USD_IMAGING_USD_IMAGING_MARKUP_PARSER_REGISTRY_H
#define PXR_USD_IMAGING_USD_IMAGING_MARKUP_PARSER_REGISTRY_H

/// \file usdImaging/markupParserRegistry.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/base/tf/singleton.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/token.h"

#include <memory>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

using UsdImagingMarkupParserSharedPtr = std::shared_ptr<class UsdImagingMarkupParser>;


/// \class UsdImagingMarkupParserRegistry
///
/// Manages plugin registration and loading for UsdImagingMarkupParser subclasses.
///
class UsdImagingMarkupParserRegistry : public TfSingleton<UsdImagingMarkupParserRegistry>
{
public:
    USDIMAGING_API
    static UsdImagingMarkupParserRegistry& GetInstance();

private:
    friend class TfSingleton<UsdImagingMarkupParserRegistry>;
    UsdImagingMarkupParserRegistry();

    friend class UsdImagingMarkupParser;

    UsdImagingMarkupParserSharedPtr _GetParser(const TfHashMap<TfToken, std::string, TfToken::HashFunctor>& setting);
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_USD_IMAGING_MARKUP_PARSER_REGISTRY_H
