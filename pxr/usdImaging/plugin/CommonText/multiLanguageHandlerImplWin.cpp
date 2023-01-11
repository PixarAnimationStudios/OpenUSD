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

#include "multiLanguageHandlerImplWin.h"
#include "codePage.h"

PXR_NAMESPACE_OPEN_SCOPE
// Enumerate the codepages in the system,
BOOL 
CALLBACK CommonTextMultiLanguageHandlerImplWin::EnumCodePagesProc(wchar_t* lpCodePageString)
{
    int cp = ::_wtoi(lpCodePageString);
    if (errno == ERANGE)
    {
        return FALSE;
    }
    if (CommonTextMultiLanguageHandlerImplWin::_AddCodePageAndDefaultFont(cp) != CommonTextStatus::CommonTextStatusSuccess)
        return FALSE;

    return TRUE;
}

IMultiLanguage2* CommonTextMultiLanguageHandlerImplWin::_multiLang = nullptr;
IMLangFontLink2* CommonTextMultiLanguageHandlerImplWin::_fontLink  = nullptr;
IMLangCodePages* CommonTextMultiLanguageHandlerImplWin::_codePages = nullptr;

HDC CommonTextMultiLanguageHandlerImplWin::_mhDC = nullptr;

std::shared_ptr<CommonTextFontMapCache> CommonTextMultiLanguageHandlerImplWin::_trueTypeFontMapCache;

const SCRIPT_PROPERTIES** CommonTextMultiLanguageHandlerImplWin::_scriptProperties = nullptr;
int CommonTextMultiLanguageHandlerImplWin::_maxScript                              = 0;

std::unordered_map<std::wstring, SCRIPT_CACHE> CommonTextMultiLanguageHandlerImplWin::_scriptCaches;

std::unordered_map<std::wstring, SCRIPT_FONTPROPERTIES*>
    CommonTextMultiLanguageHandlerImplWin::_scriptFontProperties;

void 
CommonTextMultiLanguageHandlerImplWin::Release()
{
    if (_fontLink != nullptr)
    {
        _fontLink->ResetFontMapping();
        _fontLink->Release();
    }
    _fontLink = nullptr;

    if (_codePages != nullptr)
        _codePages->Release();
    _codePages = nullptr;

    if (_multiLang != nullptr)
        _multiLang->Release();
    _multiLang = nullptr;

    CoUninitialize();

    if (_mhDC != nullptr)
    {
        DeleteDC(_mhDC);
        _mhDC = nullptr;
    }

    int propertiesCount = (int)_scriptFontProperties.size();
    if (propertiesCount > 0)
    {
        for (const auto& iter : _scriptFontProperties)
        {
            const SCRIPT_FONTPROPERTIES* pProperties = iter.second;
            if (pProperties != nullptr)
            {
                delete pProperties;
            }
        }
    }
}

CommonTextStatus 
CommonTextMultiLanguageHandlerImplWin::Initialize()
{
    // Initialize the COM object and the MLang interface we used.
    if (_multiLang == nullptr)
    {
        HRESULT hr = CoInitialize(nullptr);
        if (hr != S_OK && hr != S_FALSE)
            return CommonTextStatus::CommonTextStatusFail;

        hr = CoCreateInstance(
            CLSID_CMultiLanguage, nullptr, CLSCTX_ALL, IID_IMultiLanguage, (void**)&_multiLang);
        if (hr != S_OK)
            return CommonTextStatus::CommonTextStatusFail;

        hr = _multiLang->QueryInterface(IID_IMLangFontLink2, (void**)&_fontLink);
        if (hr != S_OK)
            return CommonTextStatus::CommonTextStatusFail;

        hr = _multiLang->QueryInterface(IID_IMLangCodePages, (void**)&_codePages);
        if (hr != S_OK)
            return CommonTextStatus::CommonTextStatusFail;
    }

    // Create the DC.
    if (_mhDC == nullptr)
        _mhDC = CreateCompatibleDC(nullptr);

    // Initialize Script properties.
    if (_scriptProperties == nullptr)
        _InitializeScriptProperties();

    return CommonTextStatus::CommonTextStatusSuccess;
}

void 
CommonTextMultiLanguageHandlerImplWin::_InitializeScriptProperties()
{
    HRESULT hr = ScriptGetProperties(&_scriptProperties, &_maxScript);
    if (FAILED(hr))
    {
        _maxScript        = 0;
        _scriptProperties = nullptr;
    }
}

CommonTextStatus 
CommonTextMultiLanguageHandlerImplWin::InitializeTrueTypeCache(
    std::shared_ptr<CommonTextFontMapCache> pTrueTypeCache)
{
    if (_multiLang == nullptr || _mhDC == nullptr)
        Initialize();

    _trueTypeFontMapCache = pTrueTypeCache;

    // Enumerate the codepages in the system, and initialize the cache.
    if (EnumSystemCodePages(EnumCodePagesProc, CP_SUPPORTED) == FALSE)
    {
        return CommonTextStatus::CommonTextStatusFail;
    }
    return CommonTextStatus::CommonTextStatusSuccess;
}

CommonTextStatus 
CommonTextMultiLanguageHandlerImplWin::_AddCodePageAndDefaultFont(int codepage)
{
    assert(_codePages != nullptr || _fontLink != nullptr);

    // We must convert the codepage to a codepages.
    // Because MapFont only receives codepages.
    DWORD codepages;
    HRESULT hr = _codePages->CodePageToCodePages(codepage, &codepages);
    if (hr != S_OK)
        return CommonTextStatus::CommonTextStatusFail;

    // Get the TrueType font supported by the codepages.
    HFONT ttFont;
    hr = _fontLink->MapFont(_mhDC, codepages, wchar_t(0), &ttFont);
    if (hr != S_OK)
        return CommonTextStatus::CommonTextStatusFail;

    HFONT oldfont = (HFONT)SelectObject(_mhDC, ttFont);

    wchar_t ttFontFace[256];
    ttFontFace[0] = '\0';
    GetTextFace(_mhDC, 256, ttFontFace);

    SelectObject(_mhDC, oldfont);

    // Add the supported true type font.
    std::shared_ptr<CommonTextStringArray> typefaceArray = _trueTypeFontMapCache->at(codepage);
    typefaceArray->push_back(w2s(ttFontFace));
    return CommonTextStatus::CommonTextStatusSuccess;
}

int 
CommonTextMultiLanguageHandlerImplWin::GetStringCodePages(
    const wchar_t* start,
    int length,
    unsigned long& codepages,
    long& count,
    long priorityCodepages)
{
    if (_codePages == nullptr)
        Initialize();

    // Use IMLangCodePages to get the codepages which supports the characters in the string.
    HRESULT hr = _codePages->GetStrCodePages(
        start, length, (priorityCodepages == 0) ? 0 : priorityCodepages, &codepages, &count);
    if (hr != S_OK)
    {
        return -1;
    }

    return 0;
}

int 
CommonTextMultiLanguageHandlerImplWin::CodepagesToCodepage(
    unsigned long& codepages,
    unsigned int& uCodePage,
    int defaultCodepage)
{
    if (_codePages == nullptr)
        Initialize();

    DWORD dwTemporaryCodePages;

    // Get a code page from the codepages
    HRESULT hr = _codePages->CodePagesToCodePage(
        codepages, (defaultCodepage == 0) ? CP_ACP : defaultCodepage, &uCodePage);
    if (hr != S_OK)
    {
        return -1;
    }

    // remove the codepage from the codepages
    hr = _codePages->CodePageToCodePages(uCodePage, &dwTemporaryCodePages);
    if (hr != S_OK)
    {
        return -1;
    }

    codepages &= ~dwTemporaryCodePages;
    return 0;
}

CommonTextStatus 
CommonTextMultiLanguageHandlerImplWin::DefaultFontFromCodepages(
    long codepages,
    std::wstring& fontTypeface)
{
    if (_fontLink == nullptr)
        Initialize();

    // Use Map font to find the default system font for the codepages.
    HFONT destFont;
    HRESULT hr = _fontLink->MapFont(_mhDC, codepages, wchar_t(0), &destFont);
    if (hr != S_OK)
    {
        return CommonTextStatus::CommonTextStatusFail;
    }

    if (destFont != nullptr)
    {
        HFONT oldfont = (HFONT)SelectObject(_mhDC, destFont);

        wchar_t ttNewFontFace[256];
        ttNewFontFace[0] = '\0';
        GetTextFace(_mhDC, 256, ttNewFontFace);
        fontTypeface = ttNewFontFace;

        SelectObject(_mhDC, oldfont);
    }
    else
        fontTypeface.clear();

    return CommonTextStatus::CommonTextStatusSuccess;
}

int 
CommonTextMultiLanguageHandlerImplWin::CharSetToCodePage(int charSet)
{
    return CommonTextCodePage::CharSetToCodePage(charSet);
}

int 
CommonTextMultiLanguageHandlerImplWin::CharSetToCodePageIndex(int charSet)
{
    return CommonTextCodePage::CharSetToCodePageIndex(charSet);
}

int 
CommonTextMultiLanguageHandlerImplWin::CharSetToCodePageId(int charSet)
{
    return CommonTextCodePage::CharSetToCodePageId(charSet);
}

short 
CommonTextMultiLanguageHandlerImplWin::CharSetToLanguage(int charSet)
{
    return CommonTextCodePage::CharSetToLanguage(charSet);
}

bool 
CommonTextMultiLanguageHandlerImplWin::CharSetIsDoubleByte(int charSet)
{
    return CommonTextCodePage::CharSetIsDoubleByte(charSet);
}

int 
CommonTextMultiLanguageHandlerImplWin::CodePageToCharSet(int codePage)
{
    return CommonTextCodePage::CodePageToCharSet(codePage);
}

int 
CommonTextMultiLanguageHandlerImplWin::CodePageToCodePageIndex(int codePage)
{
    return CommonTextCodePage::CodePageToCodePageIndex(codePage);
}

int 
CommonTextMultiLanguageHandlerImplWin::CodePageToCodePageId(int codePage)
{
    return CommonTextCodePage::CodePageToCodePageId(codePage);
}

short 
CommonTextMultiLanguageHandlerImplWin::CodePageToLanguage(int codePage)
{
    return CommonTextCodePage::CodePageToLanguage(codePage);
}

bool 
CommonTextMultiLanguageHandlerImplWin::CodePageIsDoubleByte(int codePage)
{
    return CommonTextCodePage::CodePageIsDoubleByte(codePage);
}

int 
CommonTextMultiLanguageHandlerImplWin::CodePageIdToCharSet(int codePageId)
{
    return CommonTextCodePage::CodePageIdToCharSet(codePageId);
}

int 
CommonTextMultiLanguageHandlerImplWin::CodePageIdToCodePage(int codePageId)
{
    return CommonTextCodePage::CodePageIdToCodePage(codePageId);
}

int 
CommonTextMultiLanguageHandlerImplWin::CodePageIdToCodePageIndex(int codePageId)
{
    return CommonTextCodePage::CodePageIdToCodePageIndex(codePageId);
}

short 
CommonTextMultiLanguageHandlerImplWin::CodePageIdToLanguage(int codePageId)
{
    return CommonTextCodePage::CodePageIdToLanguage(codePageId);
}

bool 
CommonTextMultiLanguageHandlerImplWin::CodePageIdIsDoubleByte(int codePageId)
{
    return CommonTextCodePage::CodePageIdIsDoubleByte(codePageId);
}

int 
CommonTextMultiLanguageHandlerImplWin::CodePageIndexToCharSet(int codePageIndex)
{
    return CommonTextCodePage::CodePageIndexToCharSet(codePageIndex);
}

int 
CommonTextMultiLanguageHandlerImplWin::CodePageIndexToCodePage(int codePageIndex)
{
    return CommonTextCodePage::CodePageIndexToCodePage(codePageIndex);
}

int 
CommonTextMultiLanguageHandlerImplWin::CodePageIndexToCodePageId(int codePageIndex)
{
    return CommonTextCodePage::CodePageIndexToCodePageId(codePageIndex);
}

short 
CommonTextMultiLanguageHandlerImplWin::CodePageIndexToLanguage(int codePageIndex)
{
    return CommonTextCodePage::CodePageIndexToLanguage(codePageIndex);
}

bool
CommonTextMultiLanguageHandlerImplWin::CodePageIndexIsDoubleByte(int codePageIndex)
{
    return CommonTextCodePage::CodePageIndexIsDoubleByte(codePageIndex);
}

int
CommonTextMultiLanguageHandlerImplWin::LanguageToCharSet(short language)
{
    return CommonTextCodePage::LanguageToCharSet(language);
}

int
CommonTextMultiLanguageHandlerImplWin::LanguageToCodePage(short language)
{
    return CommonTextCodePage::LanguageToCodePage(language);
}

int 
CommonTextMultiLanguageHandlerImplWin::LanguageToCodePageIndex(short language)
{
    return CommonTextCodePage::LanguageToCodePageIndex(language);
}

int 
CommonTextMultiLanguageHandlerImplWin::LanguageToCodePageId(short language)
{
    return CommonTextCodePage::LanguageToCodePageId(language);
}

bool
CommonTextMultiLanguageHandlerImplWin::LanguageIsDoubleByte(short language)
{
    return CommonTextCodePage::LanguageIsDoubleByte(language);
}

bool
CommonTextMultiLanguageHandlerImplWin::LanguageIsRtoL(short language)
{
    return CommonTextCodePage::LanguageIsRtoL(language);
}

short 
CommonTextMultiLanguageHandlerImplWin::LanguageFromUnicode(
    wchar_t wch,
    short def_lang)
{
    return CommonTextCodePage::LanguageFromUnicode(wch, def_lang);
}

bool
CommonTextMultiLanguageHandlerImplWin::IsLeadByte(
    int codePage,
    char c)
{
    return CommonTextCodePage::IsLeadByte(codePage, c);
}

bool
CommonTextMultiLanguageHandlerImplWin::IsLeadByte(char c)
{
    return CommonTextCodePage::IsLeadByte(c);
}

bool
CommonTextMultiLanguageHandlerImplWin::UnicodeForUserDefinedDBCS(
    wchar_t& unicodeValue,
    wchar_t DBCSCode,
    unsigned int Codepage)
{
    return CommonTextCodePage::UnicodeForUserDefinedDBCS(unicodeValue, DBCSCode, Codepage);
}

bool
CommonTextMultiLanguageHandlerImplWin::DBCSForUserDefinedUnicode(
    wchar_t& dbcs,
    wchar_t unicodeValue)
{
    return CommonTextCodePage::DBCSForUserDefinedUnicode(dbcs, unicodeValue);
}

bool
CommonTextMultiLanguageHandlerImplWin::CodepageForUserDefinedUnicode(
    int& codePage,
    wchar_t unicodeValue)
{
    return CommonTextCodePage::CodepageForUserDefinedUnicode(codePage, unicodeValue);
}

int
CommonTextMultiLanguageHandlerImplWin::LineBreakClass(int c)
{
    return CommonTextCodePage::LineBreakClass(c);
}

const wchar_t*
CommonTextMultiLanguageHandlerImplWin::MapFontFromCharset(int charset)
{
    return CommonTextCodePage::MapFontFromCharset(charset);
}

int
CommonTextMultiLanguageHandlerImplWin::CodePageCount()
{
    return CommonTextCodePage::CodePageCount();
}

int
CommonTextMultiLanguageHandlerImplWin::CodePageEntry(int i)
{
    return CommonTextCodePage::CodePageEntry(i);
}

bool
CommonTextMultiLanguageHandlerImplWin::RequireComplexScriptHandling(
    const wchar_t* start, 
    int length)
{
    return ScriptIsComplex(start, length, SIC_COMPLEX) == S_OK;
}

int
CommonTextMultiLanguageHandlerImplWin::SizeOfScriptAttribute()
{
    return sizeof(SCRIPT_ITEM);
}

int
CommonTextMultiLanguageHandlerImplWin::SizeOfClusterAttribute()
{
    return sizeof(SCRIPT_VISATTR);
}

bool
CommonTextMultiLanguageHandlerImplWin::ScriptsBreakString(
    const wchar_t* start,
    int length,
    bool containsComplex,
    std::vector<CommonTextScriptInfo>& scriptInfoArray,
    CommonTextStringsScriptAttribute& attributeOfEachSubString)
{
    // Use ScriptItemize to break the string into sub-strings.
    auto* items =
        reinterpret_cast<SCRIPT_ITEM*>(attributeOfEachSubString._scriptAttributeForStrings);

    HRESULT hr = ScriptItemize(start, length, attributeOfEachSubString._capacityOfAttributes,
        nullptr, nullptr, items, &attributeOfEachSubString._countOfSubStrings);

    if (FAILED(hr))
        return false;
    else
    {
        // Add the script information of each substring to the scriptInfoArray.
        scriptInfoArray.clear();
        for (int i = 0; i < attributeOfEachSubString._countOfSubStrings; i++)
        {
            CommonTextScriptInfo info = { items[i].iCharPos, items[i].a.eScript };
            scriptInfoArray.push_back(std::move(info));
        }

        // If complex script is not in this string, we don't need to save the complex
        // script information.
        if (!containsComplex)
            return true;

        // If two adjacent substrings are not in complex script, connect them. We don't need
        // to separately handle each non-complex string.

        // The index of the _subStringLength and _subStringIsComplex that we will save the
        // information of the next substring.
        int currentSubStringIndex = 0;
        // The character position of the last sub string which we have processed.
        int lastSubStringCharPos = 0;
        for (int i = 0; i < attributeOfEachSubString._countOfSubStrings; i++)
        {
            if (_scriptProperties[items[i].a.eScript]->fComplex == TRUE)
            {
                // The substring is a complex string.
                if (lastSubStringCharPos != items[i].iCharPos)
                {
                    // We don't process the strings between lastSubStringCharPos and this
                    // string. These sub-strings should all be non-complex. So they are all
                    // marked as one sub-string, and set complex to false.
                    attributeOfEachSubString._subStringLength[currentSubStringIndex] =
                        (short)(items[i].iCharPos - lastSubStringCharPos);
                    attributeOfEachSubString._subStringIsComplex[currentSubStringIndex] = false;
                    lastSubStringCharPos = items[i].iCharPos;
                    currentSubStringIndex++;
                }
                // Process this substring. It is a complex string.
                attributeOfEachSubString._subStringLength[currentSubStringIndex] =
                    (short)(items[i + 1].iCharPos - lastSubStringCharPos);
                items[currentSubStringIndex]                                        = items[i];
                attributeOfEachSubString._subStringIsComplex[currentSubStringIndex] = true;
                lastSubStringCharPos = items[i + 1].iCharPos;
                currentSubStringIndex++;
            }
        }
        // Process the remain sub-strings after the last complex substring. It is non-complex.
        if (lastSubStringCharPos != items[attributeOfEachSubString._countOfSubStrings].iCharPos)
        {
            attributeOfEachSubString._subStringLength[currentSubStringIndex] =
                (short)(items[attributeOfEachSubString._countOfSubStrings].iCharPos -
                    lastSubStringCharPos);
            attributeOfEachSubString._subStringIsComplex[currentSubStringIndex] = false;
            currentSubStringIndex++;
        }
        attributeOfEachSubString._countOfSubStrings = currentSubStringIndex;
        return true;
    }
}

SCRIPT_CACHE*
CommonTextMultiLanguageHandlerImplWin::_AcquireScriptCache(std::wstring key)
{
    auto iter = _scriptCaches.find(key);
    if (iter != _scriptCaches.end())
        return &(iter->second);
    else
    {
        iter = _scriptCaches.emplace(key, SCRIPT_CACHE(0)).first;
        return &(iter->second);
    }
}

SCRIPT_FONTPROPERTIES*
CommonTextMultiLanguageHandlerImplWin::_AcquireScriptFontProperties(std::wstring key)
{
    auto iter = _scriptFontProperties.find(key);
    if (iter == _scriptFontProperties.end())
    {
        auto* pProperties = new SCRIPT_FONTPROPERTIES;
        // Initialize the bytes to an invalid value.
        pProperties->cBytes = 0;
        _scriptFontProperties.emplace(key, pProperties);
        return pProperties;
    }
    else
    {
        SCRIPT_FONTPROPERTIES* pProperties = iter->second;
        return pProperties;
    }
}

bool
CommonTextMultiLanguageHandlerImplWin::ScriptsGetGlyphIndices(
    const UsdImagingTextStyle& style,
    const wchar_t* start,
    int length,
    void* attributeOfString,
    bool& isAllSupport,
    CommonTextClustersScriptAttribute& clustersAttribute)
{
    auto* cache          = _AcquireScriptCache(_GetUStringFromStyle(style));
    auto* fontProperties = _AcquireScriptFontProperties(_GetUStringFromStyle(style));
    auto* item           = reinterpret_cast<SCRIPT_ITEM*>(attributeOfString);
    auto* visAttr        = reinterpret_cast<SCRIPT_VISATTR*>(clustersAttribute._clustersAttribute);

    // Get the indices from the script cache.
    HRESULT hr = S_FALSE;
    if (fontProperties->cBytes != 0)
    {
        // If the font properties is invalid, the script cache is also not initialized. Call
        // ScriptShape only when the font properties is valid, which means the cache is
        // also initialized.
        hr = ScriptShape(nullptr, cache, start, length, clustersAttribute._capacityOfAttributes,
            &(item->a), clustersAttribute._indices, clustersAttribute._characterToGlyphMap, visAttr,
            &clustersAttribute._countOfGlyphs);
    }

    if (fontProperties->cBytes == 0 || hr == E_PENDING)
    {
        // The cache is not initialized or it is not enough. So create a font from the style
        // and select it in the device context. Get the indices from the device context.
        HFONT hFont = CreateFont(-(int)round(style._height), 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
            s2w(style._typeface).c_str());

        HFONT hFontOld = (HFONT)SelectObject(_mhDC, hFont);

        // The size of the font properties must be initialized first.
        fontProperties->cBytes = sizeof(SCRIPT_FONTPROPERTIES);

        // The font properties is used to get the default index if a character is missing
        // in the font.
        if (ScriptGetFontProperties(_mhDC, cache, fontProperties) < 0)
        {
            SelectObject(_mhDC, hFontOld);
            DeleteObject(hFont);
            // Mark the font properties invalid.
            fontProperties->cBytes = 0;
            return false;
        }

        hr = ScriptShape(_mhDC, cache, start, length, clustersAttribute._capacityOfAttributes,
            &(item->a), clustersAttribute._indices, clustersAttribute._characterToGlyphMap, visAttr,
            &clustersAttribute._countOfGlyphs);

        SelectObject(_mhDC, hFontOld);
        DeleteObject(hFont);
    }

    if (hr == USP_E_SCRIPT_NOT_IN_FONT)
    {
        // The font doesn't support the script.
        clustersAttribute._countOfGlyphs = 0;
        clustersAttribute._countOfClusters = 0;
        isAllSupport = false;
    }
    else if (hr == S_OK)
    {
        isAllSupport = true;

        // Although ScriptShape is successful, maybe some characters are missing. The index
        // of these characters will be the default glyph. And we will use the missing index
        // to replace the index.
        for (int i = 0; i < clustersAttribute._countOfGlyphs; i++)
        {
            if (clustersAttribute._indices[i] == fontProperties->wgDefault)
            {
                // Not all characters are supported.
                clustersAttribute._indices[i] = TRUETYPE_MISSING_GLYPH_INDEX;
                isAllSupport                  = false;
            }
        }
    }
    else
    {
        return false;
    }

    // We need to get the map from character to cluster using the map from character to
    // glyph. The current cluster index.
    short currentClusterIndex = 0;
    if (clustersAttribute._countOfGlyphs > 0)
    {
        clustersAttribute._characterToClusterMap[0] = currentClusterIndex;
        for (int i = 1; i < length; i++)
        {
            // If the glyph of this character is different from the last character, it's a
            // new cluster.
            if (clustersAttribute._characterToGlyphMap[i] !=
                clustersAttribute._characterToGlyphMap[i - 1])
                currentClusterIndex++;
            clustersAttribute._characterToClusterMap[i] = currentClusterIndex;
        }
        clustersAttribute._countOfClusters = currentClusterIndex + 1;
    }
    return true;
}

bool 
CommonTextMultiLanguageHandlerImplWin::ScriptIfAllCharactersAreSupported(
    const UsdImagingTextStyle& style,
    const wchar_t* start,
    int length,
    bool& IsAllSupported,
    unsigned short* indices)
{
    static unsigned short localIndices[MAXIMUM_COUNT_OF_CHAR_IN_LINE];
    SCRIPT_CACHE* cache                   = _AcquireScriptCache(_GetUStringFromStyle(style));
    SCRIPT_FONTPROPERTIES* fontProperties = _AcquireScriptFontProperties(_GetUStringFromStyle(style));

    // If indices is nullptr, it means we don't care about the indices. So we just use the
    // local one.
    unsigned short* resultIndices = localIndices;
    if (indices != nullptr)
        resultIndices = indices;

    // Get the indices according to the cmap table.
    HRESULT hr = S_FALSE;
    if (fontProperties->cBytes != 0)
    {
        // If the font properties is invalid, the script cache is also not initialized. Call
        // ScriptGetCMap only when the font properties is valid, which means the cache is
        // also initialized.
        hr = ScriptGetCMap(nullptr, cache, start, length, 0, resultIndices);
    }

    if (fontProperties->cBytes == 0 || hr == E_PENDING)
    {
        // The cache is not initialized or it is not enough. So create a font from the style
        // and select it in the device context. Get the indices from the device context.
        HFONT hFont = CreateFont(-(int)round(style._height), 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
            s2w(style._typeface).c_str());

        HFONT hFontOld = (HFONT)SelectObject(_mhDC, hFont);

        // The size of the font properties must be initialized first.
        fontProperties->cBytes = sizeof(SCRIPT_FONTPROPERTIES);

        // The font properties is used to get the default index if a character is missing
        // in the font.
        if (FAILED(ScriptGetFontProperties(_mhDC, cache, fontProperties)))
        {
            SelectObject(_mhDC, hFontOld);
            DeleteObject(hFont);
            // Mark the font properties invalid.
            fontProperties->cBytes = 0;
            return false;
        }

        hr = ScriptGetCMap(_mhDC, cache, start, length, 0, resultIndices);

        SelectObject(_mhDC, hFontOld);
        DeleteObject(hFont);
    }

    if (hr == S_OK)
    {
        // All characters are supported.
        IsAllSupported = true;
        return true;
    }
    else if (hr == S_FALSE)
    {
        // Not all characters are supported.
        IsAllSupported = false;
        if (indices != nullptr)
        {
            // Set the missing character to TRUETYPE_MISSING_GLYPH_INDEX.
            for (int i = 0; i < length; i++)
            {
                if (indices[i] == fontProperties->wgDefault)
                {
                    indices[i] = TRUETYPE_MISSING_GLYPH_INDEX;
                }
            }
        }
        return true;
    }
    else
        return false;
}
PXR_NAMESPACE_CLOSE_SCOPE
