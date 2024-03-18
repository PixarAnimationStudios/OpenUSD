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
#include "intermediateInfo.h"

PXR_NAMESPACE_OPEN_SCOPE
CommonTextRunInfo CommonTextIntermediateInfo::_defaultTextRunInfo;
WordBreakIndexList CommonTextIntermediateInfo::_defaultWordBreakIndexList;
void 
CommonTextRunInfo::CopyPartOfData(
    const CommonTextRunInfo& fromInfo,
    int startOffset,
    int length)
{
    // Copy the CommonTextScriptInfo.
    CommonTextScriptInfo lastInfo;
    for (auto info : fromInfo._scriptInfoArray)
    {
        if (info._indexOfFirstCharacter >= (int)startOffset &&
            info._indexOfFirstCharacter < (int)(startOffset + length))
        {
            info._indexOfFirstCharacter -= startOffset;
            if (_scriptInfoArray.size() == 0)
            {
                lastInfo._indexOfFirstCharacter = 0;
                _scriptInfoArray.push_back(lastInfo);
            }
            _scriptInfoArray.push_back(info);
        }
        else
            lastInfo = info;
    }

    // Copy the complex information.
    if (fromInfo._complexScriptInfo != nullptr)
    {
        _complexScriptInfo =
            std::make_shared<CommonTextComplexScriptInfo>(*(fromInfo._complexScriptInfo.get()));
    }
}

CommonTextIntermediateInfo::CommonTextIntermediateInfo(std::shared_ptr<UsdImagingMarkupText> markupText)
{
    const std::shared_ptr<UsdImagingTextRunList> textRunList = markupText->ListOfTextRuns();
    for (auto textRunIter = textRunList->begin(); textRunIter != textRunList->end(); ++textRunIter)
    {
        // For each TextRun, add a default information.
        _AddTextRunInfo(textRunIter, CommonTextRunInfo());
    }

    const std::shared_ptr<UsdImagingTextLineList> textLineList = markupText->ListOfTextLines();
    for (auto textLineIter = textLineList->begin(); textLineIter != textLineList->end(); ++textLineIter)
    {
        if (textLineIter->LineType() == UsdImagingTextLineType::UsdImagingTextLineTypeNormal)
        {
            // For each normal TextLine, add a default WordBreakIndexList.
            _AddWordBreakIndexList(textLineIter, WordBreakIndexList());
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
