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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_CHARACTER_SUPPORT_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_CHARACTER_SUPPORT_H

#include "definitions.h"
#include "fontDevice.h"

PXR_NAMESPACE_OPEN_SCOPE

class IFontDevice;

/// \class CommonTextFontSupportCharacterTest
///
/// This class will test if the font support the character
///
class CommonTextFontSupportCharacterTest
{
public:
    /// The constructor
    CommonTextFontSupportCharacterTest() = default;

    /// Test if all the characters are supported in the font.
    /// \param firstUnsupportedCharacter
    /// If not all characters are supported, this is the first character which is not
    /// supported. If all the characters are supported, the value is meaningless.
    /// Return true to indicate all the characters are supported, false to indicate
    /// some of them are not unsupported.
    bool IsAllSupported(std::wstring asciiString, 
                        int firstUnsupportedCharacter);

    /// Set the typeface and initialize the mpFontDevice.
    /// For the TrueType font, the typeface should be like "Times New Roman".
    /// For the SHX font, the typeface can be in three forms: "txt" for normal font;
    /// ",bigfont" for extended font; "txt,bigfont" for both normal font and extended font.
    /// Return true to indicate success, false to indicate there is error.
    CommonTextStatus Initialize(const std::string& typeFace, 
                                bool isComplex);

private:
    CommonTextTrueTypeFontDevicePtr _fontDevice;
    std::string _typeface;
    bool _isComplex        = false;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_CHARACTER_SUPPORT_H
