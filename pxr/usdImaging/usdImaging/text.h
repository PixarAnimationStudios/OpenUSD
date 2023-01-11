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
#ifndef PXR_USD_IMAGING_USD_IMAGING_TEXT_H
#define PXR_USD_IMAGING_USD_IMAGING_TEXT_H

/// \file usdImaging/text.h

#include "pxr/pxr.h"
#include "pxr/usdImaging/usdImaging/api.h"
#include "pxr/usdImaging/usdImaging/markupText.h"
#include "pxr/usdImaging/usdImaging/textStyle.h"
#include "pxr/usdImaging/usdImaging/tokens.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/usd/usd/prim.h"
#include <string>

#define M_EPSILON 1e-10f

PXR_NAMESPACE_OPEN_SCOPE

class UsdImagingTextRenderer;

using UsdImagingTextSharedPtr = std::shared_ptr<class UsdImagingText>;
using UsdImagingTextRendererSharedPtr = std::shared_ptr<class UsdImagingTextRenderer>;

/// \class UsdImagingText
///
/// Base class for the text plugin.
///
class UsdImagingText {
public:
    typedef TfHashMap<TfToken, std::string, TfToken::HashFunctor> TextSettingMap;

    /// If the text plugin is initialized.
    USDIMAGING_API
    static bool IsInitialized()
    {
        return _textSystem != nullptr;
    }

    /// Initialize the text plugin using a default text setting.
    USDIMAGING_API
    static bool DefaultInitialize();

    /// Initialize the text plugin using a text setting.
    USDIMAGING_API
    static bool Initialize(const TextSettingMap&);

    /// Generate the geometry for markupText.
    USDIMAGING_API
    static bool GenerateMarkupTextGeometries(UsdImagingTextRendererSharedPtr renderer, 
                                             std::shared_ptr<UsdImagingMarkupText> markupText,
                                             VtVec3fArray& geometries,
                                             VtVec4fArray& textCoords, 
                                             VtVec3fArray& textColor, 
                                             VtFloatArray& textOpacity,
                                             VtVec3fArray& lineColors, 
                                             VtFloatArray& lineOpacities, 
                                             VtVec3fArray& lineGeometries);

    /// Generate the geometry for simple text.
    USDIMAGING_API
    static bool GenerateSimpleTextGeometries(UsdImagingTextRendererSharedPtr renderer, 
                                             const std::string& textData,
                                             const UsdImagingTextStyle& style, 
                                             VtVec3fArray& geometries,
                                             VtVec4fArray& textCoords, 
                                             VtVec3fArray& lineGeometries);

protected:
    USDIMAGING_API
    UsdImagingText() = default;
    USDIMAGING_API
    virtual ~UsdImagingText();

private:
    friend class UsdImagingTextRegistry;

    // This class doesn't require copy support.
    UsdImagingText(const UsdImagingText &)             = delete;
    UsdImagingText &operator =(const UsdImagingText &) = delete;

    /// Initialize the text plugin using a text setting.
    USDIMAGING_API
    virtual bool _Initialize(const TextSettingMap&) = 0;

    /// Generate the geometry for markupText.
    USDIMAGING_API
    virtual bool _GenerateMarkupTextGeometries(UsdImagingTextRendererSharedPtr renderer,
                                               std::shared_ptr<UsdImagingMarkupText> markupText,
                                               VtVec3fArray& geometries,
                                               VtVec4fArray& textCoords,
                                               VtVec3fArray& textColor,
                                               VtFloatArray& textOpacity,
                                               VtVec3fArray& lineColors,
                                               VtFloatArray& lineOpacities,
                                               VtVec3fArray& lineGeometries) = 0;

    /// Generate the geometry for simple text.
    USDIMAGING_API
    virtual bool _GenerateSimpleTextGeometries(UsdImagingTextRendererSharedPtr renderer, 
                                               const std::string& textData,
                                               const UsdImagingTextStyle& style, 
                                               VtVec3fArray& geometries,
                                               VtVec4fArray& textCoords, 
                                               VtVec3fArray& lineGeometries) = 0;


    static UsdImagingTextSharedPtr _textSystem;
    static std::mutex _initializeMutex;
};

/// \class UsdImagingTextFactoryBase
///
/// Base class for the factory of UsdImagingText.
///
class UsdImagingTextFactoryBase : public TfType::FactoryBase {
public:
    virtual UsdImagingTextSharedPtr New() const = 0;
};

/// \class UsdImagingTextFactory
///
/// The factory to create UsdImagingText.
///
template <class T>
class UsdImagingTextFactory : public UsdImagingTextFactoryBase {
public:
    virtual UsdImagingTextSharedPtr New() const
    {
        return UsdImagingTextSharedPtr(new T);
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_USD_IMAGING_USD_IMAGING_TEXT_H
