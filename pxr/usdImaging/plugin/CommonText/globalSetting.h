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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_GLOBAL_SETTING_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_GLOBAL_SETTING_H

#include "definitions.h"
#include "globals.h"

PXR_NAMESPACE_OPEN_SCOPE

class CommonTextSystem;

/// The default font texture size.
const int DEFAULT_FONT_TEXTURE_SIZE = 512;

/// The default font tile size.
const int DEFAULT_FONT_TILE_SIZE = 64;

/// The default font texture border size.
const int DEFAULT_FONT_TEXTURE_BORDER_SIZE = 1;

/// The default count of basic glyphs.
const int DEFAULT_COUNT_OF_BASIC_GLYPHS = 200;

/// The default maximum resolution of font.
const int DEFAULT_FONT_MAXIMUM_RESOLUTION = 64;

/// The default mipmap levels of font.
const int DEFAULT_FONT_MIPMAP_LEVELS = 4;

/// The default size of a tab.
const int DEFAULT_SIZE_OF_TAB = 24;

/// The default position of the first line of double strike through.
/// The two lines of double strike through should always be between the top and the bottom 
/// of the string bounding box. This value tells the ratio that the first line will be
/// positioned. The ratio of the second line will always be (1 - this value).
const float DEFAULT_POS_OF_FIRST_DOUBLE_STRIKE_THROUGH = 0.6;

/// \enum class CommonTextFontSubstitutionSettingFlag
///
/// The setting of font substitution
/// If the font can not support the character in a special language, you need to enable the
/// font substitution feature, and text system will automatically choose one font which can
/// support the language.
///
/// CommonTextEnableFontSubstitution: enable the font substitution feature.
/// CommonTextEnableSystemFontSubstitution: use the system default font for the language.
/// CommonTextEnablePredefinedFontSubstitution: use the predefined character to charset mapping to find the font.
/// CommonTextEnableUserDefinedFontSubstitution: user can define the substituted font for the language.
///
/// If CommonTextEnableUserDefinedFontSubstitution is set to true, you need to add a font's typeface for a
/// charset. The truetype font map cache can be got using
/// CommonTextSystem::instance()->getFontMapCache().
///
/// Difference between alternate font and font substitution: we use alternate font if the
/// font itself can not be supported, we use font substitution if the font can be loaded
/// successfully but some characters can not be rendered with the font.
///
/// The option flag for the substitution.
///
enum class CommonTextFontSubstitutionSettingFlag
{
    /// The font substitution is disabled
    CommonTextDisableFontSubstitution = 0,

    /// The font substitution is enabled
    CommonTextEnableFontSubstitution = (0x1 << 0),

    /// The font substitution using system codepages is enabled
    CommonTextEnableSystemFontSubstitution = (0x1 << 1),

    /// The user defined font substitution is enabled
    /// user defined font substitution has higher priority than default font substitution.
    CommonTextEnableUserDefinedFontSubstitution = (0x1 << 2),

    /// The font substitution using predefined character to charset mapping is enabled.
    CommonTextEnablePredefinedFontSubstitution = (0x1 << 3)
};

/// \class CommonTextFreeTypeFontDevice
///
/// The setting for font substitution.
///
struct CommonTextFontSubstitutionSetting
{
private:
    int _fontSubstitutionSetting = (int)CommonTextFontSubstitutionSettingFlag::CommonTextDisableFontSubstitution;

public:
    /// The constructor.
    CommonTextFontSubstitutionSetting() = default;

    /// Test if some bits of the _fontSubstitutionSetting are set.
    /// \note The flag should be one enumeration or a set of enumerations in
    /// CommonTextFontSubstitutionSettingFlag.
    inline bool TestSetting(int flagToTest)
    {
        return ((int)_fontSubstitutionSetting & flagToTest) != 0;
    }

    /// Set or unset some bits of the _fontSubstitutionSetting.
    /// \note The flag should be one enumeration or a set of enumerations in
    /// FontSubstitutionSettingFlag.
    inline void SetSetting(CommonTextFontSubstitutionSettingFlag flagsToSet, 
                           bool setOrClear)
    {
        if (setOrClear)
            _fontSubstitutionSetting |= (int)flagsToSet;
        else
            _fontSubstitutionSetting &= ~(int)flagsToSet;
    }
};

/// \class CommonTextGlobalSetting
///
/// The global setting of the text system
///
class CommonTextGlobalSetting
{
private:
    CommonTextStringArray _trueTypeFontDirectories;

    int _tabSize = DEFAULT_SIZE_OF_TAB;
    float _posFirstLineOfDoubleStrikethrough = DEFAULT_POS_OF_FIRST_DOUBLE_STRIKE_THROUGH;

public:
    /// The constructor.
    CommonTextGlobalSetting() = default;

    /// The copy constructor.
    CommonTextGlobalSetting(const CommonTextGlobalSetting& setting)
        : _tabSize(setting._tabSize)
        , _posFirstLineOfDoubleStrikethrough(setting._posFirstLineOfDoubleStrikethrough)
    {
        _trueTypeFontDirectories = setting._trueTypeFontDirectories;
    }

    /// The destructor
    ~CommonTextGlobalSetting() = default;

    int TabSize() const { return _tabSize; }
    void TabSize(int value) { _tabSize = value; }

    float PosFirstLineOfDoubleStrikethrough() const { return _posFirstLineOfDoubleStrikethrough; }
    void PosFirstLineOfDoubleStrikethrough(float value) { _posFirstLineOfDoubleStrikethrough = value; }

    const CommonTextStringArray& TrueTypeFontDirectories() const { return _trueTypeFontDirectories; }

    CommonTextStringArray& TrueTypeFontDirectories() { return _trueTypeFontDirectories; }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_GLOBAL_SETTING_H
