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

#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_CODE_PAGE_ENUMS_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_CODE_PAGE_ENUMS_H

#include "Definitions.h"

#pragma pack(push, 8)

/// The codepages enumeration
PXR_NAMESPACE_OPEN_SCOPE
/// \enum CommonTextLanguageLocalId
///
/// The enumeration of languages.
///
enum CommonTextLanguageLocalId
{
    EASTEUROPE_LCID  = 0x405,
    RUSSIAN_LCID     = 0x419,
    HEBREW_LCID      = 0x40d,
    ARABIC_LCID      = 0x401,
    BALTIC_LCID      = 0x425,
    GREEK_LCID       = 0x408,
    TURKISH_LCID     = 0x41f,
    VIETNAMESE_LCID  = 0x42a,
    JAPANESE_LCID    = 0x411,
    KOREAN_LCID      = 0x412,
    CHINESESIMP_LCID = 0x804,
    CHINESETRAD_LCID = 0x404,
    ANSI_LCID        = 0x409,
    THAI_LCID        = 0x41e,
};

/// \enum CommonTextSystemCharset
///
/// The enumeration of charsets.
///
enum CommonTextSystemCharset
{
    JAPANESE_CHARSET    = SHIFTJIS_CHARSET,
    CHINESETRAD_CHARSET = CHINESEBIG5_CHARSET,
    CHINESESIMP_CHARSET = GB2312_CHARSET,
    KOREAN_CHARSET      = HANGEUL_CHARSET,

    UNICODE_CHARSET  = DEFAULT_CHARSET,
    INTERNAL_CHARSET = 256,

    BENGALI_CHARSET = INTERNAL_CHARSET,
    GURMUKHI_CHARSET,
    GUJARATI_CHARSET,
    TAMIL_CHARSET,
    TELUGU_CHARSET,
    KANNADA_CHARSET,
    MALAYALAM_CHARSET,
    DEVANAGARI_CHARSET,
    ORIYA_CHARSET, // = UNICODE_CHARSET, // FOR NOW!

    MARATHI_CHARSET  = DEVANAGARI_CHARSET,
    HINDI_CHARSET    = DEVANAGARI_CHARSET,
    KONKANI_CHARSET  = DEVANAGARI_CHARSET,
    SANSKRIT_CHARSET = DEVANAGARI_CHARSET,

    PUNJABI_CHARSET = GURMUKHI_CHARSET,

    ASSAMESE_CHARSET = UNICODE_CHARSET,
    FINNISH_CHARSET  = UNICODE_CHARSET,
    BELGIAN_CHARSET  = UNICODE_CHARSET,
    GEORGIAN_CHARSET = UNICODE_CHARSET,
};

/// \enum CommonTextCodePageEnum
///
/// The enumeration of codepage.
///
enum CommonTextCodePageEnum
{
    EASTEUROPE_CODEPAGE  = 1250,
    RUSSIAN_CODEPAGE     = 1251,
    HEBREW_CODEPAGE      = 1255,
    ARABIC_CODEPAGE      = 1256,
    BALTIC_CODEPAGE      = 1257,
    GREEK_CODEPAGE       = 1253,
    TURKISH_CODEPAGE     = 1254,
    VIETNAMESE_CODEPAGE  = 1258,
    JAPANESE_CODEPAGE    = 932,
    KOREAN_CODEPAGE      = 949,
    CHINESESIMP_CODEPAGE = 936,
    CHINESETRAD_CODEPAGE = 950,
    ANSI_CODEPAGE        = 1252,
    JOHAB_CODEPAGE       = 1361,
    THAI_CODEPAGE        = 874,
};

/// \enum CommonTextCodePageId
///
/// The enumeration of codepage id.
/// This list contains identifiers for all of the code pages used with
/// Autodesk.  You can add entries (before the CODE_PAGE_CNT item), but
/// don't ever delete one.
///
enum CommonTextCodePageId
{
    CODE_PAGE_UNDEFINED = 0,
    CODE_PAGE_ASCII,
    CODE_PAGE_8859_1,
    CODE_PAGE_8859_2,
    CODE_PAGE_8859_3,
    CODE_PAGE_8859_4,
    CODE_PAGE_8859_5,
    CODE_PAGE_8859_6,
    CODE_PAGE_8859_7,
    CODE_PAGE_8859_8,
    CODE_PAGE_8859_9,
    CODE_PAGE_DOS437,
    CODE_PAGE_DOS850,
    CODE_PAGE_DOS852,
    CODE_PAGE_DOS855,
    CODE_PAGE_DOS857,
    CODE_PAGE_DOS860,
    CODE_PAGE_DOS861,
    CODE_PAGE_DOS863,
    CODE_PAGE_DOS864,
    CODE_PAGE_DOS865,
    CODE_PAGE_DOS869,
    CODE_PAGE_DOS932,
    CODE_PAGE_MACINTOSH,
    CODE_PAGE_BIG5,
    CODE_PAGE_KSC5601,
    CODE_PAGE_JOHAB,
    CODE_PAGE_DOS866,
    CODE_PAGE_ANSI_1250,
    CODE_PAGE_ANSI_1251,
    CODE_PAGE_ANSI_1252,
    CODE_PAGE_GB2312,
    CODE_PAGE_ANSI_1253,
    CODE_PAGE_ANSI_1254,
    CODE_PAGE_ANSI_1255,
    CODE_PAGE_ANSI_1256,
    CODE_PAGE_ANSI_1257,
    CODE_PAGE_ANSI_874,
    CODE_PAGE_ANSI_932,
    CODE_PAGE_ANSI_936,
    CODE_PAGE_ANSI_949,
    CODE_PAGE_ANSI_950,
    CODE_PAGE_ANSI_1361,
    CODE_PAGE_ANSI_1200,
    CODE_PAGE_ANSI_1258,
    CODE_PAGE_CNT
};

#pragma pack(pop)

inline bool IsValidCodePageId(unsigned int value)
{
    return (value < CODE_PAGE_CNT);
}

PXR_NAMESPACE_CLOSE_SCOPE
#endif // PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_CODE_PAGE_ENUMS_H