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

#include "freeTypeFontList.h"
#include "globalSetting.h"
#include "system.h"
#include <cassert>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

CommonTextFreeTypeFontList*
CommonTextFreeTypeFontList::_pInstance = nullptr;

CommonTextFreeTypeFontList::~CommonTextFreeTypeFontList()
{
    _ReleaseFreeTypeFontList();
}

bool
CommonTextFreeTypeFontList::_InitializeFreeTypeFontList()
{
    if(!_fontListMap.empty())
        return true;

    FT_Library library;
    FT_Error error = FT_Init_FreeType(&library);
    if (error != 0)
        return false;

    // Collect all the TrueType information in the
    // truetype font folders and build the list.
    _BuildFontListMap(library);

    FT_Done_FreeType(library);
    return true;
}

bool
CommonTextFreeTypeFontList::_ReleaseFreeTypeFontList()
{
    _fontListMap.clear();
    return true;
}

bool
CommonTextFreeTypeFontList::_AddFont(
    FT_Library library,
    const char* path)
{
    // For every font file, find all the fonts in the file,
    // and add the information to the map.
    FT_Face face;
    FT_Long faceIndex = 0;
    FT_Long faceCount = 0;
    while (FT_New_Face(library, path, faceIndex, &face) == 0)
    {
        if (faceCount == 0)
            faceCount = face->num_faces;

        // If this is a TrueType font, load it
        if (face != nullptr && IsFaceTrueType(face))
            _SaveFontToTheMap(face, faceIndex, path);

        // Release the face
        FT_Done_Face(face);

        // Increment the face index
        if (++faceIndex >= faceCount)
            break;
    }
    return true;
}

void
CommonTextFreeTypeFontList::_BuildFontListMap(FT_Library library)
{
    // Add fonts in the private directories.
    const CommonTextGlobalSetting& textSetting = CommonTextSystem::Instance()->GetTextGlobalSetting();
    const CommonTextStringArray& textDirectories   = textSetting.TrueTypeFontDirectories();

    // Collect font paths.
    CommonTextStringArray filePaths;
    {
        for (const auto& directory : textDirectories)
        {
            for (const auto& entry : std::filesystem::directory_iterator(directory))
            {
                if (entry.is_regular_file())
                {
                    filePaths.push_back(entry.path().string().c_str());
                }
            }
        }
    }
    for (auto& path : filePaths)
    {
        _AddFont(library, path.c_str());
    }
}

void
CommonTextFreeTypeFontList::_SaveFontToTheMap(
    FT_Face face,
    FT_Long index,
    const std::string& filePath)
{
    char* familyNameA(face->family_name);
    char* style(face->style_name);

    // Find if the font is already saved in the map.
    auto it = _fontListMap.find(face->family_name);
    if (it != _fontListMap.end())
    {
        assert(it->second);
        std::shared_ptr<CommonTextFontList>& fontList = it->second;
        size_t listSize                     = fontList->size();
        if (listSize > 0)
        {
            for (const auto& info : *fontList)
            {
                if (info._styleName == style)
                    return;
            }
        }
    }
    else
    {
        std::shared_ptr<CommonTextFontList> newFontList = std::make_shared<CommonTextFontList>();

        it = _fontListMap.emplace(face->family_name, newFontList).first;
    }
    // Add the font to the list.
    CommonTextFontInfo newInfo;
    newInfo._fontPath   = filePath;
    newInfo._index      = index;
    newInfo._familyName = familyNameA;
    newInfo._styleName  = style;
    newInfo._bold       = IsFaceBold(face);
    newInfo._italic     = IsFaceItalic(face);
    it->second->push_back(std::move(newInfo));
}

bool
CommonTextFreeTypeFontList::FindFont(
    const std::string& familyName,
    bool& ifBold,
    bool& ifItalic,
    std::string& filePath,
    long& index)
{
    // Find if the font is already saved in the map.
    auto it = _fontListMap.find(familyName);
    if (it != _fontListMap.end())
    {
        std::shared_ptr<CommonTextFontList> fontList = it->second;
        assert(fontList);
        if (fontList->size() != 0)
        {
            for (const auto& info : *fontList)
            {
                if (info._bold == ifBold && info._italic == ifItalic)
                {
                    filePath = info._fontPath;
                    index    = info._index;
                    return true;
                }
            }

            // If we can find the font with the family name,
            // but the style is different, we will return this font,
            // and set ifBold and ifItalic to the font style we found.
            size_t count = fontList->size();
            if (count != 0)
            {
                CommonTextFontInfo& info = (*fontList).at(0);
                filePath       = info._fontPath;
                index          = 0;
                ifBold         = info._bold;
                ifItalic       = info._italic;
                return true;
            }
        }
    }
    return false;
}

PXR_NAMESPACE_CLOSE_SCOPE
