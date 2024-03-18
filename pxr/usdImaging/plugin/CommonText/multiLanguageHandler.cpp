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

#include "multiLanguageHandler.h"
#include "characterSupport.h"
#include "genericLayout.h"
#include "metrics.h"
#ifdef _WIN32
#include "multiLanguageHandlerImplWin.h"
#endif
#include "portableUtils.h"
#include "simpleLayout.h"

PXR_NAMESPACE_OPEN_SCOPE
/// Acquire the implementation
std::shared_ptr<CommonTextMultiLanguageHandlerImpl> 
CommonTextMultiLanguageHandler::AcquireImplementation()
{
    if (_multiLanguageHandlerImpl != nullptr)
        return _multiLanguageHandlerImpl;

    else
    {
#if defined(WIN32)
        _multiLanguageHandlerImpl = std::make_shared<CommonTextMultiLanguageHandlerImplWin>();
#endif
    }
    return _multiLanguageHandlerImpl;
}

CommonTextStatus 
CommonTextMultiLanguageHandler::AddDefaultFontToFontMapCache()
{
    if (_multiLanguageHandlerImpl == nullptr)
    {
        // Create the multi-language handler
        AcquireImplementation();
    }

    // Initialize the truetype font cache.
    return _multiLanguageHandlerImpl->InitializeTrueTypeCache(_trueTypeFontMapCache);
}

#ifdef _WIN32
CommonTextStatus 
CommonTextMultiLanguageHandler::_PredefinedSubstituteFont(
    wchar_t wch,
    bool isComplex, 
    bool& newTypeFaceFind, 
    std::string& newTypeFace)
{
    // Get the language from the unicode.
    WORD language = _multiLanguageHandlerImpl->LanguageFromUnicode(wch);

    // Get the charset from the language.
    int charset = _multiLanguageHandlerImpl->LanguageToCharSet(language);

    // Find the font which supports this charset.
    const wchar_t* mapFont = _multiLanguageHandlerImpl->MapFontFromCharset(charset);
    if (mapFont != nullptr)
    {
        // Test if this font can support this character.
        CommonTextFontSupportCharacterTest fontTest;
        if (fontTest.Initialize(w2s(mapFont), isComplex) == CommonTextStatus::CommonTextStatusSuccess)
        {
            // Only one character, so we don't care about the first unsupported character.
            std::wstring strChar(1, wch);
            if (fontTest.IsAllSupported(strChar, -1))
            {
                newTypeFaceFind = true;
                newTypeFace     = w2s(mapFont);

                return CommonTextStatus::CommonTextStatusSuccess;
            }
        }
    }

    // found desired "language" font, but it doesn't support character!
    if (_multiLanguageHandlerImpl->CharSetIsDoubleByte(charset))
    {
        // if double-byte, check other DB languages
        // There are five different codepage indices, and each map one DB language
        for (int l = 5; l >= 1; l--)
        {
            // Get the db language of the codepage index.
            short dbLanguage = _multiLanguageHandlerImpl->CodePageIndexToLanguage(l);

            // Get the charset of this codepage index
            int cs = _multiLanguageHandlerImpl->CodePageIndexToCharSet(l);

            // If the charset is the same with the charset which we have already tested
            // we will not go on.
            // And we also test if this language support this character.
            if (cs != charset &&
                _multiLanguageHandlerImpl->LanguageFromUnicode(wch, dbLanguage) == dbLanguage)
            {
                // Find the font which supports this charset.
                mapFont = _multiLanguageHandlerImpl->MapFontFromCharset(cs);
                if (mapFont != nullptr)
                {
                    // Test if this font can support this character.
                    CommonTextFontSupportCharacterTest fontTest;
                    if (fontTest.Initialize(w2s(mapFont), isComplex) ==
                        CommonTextStatus::CommonTextStatusSuccess)
                    {
                        // Only one character, so we don't care about the first unsupported
                        // character.
                        if (fontTest.IsAllSupported(&wch, -1))
                        {
                            newTypeFaceFind = true;
                            newTypeFace     = w2s(mapFont);

                            return CommonTextStatus::CommonTextStatusSuccess;
                        }
                    }
                }
            }
        }
    }

    return CommonTextStatus::CommonTextStatusSuccess;
}
#endif

CommonTextStatus 
CommonTextMultiLanguageHandler::_SystemSubstituteFont(
    const std::wstring unicodeString,
    bool isComplex, 
    const std::string& originalTypeface, 
    long dwCodePages,
    std::shared_ptr<CommonTextFontMapCache> fontMapCache, 
    bool& newTypeFaceFind, 
    std::string& newTypeFace)
{
    // Using in findNewFontFaceFromCache.
    // findNewFontFaceFromCache will change the codepages.
    unsigned long dwCodePagesRemain = dwCodePages;

    bool needSubstitutionFromCache = _allowFromCache;
    bool needSystemSubstitution   = _allowSystem;

    // If dwCodePagesRemain is zero, it means the remain code pages is not valid. In this
    // case, we skip this font substitution process. We will directly try the default font
    // list.
    if (dwCodePagesRemain != 0)
        while (!newTypeFaceFind)
        {
            if (needSubstitutionFromCache)
            {
                // Find in the cache. This is user defined font substitution. It has higher
                // priority. the dwCodePagesRemain may contain many different codepages, and each
                // has a supported font. If we have test one code page in this function, it is
                // removed from dwCodePagesRemain. If the found font can not support all the
                // characters, we can call this function again with the dwCodePagesRemain to test
                // the supported font of the next codepage.
                needSubstitutionFromCache = _FindNewFontFaceFromCache(
                    originalTypeface, newTypeFace, dwCodePagesRemain, fontMapCache);
            }

            if (!needSubstitutionFromCache && needSystemSubstitution)
            {
                // Find the default font.
                needSystemSubstitution =
                    _FindDefaultFontFace(originalTypeface, newTypeFace, dwCodePages);
            }

            if (needSubstitutionFromCache || needSystemSubstitution)
            {
                newTypeFaceFind = true;

                // Test if this font can support this character.
                CommonTextFontSupportCharacterTest fontTest;
                if (fontTest.Initialize(newTypeFace, isComplex) !=
                    CommonTextStatus::CommonTextStatusSuccess)
                    newTypeFaceFind = false;

                // We don't care about the first unsupported character.
                if (!fontTest.IsAllSupported(unicodeString, -1))
                {
                    newTypeFaceFind = false;
                }
                if (!newTypeFaceFind && needSystemSubstitution)
                    break;
            }
            else
                break;
        }

    if (!newTypeFaceFind)
    {
        // If we still don't get the font, try the default font in the list.
        // Iterate all the font in the list, until we find one which can support the
        // characters.
        for (int i = 0; i < _defaultTrueTypeFontList->size(); i++)
        {
            CommonTextFontSupportCharacterTest fontTest;
            if (fontTest.Initialize(_defaultTrueTypeFontList->at(i),
                    isComplex) != CommonTextStatus::CommonTextStatusSuccess)
                continue;

            // We don't care about the first unsupported character.
            if (fontTest.IsAllSupported(unicodeString, -1))
            {
                newTypeFace     = _defaultTrueTypeFontList->at(i);
                newTypeFaceFind = true;
                break;
            }
        }
    }
    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextStatus 
CommonTextMultiLanguageHandler::SubstituteFont(
    std::shared_ptr<UsdImagingMarkupText> markupText,
    std::shared_ptr<CommonTextIntermediateInfo> intermediateInfo,
    UsdImagingTextRunList::iterator textRunIter,
    const UsdImagingTextStyle& runTextStyle,
    UsdImagingTextLineList::iterator textLineIter,
    const CommonTextSimpleLayout& simpleLayout,
    UsdImagingTextRunList::iterator& lastSubRunIter)
{
    if (textRunIter->Length() == 0)
    {
        return CommonTextStatus::CommonTextStatusFail;
    }

    if (_multiLanguageHandlerImpl == nullptr)
    {
        // Create the multi-language handler
        AcquireImplementation();
    }

    std::wstring characters(markupText->MarkupString(), textRunIter->StartIndex(), textRunIter->Length());
    int textRunLength = textRunIter->Length();

    CommonTextRunInfo& textRunInfo = intermediateInfo->GetTextRunInfo(textRunIter);

    CommonTextFontSubstitutionSetting fontSubstitutionSetting =
        CommonTextSystem::Instance()->GetFontSubstitutionSetting();
    _allowFromCache = fontSubstitutionSetting.TestSetting(
        (int)CommonTextFontSubstitutionSettingFlag::CommonTextEnableUserDefinedFontSubstitution);
    _allowSystem = fontSubstitutionSetting.TestSetting(
        (int)CommonTextFontSubstitutionSettingFlag::CommonTextEnableSystemFontSubstitution);
    _allowPredefined = fontSubstitutionSetting.TestSetting(
        (int)CommonTextFontSubstitutionSettingFlag::CommonTextEnablePredefinedFontSubstitution);

    // Whether the first glyph is missing.
    // If missing glyph is true, it means we are finding the end of a string of characters which are
    // not supported by the font. If missing glyph is false, it means we are finding the end of a
    // string of characters which are supported by the font.
    bool missingGlyph =
        !simpleLayout.TestMetricsInfoAvailability(CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityIndicesAvailable) ||
        !simpleLayout.IsGlyphIndexValidAt(0);

    CommonTextStatus substitutionStatus = CommonTextStatus::CommonTextStatusSuccess;
    CommonTextFontSupportCharacterTest fontTest;

    // The function that create sub textrun for the part that the characters can not be displayed,
    // and find a font which can display the characters.
    // startOffset is the index of the first character that has missing glyph, and sublength is the
    // length of the textrun that needs font substitution.
    auto substituteFontForSubRun = [&](int& startOffset, int& subLength, std::vector<int>& dividePos,
        std::vector<std::shared_ptr<UsdImagingTextStyleChange>>& styleChangeList) {
        // If the indices are invalid or missing, we will do font substitution.
        std::string newTypeFace;
        bool newTypeFaceFind = false;

        while (subLength > 0)
        {
            const wchar_t* start         = characters.c_str() + startOffset;
            int startOffsetBeforeProcess = startOffset;
            newTypeFaceFind              = false;
#ifdef _WIN32
            // First, we will try to do the font substitution using predefined character to 
            // charset mapping.
            if (_allowPredefined)
            {
                substitutionStatus = _PredefinedSubstituteFont(*start,
                    textRunInfo.ComplexScriptInformation() != nullptr, newTypeFaceFind, newTypeFace);

                if (substitutionStatus != CommonTextStatus::CommonTextStatusSuccess &&
                    substitutionStatus != CommonTextStatus::CommonTextStatusNeedSubstitution)
                {
                    return substitutionStatus;
                }
                // If the typeface is the same with the original typeface, the substitution
                // fails.
                if (newTypeFaceFind && newTypeFace == runTextStyle._typeface)
                    newTypeFaceFind = false;

                if (newTypeFaceFind)
                {
                    fontTest.Initialize(newTypeFace,
                        textRunInfo.ComplexScriptInformation() != nullptr);
                }

                if (newTypeFaceFind)
                {
                    // Test if this font can support all the characters in the sub-textrun.
                    int firstUnsupported = 0;
                    if (!fontTest.IsAllSupported(start, firstUnsupported))
                    {
                        // If this is not the last textrun, add the position after the textrun
                        // into the divide position list.
                        if ((startOffset + firstUnsupported) != textRunLength)
                            dividePos.push_back(startOffset + firstUnsupported);
                        // Reset startOffset, and in the next iteration we will handle we will
                        // handle the remaining part.
                        startOffset = startOffset + firstUnsupported;
                        subLength -= firstUnsupported;
                    }
                    else
                    {
                        // If this is not the last textrun, add the position after the textrun
                        // into the divide position list.
                        if ((startOffset + firstUnsupported) != textRunLength)
                            dividePos.push_back(startOffset + subLength);
                        startOffset = startOffset + subLength;
                        subLength   = 0;
                    }
                    // Add typeface change into the styleChangeList.
                    std::shared_ptr<UsdImagingTextStyleChange> typefaceChange =
                        std::make_shared<UsdImagingTextStyleChange>();
                    typefaceChange->_changeType  = UsdImagingTextProperty::UsdImagingTextPropertyTypeface;
                    typefaceChange->_stringValue = std::make_shared<std::string>(newTypeFace);
                    styleChangeList.push_back(typefaceChange);
                }
            }
#endif
            if (!newTypeFaceFind && (_allowFromCache || _allowSystem))
            {
                // First we will do font substitution using the system defined codepage.
                unsigned long dwCodePages = 0;
                long cchCodePages         = 0;

                // Get the codepages for these characters. If this fails, we will not do font
                // substitution.
                if ((_multiLanguageHandlerImpl->GetStringCodePages(
                         (wchar_t*)start, subLength, dwCodePages, cchCodePages) == -1) ||
                    (cchCodePages == 0))
                {
                    break;
                }

                // If this is not the last textrun, add the position after the textrun
                // into the divide position list.
                if ((startOffset + cchCodePages) != textRunLength)
                    dividePos.push_back(startOffset + cchCodePages);
                // Reset startOffset, and in the next iteration we will handle the 
                // remaining part.
                startOffset = startOffset + cchCodePages;
                subLength -= cchCodePages;

                // Get the FontMapCacheIterator.
                std::wstring subString(start, cchCodePages);
                substitutionStatus = _SystemSubstituteFont(subString,
                    textRunInfo.ComplexScriptInformation() != nullptr, runTextStyle._typeface,
                    dwCodePages, _trueTypeFontMapCache, newTypeFaceFind, newTypeFace);
                if (substitutionStatus != CommonTextStatus::CommonTextStatusSuccess &&
                    substitutionStatus != CommonTextStatus::CommonTextStatusNeedSubstitution)
                {
                    return substitutionStatus;
                }
                if (newTypeFaceFind)
                {
                    // Add typeface change into the styleChangeList.
                    std::shared_ptr<UsdImagingTextStyleChange> typefaceChange =
                        std::make_shared<UsdImagingTextStyleChange>();
                    typefaceChange->_changeType  = UsdImagingTextProperty::UsdImagingTextPropertyTypeface;
                    typefaceChange->_stringValue = std::make_shared<std::string>(newTypeFace);
                    styleChangeList.push_back(typefaceChange);
                }
            }
            // If startOffset doesn't change, it means no substitution happens. We wills stop.
            if (startOffset == startOffsetBeforeProcess)
                break;
        }
        if (subLength > 0)
        {
            // If this is not the last textrun, add the position after the textrun
            // into the divide position list.
            if ((startOffset + subLength) != textRunLength)
                dividePos.push_back(subLength);
            // Add an empty styleChange.
            styleChangeList.push_back(nullptr);
            subLength = 0;
        }
        return CommonTextStatus::CommonTextStatusSuccess;
    };

    // The function to create a sub textrun and then call substituteFontForSubRun if the glyphs
    // in the subrun is missing.
    auto createSubTextRunAndSubstitute = [&](int& startOffset, int length, std::vector<int>& dividePos,
        std::vector<std::shared_ptr<UsdImagingTextStyleChange>>& styleChangeList) {
        if (missingGlyph)
        {
            // If the glyph of this part of text is missing, we will do font substitution for
            // this part.
            substituteFontForSubRun(startOffset, length, dividePos, styleChangeList);
            if (length > 0)
                return CommonTextStatus::CommonTextStatusFail;
        }
        else
        {
            // If this is not the last textrun, add the position after the textrun
            // into the divide position list.
            if((startOffset + length) != textRunLength)
                dividePos.push_back(startOffset + length);
            // Add an empty styleChange.
            styleChangeList.push_back(nullptr);
        }
        return CommonTextStatus::CommonTextStatusSuccess;
    };

    int lastDivideIndex = 0;
    std::vector<int> dividePos;
    std::vector<std::shared_ptr<UsdImagingTextStyleChange>> styleChangeList;
    // If the indices is not available, it means the whole text run is not supported by the current
    // font. In this case we don't need to check if we will divide the run into sub-textrun.
    if (simpleLayout.TestMetricsInfoAvailability(CommonTextMetricsInfoAvailability::CommonTextMetricsInfoAvailabilityIndicesAvailable))
    {
        int glyphCount = textRunIter->Length();
        std::shared_ptr<CommonTextComplexScriptMetrics> complexMetrics =
            simpleLayout.GetComplexScriptMetrics();
        if (complexMetrics != nullptr)
            glyphCount = complexMetrics->GlyphCount();
        for (int i = 1; i < glyphCount; i++)
        {
            if ((!simpleLayout.IsGlyphIndexValidAt(i)) ^ missingGlyph)
            {
                // Whether the glyph is missing is changed, so we should divide here.
                // We need to find the index of the first character of the cluster. Iterate from
                // the character of the lastDivideIndex, until we find the character whose glyph is
                // the same as the current glyph.
                int divideIndex = lastDivideIndex;
                if (complexMetrics != nullptr)
                {
                    while (complexMetrics->CharacterToGlyphMap()[divideIndex] < i)
                    {
                        assert(divideIndex < i);
                        divideIndex++;
                    }
                }
                else
                    divideIndex = i;

                CommonTextStatus status =
                    createSubTextRunAndSubstitute(lastDivideIndex, divideIndex - lastDivideIndex,
                        dividePos, styleChangeList);
                if (status != CommonTextStatus::CommonTextStatusSuccess)
                    return status;

                // Flip missingGlyph.
                missingGlyph    = !missingGlyph;
                lastDivideIndex = divideIndex;
            }
        }
    }
    // The remain characters at the end are cloned.
    CommonTextStatus status =
        createSubTextRunAndSubstitute(lastDivideIndex, textRunIter->Length() - lastDivideIndex,
            dividePos, styleChangeList);
    if (status != CommonTextStatus::CommonTextStatusSuccess)
        return status;

    status = CommonTextTrueTypeGenericLayoutManager::DivideTextRun(
        markupText, intermediateInfo, textRunIter, dividePos, textLineIter, lastSubRunIter);
    if (status != CommonTextStatus::CommonTextStatusSuccess)
        return status;

    UsdImagingTextRunList::iterator iter = textRunIter;
    for (auto styleChangePtr : styleChangeList)
    {
        // If the styleChange is not empty, add it to the textRun.
        if (styleChangePtr)
            iter->AddStyleChange(*styleChangePtr);
        ++iter;
    }
    // No matter whether we find the supported font or not, return the substitution text run
    // list.
    return CommonTextStatus::CommonTextStatusSuccess;
}

bool 
CommonTextMultiLanguageHandler::_FindNewFontFaceFromCache(
    const std::string& originalTypeface,
    std::string& newTypeFace, 
    unsigned long& dwCodePages,
    std::shared_ptr<CommonTextFontMapCache> fontMapCache)
{
    std::shared_ptr<CommonTextStringArray> pTypefaceArray = nullptr;

    std::string* typeface                      = nullptr;
    std::shared_ptr<CommonTextStringArray> pCurrentArray = nullptr;
    int currentIndex                           = 0;

    // If typeface == nullptr, it means the iterator is not initialized
    // or the iterator points to the end.
    // We need to reinitialize the iterator.
    while (typeface == nullptr || typeface->empty() || originalTypeface == *typeface)
    {
        if (typeface == nullptr)
        {
            if (dwCodePages == 0)
                break;

            // Get a codepage from the codepages.
            unsigned int uCodePage;
            if (_multiLanguageHandlerImpl->CodepagesToCodepage(dwCodePages, uCodePage) == -1)
                return false;

            // Initialize the iterator with the codepage.
            auto iter = fontMapCache->find(uCodePage);
            if (iter != fontMapCache->end())
            {
                pTypefaceArray = iter->second;
            }

            if (pTypefaceArray == nullptr)
                continue;
            else
            {
                pCurrentArray = pTypefaceArray;
            }
        }

        // Move to the next typeface and return it.
        if (pCurrentArray == nullptr || pCurrentArray->size() <= currentIndex)
            typeface = nullptr;

        typeface = &(pCurrentArray->at(currentIndex));
        currentIndex++;
    }
    if (typeface != nullptr)
    {
        newTypeFace = *typeface;
        return true;
    }
    else
        return false;
}

bool 
CommonTextMultiLanguageHandler::_FindDefaultFontFace(
    const std::string& originalTypeface, 
    std::string& newTypeFace, 
    long dwCodePages)
{
    // Find the default system font for the codepages.
    std::wstring wTypeFace;
    if (_multiLanguageHandlerImpl->DefaultFontFromCodepages(dwCodePages, wTypeFace) !=
        CommonTextStatus::CommonTextStatusSuccess)
        return false;

    newTypeFace = w2s(wTypeFace);

    return !newTypeFace.empty() && originalTypeface != newTypeFace;
}

std::shared_ptr<CommonTextFontMapCache> 
CommonTextMultiLanguageHandler::GetFontMapCache()
{
    return _trueTypeFontMapCache;
}

bool 
CommonTextMultiLanguageHandler::RequireComplexScriptHandling(
    const wchar_t* start, 
    int length)
{
    if (_multiLanguageHandlerImpl == nullptr)
    {
        // Create the multi-language handler
        AcquireImplementation();
    }
    // If the multilanguage handler doesn't support complex script handling, just return
    // false to indicate no complex handling is needed.
    if (!_multiLanguageHandlerImpl->SupportComplexScriptHandling())
        return false;
    return _multiLanguageHandlerImpl->RequireComplexScriptHandling(start, length);
}

CommonTextStatus 
CommonTextMultiLanguageHandler::DivideStringByScripts(
    std::shared_ptr<UsdImagingMarkupText> markupText,
    std::shared_ptr<CommonTextIntermediateInfo> intermediateInfo,
    UsdImagingTextRunList::iterator textRunIter,
    UsdImagingTextLineList::iterator textLineIter,
    UsdImagingTextRunList::iterator& lastSubRunIter)
{
    if (textRunIter->Length() == 0)
    {
        return CommonTextStatus::CommonTextStatusFail;
    }

    lastSubRunIter = textRunIter;
    // Get the language handler.
    if (_multiLanguageHandlerImpl == nullptr)
    {
        // Create the multi-language handler
        AcquireImplementation();
    }
    // If the multilanguage handler doesn't support complex script handling, just return
    // success.
    if (!_multiLanguageHandlerImpl->SupportComplexScriptHandling())
        return CommonTextStatus::CommonTextStatusSuccess;

    CommonTextRunInfo& textRunInfo = intermediateInfo->GetTextRunInfo(textRunIter);
    const wchar_t* pStart = markupText->MarkupString().c_str() + textRunIter->StartIndex();
    int length = textRunIter->Length();
    bool containComplex = RequireComplexScriptHandling(pStart, length);

    int sizeOfPlatformAttribute = _multiLanguageHandlerImpl->SizeOfScriptAttribute();
    CommonTextStringsScriptAttribute scriptAttribute(sizeOfPlatformAttribute);

    // Use the platform specified multilanguage handler to break the string into scripts.
    // And also get the indicies of characters who is the first of each new script.
    if (!_multiLanguageHandlerImpl->ScriptsBreakString(
            (wchar_t*)pStart, length, containComplex, textRunInfo.GetScriptInfo(), scriptAttribute))
    {
        return CommonTextStatus::CommonTextStatusFail;
    }

    // Do text run break if there is complex script.
    if (containComplex)
    {
        if (scriptAttribute._countOfSubStrings > 1)
        {
            std::vector<int> dividePos;
            int lengthFromStart = 0;
            for (int i = 0; i < scriptAttribute._countOfSubStrings - 1; i++)
            {
                lengthFromStart += scriptAttribute._subStringLength[i];
                dividePos.push_back(lengthFromStart);
            }
            CommonTextStatus status = CommonTextTrueTypeGenericLayoutManager::DivideTextRun(
                markupText, intermediateInfo, textRunIter, dividePos, textLineIter, lastSubRunIter);
            if (status != CommonTextStatus::CommonTextStatusSuccess)
                return status;
        }
        // Handle the first sub string. The information is directly set to the original text run.
        if (scriptAttribute._subStringIsComplex[0])
        {
            std::shared_ptr<CommonTextComplexScriptInfo> complexScriptInfo =
                std::make_shared<CommonTextComplexScriptInfo>();
            char* attribute = new char[scriptAttribute._sizeOfSingleScriptAttribute];
            memcpy(attribute,
                scriptAttribute._scriptAttributeForStrings,
                scriptAttribute._sizeOfSingleScriptAttribute);
            complexScriptInfo->Attributes(scriptAttribute._sizeOfSingleScriptAttribute, attribute);
            textRunInfo.ComplexScriptInformation(complexScriptInfo);
        }
        UsdImagingTextRunList::iterator subTextRunIter = textRunIter;
        ++subTextRunIter;
        for (int i = 1; i < scriptAttribute._countOfSubStrings; i++)
        {
            // Create the complex information for the sub-textrun and set it to the sub textrun.
            if (scriptAttribute._subStringIsComplex[i])
            {
                std::shared_ptr<CommonTextComplexScriptInfo> complexScriptInfo =
                    std::make_shared<CommonTextComplexScriptInfo>();
                char* attribute = new char[scriptAttribute._sizeOfSingleScriptAttribute];
                // The ith ScriptAttribute starts at _sizeOfSingleScriptAttribute * i.
                int copyStartIndex = scriptAttribute._sizeOfSingleScriptAttribute * i;
                void* scriptAttributeForStrings =
                    static_cast<char*>(scriptAttribute._scriptAttributeForStrings) + copyStartIndex;
                memcpy(attribute,
                    scriptAttributeForStrings, scriptAttribute._sizeOfSingleScriptAttribute);
                complexScriptInfo->Attributes(
                    scriptAttribute._sizeOfSingleScriptAttribute, attribute);
                CommonTextRunInfo& subTextRunInfo = intermediateInfo->GetTextRunInfo(subTextRunIter);
                subTextRunInfo.ComplexScriptInformation(complexScriptInfo);
            }
            ++subTextRunIter;
        }
    }

    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextStatus 
CommonTextMultiLanguageHandler::AcquireComplexIndices(
    std::wstring unicodeString,
    CommonTextSimpleLayout& pSimpleLayout, 
    const UsdImagingTextStyle& textStyle, 
    bool& isAllSupported,
    std::vector<unsigned short>& indices, 
    std::shared_ptr<CommonTextComplexScriptInfo> pComplexScriptInfo)
{
    if (unicodeString.length() == 0)
    {
        return CommonTextStatus::CommonTextStatusFail;
    }

    // The text run must contain complex information. Or else we don't use this method.
    if (pComplexScriptInfo == nullptr)
        return CommonTextStatus::CommonTextStatusFail;
    std::shared_ptr<CommonTextComplexScriptMetrics> complexMetrics = pSimpleLayout.GetComplexScriptMetrics();

    if (complexMetrics == nullptr)
    {
        complexMetrics = std::make_shared<CommonTextComplexScriptMetrics>();
        pSimpleLayout.SetComplexScriptMetrics(complexMetrics);
    }

    // Get the language handler.
    if (_multiLanguageHandlerImpl == nullptr)
    {
        // Create the multi-language handler
        AcquireImplementation();
    }
    // If the multilanguage handler doesn't support complex script handling, just return
    // fail.
    if (!_multiLanguageHandlerImpl->SupportComplexScriptHandling())
        return CommonTextStatus::CommonTextStatusFail;

    const wchar_t* pStart = unicodeString.c_str();
    int length = (int)unicodeString.length();

    // Initialize the pClustersAttribute.
    int sizeOfClustersAttribute = _multiLanguageHandlerImpl->SizeOfClusterAttribute();
    CommonTextClustersScriptAttribute clustersAttribute(sizeOfClustersAttribute);

    // Use the platform specified multilanguage handler to get the glyph indices.
    if (!_multiLanguageHandlerImpl->ScriptsGetGlyphIndices(textStyle, (wchar_t*)pStart, length,
            pComplexScriptInfo->Attributes(), isAllSupported, clustersAttribute))
    {
        return CommonTextStatus::CommonTextStatusFail;
    }

    // Set the glyph count.
    complexMetrics->GlyphCount(clustersAttribute._countOfGlyphs);
    // Set the cluster count.
    complexMetrics->ClusterCount(clustersAttribute._countOfClusters);

    // If the multilanguage handler can not generate the index of any glyph, it means the font
    // doesn't support the script. We will return.
    if (clustersAttribute._countOfGlyphs == 0)
        return CommonTextStatus::CommonTextStatusCharacterNotFound;

    // Copy the map between character and glyph from cluster attributes to the complex
    // information.
    std::vector<short>& characterToGlyphMap = complexMetrics->CharacterToGlyphMap();
    characterToGlyphMap.resize(length);

    memcpy(characterToGlyphMap.data(),
        clustersAttribute._characterToGlyphMap, sizeof(short) * length);

    // Copy the map between character and cluster from cluster attributes to the complex
    // information.
    std::vector<short>& characterToClusterMap = complexMetrics->CharacterToClusterMap();
    characterToClusterMap.resize(length);
    memcpy(characterToClusterMap.data(),
        clustersAttribute._characterToClusterMap, sizeof(short) * length);
    
    // Set the cluster attribute.
    char* clusterAttributes = new char[clustersAttribute._sizeOfSingleClusterAttribute *
        clustersAttribute._countOfGlyphs];
    memcpy(clusterAttributes,
        clustersAttribute._clustersAttribute,
        clustersAttribute._sizeOfSingleClusterAttribute * clustersAttribute._countOfGlyphs);
    complexMetrics->ClusterAttributes(
        clustersAttribute._sizeOfSingleClusterAttribute * clustersAttribute._countOfGlyphs,
        clusterAttributes);

    // Set the indices of the glyphs.
    indices.resize(clustersAttribute._countOfGlyphs);

    memcpy(indices.data(), clustersAttribute._indices,
        sizeof(short) * clustersAttribute._countOfGlyphs);
    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextStatus 
CommonTextMultiLanguageHandler::IsAllCharactersSupported(
    const UsdImagingTextStyle& style,
    std::wstring unicodeString, 
    bool& isAllSupported, 
    unsigned short* indices)
{
    if (_multiLanguageHandlerImpl == nullptr)
    {
        // Create the multi-language handler
        AcquireImplementation();
    }
    // If the multilanguage handler doesn't support complex script handling, just return
    // fail.
    if (!_multiLanguageHandlerImpl->SupportComplexScriptHandling())
        return CommonTextStatus::CommonTextStatusFail;

    if (_multiLanguageHandlerImpl->ScriptIfAllCharactersAreSupported(
            style, unicodeString.c_str(), (int)unicodeString.length(), isAllSupported, indices))
        return CommonTextStatus::CommonTextStatusSuccess;
    else
        return CommonTextStatus::CommonTextStatusFail;
}
PXR_NAMESPACE_CLOSE_SCOPE
