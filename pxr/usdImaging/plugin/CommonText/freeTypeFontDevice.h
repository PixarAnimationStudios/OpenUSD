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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_FREETYPE_FONT_DEVICE_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_FREETYPE_FONT_DEVICE_H

#include "definitions.h"
#include "fontDevice.h"
#include "freeTypeFontDeviceImp.h"
#include "freeTypeFontList.h"

#include <ft2build.h>
#include FT_FREETYPE_H

PXR_NAMESPACE_OPEN_SCOPE

/// \class CommonTextFreeTypeFontDevice
///
/// A FreeType Text implementation
///
class CommonTextFreeTypeFontDevice : public CommonTextTrueTypeFontDevice
{
protected:
    static bool _isInitialized;
    UsdImagingTextStyle _textStyle;
    bool _isAttributesApplied = false;
    std::shared_ptr<CommonTextFreeTypeFontDeviceImp> _fontDeviceImp;

public:
    /// Constructor.
    CommonTextFreeTypeFontDevice() = default;

    /// Release the Core Text resources.
    ~CommonTextFreeTypeFontDevice() override = default;

    /// Do initialization when first create CommonTextFreeTypeFontDevice.
    CommonTextStatus Initialize() override;

    /// Shutdown the font device.
    void ShutDown() override;

    /// If the font device is available.
    bool IsAvailable() const override { return _isInitialized; }

    /// The name of the font device.
    std::string Name() const override { return std::string("CommonTextFreeTypeFontDevice"); }

    /// Override the CommonTextTrueTypeFontDevice::clone().
    std::shared_ptr<CommonTextTrueTypeFontDevice> Clone() override;

    /// Implements applyTextStyle().
    /// \note If the typeface, weight and italic member is not changed, we can change the text
    /// style even _isAttributesApplied is true, because this will not impact on the
    /// implementation.
    CommonTextStatus ApplyTextStyle(const UsdImagingTextStyle& textStyle) override;

    /// Implements getTextStyle()
    UsdImagingTextStyle GetTextStyle() const override;

    /// Implement queryFullSize().
    CommonTextStatus QueryFullSize(int& fullSize) override;

    /// Implements queryGlyphIndices().
    /// \param asciiString The string we will get glyph indices.
    /// \param[out] arrayIndices The indices we get.
    CommonTextStatus QueryGlyphIndices(std::string asciiString, 
                                       unsigned short* arrayIndices) override;

    /// Implements queryGlyphIndices().
    /// \param unicodeString The string we will get glyph indices.
    /// \param[out] arrayIndices The indices we get.
    CommonTextStatus QueryGlyphIndices(std::wstring unicodeString, 
                                       unsigned short* arrayIndices) override;

    /// Get the glyph metrics of the specified glyph.
    CommonTextStatus QueryGlyphMetrics(short glyphIndex, 
                                       CommonTextGlyphMetrics& glyphMetrics) override;

    /// Implements IqueryFontMetrics().
    CommonTextStatus QueryFontMetrics(CommonTextFontMetrics& fontMetrics) override;

    /// Get the rasterized data of the specified glyph.
    CommonTextStatus QueryRasterizedData(short glyphIndex, 
                                         CommonTextGlyphMetrics& rasGlyphMetrics, 
                                         int& dataLength, 
                                         void* data) override;

    /// Implement queryTTRawGlyph().
    CommonTextStatus QueryTTRawGlyph(int glyphIndex, 
                                     CommonTextGlyphMetrics& ttRawGlyphMetrics, 
                                     UsdImagingTextRawGlyph& ttRawGlyph) override;

    /// Verify if a specified font is installed.
    static bool IsFontInstalled(const std::string& strTypeface);
};
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_FREETYPE_FONT_DEVICE_H
