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

#include "characterSupport.h"
#include "fontDevice.h"
#include "multiLanguageHandler.h"
#include "system.h"

PXR_NAMESPACE_OPEN_SCOPE

bool 
CommonTextFontSupportCharacterTest::IsAllSupported(
    std::wstring asciiString, 
    int firstUnsupported)
{
    // If we want to get the first unsupported character, initialize it to zero. Because
    // if all the characters are not supported, this value should be zero.
    if (firstUnsupported != -1)
        firstUnsupported = 0;

    // The indices we acquired are only for checking the first unsupported character.
    // So it is locally used in this function.
    static unsigned short localIndices[MAXIMUM_COUNT_OF_CHAR_IN_LINE];
    if (!_isComplex)
    {
        // TrueType font, non-complex scripts. Using the TrueType font device
        // to get the indices.
        if (!_fontDevice.IsValid())
            return false;

        CommonTextStatus status = _fontDevice->QueryGlyphIndices(asciiString, localIndices);

        if (status != CommonTextStatus::CommonTextStatusSuccess)
            // All characters are not supported.
            return false;
        else
        {
            // TRUETYPE_MISSING_GLYPH_INDEX means the character is missing. Find
            // the first character that is missing. If we can find, we know not all
            // are supported.
            for (int i = 0; i < asciiString.length(); i++)
            {
                if (localIndices[i] == TRUETYPE_MISSING_GLYPH_INDEX)
                {
                    if (firstUnsupported != -1)
                        firstUnsupported = i;
                    return false;
                }
            }
            return true;
        }
    }
    else
    {
        // TrueType font, complex scripts. Using the multilanguage handler to check
        // if all the characters are supported.
        UsdImagingTextStyle temporaryStyle;
        temporaryStyle._typeface = _typeface;
        bool isSupported = true;
        // This function can tell us if all the characters are supported. But it
        // will not return the first unsupported character. If firstUnsupported is
        // needed, we need to use the localIndices to get the indices of the
        // characters, and check the indices to find the first missing character.
        // If firstUnsupported is not needed, we don't need to pass the indices
        // so that the function will be faster.
        std::shared_ptr<CommonTextMultiLanguageHandler> pMultiLanguageHandler =
            CommonTextSystem::Instance()->GetMultiLanguageHandler();
        if (pMultiLanguageHandler == nullptr)
            return false;
        pMultiLanguageHandler->IsAllCharactersSupported(temporaryStyle, asciiString,
            isSupported, (firstUnsupported != -1) ? localIndices : nullptr);

        if (firstUnsupported != -1 && !isSupported)
        {
            // We need to find the first missing character.
            for (int i = 0; i < asciiString.size(); i++)
            {
                if (localIndices[i] == TRUETYPE_MISSING_GLYPH_INDEX)
                {
                    firstUnsupported = i;
                    break;
                }
            }
        }
        return isSupported;
    }
}

CommonTextStatus CommonTextFontSupportCharacterTest::Initialize(
    const std::string& typeFace, 
    bool isComplex)
{
    if (_fontDevice.IsValid())
        return CommonTextStatus::CommonTextStatusFail;

    _isComplex = isComplex;
    _typeface  = typeFace;

    if (!isComplex)
    {
        UsdImagingTextStyle textStyle;
        textStyle._typeface = typeFace;

        if (!_fontDevice.Initialize(textStyle) || !_fontDevice.IsValid())
            return CommonTextStatus::CommonTextStatusFail;
    }
    return CommonTextStatus::CommonTextStatusSuccess;
}

PXR_NAMESPACE_CLOSE_SCOPE
