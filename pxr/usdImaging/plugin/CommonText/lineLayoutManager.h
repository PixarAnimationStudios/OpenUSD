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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_LINE_LAYOUT_MANAGER_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_LINE_LAYOUT_MANAGER_H

#include "definitions.h"
#include "genericLayout.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \struct CommonTextAccumulateParameterSet
///
/// This struct is a set of parameters used in accumulating the text runs.
///
struct CommonTextAccumulateParameterSet
{
    // The line length after we accumulate the text run
    float _textLineSemanticLength = 0.0f;

    // The line length after we accumulate the text run, without the end spaces.
    float _textLineExtentLength = 0.0f;

    // The line ascent after we accumulate the text run
    float _ascent = 0.0f;

    // The line descent after we accumulate the text run
    float _descent = 0.0f;

    // The current tabstop type.
    UsdImagingTabStopType _tabStopType = UsdImagingTabStopType::UsdImagingTabStopTypeInvalid;

    // The remain space of current tab textrun.
    float _tabTextRunRemainSpace = -1.0f;
};

/// \struct CommonTextBreakInfo
///
/// The information where we will break a line.
///
struct CommonTextBreakInfo
{
    UsdImagingTextRunList::iterator _breakRunIter;
    std::forward_list<CommonTextRunLayout>::iterator _breakRunLayoutIter;
    WordBreakIndexList::iterator _breakRunWordBreakIter;
    int _breakIndexInTextRun;
};

/// \struct CommonTextLineBreakTestInfo
///
/// The information when we do line break test.
///
struct CommonTextLineBreakTestInfo
{
    float _lineSemanticLength = 0.0f;
    float _lineExtentLength   = 0.0f;
    float _lineAscent         = 0.0f;
    float _lineDescent        = 0.0f;
    bool _ifLineBreak         = false;
};

/// \class CommonTextLineLayoutManager
///
/// The layout manager for a line.
///
class CommonTextLineLayoutManager
{
public:
    /// The constructor.
    CommonTextLineLayoutManager() = default;

    /// Initialize a line layout manager.
    CommonTextStatus Initialize(std::shared_ptr<UsdImagingMarkupText> markupText,
                                std::shared_ptr<CommonTextGenericLayout> genericLayout, 
                                std::shared_ptr<CommonTextIntermediateInfo> intermediateInfo,
                                UsdImagingTextLineList::iterator lineIter,
                                CommonTextLineLayoutList::iterator lineLayoutIter, 
                                const UsdImagingTextParagraphStyle* paragraphStyle,
                                const UsdImagingTextStyle& defaultTextStyle, 
                                float constraintInBaseline, 
                                float constraintInFlow);

    /// If the height of the line will overflow the height constraint.
    bool IsFlowOverflow(float height) const;

    /// If the length of the line will overflow the length constraint.
    bool IsBaselineOverflow(float length) const;

    /// Analyze if each character can be a line break.
    CommonTextStatus Analyze(bool bAllowLineBreakInWord, 
                             bool bAllowLineBreakBetweenScripts);

    /// Check if we need to do line break in this line. If yes, return the break information.
    CommonTextStatus BreakTest(CommonTextLineBreakTestInfo& breakTestInfo, 
                               CommonTextBreakInfo& wordBreakInfo);

    /// Do line break using the break information.
    CommonTextStatus BreakLine(const CommonTextBreakInfo& wordBreakInfo, 
                               UsdImagingTextLine& newTextLine, 
                               CommonTextLineLayout& newLineLayout);

    /// Set the positions for textruns in the line.
    CommonTextStatus RepositionTextRuns(float lineExtentLength);

    /// Generate decorations for line layout.
    CommonTextStatus GenerateDecorations(const UsdImagingTextColor& defaultColor);

private:
    enum class CommonTextAccumulateStatus
    {
        // Accumulate a string textrun and finish normally.
        CommonTextAccumulateStatusNormal,

        // Accumulate a tab textrun.
        CommonTextAccumulateStatusTabTextRun,

        // The tab is finished, either because it is a decimal tab and we met with a decimal
        // point, or a string textrun has fully filled the space of current tab textrun.
        CommonTextAccumulateStatusFinishTabTextRun,

        // Accumulation has problem.
        CommonTextAccumulateStatusFail
    };

    CommonTextStatus _HandleDecimalTab(UsdImagingTextRunList::iterator textRunIter,
                                       std::forward_list<CommonTextRunLayout>::iterator textRunLayoutIter,
                                       WordBreakIndexList::iterator wordBreakIter);

    /// Accumulate the textrun into the current text line. Calculate the semantic length, the
    /// extent length, ascent and descent after the accumulation.
    /// If length is smaller than zero, we accumulate the whole run. If length is larger, we 
    /// accumulate the part of the textrun.
    CommonTextAccumulateStatus _AccumulateTextRun(UsdImagingTextRunList::iterator textRunIter, 
                                                  const CommonTextSimpleLayout& layout,
                                                  int startOffset, 
                                                  int length, 
                                                  const CommonTextAccumulateParameterSet& inSet,
                                                  CommonTextAccumulateParameterSet& outSet) const;

    std::shared_ptr<UsdImagingMarkupText> _markupText = nullptr;
    std::shared_ptr<CommonTextGenericLayout> _genericLayout = nullptr;
    std::shared_ptr<CommonTextIntermediateInfo> _intermediateInfo = nullptr;
    UsdImagingTextLineList::iterator _lineIter;
    CommonTextLineLayoutList::iterator _lineLayoutIter;
    UsdImagingTextParagraphStyle* _paragraphStyle = nullptr;;
    UsdImagingTextStyle _defaultTextStyle;
    float _constraintInBaseline = -1.0f;
    float _constraintInFlow     = -1.0f;

    float _doubleLinesStrikethroughFirst = 0.6f;
    float _doubleLinesStrikethroughSecond = 0.4f;
};
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_LINE_LAYOUT_MANAGER_H
