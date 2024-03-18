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

#include "lineLayoutManager.h"
#include "languageAttribute.h"
#include "system.h"

PXR_NAMESPACE_OPEN_SCOPE

CommonTextStatus 
CommonTextLineLayoutManager::Initialize(
    std::shared_ptr<UsdImagingMarkupText> markupText, 
    std::shared_ptr<CommonTextGenericLayout> genericLayout,
    std::shared_ptr<CommonTextIntermediateInfo> intermediateInfo,
    UsdImagingTextLineList::iterator lineIter, 
    CommonTextLineLayoutList::iterator layoutIter, 
    const UsdImagingTextParagraphStyle* paragraphStyle,
    const UsdImagingTextStyle& defaultTextStyle, 
    float constraintInBaseline, 
    float constraintInFlow)
{
    if (!markupText)
        return CommonTextStatus::CommonTextStatusInvalidArg;

    _markupText          = markupText;
    _genericLayout        = genericLayout;
    _intermediateInfo     = intermediateInfo;
    _lineIter             = lineIter;
    _lineLayoutIter       = layoutIter;
    _paragraphStyle       = const_cast<UsdImagingTextParagraphStyle*>(paragraphStyle);
    _defaultTextStyle     = defaultTextStyle;
    _constraintInBaseline = constraintInBaseline;
    _constraintInFlow     = constraintInFlow;

    _doubleLinesStrikethroughFirst = CommonTextSystem::Instance()->GetTextGlobalSetting().PosFirstLineOfDoubleStrikethrough();
    _doubleLinesStrikethroughSecond = 1 - _doubleLinesStrikethroughFirst;
    return CommonTextStatus::CommonTextStatusSuccess;
}

bool 
CommonTextLineLayoutManager::IsFlowOverflow(float height) const
{
    // If _constraintInFlow is smaller than zero, it means that there is no constraint.
    if (_constraintInFlow < 0.0f)
        return false;
    else
        return _constraintInFlow < height;
}

bool 
CommonTextLineLayoutManager::IsBaselineOverflow(float length) const
{
    // If _constraintInBaseline is smaller than zero, it means that there is no constraint.
    if (_constraintInBaseline < 0.0f)
        return false;
    else
    {
        // If there is paragraph style, we need to subtract the indent from the constraint.
        if (_paragraphStyle)
        {
            return (_constraintInBaseline - _paragraphStyle->_rightIndent) < length;
        }
        else
            return _constraintInBaseline < length;
    }
}

CommonTextStatus 
CommonTextLineLayoutManager::Analyze(
    bool bAllowLineBreakInWord, 
    bool /*bAllowLineBreakBetweenScripts*/)
{
    // The layout manager must be initialized.
    if (!_markupText)
        return CommonTextStatus::CommonTextStatusFail;
    // If the line is zero or invalid, we don't need to analyze.
    if (_lineIter->LineType() == UsdImagingTextLineType::UsdImagingTextLineTypeZero || 
        _lineIter->LineType() == UsdImagingTextLineType::UsdImagingTextLineTypeInvalid)
        return CommonTextStatus::CommonTextStatusSuccess;

    // The start and the end of the text runs.
    UsdImagingTextRunRange range = _lineIter->Range();
    auto start               = range._firstRun;
    auto end                 = range._lastRun;
    end++;

    // Initialize the wordbreak list.
    WordBreakIndexList& wordBreakList = _intermediateInfo->GetWordBreakIndexList(_lineIter);
    wordBreakList.clear();
    auto wordBreakIter = wordBreakList.before_begin();

    const std::wstring markupString                  = _markupText->MarkupString();
    const CommonTextLanguageAttributeSet& languageAttributeSet = GetLanguageAttributeSet();
    for (auto it = start; it != end; it++)
    {
        UsdImagingTextRun& run = *it;
        switch (run.Type())
        {
        case UsdImagingTextRunType::UsdImagingTextRunTypeTab:
        {
            // For tabRun, normally there is no word break.
            CommonTextWordBreakIndex wordBreakIndex;

            auto nextIt = it;
            ++nextIt;
            if (nextIt != end && nextIt->Type() == UsdImagingTextRunType::UsdImagingTextRunTypeTab)
            {
                // If the next textrun is a tabrun, we need to add the last position to the word
                // break index.
                wordBreakIndex._breakIndexInTextRun.push_back(1);
            }
            wordBreakIter = wordBreakList.emplace_after(wordBreakIter, wordBreakIndex);
            break;
        }
        case UsdImagingTextRunType::UsdImagingTextRunTypeString:
        {
            // For each character, check if it belongs to any language attributes, and then check if
            // it is the same as the word break of the language attributes.
            CommonTextWordBreakIndex wordBreakIndex;
            for (int i = 0; i < run.Length(); i++)
            {
                wchar_t wch = markupString[i + run.StartIndex()];
                for (const auto& attribute : languageAttributeSet)
                {
                    if (wch >= attribute._startIndex && wch <= attribute._endIndex)
                    {
                        if (bAllowLineBreakInWord || !attribute._haveWordBreakCharacter ||
                            wch == attribute._wordBreakCharacter)
                        {
                            // If we allow line break inside a word, or if the language doesn't
                            // have word break character, or if the character is just the word
                            // break character, we will view this character as a word break.
                            wordBreakIndex._breakIndexInTextRun.push_back(i);
                        }
                        break;
                    }
                }
            }

            // Check if the next text run is a tabrun.
            auto nextIt = it;
            ++nextIt;
            if (nextIt != end && nextIt->Type() == UsdImagingTextRunType::UsdImagingTextRunTypeTab)
            {
                // If the next textrun is a tabrun, we need to add the last position to the word
                // break index.
                auto lastIndex = wordBreakIndex._breakIndexInTextRun.rbegin();
                if (lastIndex == wordBreakIndex._breakIndexInTextRun.rend() ||
                    (*lastIndex) != run.Length() - 1)
                    wordBreakIndex._breakIndexInTextRun.push_back(run.Length() - 1);
            }
            wordBreakIter = wordBreakList.emplace_after(wordBreakIter, wordBreakIndex);
            break;
        }
        default:
            break;
        }
    }
    return CommonTextStatus::CommonTextStatusSuccess;
}

void 
_FillTabTextRunLayout(CommonTextSimpleLayout& simpleLayout, 
                      float tabTextRunRemainSpace)
{
    simpleLayout.FullMetrics()._extentBound.Clear();
    simpleLayout.FullMetrics()._semanticBound.Min(GfVec2f(0, 0));
    simpleLayout.FullMetrics()._semanticBound.Max(GfVec2f(tabTextRunRemainSpace, 0.0f));
}

CommonTextStatus 
CommonTextLineLayoutManager::BreakTest(
    CommonTextLineBreakTestInfo& breakTestInfo, 
    CommonTextBreakInfo& wordBreakInfo)
{
    if (!_markupText)
        return CommonTextStatus::CommonTextStatusFail;
    // If the line is zero or invalid, we don't need to do break test.
    if (_lineIter->LineType() == UsdImagingTextLineType::UsdImagingTextLineTypeZero || 
        _lineIter->LineType() == UsdImagingTextLineType::UsdImagingTextLineTypeInvalid)
        return CommonTextStatus::CommonTextStatusSuccess;

    breakTestInfo._ifLineBreak        = false;
    breakTestInfo._lineSemanticLength = 0.0f;
    breakTestInfo._lineExtentLength   = 0.0f;
    breakTestInfo._lineAscent         = 0.0f;
    breakTestInfo._lineDescent        = 0.0f;

    CommonTextAccumulateParameterSet inSet;
    if (_paragraphStyle)
    {
        if (_lineIter->ParagraphStart() && _paragraphStyle->_firstLineIndent >= 0.0f)
        {
            inSet._textLineSemanticLength += _paragraphStyle->_firstLineIndent;
        }
        else
            inSet._textLineSemanticLength += _paragraphStyle->_leftIndent;
    }
    CommonTextAccumulateParameterSet outSet;

    // breakRunInSet saves the inSet when we accumulate the textrun that we do line break.
    CommonTextAccumulateParameterSet breakRunInSet;

    // The current tab textrun layout iterator.
    CommonTextRunLayoutList::iterator currentTabRunLayoutIter;

    // Iterate the textruns in the line.
    auto start = _lineIter->Range()._firstRun;
    auto end   = _lineIter->Range()._lastRun;
    end++;

    // Get the iterator to the wordbreaks in the line.
    WordBreakIndexList& wordBreakList = _intermediateInfo->GetWordBreakIndexList(_lineIter);
    auto wordBreakIter                = wordBreakList.begin();

    // Get the iterator to the textruns layout.
    auto layoutStart = _lineLayoutIter->Range()._firstRunLayout;
    auto layoutEnd   = _lineLayoutIter->Range()._lastRunLayout;
    layoutEnd++;
    auto currentLayoutIter = layoutStart;

    // The break run information.
    bool hasBreakRun           = false;
    auto breakRunIter          = start;
    auto breakRunWordBreakIter = wordBreakIter;
    auto breakRunLayoutIter    = currentLayoutIter;
    auto breakRunTabLayoutIter = currentTabRunLayoutIter;
    for (auto iter = start; iter != end; iter++)
    {
        UsdImagingTextRun& run                  = *iter;
        CommonTextRunLayout& layout         = *currentLayoutIter;
        CommonTextWordBreakIndex wordBreakIndex = *wordBreakIter;

        // If the currrent tabstop type is decimal, we need to check if there is decimal point
        // in the textrun. If there is, break the textrun at the position after the decimal
        // point.
        if(inSet._tabStopType == UsdImagingTabStopType::UsdImagingTabStopTypeDecimal)
            _HandleDecimalTab(iter, currentLayoutIter, wordBreakIter);
        // Accumulate the whole textrun.
        CommonTextAccumulateStatus accumulateStatus =
            _AccumulateTextRun(iter, layout.SimpleLayout(), 0, -1, inSet, outSet);
        if (accumulateStatus == CommonTextAccumulateStatus::CommonTextAccumulateStatusFail)
            return CommonTextStatus::CommonTextStatusFail;

        // Check if the length will overflow the baseline constraint.
        if (IsBaselineOverflow(outSet._textLineExtentLength))
        {
            // These are the cases that we may meet when we do line break:
            // Group A: the current textrun is a string textrun.
            // Case AA: there is no line break in the textrun, and there is no line break before
            //          this textrun. We will not do line break, but continue accumulation.
            // Case AB: there is no line break in the textrun, and there is line break before this
            //          textrun. We will do line break at the previous line break.
            // Case AC: there is line break in the textrun, and the first line break will overflow
            //          the baseline, and there is no line break before this textrun. We will do
            //          line break at the first line break.
            // Case AD: there is line break in the textrun, and the first line break will overflow
            //          the baseline, and there is line break before this textrun. We will do line
            //          break at previous line break.
            // Case AE: there is line break in the textrun, and the ith line break will not overflow
            //          the baseline, but the (i+1)th line break will overflow the baseline. We will
            //          do line break at the ith line break.
            // Case AF: there is line break in the textrun, and the last line break will not
            //          overflow the baseline. We will do line break at the last line break.
            // Group B: the current textrun is a tab textrun.
            // Case BA: the next textrun is not a tab, so there is no line break in this textrun,
            //          and there is no line break before this textrun. We will not do line break,
            //          but continue accumulation.
            // Case BB: the next textrun is not a tab, so there is no line break in this textrun,
            //          and there is line break before this textrun. We will do line break at the
            //          previous line break.
            // Case BC: the next textrun is a tab, so there is line break at the end of this
            //          textrun, and there is no line break before this textrun. We will do line
            //          break at the end of this textrun.
            // Case BD: the next textrun is a tab, so there is line break at the end of this
            //          textrun, and there is line break before this textrun. We will do line break
            //          at the previous line break.

            // If the length overflow, we will try to find line break position.
            bool findBreak = false;
            CommonTextAccumulateParameterSet lineBreakOutSet;

            if (wordBreakIndex._breakIndexInTextRun.size() > 0)
            {
                // Case AC, AD, AE, AF, BC and BD go to this branch.
                int prevBreakIndex = 0;
                // If this textrun is not a tab, we may find the line break within the textrun.
                if (run.Type() != UsdImagingTextRunType::UsdImagingTextRunTypeTab)
                {
                    // Case AC, AD, AE and AF go to this branch.
                    for (const auto& breakIndex : wordBreakIndex._breakIndexInTextRun)
                    {
                        // Accumulate from the accumulate length of the previous text run to the
                        // currentwordbreak.
                        accumulateStatus = _AccumulateTextRun(iter, CommonTextSimpleLayout(),
                            prevBreakIndex, breakIndex - prevBreakIndex, inSet, outSet);
                        if (accumulateStatus == CommonTextAccumulateStatus::CommonTextAccumulateStatusFail)
                            return CommonTextStatus::CommonTextStatusFail;

                        // Check if the length will overflow the baseline constraint.
                        if (IsBaselineOverflow(outSet._textLineExtentLength))
                        {
                            // Case AC, AD and AE go to this branch.
                            if (prevBreakIndex != 0)
                            {
                                // Case AE goes here.
                                // We already find a word break position which will not overflow,
                                // the previous break index is the position we should do line
                                // break.
                                // The inSet saves the accumulated length of the previous word
                                // break, which is the accumulated length at the line break. So set
                                // lineBreakOutSet to inSet.
                                lineBreakOutSet                      = inSet;
                                wordBreakInfo._breakRunIter          = iter;
                                wordBreakInfo._breakRunLayoutIter    = currentLayoutIter;
                                wordBreakInfo._breakRunWordBreakIter = wordBreakIter;
                                wordBreakInfo._breakIndexInTextRun   = prevBreakIndex;
                                breakRunTabLayoutIter                = currentTabRunLayoutIter;
                                findBreak                            = true;
                                hasBreakRun                          = true;
                            }
                            // Case AC and AD will break out.
                            break;
                        }
                        else
                        {
                            // If not, save the current break index.
                            inSet          = outSet;
                            prevBreakIndex = breakIndex;
                        }
                        // Case AF will not stop but go on.
                    }
                }
                if (!findBreak)
                {
                    // Case AC, AD, AF, BC and BD go to this branch.
                    if (!IsBaselineOverflow(outSet._textLineExtentLength))
                    {
                        // Case AF goes here.
                        // If until the last word break position, the length will not overflow the
                        // baseline constraint, but as we know the whole textrun will overflow, we
                        // can conclude that the last word break position is where we should do line
                        // break.
                        // The outSet saves the accumulated length of the last word break, which
                        // is the accumulated length at the line break. So we set lineBreakOutSet to
                        // outSet.
                        lineBreakOutSet                      = outSet;
                        wordBreakInfo._breakRunIter          = iter;
                        wordBreakInfo._breakRunLayoutIter    = currentLayoutIter;
                        wordBreakInfo._breakRunWordBreakIter = wordBreakIter;
                        wordBreakInfo._breakIndexInTextRun   = prevBreakIndex;
                        breakRunTabLayoutIter                = currentTabRunLayoutIter;
                        findBreak                            = true;
                        hasBreakRun                          = true;
                    }
                    else if (!hasBreakRun)
                    {
                        // Case AC and BC go here.
                        // If hasBreakRun is null, it means there is no word break before this
                        // textrun. In this case, we break in the first word break of this textrun.
                        // The outSet saves the accumulated length of the first word break, which
                        // is the accumulated length at the line break. So we set lineBreakOutSet to
                        // outSet.
                        lineBreakOutSet                      = outSet;
                        wordBreakInfo._breakRunIter          = iter;
                        wordBreakInfo._breakRunLayoutIter    = currentLayoutIter;
                        wordBreakInfo._breakRunWordBreakIter = wordBreakIter;
                        wordBreakInfo._breakIndexInTextRun = wordBreakIndex._breakIndexInTextRun[0];
                        breakRunTabLayoutIter              = currentTabRunLayoutIter;
                        findBreak                          = true;
                        hasBreakRun                        = true;
                    }
                    // Case AD and BD will go on.
                }
            }

            if (!findBreak && hasBreakRun)
            {
                // Case AB, AD, BB and BD go here.
                // If we don't find break, and there is already word break before this text run, we
                // will do line break at the previous word break.
                inSet            = breakRunInSet;
                int breakIndex   = breakRunWordBreakIter->_breakIndexInTextRun.back();
                accumulateStatus = _AccumulateTextRun(
                    breakRunIter, CommonTextSimpleLayout(), 0, breakIndex + 1, inSet, outSet);
                if (accumulateStatus == CommonTextAccumulateStatus::CommonTextAccumulateStatusFail)
                    return CommonTextStatus::CommonTextStatusFail;

                // Set lineBreakOutSet to save the length at the line break.
                lineBreakOutSet                      = outSet;
                wordBreakInfo._breakRunIter          = breakRunIter;
                wordBreakInfo._breakRunLayoutIter    = breakRunLayoutIter;
                wordBreakInfo._breakRunWordBreakIter = breakRunWordBreakIter;
                wordBreakInfo._breakIndexInTextRun   = breakIndex;
                findBreak                            = true;
            }

            if (findBreak)
            {
                // Case AB, AC, AD, AE, AF, BB, BC and BD all go here. Line break is found, the
                // accumulation will end.
                // If there the layout of tab textrun at the line break is not filled, we first
                // fill the layout.
                if (lineBreakOutSet._tabStopType != UsdImagingTabStopType::UsdImagingTabStopTypeInvalid)
                {
                    // After the whole line is processed, we finish the current tab textrun layout.
                    _FillTabTextRunLayout(breakRunTabLayoutIter->SimpleLayout(),
                        lineBreakOutSet._tabTextRunRemainSpace);
                }

                breakTestInfo._ifLineBreak        = true;
                breakTestInfo._lineSemanticLength = lineBreakOutSet._textLineSemanticLength;
                breakTestInfo._lineExtentLength   = lineBreakOutSet._textLineExtentLength;
                breakTestInfo._lineAscent         = lineBreakOutSet._ascent;
                breakTestInfo._lineDescent        = lineBreakOutSet._descent;

                return CommonTextStatus::CommonTextStatusSuccess;
            }
            else
            {
                // If we still don't find break position, it means until this text run, there is
                // no word break. In this case, just continue.
                inSet = outSet;
            }
        }
        else
        {
            // The length doesn't overflow the baseline. First we check if we need to fill the
            // layout of the tab textrun.
            if (accumulateStatus == CommonTextAccumulateStatus::CommonTextAccumulateStatusFinishTabTextRun)
            {
                // The current tab textrun is filled by the string text run, or a decimal tab stop is
                // finished, so we need to finish the current tab textrun layout using the new calculated
                // remain space. outSet._tabTextRunRemainSpace is the new calculated remain space.
                _FillTabTextRunLayout(currentTabRunLayoutIter->SimpleLayout(), outSet._tabTextRunRemainSpace);
                outSet._tabStopType = UsdImagingTabStopType::UsdImagingTabStopTypeInvalid;
            }
            else if (accumulateStatus == CommonTextAccumulateStatus::CommonTextAccumulateStatusTabTextRun)
            {
                if (inSet._tabStopType != UsdImagingTabStopType::UsdImagingTabStopTypeInvalid)
                {
                    // A new tab textrun is accumulated, so we finish the previous tab textrun
                    // layout using the previous calculated remain space. The inSet._tabTextRunRemainSpace
                    // is the pervious calculated remain space.
                    _FillTabTextRunLayout(
                        currentTabRunLayoutIter->SimpleLayout(), inSet._tabTextRunRemainSpace);
                }

                if (outSet._tabStopType == UsdImagingTabStopType::UsdImagingTabStopTypeLeft)
                {
                    // The current textrun is left tabstop textrun, in this case, we don't need to 
                    // insert new textrun into the space of the tab textrun. The textruns after the 
                    // tab will be handled just like there is no tab. So we finish the current tab
                    // textrun layout, and set the tabstop to invalid. The current tab textrun layout
                    // is finished using the new calculated remain space, which is the outSet._tabTextRunRemainSpace.
                    _FillTabTextRunLayout(
                        currentLayoutIter->SimpleLayout(), outSet._tabTextRunRemainSpace);

                    outSet._tabStopType = UsdImagingTabStopType::UsdImagingTabStopTypeInvalid;
                }
                else
                    // If the current tabstop is not left tabstop, we will save the iterator of the
                    // layout, so that in the future we can fill it.
                    currentTabRunLayoutIter = currentLayoutIter;
            }

            // If there is word break within the textrun, we save the current textrun iter, and
            // continue to next textrun.
            if (wordBreakIndex._breakIndexInTextRun.size() > 0)
            {
                breakRunInSet         = inSet;
                breakRunIter          = iter;
                breakRunLayoutIter    = currentLayoutIter;
                breakRunWordBreakIter = wordBreakIter;
                breakRunTabLayoutIter = currentTabRunLayoutIter;
                hasBreakRun           = true;
            }
            inSet = outSet;
        }
        currentLayoutIter++;
        wordBreakIter++;
    }
    // We reach the end of the line and there is still no line break.
    // We check if the layout of the final tab is filled. If not, fill it.
    if (outSet._tabStopType != UsdImagingTabStopType::UsdImagingTabStopTypeInvalid)
    {
        // After the whole line is processed, we finish the current tab textrun layout.
        _FillTabTextRunLayout(
            currentTabRunLayoutIter->SimpleLayout(), outSet._tabTextRunRemainSpace);
    }

    // There is no line break.
    breakTestInfo._ifLineBreak        = false;
    breakTestInfo._lineSemanticLength = outSet._textLineSemanticLength;
    breakTestInfo._lineExtentLength   = outSet._textLineExtentLength;
    breakTestInfo._lineAscent         = outSet._ascent;
    breakTestInfo._lineDescent        = outSet._descent;
    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextStatus
CommonTextLineLayoutManager::_HandleDecimalTab(
    UsdImagingTextRunList::iterator textRunIter,
    std::forward_list<CommonTextRunLayout>::iterator textRunLayoutIter,
    WordBreakIndexList::iterator wordBreakIter
)
{
    CommonTextRunInfo& textRunInfo = _intermediateInfo->GetTextRunInfo(textRunIter);
    std::wstring characters(
        _markupText->MarkupString(), textRunIter->StartIndex(), textRunIter->Length());
    // If the tabstop type is decimal, we need to check if there is decimal point in the string.
    size_t pointPos = characters.find(L'.');
    if (pointPos != std::wstring::npos)
    {
        // If there is decimal point in the middle of the text run, find the first one and divide
        // the text run at the position after decimal point.
        if (pointPos != 0)
        {
            std::vector<int> dividePos;
            dividePos.push_back(pointPos);
            UsdImagingTextRunList::iterator lastSubRunIter = textRunIter;
            CommonTextStatus divideStatus = CommonTextTrueTypeGenericLayoutManager::DivideTextRun(
                _markupText, _intermediateInfo, textRunIter, dividePos, _lineIter, lastSubRunIter);
            if (divideStatus != CommonTextStatus::CommonTextStatusSuccess)
                return divideStatus;

            // Regenerate the SimpleLayout for the divided textrun.
            textRunLayoutIter->SimpleLayout().Reset();
            const UsdImagingTextStyle& textStyle = textRunIter->GetStyle(_defaultTextStyle);
            CommonTextTrueTypeSimpleLayoutManager simpleManager =
                CommonTextSystem::Instance()->GetSimpleLayoutManager(textStyle);
            if (simpleManager.IsValid())
            {
                // Generate simple layout for the textrun before the point.
                std::wstring intergerChar(
                    _markupText->MarkupString(), textRunIter->StartIndex(), textRunIter->Length());
                textRunInfo = _intermediateInfo->GetTextRunInfo(textRunIter);
                CommonTextStatus status = simpleManager.GenerateSimpleLayout(
                    intergerChar, textRunLayoutIter->SimpleLayout(),
                    textRunInfo.ComplexScriptInformation());
                if (status != CommonTextStatus::CommonTextStatusSuccess)
                    return status;

                // Generate simple layout for the textrun after the point.
                std::wstring fractionalChar(
                    _markupText->MarkupString(), lastSubRunIter->StartIndex(), lastSubRunIter->Length());
                textRunInfo = _intermediateInfo->GetTextRunInfo(lastSubRunIter);
                CommonTextRunLayout newLayout;
                status = simpleManager.GenerateSimpleLayout(
                    fractionalChar, newLayout.SimpleLayout(),
                    textRunInfo.ComplexScriptInformation());
                if (status != CommonTextStatus::CommonTextStatusSuccess)
                    return status;
                CommonTextRunLayoutList::iterator newTextRunLayoutIter =
                    _genericLayout->ListOfTextRunLayouts().insert_after(
                        textRunLayoutIter, std::move(newLayout));
                if (_lineLayoutIter->Range()._lastRunLayout == textRunLayoutIter)
                    _lineLayoutIter->Range()._lastRunLayout = newTextRunLayoutIter;

                // The wordbreak array of the textrun should also break into two.
                CommonTextWordBreakIndex& wordBreakIndex = *wordBreakIter;
                std::vector<int> indices = wordBreakIndex._breakIndexInTextRun;
                int textRunLength = textRunIter->Length();
                CommonTextWordBreakIndex newWorkdBreakIndex;
                size_t count = indices.size();
                for (size_t i = 0; i < count; ++i)
                {
                    if (indices[i] >= textRunLength)
                    {
                        for (size_t j = i; j < count; ++j)
                        {
                            newWorkdBreakIndex._breakIndexInTextRun.push_back(indices[j] - textRunLength);
                        }
                        wordBreakIndex._breakIndexInTextRun.resize(i);
                        break;
                    }
                }
                WordBreakIndexList& wordBreakIndexList = _intermediateInfo->GetWordBreakIndexList(_lineIter);
                wordBreakIndexList.insert_after(wordBreakIter, newWorkdBreakIndex);
            }
        }
    }
    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextLineLayoutManager::CommonTextAccumulateStatus 
CommonTextLineLayoutManager::_AccumulateTextRun(
    UsdImagingTextRunList::iterator textRunIter,
    const CommonTextSimpleLayout& layout, 
    int startOffset,
    int length, 
    const CommonTextAccumulateParameterSet& inSet,
    CommonTextAccumulateParameterSet& outSet) const
{
    CommonTextRunInfo& textRunInfo = _intermediateInfo->GetTextRunInfo(textRunIter);
    // First we set outSet to inSet.
    CommonTextAccumulateStatus returnStatus = CommonTextAccumulateStatus::CommonTextAccumulateStatusNormal;
    outSet = inSet;
    switch (textRunIter->Type())
    {
    case UsdImagingTextRunType::UsdImagingTextRunTypeString:
    {
        // This textrun is a string.
        CommonTextBox2<GfVec2f> semanticBound;
        CommonTextBox2<GfVec2f> extentBound;
        if (length < 0)
        {
            // If the length is smaller than zero, it means we will accumulate the whole textrun.
            // In this case, we can directly use the layout.
            semanticBound = layout.FullMetrics()._semanticBound;
            extentBound   = layout.FullMetrics()._extentBound;
        }
        else
        {
            // Accumulates a part of the textrun.
            // For a string text run, first we get the length of the part.
            const UsdImagingTextStyle& textStyle = textRunIter->GetStyle(_defaultTextStyle);
            CommonTextTrueTypeSimpleLayoutManager simpleManager =
                CommonTextSystem::Instance()->GetSimpleLayoutManager(textStyle);
            if (simpleManager.IsValid())
            {
                CommonTextSimpleLayout newLayout;
                std::wstring characters(
                    _markupText->MarkupString(), textRunIter->StartIndex() + startOffset, length);
                CommonTextStatus status = simpleManager.GenerateSimpleLayout(
                    characters, newLayout, textRunInfo.ComplexScriptInformation());
                if (status == CommonTextStatus::CommonTextStatusSuccess)
                {
                    semanticBound = newLayout.FullMetrics()._semanticBound;
                    extentBound   = newLayout.FullMetrics()._extentBound;
                }
            }
        }

        // If the semanticBound is not empty, increase the semantic length.
        if (!semanticBound.IsEmpty())
        {
            // Handle tabstop.
            if (inSet._tabStopType != UsdImagingTabStopType::UsdImagingTabStopTypeInvalid)
            {
                switch (inSet._tabStopType)
                {
                    // For right tabstop, the text is inserted inside the tab textrun space until
                    // the tab textrun space is full.
                case UsdImagingTabStopType::UsdImagingTabStopTypeRight:
                    if (inSet._tabTextRunRemainSpace >= semanticBound.Max()[0])
                    {
                        // There is still enough tab textrun space, so simply subtract the semantic
                        // length of the textrun from the remain space. The total semantic length
                        // is not increased.
                        outSet._tabTextRunRemainSpace -= semanticBound.Max()[0];
                    }
                    else
                    {
                        // There is not enough tab textrun space, so the total semantic length is
                        // added with the semanticLength of the textrun subtract with the remain
                        // space of the tab textrun. Then mark current tab textrun finished.
                        outSet._textLineSemanticLength +=
                            semanticBound.Max()[0] - outSet._tabTextRunRemainSpace;
                        outSet._tabTextRunRemainSpace = 0.0f;
                        returnStatus = CommonTextAccumulateStatus::CommonTextAccumulateStatusFinishTabTextRun;
                    }
                    break;
                case UsdImagingTabStopType::UsdImagingTabStopTypeCenter:
                    // For center tabstop, The left half of the text is inserted inside the tab
                    // textrun space until the tab textrun space is full.
                    if (inSet._tabTextRunRemainSpace >= semanticBound.Max()[0] / 2)
                    {
                        // There is still enough tab textrun space, so subtract half of the semantic
                        // length of the textrun from the remain space. The total semantic length
                        // is added with the other half of the semantic length.
                        outSet._tabTextRunRemainSpace -= semanticBound.Max()[0] / 2;
                        outSet._textLineSemanticLength += semanticBound.Max()[0] / 2;
                    }
                    else
                    {
                        // There is not enough tab textrun space, so the total semantic length is
                        // added with the semanticLength of the textrun subtract with the remain
                        // space of the tab textrun. Then mark current tab textrun finished.
                        outSet._textLineSemanticLength +=
                            semanticBound.Max()[0] - outSet._tabTextRunRemainSpace;
                        outSet._tabTextRunRemainSpace = 0.0f;
                        returnStatus = CommonTextAccumulateStatus::CommonTextAccumulateStatusFinishTabTextRun;
                    }
                    break;
                case UsdImagingTabStopType::UsdImagingTabStopTypeDecimal:
                {
                    // Specially, if the first character is decimal point, it means there is no integer
                    // part. In this case, the tab is handled like a left tab. We will extend the textLineSemanticLength
                    // by the semantic length of the current textrun, and finish the current decimal tab textrun.
                    if (_markupText->MarkupString()[textRunIter->StartIndex()] == '.')
                    {
                        outSet._textLineSemanticLength += semanticBound.Max()[0];
                        returnStatus = CommonTextAccumulateStatus::CommonTextAccumulateStatusFinishTabTextRun;
                    }
                    // By default, the decimal tab is handled like a right tab.
                    else if (inSet._tabTextRunRemainSpace >= semanticBound.Max()[0])
                    {
                        // There is still enough tab textrun space, so simply subtract the semantic
                        // length of the textrun from the remain space. The total semantic length
                        // is not increased.
                        outSet._tabTextRunRemainSpace -= semanticBound.Max()[0];
                        int pointPos = textRunIter->StartIndex() + textRunIter->Length();
                        if (pointPos < _markupText->MarkupString().size() && _markupText->MarkupString()[pointPos] == '.')
                        {
                            // As we has met decimal point, the current decimal tabstop could be finished.
                            returnStatus = CommonTextAccumulateStatus::CommonTextAccumulateStatusFinishTabTextRun;
                        }
                    }
                    else
                    {
                        // There is not enough tab textrun space, so the total semantic length is
                        // added with the semanticLength of the textrun subtract with the remain
                        // space of the tab textrun. Then mark current tab textrun finished.
                        outSet._textLineSemanticLength +=
                            semanticBound.Max()[0] - outSet._tabTextRunRemainSpace;
                        outSet._tabTextRunRemainSpace = 0.0f;
                        returnStatus = CommonTextAccumulateStatus::CommonTextAccumulateStatusFinishTabTextRun;
                    }
                    break;
                }
                default:
                    return CommonTextAccumulateStatus::CommonTextAccumulateStatusFail;
                };
            }
            else
                // No tab textrun space or the tabstop is left, in that case, we simply add the
                // semantic length of the textrun to the total semantic length.
                outSet._textLineSemanticLength += semanticBound.Max()[0];
        }
        if (!extentBound.IsEmpty())
        {
            if (semanticBound.IsEmpty())
            {
                // If semanticBound is empty, we simply add the max of extent bound to the total
                // semantic bound, and get the extent length with this textrun.
                outSet._textLineExtentLength =
                    outSet._textLineSemanticLength + extentBound.Max()[0];
            }
            else
            {
                // If semanticBound is not empty, we simply add the max of extent bound to the total
                // semantic bound, and get the extent length with this textrun.
                outSet._textLineExtentLength =
                    outSet._textLineSemanticLength + extentBound.Max()[0] - semanticBound.Max()[0];
            }
        }
        if (!semanticBound.IsEmpty())
        {
            outSet._ascent =
                (semanticBound.Max()[1] > outSet._ascent) ? semanticBound.Max()[1] : outSet._ascent;
            outSet._descent = (semanticBound.Min()[1] < outSet._descent) ? semanticBound.Min()[1]
                                                                         : outSet._descent;
        }
        break;
    }
    case UsdImagingTextRunType::UsdImagingTextRunTypeTab:
    {
        // Find the tab stop in the tab stop list.
        float currentTabPosition = -1.0f;
        if (_paragraphStyle)
        {
            TabStopArray tabStopArray = _paragraphStyle->_tabStopList;
            for (const auto& tabStop : tabStopArray)
            {
                if (tabStop._position > outSet._textLineSemanticLength)
                {
                    outSet._tabStopType = tabStop._type;
                    currentTabPosition  = tabStop._position;
                    break;
                }
            }
        }
        // If the tabstop position is not found, the tab position is defined from global setting.
        if (currentTabPosition < 0.0f)
        {
            const CommonTextGlobalSetting& textSetting = CommonTextSystem::Instance()->GetTextGlobalSetting();
            int defaultTabSize                   = textSetting.TabSize();
            outSet._tabStopType                  = UsdImagingTabStopType::UsdImagingTabStopTypeLeft;
            currentTabPosition =
                (float)((int)(outSet._textLineSemanticLength / defaultTabSize) + 1) *
                defaultTabSize;
        }
        // Set the total semantic length at the current tabstop position.
        outSet._textLineSemanticLength = currentTabPosition;
        // Set the remain space of the tab textrun.
        outSet._tabTextRunRemainSpace = currentTabPosition - inSet._textLineSemanticLength;
        returnStatus                  = CommonTextAccumulateStatus::CommonTextAccumulateStatusTabTextRun;
        break;
    }
    default:
        break;
    }
    return returnStatus;
}

CommonTextStatus 
CommonTextLineLayoutManager::BreakLine(
    const CommonTextBreakInfo& wordBreakInfo, 
    UsdImagingTextLine& newTextLine, 
    CommonTextLineLayout& newLineLayout)
{
    if (!_markupText)
        return CommonTextStatus::CommonTextStatusFail;

    std::shared_ptr<UsdImagingTextRunList> textRunList = _markupText->ListOfTextRuns();
    UsdImagingTextRun& breakRun                        = *(wordBreakInfo._breakRunIter);
    CommonTextRunInfo& textRunInfo = _intermediateInfo->GetTextRunInfo(wordBreakInfo._breakRunIter);
    UsdImagingTextRunList::iterator newLineStart;
    CommonTextRunLayoutList::iterator newLineLayoutStart;
    if (breakRun.Length() == wordBreakInfo._breakIndexInTextRun)
    {
        // If the break position is after the last character of the BreakRun, we directly create
        // the new line from the next text run.
        newLineStart = wordBreakInfo._breakRunIter;
        newLineStart++;
        newLineLayoutStart = wordBreakInfo._breakRunLayoutIter;
        newLineLayoutStart++;
    }
    else
    {
        // Create new textrun from the breakRun, and shorten the breakRun to the break position.
        UsdImagingTextRun newRun;
        newRun.CopyPartOfRun(breakRun, wordBreakInfo._breakIndexInTextRun + 1,
            breakRun.Length() - wordBreakInfo._breakIndexInTextRun - 1);
        CommonTextRunInfo newRunInfo;
        newRunInfo.CopyPartOfData(textRunInfo, wordBreakInfo._breakIndexInTextRun + 1,
            breakRun.Length() - wordBreakInfo._breakIndexInTextRun - 1);

        breakRun.Shorten(wordBreakInfo._breakIndexInTextRun + 1);
        textRunInfo.Shorten(wordBreakInfo._breakIndexInTextRun + 1);
        // Insert the new textrun after the breakRun.
        newLineStart = CommonTextTrueTypeGenericLayoutManager::AddTextRun(
            _markupText, _intermediateInfo, wordBreakInfo._breakRunIter, newRun, newRunInfo);

        CommonTextRunLayout newLayout;
        CommonTextRunLayout& textRunLayout = *wordBreakInfo._breakRunLayoutIter;
        textRunLayout.SimpleLayout().Reset();
        UsdImagingTextStyle textStyle = breakRun.GetStyle(_defaultTextStyle);
        CommonTextTrueTypeSimpleLayoutManager simpleManager =
            CommonTextSystem::Instance()->GetSimpleLayoutManager(textStyle);
        if (simpleManager.IsValid())
        {
            // Generate the layout for the breakRun
            std::wstring characters(
                _markupText->MarkupString(), breakRun.StartIndex(), breakRun.Length());
            CommonTextStatus status = simpleManager.GenerateSimpleLayout(
                characters, textRunLayout.SimpleLayout(), textRunInfo.ComplexScriptInformation());
            if (status != CommonTextStatus::CommonTextStatusSuccess)
                return status;

            // Generate the layout for the new textRun.
            std::wstring newCharacters(
                _markupText->MarkupString(), newRun.StartIndex(), newRun.Length());
            status = simpleManager.GenerateSimpleLayout(
                newCharacters, newLayout.SimpleLayout(), newRunInfo.ComplexScriptInformation());
            if (status != CommonTextStatus::CommonTextStatusSuccess)
                return status;
        }
        // Insert the newLayout after the breakRunLayout.
        newLineLayoutStart = _genericLayout->ListOfTextRunLayouts().insert_after(
            wordBreakInfo._breakRunLayoutIter, std::move(newLayout));
    }

    // Change the range for the break line. And create the new textline.
    newTextLine = *_lineIter;
    if (_lineIter->Range()._lastRun == wordBreakInfo._breakRunIter)
    {
        UsdImagingTextRunRange range = { newLineStart, newLineStart, false };
        newTextLine.Range(range);
    }
    else
    {
        UsdImagingTextRunRange range = { newLineStart, _lineIter->Range()._lastRun, false };
        newTextLine.Range(range);
    }
    newTextLine.StartBreak(UsdImagingTextLineBreak::UsdImagingTextLineBreakWrapBreak);
    newTextLine.ParagraphStart(false);
    UsdImagingTextRunRange range = { _lineIter->Range()._firstRun, wordBreakInfo._breakRunIter, false };;
    _lineIter->Range(range);
    _lineIter->EndBreak(UsdImagingTextLineBreak::UsdImagingTextLineBreakWrapBreak);
    _lineIter->ParagraphEnd(false);

    // Set the line layout.
    auto breakRunIter                     = wordBreakInfo._breakRunLayoutIter;
    newLineLayout.Range()._firstRunLayout = newLineLayoutStart;
    if (_lineLayoutIter->Range()._lastRunLayout == wordBreakInfo._breakRunLayoutIter)
        newLineLayout.Range()._lastRunLayout = newLineLayoutStart;
    else
        newLineLayout.Range()._lastRunLayout = _lineLayoutIter->Range()._lastRunLayout;
    _lineLayoutIter->Range()._lastRunLayout = wordBreakInfo._breakRunLayoutIter;

    auto start = _lineLayoutIter->Range()._firstRunLayout;
    auto end   = _lineLayoutIter->Range()._lastRunLayout;
    end++;

    // Handle the word break.
    std::vector<int>& breakIndexArray = wordBreakInfo._breakRunWordBreakIter->_breakIndexInTextRun;
    auto breakIndexIter = std::find(breakIndexArray.begin(), breakIndexArray.end(), wordBreakInfo._breakIndexInTextRun);
    if (breakIndexIter != breakIndexArray.end())
    {
        // First, for the breakIndexArray of the break run, resize it so that we only keep the word
        // break index before the break position.
        breakIndexArray.erase(breakIndexIter, breakIndexArray.end());
        // Then in the wordBreakList, remove the breakIndexArray after the break run.
        WordBreakIndexList& wordBreakList = _intermediateInfo->GetWordBreakIndexList(_lineIter);
        WordBreakIndexList::iterator nextBreakIter = wordBreakInfo._breakRunWordBreakIter;
        ++nextBreakIter;
        wordBreakList.resize(std::distance(wordBreakList.begin(), nextBreakIter));
    }
    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextStatus 
CommonTextLineLayoutManager::RepositionTextRuns(float lineExtentLength)
{
    if (_lineIter->LineType() == UsdImagingTextLineType::UsdImagingTextLineTypeZero)
        return CommonTextStatus::CommonTextStatusSuccess;

    // Handle paragraph alignment.
    // The space before the line.
    float leftSpace = 0.0f;
    // The space added for each white space for justify alignment.
    float justifySpace = 0.0f;
    if (_paragraphStyle)
    {
        // The remain space after the line.
        float remainSpace =
            _constraintInBaseline - _paragraphStyle->_rightIndent - lineExtentLength;
        // If the alignment is not left, we need to put some space before the line.
        switch (_paragraphStyle->_alignment)
        {
        case UsdImagingParagraphAlignment::UsdImagingParagraphAlignmentCenter:
            // For center align, put half of the remain space at the left.
            leftSpace = remainSpace / 2.0f;
            break;
        case UsdImagingParagraphAlignment::UsdImagingParagraphAlignmentRight:
            // For right align, put all the remain space at the left.
            leftSpace = remainSpace;
            break;
        case UsdImagingParagraphAlignment::UsdImagingParagraphAlignmentJustify:
        case UsdImagingParagraphAlignment::UsdImagingParagraphAlignmentDistribute:
        {
            // Justify will not apply to end line of a paragraph. But distribute will apply to
            // the end line.
            if (!_lineIter->ParagraphEnd() || _paragraphStyle->_alignment ==
                UsdImagingParagraphAlignment::UsdImagingParagraphAlignmentDistribute)
            {
                WordBreakIndexList& wordBreakList = _intermediateInfo->GetWordBreakIndexList(_lineIter);
                // Count of white spaces that we will add justified space.
                int justifiedWhiteSpaceCount = 0;
                bool theFirstJustifiedTextRun = true;
                UsdImagingTextRunList::iterator runIter = _lineIter->Range()._firstRun;
                UsdImagingTextRunList::iterator lastIter = _lineIter->Range()._lastRun;
                WordBreakIndexList::iterator wordBreakIter = wordBreakList.begin();
                const std::wstring& markupString = _markupText->MarkupString();
                for (; runIter != lastIter; ++runIter)
                {
                    const wchar_t* runStringStart = &markupString[runIter->StartIndex()];
                    // Only the white spaces after the last tab in the line will be justified.
                    // So if we meet with a tab textrun, we will reset the justifiedWhiteSpaceCount.
                    if (runIter->Type() == UsdImagingTextRunType::UsdImagingTextRunTypeTab)
                    {
                        theFirstJustifiedTextRun = true;
                        justifiedWhiteSpaceCount = 0;
                        ++wordBreakIter;
                        continue;
                    }
                    else if (theFirstJustifiedTextRun)
                    {
                        // For the first justified text run, all the white spaces at the start
                        // of the textrun will not be justified.
                        std::vector<int>& wordBreakArray = wordBreakIter->_breakIndexInTextRun;
                        size_t count = wordBreakArray.size();
                        size_t i = 0;
                        for (; i < count; ++i)
                        {
                            // Find the first word break that is not at the start or not white space.
                            if (wordBreakArray[i] != i || 
                                runStringStart[wordBreakArray[i]] != L' ')
                                break;
                        }
                        // If we don't reach the last word break, it means the following white
                        // spaces will be justified.
                        if (i != count)
                        {
                            // All the following white spces will be justified.
                            for (; i < count; ++i)
                            {
                                // Check if the word break is a white space.
                                if (runStringStart[wordBreakArray[i]] == L' ')
                                {
                                    wordBreakIter->_addJustifyIndexInTextRun.push_back(wordBreakArray[i]);
                                    ++justifiedWhiteSpaceCount;
                                }
                            }
                            theFirstJustifiedTextRun = false;
                        }
                        // If word break count is the same as the length of the text run, it means
                        else if (count != runIter->Length())
                        {
                            theFirstJustifiedTextRun = false;
                        }
                    }
                    else
                    {
                        std::vector<int>& wordBreakArray = wordBreakIter->_breakIndexInTextRun;
                        size_t count = wordBreakArray.size();
                        size_t i = 0;
                        for (; i < count; ++i)
                        {
                            // Check if the word break is a white space.
                            if (runStringStart[wordBreakArray[i]] == L' ')
                            {
                                wordBreakIter->_addJustifyIndexInTextRun.push_back(wordBreakArray[i]);
                                ++justifiedWhiteSpaceCount;
                            }
                        }
                    }
                    ++wordBreakIter;
                }
                // Handle the last textrun
                // Only the white spaces after the last tab in the line will be justified.
                // So if the last textrun is a tab textrun, no white space will be justified.
                const wchar_t* runStringStart = &markupString[lastIter->StartIndex()];
                if (lastIter->Type() == UsdImagingTextRunType::UsdImagingTextRunTypeTab)
                {
                    justifiedWhiteSpaceCount = 0;
                }
                else if (theFirstJustifiedTextRun)
                {
                    // For the first justified text run, all the white spaces at the start
                    // of the textrun will not be justified.
                    std::vector<int>& wordBreakArray = wordBreakIter->_breakIndexInTextRun;
                    size_t count = wordBreakArray.size();
                    size_t i = 0;
                    for (; i < count; ++i)
                    {
                        // Find the first word break that is not at the start or not white space.
                        if (wordBreakArray[i] != i ||
                            runStringStart[wordBreakArray[i]] != L' ')
                            break;
                    }
                    // For the last justified text run, all the white spaces at the end
                    // of the textrun will not be justified.
                    size_t j = 0;
                    for (; j < count; ++j)
                    {
                        // Find the last word break that is not at the end or not white space.
                        if (wordBreakArray[count - j - 1] != lastIter->Length() - j - 1 ||
                            runStringStart[wordBreakArray[count - j - 1]] != L' ')
                            break;
                    }
                    if (i + j < count)
                    {
                        for (; i < count - j; ++i)
                        {
                            // Check if the word break is a white space.
                            if (runStringStart[wordBreakArray[i]] == L' ')
                            {
                                wordBreakIter->_addJustifyIndexInTextRun.push_back(wordBreakArray[i]);
                                ++justifiedWhiteSpaceCount;
                            }
                        }
                    }
                }
                else
                {
                    // For the last justified text run, all the white spaces at the end
                    // of the textrun will not be justified.
                    std::vector<int>& wordBreakArray = wordBreakIter->_breakIndexInTextRun;
                    size_t count = wordBreakArray.size();
                    size_t j = 0;
                    for (; j < count; ++j)
                    {
                        // Find the last word break that is not at the end or not white space.
                        if (wordBreakArray[count - j - 1] != lastIter->Length() - j - 1 ||
                            runStringStart[wordBreakArray[count - j - 1]] != L' ')
                            break;
                    }
                    if (j < count)
                    {
                        for (size_t i = 0; i < count - j; ++i)
                        {
                            // Check if the word break is a white space.
                            if (runStringStart[wordBreakArray[i]] == L' ')
                            {
                                wordBreakIter->_addJustifyIndexInTextRun.push_back(wordBreakArray[i]);
                                ++justifiedWhiteSpaceCount;
                            }
                        }
                    }
                }
                if(justifiedWhiteSpaceCount > 0)
                    justifySpace = remainSpace / justifiedWhiteSpaceCount;
            }
            break;
        }
        default:
            break;
        }
    }

    auto start = _lineLayoutIter->Range()._firstRunLayout;
    auto end   = _lineLayoutIter->Range()._lastRunLayout;
    end++;
    WordBreakIndexList::iterator wordBreakIter;
    if (justifySpace > 0.0f)
    {
        WordBreakIndexList& wordBreakList = _intermediateInfo->GetWordBreakIndexList(_lineIter);
        wordBreakIter = wordBreakList.begin();
    }
    // Get the iterator to the textrun positions.
    std::vector<std::pair<float, float>>& textRunPositions = _lineLayoutIter->ArrayOfTextRunPositions();
    float semanticLength                                   = leftSpace;
    for (auto iter = start; iter != end; ++iter)
    {
        // Set the position of the textrun to the current accumulated semantic length of the line.
        textRunPositions.push_back(std::pair<float, float>(semanticLength, 0.0f));
        if (justifySpace > 0.0f)
        {
            if (!wordBreakIter->_addJustifyIndexInTextRun.empty())
            {
                // The justified space added to each white space.
                float addJustifySpace = 0.0f;
                std::vector<int> addJustifyIndexArray;
                // For complex script, we need to first map the justified index to the glyph index.
                std::shared_ptr<CommonTextComplexScriptMetrics> complexScriptMetrics = iter->SimpleLayout().GetComplexScriptMetrics();
                if (complexScriptMetrics)
                {
                    for (auto justifyIndex : wordBreakIter->_addJustifyIndexInTextRun)
                    {
                        addJustifyIndexArray.push_back(complexScriptMetrics->CharacterToGlyphMap()[justifyIndex]);
                    }
                }
                else
                {
                    addJustifyIndexArray = wordBreakIter->_addJustifyIndexInTextRun;
                }
                int lastJustifyIndex = -1;
                for (int justifyIndex : addJustifyIndexArray)
                {
                    // For each glyph before the lastJustifyIndex and the current index, add justified space to
                    // its position.
                    if (lastJustifyIndex != -1)
                    {
                        for (int i = lastJustifyIndex + 1; i < justifyIndex; ++i)
                        {
                            CommonTextCharMetrics& metrics = iter->SimpleLayout().CharacterMetrics(i);
                            metrics._startPosition += addJustifySpace;
                            metrics._endPosition += addJustifySpace;
                        }
                    }
                    // For the white space in the current index, the space will be enlarged by one justifySpace.
                    CommonTextCharMetrics& metrics = iter->SimpleLayout().CharacterMetrics(justifyIndex);
                    metrics._startPosition += addJustifySpace;
                    addJustifySpace += justifySpace;
                    metrics._endPosition += addJustifySpace;
                    lastJustifyIndex = justifyIndex;
                }
                // Handle the glyphs after the last justified white space.
                for (int i = lastJustifyIndex + 1; i < iter->SimpleLayout().CountOfRenderableChars(); ++i)
                {
                    CommonTextCharMetrics& metrics = iter->SimpleLayout().CharacterMetrics(i);
                    metrics._startPosition += addJustifySpace;
                    metrics._endPosition += addJustifySpace;
                }
                // Handle the semantic bound and extent bound.
                CommonTextBox2<GfVec2f>& semanticBound = iter->SimpleLayout().FullMetrics()._semanticBound;
                semanticBound.Max(GfVec2f(semanticBound.Max()[0] + addJustifySpace, semanticBound.Max()[1]));
                CommonTextBox2<GfVec2f>& extentBound = iter->SimpleLayout().FullMetrics()._extentBound;
                extentBound.Max(GfVec2f(extentBound.Max()[0] + addJustifySpace, extentBound.Max()[1]));
            }
            ++wordBreakIter;
        }
        CommonTextBox2<GfVec2f>& semanticBound = iter->SimpleLayout().FullMetrics()._semanticBound;
        if (!semanticBound.IsEmpty())
            semanticLength += semanticBound.Max()[0];
    }

    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextStatus 
CommonTextLineLayoutManager::GenerateDecorations(const UsdImagingTextColor& defaultColor)
{
    if (_lineIter->LineType() == UsdImagingTextLineType::UsdImagingTextLineTypeZero)
        return CommonTextStatus::CommonTextStatusSuccess;

    CommonTextDecorationLayout currentUnderlineDecoration(UsdImagingTextProperty::UsdImagingTextPropertyUnderlineType);
    CommonTextDecorationLayout currentOverlineDecoration(UsdImagingTextProperty::UsdImagingTextPropertyOverlineType);
    CommonTextDecorationLayout currentStrikethroughDecoration(UsdImagingTextProperty::UsdImagingTextPropertyStrikethroughType);
    GfVec2f decorationYRange;

    auto start = _lineIter->Range()._firstRun;
    auto end   = _lineIter->Range()._lastRun;
    end++;

    auto currentLayoutIter = _lineLayoutIter->Range()._firstRunLayout;
    auto layoutEnd         = _lineLayoutIter->Range()._lastRunLayout;
    layoutEnd++;

    std::vector<std::pair<float, float>>& textRunPositions = _lineLayoutIter->ArrayOfTextRunPositions();
    auto textRunPositionIter                               = textRunPositions.begin();

    std::vector<CommonTextDecorationLayout>& decorationLayouts = _lineLayoutIter->Decorations();

    // Iterate all textLayouts in the text line.
    // One decoration is composed of some contiguous line sections, and the section data is
    // calculated by textLayout.
    for (auto iter = start; iter != end; iter++)
    {
        const UsdImagingTextRun& run                 = *iter;
        const CommonTextRunLayout& currentLayout = *currentLayoutIter;
        const UsdImagingTextStyle& textStyle         = run.GetStyle(_defaultTextStyle);
        std::pair<float, float>& position  = *textRunPositionIter;

        // Add lambda expression for updating current decoration.
        auto updateDecoration = [&](CommonTextDecorationLayout& currentDecoration, UsdImagingTextProperty lineProperty) {
            bool hasLine                         = false;
            TfToken textStyleLineType = UsdImagingTextTokens->none;
            if (lineProperty == UsdImagingTextProperty::UsdImagingTextPropertyUnderlineType)
            {
                hasLine           = textStyle.HasUnderline();
                textStyleLineType = textStyle._underlineType;
            }
            else if (lineProperty == UsdImagingTextProperty::UsdImagingTextPropertyOverlineType)
            {
                hasLine           = textStyle.HasOverline();
                textStyleLineType = textStyle._overlineType;
            }
            else if (lineProperty == UsdImagingTextProperty::UsdImagingTextPropertyStrikethroughType)
            {
                hasLine           = textStyle.HasStrikethrough();
                textStyleLineType = textStyle._strikethroughType;
            }

            // Run textStyle:: Normal
            if (hasLine)
            {
                GfVec2f semanticBoundMin =
                    currentLayout.SimpleLayout().FullMetrics()._semanticBound.Min();
                GfVec2f semanticBoundMax =
                    currentLayout.SimpleLayout().FullMetrics()._semanticBound.Max();

                // Add lambda expression for adding new section.
                auto addSection = [&] {
                    CommonTextDecorationLayout::CommonTextSection newSection;
                    newSection._lineColor = run.GetTextColor(defaultColor);
                    float endXPosition    = 0.0f;
                    endXPosition          = semanticBoundMax[0] + position.first;

                    newSection._endXPosition = endXPosition;
                    currentDecoration._sections.push_back(newSection);
                };

                // Decoration is None or type is changed. Is a new decoration, add new section.
                if (!currentDecoration.IsValid() || (currentDecoration._type != textStyleLineType))
                {
                    // If decoration type changes, end the current decoration.
                    if (currentDecoration.IsValid())
                    {
                        // If the decoration type is doubleLines, we need to create two decorations.
                        if (currentDecoration._type == UsdImagingTextTokens->doubleLines)
                        {
                            // If the decoration property is strike through, we will modify the y position of the decoration to
                            // _doubleLinesStrikethroughFirst and _doubleLinesStrikethroughSecond.
                            if (currentDecoration._decoration == UsdImagingTextProperty::UsdImagingTextPropertyStrikethroughType)
                            {
                                currentDecoration._yPosition = (decorationYRange[0] + decorationYRange[1]) * _doubleLinesStrikethroughFirst;
                                decorationLayouts.emplace_back(currentDecoration);

                                currentDecoration._yPosition = (decorationYRange[0] + decorationYRange[1]) * _doubleLinesStrikethroughSecond;
                                decorationLayouts.emplace_back(std::move(currentDecoration));
                            }
                        }
                        else
                            decorationLayouts.emplace_back(std::move(currentDecoration));
                    }
                    currentDecoration._type = textStyleLineType;
                    currentDecoration._startXPosition = semanticBoundMin[0] + position.first;
                    addSection();

                    // Set y position of decoration.
                    if (lineProperty == UsdImagingTextProperty::UsdImagingTextPropertyUnderlineType)
                    {
                        currentDecoration._yPosition = semanticBoundMin[1];
                    }
                    else if (lineProperty == UsdImagingTextProperty::UsdImagingTextPropertyOverlineType)
                    {
                        currentDecoration._yPosition = semanticBoundMax[1];
                    }
                    else if (lineProperty == UsdImagingTextProperty::UsdImagingTextPropertyStrikethroughType)
                    {
                        currentDecoration._yPosition = (semanticBoundMin[1] + semanticBoundMax[1]) / 2;
                    }
                    decorationYRange = GfVec2f(semanticBoundMin[1], semanticBoundMax[1]);
                }
                else
                {
                    // Same color. Extend the current decoration.
                    if (run.GetTextColor(defaultColor) == currentDecoration._sections.back()._lineColor)
                    {
                        currentDecoration._sections.back()._endXPosition =
                            semanticBoundMax[0] + position.first;
                    }
                    // Different color. Add new Section.
                    else
                        addSection();
                    if (currentDecoration._type != textStyleLineType)
                    {
                        currentDecoration._type = textStyleLineType;
                        currentDecoration._startXPosition = semanticBoundMin[0] + position.first;
                    }

                    if (decorationYRange[0] > semanticBoundMin[1])
                        decorationYRange[0] = semanticBoundMin[1];
                    if (decorationYRange[1] < semanticBoundMax[1])
                        decorationYRange[1] = semanticBoundMax[1];

                    // Update y position of decoration.
                    if (lineProperty == UsdImagingTextProperty::UsdImagingTextPropertyUnderlineType)
                    {
                        currentDecoration._yPosition = decorationYRange[0];
                    }
                    else if (lineProperty == UsdImagingTextProperty::UsdImagingTextPropertyOverlineType)
                    {
                        currentDecoration._yPosition = decorationYRange[1];
                    }
                    else if (lineProperty == UsdImagingTextProperty::UsdImagingTextPropertyStrikethroughType)
                    {
                        currentDecoration._yPosition = (decorationYRange[0] + decorationYRange[1]) / 2;
                    }

                }
            }
            // Run textStyle: None, Decoration: Normal.
            // End the current decoration, and add it to decorationLayouts.
            else if (currentDecoration.IsValid())
            {
                decorationLayouts.emplace_back(std::move(currentDecoration));
            }
        };

        // Update underline decoration.
        updateDecoration(currentUnderlineDecoration, UsdImagingTextProperty::UsdImagingTextPropertyUnderlineType);
        // Update overline decoration.
        updateDecoration(currentOverlineDecoration, UsdImagingTextProperty::UsdImagingTextPropertyOverlineType);
        // Update strike through decoration.
        updateDecoration(currentStrikethroughDecoration, UsdImagingTextProperty::UsdImagingTextPropertyStrikethroughType);

        currentLayoutIter++;
        textRunPositionIter++;
    }

    // The current underline computation is complete and it's not empty, so add it to
    // decorationLayouts.
    if (currentUnderlineDecoration.IsValid())
    {
        decorationLayouts.emplace_back(std::move(currentUnderlineDecoration));
    }
    // The current overline computation is complete and it's not empty, so add it to
    // decorationLayouts.
    if (currentOverlineDecoration.IsValid())
    {
        decorationLayouts.emplace_back(std::move(currentOverlineDecoration));
    }
    // The current strike through computation is complete and it's not empty, so add it to
    // decorationLayouts.
    if (currentStrikethroughDecoration.IsValid())
    {
        decorationLayouts.emplace_back(std::move(currentStrikethroughDecoration));
    }
    return CommonTextStatus::CommonTextStatusSuccess;
}

PXR_NAMESPACE_CLOSE_SCOPE
