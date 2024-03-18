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
#ifndef PXR_USD_IMAGING_USD_IMAGING_MARKUP_PARSER_H
#define PXR_USD_IMAGING_USD_IMAGING_MARKUP_PARSER_H

/// \file usdImaging/markupParser.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/tokens.h"
#include "pxr/usdImaging/usdImaging/markupText.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/usd/usd/prim.h"
#include <string>


PXR_NAMESPACE_OPEN_SCOPE

using UsdImagingMarkupParserSharedPtr = std::shared_ptr<class UsdImagingMarkupParser>;

/// \class UsdImagingMarkupParser
///
/// Base class for the markup parser plugin.
///
class UsdImagingMarkupParser {
public:
    typedef TfHashMap<TfToken, std::string, TfToken::HashFunctor> ParserSettingMap;

    /// If the markup parser plugin is initialized.
    USDIMAGING_API
    static bool IsInitialized()
    {
        return _markupParser != nullptr;
    }

    /// Initialize the markup parser plugin using a default parser setting.
    USDIMAGING_API
    static bool DefaultInitialize();

    /// Initialize the markup parser plugin using a parser setting.
    USDIMAGING_API
    static bool Initialize(const ParserSettingMap&);

    /// Parse the markup string in the MarkupText.
    USDIMAGING_API
    static bool ParseText(std::shared_ptr<UsdImagingMarkupText> markupText);

protected:
    USDIMAGING_API
    UsdImagingMarkupParser() = default;
    USDIMAGING_API
    virtual ~UsdImagingMarkupParser();

private:
    friend class UsdImagingMarkupParserRegistry;

    // This class doesn't require copy support.
    UsdImagingMarkupParser(const UsdImagingMarkupParser&)             = delete;
    UsdImagingMarkupParser &operator =(const UsdImagingMarkupParser&) = delete;

    /// Initialize the markup parser plugin using a text setting.
    USDIMAGING_API
    virtual bool _Initialize(const ParserSettingMap&) = 0;

    /// Parse the markup string in the MarkupText.
    USDIMAGING_API
    virtual bool _ParseText(std::shared_ptr<UsdImagingMarkupText> markupText) = 0;

    /// If a specified markup language is supported.
    USDIMAGING_API
    virtual bool _IsSupported(const std::wstring& language) = 0;

    static UsdImagingMarkupParserSharedPtr _markupParser;
    static std::mutex _initializeMutex;
};

/// \class UsdImagingMarkupParserFactoryBase
///
/// Base class for the factory of UsdImagingMarkupParser.
///
class UsdImagingMarkupParserFactoryBase : public TfType::FactoryBase {
public:
    virtual UsdImagingMarkupParserSharedPtr New() const = 0;
};

/// \class UsdImagingMarkupParserFactory
///
/// The factory to create UsdImagingMarkupParser.
///
template <class T>
class UsdImagingMarkupParserFactory : public UsdImagingMarkupParserFactoryBase {
public:
    virtual UsdImagingMarkupParserSharedPtr New() const
    {
        return UsdImagingMarkupParserSharedPtr(new T);
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_USD_IMAGING_USD_IMAGING_MARKUP_PARSER_H
