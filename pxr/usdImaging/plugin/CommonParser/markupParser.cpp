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

#include "markupParser.h"
#include "environment.h"
#include "sink.h"

PXR_NAMESPACE_OPEN_SCOPE
static bool
InitializePlainText(std::shared_ptr<UsdImagingMarkupText> markupText)
{
    // At first, add the whole string as one text run.
    const std::wstring& markupString = markupText->MarkupString();
    UsdImagingTextRun run(UsdImagingTextRunType::UsdImagingTextRunTypeString, 0, static_cast<int>(markupString.length()));
    markupText->ListOfTextRuns()->push_front(std::move(run));
    // There will be only one line. The line contains the only text run.
    UsdImagingTextRunRange range;
    range._firstRun = markupText->ListOfTextRuns()->begin();
    range._lastRun  = markupText->ListOfTextRuns()->begin();
    range._isEmpty = false;
    UsdImagingTextLine line(range);
    line.StartBreak(UsdImagingTextLineBreak::UsdImagingTextLineBreakTextStart);
    line.EndBreak(UsdImagingTextLineBreak::UsdImagingTextLineBreakTextEnd);
    markupText->ListOfTextLines()->push_back(std::move(line));

    // If the text contains no block. Add one text column.
    // That is, the text will always contain a block.
    if (markupText->TextBlockArray()->empty())
    {
        markupText->TextBlockArray()->push_back(UsdImagingTextBlock());
    }
    UsdImagingTextBlockArray::iterator blockIter = markupText->TextBlockArray()->begin();
    UsdImagingTextLineList::iterator lineIter = markupText->ListOfTextLines()->begin();
    blockIter->FirstLineIter(lineIter);
    blockIter->LastLineIter(lineIter);

    return true;
}

bool 
CommonParserMarkupParser::Initialize(std::shared_ptr<UsdImagingMarkupText> markupText)
{
    _markupText = nullptr;
    if (markupText)
    {
        if (!markupText->MarkupLanguage().empty())
        {
            // Check if the markup language in markupText is supported by the parser.
            CommonParserUniverse* pUniverse = BigBang();
            if (!pUniverse)
                return false;

            CommonParserGenerator* pGenerator =
                pUniverse->GetGenerator(markupText->MarkupLanguage().data());
            if (!pGenerator)
                return false;
        }
    }
    _markupText = markupText;
    return true;
}

bool
CommonParserMarkupParser::ParseText()
{
    if (_markupText)
    {
        if (_markupText->MarkupLanguage().empty())
        {
            // By default, we consider the markup string contains no markup, that it is a plain text
            // string.
            return InitializePlainText(_markupText);
        }

        // Create the generator from the markupLanguage.
        CommonParserUniverse* pUniverse = BigBang();
        if (!pUniverse)
            return false;

        CommonParserGenerator* pGenerator =
            pUniverse->GetGenerator(_markupText->MarkupLanguage().data());
        if (!pGenerator)
            return false;

        // Create the parser.
        CommonParserParser* pParser = nullptr;
        if (pGenerator->Create(&pParser).Succeeded())
        {
            // Use CommonTextMarkupParser to parse the data to markupText.
            return _ParseInternalRepresentation(pParser);
        }
        else
            return false;
    }
    return true;
}

bool 
CommonParserMarkupParser::_ParseInternalRepresentation(
    CommonParserParser* parser)
{
    assert(_markupText != nullptr);
    assert(parser != nullptr);
    if (_markupText->MarkupString().empty())
    {
        return true;
    }

    // If the text contains no block. Add one text column.
    // That is, the text will always contain a block.
    if (_markupText->TextBlockArray()->empty())
        _markupText->TextBlockArray()->push_back(UsdImagingTextBlock());

    // Create the sink and begin the parsing process
    CommonParserUniverse* pUniverse = BigBang();
    if (pUniverse == nullptr)
        return false;
    CommonParserGenerator* pTextGenerator = pUniverse->GetGenerator(TEXT_ATOM_GENERATOR_NAME);
    CommonParserSink* pTextSink           = nullptr;
    if (pTextGenerator == nullptr)
        return false;

    if (pTextGenerator->Create(&pTextSink).Succeeded())
    {
        ((CommonParserMarkupSink*)pTextSink)->InternalRepresentation(_markupText);

        // Create the environment.
        CommonParserAmbient textAmbient;
        CommonParserEmptyStyleTable emptyStyleTable;
        CommonParserMarkupEnvironment env(
            pTextSink, &emptyStyleTable, CommonParserColor(0, 0, 0), &textAmbient);

        // Parsing process.
        CommonParserStatus parseStatus = parser->Parse(
            CommonParserStRange(_markupText->MarkupString().c_str()), &env);
        pTextGenerator->Destroy(pTextSink);

        if (!parseStatus.Succeeded())
        {
            // TODO:add error here
            return false;
        }

        // Post process. Make the layout validate.
        _PostProcess();

        return true;
    }
    else
    {
        // TODO: add error here.
        return false;
    }
}

// The end of each line should be consistent with the start of the next line.
bool 
CommonParserMarkupParser::_PostProcess()
{
    if (_markupText == nullptr)
        return false;
    const std::shared_ptr<UsdImagingTextLineList> textLineList = _markupText->ListOfTextLines();
    if (textLineList == nullptr)
        return false;
    if (!textLineList->empty())
    {
        auto lineIter = textLineList->begin();
        lineIter->StartBreak(UsdImagingTextLineBreak::UsdImagingTextLineBreakTextStart);
        auto nextLineIter = lineIter;
        ++nextLineIter;
        while (lineIter != textLineList->end())
        {
            if (nextLineIter != textLineList->end())
            {
                lineIter->EndBreak(nextLineIter->StartBreak());
                lineIter->ParagraphEnd(nextLineIter->ParagraphStart());
                ++lineIter;
                ++nextLineIter;
            }
            else
            {
                lineIter->EndBreak(UsdImagingTextLineBreak::UsdImagingTextLineBreakTextEnd);
                ++lineIter;
            }
        }
    }
    return true;
}
PXR_NAMESPACE_CLOSE_SCOPE
