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

#define _ACCODEPAGE_CPP

#include "codePage.h"
#include "codePageEnums.h"
#include "lCID.h"
#include "langVal.h"
#include "lineBreak.h"
#include "portableUtils.h"

PXR_NAMESPACE_OPEN_SCOPE
/// \struct CodePageDsc
///
/// This struct contains information about a specific code page.
///
struct CodePageDsc
{
    /// id for \M+ sequences
    char _char_id;
    /// code page string
    const wchar_t* _szCodepageName;
    /// Windows like short representation
    short _nWinCodePage;
};

/// Table to convert from old pre Sedona code page id to new Windows like id (short)

/* The first member of each element is used to identify code pages for
which we don't have (or don't use) a Unicode translation table.  A
question mark identifies entries with Unicode mappings, other chars
are used to encode and decode \M+NXXXX sequences where N is the
char_id member of the item in the table.  For example, a character
from the undefined code page (0) would be encoded as \M+0[XX].  This
table must be kept in sync with "char_to code_page_id_tbl[]", and
there are ASSERTS in place to make sure that you remember to do so.
*/
const CodePageDsc code_page_dscs[CODE_PAGE_CNT] = {
// Unicode: Leaving first member as char because it is an id for M+
// sequence.
    { '0', L"undefined", 0 },  // CODE_PAGE_UNDEFINED
    { '?', L"ascii", CP_ACP }, // ASCII is not valid in
    // Win32 API we use ACP instead
    { '?', L"iso8859-1", 1252 }, { '?', L"iso8859-2", 1250 }, { '?', L"iso8859-3", 0 },
    { '?', L"iso8859-4", 0 }, { '?', L"iso8859-5", 0 }, { '?', L"iso8859-6", 0 },
    { '?', L"iso8859-7", 1253 }, { '?', L"iso8859-8", 0 }, { '?', L"iso8859-9", 1254 },
    { '?', L"dos437", 437 }, { '?', L"dos850", 850 }, { '?', L"dos852", 852 },
    { '?', L"dos855", 855 }, { '?', L"dos857", 857 }, { '?', L"dos860", 860 },
    { '?', L"dos861", 861 }, { '?', L"dos863", 863 }, { '?', L"dos864", 864 },
    { '?', L"dos865", 865 }, { '?', L"dos869", 869 }, { '1', L"dos932_m", 932 }, /* JAPAN */
    { '?', L"mac-roman", 0 }, { '2', L"big5_m", 950 },                           /* CHINA 1 */
    { '3', L"ksc5601_m", 949 },                                                  /* KOREA 1 */
    { '4', L"johab_m", 1361 },                                                   /* KOREA 2 */
    { '?', L"dos866", 866 }, { '?', L"ANSI_1250", 1250 }, // CODE_PAGE_ANSI_1250
    { '?', L"ANSI_1251", 1251 },                          // CODE_PAGE_ANSI_1251
    { '?', L"ANSI_1252", 1252 },                          // CODE_PAGE_ANSI_1252
    { '5', L"gb2312_m", 936 },                            /* CHINA 2 */
    { '?', L"ANSI_1253", 1253 },                          // CODE_PAGE_ANSI_1253
    { '?', L"ANSI_1254", 1254 },                          // CODE_PAGE_ANSI_1254
    { '?', L"ANSI_1255", 1255 },                          // CODE_PAGE_ANSI_1255
    { '?', L"ANSI_1256", 1256 },                          // CODE_PAGE_ANSI_1256
    { '?', L"ANSI_1257", 1257 },                          // CODE_PAGE_ANSI_1257
    { '?', L"ANSI_874", 874 },                            // CODE_PAGE_ANSI_874
    { '?', L"ANSI_932", 932 },                            // CODE_PAGE_ANSI_932
    { '?', L"ANSI_936", 936 },                            // CODE_PAGE_ANSI_936
    { '?', L"ANSI_949", 949 },                            // CODE_PAGE_ANSI_949
    { '?', L"ANSI_950", 950 },                            // CODE_PAGE_ANSI_950
    { '?', L"ANSI_1361", 1361 },                          // CODE_PAGE_ANSI_1361
    { '?', L"ANSI_1200", 1200 },                          // CODE_PAGE_ANSI_1200
    { '?', L"ANSI_1258", 1258 }                           // CODE_PAGE_ANSI_1258
};

short __stdcall WinCodePageFromId(unsigned int value)
{
    if (!IsValidCodePageId(value))
        // avoid out of range array indexing
        value = CODE_PAGE_ANSI_1252;
    return code_page_dscs[value]._nWinCodePage;
}

#define ACCP_NELTS(a) (sizeof(a) / sizeof(a[0]))

CommonTextUnicodeUserDBCSMapEntry CommonTextCodePage::_unicodeUserDBCSMap[];
const int unicodeUserDefinedDBCSMapStart = 0xe800;

int CommonTextCodePage::_codePageArray[] = {
    ANSI_CODEPAGE,
    JAPANESE_CODEPAGE,
    KOREAN_CODEPAGE,
    CHINESESIMP_CODEPAGE,
    CHINESETRAD_CODEPAGE,
    HEBREW_CODEPAGE,
    ARABIC_CODEPAGE,
    GREEK_CODEPAGE,
    TURKISH_CODEPAGE,
    VIETNAMESE_CODEPAGE,
    THAI_CODEPAGE,
    EASTEUROPE_CODEPAGE,
    RUSSIAN_CODEPAGE,
    BALTIC_CODEPAGE,
};

CommonTextCodePage::CommonTextCodePage()
{
    for (int i = 0; i < UNICODEUSERDBCSMAPSIZE; i++)
    {
        _unicodeUserDBCSMap[i]._DBCSCode     = 0;
        _unicodeUserDBCSMap[i]._codepage     = 0;
        _unicodeUserDBCSMap[i]._unicodeValue = (wchar_t)(unicodeUserDefinedDBCSMapStart + i);
    }
}

int 
CommonTextCodePage::CodePageCount()
{
    return ACCP_NELTS(_codePageArray);
}

int 
CommonTextCodePage::CodePageEntry(int i)
{
    if (i < ACCP_NELTS(_codePageArray))
        return _codePageArray[i];

    assert(i < ACCP_NELTS(_codePageArray));
    return _codePageArray[0];
}

// CharSet..
int 
CommonTextCodePage::CharSetToCodePage(int charSet)
{
    int codePage = ANSI_CODEPAGE;

    switch (charSet)
    {
    case EASTEUROPE_CHARSET:
        // CE
        codePage = EASTEUROPE_CODEPAGE;
        break;
    case RUSSIAN_CHARSET:
        // RU
        codePage = RUSSIAN_CODEPAGE;
        break;
    case HEBREW_CHARSET:
        // HE
        codePage = HEBREW_CODEPAGE;
        break;
    case ARABIC_CHARSET:
        // ARABIC
        codePage = ARABIC_CODEPAGE;
        break;
    case BALTIC_CHARSET:
        // BALTIC
        codePage = BALTIC_CODEPAGE;
        break;
    case GREEK_CHARSET:
        // GREEK
        codePage = GREEK_CODEPAGE;
        break;
    case TURKISH_CHARSET:
        codePage = TURKISH_CODEPAGE;
        break;
    case VIETNAMESE_CHARSET:
        codePage = VIETNAMESE_CODEPAGE;
        break;
    case JAPANESE_CHARSET:
        // SHIFTJIS
        codePage = JAPANESE_CODEPAGE;
        break;
    case KOREAN_CHARSET:
        // Korean - HANGEUL
        codePage = KOREAN_CODEPAGE;
        break;
    case JOHAB_CHARSET:
        // Johab
        codePage = JOHAB_CODEPAGE;
        break;
    case CHINESESIMP_CHARSET:
        // Simplified Chinese
        codePage = CHINESESIMP_CODEPAGE;
        break;
    case CHINESETRAD_CHARSET:
        // Traditional Chinese
        codePage = CHINESETRAD_CODEPAGE;
        break;
    case THAI_CHARSET:
        codePage = THAI_CODEPAGE;
        break;
    case ANSI_CHARSET:
    case SYMBOL_CHARSET:
        codePage = ANSI_CODEPAGE; // ANSI
        break;
    default:
        assert(L"Unidentified charset");
        break;
    }

    return codePage;
}

int 
CommonTextCodePage::CharSetToCodePageIndex(int charSet)
{
    switch (charSet)
    {
    case JAPANESE_CHARSET:
        return (int)CommonTextMultiLanguageHandlerImpl::CodePageIndex::kJAPANESE_CODEPAGE_INDEX;
    case CHINESETRAD_CHARSET:
        return (int)CommonTextMultiLanguageHandlerImpl::CodePageIndex::kCHINESETRAD_CODEPAGE_INDEX;
    case KOREAN_CHARSET:
        return (int)CommonTextMultiLanguageHandlerImpl::CodePageIndex::kKOREAN_CODEPAGE_INDEX;
    case JOHAB_CHARSET:
        return (int)CommonTextMultiLanguageHandlerImpl::CodePageIndex::kJOHAB_CODEPAGE_INDEX;
    case CHINESESIMP_CHARSET:
        return (int)CommonTextMultiLanguageHandlerImpl::CodePageIndex::kCHINESESIMP_CODEPAGE_INDEX;
    default:
        return (int)CommonTextMultiLanguageHandlerImpl::CodePageIndex::kUNDEFINED_CODEPAGE_INDEX;
    }
}

int 
CommonTextCodePage::CharSetToCodePageId(int charSet)
{
    switch (charSet)
    {
    case EASTEUROPE_CHARSET:
        return CODE_PAGE_ANSI_1250;
    case RUSSIAN_CHARSET:
        return CODE_PAGE_ANSI_1251;
    case ANSI_CHARSET:
    case SYMBOL_CHARSET:
        return CODE_PAGE_ANSI_1252;
    case GREEK_CHARSET:
        return CODE_PAGE_ANSI_1253;
    case TURKISH_CHARSET:
        return CODE_PAGE_ANSI_1254;
    case HEBREW_CHARSET:
        return CODE_PAGE_ANSI_1255;
    case ARABIC_CHARSET:
        return CODE_PAGE_ANSI_1256;
    case BALTIC_CHARSET:
        return CODE_PAGE_ANSI_1257;
    case THAI_CHARSET:
        return CODE_PAGE_ANSI_874;
    case JAPANESE_CHARSET:
        return CODE_PAGE_ANSI_932;
    case CHINESESIMP_CHARSET:
        return CODE_PAGE_ANSI_936;
    case KOREAN_CHARSET:
        return CODE_PAGE_ANSI_949;
    case CHINESETRAD_CHARSET:
        return CODE_PAGE_ANSI_950;
    case JOHAB_CHARSET:
        return CODE_PAGE_ANSI_1361;
    case VIETNAMESE_CHARSET:
        return CODE_PAGE_ANSI_1258;
    default:
        assert(L"Unidentified code page");
        return CODE_PAGE_UNDEFINED;
    }
}

int 
CommonTextCodePage::CharSetToLCID(int charSet)
{
    // WARNING! Not a 1-1 mapping; simply picking the "best" LCID
    int lcid = ANSI_LCID;

    switch (charSet)
    {
    case EASTEUROPE_CHARSET:
        lcid = EASTEUROPE_LCID;
        break;
    case RUSSIAN_CHARSET:
        lcid = RUSSIAN_LCID;
        break;
    case HEBREW_CHARSET:
        lcid = HEBREW_LCID;
        break;
    case ARABIC_CHARSET:
        lcid = ARABIC_LCID;
        break;
    case BALTIC_CHARSET:
        lcid = BALTIC_LCID;
        break;
    case GREEK_CHARSET:
        lcid = GREEK_LCID;
        break;
    case TURKISH_CHARSET:
        lcid = TURKISH_LCID;
        break;
    case VIETNAMESE_CHARSET:
        lcid = VIETNAMESE_LCID;
        break;
    case JAPANESE_CHARSET:
        lcid = JAPANESE_LCID;
        break;
    case KOREAN_CHARSET:
        lcid = KOREAN_LCID;
        break;
    case CHINESESIMP_CHARSET:
        lcid = CHINESESIMP_LCID;
        break;
    case CHINESETRAD_CHARSET:
        lcid = CHINESETRAD_LCID;
        break;
    case ANSI_CHARSET:
    case SYMBOL_CHARSET:
        lcid = ANSI_LCID;
        break;
    case JOHAB_CHARSET:
        lcid = KOREAN_LCID; // JOHAB_LCID;
        break;
    case THAI_CHARSET:
        lcid = THAI_LCID;
        break;
    case BENGALI_CHARSET:
        lcid = MAKELANGID(LANG_BENGALI, SUBLANG_DEFAULT);
        break;
    case GUJARATI_CHARSET:
        lcid = MAKELANGID(LANG_GUJARATI, SUBLANG_DEFAULT);
        break;
    case TAMIL_CHARSET:
        lcid = MAKELANGID(LANG_TAMIL, SUBLANG_DEFAULT);
        break;
    case TELUGU_CHARSET:
        lcid = MAKELANGID(LANG_TELUGU, SUBLANG_DEFAULT);
        break;
    case KANNADA_CHARSET:
        lcid = MAKELANGID(LANG_KANNADA, SUBLANG_DEFAULT);
        break;
    case MALAYALAM_CHARSET:
        lcid = MAKELANGID(LANG_MALAYALAM, SUBLANG_DEFAULT);
        break;
    case DEVANAGARI_CHARSET:
        // case MARATHI_CHARSET:
        // case HINDI_CHARSET:
        // case KONKANI_CHARSET:
        // case SANSKRIT_CHARSET:
        lcid = MAKELANGID(LANG_MARATHI, SUBLANG_DEFAULT);
        break;
    case GURMUKHI_CHARSET:
        // case PUNJABI_CHARSET:
        lcid = MAKELANGID(LANG_PUNJABI, SUBLANG_DEFAULT);
        break;
    case ORIYA_CHARSET:
        lcid = MAKELANGID(LANG_ORIYA, SUBLANG_DEFAULT);
        break;
    default:
        assert(L"Unidentified charset");
    }

    return lcid;
}

short 
CommonTextCodePage::CharSetToLanguage(int charSet)
{
    return (short)CharSetToLCID(charSet);
}

bool 
CommonTextCodePage::CharSetIsDoubleByte(int charSet)
{
    return (charSet == JAPANESE_CHARSET   // Shift-JIS (Japanese)
        || charSet == CHINESETRAD_CHARSET // Big-5 (Traditional Chinese)
        || charSet == KOREAN_CHARSET      // KS C-5601-1987 (Wansung)
        || charSet == JOHAB_CHARSET       // KS C-5601-1992 (Johab)
        || charSet == CHINESESIMP_CHARSET // GB 2312-80 (Simplified Chinese)
    );
}

// codePage...

int 
CommonTextCodePage::CodePageToCharSet(int codePage)
{
    int charSet;

    switch (codePage)
    {
    case EASTEUROPE_CODEPAGE:
        charSet = EASTEUROPE_CHARSET;
        break;
    case RUSSIAN_CODEPAGE:
        charSet = RUSSIAN_CHARSET;
        break;
    case HEBREW_CODEPAGE:
        charSet = HEBREW_CHARSET;
        break;
    case ARABIC_CODEPAGE:
        charSet = ARABIC_CHARSET;
        break;
    case BALTIC_CODEPAGE:
        charSet = BALTIC_CHARSET;
        break;
    case GREEK_CODEPAGE:
        charSet = GREEK_CHARSET;
        break;
    case TURKISH_CODEPAGE:
        charSet = TURKISH_CHARSET;
        break;
    case VIETNAMESE_CODEPAGE:
        charSet = VIETNAMESE_CHARSET;
        break;
    case JAPANESE_CODEPAGE:
        charSet = JAPANESE_CHARSET;
        break;
    case KOREAN_CODEPAGE:
        charSet = KOREAN_CHARSET;
        break;
    case JOHAB_CODEPAGE:
        charSet = JOHAB_CHARSET;
        break;
    case CHINESESIMP_CODEPAGE:
        charSet = CHINESESIMP_CHARSET;
        break;
    case CHINESETRAD_CODEPAGE:
        charSet = CHINESETRAD_CHARSET;
        break;
    case THAI_CODEPAGE:
        charSet = THAI_CHARSET;
        break;
    case ANSI_CODEPAGE:
    default:
        charSet = ANSI_CHARSET;
        break;
    }

    return charSet;
}

int 
CommonTextCodePage::CodePageToCodePageIndex(int codePage)
{
    return CharSetToCodePageIndex(CodePageToCharSet(codePage));
}

int 
CommonTextCodePage::CodePageToCodePageId(int codePage)
{
    return CharSetToCodePageId(CodePageToCharSet(codePage));
}

int 
CommonTextCodePage::CodePageToLCID(int codePage)
{
    return CharSetToLCID(CodePageToCharSet(codePage));
}

short 
CommonTextCodePage::CodePageToLanguage(int codePage)
{
    return (short)CodePageToLCID(codePage);
}

bool
CommonTextCodePage::CodePageIsDoubleByte(int codePage)
{
    return CharSetIsDoubleByte(CodePageToCharSet(codePage));
}

// codePageId...

int
CommonTextCodePage::CodePageIdToCharSet(int codePageId)
{
    return CodePageToCharSet(CodePageIdToCodePage(codePageId));
}

int
CommonTextCodePage::CodePageIdToCodePage(int codePageId)
{
    return WinCodePageFromId(codePageId);
}

int
CommonTextCodePage::CodePageIdToCodePageIndex(int codePageId)
{
    return CharSetToCodePageIndex(CodePageIdToCharSet(codePageId));
}

int
CommonTextCodePage::CodePageIdToLCID(int codePageId)
{
    return CharSetToLCID(CodePageIdToCharSet(codePageId));
}

short
CommonTextCodePage::CodePageIdToLanguage(int codePageId)
{
    return (short)CodePageIdToLCID(codePageId);
}

bool 
CommonTextCodePage::CodePageIdIsDoubleByte(int codePageId)
{
    return CharSetIsDoubleByte(CodePageIdToCharSet(codePageId));
}

// codePageIndex...

int
CommonTextCodePage::CodePageIndexToCharSet(int codePageIndex)
{
    switch (codePageIndex)
    {
    case (int)CommonTextMultiLanguageHandlerImpl::CodePageIndex::kJAPANESE_CODEPAGE_INDEX:
        return JAPANESE_CHARSET;
    case (int)CommonTextMultiLanguageHandlerImpl::CodePageIndex::kCHINESETRAD_CODEPAGE_INDEX:
        return CHINESETRAD_CHARSET;
    case (int)CommonTextMultiLanguageHandlerImpl::CodePageIndex::kKOREAN_CODEPAGE_INDEX:
        return KOREAN_CHARSET;
    case (int)CommonTextMultiLanguageHandlerImpl::CodePageIndex::kJOHAB_CODEPAGE_INDEX:
        return JOHAB_CHARSET;
    case (int)CommonTextMultiLanguageHandlerImpl::CodePageIndex::kCHINESESIMP_CODEPAGE_INDEX:
        return CHINESESIMP_CHARSET;
    case (int)CommonTextMultiLanguageHandlerImpl::CodePageIndex::kUNDEFINED_CODEPAGE_INDEX:
    default:
        assert(L"Unidentified charset");
        return ANSI_CHARSET;
    }
}

int
CommonTextCodePage::CodePageIndexToCodePage(int codePageIndex)
{
    return CharSetToCodePage(CodePageIndexToCharSet(codePageIndex));
}

int
CommonTextCodePage::CodePageIndexToCodePageId(int codePageIndex)
{
    return CharSetToCodePageId(CodePageIndexToCharSet(codePageIndex));
}

int
CommonTextCodePage::CodePageIndexToLCID(int codePageIndex)
{
    return CharSetToLCID(CodePageIndexToCharSet(codePageIndex));
}

short
CommonTextCodePage::CodePageIndexToLanguage(int codePageIndex)
{
    return (short)CodePageIndexToLCID(codePageIndex);
}

bool
CommonTextCodePage::CodePageIndexIsDoubleByte(int codePageIndex)
{
    return CharSetIsDoubleByte(CodePageIndexToCharSet(codePageIndex));
}

// LCID...
int 
CommonTextCodePage::LCIDCount()
{
    return _iLCIDCount;
}

long
CommonTextCodePage::LCIDEntry(int i)
{
    if (i < LCIDCount())
        return _LCIDAndCharSetArray[i]._lcid;

    assert(i < LCIDCount());
    return _LCIDAndCharSetArray[0]._lcid;
}

int 
CommonTextCodePage::LCIDToCharSet(long lcid)
{
    for (int i = 0; i < LCIDCount(); i++)
    {
        if (_LCIDAndCharSetArray[i]._lcid == lcid)
            return _LCIDAndCharSetArray[i]._charset;
    }

    CHARSETINFO csi;

    if (TranslateCharsetInfo((LPDWORD)(DWORD_PTR)lcid, &csi, TCI_SRCLOCALE))
    {
        wchar_t cpbuf[UNICODEUSERDBCSMAPSIZE];

        if (GetLocaleInfo(lcid, LOCALE_IDEFAULTANSICODEPAGE, cpbuf, ACCP_NELTS(cpbuf)) > 0)
        {
            bool unicode = wtoi(cpbuf) == 0;
            int charset  = unicode ? UNICODE_CHARSET : csi.ciCharset;

            if (_iLCIDCount < ACCP_NELTS(_LCIDAndCharSetArray))
            {
                _LCIDAndCharSetArray[_iLCIDCount]._charset = charset;
                _iLCIDCount++;
            }

            return charset;
        }
    }

    assert(
        L"Unidentified LCID"); // return
                               // codePageIdToCharSet(acdbHostApplicationServices()->getSystemCodePage());
    return 0;
}

int 
CommonTextCodePage::LCIDToCodePage(long lcid)
{
    return CharSetToCodePage(LCIDToCharSet(lcid));
}

int 
CommonTextCodePage::LCIDToCodePageId(long lcid)
{
    return CharSetToCodePageId(LCIDToCharSet(lcid));
}

int
CommonTextCodePage::LCIDToCodePageIndex(long lcid)
{
    return CharSetToCodePageIndex(LCIDToCharSet(lcid));
}

short
CommonTextCodePage::LCIDToLanguage(long lcid)
{
    return MAKELANGID(PRIMARYLANGID(lcid), SUBLANGID(lcid));
}

bool
CommonTextCodePage::LCIDIsDoubleByte(long lcid)
{
    return CharSetIsDoubleByte(LCIDToCharSet(lcid));
}

// Language...

int 
CommonTextCodePage::LanguageToCharSet(short language)
{
    return LCIDToCharSet(language);
}

int
CommonTextCodePage::LanguageToCodePage(short language)
{
    return LCIDToCodePage(language);
}

int
CommonTextCodePage::LanguageToCodePageId(short language)
{
    return LCIDToCodePageId(language);
}

int
CommonTextCodePage::LanguageToCodePageIndex(short language)
{
    return LCIDToCodePageIndex(language);
}

int
CommonTextCodePage::LanguageToLCID(short language)
{
    return MAKELCID(language, SORT_DEFAULT);
}

bool
CommonTextCodePage::LanguageIsDoubleByte(short language)
{
    return LCIDIsDoubleByte(language);
}

bool
CommonTextCodePage::LanguageIsRtoL(short language)
{
    return PRIMARYLANGID(language) == LANG_HEBREW || PRIMARYLANGID(language) == LANG_ARABIC;
}

int CommonTextCodePage::_langCharsets[] = {
    ANSI_CHARSET,
    EASTEUROPE_CHARSET,
    RUSSIAN_CHARSET,
    BALTIC_CHARSET,
    GREEK_CHARSET,
    TURKISH_CHARSET,
    HEBREW_CHARSET,
    ARABIC_CHARSET,
    VIETNAMESE_CHARSET,
    THAI_CHARSET,
    CHINESESIMP_CHARSET,
    JOHAB_CHARSET,
    KOREAN_CHARSET,
    CHINESETRAD_CHARSET,
    JAPANESE_CHARSET,

    BENGALI_CHARSET,
    GURMUKHI_CHARSET,
    GUJARATI_CHARSET,
    TAMIL_CHARSET,
    TELUGU_CHARSET,
    KANNADA_CHARSET,
    MALAYALAM_CHARSET,
    DEVANAGARI_CHARSET,

    MARATHI_CHARSET,
    HINDI_CHARSET,
    KONKANI_CHARSET,
    SANSKRIT_CHARSET,

    PUNJABI_CHARSET,

    ORIYA_CHARSET,
};

short
CommonTextCodePage::LanguageFromUnicode(WCHAR wch,
                                        short def_lang)
{
    int lf = _langFlags[_langIdx[wch]];

    // Special handling for the Greeks.
    // Because greeks are not supported in the ansi shx font.
    if (lf == 0x74d1)
        return CharSetToLanguage(GREEK_CHARSET);

    if (lf)
    {
        int best_index  = -1;
        int def_charset = LanguageToCharSet(def_lang);

        for (int l = 0; l < ACCP_NELTS(_langCharsets); l++)
        {
            if (lf & (1 << l))
            {
                if (def_charset == _langCharsets[l])
                    return def_lang;
                if (best_index == -1)
                {
                    best_index = l;
                }
            }
        }

        if (best_index != -1)
            return CharSetToLanguage(_langCharsets[best_index]);
    }

    return def_lang;
}

#define ULB_JL ULB_ID
#define ULB_JV ULB_ID
#define ULB_JT ULB_ID
#define ULB_SG ULB_ID
#define ULB_H2 ULB_ID
#define ULB_H3 ULB_ID

//#define ACCP_GENERATE_LINEBREAK

const int CommonTextCodePage::_directLineBreakClassArrayCount   = ACCP_NELTS(_directLineBreakClass);
const int CommonTextCodePage::_indirectLineBreakClassArrayCount = ACCP_NELTS(_indirectLineBreakClass);

typedef CommonTextMultiLanguageHandlerImpl LineBreakClass;

#undef PBK
#define PBK LineBreakClass::ULB_PBK
#undef CBK
#define CBK LineBreakClass::ULB_CBK
#undef IBK
#define IBK LineBreakClass::ULB_IBK
#undef DBK
#define DBK LineBreakClass::ULB_DBK

const char CommonTextCodePage::_lineBreakPairs[LineBreakClass::ULB_WJ + 1][LineBreakClass::ULB_WJ + 1] = {
    /* OP   CL   QU   GL   NS   EX   SY   IS   PR   PO   NU   AL   ID   IN   HY   BA   BB   B2
       ZW   CM   WJ */
    /*OP*/ {
        PBK,
        PBK,
        PBK,
        PBK,
        PBK,
        PBK,
        PBK,
        PBK,
        PBK,
        PBK,
        PBK,
        PBK,
        PBK,
        PBK,
        PBK,
        PBK,
        PBK,
        PBK,
        PBK,
        PBK,
        PBK,
    },
    /*CL*/
    {
        DBK,
        PBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
        PBK,
        DBK,
        IBK,
        DBK,
        DBK,
        DBK,
        DBK,
        IBK,
        IBK,
        DBK,
        DBK,
        PBK,
        CBK,
        PBK,
    },
    /*QU*/
    {
        PBK,
        PBK,
        IBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
    },
    /*GL*/
    {
        IBK,
        PBK,
        IBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
    },
    /*NS*/
    {
        DBK,
        PBK,
        IBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        IBK,
        IBK,
        DBK,
        DBK,
        PBK,
        CBK,
        PBK,
    },
    /*EX*/
    {
        DBK,
        PBK,
        IBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        IBK,
        IBK,
        DBK,
        DBK,
        PBK,
        CBK,
        PBK,
    },
    /*SY*/
    {
        DBK,
        PBK,
        IBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
        DBK,
        DBK,
        IBK,
        DBK,
        DBK,
        DBK,
        IBK,
        IBK,
        DBK,
        DBK,
        PBK,
        CBK,
        PBK,
    },
    /*IS*/
    {
        DBK,
        PBK,
        IBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
        DBK,
        DBK,
        IBK,
        IBK,
        DBK,
        DBK,
        IBK,
        IBK,
        DBK,
        DBK,
        PBK,
        CBK,
        PBK,
    },
    /*PR*/
    {
        IBK,
        PBK,
        IBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
        DBK,
        DBK,
        IBK,
        IBK,
        IBK,
        DBK,
        IBK,
        IBK,
        DBK,
        DBK,
        PBK,
        PBK,
        PBK,
    },
    /*PO*/
    {
        DBK,
        PBK,
        IBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        IBK,
        IBK,
        DBK,
        DBK,
        PBK,
        CBK,
        PBK,
    },
    /*NU*/
    {
        DBK,
        PBK,
        IBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
        DBK,
        IBK,
        IBK,
        IBK,
        DBK,
        IBK,
        IBK,
        IBK,
        DBK,
        DBK,
        PBK,
        CBK,
        PBK,
    },
    /*AL*/
    {
        DBK,
        PBK,
        IBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
        DBK,
        DBK,
        IBK,
        IBK,
        DBK,
        IBK,
        IBK,
        IBK,
        DBK,
        DBK,
        PBK,
        CBK,
        PBK,
    },
    /*ID*/
    {
        DBK,
        PBK,
        IBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
        DBK,
        IBK,
        DBK,
        DBK,
        DBK,
        IBK,
        IBK,
        IBK,
        DBK,
        DBK,
        PBK,
        CBK,
        PBK,
    },
    /*IN*/
    {
        DBK,
        PBK,
        IBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        IBK,
        IBK,
        IBK,
        DBK,
        DBK,
        PBK,
        CBK,
        PBK,
    },
    /*HY*/
    {
        DBK,
        PBK,
        IBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
        DBK,
        DBK,
        IBK,
        DBK,
        DBK,
        DBK,
        IBK,
        IBK,
        DBK,
        DBK,
        PBK,
        CBK,
        PBK,
    },
    /*BA*/
    {
        DBK,
        PBK,
        IBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        IBK,
        IBK,
        DBK,
        DBK,
        PBK,
        CBK,
        PBK,
    },
    /*BB*/
    {
        IBK,
        PBK,
        IBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
    },
    /*B2*/
    {
        DBK,
        PBK,
        IBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        IBK,
        IBK,
        DBK,
        PBK,
        PBK,
        CBK,
        PBK,
    },
    /*ZW*/
    {
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        DBK,
        PBK,
        CBK,
        DBK,
    },
    /*CM*/
    {
        DBK,
        PBK,
        IBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
        DBK,
        DBK,
        IBK,
        IBK,
        DBK,
        IBK,
        IBK,
        IBK,
        DBK,
        DBK,
        PBK,
        CBK,
        PBK,
    },
    /*WJ*/
    {
        IBK,
        PBK,
        IBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        IBK,
        PBK,
        PBK,
        PBK,
    }
};

// placeholder function for complex break analysis
int 
CommonTextCodePage::_AnalyzeComplexLineBreaks(
    int* pcls, 
    int* /*pbrk*/, 
    int cch)
{
    if (!cch)
        return 0;
    // int cls = pcls[0];
    int ich;
    for (ich = 0; ich < cch; ich++)
    {
        // Do complex break analysis here
        // and report any break opportunities in pbrk ..

        if (pcls[ich] != LineBreakClass::ULB_SA)
            break;
    }
    return ich;
}

// pcls - pointer to array of line breaking classes (input)
// pbrk - pointer to array of line break opportunities (output)
// cch - number of elements in the arrays (count of characters) (input)
// ich - current index into the arrays (variable)
int 
CommonTextCodePage::AnalyzeLineBreaks(
    int* pcls, 
    int* pbrk, 
    int cch)
{
    if (!cch)
        return 0;

    int cls = pcls[0];
    assert(cls != LineBreakClass::ULB_AI);

    // loop over all pairs in the string up to a hard break
    int ich;
    for (ich = 1; (ich < cch) && (cls != LineBreakClass::ULB_BK); ich++)
    {

        assert(pcls[ich] != LineBreakClass::ULB_AI);
        // handle spaces
        if (pcls[ich] == LineBreakClass::ULB_SP)
        {
            pbrk[ich - 1] = LineBreakClass::ULB_PBK;
            continue;
        }

        // handle combining marks
        if (pcls[ich] == LineBreakClass::ULB_CM)
        {
            if (pcls[ich - 1] == LineBreakClass::ULB_SP)
            {
                cls = LineBreakClass::ULB_ID;
                if (ich > 1)
                    pbrk[ich - 2] = _lineBreakPairs[pcls[ich - 2]][LineBreakClass::ULB_ID] ==
                            LineBreakClass::ULB_DBK
                        ? LineBreakClass::ULB_DBK
                        : LineBreakClass::ULB_PBK;
            }
            pbrk[ich - 1] = LineBreakClass::ULB_PBK;
            continue;
        }

        // handle complex scripts
        if (pcls[ich] == LineBreakClass::ULB_SA)
        {
            ich += _AnalyzeComplexLineBreaks(&pcls[ich - 1], &pbrk[ich - 1], cch - (ich - 1));
            if (ich < cch)
                cls = pcls[ich];
            continue;
        }

        // lookup pair table information
        int brk = _lineBreakPairs[cls][pcls[ich]];

        if (brk == LineBreakClass::ULB_IBK)
        {
            pbrk[ich - 1] = ((pcls[ich - 1] == LineBreakClass::ULB_SP) ? LineBreakClass::ULB_IBK
                                                                       : LineBreakClass::ULB_PBK);
        }
        else if (brk == LineBreakClass::ULB_CBK)
        {
            if (ich > 1 && (pcls[ich - 1] == LineBreakClass::ULB_SP))
                pbrk[ich - 2] =
                    ((pcls[ich - 2] == LineBreakClass::ULB_SP) ? LineBreakClass::ULB_IBK
                                                               : LineBreakClass::ULB_DBK);
            pbrk[ich - 1] = LineBreakClass::ULB_PBK;
        }
        else
        {
            pbrk[ich - 1] = brk;
        }
        cls = pcls[ich];
    }
    // always break at the end
    pbrk[ich - 1] = LineBreakClass::ULB_DBK;

    return ich;
}

// Support for mapping a DBCS code that is not in the range of valid DBCS (e.g. custom
// codes defined in a bigfont (536437).  When these codes are used by ::MultiByteCIFToWideChar
// invalid is returned and the unicode character returned is 30fb.
//
// This table maps these invalid DBCS codes to user defined Unicode values starting at 0xe800
// so that Unicode values can be used.
//
// The table size is 256 (UNICODEUSERDBCSMAPSIZE)
//
bool 
CommonTextCodePage::UnicodeForUserDefinedDBCS(
    wchar_t& unicodeValue, 
    wchar_t DBCSCode, 
    int codepage)
{
    int i = 0;
    for (i = 0; i < UNICODEUSERDBCSMAPSIZE && _unicodeUserDBCSMap[i]._DBCSCode != 0; i++)
    {
        if (_unicodeUserDBCSMap[i]._DBCSCode == DBCSCode)
        {
            unicodeValue = _unicodeUserDBCSMap[i]._unicodeValue;
            return true;
        }
    }

    if (i == UNICODEUSERDBCSMAPSIZE)
        return false;

    _unicodeUserDBCSMap[i]._DBCSCode = DBCSCode;
    _unicodeUserDBCSMap[i]._codepage = codepage;
    unicodeValue                     = _unicodeUserDBCSMap[i]._unicodeValue;
    return true;
}

bool 
CommonTextCodePage::DBCSForUserDefinedUnicode(
    wchar_t& dbcs, 
    wchar_t unicodeValue)
{
    bool retValue = true;
    if (unicodeValue >= unicodeUserDefinedDBCSMapStart &&
        unicodeValue < (unicodeUserDefinedDBCSMapStart + UNICODEUSERDBCSMAPSIZE))
        dbcs = _unicodeUserDBCSMap[unicodeValue - unicodeUserDefinedDBCSMapStart]._DBCSCode;
    else
        retValue = false;

    return retValue;
}

bool 
CommonTextCodePage::CodepageForUserDefinedUnicode(
    int& codePage, 
    wchar_t unicodeValue)
{
    bool retValue = true;
    if (unicodeValue >= unicodeUserDefinedDBCSMapStart &&
        unicodeValue < (unicodeUserDefinedDBCSMapStart + UNICODEUSERDBCSMAPSIZE))
        codePage = _unicodeUserDBCSMap[unicodeValue - unicodeUserDefinedDBCSMapStart]._codepage;
    else
        retValue = false;

    return retValue;
}

const wchar_t* 
CommonTextCodePage::MapFontFromCharset(int charset)
{
    // no acceptable match; find a font that supports the character
    static const struct IPELanguageFont
    {
        int charset;
        const wchar_t* szTTFont;
    } IPELanguageFonts[] = {
        { JAPANESE_CHARSET, L"MS PGothic"},
        { KOREAN_CHARSET, L"Gulim" },
        { CHINESESIMP_CHARSET, L"SimSun" },
        { CHINESETRAD_CHARSET, L"PMingLiU" },
        { THAI_CHARSET, L"Cordia New" },
        { GREEK_CHARSET, L"Arial"},
        { BENGALI_CHARSET, L"Vrinda" },
        { PUNJABI_CHARSET, L"Raavi" },
        { GUJARATI_CHARSET, L"Shruti" },
        { TAMIL_CHARSET, L"Latha" },
        { TELUGU_CHARSET, L"Gautami" },
        { KANNADA_CHARSET, L"Tunga"},
        { MARATHI_CHARSET, L"Mangal" },
        { HINDI_CHARSET, L"Mangal" },
        { MALAYALAM_CHARSET, L"Kartika" },
        { ORIYA_CHARSET, L"Sendnya" },
    };

    for (int i = 0; i < ACCP_NELTS(IPELanguageFonts); i++)
    {
        if (IPELanguageFonts[i].charset == charset)
        {
            return IPELanguageFonts[i].szTTFont;
        }
    }

    return L"Arial";
}

CommonTextCodePage acCodePage;

PXR_NAMESPACE_CLOSE_SCOPE

