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

#include "freeTypeFontDevice.h"
#include "freeTypeFontDeviceImp.h"
#include <freetype/ftbitmap.h>
#include <freetype/ftstroke.h>

PXR_NAMESPACE_OPEN_SCOPE

bool CommonTextFreeTypeFontDevice::_isInitialized = false;

CommonTextStatus
CommonTextFreeTypeFontDevice::Initialize()
{
    // Initialize the freetype fontlist.
    if (_isInitialized || CommonTextFreeTypeFontList::InitInstance())
    {
        _isInitialized = true;
        return CommonTextStatus::CommonTextStatusSuccess;
    }
    else
        return CommonTextStatus::CommonTextStatusNotInitialized;
}

void
CommonTextFreeTypeFontDevice::ShutDown()
{
    if (_isInitialized)
    {

        CommonTextFreeTypeFontList::ReleaseInstance();
        _isInitialized = false;
    }
}

std::shared_ptr<CommonTextTrueTypeFontDevice>
CommonTextFreeTypeFontDevice::Clone()
{
    // All the members are cloned.
    std::shared_ptr<CommonTextFreeTypeFontDevice> pNewFontDevice = std::make_shared<CommonTextFreeTypeFontDevice>();
    pNewFontDevice->ApplyTextStyle(_textStyle);

    // The _fontDeviceImp is shared. So if the original font device has applied the
    // attributes, the _isAttributesApplied of the new font device is also true.
    pNewFontDevice->_fontDeviceImp       = _fontDeviceImp;
    pNewFontDevice->_isAttributesApplied = _isAttributesApplied;
    return pNewFontDevice;
}

CommonTextStatus
CommonTextFreeTypeFontDevice::ApplyTextStyle(const UsdImagingTextStyle& textStyle)
{
    if (!_isAttributesApplied || textStyle._typeface != _textStyle._typeface ||
        textStyle._italic != _textStyle._italic || textStyle._underlineType != _textStyle._underlineType ||
        textStyle._overlineType != _textStyle._overlineType ||
        textStyle._strikethroughType != _textStyle._strikethroughType )
    {
        _textStyle = textStyle;
    }
    else
        return CommonTextStatus::CommonTextStatusSuccess;

    // If the implementation is not created, create the implementation.
    // If the implementation is shared by other font devices, recreate the implementation.
    if (!_fontDeviceImp)
    {
        _fontDeviceImp = std::make_shared<CommonTextFreeTypeFontDeviceImp>();
    }

    // Find the font file path and index of font.
    std::string filePath;
    long fontIndex = 0;
    bool bold      = _textStyle._bold;
    bool italic    = _textStyle._italic;

    std::string typeface = _textStyle._typeface;

    if (!CommonTextFreeTypeFontList::GetInstance().FindFont(typeface, bold, italic, filePath, fontIndex))
    {
        // The suggested typeface is "Arial".
        _textStyle._typeface = "Arial";

        // TODO need to add warning here.
        return CommonTextStatus::CommonTextStatusFontNotFound;
    }
    else
    {
        // Apply the font attributes.
        CommonTextStatus status = _fontDeviceImp->ApplyFontAttributes(filePath, fontIndex);
        if (status == CommonTextStatus::CommonTextStatusSuccess)
        {
            _isAttributesApplied = true;
            return CommonTextStatus::CommonTextStatusSuccess;
        }
        else
        {
            // The suggested typeface is "Arial".
            _textStyle._typeface = "Arial";
            return status;
        }
    }
}

UsdImagingTextStyle
CommonTextFreeTypeFontDevice::GetTextStyle() const
{
    return _textStyle;
}

CommonTextStatus
CommonTextFreeTypeFontDevice::QueryFullSize(int& fullSize)
{
    if (!_isAttributesApplied)
    {
        return CommonTextStatus::CommonTextStatusNotInitialized;
    }

    return _fontDeviceImp->QueryFullSize(_textStyle._height, _textStyle._widthFactor, fullSize);
}

CommonTextStatus
CommonTextFreeTypeFontDevice::QueryGlyphIndices(
    std::string asciiString,
    unsigned short* arrayIndices)
{
    if (!_isAttributesApplied)
    {
        return CommonTextStatus::CommonTextStatusNotInitialized;
    }

    return _fontDeviceImp->QueryGlyphIndices(asciiString, arrayIndices);
}

CommonTextStatus
CommonTextFreeTypeFontDevice::QueryGlyphIndices(
    std::wstring unicodeString,
    unsigned short* arrayIndices)
{
    if (!_isAttributesApplied)
    {
        return CommonTextStatus::CommonTextStatusNotInitialized;
    }

    return _fontDeviceImp->QueryGlyphIndices(unicodeString, arrayIndices);
}

CommonTextStatus
CommonTextFreeTypeFontDevice::QueryFontMetrics(CommonTextFontMetrics& fontMetrics)
{
    if (!_isAttributesApplied)
    {
        return CommonTextStatus::CommonTextStatusNotInitialized;
    }

    return _fontDeviceImp->QueryFontMetrics(
        _textStyle._height, _textStyle._widthFactor, fontMetrics);
}

CommonTextStatus
CommonTextFreeTypeFontDevice::QueryGlyphMetrics(
    short glyphIndex,
    CommonTextGlyphMetrics& glyphMetrics)
{
    if (!_isAttributesApplied)
    {
        return CommonTextStatus::CommonTextStatusNotInitialized;
    }

    return _fontDeviceImp->QueryGlyphMetrics(
        _textStyle._height, _textStyle._widthFactor, glyphIndex, glyphMetrics);
}

CommonTextStatus
CommonTextFreeTypeFontDevice::QueryRasterizedData(
    short glyphIndex,
    CommonTextGlyphMetrics& rasGlyphMetrics,
    int& dataLength,
    void* data)
{
    if (!_isAttributesApplied)
    {
        return CommonTextStatus::CommonTextStatusNotInitialized;
    }

    return _fontDeviceImp->QueryRasterizedGlyph(
        _textStyle._height, _textStyle._widthFactor, glyphIndex, dataLength, rasGlyphMetrics, data);
}

CommonTextStatus
CommonTextFreeTypeFontDevice::QueryTTRawGlyph(
    const int character,
    CommonTextGlyphMetrics& ttRawGlyphMetrics,
    UsdImagingTextRawGlyph& ttRawGlyph)
{
    if (!_isAttributesApplied)
    {
        return CommonTextStatus::CommonTextStatusNotInitialized;
    }

    return _fontDeviceImp->QueryTTRawGlyph(
        _textStyle._height, _textStyle._widthFactor, character, ttRawGlyphMetrics, ttRawGlyph);
}

bool
CommonTextFreeTypeFontDevice::IsFontInstalled(const std::string& strTypeface)
{
    bool ifBold   = false;
    bool ifItalic = false;
    long index    = 0;
    std::string filePath;
    return CommonTextFreeTypeFontList::GetInstance().FindFont(strTypeface, ifBold, ifItalic, filePath, index);
}
PXR_NAMESPACE_CLOSE_SCOPE
