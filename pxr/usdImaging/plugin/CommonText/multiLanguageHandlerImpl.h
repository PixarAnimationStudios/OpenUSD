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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_MULTILANGUAGE_HANDLER_IMP_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_MULTILANGUAGE_HANDLER_IMP_H

#include "definitions.h"
#include "globals.h"
#include "metrics.h"

#include <cstring>

PXR_NAMESPACE_OPEN_SCOPE
#define MAX_SCRIPT_ITEM 200
#define MAX_GLYPHS 200

/// \struct CommonTextStringsScriptAttribute
///
/// The output when we break the string by scripts. Include the length of the substring,
/// the analysis of the substring and if the substring is in complex scripts.
/// The struct is variable in length.
///
struct CommonTextStringsScriptAttribute
{
    // The count of substrings.
    int _countOfSubStrings = 1;

    // The length of each substring.
    short _subStringLength[MAX_SCRIPT_ITEM];

    // If the substring is in complex scripts.
    bool _subStringIsComplex[MAX_SCRIPT_ITEM];

    // The capacity of the _scriptAttributeForStrings. That is, how many Script Attribute
    // can be put into the _scriptAttributeForStrings.
    int _capacityOfAttributes = 0;

    // The size of a single Script Attribute.
    int _sizeOfSingleScriptAttribute = 0;

    // A placeholder for an array of the attributes. The size of this array is
    // _capacityOfAttributes. The count of element is _countOfSubStrings. The size of each
    // ScriptAttribute is _sizeOfSingleScriptAttribute.
    void* _scriptAttributeForStrings = nullptr;

    /// The constructor.
    CommonTextStringsScriptAttribute(int sizeOfSinglePlatformAttribute)
        : _sizeOfSingleScriptAttribute(sizeOfSinglePlatformAttribute)
    {
        memset(_subStringIsComplex, 0, sizeof(bool) * MAX_SCRIPT_ITEM);
        memset(_subStringLength, 0, sizeof(short) * MAX_SCRIPT_ITEM);
        // _capacityOfAttributes is used in ScriptItemize. In MSDN documentation, 
        // it seems that _capacityOfAttributes should be the maximunm count of items.
        // But in our practice, we found that it should be the maximum size of the buffer 
        // that will save the items.
        _capacityOfAttributes = sizeof(char) * MAX_SCRIPT_ITEM * sizeOfSinglePlatformAttribute;
        _scriptAttributeForStrings =
            malloc(sizeof(char) * (_capacityOfAttributes + 1) * sizeOfSinglePlatformAttribute);
    }

    ~CommonTextStringsScriptAttribute() { free(_scriptAttributeForStrings); }
};

/// \struct CommonTextClustersScriptAttribute
///
/// The output when we get the indices and clusters. Include the indices of every character,
/// the map between character, glyph and cluster, and the attributes of each cluster.
struct CommonTextClustersScriptAttribute
{
    // The count of generated glyphs in the string.
    int _countOfGlyphs = 0;

    // The count of clusters.
    int _countOfClusters = 0;

    // The index of each glyph.
    unsigned short _indices[MAX_GLYPHS];

    // The map between glyph and cluster.
    unsigned short _characterToGlyphMap[MAX_GLYPHS];

    // The map between character and cluster.
    short _characterToClusterMap[MAX_GLYPHS];

    // The capacity of the _clustersAttribute. That is, how many cluster attribute can be
    // put into the _clustersAttribute.
    int _capacityOfAttributes = MAX_GLYPHS;

    // The size of a single cluster attribute.
    int _sizeOfSingleClusterAttribute = 0;

    // A placeholder for an array of the attributes. The size of this array is
    // _capacityOfAttributes. The count of element is _countOfClusters. The size of each
    // ScriptAttribute is _sizeOfSingleClusterAttribute.
    void* _clustersAttribute = nullptr;

    /// The constructor.
    CommonTextClustersScriptAttribute(int sizeOfSingleClusterAttribute)
        : _sizeOfSingleClusterAttribute(sizeOfSingleClusterAttribute)
    {
        memset(_indices, 0, sizeof(short) * MAX_GLYPHS);
        memset(_characterToGlyphMap, 0, sizeof(short) * MAX_GLYPHS);
        memset(_characterToClusterMap, 0, sizeof(short) * MAX_GLYPHS);
        _clustersAttribute =
            malloc(sizeof(char) * sizeOfSingleClusterAttribute * _capacityOfAttributes);
    }
    ~CommonTextClustersScriptAttribute() { free(_clustersAttribute); }
};

/// \class CommonTextMultiLanguageHandlerImpl
///
/// Interface for the multilanguage handler.
///
class CommonTextMultiLanguageHandlerImpl
{
public:
    /// Initialize the truetype cache
    virtual CommonTextStatus InitializeTrueTypeCache(std::shared_ptr<CommonTextFontMapCache>) = 0;

    /// Get the codepages which support the most characters
    /// from the beginning of the string
    /// \param start The pointer of the string
    /// \param length The length of the string
    /// \param codepages The codepages that contain the characters in the string
    /// \param count The number of characters that is supported by codepages.
    /// \param priorityCodepages Specifies a set of code pages to give priority
    virtual int GetStringCodePages(const wchar_t* start, 
                                   int length, 
                                   unsigned long& codepages,
                                   long& count,
                                   long priorityCodepages = 0) = 0;

    /// Get a codepage from the set of codepages, and remove it from the codepages
    /// \param codepages The set of codepages.
    /// \param uCodePage code page returned.
    /// \param defaultCodepage Default codepage we will look for.
    virtual int CodepagesToCodepage(unsigned long& codepages, 
                                    unsigned int& uCodePage, 
                                    int defaultCodepage = 0) = 0;

    /// Get a system default font which supports the set of codepages.
    /// \param codepages The set of codepages.
    /// \param fontTypeface The typeface of the font. If this value is empty, it means
    /// there is no font which supports the codepages.
    virtual CommonTextStatus DefaultFontFromCodepages(
        long codepages, 
        std::wstring& fontTypeface) = 0;

    //======================codepage and charset conversion====================
    /// The enumeration of codepage index.
    enum class CodePageIndex
    {
        kUNDEFINED_CODEPAGE_INDEX   = 0,
        kJAPANESE_CODEPAGE_INDEX    = 1,
        kCHINESETRAD_CODEPAGE_INDEX = 2,
        kKOREAN_CODEPAGE_INDEX      = 3,
        kJOHAB_CODEPAGE_INDEX       = 4,
        kCHINESESIMP_CODEPAGE_INDEX = 5
    };

    /// Convert from a charset to a codepage.
    virtual int CharSetToCodePage(int charSet) = 0;

    /// Convert from a charset to a codepage index.
    virtual int CharSetToCodePageIndex(int charSet) = 0;

    /// Convert from a charset to a codepage id.
    virtual int CharSetToCodePageId(int charSet) = 0;

    /// Convert from a charset to a language.
    virtual short CharSetToLanguage(int charSet) = 0;

    /// Test if the characters of a charset is double byte.
    virtual bool CharSetIsDoubleByte(int charSet) = 0;

    /// Convert from a codepage to a charset.
    virtual int CodePageToCharSet(int codePage) = 0;

    /// Convert from a codepage to a codepage index.
    virtual int CodePageToCodePageIndex(int codePage) = 0;

    /// Convert from a codepage to a codepage id.
    virtual int CodePageToCodePageId(int codePage) = 0;

    /// Convert from a codepage to a language.
    virtual short CodePageToLanguage(int codePage) = 0;

    /// Test if the characters of a codepage is double byte.
    virtual bool CodePageIsDoubleByte(int codePage) = 0;

    /// Convert from a codepage id to a charset.
    virtual int CodePageIdToCharSet(int codePageId) = 0;

    /// Convert from a codepage id to a codepage.
    virtual int CodePageIdToCodePage(int codePageId) = 0;

    /// Convert from a codepage id to a codepage index.
    virtual int CodePageIdToCodePageIndex(int codePageId) = 0;

    /// Convert from a codepage id to a language.
    virtual short CodePageIdToLanguage(int codePageId) = 0;

    /// Test if the characters of a codepage is double byte.
    virtual bool CodePageIdIsDoubleByte(int codePageId) = 0;

    /// Convert from a codepage index to a charset.
    virtual int CodePageIndexToCharSet(int codePageIndex) = 0;

    /// Convert from a codepage index to a codepage.
    virtual int CodePageIndexToCodePage(int codePageIndex) = 0;

    /// Convert from a codepage index to a codepage id.
    virtual int CodePageIndexToCodePageId(int codePageIndex) = 0;

    /// Convert from a codepage index to a language.
    virtual short CodePageIndexToLanguage(int codePageIndex) = 0;

    /// Test if the characters of a codepage index is double byte.
    virtual bool CodePageIndexIsDoubleByte(int codePageIndex) = 0;

    /// Convert from a language to a charset.
    virtual int LanguageToCharSet(short language) = 0;

    /// Convert from a language to a codepage.
    virtual int LanguageToCodePage(short language) = 0;

    /// Convert from a language to a codepage index.
    virtual int LanguageToCodePageIndex(short language) = 0;

    /// Convert from a language to a codepage id.
    virtual int LanguageToCodePageId(short language) = 0;

    /// Test if the characters of a language is double byte.
    virtual bool LanguageIsDoubleByte(short language) = 0;

    /// Test if the language is read from right to left.
    virtual bool LanguageIsRtoL(short language) = 0;

    /// Retrieve the language of a character
    /// \param wch The unicode character which we want to get its language
    /// \param def_lang The default language.
    virtual short LanguageFromUnicode(wchar_t wch, 
                                      short def_lang = 0) = 0;

    /// The count of codepages.
    virtual int CodePageCount() = 0;

    /// Get the codepage from the index.
    virtual int CodePageEntry(int i) = 0;

    /// If the byte is a lead byte of the chararacters in the codepage.
    virtual bool IsLeadByte(int codePage, 
                            char c) = 0;

    /// If the byte is a lead byte.
    virtual bool IsLeadByte(char c) = 0;

    /// Get a font which support the characters in the charset
    virtual const wchar_t* MapFontFromCharset(int charset) = 0;

    /// Support for mapping a DBCS code that is not in the range of valid DBCS
    /// Convert a user defined DBCS code to unicode value.
    virtual bool UnicodeForUserDefinedDBCS(wchar_t& unicodeValue, 
                                           wchar_t DBCSCode, 
                                           unsigned int Codepage) = 0;

    /// Support for mapping a DBCS code that is not in the range of valid DBCS
    /// Convert a user defined unicode value to DBCS.
    virtual bool DBCSForUserDefinedUnicode(wchar_t& dbcs, 
                                           wchar_t unicodeValue) = 0;

    /// Support for mapping a DBCS code that is not in the range of valid DBCS
    /// Get the codepage of a user defined unicode value.
    virtual bool CodepageForUserDefinedUnicode(int& codePage, 
                                               wchar_t unicodeValue) = 0;

    //================================Line break class==========================================
    enum
    {
        ULB_OP,
        ULB_CL,
        ULB_QU,
        ULB_GL,
        ULB_NS,
        ULB_EX,
        ULB_SY,
        ULB_IS,
        ULB_PR,
        ULB_PO,
        ULB_NU,
        ULB_AL,
        ULB_ID,
        ULB_IN,
        ULB_HY,
        ULB_BA,
        ULB_BB,
        ULB_B2,
        ULB_ZW,
        ULB_CM,
        ULB_WJ,

        ULB_CR,
        ULB_LF,
        ULB_NL,
        ULB_CB,
        ULB_XX,

        ULB_SP,
        ULB_BK,
        ULB_SA,
        ULB_AI,

        // ULB_SG,
        ULB_NN,
    };

    enum
    {
        ULB_DBK, // direct break
        ULB_IBK, // indirect break
        ULB_CBK, // combining break
        ULB_PBK, // prohibited break
    };

    /// Get the line break class of the character.
    virtual int LineBreakClass(int c) = 0;

    /// Test if the character is a justifiable character.
    virtual bool IsJustifiableChar(wchar_t wch)
    {
        return wch == ' ' || LineBreakClass(wch) == ULB_ID;
    }

    //================================Complex Script Handling===============================
    /// If the multilanguage handler support complex script handling on this platform.
    virtual bool SupportComplexScriptHandling() const = 0;

    /// If the string contain characters that require complex script handling
    virtual bool RequireComplexScriptHandling(const wchar_t* start, 
                                              int length) = 0;

    /// Get the size of the script attribute structure.
    virtual int SizeOfScriptAttribute() = 0;

    /// Get the size of the cluster attribute structure.
    virtual int SizeOfClusterAttribute() = 0;

    /// Break the string by scripts, and get the
    /// \param start The start of the string.
    /// \param length The length of the string.
    /// \param containsComplex If the text string contains complex script.
    /// \param scriptInfoArray The
    /// information of the script in the string.
    /// \param attributeOfEachSubString The output result
    /// which contains the length and the analysis of each substring.
    virtual bool ScriptsBreakString(const wchar_t* start, 
                                    int length, 
                                    bool containsComplex,
                                    std::vector<CommonTextScriptInfo>& scriptInfoArray,
                                    CommonTextStringsScriptAttribute& attributeOfEachSubString) = 0;

    /// Get the indices of the characters in the font.
    /// \param style The text style.
    /// \param start The start of the string.
    /// \param length The length of the string.
    /// \param attributeOfString The attribute of the string. Such as the scripts.
    /// \param isAllSupport Get if all the characters are
    /// supported by the style.
    /// \param clustersAttribute The output result which contains the
    /// indices and information of each cluster.
    virtual bool ScriptsGetGlyphIndices(const UsdImagingTextStyle& style,
                                        const wchar_t* start, 
                                        int length,
                                        void* attributeOfString,
                                        bool& isAllSupport,
                                        CommonTextClustersScriptAttribute& clustersAttribute) = 0;

    /// Check if all the characters are supported in the font.
    /// Indices is only for checking if the character is supported. Don't use them as the
    /// final indices of the characters. Indices can be set to nullptr, so that we don't
    /// get the indices.
    virtual bool ScriptIfAllCharactersAreSupported(
        const UsdImagingTextStyle& style, 
        const wchar_t* start,
        int length, 
        bool& IsAllSupported, 
        unsigned short* indices = nullptr) = 0;
};
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_MULTILANGUAGE_HANDLER_IMP_H
