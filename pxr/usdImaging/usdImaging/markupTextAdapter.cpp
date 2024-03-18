//
// Copyright 2024 Pixar
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
#include "pxr/usdImaging/usdImaging/markupTextAdapter.h"

#include "pxr/usdImaging/usdImaging/delegate.h"
#include "pxr/usdImaging/usdImaging/markupText.h"
#include "pxr/usdImaging/usdImaging/indexProxy.h"
#include "pxr/usdImaging/usdImaging/markupParser.h"
#include "pxr/usdImaging/usdImaging/text.h"
#include "pxr/usdImaging/usdImaging/textRenderer.h"
#include "pxr/usdImaging/usdImaging/textStyle.h"
#include "pxr/usdImaging/usdImaging/tokens.h"

#include "pxr/imaging/hd/simpleText.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/usdGeom/primvarsAPI.h"
#include "pxr/usd/usdGeom/xformCache.h"

#include "pxr/usd/usdText/simpleText.h"
#include "pxr/usd/usdText/markupText.h"
#include "pxr/usd/usdText/textStyle.h"
#include "pxr/usd/usdText/textStyleAPI.h"
#include "pxr/usd/usdText/columnStyle.h"
#include "pxr/usd/usdText/columnStyleAPI.h"
#include "pxr/usd/usdText/paragraphStyle.h"
#include "pxr/usd/usdText/paragraphStyleAPI.h"

#include "pxr/base/tf/type.h"

#include <codecvt>
#include <locale>

PXR_NAMESPACE_OPEN_SCOPE

// Register the adapter.
TF_REGISTRY_FUNCTION(TfType)
{
    typedef UsdImagingMarkupTextAdapter Adapter;
    TfType t = TfType::Define<Adapter, TfType::Bases<Adapter::BaseAdapter> >();
    t.SetFactory< UsdImagingPrimAdapterFactory<Adapter> >();
}

UsdImagingMarkupTextAdapter::~UsdImagingMarkupTextAdapter()
{
}

bool
UsdImagingMarkupTextAdapter::IsSupported(
    UsdImagingIndexProxy const* index) const
{
    if (!UsdImagingMarkupParser::IsInitialized())
    {
        if (!UsdImagingMarkupParser::DefaultInitialize())
            return false;
    }
    if (!UsdImagingText::IsInitialized())
    {
        if (!UsdImagingText::DefaultInitialize())
            return false;
    }
    return index->IsRprimTypeSupported(HdPrimTypeTokens->markupText);
}

bool 
UsdImagingMarkupTextAdapter::_GenerateMarkupTextGeometries(
    UsdPrim const& prim, 
    UsdTimeCode time, 
    VtVec3fArray& geometries,
    VtVec4fArray& textCoords, 
    VtVec3fArray& textColor, 
    VtFloatArray& textOpacity,
    VtVec3fArray& lineColors, 
    VtFloatArray& lineOpacities, 
    VtVec3fArray& lineGeometries) const
{
    // Get the markup string. 
    UsdTextMarkupText text(prim);
    std::string stringInput;
    text.GetMarkupStringAttr().Get(&stringInput, 0);
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    std::wstring markupString = converter.from_bytes(stringInput);

    // Get the markup language.
    TfToken markupLanguage = UsdTextTokens->noMarkup;
    if (!text.GetMarkupLanguageAttr().Get(&markupLanguage, time)) {
        markupLanguage = UsdTextTokens->noMarkup;
    }

    TextBlockStyleArray blockStyleArray;
    // Get the columnStyle from the ColumnStyleBinding.
    if (UsdTextColumnStyleAPI::CanApply(prim))
    {
        UsdTextColumnStyleAPI::ColumnStyleBinding columnStyleBinding =
            UsdTextColumnStyleAPI(prim).GetColumnStyleBinding(prim.GetPath());
        std::vector<UsdTextColumnStyle> styles = columnStyleBinding.GetColumnStyles();

        // The text prim can bind several column styles, and each represents one column.
        if (styles.size() > 0)
        {
            for (auto style : styles)
            {
                UsdImagingTextBlockStyle blockStyle;
                float columnWidth = 0.0f;
                float columnHeight = 0.0f;
                GfVec2f offset(0.0f, 0.0f);
                GfVec4f margin(0.0f, 0.0f, 0.0f, 0.0f);
                TfToken blockAlignment = UsdTextTokens->top;

                // The column width, height, and offset must be specified.
                if (!TF_VERIFY(style.GetColumnWidthAttr().Get(&columnWidth, time),
                    "The column style must have width."))
                    return false;
                if (!TF_VERIFY(style.GetColumnHeightAttr().Get(&columnHeight, time),
                    "The column style must have height."))
                    return false;
                if (!TF_VERIFY(style.GetOffsetAttr().Get(&offset, time),
                    "The column style must have offset."))
                    return false;
                if (!style.GetMarginsAttr().Get(&margin, time))
                {
                    margin = GfVec4f(0.0f, 0.0f, 0.0f, 0.0f);
                }
                if (!style.GetBlockAlignmentAttr().Get(&blockAlignment, time))
                {
                    blockAlignment = UsdTextTokens->top;
                }

                // Set the columnStyle to the block.
                blockStyle.Width(columnWidth);
                blockStyle.Height(columnHeight);
                if (blockAlignment == UsdTextTokens->bottom)
                    blockStyle.Alignment(UsdImagingBlockAlignment::UsdImagingBlockAlignmentBottom);
                else if (blockAlignment == UsdTextTokens->center)
                    blockStyle.Alignment(UsdImagingBlockAlignment::UsdImagingBlockAlignmentCenter);
                else
                    blockStyle.Alignment(UsdImagingBlockAlignment::UsdImagingBlockAlignmentTop);
                blockStyle.Offset(GfVec2f(offset[0], offset[1]));
                blockStyle.LeftMargin(margin[0]);
                blockStyle.RightMargin(margin[1]);
                blockStyle.TopMargin(margin[2]);
                blockStyle.BottomMargin(margin[3]);

                // Add a block.
                blockStyleArray.push_back(blockStyle);
            }
        }
    }
    else {
        return false;
    }

    UsdImagingTextStyle globalTextStyle;
    // Get the textstyle from the TextStyleBinding.
    if (UsdTextTextStyleAPI::CanApply(prim))
    {
        UsdTextTextStyleAPI::TextStyleBinding textStyleBinding =
            UsdTextTextStyleAPI(prim).GetTextStyleBinding(prim.GetPath());
        UsdTextTextStyle style = textStyleBinding.GetTextStyle();
        if (!style.GetPath().IsEmpty())
        {
            // The typeface and height must be specified.
            if (!TF_VERIFY(style.GetTypefaceAttr().Get(&globalTextStyle._typeface, time),
                "The text style must contain a typeface of the font.")) {
                return false;
            }
            if (!TF_VERIFY(style.GetTextHeightAttr().Get(&globalTextStyle._height, time),
                "The text style must have a height.")) {
                return false;
            }
            // The widthFactor, obliqueAngle, characterSpace, bold, italic, underline,  
            // overline and strikethrough can have default value.
            if (!style.GetTextWidthFactorAttr().Get(&globalTextStyle._widthFactor, time)) {
                globalTextStyle._widthFactor = 1.0f;
            }
            if (!style.GetObliqueAngleAttr().Get(&globalTextStyle._obliqueAngle, time)) {
                globalTextStyle._obliqueAngle = 0.0f;
            }
            if (!style.GetCharSpacingAttr().Get(&globalTextStyle._characterSpaceFactor, time)) {
                globalTextStyle._characterSpaceFactor = 0.0f;
            }
            if (!style.GetBoldAttr().Get(&globalTextStyle._bold, time)) {
                globalTextStyle._bold = false;
            }
            if (!style.GetItalicAttr().Get(&globalTextStyle._italic, time)) {
                globalTextStyle._italic = false;
            }
            std::string lineType = "none";
            if (!style.GetUnderlineTypeAttr().Get(&lineType, time)) {
                lineType = "none";
            }
            if (lineType == "normal")
                globalTextStyle._underlineType = UsdImagingTextTokens->normal;
            else
                globalTextStyle._underlineType = UsdImagingTextTokens->none;

            if (!style.GetOverlineTypeAttr().Get(&lineType, time)) {
                lineType = "none";
            }
            if (lineType == "normal")
                globalTextStyle._overlineType = UsdImagingTextTokens->normal;
            else
                globalTextStyle._overlineType = UsdImagingTextTokens->none;

            if (!style.GetStrikethroughTypeAttr().Get(&lineType, time)) {
                lineType = "none";
            }
            if (lineType == "normal")
                globalTextStyle._strikethroughType = UsdImagingTextTokens->normal;
            else if (lineType == "doubleLines")
                globalTextStyle._strikethroughType = UsdImagingTextTokens->doubleLines;
            else
                globalTextStyle._strikethroughType = UsdImagingTextTokens->none;

        }
    }
    else {
        return false;
    }

    // Get the global text paragraph style from ParagraphStyleBinding.
    TextParagraphStyleArray paragraphStyleArray;
    if (UsdTextParagraphStyleAPI::CanApply(prim))
    {
        UsdTextParagraphStyleAPI::ParagraphStyleBinding paragraphStyleBinding =
            UsdTextParagraphStyleAPI(prim).GetParagraphStyleBinding(prim.GetPath());
        std::vector<UsdTextParagraphStyle> styles = paragraphStyleBinding.GetParagraphStyles();
        if (styles.size() > 0)
        {
            for (auto style : styles)
            {
                UsdImagingTextParagraphStyle paragraphStyle;
                if (!style.GetFirstLineIndentAttr().Get(&paragraphStyle._firstLineIndent, time)) {
                    paragraphStyle._firstLineIndent = -1.0f;
                }
                if (!style.GetLeftIndentAttr().Get(&paragraphStyle._leftIndent, time)) {
                    paragraphStyle._leftIndent = 0.0f;
                }
                if (!style.GetRightIndentAttr().Get(&paragraphStyle._rightIndent, time)) {
                    paragraphStyle._rightIndent = 0.0f;
                }
                if (!style.GetParagraphSpaceAttr().Get(&paragraphStyle._paragraphSpace, time)) {
                    paragraphStyle._paragraphSpace = 0.0f;
                }
                TfToken paragraphAlignment = UsdTextTokens->left;
                if (!style.GetParagraphAlignmentAttr().Get(&paragraphAlignment, time)) {
                    paragraphAlignment = UsdTextTokens->left;
                }
                if (paragraphAlignment == UsdTextTokens->left)
                    paragraphStyle._alignment = UsdImagingParagraphAlignment::UsdImagingParagraphAlignmentLeft;
                else if (paragraphAlignment == UsdTextTokens->right)
                    paragraphStyle._alignment = UsdImagingParagraphAlignment::UsdImagingParagraphAlignmentRight;
                else if (paragraphAlignment == UsdTextTokens->center)
                    paragraphStyle._alignment = UsdImagingParagraphAlignment::UsdImagingParagraphAlignmentCenter;
                else if (paragraphAlignment == UsdTextTokens->justify)
                    paragraphStyle._alignment = UsdImagingParagraphAlignment::UsdImagingParagraphAlignmentJustify;
                else if (paragraphAlignment == UsdTextTokens->distributed)
                    paragraphStyle._alignment = UsdImagingParagraphAlignment::UsdImagingParagraphAlignmentDistribute;
                else
                    paragraphStyle._alignment = UsdImagingParagraphAlignment::UsdImagingParagraphAlignmentNo;

                TfToken lineSpaceType = UsdTextTokens->atLeast;
                if (!style.GetLineSpaceTypeAttr().Get(&lineSpaceType, time)) {
                    lineSpaceType = UsdTextTokens->atLeast;
                }
                if (lineSpaceType == UsdTextTokens->exactly)
                    paragraphStyle._lineSpaceType = UsdImagingLineSpaceType::UsdImagingLineSpaceTypeExactly;
                else if (lineSpaceType == UsdTextTokens->multiple)
                    paragraphStyle._lineSpaceType = UsdImagingLineSpaceType::UsdImagingLineSpaceTypeMulti;
                else
                    paragraphStyle._lineSpaceType = UsdImagingLineSpaceType::UsdImagingLineSpaceTypeAtLeast;

                if (!style.GetLineSpaceAttr().Get(&paragraphStyle._lineSpace, time)) {
                    paragraphStyle._lineSpace = 0.0f;
                }

                // Get the tabstop information.
                VtFloatArray tabStopPositions;
                style.GetTabStopPositionsAttr().Get(&tabStopPositions, time);
                VtTokenArray tabStopTypes;
                style.GetTabStopTypesAttr().Get(&tabStopTypes, time);
                VtTokenArray::iterator typeIter = tabStopTypes.begin();
                for (VtFloatArray::iterator positionIter = tabStopPositions.begin();
                    positionIter != tabStopPositions.end(); positionIter++)
                {
                    UsdImagingTabStop tabStop;
                    tabStop._position = *positionIter;
                    if (typeIter != tabStopTypes.end())
                    {
                        if (*typeIter == UsdTextTokens->leftTab)
                            tabStop._type = UsdImagingTabStopType::UsdImagingTabStopTypeLeft;
                        else if (*typeIter == UsdTextTokens->rightTab)
                            tabStop._type = UsdImagingTabStopType::UsdImagingTabStopTypeRight;
                        else if (*typeIter == UsdTextTokens->centerTab)
                            tabStop._type = UsdImagingTabStopType::UsdImagingTabStopTypeCenter;
                        else if (*typeIter == UsdTextTokens->decimalTab)
                            tabStop._type = UsdImagingTabStopType::UsdImagingTabStopTypeDecimal;
                        else
                            tabStop._type = UsdImagingTabStopType::UsdImagingTabStopTypeLeft;
                        typeIter++;
                    }
                    else
                        tabStop._type = UsdImagingTabStopType::UsdImagingTabStopTypeLeft;
                    paragraphStyle._tabStopList.push_back(tabStop);
                }

                // Add a paragraph.
                paragraphStyleArray.push_back(paragraphStyle);
            }
        }
    }
    else {
        return false;
    }

    // Get the rendering technique of the text prim. By default it is shader based.
    std::string rendererName("");
    if (!text.GetRendererAttr().Get(&rendererName, 0))
    {
        rendererName = std::string("");
    }

    UsdImagingTextRendererSharedPtr renderer = UsdImagingTextRenderer::GetTextRenderer(rendererName);
    if (!TF_VERIFY(renderer, "The text primitive must set a reasonable renderer."))
        return false;

    // First create the markupText from the input.
    // Set the markup string and the language.
    std::shared_ptr<UsdImagingMarkupText> markupText = std::make_shared<UsdImagingMarkupText>();
    markupText->MarkupString(markupString);
    if (markupLanguage == UsdTextTokens->mtext)
    {
        markupText->MarkupLanguage(L"MTEXT");
    }
    else if (markupLanguage == UsdTextTokens->noMarkup)
    {
        markupText->MarkupLanguage(L"");
    }
    else
        // For unsupported markup language, set MarkupLanguage to empty.
        markupText->MarkupLanguage(L"");

    // Set the block information.
    for (auto blockStyle : blockStyleArray)
    {
        UsdImagingTextBlock block(blockStyle);
        markupText->TextBlockArray()->push_back(block);
    }

    // Set the Paragraph information.
    for (auto paragraphStyle : paragraphStyleArray)
    {
        markupText->ParagraphStyleArray()->push_back(paragraphStyle);
    }

    // Set the global text style and global paragraph style.
    markupText->GlobalTextStyle(globalTextStyle);
    if(!paragraphStyleArray.empty())
        markupText->GlobalParagraphStyle(paragraphStyleArray[0]);
    else
        markupText->GlobalParagraphStyle(UsdImagingTextParagraphStyle());

    // Set the default text color.
    VtValue colorValue = Get(prim, SdfPath(), HdTokens->displayColor, time, nullptr);
    const GfVec3f& colorVec = *(colorValue.Get<VtVec3fArray>().begin());
    UsdImagingTextColor defaultColor = { colorVec.data()[0], colorVec.data()[1], colorVec.data()[2]};
    markupText->DefaultTextColor(defaultColor);

    // Parse the string.
    bool result = UsdImagingMarkupParser::ParseText(markupText);
    if(result)
    {
        // Generate the layout for the markupText.
        return UsdImagingText::GenerateMarkupTextGeometries(renderer, markupText,
            geometries, textCoords, textColor, textOpacity, lineColors, lineOpacities, lineGeometries);
    }
    else
        return false;
}

SdfPath
UsdImagingMarkupTextAdapter::Populate(UsdPrim const& prim,
    UsdImagingIndexProxy* index,
    UsdImagingInstancerContext const* instancerContext)
{
    return _AddRprim(HdPrimTypeTokens->markupText,
        prim, index, GetMaterialUsdPath(prim), instancerContext);
}

void
UsdImagingMarkupTextAdapter::TrackVariability(UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits* timeVaryingBits,
    UsdImagingInstancerContext const*
    instancerContext) const
{
    BaseAdapter::TrackVariability(
        prim, cachePath, timeVaryingBits, instancerContext);

    if ((*timeVaryingBits & HdChangeTracker::DirtyTopology) == 0) {
        _IsVarying(prim, UsdTextTokens->markupString,
            HdChangeTracker::DirtyTopology,
            UsdImagingTokens->usdVaryingTopology,
            timeVaryingBits, /*inherited*/false);
    }
    if ((*timeVaryingBits & HdChangeTracker::DirtyTopology) == 0) {
        _IsVarying(prim, UsdTextTokens->markupLanguage,
            HdChangeTracker::DirtyTopology,
            UsdImagingTokens->usdVaryingTopology,
            timeVaryingBits, /*inherited*/false);
    }
}

bool
UsdImagingMarkupTextAdapter::_IsBuiltinPrimvar(TfToken const& primvarName) const
{
    return (primvarName == HdTokens->textCoord
        || primvarName == HdTokens->textColor
        || primvarName == HdTokens->textOpacity
        || primvarName == HdTokens->linePoints
        || primvarName == HdTokens->lineColors
        || primvarName == HdTokens->lineOpacities || BaseAdapter::_IsBuiltinPrimvar(primvarName));
}

void
UsdImagingMarkupTextAdapter::UpdateForTime(
    UsdPrim const& prim,
    SdfPath const& cachePath,
    UsdTimeCode time,
    HdDirtyBits requestedBits,
    UsdImagingInstancerContext const* instancerContext) const
{
    BaseAdapter::UpdateForTime(
        prim, cachePath, time, requestedBits, instancerContext);

    UsdImagingPrimvarDescCache* primvarDescCache = _GetPrimvarDescCache();

    // Geometry aspect
    HdPrimvarDescriptorVector& primvars =
        primvarDescCache->GetPrimvars(cachePath);

    if (requestedBits & HdChangeTracker::DirtyTopology) {
        _MergePrimvar(&primvars, HdTokens->textCoord, HdInterpolationVertex);
    }
    if (requestedBits & HdChangeTracker::DirtyPrimvar) {
        _MergePrimvar(&primvars, HdTokens->textColor, HdInterpolationVertex);
        _MergePrimvar(&primvars, HdTokens->textOpacity, HdInterpolationVertex);
        _MergePrimvar(&primvars, HdTokens->linePoints, HdInterpolationVertex);
        _MergePrimvar(&primvars, HdTokens->lineColors, HdInterpolationConstant);
        _MergePrimvar(&primvars, HdTokens->lineOpacities, HdInterpolationConstant);
    }
}

HdDirtyBits
UsdImagingMarkupTextAdapter::ProcessPropertyChange(UsdPrim const& prim,
    SdfPath const& cachePath,
    TfToken const& propertyName)
{
    // The string and markup language will impact the topology and points.
    if (propertyName == UsdTextTokens->markupString || propertyName == UsdTextTokens->markupLanguage) {
        return HdChangeTracker::DirtyTopology | HdChangeTracker::DirtyPoints;
    }

    // If the property is prefixed with columnStyle or paragraphStyle, we will always dirty 
    // both the topology and points.
    if (TfStringStartsWith(propertyName, HdTextTokens->columnStyle) || 
        TfStringStartsWith(propertyName, HdTextTokens->paragraphStyle))
    {
        return HdChangeTracker::DirtyTopology | HdChangeTracker::DirtyPoints;
    }

    // Allow base class to handle change processing.
    return BaseAdapter::ProcessPropertyChange(prim, cachePath, propertyName);
}

/*virtual*/
VtValue
UsdImagingMarkupTextAdapter::GetTopology(UsdPrim const& prim,
    SdfPath const& cachePath,
    UsdTimeCode time) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TextGeometryCache::const_accessor accessor;
    if (!_textGeometryCache.find(accessor, cachePath)) {
        // Generate the geometry information if it is missing.
        std::shared_ptr<TextGeometry> geometryPtr = std::make_shared<TextGeometry>();
        if (_GenerateMarkupTextGeometries(prim, UsdTimeCode::Default(), geometryPtr->geometries,
            geometryPtr->textCoords, geometryPtr->textColor, geometryPtr->textOpacity,
            geometryPtr->lineColors, geometryPtr->lineOpacities, geometryPtr->lineGeometries)) {
            _textGeometryCache.emplace(accessor, cachePath, geometryPtr);
        }
        else {
            HdMarkupTextTopology topology(0, 0);
            return VtValue(topology);
        }
    }

    // Get the point count from geometries.
    std::shared_ptr<TextGeometry> geometryPtr = accessor->second;
    size_t pointCount = geometryPtr->geometries.size();
    size_t decorationCount = geometryPtr->lineGeometries.size() / 2;
    HdMarkupTextTopology topology(pointCount, decorationCount);
    return VtValue(topology);
}

/*virtual*/
VtValue
UsdImagingMarkupTextAdapter::Get(UsdPrim const& prim,
    SdfPath const& cachePath,
    TfToken const& key,
    UsdTimeCode time,
    VtIntArray *outIndices) const
{
    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    VtValue value;
    UsdGeomGprim gprim(prim);
    if (!TF_VERIFY(gprim)) {
        return value;
    }

    if (key == HdTokens->points || key == HdTokens->textCoord || key == HdTokens->textColor ||
        key == HdTokens->textOpacity || key == HdTokens->linePoints || key == HdTokens->lineColors || key == HdTokens->lineOpacities)
    {
        TextGeometryCache::const_accessor accessor;
        if (!_textGeometryCache.find(accessor, cachePath)) {
            // Generate the geometry information if it is missing.
            std::shared_ptr<TextGeometry> geometryPtr = std::make_shared<TextGeometry>();
            if (_GenerateMarkupTextGeometries(prim, UsdTimeCode::Default(), geometryPtr->geometries,
                geometryPtr->textCoords, geometryPtr->textColor, geometryPtr->textOpacity,
                geometryPtr->lineColors, geometryPtr->lineOpacities, geometryPtr->lineGeometries)) {
                _textGeometryCache.emplace(accessor, cachePath, geometryPtr);
            }
            else {
                return VtValue(VtVec3fArray(0));
            }
        }
        std::shared_ptr<TextGeometry> geometryPtr = accessor->second;

        if (key == HdTokens->points)
            value = geometryPtr->geometries;
        else if (key == HdTokens->textCoord)
            value = geometryPtr->textCoords;
        else if (key == HdTokens->textColor)
            value = geometryPtr->textColor;
        else if (key == HdTokens->textOpacity)
            value = geometryPtr->textOpacity;
        else if (key == HdTokens->lineColors)
            value = geometryPtr->lineColors;
        else if (key == HdTokens->lineOpacities)
            value = geometryPtr->lineOpacities;
        else if (key == HdTokens->linePoints)
            value = geometryPtr->lineGeometries;
        return value;
    }
    return BaseAdapter::Get(prim, cachePath, key, time, outIndices);
}

void
UsdImagingMarkupTextAdapter::_RemovePrim(const SdfPath& cachePath, UsdImagingIndexProxy* index)
{
    _textGeometryCache.erase(cachePath);
    index->RemoveRprim(cachePath);
}

void
UsdImagingMarkupTextAdapter::MarkDirty(UsdPrim const& prim,
    SdfPath const& cachePath,
    HdDirtyBits dirty,
    UsdImagingIndexProxy* index)
{
    // Need to remove the created geometry and other points related information, so that we can
    // recalculate them.
    if (dirty & HdChangeTracker::DirtyPoints) {
        _textGeometryCache.erase(cachePath);
    }
    index->MarkRprimDirty(cachePath, dirty);
}

PXR_NAMESPACE_CLOSE_SCOPE
