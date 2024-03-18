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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_FONT_DEVICE_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_FONT_DEVICE_H

#include "definitions.h"
#include "globals.h"
#include "metrics.h"
#include "system.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class CommonTextTrueTypeFontDevice
///
/// The wrapper of a font device which will provide font and glyph information.
///
class CommonTextTrueTypeFontDevice
{
public:
    virtual ~CommonTextTrueTypeFontDevice() = default;

    /// Initialize some global settings for the font device.
    /// \note This function must be called in single thread.
    virtual CommonTextStatus Initialize() = 0;

    /// Shutdown the font device.
    /// \note This function must be called in single thread.
    virtual void ShutDown() = 0;

    /// Check if the device available in the current OS.
    virtual bool IsAvailable() const = 0;

    /// The name of the font device.
    virtual std::string Name() const = 0;

    /// Clone the font device.
    virtual std::shared_ptr<CommonTextTrueTypeFontDevice> Clone() = 0;

    /// Set the font style.
    virtual CommonTextStatus ApplyTextStyle(const UsdImagingTextStyle& style) = 0;

    /// Get the font style
    virtual UsdImagingTextStyle GetTextStyle() const = 0;

    /// Get the full em size of the font.
    virtual CommonTextStatus QueryFullSize(int& fullSize) = 0;

    /// Query the glyph indices of the specified text string.
    /// \param asciiString The string we will get glyph indices.
    /// \param[out] arrayIndices The indices we get.
    /// \return -1 when failed
    virtual CommonTextStatus QueryGlyphIndices(std::string asciiString, 
                                               unsigned short* arrayIndices) = 0;

    /// Query the glyph indices of the specified text string.
    /// \param unicodeString The string we will get glyph indices.
    /// \param[out] arrayIndices The indices we get.
    /// \return -1 when failed
    virtual CommonTextStatus QueryGlyphIndices(std::wstring unicodeString, 
                                               unsigned short* arrayIndices) = 0;

    /// Get the font metrics.
    virtual CommonTextStatus QueryFontMetrics(CommonTextFontMetrics& metrics) = 0;

    /// Get the glyph metrics of the specified glyph.
    virtual CommonTextStatus QueryGlyphMetrics(short glyphIndex, 
                                               CommonTextGlyphMetrics& glyphMetrics) = 0;

    /// Get the rasterized data of the specified glyph.
    virtual CommonTextStatus QueryRasterizedData(short glyphIndex, 
                                                 CommonTextGlyphMetrics& rasGlyphMetrics, 
                                                 int& dataLength, 
                                                 void* data) = 0;

    /// Query raw glyph outlines of TrueType fonts.
    virtual CommonTextStatus QueryTTRawGlyph(int glyphIndex, 
                                             CommonTextGlyphMetrics& ttRawGlyphMetrics, 
                                             UsdImagingTextRawGlyph& ttRawGlyph) = 0;
};

/// \enum class CommonTextAntialiasOption
///
/// The antialias option used in the texture-based rendering.
///
enum class CommonTextAntialiasOption
{
    /// Use the gray scale bitmap got from font device.
    CommonTextGrayBitmap,

    /// GrayScale ClearType-like bitmap.
    CommonTextGrayScaleClearType,

    /// Colored ClearType-like bitmap.
    CommonTextClearType
};

/// \class CommonTextTrueTypeFontDevicePtr
///
/// A wrapper to the CommonTextTrueTypeFontDevice.
///
class CommonTextTrueTypeFontDevicePtr
{
private:
    std::shared_ptr<CommonTextTrueTypeFontDevice> _fontDevice = nullptr;
    UsdImagingTextStyle _textStyle;

public:
    /// The default constructor.
    inline CommonTextTrueTypeFontDevicePtr()
    {
    }

    /// The constructor from a style.
    inline CommonTextTrueTypeFontDevicePtr(const UsdImagingTextStyle& style)
    {
        _fontDevice = CommonTextSystem::Instance()->GetFontDevice(style);
        _textStyle  = style;
    }

    /// The copy constructor is not allowed.
    CommonTextTrueTypeFontDevicePtr(const CommonTextTrueTypeFontDevicePtr& other) = delete;

    /// The move constructor. The original object will be invalid.
    inline CommonTextTrueTypeFontDevicePtr(
        CommonTextTrueTypeFontDevicePtr&& other) noexcept
        : _fontDevice(std::move(other._fontDevice))
        , _textStyle(std::move(other._textStyle))
    {
        other._fontDevice = nullptr;
    }

    /// The destructor.
    /// Will return the font device to the system.
    inline ~CommonTextTrueTypeFontDevicePtr()
    {
        if (_fontDevice)
        {
            CommonTextSystem::Instance()->ReturnFontDevice(_textStyle, _fontDevice);
            _fontDevice = nullptr;
        }
    }

    /// The copy assignment is not allowed.
    CommonTextTrueTypeFontDevicePtr& operator=(const CommonTextTrueTypeFontDevicePtr& other) = delete;

    /// The move assignment. The original object will be invalid.
    inline CommonTextTrueTypeFontDevicePtr& operator=(CommonTextTrueTypeFontDevicePtr&& other) noexcept
    {
        _fontDevice       = std::move(other._fontDevice);
        other._fontDevice = nullptr;
        _textStyle        = std::move(other._textStyle);
        return *this;
    }

    /// Member access operator.
    inline CommonTextTrueTypeFontDevice* operator->() const
    {
        if (_fontDevice)
        {
            return _fontDevice.get();
        }
        else
            return nullptr;
    }

    /// Test if this object is valid.
    inline bool IsValid() const { return _fontDevice != nullptr; }

    /// Initialize the text style. 
    inline bool Initialize(const UsdImagingTextStyle& style)
    {
        if (!_fontDevice)
        {
            _fontDevice = CommonTextSystem::Instance()->GetFontDevice(style);
            _textStyle  = style;
            return true;
        }
        else
            return false;
    }

    /// Get the textStyle.
    inline const UsdImagingTextStyle& GetStyle() const {
        return _textStyle;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE
#endif // PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_FONT_DEVICE_H
