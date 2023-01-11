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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_CODE_PAGE_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_CODE_PAGE_H

#include "Definitions.h"
#include "multiLanguageHandlerImpl.h"

PXR_NAMESPACE_OPEN_SCOPE

#define UNICODEUSERDBCSMAPSIZE 256

/// \struct CommonTextUnicodeUserDBCSMapEntry
///
/// The map entry between codepage and DBCS code
///
struct CommonTextUnicodeUserDBCSMapEntry
{
    wchar_t _DBCSCode;
    int _codepage;
    wchar_t _unicodeValue;
};

/// \class CommonTextCodePage
///
/// Utilities for mapping between:
///    1. Codepage
///    2. Character Set
///    3. Codepage ID
///    4. Codepage Index (the '#' in mif sequence "\M+#XXXX")
///    5. Locale ID (language)
///
class CommonTextCodePage
{
public:
    /// The constructor
    CommonTextCodePage();

    //==================Codepage and Charset conversion=========================//
    /// Convert from a charset to a codepage.
    static int CharSetToCodePage(int charSet);

    /// Convert from a charset to a codepage index.
    static int CharSetToCodePageIndex(int charSet);

    /// Convert from a charset to a codepage id.
    static int CharSetToCodePageId(int charSet);

    /// Convert from a charset to a LCID.
    static int CharSetToLCID(int charSet);

    /// Convert from a charset to a language.
    static short CharSetToLanguage(int charSet);

    /// Test if the characters of a charset is double byte.
    static bool CharSetIsDoubleByte(int charSet);

    /// Convert from a codepage to a charset.
    static int CodePageToCharSet(int codePage);

    /// Convert from a codepage to a codepage index.
    static int CodePageToCodePageIndex(int codePage);

    /// Convert from a codepage to a codepage id.
    static int CodePageToCodePageId(int codePage);

    /// Convert from a codepage to a LCID.
    static int CodePageToLCID(int codePage);

    /// Convert from a codepage to a language.
    static short CodePageToLanguage(int codePage);

    /// Test if the characters of a codepage is double byte.
    static bool CodePageIsDoubleByte(int codePage);

    /// Convert from a codepage id to a charset.
    static int CodePageIdToCharSet(int codePageId);

    /// Convert from a codepage id to a codepage.
    static int CodePageIdToCodePage(int codePageId);

    /// Convert from a codepage id to a codepage index.
    static int CodePageIdToCodePageIndex(int codePageId);

    /// Convert from a codepage id to a LCID.
    static int CodePageIdToLCID(int codePageId);

    /// Convert from a codepage id to a language.
    static short CodePageIdToLanguage(int codePageId);

    /// Test if the characters of a codepage is double byte.
    static bool CodePageIdIsDoubleByte(int codePageId);

    /// Convert from a codepage index to a charset.
    static int CodePageIndexToCharSet(int codePageIndex);

    /// Convert from a codepage index to a codepage.
    static int CodePageIndexToCodePage(int codePageIndex);

    /// Convert from a codepage index to a codepage id.
    static int CodePageIndexToCodePageId(int codePageIndex);

    /// Convert from a codepage index to a LCID.
    static int CodePageIndexToLCID(int codePageIndex);

    /// Convert from a codepage index to a language.
    static short CodePageIndexToLanguage(int codePageIndex);

    /// Test if the characters of a codepage index is double byte.
    static bool CodePageIndexIsDoubleByte(int codePageIndex);

    /// Convert from a LCID to a charset.
    static int LCIDToCharSet(long lcid);

    /// Convert from a LCID to a codepage.
    static int LCIDToCodePage(long lcid);

    /// Convert from a LCID to a codepage index.
    static int LCIDToCodePageIndex(long lcid);

    /// Convert from a LCID to a codepage id.
    static int LCIDToCodePageId(long lcid);

    /// Convert from a LCID to a language.
    static short LCIDToLanguage(long lcid);

    /// Test if the characters in this LCID is double byte
    static bool LCIDIsDoubleByte(long lcid);

    /// Convert from a language to a charset.
    static int LanguageToCharSet(short language);

    /// Convert from a language to a codepage.
    static int LanguageToCodePage(short language);

    /// Convert from a language to a codepage index.
    static int LanguageToCodePageIndex(short language);

    /// Convert from a language to a codepage id.
    static int LanguageToCodePageId(short language);

    /// Convert from a language to a LCID.
    static int LanguageToLCID(short language);

    /// Test if the characters of a language is double byte.
    static bool LanguageIsDoubleByte(short language);

    /// Test if the language is read from right to left.
    static bool LanguageIsRtoL(short language);

    /// Retrieve the language of a character
    /// \param wch The unicode character which we want to get its language
    /// \param def_lang The default language.
    static short LanguageFromUnicode(wchar_t wch, 
                                     short def_lang = 0);

    // VC8: Missing type specifier error.  Commented out until ready to be implemented.
    // UnicodeUserDBCSMap();

    /// The count of codepages.
    static int CodePageCount();

    /// Get the codepage from the index.
    static int CodePageEntry(int i);

    /// The count of LCIDs.
    static int LCIDCount();

    /// Get an LCID from the index.
    static long LCIDEntry(int i);

    /// Support for mapping a DBCS code that is not in the range of valid DBCS
    /// Convert a user defined DBCS code to unicode value.
    static bool UnicodeForUserDefinedDBCS(wchar_t& unicodeValue, 
                                          wchar_t DBCSCode, 
                                          int codepage);

    /// Support for mapping a DBCS code that is not in the range of valid DBCS
    /// Convert a user defined unicode value to DBCS.
    static bool DBCSForUserDefinedUnicode(wchar_t& dbcs, 
                                          wchar_t unicodeValue);

    /// Support for mapping a DBCS code that is not in the range of valid DBCS
    /// Get the codepage of a user defined unicode value.
    static bool CodepageForUserDefinedUnicode(int& codePage, 
                                              wchar_t unicodeValue);

    /// Get a font which support the characters in the charset
    static const wchar_t* MapFontFromCharset(int charset);

    /// If the byte is a lead byte of the chararacters in the codepage.
    /// Unicode: "c" remains char-typed,
    /// it should never be passed a wide char.
    static inline bool IsLeadByte(int codePage, 
                                  char c)
    {
        if ((unsigned char)c >= 0x20 && (unsigned char)c <= 0x7f)
        {
            assert(!IsDBCSLeadByteEx(codePage, c));
            return false;
        }
        return IsDBCSLeadByteEx(codePage, c) != 0;
    }

    /// If the byte is a lead byte.
    /// Unicode: "c" remains char-typed,
    /// it should never be passed a wide char.
    static inline bool IsLeadByte(char c)
    {
        if ((unsigned char)c >= 0x20 && (unsigned char)c <= 0x7f)
        {
            assert(!IsDBCSLeadByte(c));
            return false;
        }
        return IsDBCSLeadByte(c) != 0;
    }

    //==================The line break definition and function=========================//
    static inline int LineBreakClass(int c)
    {
        if (c < _directLineBreakClassArrayCount)
            return _directLineBreakClass[c];

        for (int i = 0; i < _indirectLineBreakClassArrayCount; i++)
        {
            if (/*c >= m_IndirectLineBreakClass[i].mincode && */ c <=
                _indirectLineBreakClass[i]._maxcode)
            {
                return _indirectLineBreakClass[i]._type;
            }
        }

        return CommonTextMultiLanguageHandlerImpl::ULB_ID;
    }

    static int AnalyzeLineBreaks(int* pcls, 
                                 int* pbrk, 
                                 int cch);

private:
    static const int _directLineBreakClassArrayCount;
    static const int _indirectLineBreakClassArrayCount;

    // Unicode: I *think* these are binary values, so leaving as
    // char for now.
    static char _directLineBreakClass[];

    static CommonTextUnicodeUserDBCSMapEntry _unicodeUserDBCSMap[UNICODEUSERDBCSMAPSIZE];

    struct CommonTextIndirectLineBreakClass
    {
        int _mincode, _maxcode;
        int _type;
    };

    static CommonTextIndirectLineBreakClass _indirectLineBreakClass[];

    // Unicode: I *think* these are binary values, so leaving as
    // char for now.
    static const char _lineBreakPairs[CommonTextMultiLanguageHandlerImpl::ULB_WJ + 1]
                                     [CommonTextMultiLanguageHandlerImpl::ULB_WJ + 1];

    static int _AnalyzeComplexLineBreaks(int* pcls, int* pbrk, int cch);

    static int _langFlags[];
    static BYTE _langIdx[];

    struct CommonTextLCIDAndCharSet
    {
        short _lcid;
        int _charset;
    };

    static CommonTextLCIDAndCharSet _LCIDAndCharSetArray[];
    static int _iLCIDCount;

    static int _langCharsets[];

    static int _codePageArray[];
};
PXR_NAMESPACE_CLOSE_SCOPE
#endif // PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_CODE_PAGE_H