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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_MULTILANGUAGE_HANDLER_IMPL_WIN_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_MULTILANGUAGE_HANDLER_IMPL_WIN_H

#include "definitions.h"
#include "multiLanguageHandlerImpl.h"
#include "portableUtils.h"
#include <MLang.h>
#include <usp10.h>
#include <windows.h>

PXR_NAMESPACE_OPEN_SCOPE
/// \class CommonTextMultiLanguageHandlerImplWin
///
/// The windows implementation of the Multi-language Handler
/// We use Uniscribe and Mlang to implement the multi-language handling.
///
class CommonTextMultiLanguageHandlerImplWin : public CommonTextMultiLanguageHandlerImpl
{
private:
    static BOOL CALLBACK EnumCodePagesProc(wchar_t*);

    static HDC _mhDC;

    static IMultiLanguage2* _multiLang;
    static IMLangFontLink2* _fontLink;
    static IMLangCodePages* _codePages;

    static const SCRIPT_PROPERTIES** _scriptProperties;
    static int _maxScript;

    static std::unordered_map<std::wstring, SCRIPT_CACHE> _scriptCaches;

    static std::unordered_map<std::wstring, SCRIPT_FONTPROPERTIES*> _scriptFontProperties;

    static std::shared_ptr<CommonTextFontMapCache> _trueTypeFontMapCache;

public:
    /// Constructor
    CommonTextMultiLanguageHandlerImplWin() { Initialize(); }

    /// Destructor
    ~CommonTextMultiLanguageHandlerImplWin() {}

    /// initialize the Mlang and Uniscribe resource.
    static CommonTextStatus Initialize();

    /// release the Mlang and Uniscribe resource.
    static void Release();

    /// Implement CommonTextMultiLanguageHandlerImpl::InitializeTrueTypeCache;
    CommonTextStatus InitializeTrueTypeCache(std::shared_ptr<CommonTextFontMapCache>) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::GetStringCodePages;
    int GetStringCodePages(const wchar_t* start,
                           int length,
                           unsigned long& codepages,
                           long& count, 
                           long priorityCodepages = 0) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CodepagesToCodepage;
    int CodepagesToCodepage(unsigned long& codepages, 
                            unsigned int& uCodePage, 
                            int defaultCodepage = 0) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::DefaultFontFromCodepages;
    CommonTextStatus DefaultFontFromCodepages(long codepages, 
                                              std::wstring& fontTypeface) override;

    //======================codepage and charset conversion====================
    /// Implement CommonTextMultiLanguageHandlerImpl::CharSetToCodePage.
    int CharSetToCodePage(int charSet) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CharSetToCodePageIndex.
    int CharSetToCodePageIndex(int charSet) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CharSetToCodePageId.
    int CharSetToCodePageId(int charSet) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CharSetToLanguage.
    short CharSetToLanguage(int charSet) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CharSetIsDoubleByte.
    bool CharSetIsDoubleByte(int charSet) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CodePageToCharSet.
    int CodePageToCharSet(int codePage) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CodePageToCodePageIndex.
    int CodePageToCodePageIndex(int codePage) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CodePageToCodePageId.
    int CodePageToCodePageId(int codePage) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CodePageToLanguage.
    short CodePageToLanguage(int codePage) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CodePageIsDoubleByte.
    bool CodePageIsDoubleByte(int codePage) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CodePageIdToCharSet.
    int CodePageIdToCharSet(int codePageId) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CodePageIdToCodePage.
    int CodePageIdToCodePage(int codePageId) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CodePageIdToCodePageIndex.
    int CodePageIdToCodePageIndex(int codePageId) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CodePageIdToLanguage.
    short CodePageIdToLanguage(int codePageId) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CodePageIdIsDoubleByte.
    bool CodePageIdIsDoubleByte(int codePageId) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CodePageIndexToCharSet.
    int CodePageIndexToCharSet(int codePageIndex) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CodePageIndexToCodePage.
    int CodePageIndexToCodePage(int codePageIndex) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CodePageIndexToCodePageId.
    int CodePageIndexToCodePageId(int codePageIndex) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CodePageIndexToLanguage.
    short CodePageIndexToLanguage(int codePageIndex) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CodePageIndexIsDoubleByte.
    bool CodePageIndexIsDoubleByte(int codePageIndex) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::LanguageToCharSet.
    int LanguageToCharSet(short language) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::LanguageToCodePage.
    int LanguageToCodePage(short language) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::LanguageToCodePageIndex.
    int LanguageToCodePageIndex(short language) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::LanguageToCodePageId.
    int LanguageToCodePageId(short language) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::LanguageIsDoubleByte.
    bool LanguageIsDoubleByte(short language) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::LanguageIsRtoL.
    bool LanguageIsRtoL(short language) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::LanguageFromUnicode.
    short LanguageFromUnicode(wchar_t wch, 
                              short def_lang = 0) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::IsLeadByte.
    bool IsLeadByte(int codePage, 
                    char c) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::IsLeadByte.
    bool IsLeadByte(char c) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CodePageCount.
    int CodePageCount() override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CodePageEntry.
    int CodePageEntry(int i) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::UnicodeForUserDefinedDBCS.
    bool UnicodeForUserDefinedDBCS(wchar_t& unicodeValue, 
                                   wchar_t DBCSCode, 
                                   unsigned int Codepage) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::DBCSForUserDefinedUnicode.
    bool DBCSForUserDefinedUnicode(wchar_t& dbcs, 
                                   wchar_t unicodeValue) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::CodepageForUserDefinedUnicode.
    bool CodepageForUserDefinedUnicode(int& codePage, 
                                       wchar_t unicodeValue) override;

    /// Get the line break class of the character.
    int LineBreakClass(int c) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::MapFontFromCharset.
    const wchar_t* MapFontFromCharset(int charset) override;

    //======================Complex script handling====================
    /// Implement CommonTextMultiLanguageHandlerImpl::SupportComplexScriptHandling
    bool SupportComplexScriptHandling() const override { return true; }

    /// Implement CommonTextMultiLanguageHandlerImpl::RequireComplexScriptHandling
    bool RequireComplexScriptHandling(const wchar_t* start, 
                                      int length) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::SizeOfPlatformAttribute.
    int SizeOfScriptAttribute() override;

    /// Implement CommonTextMultiLanguageHandlerImpl::SizeOfClusterAttribute.
    int SizeOfClusterAttribute() override;

    /// Implement CommonTextMultiLanguageHandlerImpl::ScriptsBreakString.
    bool ScriptsBreakString(const wchar_t* start,
                            int length,
                            bool containsComplex,
                            std::vector<CommonTextScriptInfo>& scriptInfoArray,
                            CommonTextStringsScriptAttribute& attributeOfEachSubString) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::ScriptsGetGlyphIndices.
    bool ScriptsGetGlyphIndices(const UsdImagingTextStyle& style,
                                const wchar_t* start,
                                int length,
                                void* attributeOfString,
                                bool& isAllSupport,
                                CommonTextClustersScriptAttribute& clustersAttribute) override;

    /// Implement CommonTextMultiLanguageHandlerImpl::ScriptIfAllCharactersAreSupported.
    bool ScriptIfAllCharactersAreSupported(const UsdImagingTextStyle& style,
                                           const wchar_t* start,
                                           int length,
                                           bool& IsAllSupported,
                                           unsigned short* indices = nullptr) override;

protected:
    /// Find the default TrueType font and SHXFont support the codepage, and add to the cache.
    static CommonTextStatus _AddCodePageAndDefaultFont(int codepage);

    /// initialize the array of ScriptProperties.
    static void _InitializeScriptProperties();

    /// Get a script cache from the key.
    SCRIPT_CACHE* _AcquireScriptCache(std::wstring key);

    /// Get a script cache from the key.
    SCRIPT_FONTPROPERTIES* _AcquireScriptFontProperties(std::wstring key);

    /// Get a script cache from the key.
    std::wstring _GetUStringFromStyle(const UsdImagingTextStyle& style)
    {
        return s2w(style._typeface + std::to_string(style._height));
    }
};
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_MULTILANGUAGE_HANDLER_IMPL_WIN_H
