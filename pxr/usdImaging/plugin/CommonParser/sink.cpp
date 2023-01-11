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

#include "sink.h"
#include "environment.h"
#include "portableUtils.h"

PXR_NAMESPACE_OPEN_SCOPE
CommonParserMarkupGenerator LongLiveATOM;
CommonParserMarkupGenerator::CommonParserMarkupGenerator()
{
    BigBang()->Register(this);
    _endRegister = false;
}

CommonParserMarkupGenerator::~CommonParserMarkupGenerator()
{
    if (_endRegister == false)
    {
        BigBang()->Unregister(this);
        _endRegister = true;
    }
}

CommonParserStatus 
CommonParserMarkupGenerator::Create(CommonParserSink** ppNewSink)
{
    if (ppNewSink == nullptr)
        return CommonParserStatusTypeInvalidArg;

    *ppNewSink = new CommonParserMarkupSink(this);

    return CommonParserStatusTypeOk;
}

CommonParserStatus 
CommonParserMarkupGenerator::Create(CommonParserParser** ppNewParser)
{
    if (ppNewParser == nullptr)
        return CommonParserStatusTypeInvalidArg;

    return CommonParserStatusTypeNotImplemented;
}

bool 
CommonParserMarkupGenerator::HasSink() const
{
    return false;
}

CommonParserStatus 
CommonParserMarkupGenerator::Destroy(CommonParserSink* pOldSink)
{
    if (pOldSink == nullptr)
        return CommonParserStatusTypeInvalidArg;

    CommonParserMarkupSink* pOldATOMSink = dynamic_cast<CommonParserMarkupSink*>(pOldSink);

    if (pOldATOMSink != nullptr)
    {
        delete (pOldATOMSink);
        return CommonParserStatusTypeOk;
    }
    else
        return CommonParserStatusTypeInvalidArg;
}

CommonParserStatus
CommonParserMarkupGenerator::Destroy(CommonParserParser* pOldParser)
{
    if (pOldParser == nullptr)
        return CommonParserStatusTypeInvalidArg;

    return CommonParserStatusTypeNotImplemented;
}

const CommonParserStRange 
CommonParserMarkupGenerator::Name() const
{
    return TEXT_ATOM_GENERATOR_NAME;
}

const CommonParserStRange 
CommonParserMarkupGenerator::Description() const
{
    return TEXT_ATOM_GENERATOR_DESC;
}

CommonParserStatus 
CommonParserMarkupGenerator::RegisterNull()
{
    _endRegister = true;
    return CommonParserStatusTypeOk;
}

CommonParserMarkupSink::CommonParserMarkupSink(CommonParserGenerator* pGenerator)
    : _generator(pGenerator)
{
    assert(nullptr != pGenerator);
}

CommonParserMarkupSink::~CommonParserMarkupSink(void)
{
    if (_sinkState != CommonParserSinkStateTypeWaiting)
        Terminate(nullptr);
}
CommonParserSinkStateType 
CommonParserMarkupSink::SinkState()
{
    return _sinkState;
}

CommonParserGenerator* 
CommonParserMarkupSink::GetGenerator()
{
    return _generator;
}

CommonParserStatus 
CommonParserMarkupSink::TextRun(CommonParserTextRun* pITextRun,
                                CommonParserEnvironment*)
{
    if (!_HandleStructure(pITextRun))
        return CommonParserStatusTypeAbandoned;
    std::shared_ptr<UsdImagingTextRun> pTextRun;
    if (!_AddTextRun(pITextRun, pTextRun))
        return CommonParserStatusTypeAbandoned;

    if (pTextRun != nullptr)
    {
        if (!_HandleTransform(pITextRun, pTextRun))
            return CommonParserStatusTypeAbandoned;
    }
    if (!_HandleLocation(pITextRun))
        return CommonParserStatusTypeAbandoned;
    return CommonParserStatusTypeOk;
}

CommonParserStatus 
CommonParserMarkupSink::Abandon(CommonParserAbandonment* abandonElement,
                                CommonParserEnvironment* /*pEnv*/)
{
    if (_sinkState != CommonParserSinkStateTypeInitialized)
        return CommonParserStatusTypeNotReady;

    _sinkState = CommonParserSinkStateTypeAbandoned;
    return CommonParserStatusTypeOk;
}

CommonParserStatus 
CommonParserMarkupSink::Terminate(CommonParserEnvironment*)
{
    if (_sinkState != CommonParserSinkStateTypeInitialized && 
        _sinkState != CommonParserSinkStateTypeAbandoned)
        return CommonParserStatusTypeNotReady;

    // Finish the last paragraph.
    std::shared_ptr<UsdImagingTextParagraphArray> pParagraphArray = (_markupText)->TextParagraphArray();
    if (!(pParagraphArray->empty()))
    {
        UsdImagingTextParagraph& currentParargraph = pParagraphArray->at(_currentParagraphIndex);
        currentParargraph.LastLineIter(_currentTextLineIter);
    }

    // Finish the last column.
    UsdImagingTextBlock& currentBlock = _markupText->TextBlockArray()->at(_currentColumnIndex);
    currentBlock.LastLineIter(_currentTextLineIter);

    _sinkState = CommonParserSinkStateTypeWaiting;

    return CommonParserStatusTypeOk;
}

CommonParserStatus 
CommonParserMarkupSink::Initialize(CommonParserEnvironment*)
{
    if (_sinkState != CommonParserSinkStateTypeWaiting)
        return CommonParserStatusTypeNotReady;

    _sinkState = CommonParserSinkStateTypeInitialized;

    _currentParagraphStyle = _markupText->GlobalParagraphStyle();
    const std::shared_ptr<TextParagraphStyleArray> pParagraphStyleArray =
        _markupText->ParagraphStyleArray();
    if (!pParagraphStyleArray->empty())
    {
        _currentParagraphStyle = pParagraphStyleArray->at(0);
    }
    _currentTextStyle = std::make_shared<UsdImagingTextStyle>(_markupText->GlobalTextStyle());

    assert(_markupText->ListOfTextRuns());
    _currentTextRunIter = _markupText->ListOfTextRuns()->before_begin();
    // By default, there is no line and no paragraph.
    _currentParagraphIndex = -1;
    // There will be at least one column.
    _currentColumnIndex = 0;
    return CommonParserStatusTypeOk;
}

bool 
CommonParserMarkupSink::_AddTextRun(CommonParserTextRun* pITextRun,
                                    std::shared_ptr<UsdImagingTextRun>& pTextRun)
{
    CommonParserStyleChangeElement* styleChange = (CommonParserStyleChangeElement*)pITextRun->Style();
    if (styleChange == nullptr)
        return false;
    const CommonParserStRange contents = pITextRun->Contents();
    if (contents.Length() == 0)
    {
        // Don't add textrun if the content is empty.
        return true;
    }

    if (pTextRun == nullptr)
        pTextRun = std::make_shared<UsdImagingTextRun>();

    // Set the position in the Markup String of UsdImagingMarkupText that this textRun starts.
    pTextRun->StartIndex((int)((wchar_t*)contents.Start() - _markupText->MarkupString().c_str()));

    pTextRun->Length(contents.Length());

    // 0: No height change. 1: Proportional height change. 2, Inproportional height change.
    CommonParserHeightChange heightChange = CommonParserHeightChange::CommonParserHeightChangeNoChange;
    const CommonParserStyleParticle* styleParticle = styleChange->Description();
    while (styleParticle != nullptr)
    {
        switch (styleParticle->Type())
        {
            // The typeface particle
        case CommonParserStyleParticleTypeTypeface:
        {
            const CommonParserTypefaceStyleParticle* typefaceStyle =
                dynamic_cast<const CommonParserTypefaceStyleParticle*>(styleParticle);
            if (typefaceStyle == nullptr)
                return false;
            _currentTextStyle->_typeface =
                w2s(std::wstring(typefaceStyle->Value().Start(), typefaceStyle->Value().Length()));
            std::shared_ptr<UsdImagingTextStyleChange> pTypefaceChange = std::make_shared<UsdImagingTextStyleChange>();
            pTypefaceChange->_changeType = UsdImagingTextProperty::UsdImagingTextPropertyTypeface;
            pTypefaceChange->_stringValue =
                std::make_shared<std::string>(_currentTextStyle->_typeface);
            pTextRun->AddStyleChange(*pTypefaceChange);
            break;
        }
            // The Cap size particle
        case CommonParserStyleParticleTypeSize:
        {
            // The text height is set later. Because it requires the typeface
            // is updated first.
            const CommonParserSizeStyleParticle* sizeStyle =
                dynamic_cast<const CommonParserSizeStyleParticle*>(styleParticle);
            if (sizeStyle == nullptr)
                return false;
            if (sizeStyle->Value().Units() == CommonParserMeasureUnitProportion)
            {
                // If the SizeStyleParticle is a Proportional particle,
                // we have to change it to a Model particle.
                _currentTextStyle->_height =
                    _currentTextStyle->_height * (int)sizeStyle->Value().Number();
                // Proportional height change
                heightChange = CommonParserHeightChange::CommonParserHeightChangeProportional;
            }
            else
            {
                _currentTextStyle->_height = (int)sizeStyle->Value().Number();
                // Inproportional height change
                heightChange = CommonParserHeightChange::CommonParserHeightChangeInproportional;
            }
            std::shared_ptr<UsdImagingTextStyleChange> pHeightChange = std::make_shared<UsdImagingTextStyleChange>();
            pHeightChange->_changeType = UsdImagingTextProperty::UsdImagingTextPropertyHeight;
            pHeightChange->_intValue = _currentTextStyle->_height;
            pTextRun->AddStyleChange(*pHeightChange);
            break;
        }
        // The italic particle
        case CommonParserStyleParticleTypeItalic:
        {
            const CommonParserItalicStyleParticle* italicStyle =
                dynamic_cast<const CommonParserItalicStyleParticle*>(styleParticle);
            if (italicStyle == nullptr)
                return false;
            std::shared_ptr<UsdImagingTextStyleChange> pItalicChange = std::make_shared<UsdImagingTextStyleChange>();
            pItalicChange->_changeType = UsdImagingTextProperty::UsdImagingTextPropertyItalic;
            _currentTextStyle->_italic = italicStyle->Value();
            pItalicChange->_boolValue = _currentTextStyle->_italic;
            pTextRun->AddStyleChange(*pItalicChange);
            break;
        }
        // The italic particle
        case CommonParserStyleParticleTypeFontWeight:
        {
            const CommonParserFontWeightStyleParticle* weightStyle =
                dynamic_cast<const CommonParserFontWeightStyleParticle*>(styleParticle);
            if (weightStyle == nullptr)
                return false;
            std::shared_ptr<UsdImagingTextStyleChange> pBoldChange = std::make_shared<UsdImagingTextStyleChange>();
            pBoldChange->_changeType = UsdImagingTextProperty::UsdImagingTextPropertyBold;
            _currentTextStyle->_bold = (weightStyle->Value() > 500);
            pBoldChange->_boolValue = _currentTextStyle->_bold;
            pTextRun->AddStyleChange(*pBoldChange);
            break;
        }
        // The color particle.
        case CommonParserStyleParticleTypeFillColor:
        {
            const CommonParserFillColorStyleParticle* colorStyle =
                dynamic_cast<const CommonParserFillColorStyleParticle*>(styleParticle);
            if (colorStyle == nullptr)
                return false;
            CommonParserColor fillColor = colorStyle->Value();
            pxr::UsdImagingTextColor color;
            color.red   = fillColor.R() / 255.0f;
            color.green = fillColor.G() / 255.0f;
            color.blue  = fillColor.B() / 255.0f;
            color.alpha = fillColor.A() / 255.0f;
            pTextRun->SetTextColor(color);
            break;
        }
        // The inter-character space particle
        case CommonParserStyleParticleTypeTrackingAugment:
        {
            const CommonParserTrackingAugmentStyleParticle* trackingAugmentStyleParticle =
                dynamic_cast<const CommonParserTrackingAugmentStyleParticle*>(styleParticle);
            if (trackingAugmentStyleParticle == nullptr)
                return false;
            std::shared_ptr<UsdImagingTextStyleChange> pCharacterSpaceChange =
                std::make_shared<UsdImagingTextStyleChange>();
            pCharacterSpaceChange->_changeType = UsdImagingTextProperty::UsdImagingTextPropertyCharacterSpaceFactor;
            _currentTextStyle->_characterSpaceFactor =
                trackingAugmentStyleParticle->Value().Number();
            pCharacterSpaceChange->_floatValue = _currentTextStyle->_characterSpaceFactor;
            pTextRun->AddStyleChange(*pCharacterSpaceChange);
            break;
        }
        // The line height particle.
        case CommonParserStyleParticleTypeLineHeight:
        {
            const CommonParserLineHeightStyleParticle* lineHeightStyleParticle =
                dynamic_cast<const CommonParserLineHeightStyleParticle*>(styleParticle);
            if (lineHeightStyleParticle == nullptr)
                return false;
            CommonParserLineHeightMeasureType lineHeightType =
                lineHeightStyleParticle->Value()._lineHeightType;

            switch (lineHeightType)
            {
            case CommonParserLineHeightMeasureTypeExactly:
                _currentParagraphStyle._lineSpaceType = UsdImagingLineSpaceType::UsdImagingLineSpaceTypeExactly;
                break;
            case CommonParserLineHeightMeasureTypeAtLeast:
                _currentParagraphStyle._lineSpaceType = UsdImagingLineSpaceType::UsdImagingLineSpaceTypeAtLeast;
                break;
            case CommonParserLineHeightMeasureTypeMultiple:
                _currentParagraphStyle._lineSpaceType = UsdImagingLineSpaceType::UsdImagingLineSpaceTypeMulti;
                break;
            default:
                _currentParagraphStyle._lineSpaceType = UsdImagingLineSpaceType::UsdImagingLineSpaceTypeAtLeast;
            }

            if (lineHeightStyleParticle->Value()._lineHeight.Units() ==
                CommonParserMeasureUnitProportion)
            {
                // If the LineHeightStyleParticle is a Proportional particle,
                // we have to change it to a Model particle.
                _currentParagraphStyle._lineSpace =
                    _currentParagraphStyle._lineSpace * lineHeightStyleParticle->Value()._lineHeight.Number();
                styleChange->AddToDescription(CommonParserLineHeightStyleParticle(
                    CommonParserLineHeightMeasure(CommonParserMeasure(_currentParagraphStyle._lineSpace,
                    CommonParserMeasureUnitModel, NULL),
                    lineHeightType)));
            }
            else
            {
                _currentParagraphStyle._lineSpace = lineHeightStyleParticle->Value()._lineHeight.Number();
            }
            break;
        }
        // The underlineType particle.
        case CommonParserStyleParticleTypeUnderline:
        {
            const CommonParserUnderlineStyleParticle* underlineTypeStyle =
                dynamic_cast<const CommonParserUnderlineStyleParticle*>(styleParticle);
            if (underlineTypeStyle == nullptr)
                return false;
            if (underlineTypeStyle->Value() == CommonParserTextLineTypeNone)
                _currentTextStyle->_underlineType = UsdImagingTextTokens->none;
            else
                _currentTextStyle->_underlineType = UsdImagingTextTokens->normal;
            std::shared_ptr<UsdImagingTextStyleChange> pUnderlineTypeChange =
                std::make_shared<UsdImagingTextStyleChange>();
            pUnderlineTypeChange->_changeType = UsdImagingTextProperty::UsdImagingTextPropertyUnderlineType;
            pUnderlineTypeChange->_stringValue = std::make_shared<std::string>(_currentTextStyle->_underlineType.data());
            pTextRun->AddStyleChange(*pUnderlineTypeChange);
            break;
        }
        // The overlineType particle.
        case CommonParserStyleParticleTypeOverline:
        {
            const CommonParserOverlineStyleParticle* overlineTypeStyle =
                dynamic_cast<const CommonParserOverlineStyleParticle*>(styleParticle);
            if (overlineTypeStyle == nullptr)
                return false;
            if (overlineTypeStyle->Value() == CommonParserTextLineTypeNone)
                _currentTextStyle->_overlineType = UsdImagingTextTokens->none;
            else
                _currentTextStyle->_overlineType = UsdImagingTextTokens->normal;
            std::shared_ptr<UsdImagingTextStyleChange> pOverlineTypeChange =
                std::make_shared<UsdImagingTextStyleChange>();
            pOverlineTypeChange->_changeType = UsdImagingTextProperty::UsdImagingTextPropertyOverlineType;
            pOverlineTypeChange->_stringValue = std::make_shared<std::string>(_currentTextStyle->_overlineType.data());
            pTextRun->AddStyleChange(*pOverlineTypeChange);
            break;
        }
        // The strike through particle.
        case CommonParserStyleParticleTypeStrikethrough:
        {
            const CommonParserStrikethroughStyleParticle* strikethroughTypeStyle =
                dynamic_cast<const CommonParserStrikethroughStyleParticle*>(styleParticle);
            if (strikethroughTypeStyle == nullptr)
                return false;
            if (strikethroughTypeStyle->Value() == CommonParserTextLineTypeNone)
                _currentTextStyle->_strikethroughType = UsdImagingTextTokens->none;
            else if (strikethroughTypeStyle->Value() == CommonParserTextLineTypeDouble)
                _currentTextStyle->_strikethroughType = UsdImagingTextTokens->doubleLines;
            else
                _currentTextStyle->_strikethroughType = UsdImagingTextTokens->normal;
            std::shared_ptr<UsdImagingTextStyleChange> pStrikethroughTypeChange =
                std::make_shared<UsdImagingTextStyleChange>();
            pStrikethroughTypeChange->_changeType = UsdImagingTextProperty::UsdImagingTextPropertyStrikethroughType;
            pStrikethroughTypeChange->_stringValue = std::make_shared<std::string>(_currentTextStyle->_strikethroughType.data());
            pTextRun->AddStyleChange(*pStrikethroughTypeChange);
            break;
        }
        default:
            // other particles unidentified
            break;
        }
        styleParticle = styleParticle->Next();
    }

    // Insert textRun after the previous textrun.
    std::shared_ptr<UsdImagingTextRunList> pTextRunList = _markupText->ListOfTextRuns();
    _currentTextRunIter = pTextRunList->insert_after(_currentTextRunIter, *pTextRun);

    // If the textline list is empty, we will add a first line.
    if (_markupText->ListOfTextLines()->size() == 0)
    {
        // Add the first text line.
        UsdImagingTextLine textLine;
        textLine.StartBreak(UsdImagingTextLineBreak::UsdImagingTextLineBreakTextStart);
        _markupText->ListOfTextLines()->push_back(std::move(textLine));
        _currentTextLineIter = _markupText->ListOfTextLines()->begin();

        // This is the first line, so we get the current column and add the line to the column.
        UsdImagingTextBlock& block = _markupText->TextBlockArray()->at(_currentColumnIndex);
        block.FirstLineIter(_currentTextLineIter);
    }
    // Add the text run to the current line.
    _currentTextLineIter->AddTextRun(_currentTextRunIter);

    return true;
}

bool 
CommonParserMarkupSink::_HandleStructure(CommonParserTextRun* pITextRun)
{
    int depth = pITextRun->Structure()->Depth();
    if (depth > _currentDepth)
    {
        int key = 0;
        for (int i = 0; i < depth - _currentDepth; i++)
        {
            bool foundTextStyle = false;
            // Look for the textstyle in the text style list.
            for (const auto& it : _textStyleMap)
            {
                if (it.second == _currentTextStyle)
                {
                    key            = it.first;
                    foundTextStyle = true;
                }
            }

            // If not found, insert it at the end
            if (!foundTextStyle)
            {
                key = static_cast<int>(_textStyleMap.size());
                _textStyleMap.emplace(key, _currentTextStyle);
            }
        }
        _textStyleStack.emplace(key);
        _currentDepth = depth;
    }

    else if (depth < _currentDepth)
    {
        for (int i = 0; i < depth - _currentDepth - 1; i++)
        {
            _textStyleStack.pop();
        }
        _currentTextStyle = _textStyleMap.at(_textStyleStack.top());
        _textStyleStack.pop();
        _currentDepth = depth;
    }
    return true;
}

bool 
CommonParserMarkupSink::_HandleLocation(CommonParserTextRun* pITextRun)
{
    const CommonParserLocation* pLocation = pITextRun->Location();

    if (pLocation->Semantics() == CommonParserSemanticTypeNormal)
    {
        return true;
    }
    else if (pLocation->Semantics() == CommonParserSemanticTypeLine)
    {
        // The line break create a new line.
        // First check if the textline list is empty. If it is empty, we will first add a zero
        // line.
        const std::shared_ptr<UsdImagingTextLineList> textLineList = _markupText->ListOfTextLines();
        if (textLineList->empty())
        {
            // We create a zero line before the line break. This zero line is the first line.
            UsdImagingTextLine zeroLine;
            zeroLine.StartBreak(UsdImagingTextLineBreak::UsdImagingTextLineBreakTextStart);
            textLineList->push_back(std::move(zeroLine));
            _currentTextLineIter = textLineList->begin();

            // Get the current column and add the first line to the column.
            UsdImagingTextBlock& block = _markupText->TextBlockArray()->at(_currentColumnIndex);
            block.FirstLineIter(_currentTextLineIter);
        }
        // Then create a new line.
        UsdImagingTextLine textLine;
        textLine.StartBreak(UsdImagingTextLineBreak::UsdImagingTextLineBreakLineBreak);
        textLineList->push_back(std::move(textLine));

        ++_currentTextLineIter;
        return true;
    }
    else if (pLocation->Semantics() == CommonParserSemanticTypeParagraph)
    {
        // The paragraph break will create a new paragraph,
        // and also it will create a new line.
        const CommonParserLocationParticle* pLocationParticle = pLocation->Operations();
        while (pLocationParticle != nullptr)
        {
            switch (pLocationParticle->Type())
            {
            case CommonParserLocationParticleTypeLineBreak:
            {
                // If we meet with a paragraph break, we know that this piece of text contains
                // paragraph information. So we must make the lines before this paragraph break
                // into a paragraph too.
                // So we add a paragraph which starts from the first line, ends at the current line.

                // First check if there is already text lines before the paragraph break.
                const std::shared_ptr<UsdImagingTextLineList> textLineList = _markupText->ListOfTextLines();
                const std::shared_ptr<UsdImagingTextParagraphArray> paragraphArray =
                    _markupText->TextParagraphArray();
                const std::shared_ptr<TextParagraphStyleArray> paragraphStyleArray =
                    _markupText->ParagraphStyleArray();
                // There is already lines before the paragraph, and the paragraph array is still empty,
                // in this case, add a new paragraph, and set the firstline in the line list as the 
                // first line of the paragraph.
                if (!textLineList->empty() && paragraphArray->empty())
                {
                    UsdImagingTextParagraph pParagraph;
                    UsdImagingTextLineList::iterator firstLineIter = textLineList->begin();
                    pParagraph.FirstLineIter(firstLineIter);
                    pParagraph.Style(*(_currentParagraphStyle.clone()));
                    paragraphArray->push_back(std::move(pParagraph));
                    _currentParagraphIndex = 0;

                    firstLineIter->ParagraphStart(true);
                }

                // Add a new line and set it to paragraph start.
                bool isFirstLine = textLineList->empty();
                if (isFirstLine ||
                    _currentTextLineIter->LineType() != UsdImagingTextLineType::UsdImagingTextLineTypeZero ||
                    _currentTextLineIter->StartBreak() != UsdImagingTextLineBreak::UsdImagingTextLineBreakBlockBreak)
                {
                    // If currently there is already valid paragraph, we will finish the paragraph.
                    if (_currentParagraphIndex != -1)
                    {
                        UsdImagingTextParagraph& currentParargraph = paragraphArray->at(_currentParagraphIndex);
                        _currentTextLineIter->ParagraphEnd(true);
                        currentParargraph.LastLineIter(_currentTextLineIter);
                    }
                    // It is not the first line of a block, add a new line for the paragraph.
                    UsdImagingTextLine textLine;
                    textLine.StartBreak(isFirstLine ? UsdImagingTextLineBreak::UsdImagingTextLineBreakTextStart
                        : UsdImagingTextLineBreak::UsdImagingTextLineBreakLineBreak);
                    textLineList->push_back(std::move(textLine));
                    if (isFirstLine)
                        _currentTextLineIter = textLineList->begin();
                    else
                        ++_currentTextLineIter;
                }
                else
                {
                    // It is the first line of a block, so this line will be the start of the paragraph.
                    // If currently there is already valid paragraph, we will finish the paragraph. The 
                    // previous line will be the end of the paragraph.
                    if (_currentParagraphIndex != -1)
                    {
                        UsdImagingTextParagraph& currentParargraph = paragraphArray->at(_currentParagraphIndex);
                        UsdImagingTextLineList::iterator previousLineIter = _currentTextLineIter;
                        --previousLineIter;
                        previousLineIter->ParagraphEnd(true);
                        currentParargraph.LastLineIter(previousLineIter);
                    }
                }

                _currentParagraphIndex++;
                if(_currentParagraphIndex < paragraphStyleArray->size())
                { 
                    _currentParagraphStyle = paragraphStyleArray->at(_currentParagraphIndex);
                }
                if (_currentParagraphIndex == paragraphArray->size())
                {
                    // Add a new paragraph.
                    UsdImagingTextParagraph pParagraph;
                    pParagraph.Style(*(_currentParagraphStyle.clone()));
                    paragraphArray->push_back(std::move(pParagraph));
                }
                UsdImagingTextParagraph& currentParargraph = paragraphArray->at(_currentParagraphIndex);
                _currentTextLineIter->ParagraphStart(true);
                currentParargraph.FirstLineIter(_currentTextLineIter);
                break;
            }
            default:
                break;
            }
            pLocationParticle = pLocationParticle->Next();
        }
        return true;
    }
    else if (pLocation->Semantics() == CommonParserSemanticTypeFlowColumn)
    {
        // The column break will not create a new column, but put the following line to the next
        // column. The column break will create a new line, although.
        // First check if the textline list is empty. If it is empty, we will first add a zero
        // line.
        const std::shared_ptr<UsdImagingTextLineList> textLineList = _markupText->ListOfTextLines();
        if (textLineList->empty())
        {
            // We create a zero line before the column break. This zero line is the first
            // line.
            UsdImagingTextLine zeroLine;
            zeroLine.StartBreak(UsdImagingTextLineBreak::UsdImagingTextLineBreakTextStart);
            textLineList->push_back(std::move(zeroLine));
            _currentTextLineIter = textLineList->begin();

            // Get the current column and add the first line to the column.
            UsdImagingTextBlock& block = _markupText->TextBlockArray()->at(_currentColumnIndex);
            block.FirstLineIter(_currentTextLineIter);
        }

        // If this is not the last column, we will finish the current column, and increase the lineIter.
        if (_currentColumnIndex < _markupText->TextBlockArray()->size() - 1)
        {
            // Set the LastLineIter of the current column, and then move to next column.
            UsdImagingTextBlock& currentBlock = _markupText->TextBlockArray()->at(_currentColumnIndex);
            currentBlock.LastLineIter(_currentTextLineIter);
        }

        // Add a new line.
        UsdImagingTextLine textLine;
        textLine.StartBreak(UsdImagingTextLineBreak::UsdImagingTextLineBreakBlockBreak);
        textLineList->push_back(std::move(textLine));
        ++_currentTextLineIter;

        // If this is not the last column, we will go to the next column.
        if (_currentColumnIndex < _markupText->TextBlockArray()->size() - 1)
        {
            _currentColumnIndex++;

            // Set the FirstLineIter.
            UsdImagingTextBlock& currentBlock = _markupText->TextBlockArray()->at(_currentColumnIndex);
            currentBlock.FirstLineIter(_currentTextLineIter);
        }

        return true;
    }
    else
        return true;
}

bool 
CommonParserMarkupSink::_HandleTransform(CommonParserTextRun* pITextRun,
                                         std::shared_ptr<UsdImagingTextRun> pTextRun)
{
    const CommonParserTransform* pTransform = pITextRun->Transform();
    const CommonParserTransformParticle* pTransformParticle  = pTransform->Description();
    while (pTransformParticle != nullptr)
    {
        switch (pTransformParticle->Semantics())
        {
        case CommonParserTransformParticleSemanticsOblique:
        {
            CommonParserSkewTransformParticle* pSkewTransformParticle =
                (CommonParserSkewTransformParticle*)pTransformParticle;

            std::shared_ptr<UsdImagingTextStyleChange> pObliqueAngleChange =
                std::make_shared<UsdImagingTextStyleChange>();
            pObliqueAngleChange->_changeType = UsdImagingTextProperty::UsdImagingTextPropertyObliqueAngle;
            _currentTextStyle->_obliqueAngle =
                pSkewTransformParticle->SkewX().Radians() * 180.0f / (float)M_PI;
            pObliqueAngleChange->_floatValue = _currentTextStyle->_obliqueAngle;
            pTextRun->AddStyleChange(*pObliqueAngleChange);
            break;
        }
        case CommonParserTransformParticleSemanticsWidth:
        {
            CommonParserScaleTransformParticle* pScaleTransformParticle =
                (CommonParserScaleTransformParticle*)pTransformParticle;

            std::shared_ptr<UsdImagingTextStyleChange> pWidthChange = std::make_shared<UsdImagingTextStyleChange>();
            pWidthChange->_changeType                     = UsdImagingTextProperty::UsdImagingTextPropertyWidthFactor;
            _currentTextStyle->_widthFactor               = pScaleTransformParticle->ScaleX();
            pWidthChange->_floatValue                     = _currentTextStyle->_widthFactor;
            pTextRun->AddStyleChange(*pWidthChange);
            break;
        }
        default:
            break;
        }
        pTransformParticle = pTransformParticle->Next();
    }
    return true;
}
PXR_NAMESPACE_CLOSE_SCOPE
