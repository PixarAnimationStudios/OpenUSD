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
#include "lineLayoutManager.h"
#include "multiLanguageHandler.h"
#include "simpleLayout.h"
#include "system.h"
#include "utilities.h"

PXR_NAMESPACE_OPEN_SCOPE
void _CalculateLineHeight(const UsdImagingTextParagraphStyle* paragraphStyle,
                          float ascent, 
                          float descent, 
                          float& topSpace,
                          float& bottomSpace, 
                          float& lineHeight);

void _AccumulateLine(const UsdImagingTextLine& line, 
                     const UsdImagingTextBlock& currentColumn,
                     const UsdImagingTextParagraphStyle* currentParagraphStyle, 
                     float topSpace, 
                     float lineTotalSpace,
                     std::vector<std::pair<float, float>>& linePositions, 
                     float& flowAccumulation);

CommonTextStatus 
CommonTextTrueTypeGenericLayoutManager::_GenerateLayoutFromLayout()
{
    // TODO: regenerate the layout.
    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextStatus 
CommonTextTrueTypeGenericLayoutManager::_GenerateLayoutFromPreLayout()
{
    const UsdImagingTextStyle globalTextStyle                          = _markupText->GlobalTextStyle();
    const std::shared_ptr<UsdImagingTextRunList> textRunList               = _markupText->ListOfTextRuns();
    const std::shared_ptr<UsdImagingTextLineList> textLineList             = _markupText->ListOfTextLines();
    const std::shared_ptr<UsdImagingTextParagraphArray> textParagraphArray = _markupText->TextParagraphArray();
    const std::shared_ptr<UsdImagingTextBlockArray> textBlockArray         = _markupText->TextBlockArray();

    if (textRunList->empty())
    {
        return CommonTextStatus::CommonTextStatusInvalidArg;
    }
    else if (textLineList->empty() || textBlockArray->empty())
        return CommonTextStatus::CommonTextStatusInvalidArg;

    // First set the block positions and initialize layout for all blocks.
    std::vector<std::pair<float, float>>& blockPositions  = _genericLayout->ArrayOfBlockPositions();
    std::vector<CommonTextBlockLayout>& blockLayoutsArray = _genericLayout->ArrayBlockLayouts();
    for (const auto& block : *textBlockArray)
    {
        std::pair<float, float> blockPosition;
        blockPosition.first  = block.Offset()[0];
        blockPosition.second = block.Offset()[1];
        blockPositions.push_back(std::move(blockPosition));
        CommonTextBlockLayout blockLayout;
        blockLayoutsArray.push_back(std::move(blockLayout));
    }

    CommonTextLineLayoutList& listOfTextLineLayouts = _genericLayout->ListOfTextLineLayouts();
    CommonTextRunLayoutList& listOfTextRunLayouts = _genericLayout->ListOfTextRunLayouts();

    // Initialize the line layout for each line.
    CommonTextStatus status =
        _InitializeLineLayouts(textRunList, listOfTextRunLayouts, textLineList, listOfTextLineLayouts);
    if (status != CommonTextStatus::CommonTextStatusSuccess)
        return status;

    CommonTextLineLayoutManager lineManager;
    UsdImagingTextStyle currentStyle = globalTextStyle;

    // The current line.
    auto currentLineIter       = textLineList->begin();
    auto currentLineLayoutIter = listOfTextLineLayouts.begin();

    // The current block.
    int currentBlockIndex = 0;
    if (currentBlockIndex == textBlockArray->size())
        return CommonTextStatus::CommonTextStatusInvalidArg;

    // Check if the firstLineIter is valid.
    UsdImagingTextBlock& firstBlock = textBlockArray->at(currentBlockIndex);
    assert(currentLineIter == firstBlock.FirstLineIter());
    // Set the FirstLineLayoutIter for the first block layout.
    CommonTextBlockLayout& firstBlockLayout = blockLayoutsArray.at(currentBlockIndex);
    firstBlockLayout.FirstLineLayoutIter(currentLineLayoutIter);

    float widthConstraint  = firstBlock.WidthConstraint();
    float heightConstraint = firstBlock.HeightConstraint();
    std::vector<std::pair<float, float>> arrayOfLinePositions;
    arrayOfLinePositions.clear();

    // The accumulation of height of lines.
    float flowAccumulation = 0.0f;

    // The current paragraph.
    UsdImagingTextParagraphArray::iterator paragraphIter;
    const UsdImagingTextParagraphStyle* currentParagraphStyle = nullptr;
    if (textParagraphArray)
    {
        paragraphIter = textParagraphArray->begin();
        if (paragraphIter != textParagraphArray->end())
            currentParagraphStyle = &(paragraphIter->Style());
    }

    // The lambda expression for setting the lastLineIter and set the layout of current column. Each time when
    // a column reaches the end, we need to call this expression.
    auto finishCurrentColumn = [&] (UsdImagingTextLineList::iterator lineIter, CommonTextLineLayoutList::iterator lineLayoutIter) {
        // Set the lastLineIter.
        UsdImagingTextBlock& currentBlock = textBlockArray->at(currentBlockIndex); 
        currentBlock.LastLineIter(lineIter);
        // Set the lastLineLayoutIter.
        CommonTextBlockLayout& currentBlockLayout = blockLayoutsArray.at(currentBlockIndex);
        currentBlockLayout.LastLineLayoutIter(lineLayoutIter);

        // Move the line positions in the y direction according to the alignment of the column.
        // If the line already flows out of the column, we will not reset the line position.
        if (currentBlock.Alignment() != UsdImagingBlockAlignment::UsdImagingBlockAlignmentTop &&
            currentBlock.HeightConstraint() >= flowAccumulation)
        {
            float heightRemainSpace = currentBlock.HeightConstraint() - flowAccumulation;
            float alignmentOffset = (currentBlock.Alignment() == UsdImagingBlockAlignment::UsdImagingBlockAlignmentCenter)
                ? heightRemainSpace / 2
                : heightRemainSpace;
            for (auto&& linePosition : arrayOfLinePositions)
            {
                linePosition.second -= alignmentOffset;
            }
        }
        currentBlockLayout.ArrayOfLinePositions() = arrayOfLinePositions;
    };

    // The lambda expression for moving to next column.
    auto moveToNextColumn = [&] (UsdImagingTextLineList::iterator lineIter, CommonTextLineLayoutList::iterator lineLayoutIter) {
        // Go to the next column and set the FirstLineIter.
        ++currentBlockIndex;
        UsdImagingTextBlock& currentBlock = textBlockArray->at(currentBlockIndex);
        currentBlock.FirstLineIter(lineIter);
        // Set the FirstLineLayoutIter for the block layout.
        CommonTextBlockLayout& blockLayout = blockLayoutsArray.at(currentBlockIndex);
        blockLayout.FirstLineLayoutIter(lineLayoutIter);

        // Reset the width constraint and height constraint.
        widthConstraint  = currentBlock.WidthConstraint();
        heightConstraint = currentBlock.HeightConstraint();
        arrayOfLinePositions.clear();

        // Reset flow accumulation.
        flowAccumulation = 0.0f;
    };

    // The lambda expression for setting the lastLineIter of current paragraph. Each time when a
    // paragraph reaches the end, we need to call this expression.
    auto finishCurrentParagraph = [&](UsdImagingTextLineList::iterator lineIter){
        // If currentParagraphStyle is not nullptr, it means there is paragraph definition now.
        if (currentParagraphStyle)
        {
            // Set the LastLineIter.
            paragraphIter->LastLineIter(lineIter);
        }
    };

    // The lambda expression for moving to next paragraph.
    auto moveToNextParagraph = [&](UsdImagingTextLineList::iterator lineIter) {
        paragraphIter++;
        // If current paragraph is the last paragraph, in the next parargraph, there is no
        // paragraphStyle.
        if (paragraphIter != textParagraphArray->end())
        {
            // Set the FirstLineIter.
            paragraphIter->FirstLineIter(lineIter);
            currentParagraphStyle = &(paragraphIter->Style());
        }
        else
            currentParagraphStyle = nullptr;
    };

    // This flag indicate whether the line need to be broke.
    CommonTextLineBreakTestInfo breakTestInfo;
    // Iterate all lines until the end.
    while (currentLineIter != textLineList->end())
    {
        UsdImagingTextBlock& currentBlock = textBlockArray->at(currentBlockIndex);
        UsdImagingTextLine& line          = *currentLineIter;
        if (currentLineLayoutIter == listOfTextLineLayouts.end())
            return CommonTextStatus::CommonTextStatusFail;
        CommonTextLineLayout& lineLayout = *currentLineLayoutIter;
        CommonTextBreakInfo wordBreakInfo;

        // If the accumulation in flow direction already overflow the height constraint of the
        // column, we will go to next column. If this is there is still no line in the column,
        // or if this is the last column, we will still keep in current column.
        if (heightConstraint >= 0.0f && (heightConstraint - flowAccumulation) < 0.0f &&
            currentLineIter != currentBlock.FirstLineIter() &&
            (currentBlockIndex + 1) != textBlockArray->size())
        {
            // The current line will not be added to the current column. So here we set the last
            // line to the lastLineIter of the current column, and set the current line as the 
            // firstLineIter of the next column.
            auto lineIter = currentLineIter;
            --lineIter;
            auto lineLayoutIter = currentLineLayoutIter;
            --lineLayoutIter;
            finishCurrentColumn(lineIter, lineLayoutIter);
            moveToNextColumn(currentLineIter, currentLineLayoutIter);
        }

        // Initialize the line layout manager.
        lineManager.Initialize(_markupText, _genericLayout, _intermediateInfo,
            currentLineIter, currentLineLayoutIter, currentParagraphStyle,
            globalTextStyle, widthConstraint, heightConstraint - flowAccumulation);

        if (line.LineType() == UsdImagingTextLineType::UsdImagingTextLineTypeZero)
        {
            // For zero line, the ascent and descent is simply the ascent and descent of the
            // current text style.
            CommonTextTrueTypeFontDevicePtr fontDevice(currentStyle);
            if (fontDevice.IsValid())
            {
                CommonTextFontMetrics fontMetrics;
                status                     = fontDevice->QueryFontMetrics(fontMetrics);
                breakTestInfo._lineAscent  = static_cast<float>(fontMetrics._ascent);
                breakTestInfo._lineDescent = static_cast<float>(fontMetrics._descent);
            }
            else
                return CommonTextStatus::CommonTextStatusFail;
        }
        else if (line.LineType() == UsdImagingTextLineType::UsdImagingTextLineTypeNormal)
        {
            // Analyze the word breaks for the line.
            lineManager.Analyze(_allowLineBreakInWord, _allowLineBreakBetweenScripts);

            // Try to find the break position in the line if the line is longer than the
            // constraint.
            CommonTextStatus processStatus = lineManager.BreakTest(breakTestInfo, wordBreakInfo);

            if (processStatus != CommonTextStatus::CommonTextStatusSuccess)
                return processStatus;
        }

        // Calculate the total space that the line will occupy in y direction. It includes the top
        // space before the line, the line height, the bottom space after the line, and the
        // paragraph space if this line is the end of a paragraph.
        float topSpace    = 0.0f;
        float bottomSpace = 0.0f;
        float lineHeight  = 0.0f;
        _CalculateLineHeight(currentParagraphStyle, breakTestInfo._lineAscent, breakTestInfo._lineDescent, topSpace,
            bottomSpace, lineHeight);
        float lineTotalSpace = topSpace + lineHeight + bottomSpace;
        // If this is the end of the paragraph, add the paragraph space.
        if (currentParagraphStyle && line.ParagraphEnd() && !breakTestInfo._ifLineBreak)
            lineTotalSpace += currentParagraphStyle->_paragraphSpace;

        // If the total line space overflow the remaining room in the column, and this is not the
        // first line in the column, and this is not the last column, the column is finished, and
        // the line is moved to next column.
        if (lineManager.IsFlowOverflow(lineTotalSpace) &&
            currentLineIter != currentBlock.FirstLineIter() &&
            (currentBlockIndex + 1) != textBlockArray->size())
        {
            // The current line will not be added to the current column. So here we set the last
            // line to the lastLineIter of the current column, and set the current line as the 
            // firstLineIter of the next column.
            auto lineIter = currentLineIter;
            --lineIter;
            auto lineLayoutIter = currentLineLayoutIter;
            --lineLayoutIter;
            finishCurrentColumn(lineIter, lineLayoutIter);
            moveToNextColumn(currentLineIter, currentLineLayoutIter);
        }
        else
        {
            // The height of the line doesn't overflow the remaining room in current block, we
            // will add this line to the block.
            UsdImagingTextLine newTextLine;
            CommonTextLineLayout newLineLayout;
            if (breakTestInfo._ifLineBreak)
            {
                // If we find line break in the line, we will break the line into two lines.
                CommonTextStatus processStatus =
                    lineManager.BreakLine(wordBreakInfo, newTextLine, newLineLayout);
                if (processStatus != CommonTextStatus::CommonTextStatusSuccess)
                    return processStatus;
            }

            // Reposition the textruns
            CommonTextStatus processStatus =
                lineManager.RepositionTextRuns(breakTestInfo._lineExtentLength);
            if (processStatus != CommonTextStatus::CommonTextStatusSuccess)
                return processStatus;

            // Generate decorations.
            processStatus = lineManager.GenerateDecorations(_markupText->DefaultTextColor());
            if (processStatus != CommonTextStatus::CommonTextStatusSuccess)
                return processStatus;

            // Set the ascent and descent of the lines.
            lineLayout.LineAscent(breakTestInfo._lineAscent);
            lineLayout.LineDescent(breakTestInfo._lineDescent);

            // Accumulate the line in the block.
            _AccumulateLine(line, currentBlock, currentParagraphStyle, topSpace, lineTotalSpace,
                arrayOfLinePositions, flowAccumulation);

            // If this line is the end of a paragraph, and currentParagraphStyle is valid, we need
            // to go to the next paragraph.
            if (line.ParagraphEnd() && currentParagraphStyle)
            {
                // Set the current line as the lastLineIter of the current paragraph.
                finishCurrentParagraph(currentLineIter);
                // Set the next line as the firstLineIter of the next paragraph.
                UsdImagingTextLineList::iterator lineIter = currentLineIter;
                ++lineIter;
                moveToNextParagraph(lineIter);
            }

            // Go to next line.
            if (breakTestInfo._ifLineBreak)
            {
                UsdImagingTextLineList::iterator insertPos = currentLineIter;
                ++insertPos;
                // Insert the newline
                currentLineIter = AddTextLine(_markupText, _intermediateInfo, insertPos, newTextLine, WordBreakIndexList());
                if (currentLineIter->EndBreak() == UsdImagingTextLineBreak::UsdImagingTextLineBreakBlockBreak)
                    currentBlock.LastLineIter(currentLineIter);
                CommonTextLineLayoutList::iterator insertLayoutPos = currentLineLayoutIter;
                ++insertLayoutPos;
                currentLineLayoutIter =
                    listOfTextLineLayouts.insert(insertLayoutPos, std::move(newLineLayout));
            }
            else
            {
                bool columnEnd = false;
                if (currentLineIter->EndBreak() == UsdImagingTextLineBreak::UsdImagingTextLineBreakBlockBreak &&
                    (currentBlockIndex + 1) != textBlockArray->size())
                {
                    finishCurrentColumn(currentLineIter, currentLineLayoutIter);
                    columnEnd = true;
                }
                ++currentLineIter;
                ++currentLineLayoutIter;
                if (columnEnd)
                    moveToNextColumn(currentLineIter, currentLineLayoutIter);
            }

            breakTestInfo._ifLineBreak        = false;
            breakTestInfo._lineAscent         = 0.0f;
            breakTestInfo._lineDescent        = 0.0f;
            breakTestInfo._lineSemanticLength = 0.0f;
            breakTestInfo._lineExtentLength   = 0.0f;
        }
    }
    // Finish the last column and paragraph.
    // CurrentLineIter point to the end of the line list. So use the last line as the 
    // lastLineIter of the current column and the current paragraph.
    auto lineIter = currentLineIter;
    --lineIter;
    auto lineLayoutIter = currentLineLayoutIter;
    --lineLayoutIter;
    finishCurrentColumn(lineIter, lineLayoutIter);
    finishCurrentParagraph(lineIter);

    return CommonTextStatus::CommonTextStatusSuccess;
}

void 
_CalculateLineHeight(const UsdImagingTextParagraphStyle* paragraphStyle,
                     float ascent, 
                     float descent, 
                     float& topSpace,
                     float& bottomSpace, 
                     float& lineHeight)
{
    lineHeight = ascent - descent;
    float lineSpace = 0.0f;
    UsdImagingLineSpaceType lineSpaceType = UsdImagingLineSpaceType::UsdImagingLineSpaceTypeAtLeast;
    if (paragraphStyle)
    {
        lineSpace = paragraphStyle->_lineSpace;
        lineSpaceType = paragraphStyle->_lineSpaceType;
    }
    // TODO: need comments for the line space calculation.
    switch (lineSpaceType)
    {
    case UsdImagingLineSpaceType::UsdImagingLineSpaceTypeExactly:
        topSpace    = lineSpace - ascent;
        bottomSpace = lineSpace * 1 / 3;
        break;
    case UsdImagingLineSpaceType::UsdImagingLineSpaceTypeAtLeast:
        if (lineSpace > lineHeight)
        {
            topSpace += lineSpace * 4 / 3 - lineHeight;
            bottomSpace += lineSpace * 1 / 3;
        }
        else
            bottomSpace += lineHeight * 1 / 4;
        break;
    case UsdImagingLineSpaceType::UsdImagingLineSpaceTypeMulti:
        bottomSpace += (lineSpace * 5 / 3 - 1) * lineHeight;
        break;
    default:
        break;
    }
}

// Accumulate a line in a block.
void 
_AccumulateLine(const UsdImagingTextLine& line, 
                const UsdImagingTextBlock& currentColumn,
                const UsdImagingTextParagraphStyle* currentParagraphStyle, 
                float topSpace, 
                float lineTotalSpace,
                std::vector<std::pair<float, float>>& linePositions,
                float& flowAccumulation)
{
    // Set the position of current line.
    float left = 0.0f;
    left += currentColumn.LeftMargin();
    // Add the indent of the paragraph.
    if (currentParagraphStyle)
        left += (line.ParagraphStart() && currentParagraphStyle->_firstLineIndent >= 0.0f)
            ? currentParagraphStyle->_firstLineIndent
            : currentParagraphStyle->_leftIndent;
    // The lines flow to the opposite of the y direction, so in the y direction, the line position
    // is "- flowAccumulation - topSpace - topMargin".
    float top = -flowAccumulation - topSpace - currentColumn.TopMargin();
    linePositions.emplace_back(left, top);
    flowAccumulation += lineTotalSpace;
}

CommonTextStatus 
CommonTextTrueTypeGenericLayoutManager::GenerateGenericLayout()
{
    if (_markupText)
    {
        _intermediateInfo = std::make_shared<CommonTextIntermediateInfo>(_markupText);

        // Divide the text run if it contains different scripts.
        CommonTextStatus divideScriptsStatus = _DivideTextRun(_DivideTextRunByTabs);
        if (divideScriptsStatus != CommonTextStatus::CommonTextStatusSuccess)
            return divideScriptsStatus;
        // Only enable multiLanguageHandler on Windows for now.
#ifdef _WIN32
            // Divide the text run if it contains tab string.
        divideScriptsStatus = _DivideTextRun(_DivideTextRunByScripts);
        if (divideScriptsStatus != CommonTextStatus::CommonTextStatusSuccess)
            return divideScriptsStatus;
#endif
        const UsdImagingTextStyle globalTextStyle = _markupText->GlobalTextStyle();
        std::shared_ptr<UsdImagingTextRunList> textRunList = _markupText->ListOfTextRuns();
        const std::wstring& markupString = _markupText->MarkupString();
        CommonTextRunLayoutList& listOfTextRunLayouts = _genericLayout->ListOfTextRunLayouts();

        // Generate the textrun layout.
        CommonTextStatus status = _GenerateSimpleLayoutForAllRuns(
            globalTextStyle, markupString, textRunList, listOfTextRunLayouts);
        if (status != CommonTextStatus::CommonTextStatusSuccess)
            return status;

        return _GenerateLayoutFromPreLayout();
    }
    else
        return CommonTextStatus::CommonTextStatusFail;
}

CommonTextStatus 
CommonTextTrueTypeGenericLayoutManager::_GenerateSimpleLayoutForAllRuns(
    const UsdImagingTextStyle& globalTextStyle,
    const std::wstring& markupString, 
    std::shared_ptr<UsdImagingTextRunList>& textRunList,
    CommonTextRunLayoutList& listOfTextRunLayouts)
{
    // iterate all the text runs.
    auto layoutIter        = listOfTextRunLayouts.before_begin();
    auto beforeTextRunIter = textRunList->before_begin();

    // fontSubstitutionEndIter save the position for the last replaced textrun if font substitution
    // happens.
    auto fontSubstitutionEndIter = textRunList->end();
    const std::shared_ptr<UsdImagingTextLineList> textLineList = _markupText->ListOfTextLines();

    // Traverse all lines.
    for (auto lineIter = textLineList->begin(); lineIter != textLineList->end(); lineIter++)
    {
        auto& range = lineIter->Range();
        if (range._isEmpty)
            continue;

        // Traverse the text runs in range.
        auto textRunIter = range._firstRun;
        auto lastTextRun = range._lastRun;
        lastTextRun++;
        while (textRunIter != lastTextRun)
        {
            UsdImagingTextRun& run = *textRunIter;
            CommonTextRunInfo& textRunInfo = _intermediateInfo->GetTextRunInfo(textRunIter);
            if (run.Type() == UsdImagingTextRunType::UsdImagingTextRunTypeString)
            {
                // If the textrun is string run, generate the layout using the simple manager.
                UsdImagingTextStyle textStyle = run.GetStyle(globalTextStyle);

                float scale = 1.0f;
                if (_useFullSizeToGenerateLayout)
                {
                    // The text shown on the screen can be zoom in/out. But the layout may not be
                    // proportional with the text size. To keep the text not jumping in the screen,
                    // we need to use a unified layout in different size. So it may be required to
                    // use full size to generate the layout and then scale to current size.
                    if (!CommonTextUtilities::GetFullSizeStyle(textStyle, scale))
                        return CommonTextStatus::CommonTextStatusFail;
                }

                CommonTextSimpleLayout textRunLayout;
                CommonTextTrueTypeSimpleLayoutManager simpleManager =
                    _textSystem->GetSimpleLayoutManager(textStyle);
                if (simpleManager.IsValid())
                {
                    // The string of the textrun.
                    std::wstring characters(markupString, run.StartIndex(), run.Length());
                    // Generate the metrics and indices for the characters.
                    CommonTextStatus status = simpleManager.GenerateSimpleLayout(
                        characters, textRunLayout, textRunInfo.ComplexScriptInformation());
                    if (status == CommonTextStatus::CommonTextStatusSuccess)
                    {
                        if (_useFullSizeToGenerateLayout)
                        {
                            textRunLayout.Scale(scale);
                        }

                        // Insert the textrun layout.
                        layoutIter = listOfTextRunLayouts.insert_after(
                            layoutIter, CommonTextRunLayout(textRunLayout));

                        // beforeTextRunIter must be increased.
                        beforeTextRunIter++;
                    }
                    // If generate returns NeedSubstitution, we will do font substitution process in
                    // the following progress.
                    else if (status == CommonTextStatus::CommonTextStatusNeedSubstitution)
                    {
                        // If fontSubstitutionEndIter is end, it means this text run is not the
                        // replaced textruns after font substitution. So we can do font substitution
                        // for this text run.
                        if (fontSubstitutionEndIter == textRunList->end())
                        {
                            std::shared_ptr<CommonTextMultiLanguageHandler> pMultiLanguageHandler =
                                _textSystem->GetMultiLanguageHandler();
                            if (pMultiLanguageHandler == nullptr)
                                return CommonTextStatus::CommonTextStatusFail;

                            // Get the subsitituted text runs after subsitituteFont.
                            UsdImagingTextRunList::iterator lastRunIter = textRunIter;
                            CommonTextStatus substitutionStatus = pMultiLanguageHandler->SubstituteFont(
                                _markupText, _intermediateInfo, textRunIter, textStyle, lineIter, textRunLayout, lastRunIter);
                            if (substitutionStatus == CommonTextStatus::CommonTextStatusSuccess)
                            {
                                // Set the lastTextRun to the fontSubstitutionEndIter. In this way,
                                // we don't do duplicate substitution for these textRuns.
                                fontSubstitutionEndIter = lastRunIter;
                                // Reset textRunIter to beforeTextRunIter, and we don't increase
                                // beforeTextRunIter. So in next iteration, we will handle the
                                // replaced textruns.
                                textRunIter = beforeTextRunIter;
                            }
                            else
                                return substitutionStatus;
                        }
                        else
                        {
                            // If fontSubstitutionEndIter is not the end, it means this text run is
                            // the replaced textruns after font substitution. We should not do font
                            // substitution again on them. Just create layout and move to the next
                            // textrun.

                            // Insert the textrun layout.
                            layoutIter = listOfTextRunLayouts.insert_after(
                                layoutIter, CommonTextRunLayout(textRunLayout));

                            beforeTextRunIter++;
                        }
                    }
                    else
                        return status;

                    // If we have processed all the replaced textruns after font substitution, we
                    // need to mark fontSubstitutionEndIter to end, so that we know that we can do
                    // font substitution for the following textruns.
                    if (textRunIter == fontSubstitutionEndIter)
                        fontSubstitutionEndIter = textRunList->end();
                }
                else
                    return CommonTextStatus::CommonTextStatusFail;
            }
            else
            {
                // For tab text run, just insert an empty textrun layout.
                CommonTextSimpleLayout textRunLayout;
                layoutIter = listOfTextRunLayouts.insert_after(
                    layoutIter, CommonTextRunLayout(textRunLayout));
                beforeTextRunIter++;
            }
            ++textRunIter;
        }
    }
    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextStatus 
CommonTextTrueTypeGenericLayoutManager::_InitializeLineLayouts(
    const std::shared_ptr<UsdImagingTextRunList>& textRunList,
    CommonTextRunLayoutList& listOfTextRunLayouts,
    const std::shared_ptr<UsdImagingTextLineList> textLineList, 
    CommonTextLineLayoutList& listOfTextLineLayouts)
{
    // Initialize the line layout for each line.
    auto textRunIter       = textRunList->begin();
    auto textRunLayoutIter = listOfTextRunLayouts.begin();
    for (const auto& line : *textLineList)
    {
        CommonTextLineLayout lineLayout;
        if (line.Range()._isEmpty)
        {
            lineLayout.Range()._isEmpty = true;
        }
        else
        {
            while (textRunIter != line.Range()._firstRun)
            {
                ++textRunIter;
                ++textRunLayoutIter;
            }
            lineLayout.Range()._firstRunLayout = textRunLayoutIter;
            while (textRunIter != line.Range()._lastRun)
            {
                ++textRunIter;
                ++textRunLayoutIter;
            }
            lineLayout.Range()._lastRunLayout = textRunLayoutIter;
        }
        listOfTextLineLayouts.push_back(std::move(lineLayout));
    }
    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextStatus 
CommonTextTrueTypeGenericLayoutManager::GetAbsolutePositionForAllTextRuns(
    CommonTextPosition2DArray& positionArray)
{
    size_t countOfBlocks = _genericLayout->ArrayBlockLayouts().size();
    if (_genericLayout->ArrayOfBlockPositions().size() != countOfBlocks)
        return CommonTextStatus::CommonTextStatusFail;
    for (size_t i = 0; i < countOfBlocks; i++)
    {
        // For each text block, get position.
        const CommonTextBlockLayout& blockLayout               = _genericLayout->ArrayBlockLayouts()[i];
        const std::pair<float, float>& blockPosition = _genericLayout->ArrayOfBlockPositions()[i];

        auto lineLayoutIter = blockLayout.FirstLineLayoutIter();
        size_t countOfLines = blockLayout.ArrayOfLinePositions().size();
        for(size_t j = 0; j < countOfLines; j++)
        {
            // For each line in the block, add the block position with the line position.
            const CommonTextLineLayout& lineLayout = *lineLayoutIter;
            const std::pair<float, float>& linePositionInBlock =
                blockLayout.ArrayOfLinePositions()[j];

            // Get the baseline position of the line. First we get the position of the line.
            std::pair<float, float> baselinePosition =
                std::pair<float, float>(blockPosition.first + linePositionInBlock.first,
                    blockPosition.second + linePositionInBlock.second);
            // Then move down by the line ascent to get the baseline position.
            baselinePosition.second -= lineLayout.LineAscent();

            const std::vector<std::pair<float, float>> runPositions =
                lineLayout.ArrayOfTextRunPositions();
            auto runLayoutIter = lineLayout.Range()._firstRunLayout;
            for (const auto& runPositionInLine : runPositions)
            {
                // For each textrun in the line, add the line position with the run position.
                // In y direction, the position of the textrun is the position of the basline plus
                // the ascent of the textrun.
                float textRunAscent =
                    runLayoutIter->SimpleLayout().FullMetrics()._semanticBound.Max()[1];
                std::pair<float, float> runPosition =
                    std::pair<float, float>(baselinePosition.first + runPositionInLine.first,
                        baselinePosition.second + runPositionInLine.second + textRunAscent);
                positionArray.push_back(std::move(runPosition));
                runLayoutIter++;
            }
            ++lineLayoutIter;
        }
    }
    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextStatus 
CommonTextTrueTypeGenericLayoutManager::_DivideTextRunByScripts(
    std::shared_ptr<UsdImagingMarkupText> markupText,
    std::shared_ptr<CommonTextIntermediateInfo> intermediateInfo,
    UsdImagingTextRunList::iterator textRunIter,
    UsdImagingTextLineList::iterator textLineIter,
    UsdImagingTextRunList::iterator& lastSubRunIter)
{
    if (textRunIter->Length() == 0 || textRunIter->Type() == UsdImagingTextRunType::UsdImagingTextRunTypeTab)
        return CommonTextStatus::CommonTextStatusSuccess;

    // Use the multilanguage handler to divide the text run by scripts.
    std::shared_ptr<CommonTextMultiLanguageHandler> languageHandler =
        CommonTextSystem::Instance()->GetMultiLanguageHandler();

    if (!languageHandler)
        return CommonTextStatus::CommonTextStatusSuccess;

    CommonTextStatus divideStatus =
        languageHandler->DivideStringByScripts(markupText, intermediateInfo, 
            textRunIter, textLineIter, lastSubRunIter);

    return divideStatus;
}

CommonTextStatus 
CommonTextTrueTypeGenericLayoutManager::_DivideTextRunByTabs(
    std::shared_ptr<UsdImagingMarkupText> markupText,
    std::shared_ptr<CommonTextIntermediateInfo> intermediateInfo,
    UsdImagingTextRunList::iterator textRunIter,
    UsdImagingTextLineList::iterator textLineIter,
    UsdImagingTextRunList::iterator& lastSubRunIter)
{
    if (textRunIter->Length() == 0)
    {
        lastSubRunIter = textRunIter;
        return CommonTextStatus::CommonTextStatusSuccess;
    }

    // The string of the textrun.
    std::wstring characters(markupText->MarkupString(),
        textRunIter->StartIndex(), textRunIter->Length());

    // Check from the first character forward.
    std::vector<int> dividePos;
    for (size_t index = 0; index < (int)characters.size(); ++index)
    {
        if (characters[index] == L'\t')
        {
            // This character is a tab.
            if (index != 0)
                dividePos.push_back(index);
            if (index != characters.size() - 1)
                dividePos.push_back(index + 1);
        }
    }
    if (dividePos.size() != 0)
    {
        CommonTextStatus status = DivideTextRun(markupText, intermediateInfo, 
            textRunIter, dividePos, textLineIter, lastSubRunIter);
        if (status != CommonTextStatus::CommonTextStatusSuccess)
            return status;
    }
    else
        lastSubRunIter = textRunIter;
    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextStatus 
CommonTextTrueTypeGenericLayoutManager::_DivideTextRun(
    CommonTextDivideTextRunFunc divideFunc)
{
    // For every text run, try to find the tab in it and then divide the text run from the tab.
    // The new created text run will be added to the current text line.
    const std::shared_ptr<UsdImagingTextRunList> textRunList = _markupText->ListOfTextRuns();
    if (textRunList->empty())
        return CommonTextStatus::CommonTextStatusSuccess;

    const std::shared_ptr<UsdImagingTextLineList> textLineList = _markupText->ListOfTextLines();
    UsdImagingTextLineList::iterator currentLineIter = textLineList->begin();
    // Divide the text run by tabs or scripts.
    for (UsdImagingTextRunList::iterator iter = textRunList->begin(); iter != textRunList->end(); ++iter)
    {
        UsdImagingTextRunList::iterator lastRunIter = iter;
        // Use divideFunc to do the dividing.
        CommonTextStatus divideStatus = divideFunc(_markupText, _intermediateInfo,
            iter, currentLineIter, lastRunIter);
        if (divideStatus != CommonTextStatus::CommonTextStatusSuccess)
            return divideStatus;
        iter = lastRunIter;
        if (iter == currentLineIter->Range()._lastRun)
        {
            ++currentLineIter;
        }
    }
    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextStatus 
CommonTextTrueTypeGenericLayoutManager::CollectDecorations(
    std::vector<CommonTextDecorationLayout>& decorationsArray)
{
    size_t countOfBlocks = _genericLayout->ArrayBlockLayouts().size();
    if (_genericLayout->ArrayBlockLayouts().size() != countOfBlocks)
        return CommonTextStatus::CommonTextStatusFail;
    for (size_t i = 0; i < countOfBlocks; i++)
    {
        // For each text block, get position.
        const CommonTextBlockLayout& blockLayout               = _genericLayout->ArrayBlockLayouts()[i];
        const std::pair<float, float>& blockPosition = _genericLayout->ArrayOfBlockPositions()[i];

        auto lineLayoutIter = blockLayout.FirstLineLayoutIter();
        size_t countOfLines = blockLayout.ArrayOfLinePositions().size();
        for (size_t j = 0; j < countOfLines; j++)
        {
            // For each line in the block, add the block position with the line position.
            const CommonTextLineLayout& lineLayout = *lineLayoutIter;
            const std::pair<float, float>& linePositionInBlock =
                blockLayout.ArrayOfLinePositions()[j];

            // Get the baseline position of the line. First we get the position of the line.
            std::pair<float, float> baselinePosition =
                std::pair<float, float>(blockPosition.first + linePositionInBlock.first,
                    blockPosition.second + linePositionInBlock.second);
            // Then move down by the line ascent to get the baseline position.
            baselinePosition.second -= lineLayout.LineAscent();

            // Update decorations by baseline position.
            const std::vector<CommonTextDecorationLayout>& decorations = lineLayout.Decorations();
            for (const auto& decoration : decorations)
            {
                CommonTextDecorationLayout outDecoration(UsdImagingTextProperty::UsdImagingTextPropertyUnderlineType);
                outDecoration._decoration     = decoration._decoration;
                outDecoration._type           = decoration._type;
                outDecoration._startXPosition = baselinePosition.first + decoration._startXPosition;
                outDecoration._yPosition      = baselinePosition.second + decoration._yPosition;
                for (const auto& section : decoration._sections)
                {
                    CommonTextDecorationLayout::CommonTextSection outSection;
                    outSection._lineColor    = section._lineColor;
                    outSection._endXPosition = baselinePosition.first + section._endXPosition;
                    outDecoration._sections.push_back(std::move(outSection));
                }
                decorationsArray.push_back(std::move(outDecoration));
            }
            ++lineLayoutIter;
        }
    }
    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextStatus
CommonTextTrueTypeGenericLayoutManager::DivideTextRun(
    std::shared_ptr<UsdImagingMarkupText> markupText,
    std::shared_ptr<CommonTextIntermediateInfo> intermediateInfo,
    UsdImagingTextRunList::iterator textRunIter,
    std::vector<int> dividePosInTextRun,
    UsdImagingTextLineList::iterator textLineIter,
    UsdImagingTextRunList::iterator& lastSubRunIter)
{
    // If there is no divide position, just return.
    int countOfSubRun = dividePosInTextRun.size();
    if (countOfSubRun == 0)
        return CommonTextStatus::CommonTextStatusSuccess;
    if(dividePosInTextRun[0] == 0)
        return CommonTextStatus::CommonTextStatusFail;

    // Add the position after the last character into the dividePosInTextRun.
    // In this way, we can always use dividePos[next]-dividePos[current] to get the length
    // of the current subTextRun.
    UsdImagingTextRun& fromRun = *textRunIter;
    dividePosInTextRun.push_back(fromRun.Length());

    std::wstring characters(markupText->MarkupString(), textRunIter->StartIndex(), textRunIter->Length());
    CommonTextRunInfo& textRunInfo = intermediateInfo->GetTextRunInfo(textRunIter);
    std::shared_ptr<UsdImagingTextRunList> listOfTextRuns = markupText->ListOfTextRuns();
    UsdImagingTextRunList::iterator subTextRunIter = textRunIter;
    for (int index = 0; index < countOfSubRun; ++index)
    {
        int dividePos = dividePosInTextRun[index];
        int nextPos = dividePosInTextRun[index + 1];
        // Create the subTextRun.
        UsdImagingTextRun subTextRun;
        subTextRun.CopyPartOfRun(fromRun, dividePos, nextPos - dividePos);
        // If the character is a tab, this is a tab run.
        if (characters[dividePos] == '\t')
            subTextRun.Type(UsdImagingTextRunType::UsdImagingTextRunTypeTab);

        CommonTextRunInfo subTextRunInfo;
        subTextRunInfo.CopyPartOfData(textRunInfo, dividePos, nextPos - dividePos);

        subTextRunIter = AddTextRun(markupText, intermediateInfo, subTextRunIter, subTextRun, subTextRunInfo);
    }
    // The original text run is shorten.
    fromRun.Shorten(dividePosInTextRun[0]);
    textRunInfo.Shorten(dividePosInTextRun[0]);
    // If the character is a tab, this is a tab run.
    if (characters[0] == '\t')
        fromRun.Type(UsdImagingTextRunType::UsdImagingTextRunTypeTab);
    lastSubRunIter = subTextRunIter;

    if (textLineIter->Range()._lastRun == textRunIter)
    {
        UsdImagingTextRunRange range = { textLineIter->Range()._firstRun, subTextRunIter, false };
        textLineIter->Range(range);
    }
    return CommonTextStatus::CommonTextStatusSuccess;
}

UsdImagingTextRunList::iterator
CommonTextTrueTypeGenericLayoutManager::AddTextRun(
    std::shared_ptr<UsdImagingMarkupText> markupText,
    std::shared_ptr<CommonTextIntermediateInfo> intermediateInfo,
    UsdImagingTextRunList::iterator insertPos,
    const UsdImagingTextRun& addedRun,
    const CommonTextRunInfo& textRunInfo)
{
    std::shared_ptr<UsdImagingTextRunList> listOfTextRuns = markupText->ListOfTextRuns();
    UsdImagingTextRunList::iterator newRunIter = listOfTextRuns->insert_after(insertPos, addedRun);
    intermediateInfo->_AddTextRunInfo(newRunIter, textRunInfo);
    return newRunIter;
}

UsdImagingTextLineList::iterator
CommonTextTrueTypeGenericLayoutManager::AddTextLine(
    std::shared_ptr<UsdImagingMarkupText> markupText,
    std::shared_ptr<CommonTextIntermediateInfo> intermediateInfo,
    UsdImagingTextLineList::iterator insertPos,
    const UsdImagingTextLine& addedLine,
    const WordBreakIndexList& wordBreakIndexList)
{
    std::shared_ptr<UsdImagingTextLineList> listOfTextLines = markupText->ListOfTextLines();
    UsdImagingTextLineList::iterator newLineIter = listOfTextLines->insert(insertPos, addedLine);
    if(newLineIter->LineType() == UsdImagingTextLineType::UsdImagingTextLineTypeNormal)
        intermediateInfo->_AddWordBreakIndexList(newLineIter, wordBreakIndexList);
    return newLineIter;
}
PXR_NAMESPACE_CLOSE_SCOPE
