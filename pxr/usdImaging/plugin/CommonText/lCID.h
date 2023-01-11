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

#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_LCID_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_LCID_H

#include "definitions.h"
#include "codePageEnums.h"

PXR_NAMESPACE_OPEN_SCOPE
CommonTextCodePage::CommonTextLCIDAndCharSet CommonTextCodePage::_LCIDAndCharSetArray[256] = {
    {
        0x0401,
        ARABIC_CHARSET,
    }, // Arabic (Saudi Arabia)
    {
        0x0801,
        ARABIC_CHARSET,
    }, // Arabic (Iraq)
    {
        0x0C01,
        ARABIC_CHARSET,
    }, // Arabic (Egypt)
    {
        0x1001,
        ARABIC_CHARSET,
    }, // Arabic (Libya)
    {
        0x1401,
        ARABIC_CHARSET,
    }, // Arabic (Algeria)
    {
        0x1801,
        ARABIC_CHARSET,
    }, // Arabic (Morocco)
    {
        0x1C01,
        ARABIC_CHARSET,
    }, // Arabic (Tunisia)
    {
        0x2001,
        ARABIC_CHARSET,
    }, // Arabic (Oman)
    {
        0x2401,
        ARABIC_CHARSET,
    }, // Arabic (Yemen)
    {
        0x2801,
        ARABIC_CHARSET,
    }, // Arabic (Syria)
    {
        0x2C01,
        ARABIC_CHARSET,
    }, // Arabic (Jordan)
    {
        0x3001,
        ARABIC_CHARSET,
    }, // Arabic (Lebanon)
    {
        0x3401,
        ARABIC_CHARSET,
    }, // Arabic (Kuwait)
    {
        0x3801,
        ARABIC_CHARSET,
    }, // Arabic (U.A.E.)
    {
        0x3C01,
        ARABIC_CHARSET,
    }, // Arabic (Bahrain)
    {
        0x4001,
        ARABIC_CHARSET,
    }, // Arabic (Qatar)
    {
        0x0402,
        RUSSIAN_CHARSET,
    }, // Bulgarian
    {
        0x0403,
        ANSI_CHARSET,
    }, // Catalan
    {
        0x0404,
        CHINESETRAD_CHARSET,
    }, // Chinese (Taiwan)
    {
        0x0804,
        CHINESESIMP_CHARSET,
    }, // Chinese (PRC)
    {
        0x0C04,
        CHINESESIMP_CHARSET,
    }, // Chinese (Hong Kong S.A.R.)
    {
        0x1004,
        CHINESESIMP_CHARSET,
    }, // Chinese (Singapore)
    {
        0x1404,
        CHINESETRAD_CHARSET,
    }, // Chinese (Macau S.A.R.)
    {
        0x0405,
        EASTEUROPE_CHARSET,
    }, // Czech
    {
        0x0406,
        ANSI_CHARSET,
    }, // Danish
    {
        0x0407,
        ANSI_CHARSET,
    }, // German (Germany)
    {
        0x0807,
        ANSI_CHARSET,
    }, // German (Switzerland)
    {
        0x0C07,
        ANSI_CHARSET,
    }, // German (Austria)
    {
        0x1007,
        ANSI_CHARSET,
    }, // German (Luxembourg)
    {
        0x1407,
        ANSI_CHARSET,
    }, // German (Liechtenstein)
    {
        0x0408,
        GREEK_CHARSET,
    }, // Greek
    {
        0x0409,
        ANSI_CHARSET,
    }, // English (United States)
    {
        0x0809,
        ANSI_CHARSET,
    }, // English (United Kingdom)
    {
        0x0C09,
        ANSI_CHARSET,
    }, // English (Australia)
    {
        0x1009,
        ANSI_CHARSET,
    }, // English (Canada)
    {
        0x1409,
        ANSI_CHARSET,
    }, // English (New Zealand)
    {
        0x1809,
        ANSI_CHARSET,
    }, // English (Ireland)
    {
        0x1C09,
        ANSI_CHARSET,
    }, // English (South Africa)
    {
        0x2009,
        ANSI_CHARSET,
    }, // English (Jamaica)
    {
        0x2409,
        ANSI_CHARSET,
    }, // English (Caribbean)
    {
        0x2809,
        ANSI_CHARSET,
    }, // English (Belize)
    {
        0x2C09,
        ANSI_CHARSET,
    }, // English (Trinidad)
    {
        0x3009,
        ANSI_CHARSET,
    }, // English (Zimbabwe)
    {
        0x3409,
        ANSI_CHARSET,
    }, // English (Philippines)
    {
        0x040A,
        ANSI_CHARSET,
    }, // Spanish (Traditional Sort)
    {
        0x080A,
        ANSI_CHARSET,
    }, // Spanish (Mexico)
    {
        0x0C0A,
        ANSI_CHARSET,
    }, // Spanish (International Sort)
    {
        0x100A,
        ANSI_CHARSET,
    }, // Spanish (Guatemala)
    {
        0x140A,
        ANSI_CHARSET,
    }, // Spanish (Costa Rica)
    {
        0x180A,
        ANSI_CHARSET,
    }, // Spanish (Panama)
    {
        0x1C0A,
        ANSI_CHARSET,
    }, // Spanish (Dominican Republic)
    {
        0x200A,
        ANSI_CHARSET,
    }, // Spanish (Venezuela)
    {
        0x240A,
        ANSI_CHARSET,
    }, // Spanish (Colombia)
    {
        0x280A,
        ANSI_CHARSET,
    }, // Spanish (Peru)
    {
        0x2C0A,
        ANSI_CHARSET,
    }, // Spanish (Argentina)
    {
        0x300A,
        ANSI_CHARSET,
    }, // Spanish (Ecuador)
    {
        0x340A,
        ANSI_CHARSET,
    }, // Spanish (Chile)
    {
        0x380A,
        ANSI_CHARSET,
    }, // Spanish (Uruguay)
    {
        0x3C0A,
        ANSI_CHARSET,
    }, // Spanish (Paraguay)
    {
        0x400A,
        ANSI_CHARSET,
    }, // Spanish (Bolivia)
    {
        0x440A,
        ANSI_CHARSET,
    }, // Spanish (El Salvador)
    {
        0x480A,
        ANSI_CHARSET,
    }, // Spanish (Honduras)
    {
        0x4C0A,
        ANSI_CHARSET,
    }, // Spanish (Nicaragua)
    {
        0x500A,
        ANSI_CHARSET,
    }, // Spanish (Puerto Rico)
    {
        0x040B,
        ANSI_CHARSET,
    }, // Finnish
    {
        0x040C,
        ANSI_CHARSET,
    }, // French (France)
    {
        0x080C,
        ANSI_CHARSET,
    }, // French (Belgium)
    {
        0x0C0C,
        ANSI_CHARSET,
    }, // French (Canada)
    {
        0x100C,
        ANSI_CHARSET,
    }, // French (Switzerland)
    {
        0x140C,
        ANSI_CHARSET,
    }, // French (Luxembourg)
    {
        0x180C,
        ANSI_CHARSET,
    }, // French (Monaco)
    {
        0x040D,
        HEBREW_CHARSET,
    }, // Hebrew
    {
        0x040E,
        EASTEUROPE_CHARSET,
    }, // Hungarian
    {
        0x040F,
        ANSI_CHARSET,
    }, // Icelandic
    {
        0x0410,
        ANSI_CHARSET,
    }, // Italian (Italy)
    {
        0x0810,
        ANSI_CHARSET,
    }, // Italian (Switzerland)
    {
        0x0411,
        JAPANESE_CHARSET,
    }, // Japanese
    {
        0x0412,
        KOREAN_CHARSET,
    }, // Korean
    {
        0x0413,
        ANSI_CHARSET,
    }, // Dutch (Netherlands)
    {
        0x0813,
        ANSI_CHARSET,
    }, // Dutch (Belgium)
    {
        0x0414,
        ANSI_CHARSET,
    }, // Norwegian (Bokmal)
    {
        0x0814,
        ANSI_CHARSET,
    }, // Norwegian (Nynorsk)
    {
        0x0415,
        EASTEUROPE_CHARSET,
    }, // Polish
    {
        0x0416,
        ANSI_CHARSET,
    }, // Portuguese (Brazil)
    {
        0x0816,
        ANSI_CHARSET,
    }, // Portuguese (Portugal)
    {
        0x0418,
        EASTEUROPE_CHARSET,
    }, // Romanian
    {
        0x0419,
        RUSSIAN_CHARSET,
    }, // Russian
    {
        0x041A,
        EASTEUROPE_CHARSET,
    }, // Croatian
    {
        0x081A,
        EASTEUROPE_CHARSET,
    }, // Serbian (Latin)
    {
        0x0C1A,
        RUSSIAN_CHARSET,
    }, // Serbian (Cyrillic)
    {
        0x101A,
        EASTEUROPE_CHARSET,
    }, // Croatian (Bosnia and Herzegovina)
    {
        0x141A,
        EASTEUROPE_CHARSET,
    }, // Bosnian (Latin, Bosnia and Herzegovina)
    {
        0x181A,
        EASTEUROPE_CHARSET,
    }, // Serbian (Latin, Bosnia and Herzegovina)
    {
        0x1C1A,
        RUSSIAN_CHARSET,
    }, // Serbian (Cyrillic, Bosnia and Herzegovina)
    {
        0x041B,
        EASTEUROPE_CHARSET,
    }, // Slovak
    {
        0x041C,
        EASTEUROPE_CHARSET,
    }, // Albanian
    {
        0x041D,
        ANSI_CHARSET,
    }, // Swedish
    {
        0x081D,
        ANSI_CHARSET,
    }, // Swedish (Finland)
    {
        0x041E,
        THAI_CHARSET,
    }, // Thai
    {
        0x041F,
        TURKISH_CHARSET,
    }, // Turkish
    {
        0x0420,
        ARABIC_CHARSET,
    }, // Urdu
    {
        0x0421,
        ANSI_CHARSET,
    }, // Indonesian
    {
        0x0422,
        RUSSIAN_CHARSET,
    }, // Ukrainian
    {
        0x0423,
        RUSSIAN_CHARSET,
    }, // Belarusian
    {
        0x0424,
        EASTEUROPE_CHARSET,
    }, // Slovenian
    {
        0x0425,
        BALTIC_CHARSET,
    }, // Estonian
    {
        0x0426,
        BALTIC_CHARSET,
    }, // Latvian
    {
        0x0427,
        BALTIC_CHARSET,
    }, // Lithuanian
    {
        0x0429,
        ARABIC_CHARSET,
    }, // Farsi
    {
        0x042A,
        VIETNAMESE_CHARSET,
    }, // Vietnamese
    {
        0x042B,
        UNICODE_CHARSET,
    }, // Armenian
    {
        0x042C,
        EASTEUROPE_CHARSET,
    }, // Azeri (Latin)
    {
        0x082C,
        RUSSIAN_CHARSET,
    }, // Azeri (Cyrillic)
    {
        0x042D,
        ANSI_CHARSET,
    }, // Basque
    {
        0x042F,
        RUSSIAN_CHARSET,
    }, // FYRO Macedonian
    {
        0x0432,
        ANSI_CHARSET,
    }, // Tswana
    {
        0x0434,
        ANSI_CHARSET,
    }, // Xhosa
    {
        0x0435,
        ANSI_CHARSET,
    }, // Zulu
    {
        0x0436,
        ANSI_CHARSET,
    }, // Afrikaans
    {
        0x0437,
        UNICODE_CHARSET,
    }, // Georgian
    {
        0x0438,
        ANSI_CHARSET,
    }, // Faeroese
    {
        0x0439,
        HINDI_CHARSET,
    }, // Hindi
    {
        0x043A,
        UNICODE_CHARSET,
    }, // Maltese
    {
        0x043B,
        ANSI_CHARSET,
    }, // Sami, Northern (Norway)
    {
        0x083B,
        ANSI_CHARSET,
    }, // Sami, Northern (Sweden)
    {
        0x0C3B,
        ANSI_CHARSET,
    }, // Sami, Northern (Finland)
    {
        0x103B,
        ANSI_CHARSET,
    }, // Sami, Lule (Norway)
    {
        0x143B,
        ANSI_CHARSET,
    }, // Sami, Lule (Sweden)
    {
        0x183B,
        ANSI_CHARSET,
    }, // Sami, Southern (Norway)
    {
        0x1C3B,
        ANSI_CHARSET,
    }, // Sami, Southern (Sweden)
    {
        0x203B,
        ANSI_CHARSET,
    }, // Sami, Skolt (Finland)
    {
        0x243B,
        ANSI_CHARSET,
    }, // Sami, Inari (Finland)
    {
        0x043E,
        ANSI_CHARSET,
    }, // Malay (Malaysia)
    {
        0x083E,
        ANSI_CHARSET,
    }, // Malay (Brunei Darussalam)
    {
        0x043F,
        RUSSIAN_CHARSET,
    }, // Kazakh
    {
        0x0440,
        RUSSIAN_CHARSET,
    }, // Kyrgyz (Cyrillic)
    {
        0x0441,
        ANSI_CHARSET,
    }, // Swahili
    {
        0x0443,
        EASTEUROPE_CHARSET,
    }, // Uzbek (Latin)
    {
        0x0843,
        RUSSIAN_CHARSET,
    }, // Uzbek (Cyrillic)
    {
        0x0444,
        RUSSIAN_CHARSET,
    }, // Tatar
    {
        0x0445,
        BENGALI_CHARSET,
    }, // Bengali (India)
    {
        0x0446,
        PUNJABI_CHARSET,
    }, // Punjabi
    {
        0x0447,
        GUJARATI_CHARSET,
    }, // Gujarati
    {
        0x0449,
        TAMIL_CHARSET,
    }, // Tamil
    {
        0x044A,
        TELUGU_CHARSET,
    }, // Telugu
    {
        0x044B,
        KANNADA_CHARSET,
    }, // Kannada
    {
        0x044C,
        MALAYALAM_CHARSET,
    }, // Malayalam (India)
    {
        0x044E,
        MARATHI_CHARSET,
    }, // Marathi
    {
        0x044F,
        SANSKRIT_CHARSET,
    }, // Sanskrit
    {
        0x0450,
        RUSSIAN_CHARSET,
    }, // Mongolian (Cyrillic)
    {
        0x0452,
        ANSI_CHARSET,
    }, // Welsh
    {
        0x0456,
        ANSI_CHARSET,
    }, // Galician
    {
        0x0457,
        KONKANI_CHARSET,
    }, // Konkani
    {
        0x045A,
        UNICODE_CHARSET,
    }, // Syriac
    {
        0x0465,
        UNICODE_CHARSET,
    }, // Divehi
    {
        0x046B,
        ANSI_CHARSET,
    }, // Quechua (Bolivia)
    {
        0x086B,
        ANSI_CHARSET,
    }, // Quechua (Ecuador)
    {
        0x0C6B,
        ANSI_CHARSET,
    }, // Quechua (Peru)
    {
        0x046C,
        ANSI_CHARSET,
    }, // Northern Sotho
    {
        0x0481,
        UNICODE_CHARSET,
    }, // Maori
};

int CommonTextCodePage::_iLCIDCount = 160;
PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_LCID_H