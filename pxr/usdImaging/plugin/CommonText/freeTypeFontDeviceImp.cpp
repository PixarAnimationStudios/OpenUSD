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

#include "freeTypeFontDeviceImp.h"

#if defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IPHONE
        #include <map>
        #include <sys/utsname.h>
        #include <UIKit/UIKit.h>
    #elif TARGET_OS_MAC
        #include <ApplicationServices/ApplicationServices.h>
        #include <CoreFoundation/CFDictionary.h>
        #include <CoreFoundation/CFNumber.h>
        #include <IOKit/graphics/IOGraphicsLib.h>
    #else
    #   error "Unknown Apple platform"
    #endif
#endif

#if defined(__linux__)
#include <X11/Xlib.h>
#endif

#include <freetype/ftbitmap.h>
#include <freetype/ftstroke.h>
#include <freetype/tttables.h>
#include <freetype/tttags.h>

PXR_NAMESPACE_OPEN_SCOPE

// TODO: in the future, we may need to handle multi-display problem.
#if defined(_WIN32)
int 
GetDisplayDPI(double& horizontalDPI, 
              double& verticalDPI)
{
    HDC tempDC = CreateCompatibleDC(nullptr);
    if (!tempDC)
        return -1;

    horizontalDPI = GetDeviceCaps(tempDC, LOGPIXELSX);
    verticalDPI   = GetDeviceCaps(tempDC, LOGPIXELSY);
    DeleteDC(tempDC);
    return 0;
}

#elif defined(__APPLE__)
int 
GetDisplayDPI(double& horizontalDPI, 
              double& verticalDPI)
{
#if TARGET_OS_IPHONE
    const NSDictionary *deviceNamesByCode = @{
        // iPhone
        @"iPhone8,1" : @"iPhone 6S",
        @"iPhone8,2" : @"iPhone 6S Plus",
        @"iPhone8,4" : @"iPhone SE",
        @"iPhone9,1" : @"iPhone 7",
        @"iPhone9,3" : @"iPhone 7",
        @"iPhone9,2" : @"iPhone 7 Plus",
        @"iPhone9,4" : @"iPhone 7 Plus",
        @"iPhone10,1": @"iPhone 8",
        @"iPhone10,4": @"iPhone 8",
        @"iPhone10,2": @"iPhone 8 Plus",
        @"iPhone10,5": @"iPhone 8 Plus",
        @"iPhone10,3": @"iPhone X",
        @"iPhone10,6": @"iPhone X",
        @"iPhone11,2": @"iPhone XS",
        @"iPhone11,4": @"iPhone XS Max",
        @"iPhone11,6": @"iPhone XS Max",
        @"iPhone11,8": @"iPhone XR",
        @"iPhone12,1": @"iPhone 11",
        @"iPhone12,3": @"iPhone 11 Pro",
        @"iPhone12,5": @"iPhone 11 Pro Max",
        @"iPhone12,8": @"iPhone SE (2nd Gen)",
        @"iPhone13,1": @"iPhone 12 Mini",
        @"iPhone13,2": @"iPhone 12",
        @"iPhone13,3": @"iPhone 12 Pro",
        @"iPhone13,4": @"iPhone 12 Pro Max",
        @"iPhone14,2": @"iPhone 13 Pro",
        @"iPhone14,3": @"iPhone 13 Pro Max",
        @"iPhone14,4": @"iPhone 13 Mini",
        @"iPhone14,5": @"iPhone 13",
        @"iPhone14,6": @"iPhone SE (3rd Gen)",
        @"iPhone14,7": @"iPhone 14",
        @"iPhone14,8": @"iPhone 14 Plus",
        @"iPhone15,2": @"iPhone 14 Pro",
        @"iPhone15,3": @"iPhone 14 Pro Max",

        // iPad
        @"iPad5,1"  : @"iPad Mini 4",
        @"iPad5,2"  : @"iPad Mini 4",
        @"iPad5,3"  : @"iPad Air 2",
        @"iPad5,4"  : @"iPad Air 2",
        @"iPad6,3"  : @"iPad Pro 9.7-in.",
        @"iPad6,4"  : @"iPad Pro 9.7-in.",
        @"iPad6,7"  : @"iPad Pro 12.9-in.",
        @"iPad6,8"  : @"iPad Pro 12.9-in.",
        @"iPad6,11" : @"iPad 5",
        @"iPad6,12" : @"iPad 5",
        @"iPad7,1"  : @"iPad Pro 12.9-in. (2nd Gen)",
        @"iPad7,2"  : @"iPad Pro 12.9-in. (2nd Gen)",
        @"iPad7,3"  : @"iPad Pro 10.5-in.",
        @"iPad7,4"  : @"iPad Pro 10.5-in.",
        @"iPad7,5"  : @"iPad 6",
        @"iPad7,6"  : @"iPad 6",
        @"iPad7,11" : @"iPad 7",
        @"iPad7,12" : @"iPad 7",
        @"iPad8,1"  : @"iPad Pro 11-in.",
        @"iPad8,2"  : @"iPad Pro 11-in.",
        @"iPad8,3"  : @"iPad Pro 11-in.",
        @"iPad8,4"  : @"iPad Pro 11-in.",
        @"iPad8,5"  : @"iPad Pro 12.9-in. (3rd Gen)",
        @"iPad8,6"  : @"iPad Pro 12.9-in. (3rd Gen)",
        @"iPad8,7"  : @"iPad Pro 12.9-in. (3rd Gen)",
        @"iPad8,8"  : @"iPad Pro 12.9-in. (3rd Gen)",
        @"iPad8,9"  : @"iPad Pro 11-in. (2nd Gen)",
        @"iPad8,10" : @"iPad Pro 11-in. (2nd Gen)",
        @"iPad8,11" : @"iPad Pro 12.9-in. (4th Gen)",
        @"iPad8,12" : @"iPad Pro 12.9-in. (4th Gen)",
        @"iPad11,1" : @"iPad Mini 5",
        @"iPad11,2" : @"iPad Mini 5",
        @"iPad11,3" : @"iPad Air 3",
        @"iPad11,4" : @"iPad Air 3",
        @"iPad11,6" : @"iPad 8",
        @"iPad11,7" : @"iPad 8",
        @"iPad12,1" : @"iPad 9",
        @"iPad12,2" : @"iPad 9",
        @"iPad13,1" : @"iPad Air 4",
        @"iPad13,2" : @"iPad Air 4",
        @"iPad13,4" : @"iPad Pro 11-in. (3rd Gen)",
        @"iPad13,5" : @"iPad Pro 11-in. (3rd Gen)",
        @"iPad13,6" : @"iPad Pro 11-in. (3rd Gen)",
        @"iPad13,7" : @"iPad Pro 11-in. (3rd Gen)",
        @"iPad13,8" : @"iPad Pro 12.9-in. (5th Gen)",
        @"iPad13,9" : @"iPad Pro 12.9-in. (5th Gen)",
        @"iPad13,10": @"iPad Pro 12.9-in. (5th Gen)",
        @"iPad13,11": @"iPad Pro 12.9-in. (5th Gen)",
        @"iPad13,16": @"iPad Air 5",
        @"iPad13,17": @"iPad Air 5",
        @"iPad13,18": @"iPad 10",
        @"iPad13,19": @"iPad 10",
        @"iPad14,1" : @"iPad Mini 6",
        @"iPad14,2" : @"iPad Mini 6",
        @"iPad14,3" : @"iPad Pro 11-in. (4th Gen)",
        @"iPad14,4" : @"iPad Pro 11-in. (4th Gen)",
        @"iPad14,5" : @"iPad Pro 12.9-in. (6th Gen)",
        @"iPad14,6" : @"iPad Pro 12.9-in. (6th Gen)"};

    struct utsname systemInfo;
    uname(&systemInfo);
    NSString *deviceModel = [NSString stringWithCString:systemInfo.machine encoding:NSUTF8StringEncoding];
    NSString *device = deviceNamesByCode[deviceModel];

    if (device) {
        const std::map<std::string, std::pair<double, double>> DEFAULT_APPLE_DPI = {
            // iPhone
            {"iPhone 6S", std::pair(2.3, 4.1)},
            {"iPhone 6S Plus", std::pair(2.7, 4.8)},
            {"iPhone SE", std::pair(1.9, 3.5)},
            {"iPhone 7", std::pair(2.3, 4.1)},
            {"iPhone 7 Plus", std::pair(2.7, 4.8)},
            {"iPhone 8", std::pair(2.3, 4.1)},
            {"iPhone 8 Plus", std::pair(2.7, 4.8)},
            {"iPhone X", std::pair(2.45, 5.31)},
            {"iPhone XS", std::pair(2.45, 5.31)},
            {"iPhone XS Max", std::pair(2.73, 5.9)},
            {"iPhone XR", std::pair(2.56, 5.54)},
            {"iPhone 11", std::pair(2.56, 5.54)},
            {"iPhone 11 Pro", std::pair(2.45, 5.31)},
            {"iPhone 11 Pro Max", std::pair(2.73, 5.9)},
            {"iPhone SE (2nd Gen)", std::pair(2.3, 4.1)},
            {"iPhone 12 Mini", std::pair(2.27, 4.92)},
            {"iPhone 12", std::pair(2.56, 5.54)},
            {"iPhone 12 Pro", std::pair(2.56, 5.54)},
            {"iPhone 12 Pro Max", std::pair(2.8, 6.06)},
            {"iPhone 13 Pro", std::pair(2.56, 5.54)},
            {"iPhone 13 Pro Max", std::pair(2.8, 6.06)},
            {"iPhone 13 Mini", std::pair(2.27, 4.92)},
            {"iPhone 13", std::pair(2.56, 5.54)},
            {"iPhone SE (3rd Gen)", std::pair(2.3, 4.1)},
            {"iPhone 14", std::pair(2.56, 5.54)},
            {"iPhone 14 Plus", std::pair(2.8, 6.06)},
            {"iPhone 14 Pro", std::pair(2.56, 5.56)},
            {"iPhone 14 Pro Max", std::pair(2.8, 6.07)},

            // iPad
            {"iPad Mini 4", std::pair(4.78, 6.35)},
            {"iPad Air 2", std::pair(6.05, 8.0)},
            {"iPad Pro 9.7-in.", std::pair(5.8, 7.8)},
            {"iPad Pro 12.9-in.", std::pair(7.8, 10.3)},
            {"iPad 5", std::pair(5.83, 7.74)},
            {"iPad Pro 12.9-in. (2nd Gen)", std::pair(7.8, 10.3)},
            {"iPad Pro 10.5-in.", std::pair(6.3, 8.4)},
            {"iPad 6", std::pair(5.83, 7.74)},
            {"iPad 7", std::pair(6.12, 8.16)},
            {"iPad Pro 11-in.", std::pair(6.3, 9.0)},
            {"iPad Pro 12.9-in. (3rd Gen)", std::pair(7.8, 10.3)},
            {"iPad Pro 11-in. (2nd Gen)", std::pair(6.3, 9.0)},
            {"iPad Pro 12.9-in. (4th Gen)", std::pair(7.8, 10.3)},
            {"iPad Mini 5", std::pair(4.76, 6.33)},
            {"iPad Air 3", std::pair(6.3, 8.4)},
            {"iPad 8", std::pair(6.12, 8.16)},
            {"iPad 9", std::pair(6.12, 8.16)},
            {"iPad Air 4", std::pair(6.24, 8.96)},
            {"iPad Pro 11-in. (3rd Gen)", std::pair(6.3, 9.0)},
            {"iPad Pro 12.9-in. (5th Gen)", std::pair(7.8, 10.3)},
            {"iPad Air 5", std::pair(6.24, 8.96)},
            {"iPad 10", std::pair(6.21, 8.94)},
            {"iPad Mini 6", std::pair(4.56, 6.95)},
            {"iPad Pro 11-in. (4th Gen)", std::pair(6.3, 9.0)},
            {"iPad Pro 12.9-in. (6th Gen)", std::pair(7.8, 10.3)},
        };

        // It's suggested to use real device for testing effect corresponding to DPI,
        // please refer to OGSMOD-2909 for known limitations on My Mac and simulators
        CGFloat displayWidthInPixels = UIScreen.mainScreen.nativeBounds.size.width;
        CGFloat displayHeightInPixels = UIScreen.mainScreen.nativeBounds.size.height;

        horizontalDPI = displayWidthInPixels / DEFAULT_APPLE_DPI.at([device UTF8String]).first;
        verticalDPI = displayHeightInPixels / DEFAULT_APPLE_DPI.at([device UTF8String]).second;
    } else {
        // May be iPod, iWatch, simulator, or new iPhone / iPad devices
        NSLog(@"Unexpected device type %@.", deviceModel);
        horizontalDPI = 460;
        verticalDPI = 460;
    }

    return 0;
#elif TARGET_OS_MAC
    const double mmPerInch  = 25.4;
    CGSize displaySizeInMMs = CGDisplayScreenSize(kCGDirectMainDisplay);
    if (displaySizeInMMs.width == 0 || displaySizeInMMs.height == 0)
        return -1;
    size_t displayHeightInPixels  = CGDisplayPixelsHigh(kCGDirectMainDisplay);
    size_t displayWidthInPixels   = CGDisplayPixelsWide(kCGDirectMainDisplay);
    double horizontalSizeInInches = (double)displaySizeInMMs.width / mmPerInch;
    double verticalSizeInInches   = (double)displaySizeInMMs.height / mmPerInch;
    horizontalDPI                 = (double)displayWidthInPixels / horizontalSizeInInches;
    verticalDPI                   = (double)displayHeightInPixels / verticalSizeInInches;
    return 0;
#endif
}
#elif defined(__linux__)
int
GetDisplayDPI(double& horizontalDPI, 
              double& verticalDPI)
{
    assert(!"TODO: verify the implementation!"); //TODO
    int scr = 0;

    char* displayname = NULL;
    Display* dpy = XOpenDisplay(displayname);

    const double mmPerInch = 25.4;
    double xres =
        ((((double)DisplayWidth(dpy, scr)) * mmPerInch) / ((double)DisplayWidthMM(dpy, scr)));
    double yres =
        ((((double)DisplayHeight(dpy, scr)) * mmPerInch) / ((double)DisplayHeightMM(dpy, scr)));

    horizontalDPI = (int)(xres + 0.5);
    verticalDPI = (int)(yres + 0.5);

    XCloseDisplay(dpy);
    return 0;
}
#endif

#define FT_PIX_FLOOR(x) ((x) & ~63)
#define FT_PIX_ROUND(x) FT_PIX_FLOOR((x) + 32)
#define FT_PIX_CEIL(x) FT_PIX_FLOOR((x) + 63)

// DEFINE_NETOBJECTCOUNT(FreeTypeFontDeviceImp);

CommonTextFreeTypeFontDeviceImp::CommonTextFreeTypeFontDeviceImp()
{
    // Initialize the FT_Library.
    FT_Error error = FT_Init_FreeType(&_library);
    if (error != 0)
        return;

    // Get the DPI information of the display.
    double horizontalDPI, verticalDPI;
    if (GetDisplayDPI(horizontalDPI, verticalDPI) != 0)
    {
        // By default, set the DPI to 72.0.
        horizontalDPI = 72.0;
        verticalDPI   = 72.0;
    }
    _horizontalDPI = (FT_UInt)horizontalDPI;
    _verticalDPI   = (FT_UInt)verticalDPI;
}

CommonTextFreeTypeFontDeviceImp::~CommonTextFreeTypeFontDeviceImp()
{
    this->_ReleaseResources();

    // Release the library.
    if (_library != 0)
    {
        FT_Done_FreeType(_library);
        _library = 0;
    }
}

CommonTextStatus 
CommonTextFreeTypeFontDeviceImp::ApplyFontAttributes(
    const std::string& path, 
    long index)
{
    if (_library == 0)
        return CommonTextStatus::CommonTextStatusNotInitialized;

    this->_ReleaseResources();

    // Create the FT_Face.
    FT_Error error = FT_New_Face(_library, path.c_str(), index, &_face);
    if (error != 0)
        return CommonTextStatus::CommonTextStatusFontNotFound;

    // Set charmap for symbol font
    for (int i = 0; i < _face->num_charmaps; ++i)
    {
        if (_face->charmaps[i]->encoding == FT_ENCODING_MS_SYMBOL)
        {
            FT_Select_Charmap(_face, FT_ENCODING_MS_SYMBOL);
            break;
        }
    }
    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextStatus 
CommonTextFreeTypeFontDeviceImp::QueryFullSize(
    int size, 
    float widthFactor, 
    int& fullSize)
{
    if (_face == 0)
        return CommonTextStatus::CommonTextStatusFontNotFound;

    if (_currentSize != size || !IsFloatEqual(_currentWidthFactor, widthFactor))
    {
        // First we need to set the character size.
        int ptHorizontalSize = static_cast<int>(widthFactor * size * 72 * 64 / _horizontalDPI);
        int ptVerticalSize   = size * 72 * 64 / _verticalDPI;
        FT_Error error =
            FT_Set_Char_Size(_face, ptHorizontalSize, ptVerticalSize, _horizontalDPI, _verticalDPI);

        if (error != 0)
            return CommonTextStatus::CommonTextStatusFontNotFound;

        _currentSize        = size;
        _currentWidthFactor = widthFactor;
    }

    // The size of the EmSquare
    fullSize = _face->units_per_EM;
    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextStatus 
CommonTextFreeTypeFontDeviceImp::QueryGlyphIndices(
    const std::wstring& unicodeString,
    unsigned short* arrayIndices)
{
    if (_face == 0)
        return CommonTextStatus::CommonTextStatusFontNotFound;

    size_t charCount = unicodeString.size();
    for (size_t i = 0; i < charCount; ++i)
    {
        // Get the index of each character.
        arrayIndices[i] = (unsigned short)FT_Get_Char_Index(_face, unicodeString[i]);

        // For symbol font, we also need to check the character added by 0xf000.
        if (arrayIndices[i] == 0 && _face->charmap->encoding == FT_ENCODING_MS_SYMBOL)
            arrayIndices[i] = (unsigned short)FT_Get_Char_Index(_face, (unicodeString[i] + 0xf000));

        // FT_Get_Char_Index will return zero if the character has no glyph in the font,
        // but CommonTextSystem will regard TRUETYPE_MISSING_GLYPH_INDEX as the missing
        // character.
        if (arrayIndices[i] == 0)
            arrayIndices[i] = TRUETYPE_MISSING_GLYPH_INDEX;
    }

    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextStatus 
CommonTextFreeTypeFontDeviceImp::QueryGlyphIndices(
    const std::string& asciiString,
    unsigned short* arrayIndices)
{
    if (_face == 0)
        return CommonTextStatus::CommonTextStatusFontNotFound;

    size_t charCount = asciiString.size();
    for (size_t i = 0; i < charCount; ++i)
    {
        // Get the index of each character.
        arrayIndices[i] = (unsigned short)FT_Get_Char_Index(_face, asciiString[i]);

        // For symbol font, we also need to check the character added by 0xf000.
        if (arrayIndices[i] == 0 && _face->charmap->encoding == FT_ENCODING_MS_SYMBOL)
            arrayIndices[i] = (unsigned short)FT_Get_Char_Index(_face, (asciiString[i] + 0xf000));

        // FT_Get_Char_Index will return zero if the character has no glyph in the font,
        // but CommonTextSystem will regard TRUETYPE_MISSING_GLYPH_INDEX as the missing
        // character.
        if (arrayIndices[i] == 0)
            arrayIndices[i] = 0xff;
    }

    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextStatus 
CommonTextFreeTypeFontDeviceImp::QueryFontMetrics(
    int size, 
    float widthFactor, 
    CommonTextFontMetrics& fontMetrics)
{
    if (_face == 0)
        return CommonTextStatus::CommonTextStatusFontNotFound;

    if (_currentSize != size || !IsFloatEqual(_currentWidthFactor, widthFactor))
    {
        // First we need to set the character size.
        int ptHorizontalSize = static_cast<int>(widthFactor * size * 72 * 64 / _horizontalDPI);
        int ptVerticalSize   = size * 72 * 64 / _verticalDPI;
        FT_Error error =
            FT_Set_Char_Size(_face, ptHorizontalSize, ptVerticalSize, _horizontalDPI, _verticalDPI);

        if (error != 0)
            return CommonTextStatus::CommonTextStatusFontNotFound;

        _currentSize        = size;
        _currentWidthFactor = widthFactor;
    }

    // Get the font metrics.
    FT_Size_Metrics& scaledMetrics = _face->size->metrics;

    fontMetrics._emSquareSize       = _face->units_per_EM;
    fontMetrics._typographicAscent  = _face->ascender;
    fontMetrics._typographicDescent = -abs(_face->descender);
    // The scaledMetrics' unit is 1/64 pixel.
    fontMetrics._height  = scaledMetrics.height / 64;
    fontMetrics._ascent  = scaledMetrics.ascender / 64;
    fontMetrics._descent = -abs((int)(scaledMetrics.descender / 64));

    fontMetrics._internalLeading =
        scaledMetrics.y_ppem - fontMetrics._ascent + fontMetrics._descent;
    fontMetrics._externalLeading = 0;

    // The bbox is in design units, so convert it to pixels.
    fontMetrics._maxCharWidth =
        FT_PIX_ROUND(FT_MulFix(_face->bbox.xMax - _face->bbox.xMin, scaledMetrics.x_scale)) / 64;
    fontMetrics._defaultChar = L'?';

    // The capital height is the height of the character 'A'.
    FT_UInt charIndex = FT_Get_Char_Index(_face, 'A');
    if (charIndex != 0)
    {
        // Load the glyph of 'A'.
        if (FT_Load_Glyph(_face, charIndex, FT_LOAD_NO_SCALE) != 0)
        {
            return CommonTextStatus::CommonTextStatusCharacterNotFound;
        }
        FT_Glyph_Metrics& metrics = _face->glyph->metrics;

        // The metrics is in design units, so convert it to pixels.
        fontMetrics._capHeight =
            FT_PIX_ROUND(FT_MulFix(metrics.height, scaledMetrics.y_scale)) / 64;
        fontMetrics._emHeight = _face->units_per_EM;
    }
    else if (fontMetrics._typographicAscent == 0)
    {
        fontMetrics._emHeight  = fontMetrics._height - fontMetrics._internalLeading;
        fontMetrics._capHeight = fontMetrics._ascent - fontMetrics._internalLeading;
    }
    else if (fontMetrics._typographicAscent > fontMetrics._emSquareSize)
    {
        // The capheight is the same as the height.
        fontMetrics._emHeight  = fontMetrics._emSquareSize;
        fontMetrics._capHeight = fontMetrics._emSquareSize;
    }
    else
    {
        // Use the typograghic cap height.
        fontMetrics._emHeight  = fontMetrics._emSquareSize;
        fontMetrics._capHeight = fontMetrics._typographicAscent;
    }

    // The average width is the width of the character 'x'.
    // TODO: FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH is only for "NSimsun". In the future, this
    // flag should be configurable.
    charIndex = FT_Get_Char_Index(_face, 'x');
    if (charIndex != 0)
    {
        if (FT_Load_Glyph(
                _face, charIndex, FT_LOAD_NO_SCALE | FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH) != 0)
        {
            return CommonTextStatus::CommonTextStatusCharacterNotFound;
        }

        FT_Glyph_Metrics& metrics = _face->glyph->metrics;

        // The metrics is in design units, so convert it to pixels.
        fontMetrics._avgCharWidth =
            FT_PIX_ROUND(FT_MulFix(metrics.horiAdvance, scaledMetrics.x_scale)) / 64;
    }
    else
        // If the font has no English character, use the maximum advance width as the
        // average width.
        fontMetrics._avgCharWidth = _face->max_advance_width;

    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextStatus 
CommonTextFreeTypeFontDeviceImp::QueryGlyphMetrics(
    int size, 
    float widthFactor, 
    const int glyphIndex, 
    CommonTextGlyphMetrics& glyphMetrics)
{
    if (_face == 0)
        return CommonTextStatus::CommonTextStatusFontNotFound;

    if (_currentSize != size || !IsFloatEqual(_currentWidthFactor, widthFactor))
    {
        // First we need to set the character size.
        int ptHorizontalSize = static_cast<int>(widthFactor * size * 72 * 64 / _horizontalDPI);
        int ptVerticalSize   = size * 72 * 64 / _verticalDPI;
        FT_Error error =
            FT_Set_Char_Size(_face, ptHorizontalSize, ptVerticalSize, _horizontalDPI, _verticalDPI);

        if (error != 0)
            return CommonTextStatus::CommonTextStatusFontNotFound;

        _currentSize        = size;
        _currentWidthFactor = widthFactor;
    }

    FT_UInt glyph(glyphIndex);

    // Load the glyph.
    // TODO: FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH is only for "NSimsun". In the future, this
    // flag should be configurable.
    if (FT_Load_Glyph(_face, glyph, FT_LOAD_NO_SCALE | FT_LOAD_IGNORE_GLOBAL_ADVANCE_WIDTH) != 0)
    {
        return CommonTextStatus::CommonTextStatusCharacterNotFound;
    }

    // Get the glyph metrics.
    FT_Glyph_Metrics& metrics      = _face->glyph->metrics;
    FT_Size_Metrics& scaledMetrics = _face->size->metrics;

    // The metrics is in design units, so convert it to pixels.
    glyphMetrics._blackBoxX = FT_PIX_ROUND(FT_MulFix(metrics.width, scaledMetrics.x_scale)) / 64;
    glyphMetrics._blackBoxY = FT_PIX_ROUND(FT_MulFix(metrics.height, scaledMetrics.y_scale)) / 64;
    glyphMetrics._glyphOriginX =
        FT_PIX_ROUND(FT_MulFix(metrics.horiBearingX, scaledMetrics.x_scale)) / 64;
    glyphMetrics._glyphOriginY =
        FT_PIX_ROUND(FT_MulFix(metrics.horiBearingY, scaledMetrics.y_scale)) / 64;
    glyphMetrics._cellIncX =
        FT_PIX_ROUND(FT_MulFix(metrics.horiAdvance, scaledMetrics.x_scale)) / 64;
    glyphMetrics._cellIncY = 0;
    glyphMetrics._abcA = FT_PIX_ROUND(FT_MulFix(metrics.horiBearingX, scaledMetrics.x_scale)) / 64;
    glyphMetrics._abcB = FT_PIX_ROUND(FT_MulFix(metrics.width, scaledMetrics.x_scale)) / 64;
    glyphMetrics._abcC =
        FT_PIX_ROUND(FT_MulFix(
            metrics.horiAdvance - metrics.width - metrics.horiBearingX, scaledMetrics.x_scale)) /
        64;
    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextStatus 
CommonTextFreeTypeFontDeviceImp::QueryRasterizedGlyph(
    int size, 
    float widthFactor,
    const int glyphIndex,
    int& dataLength, 
    CommonTextGlyphMetrics& rasGlyphMetrics, 
    void* pData)
{
    if (_face == 0)
        return CommonTextStatus::CommonTextStatusFontNotFound;

    if (_currentSize != size || !IsFloatEqual(_currentWidthFactor, widthFactor))
    {
        // First we need to set the character size.
        int ptHorizontalSize = static_cast<int>(widthFactor * size * 72 * 64 / _horizontalDPI);
        int ptVerticalSize   = size * 72 * 64 / _verticalDPI;
        FT_Error error =
            FT_Set_Char_Size(_face, ptHorizontalSize, ptVerticalSize, _horizontalDPI, _verticalDPI);

        if (error != 0)
            return CommonTextStatus::CommonTextStatusFontNotFound;

        _currentSize        = size;
        _currentWidthFactor = widthFactor;
    }

    FT_UInt glyph(glyphIndex);

    // Load the glyph
    if (FT_Load_Glyph(_face, glyph, FT_LOAD_DEFAULT) == -1)
    {
        return CommonTextStatus::CommonTextStatusCharacterNotFound;
    }

    // Generate the bitmap.
    if (FT_Render_Glyph(_face->glyph, FT_RENDER_MODE_NORMAL) != 0)
    {
        return CommonTextStatus::CommonTextStatusFail;
    }

    // Convert the bitmap to the format we required.
    const FT_Bitmap& bitmap = _face->glyph->bitmap;

    int width  = bitmap.width;
    int height = bitmap.rows;

    if (!pData)
    {
        // The bbox is in pixels.
        rasGlyphMetrics._blackBoxX    = width;
        rasGlyphMetrics._blackBoxY    = height;
        rasGlyphMetrics._glyphOriginX = _face->glyph->bitmap_left;
        rasGlyphMetrics._glyphOriginY = _face->glyph->bitmap_top;
        dataLength                    = width * height;
        return CommonTextStatus::CommonTextStatusSuccess;
    }

    if (bitmap.pitch == width)
    {
        // Equals. Simply copy the bitmap.
        memcpy(pData, bitmap.buffer, dataLength);
    }
    else
    {
        unsigned char* src  = bitmap.buffer;
        unsigned char* dest = (unsigned char*)pData;

        int lineLength = (std::min)(width, bitmap.pitch);

        for (int y = 0; (unsigned int)y < bitmap.rows; y++)
        {
            memcpy(dest, src, lineLength);
            src += bitmap.pitch;
            dest += width;
        }
    }

    return CommonTextStatus::CommonTextStatusSuccess;
}

struct CommonTextFreeTypeFontDeviceOutlineIterator
{
    UsdImagingTextRawGlyph* _ttRawGlyph;
    bool _closedCurve;
    FT_Fixed _xscale;
    FT_Fixed _yscale;
};

// When we move to a new outline,
// we close the current curve and create a new curve.
static int
OutlineMoveFunction(const FT_Vector* to,
                    CommonTextFreeTypeFontDeviceOutlineIterator* it)
{
    if (!it->_closedCurve)
    {
        if (it->_ttRawGlyph)
            it->_ttRawGlyph->CloseCurve(false);
    }

    UsdImagingTextCtrlPoint tmpCtrlPoint;

    // The points of outline are in design units, we should convert them to pixels.
    tmpCtrlPoint._isOnCurve = true;
    tmpCtrlPoint._pos[0] = static_cast<float>(FT_PIX_ROUND(FT_MulFix(to->x, it->_xscale)) / 64.0f);
    tmpCtrlPoint._pos[1] = static_cast<float>(FT_PIX_ROUND(FT_MulFix(to->y, it->_yscale)) / 64.0f);
    if (it->_ttRawGlyph)
        it->_ttRawGlyph->AddPoint(tmpCtrlPoint);
    it->_closedCurve = false;
    return 0;
}

// Add a line to the outline.
static int 
OutlineLineFunction(const FT_Vector* to, 
                    CommonTextFreeTypeFontDeviceOutlineIterator* it)
{
    UsdImagingTextCtrlPoint tmpCtrlPoint;

    // The points of outline are in design units, we should convert them to pixels.
    tmpCtrlPoint._isOnCurve = true;
    tmpCtrlPoint._pos[0] = static_cast<float>(FT_PIX_ROUND(FT_MulFix(to->x, it->_xscale)) / 64.0f);
    tmpCtrlPoint._pos[1] = static_cast<float>(FT_PIX_ROUND(FT_MulFix(to->y, it->_yscale)) / 64.0f);
    if (it->_ttRawGlyph)
        it->_ttRawGlyph->AddPoint(tmpCtrlPoint);
    it->_closedCurve = false;
    return 0;
}

// Add a conic curve to the outline.
static int 
OutlineConicFunction(const FT_Vector* control, 
                     const FT_Vector* to,
                     CommonTextFreeTypeFontDeviceOutlineIterator* it)
{
    UsdImagingTextCtrlPoint tmpCtrlPoint;

    // The points of outline are in design units, we should convert them to pixels.
    tmpCtrlPoint._isOnCurve = false;
    tmpCtrlPoint._pos[0] =
        static_cast<float>(FT_PIX_ROUND(FT_MulFix(control->x, it->_xscale)) / 64.0f);
    tmpCtrlPoint._pos[1] =
        static_cast<float>(FT_PIX_ROUND(FT_MulFix(control->y, it->_yscale)) / 64.0f);
    if (it->_ttRawGlyph)
        it->_ttRawGlyph->AddPoint(tmpCtrlPoint);

    tmpCtrlPoint._isOnCurve = true;
    tmpCtrlPoint._pos[0] = static_cast<float>(FT_PIX_ROUND(FT_MulFix(to->x, it->_xscale)) / 64.0f);
    tmpCtrlPoint._pos[1] = static_cast<float>(FT_PIX_ROUND(FT_MulFix(to->y, it->_yscale)) / 64.0f);
    if (it->_ttRawGlyph)
        it->_ttRawGlyph->AddPoint(tmpCtrlPoint);

    it->_closedCurve = false;
    return 0;
}

// Add a cubic curve to the outline.
static int 
OutlineCubicFunction(const FT_Vector* control1, 
                     const FT_Vector* control2,
                     const FT_Vector* to, 
                     CommonTextFreeTypeFontDeviceOutlineIterator* it)
{
    UsdImagingTextCtrlPoint tmpCtrlPoint;

    // The points of outline are in design units, we should convert them to pixels.
    tmpCtrlPoint._isOnCurve = false;
    tmpCtrlPoint._pos[0] =
        static_cast<float>(FT_PIX_ROUND(FT_MulFix(control1->x, it->_xscale)) / 64.0f);
    tmpCtrlPoint._pos[1] =
        static_cast<float>(FT_PIX_ROUND(FT_MulFix(control1->y, it->_yscale)) / 64.0f);
    if (it->_ttRawGlyph)
        it->_ttRawGlyph->AddPoint(tmpCtrlPoint);

    tmpCtrlPoint._pos[0] =
        static_cast<float>(FT_PIX_ROUND(FT_MulFix(control2->x, it->_xscale)) / 64.0f);
    tmpCtrlPoint._pos[1] =
        static_cast<float>(FT_PIX_ROUND(FT_MulFix(control2->y, it->_yscale)) / 64.0f);
    if (it->_ttRawGlyph)
        it->_ttRawGlyph->AddPoint(tmpCtrlPoint);

    tmpCtrlPoint._isOnCurve = true;
    tmpCtrlPoint._pos[0] = static_cast<float>(FT_PIX_ROUND(FT_MulFix(to->x, it->_xscale)) / 64.0f);
    tmpCtrlPoint._pos[1] = static_cast<float>(FT_PIX_ROUND(FT_MulFix(to->y, it->_yscale)) / 64.0f);
    if (it->_ttRawGlyph)
        it->_ttRawGlyph->AddPoint(tmpCtrlPoint);

    it->_closedCurve = false;
    return 0;
}

// A list of enumerations used when we parse the font table.
enum class CommonTextEParseOptions
{
    kARG_1_AND_2_ARE_WORDS =
        0x0001, // If this is set, the arguments are words; otherwise, they are bytes.
    kARGS_ARE_XY_VALUES =
        0x0002, // If this is set, the arguments are xy values; otherwise, they are points.
    kROUND_XY_TO_GRID = 0x0004, // For the xy values if the preceding is true.
    kWE_HAVE_A_SCALE  = 0x0008, // This indicates that there is a simple scale for the component.
                                // Otherwise, scale = 1.0.

    kRESERVED        = 0x0010, // This bit is reserved. Set it to 0.
    kMORE_COMPONENTS = 0x0020, // Indicates at least one more glyph after this one.
    kWE_HAVE_AN_X_AND_Y_SCALE =
        0x0040, // The x direction will use a different scale from the y direction.
    kWE_HAVE_A_TWO_BY_TWO =
        0x0080, // There is a 2 by 2 transformation that will be used to scale the component.

    kWE_HAVE_INSTRUCTIONS =
        0x0100, // Following the last component are instructions for the composite character.
    kUSE_MY_METRICS =
        0x0200, // If set, this forces the aw and lsb (and rsb) for the composite to be equal to
                // those from this original glyph. This works for hinted and unhinted characters.
    kOVERLAP_COMPOUND        = 0x0400, // Used by Apple in GX fonts.
    kSCALED_COMPONENT_OFFSET = 0x0800, // Composite designed to have the component offset scaled
                                       // (designed for Apple rasterizer).

    kUNSCALED_COMPONENT_OFFSET = 0x1000, // Composite designed not to have the component offset
                                         // scaled (designed for the Microsoft TrueType rasterizer).
};

// Swap the two bytes.
unsigned long 
SwapTwoBytes(unsigned char* twoBytes)
{
    return (twoBytes[0] << 8) + twoBytes[1];
}

// Swap the four bytes.
unsigned long 
SwapFourBytes(unsigned char* fourBytes)
{
    return (fourBytes[0] << 24) + (fourBytes[1] << 16) + (fourBytes[2] << 8) + fourBytes[3];
}

CommonTextStatus 
CommonTextFreeTypeFontDeviceImp::_ParseGlyphComponents(
    int indexToLocFormat, 
    int index, 
    UsdImagingTextRawGlyph& ttRawGlyph)
{
    // Read the "loca" table. Get the offset of the character in the table.
    long offsetValue = 0;
    if (indexToLocFormat == 1)
    {
        // The table element is unsigned long.
        unsigned char fourBytes[4];
        unsigned long length = 4;
        if (FT_Load_Sfnt_Table(_face, TTAG_loca, 4 * index, fourBytes, &length) != 0)
        {
            return CommonTextStatus::CommonTextStatusFail;
        }

        offsetValue = (long)SwapFourBytes(fourBytes);
    }
    else
    {
        // The table element is short.
        unsigned char twoBytes[2];
        unsigned long length = 2;
        if (FT_Load_Sfnt_Table(_face, TTAG_loca, 2 * index, twoBytes, &length) != 0)
        {
            return CommonTextStatus::CommonTextStatusFail;
        }

        // If the element is short, it is the real offset divide by 2.
        offsetValue = (long)SwapTwoBytes(twoBytes) * 2;
    }

    // Read the "glyf" table. Get the number of contours in the glyph
    unsigned char twoBytes[2];
    unsigned long length = 2;
    if (FT_Load_Sfnt_Table(_face, TTAG_glyf, offsetValue, twoBytes, &length) != 0)
    {
        return CommonTextStatus::CommonTextStatusFail;
    }

    short numberOfContours = (short)SwapTwoBytes(twoBytes);

    // If the number of contours is positive, this is a simple glyph.
    if (numberOfContours >= 0)
    {
        // For simple glyph, we still need to find the count of point of each contour,
        // and ignore the degenerate contours.
        offsetValue += 10;

        // Get the count of point of each contour.
        unsigned char* twoBytesArray = new unsigned char[2 * numberOfContours];
        unsigned long lLength        = 2 * numberOfContours;
        if (FT_Load_Sfnt_Table(_face, TTAG_glyf, offsetValue, twoBytesArray, &lLength) != 0)
        {
            delete[] twoBytesArray;
            return CommonTextStatus::CommonTextStatusFail;
        }

        // Find the contour that contains less than 3 points.
        // These are degenerate contours which we should ignore.
        short currentPoint     = 0;
        short contourRealCount = numberOfContours;
        for (int i = 0; i < numberOfContours; i++)
        {
            short endPtsOfContours = (short)SwapTwoBytes(&(twoBytesArray[i * 2]));

            // If the contour contains less than 3 points,
            // we decrease the contour count.
            if (endPtsOfContours - currentPoint < 2)
                contourRealCount--;

            currentPoint = endPtsOfContours + 1;
        }

        // Only if this glyph contains more than one contours,
        // we will add it.
        if (contourRealCount > 0)
            ttRawGlyph.AddComponent(contourRealCount);

        delete[] twoBytesArray;
    }
    else
    {
        // The glyph is a composite glyph.
        // We need to parse the sub glyph.
        offsetValue += 10;

        // The composite glyph is organized that one component is followed by another.
        // Whether there is more components after this component, is set in the flags.
        bool moreComponents = false;
        do
        {
            // Get the flags.
            unsigned char bTwoBytes[2];
            unsigned long lLength = 2;
            if (FT_Load_Sfnt_Table(_face, TTAG_glyf, offsetValue, bTwoBytes, &lLength) != 0)
            {
                return CommonTextStatus::CommonTextStatusFail;
            }

            short flags = (short)SwapTwoBytes(bTwoBytes);
            offsetValue += 2;

            // Read the sub glyph index of this component.
            if (FT_Load_Sfnt_Table(_face, TTAG_glyf, offsetValue, bTwoBytes, &lLength) != 0)
            {
                return CommonTextStatus::CommonTextStatusFail;
            }

            short glyphIndex = (short)SwapTwoBytes(bTwoBytes);
            offsetValue += 2;

            // Parse the sub glyph.
            CommonTextStatus status = _ParseGlyphComponents(indexToLocFormat, glyphIndex, ttRawGlyph);
            if (status != CommonTextStatus::CommonTextStatusSuccess)
                return status;

            // Move the offsetValue to the end of this component.
            if (flags & (short)CommonTextEParseOptions::kARG_1_AND_2_ARE_WORDS)
                offsetValue += 4;
            else
                offsetValue += 2;

            if (flags & (short)CommonTextEParseOptions::kWE_HAVE_A_SCALE)
                offsetValue += 2;
            else if (flags & (short)CommonTextEParseOptions::kWE_HAVE_AN_X_AND_Y_SCALE)
                offsetValue += 4;
            else if (flags & (short)CommonTextEParseOptions::kWE_HAVE_A_TWO_BY_TWO)
                offsetValue += 8;

            // Whether there is more components after this component.
            moreComponents = ((flags & (short)CommonTextEParseOptions::kMORE_COMPONENTS) == 0) ? false : true;

        } while (moreComponents);
    }

    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextStatus 
CommonTextFreeTypeFontDeviceImp::QueryTTRawGlyph(
    int size, 
    float widthFactor,
    const int glyphIndex, 
    CommonTextGlyphMetrics& ttRawGlyphMetrics, 
    UsdImagingTextRawGlyph& ttRawGlyph)
{
    // 1. Clear the existing data.
    if (_face == 0)
        return CommonTextStatus::CommonTextStatusFontNotFound;

    if (_currentSize != size || !IsFloatEqual(_currentWidthFactor, widthFactor))
    {
        // First we must set the size.
        int ptHorizontalSize = static_cast<int>(widthFactor * size * 72 * 64 / _horizontalDPI);
        int ptVerticalSize   = size * 72 * 64 / _verticalDPI;
        FT_Error error =
            FT_Set_Char_Size(_face, ptHorizontalSize, ptVerticalSize, _horizontalDPI, _verticalDPI);

        if (error != 0)
            return CommonTextStatus::CommonTextStatusFontNotFound;

        _currentSize        = size;
        _currentWidthFactor = widthFactor;
    }

    ttRawGlyph.Clear();

    FT_UInt glyph(glyphIndex);

    // Load the glyph
    // By default the outlien will be generated.
    if (FT_Load_Glyph(_face, glyph, FT_LOAD_NO_BITMAP | FT_LOAD_NO_SCALE) != 0)
    {
        // Failed to load an outline...
        return CommonTextStatus::CommonTextStatusCharacterNotFound;
    }

    if (_face->glyph->format != FT_GLYPH_FORMAT_OUTLINE)
    {
        // We didn't received an outline... weird...
        return CommonTextStatus::CommonTextStatusFail;
    }

    FT_Glyph glyphRec;

    // Get the glyph
    if (FT_Get_Glyph(_face->glyph, &glyphRec) != 0)
    {
        return CommonTextStatus::CommonTextStatusFail;
    }

    if (glyphRec->format != FT_GLYPH_FORMAT_OUTLINE)
    {
        return CommonTextStatus::CommonTextStatusFail;
    }

    // The points of outline are in design units, we should convert them to pixels.
    FT_Size_Metrics& scaledMetrics = _face->size->metrics;

    // Get the glyph metrics of the raw glyph.
    FT_BBox bbox;
    FT_Glyph_Get_CBox(glyphRec, FT_GLYPH_BBOX_UNSCALED, &bbox);

    ttRawGlyphMetrics._blackBoxX    = FT_PIX_ROUND(FT_MulFix(bbox.xMax - bbox.xMin, scaledMetrics.x_scale)) / 64.0f;
    ttRawGlyphMetrics._blackBoxY    = FT_PIX_ROUND(FT_MulFix(bbox.yMax - bbox.yMin, scaledMetrics.y_scale)) / 64.0f;
    ttRawGlyphMetrics._glyphOriginX = FT_PIX_ROUND(FT_MulFix(bbox.xMin, scaledMetrics.x_scale)) / 64.0f;
    ttRawGlyphMetrics._glyphOriginY = FT_PIX_ROUND(FT_MulFix(bbox.yMax, scaledMetrics.y_scale)) / 64.0f;

     // Set the bounding box.
    ttRawGlyph.SetBoundBox(ttRawGlyphMetrics._glyphOriginX,
        ttRawGlyphMetrics._glyphOriginY - ttRawGlyphMetrics._blackBoxY,
        ttRawGlyphMetrics._glyphOriginX + ttRawGlyphMetrics._blackBoxX,
        ttRawGlyphMetrics._glyphOriginY);

    // Convert the glyph to FT_OutlineGlyph
    FT_OutlineGlyph glyphOutline = (FT_OutlineGlyph)glyphRec;
    if (glyphOutline->outline.n_contours == 0 || glyphOutline->outline.n_points == 0)
        return CommonTextStatus::CommonTextStatusSuccess;

    // Set the outline process functions.
    FT_Outline_Funcs glyphOutlineFunctions;

    memset(&glyphOutlineFunctions, 0, sizeof(glyphOutlineFunctions));
    glyphOutlineFunctions.move_to  = (FT_Outline_MoveToFunc)OutlineMoveFunction;
    glyphOutlineFunctions.line_to  = (FT_Outline_LineToFunc)OutlineLineFunction;
    glyphOutlineFunctions.conic_to = (FT_Outline_ConicToFunc)OutlineConicFunction;
    glyphOutlineFunctions.cubic_to = (FT_Outline_CubicToFunc)OutlineCubicFunction;

    CommonTextFreeTypeFontDeviceOutlineIterator it;

    it._ttRawGlyph  = &ttRawGlyph;
    it._closedCurve = true;
    it._xscale      = scaledMetrics.x_scale;
    it._yscale      = scaledMetrics.y_scale;

    // Decompose the outline to UsdImagingTextRawGlyph.
    if (FT_Outline_Decompose(&(glyphOutline->outline), &glyphOutlineFunctions, &it) != 0)
    {
        return CommonTextStatus::CommonTextStatusFail;
    }

    if (!it._closedCurve)
    {
        ttRawGlyph.CloseCurve(false);
    }

    FT_Done_Glyph(glyphRec);

    // Generate the components information.
    unsigned char twoBytes[2];
    unsigned long length = 2;
    if (FT_Load_Sfnt_Table(_face, TTAG_head, 50, twoBytes, &length) != 0)
    {
        return CommonTextStatus::CommonTextStatusFail;
    }

    short indexToLocFormat = (short)SwapTwoBytes(twoBytes);

    // Parse the components information
    CommonTextStatus status = _ParseGlyphComponents(indexToLocFormat, glyphIndex, ttRawGlyph);
    if (status != CommonTextStatus::CommonTextStatusSuccess)
        return status;

    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextStatus 
CommonTextFreeTypeFontDeviceImp::QueryUnicodeRanges(CommonTextFontUnicodeRanges& ranges)
{
    if (_face == 0)
        return CommonTextStatus::CommonTextStatusFontNotFound;

    ranges._cGlyphsSupported = _face->num_glyphs;

    // We can not get the glyph ranges using FreeType.
    ranges._cRanges = 0;
    ranges._ranges  = nullptr;

    return CommonTextStatus::CommonTextStatusSuccess;
}

void 
CommonTextFreeTypeFontDeviceImp::_ReleaseResources()
{
    if (_face != 0)
    {
        FT_Done_Face(_face);
        _face = 0;
    }
}
PXR_NAMESPACE_CLOSE_SCOPE
