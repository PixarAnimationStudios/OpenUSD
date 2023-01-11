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

#include "simpleLayout.h"
#include "fontDevice.h"
#include "globalSetting.h"
#include "multiLanguageHandler.h"
#include "portableUtils.h"
#include "system.h"

PXR_NAMESPACE_OPEN_SCOPE
CommonTextTrueTypeSimpleLayoutManager::~CommonTextTrueTypeSimpleLayoutManager()
{
}

CommonTextStatus 
CommonTextTrueTypeSimpleLayoutManager::GenerateCharMetricsAndIndices(
    const std::string& asciiString,
    CommonTextSimpleLayout& simpleLayout,
    std::shared_ptr<CommonTextComplexScriptInfo> complexScriptInfo)
{
    // Get the indices of characters.
    CommonTextStatus result = _QueryGlyphIndices(asciiString, simpleLayout, complexScriptInfo);
    if (result != CommonTextStatus::CommonTextStatusSuccess)
        return result;

    // Calculate metrics of characters.
    result = _CalculateCharMetrics(simpleLayout);
    return result;
}

CommonTextStatus 
CommonTextTrueTypeSimpleLayoutManager::GenerateCharMetricsAndIndices(
    const std::wstring& unicodeString,
    CommonTextSimpleLayout& simpleLayout,
    std::shared_ptr<CommonTextComplexScriptInfo> complexScriptInfo)
{
    // Get the indices of characters.
    CommonTextStatus result = _QueryGlyphIndices(unicodeString, simpleLayout, complexScriptInfo);
    if (result != CommonTextStatus::CommonTextStatusSuccess)
        return result;

    // Calculate metrics of characters.
    result = _CalculateCharMetrics(simpleLayout);
    return result;
}

CommonTextStatus 
CommonTextTrueTypeSimpleLayoutManager::GenerateTextMetrics(CommonTextSimpleLayout& simpleLayout)
{
    // Get text metrics.
    CommonTextMetrics& textMetrics = simpleLayout.FullMetrics();

    if (!simpleLayout.TestMetricsInfoAvailability(CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityIndicesAvailable |
            CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityCharMetricsAvailable))
        return CommonTextStatus::CommonTextStatusFail;
    if (simpleLayout.TestMetricsInfoAvailability(CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityTextMetricsAvailable))
        return CommonTextStatus::CommonTextStatusSuccess;

    // Get font metrics.
    CommonTextFontMetrics fontMetrics;
    _fontDevice->QueryFontMetrics(fontMetrics);

    int descent = fontMetrics._descent;
    int ascent  = fontMetrics._ascent;

    CommonTextBox2<GfVec2f> semanticBound;
    CommonTextBox2<GfVec2f> extentBound;

    // If the length of the string is zero,
    // The bounding box is zero at width, and the height is the same with the font height.
    int glyphCount = simpleLayout.CountOfRenderableChars();
    if (glyphCount == 0)
    {
        semanticBound.AddPoint(GfVec2f(0, descent));
        semanticBound.AddPoint(GfVec2f(0, ascent));
        extentBound.AddPoint(GfVec2f(0, descent));
        extentBound.AddPoint(GfVec2f(0, ascent));
    }
    else
    {
        // The width of the semantic bounding box is from start point to the semantic end of the
        // last character. The height is from descent to ascent.
        semanticBound.AddPoint(GfVec2f(0, descent));
        semanticBound.AddPoint(GfVec2f(simpleLayout.CharacterMetrics(glyphCount - 1)._endPosition,
            static_cast<float>(ascent)));

        // The extent bounding box is the sum of all the bounding box of every character.
        for (int i = 0; i < glyphCount; i++)
        {
            // For each character, we need to translate the bounding box with the position.
            CommonTextCharMetrics& charMetrics = simpleLayout.CharacterMetrics(i);
            if (!charMetrics._boundBox.IsEmpty())
            {
                CommonTextBox2<GfVec2f> moveBox(charMetrics._boundBox);
                moveBox.TranslateInX(charMetrics._startPosition);
                extentBound.AddBox(moveBox);
            }
        }
    }
    textMetrics._extentBound   = extentBound;
    textMetrics._semanticBound = semanticBound;
    simpleLayout.SetMetricsInfoAvailability(CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityTextMetricsAvailable, true);
    return CommonTextStatus::CommonTextStatusSuccess;
}

/// Get the rasterized data for the renderable characters.
CommonTextStatus 
CommonTextTrueTypeSimpleLayoutManager::GenerateRasterizedData(
    short glyphIndex,
    CommonTextBox2<GfVec2i>& rasBox,
    int& dataLength,
    void* data)
{
    CommonTextGlyphMetrics rasGlyphMetrics;
    CommonTextStatus status =
        _fontDevice->QueryRasterizedData(glyphIndex, rasGlyphMetrics, dataLength, data);
    // Set the box of the rasterization data.
    rasBox.Min(GfVec2i(
        rasGlyphMetrics._glyphOriginX, rasGlyphMetrics._glyphOriginY - rasGlyphMetrics._blackBoxY));
    rasBox.Max(GfVec2i(
        rasGlyphMetrics._glyphOriginX + rasGlyphMetrics._blackBoxX, rasGlyphMetrics._glyphOriginY));
    return status;
}

/// Get the triangular control points geometry for the renderable character.
CommonTextStatus 
CommonTextTrueTypeSimpleLayoutManager::GenerateRawGlyph(
    short glyphIndex,
    CommonTextBox2<GfVec2i>& rasBox,
    UsdImagingTextRawGlyph& rawGlyph)
{
    CommonTextGlyphMetrics rasGlyphMetrics;
    CommonTextStatus status = _fontDevice->QueryTTRawGlyph(glyphIndex, rasGlyphMetrics, rawGlyph);
    if (status != CommonTextStatus::CommonTextStatusSuccess)
        return status;
    else
    {
        // Set the box of the rasterization data.
        rasBox.Min(GfVec2i(rasGlyphMetrics._glyphOriginX,
            rasGlyphMetrics._glyphOriginY - rasGlyphMetrics._blackBoxY));
        rasBox.Max(GfVec2i(rasGlyphMetrics._glyphOriginX + rasGlyphMetrics._blackBoxX,
            rasGlyphMetrics._glyphOriginY));
        return status;
    }
}

CommonTextStatus
CommonTextTrueTypeSimpleLayoutManager::_QueryGlyphIndices(
    const std::string& asciiString,
    CommonTextSimpleLayout& simpleLayout,
    std::shared_ptr<CommonTextComplexScriptInfo> complexScriptInfo)
{
    if (simpleLayout.TestMetricsInfoAvailability(CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityIndicesAvailable))
        return CommonTextStatus::CommonTextStatusSuccess;

    // Calculate the extents for the characters
    size_t length = asciiString.size();
    if (length == 0)
    {
        simpleLayout.SetMetricsInfoAvailability(CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityIndicesAvailable, true);
        return CommonTextStatus::CommonTextStatusSuccess;
    }
    else
    {
        CommonTextStatus result                        = CommonTextStatus::CommonTextStatusSuccess;
        bool isAllCharactersSupported             = false;
        std::vector<unsigned short>& glyphIndices = simpleLayout.CharacterIndices();
        if (complexScriptInfo != nullptr)
        {
            // Use the multilanguage handler to get the indices.
            std::shared_ptr<CommonTextMultiLanguageHandler> multiLanguageHandler =
                _textSystem->GetMultiLanguageHandler();

            result = multiLanguageHandler->AcquireComplexIndices(s2w(asciiString), simpleLayout,
                _fontDevice.GetStyle(), isAllCharactersSupported, glyphIndices, complexScriptInfo);

            simpleLayout.SetCountOfRenderableChars((int)glyphIndices.size());
        }
        else
        {
            simpleLayout.SetCountOfRenderableChars((int)length);
            memset(glyphIndices.data(), TRUETYPE_MISSING_GLYPH_INDEX, (int)length);
            // Get the glyph indices.
            result = _fontDevice->QueryGlyphIndices(asciiString, glyphIndices.data());

            isAllCharactersSupported = true;
        }

        if (result == CommonTextStatus::CommonTextStatusSuccess)
        {
            // If we successfully get the indices, the indices are available.
            simpleLayout.SetMetricsInfoAvailability(
                CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityIndicesAvailable, true);
        }
        else
        {
            simpleLayout.SetMetricsInfoAvailability(
                CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityIndicesAvailable, false);
        }

        if (isAllCharactersSupported)
        {
            // if isAllCharactersSupported is true, we still need to search for invalid
            // glyph index.
            simpleLayout.SetMetricsInfoAvailability(CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityIndicesValid, true);
            for (const auto& index : glyphIndices)
            {
                if (index == TRUETYPE_MISSING_GLYPH_INDEX)
                {
                    simpleLayout.SetMetricsInfoAvailability(
                        CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityIndicesValid, false);
                    break;
                }
            }
        }
        else
            simpleLayout.SetMetricsInfoAvailability(CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityIndicesValid, false);

        // If the indices is invalid, and system setting is allowing
        // font substitution, we return CommonTextStatusNeedSubstitution.
        CommonTextFontSubstitutionSetting fontSubstitutionSetting = _textSystem->GetFontSubstitutionSetting();
        bool allowFontSubstitution = fontSubstitutionSetting.TestSetting(
            (int)CommonTextFontSubstitutionSettingFlag::CommonTextEnableFontSubstitution);

        if (!simpleLayout.TestMetricsInfoAvailability(CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityIndicesValid) &&
            allowFontSubstitution)
        {
            return CommonTextStatus::CommonTextStatusNeedSubstitution;
        }

        return CommonTextStatus::CommonTextStatusSuccess;
    }
}

CommonTextStatus 
CommonTextTrueTypeSimpleLayoutManager::_QueryGlyphIndices(
    const std::wstring& unicodeString,
    CommonTextSimpleLayout& simpleLayout,
    std::shared_ptr<CommonTextComplexScriptInfo> complexScriptInfo)
{
    if (simpleLayout.TestMetricsInfoAvailability(CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityIndicesAvailable))
        return CommonTextStatus::CommonTextStatusSuccess;

    // Calculate the extents for the characters
    size_t length = unicodeString.size();
    if (length == 0)
    {
        simpleLayout.SetMetricsInfoAvailability(CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityIndicesAvailable, true);
        return CommonTextStatus::CommonTextStatusSuccess;
    }
    else
    {
        CommonTextStatus result                        = CommonTextStatus::CommonTextStatusSuccess;
        bool isAllCharactersSupported             = false;
        std::vector<unsigned short>& glyphIndices = simpleLayout.CharacterIndices();
        if (complexScriptInfo != nullptr)
        {
            // Use the multilanguage handler to get the indices.
            std::shared_ptr<CommonTextMultiLanguageHandler> multiLanguageHandler =
                _textSystem->GetMultiLanguageHandler();

            result = multiLanguageHandler->AcquireComplexIndices(unicodeString, simpleLayout,
                _fontDevice.GetStyle(), isAllCharactersSupported, glyphIndices, complexScriptInfo);
            simpleLayout.SetCountOfRenderableChars((int)glyphIndices.size());
        }
        else
        {
            simpleLayout.SetCountOfRenderableChars((int)length);
            memset(glyphIndices.data(), TRUETYPE_MISSING_GLYPH_INDEX, (int)length);
            // Get the glyph indices.
            result = _fontDevice->QueryGlyphIndices(unicodeString, glyphIndices.data());

            isAllCharactersSupported = true;
        }

        if (result == CommonTextStatus::CommonTextStatusSuccess)
        {
            // If we successfully get the indices, the indices are available.
            simpleLayout.SetMetricsInfoAvailability(
                CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityIndicesAvailable, true);
        }
        else
        {
            simpleLayout.SetMetricsInfoAvailability(
                CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityIndicesAvailable, false);
        }

        if (isAllCharactersSupported)
        {
            // if isAllCharactersSupported is true, we still need to search for invalid
            // glyph index.
            simpleLayout.SetMetricsInfoAvailability(CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityIndicesValid, true);
            for (const auto& index : glyphIndices)
            {
                if (index == TRUETYPE_MISSING_GLYPH_INDEX)
                {
                    simpleLayout.SetMetricsInfoAvailability(
                        CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityIndicesValid, false);
                    break;
                }
            }
        }
        else
            simpleLayout.SetMetricsInfoAvailability(CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityIndicesValid, false);

        // If the indices is invalid, and system setting is allowing
        // font substitution, we return CommonTextStatusNeedSubstitution.
        CommonTextFontSubstitutionSetting fontSubstitutionSetting = _textSystem->GetFontSubstitutionSetting();
        bool allowFontSubstitution = fontSubstitutionSetting.TestSetting(
            (int)CommonTextFontSubstitutionSettingFlag::CommonTextEnableFontSubstitution);

        if (!simpleLayout.TestMetricsInfoAvailability(CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityIndicesValid) &&
            allowFontSubstitution)
        {
            return CommonTextStatus::CommonTextStatusNeedSubstitution;
        }
        else
            return CommonTextStatus::CommonTextStatusSuccess;
    }
}

// Calculate CharMetrics of CommonTextSimpleLayout
CommonTextStatus 
CommonTextTrueTypeSimpleLayoutManager::_CalculateCharMetrics(CommonTextSimpleLayout& simpleLayout)
{
    if (!simpleLayout.TestMetricsInfoAvailability(CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityIndicesAvailable))
        return CommonTextStatus::CommonTextStatusFail;
    if (simpleLayout.TestMetricsInfoAvailability(CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityCharMetricsAvailable))
        return CommonTextStatus::CommonTextStatusSuccess;

    // Get indices of characters.
    std::vector<unsigned short> characterIndices = simpleLayout.CharacterIndices();
    int countOfRenderables                       = simpleLayout.CountOfRenderableChars();

    // Get font metrics.
    CommonTextFontMetrics fontMetrics;
    _fontDevice->QueryFontMetrics(fontMetrics);

    // The character space is added with this addedWidth.
    float addedWidth =
        _fontDevice.GetStyle()._characterSpaceFactor * fontMetrics._avgCharWidth - fontMetrics._avgCharWidth;

    float simpleLength = 0.0f;
    for (int i = 0; i < countOfRenderables; ++i)
    {
        CommonTextGlyphMetrics glyphMetrics;
        CommonTextStatus status = _fontDevice->QueryGlyphMetrics(characterIndices[i], glyphMetrics);
        if (status != CommonTextStatus::CommonTextStatusSuccess)
        {
            simpleLayout.SetMetricsInfoAvailability(
                CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityCharMetricsAvailable, false);
            return status;
        }

        CommonTextCharMetrics& characterMetric = simpleLayout.CharacterMetrics(i);

        // As the glyph position is not set by the client, we need to generate it.
        // Accumulate glyph metrics to get the start position for each character.
        // If the character is from the left to right, the glyph position is at the left of the
        // character. It is before we accumulate the width of the character.
        characterMetric._startPosition = round(simpleLength);

        // The basic width of each character is the ABC.
        simpleLength += glyphMetrics._cellIncX;

        // The end position of the character.
        characterMetric._endPosition = round(simpleLength);

        // Add the extra length due to character space.
        simpleLength += addedWidth;

        // Set bound box for CharMetrics
        if (glyphMetrics._blackBoxX == 0 && glyphMetrics._blackBoxY == 0)
            characterMetric._boundBox.Clear();
        else
        {
            characterMetric._boundBox.Min(GfVec2i(
                glyphMetrics._glyphOriginX, glyphMetrics._glyphOriginY - glyphMetrics._blackBoxY));
            characterMetric._boundBox.Max(GfVec2i(
                glyphMetrics._glyphOriginX + glyphMetrics._blackBoxX, glyphMetrics._glyphOriginY));
        }
    }
    simpleLayout.SetMetricsInfoAvailability(CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityCharMetricsAvailable, true);
    return CommonTextStatus::CommonTextStatusSuccess;
}

PXR_NAMESPACE_CLOSE_SCOPE
