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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_FREETYPE_FONT_DEVICE_IMP_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_FREETYPE_FONT_DEVICE_IMP_H

#include "definitions.h"
#include "metrics.h"

#include <ft2build.h>
#include FT_FREETYPE_H

PXR_NAMESPACE_OPEN_SCOPE
struct CommonTextFontMetrics;

struct CommonTextGlyphMetrics;

/// \class CommonTextFreeTypeFontDeviceImp
///
/// A FreeType font device implementation.
///
class CommonTextFreeTypeFontDeviceImp
{
protected:
    /// Each CommonTextFreeTypeFontDevice will have a separate FT_Library.
    FT_Library _library = 0;

    FT_Face _face = 0;

    /// The DPI of the display.
    FT_UInt _horizontalDPI = 0;
    FT_UInt _verticalDPI   = 0;

    /// The current size of the font face.
    int _currentSize          = 0;
    float _currentWidthFactor = 1.0f;

public:
    /// Constructor.
    CommonTextFreeTypeFontDeviceImp();

    /// Detructor
    ~CommonTextFreeTypeFontDeviceImp();

    /// Apply the specified font attributes to the AFontDevice.
    /// Then, the further queries are all corresponding to those font attributes.
    /// Such treatment could improve the performance of our AFontDevice.
    CommonTextStatus ApplyFontAttributes(const std::string& path,
                                         long index);

    /// Query the full size of the Emsquare.
    /// \returns -1 when failed
    /// \note The size must be specified.
    CommonTextStatus QueryFullSize(int size,
                                   float widthFactor,
                                   int& fullSize);

    /// Query the glyph indices of the specified text string.
    /// \param unicodeString The string we will get glyph indices.
    /// \param[out] arrayIndices The indices we get.
    /// \returns -1 when failed<
    /// \note As the size will not impact on the glyph index, this function doesn't require to
    /// specify the size.
    CommonTextStatus QueryGlyphIndices(const std::wstring& unicodeString,
                                       unsigned short* arrayIndices);

    /// Query the glyph indices of the specified text string.
    /// \param asciiString The string we will get glyph indices.
    /// \param[out] arrayIndices The indices we get.
    /// \returns -1 when failed<
    /// \note As the size will not impact on the glyph index, this function doesn't require to
    /// specify the size.
    CommonTextStatus QueryGlyphIndices(const std::string& asciiString,
                                       unsigned short* arrayIndices);

    /// Query the font metrics for the text style in the current context.
    /// \returns -1 when failed<
    /// \note The size must be specified.
    CommonTextStatus QueryFontMetrics(int size, 
                                      float widthFactor, 
                                      CommonTextFontMetrics& fontMetrics);

    /// Query the glyph metrics for the input character or glyph index.
    /// \param size The size of the font we want.
    /// \param widthFactor The width factor of the font we want.
    /// \param glyphIndex The glyph index we want to get the metrics.
    /// \param[out] glyphMetrics The metrics we get.
    /// \returns -1 when failed
    /// \note The size must be specified.
    CommonTextStatus QueryGlyphMetrics(int size, 
                                       float widthFactor, 
                                       const int glyphIndex,
                                       CommonTextGlyphMetrics& glyphMetrics);

    /// The 'UsdImagingTextStyle.size' specifies the height of the characters in the 2d or 3d world
    /// coordinates. To avoid confusion, here we do not use the 'UsdImagingTextStyle.size' to depict the
    /// resolution of the rasterized glyph. Please use 'RasterizedResolution()' to set the
    /// resolution of the glyph rasterization.
    /// \param size The size of the font we want.
    /// \param widthFactor The width factor of the font we want.
    /// \param glyphIndex The glyph index we want to get the rasterized data.
    /// \param[out] dataLength The length of data we need to allocated for pData.
    /// \param[out] rasGlyphMetrics The metrics we get.
    /// \param[out] pData The texture data that will be filled. Can be nullptr.
    /// \returns -1 when failed
    /// \note In order to support complex script, we provide the querying by glyph index. So this
    /// method is revised to fulfill this requirement. The size must be specified in this function.
    CommonTextStatus QueryRasterizedGlyph(int size, 
                                          float widthFactor, 
                                          const int glyphIndex,
                                          int& dataLength,
                                          CommonTextGlyphMetrics& rasGlyphMetrics,
                                          void* pData);

    /// Query raw glyph outlines of TrueType fonts.
    /// \note The size must be specified.
    CommonTextStatus QueryTTRawGlyph(int size, 
                                     float widthFactor, 
                                     const int glyphIndex,
                                     CommonTextGlyphMetrics& ttRawGlyphMetrics,
                                     UsdImagingTextRawGlyph& ttRawGlyph);

    /// Get the ranges of unicode supported by this text style.
    CommonTextStatus QueryUnicodeRanges(CommonTextFontUnicodeRanges& ranges);

private:
    /// Release resources.
    void _ReleaseResources();

    /// Parse the components information of the glyph
    CommonTextStatus _ParseGlyphComponents(int indexToLocFormat,
                                           int index,
                                           UsdImagingTextRawGlyph& ttRawGlyph);
};
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_FREETYPE_FONT_DEVICE_IMP_H
