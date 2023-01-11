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
#include "pxr/usdImaging/usdImaging/markupParser.h"
#include "pxr/usdImaging/usdImaging/markupText.h"
#include "pxr/usdImaging/usdImaging/markupParserRegistry.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<UsdImagingMarkupParser>();
}

UsdImagingMarkupParserSharedPtr UsdImagingMarkupParser::_markupParser = nullptr;
std::mutex UsdImagingMarkupParser::_initializeMutex;

//
// As this class is a pure interface class, it does not need a
// vtable.  However, it is possible that some users will use rtti.
// This will cause a problem for some of our compilers:
//
// In particular clang will throw a warning: -wweak-vtables
// For gcc, there is an issue were the rtti typeid's are different.
//
// As destruction of the class is not on the performance path,
// the body of the deleter is provided here, so a vtable is created
// in this compilation unit.
UsdImagingMarkupParser::~UsdImagingMarkupParser() = default;

bool
UsdImagingMarkupParser::DefaultInitialize()
{
    std::lock_guard<std::mutex> lock(_initializeMutex);
    if (IsInitialized())
        return true;

    // Initialize the markup parser with default setting.
    UsdImagingMarkupParser::ParserSettingMap parserSetting;
    parserSetting[UsdImagingTextTokens->supportLanguages] = "MTEXT";

    // Get and initialize a plugin from the registry.
    UsdImagingMarkupParserRegistry& registry = UsdImagingMarkupParserRegistry::GetInstance();
    _markupParser = registry._GetParser(parserSetting);
    return _markupParser != nullptr;
}

bool 
UsdImagingMarkupParser::Initialize(const ParserSettingMap& setting)
{
    if (IsInitialized())
        return true;
    else
    {
        // Get and initialize a plugin from the registry.
        UsdImagingMarkupParserRegistry& registry = UsdImagingMarkupParserRegistry::GetInstance();
        _markupParser = registry._GetParser(setting);
        return _markupParser != nullptr;
    }
}

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
    range._lastRun = markupText->ListOfTextRuns()->begin();
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
UsdImagingMarkupParser::ParseText(std::shared_ptr<UsdImagingMarkupText> markupText)
{
    if (markupText->MarkupLanguage().empty())
    {
        // If the text is plain text, we don't need to use plugin to parse the text.
        return InitializePlainText(markupText);
    }
    else if (!IsInitialized() || !_markupParser->_IsSupported(markupText->MarkupLanguage()))
    {
        // If there is no markup parser plugin, we will consider the text as plain text
        // without markup.
        markupText->MarkupLanguage(L"");
        return InitializePlainText(markupText);
    }
    else
        return _markupParser->_ParseText(markupText);

}

PXR_NAMESPACE_CLOSE_SCOPE

