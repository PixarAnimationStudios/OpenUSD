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

#include "genericLayout.h"
#include "globalSetting.h"
#include "metrics.h"
#include "portableUtils.h"
#include "simpleLayout.h"
#include "system.h"
#include "utilities.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/tf/type.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/usdImaging/usdImaging/text.h"
#include "pxr/usdImaging/usdImaging/markupText.h"
#include "pxr/usdImaging/usdImaging/textRenderer.h"
#include "pxr/usdImaging/usdImaging/textStyle.h"
#include "pxr/usdImaging/usdImaging/tokens.h"
#include "pxr/usd/usdText/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \class UsdImagingCommonText
///
/// The common text plugin.
///
class UsdImagingCommonText : public UsdImagingText
{
public:
    using Base = UsdImagingText;

    UsdImagingCommonText();

    ~UsdImagingCommonText() override;

private:
    /// Initialize the text plugin using a text setting.
    bool _Initialize(const TextSettingMap&) override;

    /// Generate the geometry for markup text.
    bool _GenerateMarkupTextGeometries(UsdImagingTextRendererSharedPtr renderer, 
                                       std::shared_ptr<UsdImagingMarkupText> markupText,
                                       VtVec3fArray& geometries,
                                       VtVec4fArray& textCoords,
                                       VtVec3fArray& textColor,
                                       VtFloatArray& textOpacity,
                                       VtVec3fArray& lineColors,
                                       VtFloatArray& lineOpacities,
                                       VtVec3fArray& lineGeometries) override;

    /// Generate the geometry for simple text.
    bool _GenerateSimpleTextGeometries(UsdImagingTextRendererSharedPtr renderer, 
                                       const std::string& textData,
                                       const UsdImagingTextStyle& style, 
                                       VtVec3fArray& geometries,
                                       VtVec4fArray& textCoords, 
                                       VtVec3fArray& lineGeometries) override;
};

TF_REGISTRY_FUNCTION(TfType)
{
    TfType t = TfType::Define<UsdImagingCommonText, TfType::Bases<UsdImagingCommonText::Base> >();
    t.SetFactory< UsdImagingTextFactory<UsdImagingCommonText> >();
}

UsdImagingCommonText::UsdImagingCommonText()
{
}

/* virtual */
UsdImagingCommonText::~UsdImagingCommonText() = default;

bool 
UsdImagingCommonText::_Initialize(const TextSettingMap& textSettingMap)
{
    // Initialize the text system.
    CommonTextGlobalSetting setting;
    // Set the font folder.
    auto fontFolderIter = textSettingMap.find(UsdImagingTextTokens->fontFolder);
    if(fontFolderIter != textSettingMap.end())
        setting.TrueTypeFontDirectories().emplace_back(fontFolderIter->second);

    // Set the tab size.
    auto tabSizeIter = textSettingMap.find(UsdImagingTextTokens->tabSize);
    if (tabSizeIter != textSettingMap.end())
        setting.TabSize(std::stoi(tabSizeIter->second));

    // Set the position of the first line of the double strike through.
    auto firstLineOfDoubleStrikethroughIter = 
        textSettingMap.find(UsdImagingTextTokens->posFirstLineOfDoubleStrikethrough);
    if (firstLineOfDoubleStrikethroughIter != textSettingMap.end())
        setting.PosFirstLineOfDoubleStrikethrough(
            std::stoi(firstLineOfDoubleStrikethroughIter->second));

    // Initialize the text system.
    if (CommonTextSystem::Instance()->Initialize(setting) != CommonTextStatus::CommonTextStatusSuccess)
        return false;

    // Set the font substituion.
    auto fontSubstitutionIter = textSettingMap.find(UsdImagingTextTokens->fontSubstitution);
    if (fontSubstitutionIter != textSettingMap.end())
    {
        CommonTextFontSubstitutionSetting& substituionSetting = 
            CommonTextSystem::Instance()->GetFontSubstitutionSetting();
        if (!fontSubstitutionIter->second.compare("default"))
        {
            substituionSetting.SetSetting(
                CommonTextFontSubstitutionSettingFlag::CommonTextEnableFontSubstitution, true);
            substituionSetting.SetSetting(
                CommonTextFontSubstitutionSettingFlag::CommonTextEnableSystemFontSubstitution, true);
        }
        else
            substituionSetting.SetSetting(
                CommonTextFontSubstitutionSettingFlag::CommonTextEnableFontSubstitution, false);
    }

    return true;
}

bool 
UsdImagingCommonText::_GenerateMarkupTextGeometries(
    UsdImagingTextRendererSharedPtr renderer, 
    std::shared_ptr<UsdImagingMarkupText> markupText,
    VtVec3fArray& geometries,
    VtVec4fArray& textCoords,
    VtVec3fArray& textColor,
    VtFloatArray& textOpacity,
    VtVec3fArray& lineColors,
    VtFloatArray& lineOpacities,
    VtVec3fArray& lineGeometries)
{
    std::shared_ptr<CommonTextGenericLayout> genericLayout =
        std::make_shared<CommonTextGenericLayout>();
    CommonTextPosition2DArray point2DArray;
    std::vector<CommonTextDecorationLayout> decorations;
    CommonTextTrueTypeGenericLayoutManager layoutManager =
        CommonTextSystem::Instance()->GetGenericLayoutManager();
    layoutManager.Initialize(markupText, genericLayout, true);
    if (!layoutManager.IsValid())
        return false;
    else
    {
        // Parse the markup text.
        CommonTextStatus status = layoutManager.GenerateGenericLayout();
        if (status != CommonTextStatus::CommonTextStatusSuccess)
            return false;

        // Get the absolute position for all text runs.
        status = layoutManager.GetAbsolutePositionForAllTextRuns(point2DArray);
        if (status != CommonTextStatus::CommonTextStatusSuccess)
            return false;

        // Get decorations of all lines from GenericLayout.
        status = layoutManager.CollectDecorations(decorations);
        if (status != CommonTextStatus::CommonTextStatusSuccess)
            return false;

        UsdImagingTextStyle globalTextStyle = markupText->GlobalTextStyle();
        UsdImagingTextColor defaultColor = markupText->DefaultTextColor();

        // For each text run, we will create the geometries for it.
        std::shared_ptr<UsdImagingTextRunList> textRuns = markupText->ListOfTextRuns();
        auto runIter = textRuns->begin();
        auto runEnd = textRuns->end();
        std::forward_list<CommonTextRunLayout> layoutList = genericLayout->ListOfTextRunLayouts();
        auto layoutIter = layoutList.begin();
        auto layoutEnd = layoutList.end();
        int runIndex = 0;

        for (; runIter != runEnd; runIter++, layoutIter++, runIndex++)
        {
            if (layoutIter == layoutEnd || runIndex >= point2DArray.size())
                return false;

            // Get the color for the text run.
            UsdImagingTextRun& run = *runIter;
            UsdImagingTextColor runColor = run.GetTextColor(defaultColor);
            CommonTextSimpleLayout& layout = layoutIter->SimpleLayout();
            std::pair<float, float>& position = point2DArray[runIndex];

            UsdImagingTextStyle runStyle = run.GetStyle(globalTextStyle);

            // Get the fullSize of the font.
            float scale = 1.0f;
            if (!CommonTextUtilities::GetFullSizeStyle(runStyle, scale))
                return false;

            // The geometry generated by CommonTextSystem will use the left of the baseline as the origin. We will
            // modify it to use top left of the text node as the origin. So here we offset the text in the y
            // direction by the top height of the semantic bound.
            // And we also offset the position with the absolute position of the text run.
            float xOffset = position.first;
            float yOffset = -layout.FullMetrics()._semanticBound.Max()[1] + position.second;

            CommonTextTrueTypeSimpleLayoutManager simpleManager =
                CommonTextSystem::Instance()->GetSimpleLayoutManager(runStyle);

            for (int i = 0; i < layout.CountOfRenderableChars(); i++) {
                CommonTextCharMetrics& metrics = layout.CharacterMetrics(i);
                if (renderer->RequireInput() == TextRendererInputType::TextRendererInputTypeControlPoints)
                {
                    std::shared_ptr<UsdImagingTextRawGlyph> rawGlyphPtr =
                        std::make_shared<UsdImagingTextRawGlyph>(UsdImagingTextRawGlyph());
                    CommonTextBox2<GfVec2i> rasBox;
                    if (simpleManager.GenerateRawGlyph(layout.CharacterIndices()[i], rasBox, *rawGlyphPtr)
                        != CommonTextStatus::CommonTextStatusSuccess) {
                        return false;
                    }

                    std::shared_ptr<TextRendererInput> rendererInput = std::make_shared<ControlPointsInput>(rawGlyphPtr);
                    VtVec3fArray glyphGeometry;
                    VtVec4fArray glyphCoords;
                    if (renderer->GenerateGeometryAndCoords(rendererInput, glyphGeometry, glyphCoords))
                    {
                        for (GfVec3f vertex : glyphGeometry)
                        {
                            vertex[0] = metrics._startPosition + vertex[0] * scale + xOffset;
                            vertex[1] = vertex[1] * scale + yOffset;
                            geometries.emplace_back(vertex);
                            textColor.emplace_back(GfVec3f(runColor.red, runColor.green, runColor.blue));
                            textOpacity.emplace_back(runColor.alpha);
                        }
                        for (GfVec4f coord : glyphCoords)
                            textCoords.emplace_back(coord);
                    }
                    else
                        return false;
                }
            }
        }

        // Generate the geometries for underline, overline and strike through.
        for (auto decoration : decorations)
        {
            float startX = decoration._startXPosition;
            float lineY = decoration._yPosition;
            for (auto section : decoration._sections)
            {
                lineColors.push_back(GfVec3f(section._lineColor.red, section._lineColor.green, section._lineColor.blue));
                lineOpacities.push_back(section._lineColor.alpha);
                lineGeometries.push_back(GfVec3f(startX, lineY, 0));
                lineGeometries.push_back(GfVec3f(section._endXPosition, lineY, 0));
                startX = section._endXPosition;
            }
        }
    }
    return true;
}

bool 
UsdImagingCommonText::_GenerateSimpleTextGeometries(
    UsdImagingTextRendererSharedPtr renderer, 
    const std::string& textData,
    const UsdImagingTextStyle& style,
    VtVec3fArray& geometries,
    VtVec4fArray& textCoords,
    VtVec3fArray& lineGeometries)
{
    // Sometimes the height of the text in world unit is too small. In this case, we set the text
    // height to the full size of the font, generate the geometry, and then multiply the geometry with a
    // scale.
    // Change the font to full-size style, and get the scale ratio between current height and full
    // size.
    UsdImagingTextStyle fullSizeStyle = style;
    float scale = 1.0f;
    if (!CommonTextUtilities::GetFullSizeStyle(fullSizeStyle, scale))
        return false;

    CommonTextTrueTypeSimpleLayoutManager simpleManager =
        CommonTextSystem::Instance()->GetSimpleLayoutManager(fullSizeStyle);

    if (!simpleManager.IsValid())
        return false;
    else
    {
        // Generate the layout.
        CommonTextSimpleLayout layout;
        if (simpleManager.GenerateSimpleLayout(textData, layout)
            != CommonTextStatus::CommonTextStatusSuccess) {
            return false;
        }

        // The geometry generated by CommonTextSystem will use the left of the baseline as the origin. We will
        // modify it to use top left of the text node as the origin. So here we offset the text in the y
        // direction by the top height of the semantic bound.
        float yOffset = layout.FullMetrics()._semanticBound.Max()[1];
        float extentMinY = layout.FullMetrics()._extentBound.Min()[1];

        for (int i = 0; i < layout.CountOfRenderableChars(); i++) {
            CommonTextCharMetrics& metrics = layout.CharacterMetrics(i);
            if (renderer->RequireInput() == TextRendererInputType::TextRendererInputTypeControlPoints)
            {
                std::shared_ptr<UsdImagingTextRawGlyph> rawGlyphPtr = 
                    std::make_shared<UsdImagingTextRawGlyph>(UsdImagingTextRawGlyph());
                CommonTextBox2<GfVec2i> rasBox;
                if (simpleManager.GenerateRawGlyph(layout.CharacterIndices()[i], rasBox, *rawGlyphPtr)
                    != CommonTextStatus::CommonTextStatusSuccess) {
                    return false;
                }
                std::shared_ptr<TextRendererInput> rendererInput = 
                    std::make_shared<ControlPointsInput>(rawGlyphPtr);
                VtVec3fArray glyphGeometry;
                VtVec4fArray glyphCoords;
                if (renderer->GenerateGeometryAndCoords(rendererInput, glyphGeometry, glyphCoords))
                {
                    for (GfVec3f vertex : glyphGeometry)
                    {
                        vertex[0] = (metrics._startPosition + vertex[0]) * scale;
                        vertex[1] = (vertex[1] - yOffset) * scale;

                        geometries.emplace_back(vertex);
                    }
                    for (GfVec4f coord : glyphCoords)
                        textCoords.emplace_back(coord);
                }
                else
                    return false;
            }
            else
                return false;
        }

        CommonTextCharMetrics& firstMetrics = layout.CharacterMetrics(0);
        CommonTextCharMetrics& endMetrics = layout.CharacterMetrics(layout.CountOfRenderableChars() - 1);
        // Add overline curve data.
        if (style._overlineType == UsdImagingTextTokens->normal)
        {
            lineGeometries.emplace_back(GfVec3f(firstMetrics._startPosition * scale, 0, 0));
            lineGeometries.emplace_back(GfVec3f(endMetrics._endPosition * scale, 0, 0));
        }
        // Add underline curve data.
        if (style._underlineType == UsdImagingTextTokens->normal)
        {
            lineGeometries.emplace_back(GfVec3f(firstMetrics._startPosition * scale, (extentMinY - yOffset) * scale, 0));
            lineGeometries.emplace_back(GfVec3f(endMetrics._endPosition * scale, (extentMinY - yOffset) * scale, 0));
        }
        // Add strike through curve data.
        if (style._strikethroughType == UsdImagingTextTokens->normal)
        {
            lineGeometries.emplace_back(GfVec3f(firstMetrics._startPosition * scale, (extentMinY - yOffset) * 0.5 * scale, 0));
            lineGeometries.emplace_back(GfVec3f(endMetrics._endPosition * scale, (extentMinY - yOffset) * 0.5 * scale, 0));
        }
        else if (style._strikethroughType == UsdImagingTextTokens->doubleLines)
        {
            float posFirstLine = CommonTextSystem::Instance()->GetTextGlobalSetting().PosFirstLineOfDoubleStrikethrough();
            lineGeometries.emplace_back(GfVec3f(firstMetrics._startPosition * scale, (extentMinY - yOffset) * posFirstLine * scale, 0));
            lineGeometries.emplace_back(GfVec3f(endMetrics._endPosition * scale, (extentMinY - yOffset) * posFirstLine * scale, 0));
            lineGeometries.emplace_back(GfVec3f(firstMetrics._startPosition * scale, (extentMinY - yOffset) * (1 - posFirstLine) * scale, 0));
            lineGeometries.emplace_back(GfVec3f(endMetrics._endPosition * scale, (extentMinY - yOffset) * (1 - posFirstLine) * scale, 0));
        }

        return true;
    }
}

PXR_NAMESPACE_CLOSE_SCOPE

