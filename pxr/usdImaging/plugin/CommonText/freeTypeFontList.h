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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_FREETYPE_FONT_LIST_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_FREETYPE_FONT_LIST_H

#include "definitions.h"
#include <ft2build.h>
#include FT_FREETYPE_H
#include <tbb/concurrent_unordered_map.h>
#include <filesystem>

PXR_NAMESPACE_OPEN_SCOPE
/// \struct CommonTextFontInfo
///
/// The information about the font
///
struct CommonTextFontInfo
{
    /// The path of the font file.
    std::string _fontPath;

    // The index of the font in the file.
    long _index;

    /// The family name.
    std::string _familyName;

    /// The style name.
    std::string _styleName;

    /// If the font is bold.
    bool _bold;

    /// If the font is italic.
    bool _italic;
};

/// A list of font information.
using CommonTextFontList = std::vector<CommonTextFontInfo>;

/// Test if the font is bold.
inline bool IsFaceBold(FT_Face face)
{
    return (face->style_flags & FT_STYLE_FLAG_BOLD) ? true : false;
}

/// Test if the font is italic.
inline bool IsFaceItalic(FT_Face face)
{
    return (face->style_flags & FT_STYLE_FLAG_ITALIC) ? true : false;
}

/// Test if the font is TrueType.
inline bool IsFaceTrueType(FT_Face face)
{
    return (face->face_flags & FT_FACE_FLAG_SFNT) ? true : false;
}

/// \class CommonTextFreeTypeFontList
///
/// The FreetypeFontList holds a list of font information.
///
class CommonTextFreeTypeFontList
{
private:
    tbb::concurrent_unordered_map<std::string, std::shared_ptr<CommonTextFontList>> _fontListMap;

    static CommonTextFreeTypeFontList* _pInstance;

public:
    /// Returns the singleton instance reference (not thread-safe).
    static CommonTextFreeTypeFontList& GetInstance()
    {
        return *_Get();
    }

    /// Initializes the singleton instance (not thread-safe).
    static bool InitInstance()
    {
        if (!_pInstance)
        {
            _pInstance = new CommonTextFreeTypeFontList();
        }

        return (_pInstance->_InitializeFreeTypeFontList());
    }

    /// Releases the singleton instance (not thread-safe).
    /// This gives the caller a chance to release the instance on demand, rather than until exit.
    static void ReleaseInstance()
    {
        if (_pInstance)
        {
            delete _pInstance;
            _pInstance = nullptr;
        }
    }

    /// If the font list is initialized.
    static bool IsInitialize() { return _pInstance != nullptr && !_pInstance->_fontListMap.empty(); }

    /// The destructor.
    ~CommonTextFreeTypeFontList();

    /// Find the font's file path and index with the family name and the style.
    /// If we can find a font with the family name, but the style is not match, we will also return
    /// this font. The ifBold and ifItalic parameter will be assigned with the style of the font we
    /// found.
    /// \param[in] familyName The family name of the font.
    /// \param[in, out] ifBold If the font is bold. This will be assigned with the style of the font
    /// we found.
    /// \param[in, out] ifItalic If the font is italic. This will be assigned with the style of the
    /// font we found.
    /// \param[out] filePath The font file path of the font. If this is  set to nullptr, this
    /// function only returns if the font exists.
    /// \param[out] index The index of the font in the file.
    bool FindFont(const std::string& familyName,
                         bool& ifBold,
                         bool& ifItalic,
                         std::string& filePath,
                         long& index);

private:
    CommonTextFreeTypeFontList() = default;
    CommonTextFreeTypeFontList(CommonTextFreeTypeFontList const&) = delete;
    CommonTextFreeTypeFontList& operator=(CommonTextFreeTypeFontList const&) = delete;

    /// Returns the singleton instance pointer (not thread-safe).
    static CommonTextFreeTypeFontList* _Get()
    {
        InitInstance();
        return _pInstance;
    }

    /// Initialize the freetype font list
    /// \note This will collect all the TrueType information in the
    /// truetype font folders and build the list.
    bool _InitializeFreeTypeFontList();

    /// Release the freetype font list.
    bool _ReleaseFreeTypeFontList();

    /// To build the map which save the font informations.
    void _BuildFontListMap(FT_Library library);

    /// Save a font to the map.
    void _SaveFontToTheMap(FT_Face face,
                                  FT_Long index,
                                  const std::string& filePath);

    /// Add a font.
    bool _AddFont(FT_Library library,
                         const char* path);
};
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_FREETYPE_FONT_LIST_H
