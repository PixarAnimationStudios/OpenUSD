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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_LINE_BREAK_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_LINE_BREAK_H

#ifdef _ACCODEPAGE_CPP

#include "Definitions.h"
#include "multiLanguageHandlerImpl.h"

PXR_NAMESPACE_OPEN_SCOPE
typedef CommonTextMultiLanguageHandlerImpl LineBreakClass;
///
/// NOTE: Using Unicode version 5.0.0 version, see:
/// http://www.unicode.org/Public/5.0.0/ucd/LineBreak.txt
///

///  DESCRIPTION: The definition of the line break of each language
char CommonTextCodePage::_directLineBreakClass[] = {
    LineBreakClass::ULB_CM, // 0x0000  # <control>
    LineBreakClass::ULB_CM, // 0x0001  # <control>
    LineBreakClass::ULB_CM, // 0x0002  # <control>
    LineBreakClass::ULB_CM, // 0x0003  # <control>
    LineBreakClass::ULB_CM, // 0x0004  # <control>
    LineBreakClass::ULB_CM, // 0x0005  # <control>
    LineBreakClass::ULB_CM, // 0x0006  # <control>
    LineBreakClass::ULB_CM, // 0x0007  # <control>
    LineBreakClass::ULB_CM, // 0x0008  # <control>
    LineBreakClass::ULB_BA, // 0x0009  # <control>
    LineBreakClass::ULB_LF, // 0x000A  # <control>
    LineBreakClass::ULB_BK, // 0x000B  # <control>
    LineBreakClass::ULB_BK, // 0x000C  # <control>
    LineBreakClass::ULB_CR, // 0x000D  # <control>
    LineBreakClass::ULB_CM, // 0x000E  # <control>
    LineBreakClass::ULB_CM, // 0x000F  # <control>
    LineBreakClass::ULB_CM, // 0x0010  # <control>
    LineBreakClass::ULB_CM, // 0x0011  # <control>
    LineBreakClass::ULB_CM, // 0x0012  # <control>
    LineBreakClass::ULB_CM, // 0x0013  # <control>
    LineBreakClass::ULB_CM, // 0x0014  # <control>
    LineBreakClass::ULB_CM, // 0x0015  # <control>
    LineBreakClass::ULB_CM, // 0x0016  # <control>
    LineBreakClass::ULB_CM, // 0x0017  # <control>
    LineBreakClass::ULB_CM, // 0x0018  # <control>
    LineBreakClass::ULB_CM, // 0x0019  # <control>
    LineBreakClass::ULB_CM, // 0x001A  # <control>
    LineBreakClass::ULB_CM, // 0x001B  # <control>
    LineBreakClass::ULB_CM, // 0x001C  # <control>
    LineBreakClass::ULB_CM, // 0x001D  # <control>
    LineBreakClass::ULB_CM, // 0x001E  # <control>
    LineBreakClass::ULB_CM, // 0x001F  # <control>
    LineBreakClass::ULB_SP, // 0x0020  # SPACE
    LineBreakClass::ULB_EX, // 0x0021  # EXCLAMATION MARK
    LineBreakClass::ULB_QU, // 0x0022  # QUOTATION MARK
    LineBreakClass::ULB_AL, // 0x0023  # NUMBER SIGN
    LineBreakClass::ULB_PR, // 0x0024  # DOLLAR SIGN
    LineBreakClass::ULB_PO, // 0x0025  # PERCENT SIGN
    LineBreakClass::ULB_AL, // 0x0026  # AMPERSAND
    LineBreakClass::ULB_QU, // 0x0027  # APOSTROPHE
    LineBreakClass::ULB_OP, // 0x0028  # LEFT PARENTHESIS
    LineBreakClass::ULB_CL, // 0x0029  # RIGHT PARENTHESIS
    LineBreakClass::ULB_AL, // 0x002A  # ASTERISK
    LineBreakClass::ULB_PR, // 0x002B  # PLUS SIGN
    LineBreakClass::ULB_IS, // 0x002C  # COMMA
    LineBreakClass::ULB_HY, // 0x002D  # HYPHEN-MINUS
    LineBreakClass::ULB_IS, // 0x002E  # FULL STOP
    LineBreakClass::ULB_SY, // 0x002F  # SOLIDUS
    LineBreakClass::ULB_NU, // 0x0030  # DIGIT ZERO
    LineBreakClass::ULB_NU, // 0x0031  # DIGIT ONE
    LineBreakClass::ULB_NU, // 0x0032  # DIGIT TWO
    LineBreakClass::ULB_NU, // 0x0033  # DIGIT THREE
    LineBreakClass::ULB_NU, // 0x0034  # DIGIT FOUR
    LineBreakClass::ULB_NU, // 0x0035  # DIGIT FIVE
    LineBreakClass::ULB_NU, // 0x0036  # DIGIT SIX
    LineBreakClass::ULB_NU, // 0x0037  # DIGIT SEVEN
    LineBreakClass::ULB_NU, // 0x0038  # DIGIT EIGHT
    LineBreakClass::ULB_NU, // 0x0039  # DIGIT NINE
    LineBreakClass::ULB_IS, // 0x003A  # COLON
    LineBreakClass::ULB_IS, // 0x003B  # SEMICOLON
    LineBreakClass::ULB_AL, // 0x003C  # LESS-THAN SIGN
    LineBreakClass::ULB_AL, // 0x003D  # EQUALS SIGN
    LineBreakClass::ULB_AL, // 0x003E  # GREATER-THAN SIGN
    LineBreakClass::ULB_EX, // 0x003F  # QUESTION MARK
    LineBreakClass::ULB_AL, // 0x0040  # COMMERCIAL AT
    LineBreakClass::ULB_AL, // 0x0041  # LATIN CAPITAL LETTER A
    LineBreakClass::ULB_AL, // 0x0042  # LATIN CAPITAL LETTER B
    LineBreakClass::ULB_AL, // 0x0043  # LATIN CAPITAL LETTER C
    LineBreakClass::ULB_AL, // 0x0044  # LATIN CAPITAL LETTER D
    LineBreakClass::ULB_AL, // 0x0045  # LATIN CAPITAL LETTER E
    LineBreakClass::ULB_AL, // 0x0046  # LATIN CAPITAL LETTER F
    LineBreakClass::ULB_AL, // 0x0047  # LATIN CAPITAL LETTER G
    LineBreakClass::ULB_AL, // 0x0048  # LATIN CAPITAL LETTER H
    LineBreakClass::ULB_AL, // 0x0049  # LATIN CAPITAL LETTER I
    LineBreakClass::ULB_AL, // 0x004A  # LATIN CAPITAL LETTER J
    LineBreakClass::ULB_AL, // 0x004B  # LATIN CAPITAL LETTER K
    LineBreakClass::ULB_AL, // 0x004C  # LATIN CAPITAL LETTER L
    LineBreakClass::ULB_AL, // 0x004D  # LATIN CAPITAL LETTER M
    LineBreakClass::ULB_AL, // 0x004E  # LATIN CAPITAL LETTER N
    LineBreakClass::ULB_AL, // 0x004F  # LATIN CAPITAL LETTER O
    LineBreakClass::ULB_AL, // 0x0050  # LATIN CAPITAL LETTER P
    LineBreakClass::ULB_AL, // 0x0051  # LATIN CAPITAL LETTER Q
    LineBreakClass::ULB_AL, // 0x0052  # LATIN CAPITAL LETTER R
    LineBreakClass::ULB_AL, // 0x0053  # LATIN CAPITAL LETTER S
    LineBreakClass::ULB_AL, // 0x0054  # LATIN CAPITAL LETTER T
    LineBreakClass::ULB_AL, // 0x0055  # LATIN CAPITAL LETTER U
    LineBreakClass::ULB_AL, // 0x0056  # LATIN CAPITAL LETTER V
    LineBreakClass::ULB_AL, // 0x0057  # LATIN CAPITAL LETTER W
    LineBreakClass::ULB_AL, // 0x0058  # LATIN CAPITAL LETTER X
    LineBreakClass::ULB_AL, // 0x0059  # LATIN CAPITAL LETTER Y
    LineBreakClass::ULB_AL, // 0x005A  # LATIN CAPITAL LETTER Z
    LineBreakClass::ULB_OP, // 0x005B  # LEFT SQUARE BRACKET
    LineBreakClass::ULB_PR, // 0x005C  # REVERSE SOLIDUS
    LineBreakClass::ULB_CL, // 0x005D  # RIGHT SQUARE BRACKET
    LineBreakClass::ULB_AL, // 0x005E  # CIRCUMFLEX ACCENT
    LineBreakClass::ULB_AL, // 0x005F  # LOW LINE
    LineBreakClass::ULB_AL, // 0x0060  # GRAVE ACCENT
    LineBreakClass::ULB_AL, // 0x0061  # LATIN SMALL LETTER A
    LineBreakClass::ULB_AL, // 0x0062  # LATIN SMALL LETTER B
    LineBreakClass::ULB_AL, // 0x0063  # LATIN SMALL LETTER C
    LineBreakClass::ULB_AL, // 0x0064  # LATIN SMALL LETTER D
    LineBreakClass::ULB_AL, // 0x0065  # LATIN SMALL LETTER E
    LineBreakClass::ULB_AL, // 0x0066  # LATIN SMALL LETTER F
    LineBreakClass::ULB_AL, // 0x0067  # LATIN SMALL LETTER G
    LineBreakClass::ULB_AL, // 0x0068  # LATIN SMALL LETTER H
    LineBreakClass::ULB_AL, // 0x0069  # LATIN SMALL LETTER I
    LineBreakClass::ULB_AL, // 0x006A  # LATIN SMALL LETTER J
    LineBreakClass::ULB_AL, // 0x006B  # LATIN SMALL LETTER K
    LineBreakClass::ULB_AL, // 0x006C  # LATIN SMALL LETTER L
    LineBreakClass::ULB_AL, // 0x006D  # LATIN SMALL LETTER M
    LineBreakClass::ULB_AL, // 0x006E  # LATIN SMALL LETTER N
    LineBreakClass::ULB_AL, // 0x006F  # LATIN SMALL LETTER O
    LineBreakClass::ULB_AL, // 0x0070  # LATIN SMALL LETTER P
    LineBreakClass::ULB_AL, // 0x0071  # LATIN SMALL LETTER Q
    LineBreakClass::ULB_AL, // 0x0072  # LATIN SMALL LETTER R
    LineBreakClass::ULB_AL, // 0x0073  # LATIN SMALL LETTER S
    LineBreakClass::ULB_AL, // 0x0074  # LATIN SMALL LETTER T
    LineBreakClass::ULB_AL, // 0x0075  # LATIN SMALL LETTER U
    LineBreakClass::ULB_AL, // 0x0076  # LATIN SMALL LETTER V
    LineBreakClass::ULB_AL, // 0x0077  # LATIN SMALL LETTER W
    LineBreakClass::ULB_AL, // 0x0078  # LATIN SMALL LETTER X
    LineBreakClass::ULB_AL, // 0x0079  # LATIN SMALL LETTER Y
    LineBreakClass::ULB_AL, // 0x007A  # LATIN SMALL LETTER Z
    LineBreakClass::ULB_OP, // 0x007B  # LEFT CURLY BRACKET
    LineBreakClass::ULB_BA, // 0x007C  # VERTICAL LINE
    LineBreakClass::ULB_CL, // 0x007D  # RIGHT CURLY BRACKET
    LineBreakClass::ULB_AL, // 0x007E  # TILDE
    LineBreakClass::ULB_CM, // 0x007F  # <control>
    LineBreakClass::ULB_CM, // 0x0080  # <control>
    LineBreakClass::ULB_CM, // 0x0081  # <control>
    LineBreakClass::ULB_CM, // 0x0082  # <control>
    LineBreakClass::ULB_CM, // 0x0083  # <control>
    LineBreakClass::ULB_CM, // 0x0084  # <control>
    LineBreakClass::ULB_NL, // 0x0085  # <control>
    LineBreakClass::ULB_CM, // 0x0086  # <control>
    LineBreakClass::ULB_CM, // 0x0087  # <control>
    LineBreakClass::ULB_CM, // 0x0088  # <control>
    LineBreakClass::ULB_CM, // 0x0089  # <control>
    LineBreakClass::ULB_CM, // 0x008A  # <control>
    LineBreakClass::ULB_CM, // 0x008B  # <control>
    LineBreakClass::ULB_CM, // 0x008C  # <control>
    LineBreakClass::ULB_CM, // 0x008D  # <control>
    LineBreakClass::ULB_CM, // 0x008E  # <control>
    LineBreakClass::ULB_CM, // 0x008F  # <control>
    LineBreakClass::ULB_CM, // 0x0090  # <control>
    LineBreakClass::ULB_CM, // 0x0091  # <control>
    LineBreakClass::ULB_CM, // 0x0092  # <control>
    LineBreakClass::ULB_CM, // 0x0093  # <control>
    LineBreakClass::ULB_CM, // 0x0094  # <control>
    LineBreakClass::ULB_CM, // 0x0095  # <control>
    LineBreakClass::ULB_CM, // 0x0096  # <control>
    LineBreakClass::ULB_CM, // 0x0097  # <control>
    LineBreakClass::ULB_CM, // 0x0098  # <control>
    LineBreakClass::ULB_CM, // 0x0099  # <control>
    LineBreakClass::ULB_CM, // 0x009A  # <control>
    LineBreakClass::ULB_CM, // 0x009B  # <control>
    LineBreakClass::ULB_CM, // 0x009C  # <control>
    LineBreakClass::ULB_CM, // 0x009D  # <control>
    LineBreakClass::ULB_CM, // 0x009E  # <control>
    LineBreakClass::ULB_CM, // 0x009F  # <control>
    LineBreakClass::ULB_GL, // 0x00A0  # NO-BREAK SPACE
    LineBreakClass::ULB_AI, // 0x00A1  # INVERTED EXCLAMATION MARK
    LineBreakClass::ULB_PO, // 0x00A2  # CENT SIGN
    LineBreakClass::ULB_PR, // 0x00A3  # POUND SIGN
    LineBreakClass::ULB_PR, // 0x00A4  # CURRENCY SIGN
    LineBreakClass::ULB_PR, // 0x00A5  # YEN SIGN
    LineBreakClass::ULB_AL, // 0x00A6  # BROKEN BAR
    LineBreakClass::ULB_AI, // 0x00A7  # SECTION SIGN
    LineBreakClass::ULB_AI, // 0x00A8  # DIAERESIS
    LineBreakClass::ULB_AL, // 0x00A9  # COPYRIGHT SIGN
    LineBreakClass::ULB_AI, // 0x00AA  # FEMININE ORDINAL INDICATOR
    LineBreakClass::ULB_QU, // 0x00AB  # LEFT-POINTING DOUBLE ANGLE QUOTATION MARK
    LineBreakClass::ULB_AL, // 0x00AC  # NOT SIGN
    LineBreakClass::ULB_BA, // 0x00AD  # SOFT HYPHEN
    LineBreakClass::ULB_AL, // 0x00AE  # REGISTERED SIGN
    LineBreakClass::ULB_AL, // 0x00AF  # MACRON
    LineBreakClass::ULB_PO, // 0x00B0  # DEGREE SIGN
    LineBreakClass::ULB_PR, // 0x00B1  # PLUS-MINUS SIGN
    LineBreakClass::ULB_AI, // 0x00B2  # SUPERSCRIPT TWO
    LineBreakClass::ULB_AI, // 0x00B3  # SUPERSCRIPT THREE
    LineBreakClass::ULB_BB, // 0x00B4  # ACUTE ACCENT
    LineBreakClass::ULB_AL, // 0x00B5  # MICRO SIGN
    LineBreakClass::ULB_AI, // 0x00B6  # PILCROW SIGN
    LineBreakClass::ULB_AI, // 0x00B7  # MIDDLE DOT
    LineBreakClass::ULB_AI, // 0x00B8  # CEDILLA
    LineBreakClass::ULB_AI, // 0x00B9  # SUPERSCRIPT ONE
    LineBreakClass::ULB_AI, // 0x00BA  # MASCULINE ORDINAL INDICATOR
    LineBreakClass::ULB_QU, // 0x00BB  # RIGHT-POINTING DOUBLE ANGLE QUOTATION MARK
    LineBreakClass::ULB_AI, // 0x00BC  # VULGAR FRACTION ONE QUARTER
    LineBreakClass::ULB_AI, // 0x00BD  # VULGAR FRACTION ONE HALF
    LineBreakClass::ULB_AI, // 0x00BE  # VULGAR FRACTION THREE QUARTERS
    LineBreakClass::ULB_AI, // 0x00BF  # INVERTED QUESTION MARK
    LineBreakClass::ULB_AL, // 0x00C0  # LATIN CAPITAL LETTER A WITH GRAVE
    LineBreakClass::ULB_AL, // 0x00C1  # LATIN CAPITAL LETTER A WITH ACUTE
    LineBreakClass::ULB_AL, // 0x00C2  # LATIN CAPITAL LETTER A WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x00C3  # LATIN CAPITAL LETTER A WITH TILDE
    LineBreakClass::ULB_AL, // 0x00C4  # LATIN CAPITAL LETTER A WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x00C5  # LATIN CAPITAL LETTER A WITH RING ABOVE
    LineBreakClass::ULB_AL, // 0x00C6  # LATIN CAPITAL LETTER AE
    LineBreakClass::ULB_AL, // 0x00C7  # LATIN CAPITAL LETTER C WITH CEDILLA
    LineBreakClass::ULB_AL, // 0x00C8  # LATIN CAPITAL LETTER E WITH GRAVE
    LineBreakClass::ULB_AL, // 0x00C9  # LATIN CAPITAL LETTER E WITH ACUTE
    LineBreakClass::ULB_AL, // 0x00CA  # LATIN CAPITAL LETTER E WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x00CB  # LATIN CAPITAL LETTER E WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x00CC  # LATIN CAPITAL LETTER I WITH GRAVE
    LineBreakClass::ULB_AL, // 0x00CD  # LATIN CAPITAL LETTER I WITH ACUTE
    LineBreakClass::ULB_AL, // 0x00CE  # LATIN CAPITAL LETTER I WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x00CF  # LATIN CAPITAL LETTER I WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x00D0  # LATIN CAPITAL LETTER ETH
    LineBreakClass::ULB_AL, // 0x00D1  # LATIN CAPITAL LETTER N WITH TILDE
    LineBreakClass::ULB_AL, // 0x00D2  # LATIN CAPITAL LETTER O WITH GRAVE
    LineBreakClass::ULB_AL, // 0x00D3  # LATIN CAPITAL LETTER O WITH ACUTE
    LineBreakClass::ULB_AL, // 0x00D4  # LATIN CAPITAL LETTER O WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x00D5  # LATIN CAPITAL LETTER O WITH TILDE
    LineBreakClass::ULB_AL, // 0x00D6  # LATIN CAPITAL LETTER O WITH DIAERESIS
    LineBreakClass::ULB_AI, // 0x00D7  # MULTIPLICATION SIGN
    LineBreakClass::ULB_AL, // 0x00D8  # LATIN CAPITAL LETTER O WITH STROKE
    LineBreakClass::ULB_AL, // 0x00D9  # LATIN CAPITAL LETTER U WITH GRAVE
    LineBreakClass::ULB_AL, // 0x00DA  # LATIN CAPITAL LETTER U WITH ACUTE
    LineBreakClass::ULB_AL, // 0x00DB  # LATIN CAPITAL LETTER U WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x00DC  # LATIN CAPITAL LETTER U WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x00DD  # LATIN CAPITAL LETTER Y WITH ACUTE
    LineBreakClass::ULB_AL, // 0x00DE  # LATIN CAPITAL LETTER THORN
    LineBreakClass::ULB_AL, // 0x00DF  # LATIN SMALL LETTER SHARP S
    LineBreakClass::ULB_AL, // 0x00E0  # LATIN SMALL LETTER A WITH GRAVE
    LineBreakClass::ULB_AL, // 0x00E1  # LATIN SMALL LETTER A WITH ACUTE
    LineBreakClass::ULB_AL, // 0x00E2  # LATIN SMALL LETTER A WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x00E3  # LATIN SMALL LETTER A WITH TILDE
    LineBreakClass::ULB_AL, // 0x00E4  # LATIN SMALL LETTER A WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x00E5  # LATIN SMALL LETTER A WITH RING ABOVE
    LineBreakClass::ULB_AL, // 0x00E6  # LATIN SMALL LETTER AE
    LineBreakClass::ULB_AL, // 0x00E7  # LATIN SMALL LETTER C WITH CEDILLA
    LineBreakClass::ULB_AL, // 0x00E8  # LATIN SMALL LETTER E WITH GRAVE
    LineBreakClass::ULB_AL, // 0x00E9  # LATIN SMALL LETTER E WITH ACUTE
    LineBreakClass::ULB_AL, // 0x00EA  # LATIN SMALL LETTER E WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x00EB  # LATIN SMALL LETTER E WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x00EC  # LATIN SMALL LETTER I WITH GRAVE
    LineBreakClass::ULB_AL, // 0x00ED  # LATIN SMALL LETTER I WITH ACUTE
    LineBreakClass::ULB_AL, // 0x00EE  # LATIN SMALL LETTER I WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x00EF  # LATIN SMALL LETTER I WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x00F0  # LATIN SMALL LETTER ETH
    LineBreakClass::ULB_AL, // 0x00F1  # LATIN SMALL LETTER N WITH TILDE
    LineBreakClass::ULB_AL, // 0x00F2  # LATIN SMALL LETTER O WITH GRAVE
    LineBreakClass::ULB_AL, // 0x00F3  # LATIN SMALL LETTER O WITH ACUTE
    LineBreakClass::ULB_AL, // 0x00F4  # LATIN SMALL LETTER O WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x00F5  # LATIN SMALL LETTER O WITH TILDE
    LineBreakClass::ULB_AL, // 0x00F6  # LATIN SMALL LETTER O WITH DIAERESIS
    LineBreakClass::ULB_AI, // 0x00F7  # DIVISION SIGN
    LineBreakClass::ULB_AL, // 0x00F8  # LATIN SMALL LETTER O WITH STROKE
    LineBreakClass::ULB_AL, // 0x00F9  # LATIN SMALL LETTER U WITH GRAVE
    LineBreakClass::ULB_AL, // 0x00FA  # LATIN SMALL LETTER U WITH ACUTE
    LineBreakClass::ULB_AL, // 0x00FB  # LATIN SMALL LETTER U WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x00FC  # LATIN SMALL LETTER U WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x00FD  # LATIN SMALL LETTER Y WITH ACUTE
    LineBreakClass::ULB_AL, // 0x00FE  # LATIN SMALL LETTER THORN
    LineBreakClass::ULB_AL, // 0x00FF  # LATIN SMALL LETTER Y WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x0100  # LATIN CAPITAL LETTER A WITH MACRON
    LineBreakClass::ULB_AL, // 0x0101  # LATIN SMALL LETTER A WITH MACRON
    LineBreakClass::ULB_AL, // 0x0102  # LATIN CAPITAL LETTER A WITH BREVE
    LineBreakClass::ULB_AL, // 0x0103  # LATIN SMALL LETTER A WITH BREVE
    LineBreakClass::ULB_AL, // 0x0104  # LATIN CAPITAL LETTER A WITH OGONEK
    LineBreakClass::ULB_AL, // 0x0105  # LATIN SMALL LETTER A WITH OGONEK
    LineBreakClass::ULB_AL, // 0x0106  # LATIN CAPITAL LETTER C WITH ACUTE
    LineBreakClass::ULB_AL, // 0x0107  # LATIN SMALL LETTER C WITH ACUTE
    LineBreakClass::ULB_AL, // 0x0108  # LATIN CAPITAL LETTER C WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x0109  # LATIN SMALL LETTER C WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x010A  # LATIN CAPITAL LETTER C WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x010B  # LATIN SMALL LETTER C WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x010C  # LATIN CAPITAL LETTER C WITH CARON
    LineBreakClass::ULB_AL, // 0x010D  # LATIN SMALL LETTER C WITH CARON
    LineBreakClass::ULB_AL, // 0x010E  # LATIN CAPITAL LETTER D WITH CARON
    LineBreakClass::ULB_AL, // 0x010F  # LATIN SMALL LETTER D WITH CARON
    LineBreakClass::ULB_AL, // 0x0110  # LATIN CAPITAL LETTER D WITH STROKE
    LineBreakClass::ULB_AL, // 0x0111  # LATIN SMALL LETTER D WITH STROKE
    LineBreakClass::ULB_AL, // 0x0112  # LATIN CAPITAL LETTER E WITH MACRON
    LineBreakClass::ULB_AL, // 0x0113  # LATIN SMALL LETTER E WITH MACRON
    LineBreakClass::ULB_AL, // 0x0114  # LATIN CAPITAL LETTER E WITH BREVE
    LineBreakClass::ULB_AL, // 0x0115  # LATIN SMALL LETTER E WITH BREVE
    LineBreakClass::ULB_AL, // 0x0116  # LATIN CAPITAL LETTER E WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x0117  # LATIN SMALL LETTER E WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x0118  # LATIN CAPITAL LETTER E WITH OGONEK
    LineBreakClass::ULB_AL, // 0x0119  # LATIN SMALL LETTER E WITH OGONEK
    LineBreakClass::ULB_AL, // 0x011A  # LATIN CAPITAL LETTER E WITH CARON
    LineBreakClass::ULB_AL, // 0x011B  # LATIN SMALL LETTER E WITH CARON
    LineBreakClass::ULB_AL, // 0x011C  # LATIN CAPITAL LETTER G WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x011D  # LATIN SMALL LETTER G WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x011E  # LATIN CAPITAL LETTER G WITH BREVE
    LineBreakClass::ULB_AL, // 0x011F  # LATIN SMALL LETTER G WITH BREVE
    LineBreakClass::ULB_AL, // 0x0120  # LATIN CAPITAL LETTER G WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x0121  # LATIN SMALL LETTER G WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x0122  # LATIN CAPITAL LETTER G WITH CEDILLA
    LineBreakClass::ULB_AL, // 0x0123  # LATIN SMALL LETTER G WITH CEDILLA
    LineBreakClass::ULB_AL, // 0x0124  # LATIN CAPITAL LETTER H WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x0125  # LATIN SMALL LETTER H WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x0126  # LATIN CAPITAL LETTER H WITH STROKE
    LineBreakClass::ULB_AL, // 0x0127  # LATIN SMALL LETTER H WITH STROKE
    LineBreakClass::ULB_AL, // 0x0128  # LATIN CAPITAL LETTER I WITH TILDE
    LineBreakClass::ULB_AL, // 0x0129  # LATIN SMALL LETTER I WITH TILDE
    LineBreakClass::ULB_AL, // 0x012A  # LATIN CAPITAL LETTER I WITH MACRON
    LineBreakClass::ULB_AL, // 0x012B  # LATIN SMALL LETTER I WITH MACRON
    LineBreakClass::ULB_AL, // 0x012C  # LATIN CAPITAL LETTER I WITH BREVE
    LineBreakClass::ULB_AL, // 0x012D  # LATIN SMALL LETTER I WITH BREVE
    LineBreakClass::ULB_AL, // 0x012E  # LATIN CAPITAL LETTER I WITH OGONEK
    LineBreakClass::ULB_AL, // 0x012F  # LATIN SMALL LETTER I WITH OGONEK
    LineBreakClass::ULB_AL, // 0x0130  # LATIN CAPITAL LETTER I WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x0131  # LATIN SMALL LETTER DOTLESS I
    LineBreakClass::ULB_AL, // 0x0132  # LATIN CAPITAL LIGATURE IJ
    LineBreakClass::ULB_AL, // 0x0133  # LATIN SMALL LIGATURE IJ
    LineBreakClass::ULB_AL, // 0x0134  # LATIN CAPITAL LETTER J WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x0135  # LATIN SMALL LETTER J WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x0136  # LATIN CAPITAL LETTER K WITH CEDILLA
    LineBreakClass::ULB_AL, // 0x0137  # LATIN SMALL LETTER K WITH CEDILLA
    LineBreakClass::ULB_AL, // 0x0138  # LATIN SMALL LETTER KRA
    LineBreakClass::ULB_AL, // 0x0139  # LATIN CAPITAL LETTER L WITH ACUTE
    LineBreakClass::ULB_AL, // 0x013A  # LATIN SMALL LETTER L WITH ACUTE
    LineBreakClass::ULB_AL, // 0x013B  # LATIN CAPITAL LETTER L WITH CEDILLA
    LineBreakClass::ULB_AL, // 0x013C  # LATIN SMALL LETTER L WITH CEDILLA
    LineBreakClass::ULB_AL, // 0x013D  # LATIN CAPITAL LETTER L WITH CARON
    LineBreakClass::ULB_AL, // 0x013E  # LATIN SMALL LETTER L WITH CARON
    LineBreakClass::ULB_AL, // 0x013F  # LATIN CAPITAL LETTER L WITH MIDDLE DOT
    LineBreakClass::ULB_AL, // 0x0140  # LATIN SMALL LETTER L WITH MIDDLE DOT
    LineBreakClass::ULB_AL, // 0x0141  # LATIN CAPITAL LETTER L WITH STROKE
    LineBreakClass::ULB_AL, // 0x0142  # LATIN SMALL LETTER L WITH STROKE
    LineBreakClass::ULB_AL, // 0x0143  # LATIN CAPITAL LETTER N WITH ACUTE
    LineBreakClass::ULB_AL, // 0x0144  # LATIN SMALL LETTER N WITH ACUTE
    LineBreakClass::ULB_AL, // 0x0145  # LATIN CAPITAL LETTER N WITH CEDILLA
    LineBreakClass::ULB_AL, // 0x0146  # LATIN SMALL LETTER N WITH CEDILLA
    LineBreakClass::ULB_AL, // 0x0147  # LATIN CAPITAL LETTER N WITH CARON
    LineBreakClass::ULB_AL, // 0x0148  # LATIN SMALL LETTER N WITH CARON
    LineBreakClass::ULB_AL, // 0x0149  # LATIN SMALL LETTER N PRECEDED BY APOSTROPHE
    LineBreakClass::ULB_AL, // 0x014A  # LATIN CAPITAL LETTER ENG
    LineBreakClass::ULB_AL, // 0x014B  # LATIN SMALL LETTER ENG
    LineBreakClass::ULB_AL, // 0x014C  # LATIN CAPITAL LETTER O WITH MACRON
    LineBreakClass::ULB_AL, // 0x014D  # LATIN SMALL LETTER O WITH MACRON
    LineBreakClass::ULB_AL, // 0x014E  # LATIN CAPITAL LETTER O WITH BREVE
    LineBreakClass::ULB_AL, // 0x014F  # LATIN SMALL LETTER O WITH BREVE
    LineBreakClass::ULB_AL, // 0x0150  # LATIN CAPITAL LETTER O WITH DOUBLE ACUTE
    LineBreakClass::ULB_AL, // 0x0151  # LATIN SMALL LETTER O WITH DOUBLE ACUTE
    LineBreakClass::ULB_AL, // 0x0152  # LATIN CAPITAL LIGATURE OE
    LineBreakClass::ULB_AL, // 0x0153  # LATIN SMALL LIGATURE OE
    LineBreakClass::ULB_AL, // 0x0154  # LATIN CAPITAL LETTER R WITH ACUTE
    LineBreakClass::ULB_AL, // 0x0155  # LATIN SMALL LETTER R WITH ACUTE
    LineBreakClass::ULB_AL, // 0x0156  # LATIN CAPITAL LETTER R WITH CEDILLA
    LineBreakClass::ULB_AL, // 0x0157  # LATIN SMALL LETTER R WITH CEDILLA
    LineBreakClass::ULB_AL, // 0x0158  # LATIN CAPITAL LETTER R WITH CARON
    LineBreakClass::ULB_AL, // 0x0159  # LATIN SMALL LETTER R WITH CARON
    LineBreakClass::ULB_AL, // 0x015A  # LATIN CAPITAL LETTER S WITH ACUTE
    LineBreakClass::ULB_AL, // 0x015B  # LATIN SMALL LETTER S WITH ACUTE
    LineBreakClass::ULB_AL, // 0x015C  # LATIN CAPITAL LETTER S WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x015D  # LATIN SMALL LETTER S WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x015E  # LATIN CAPITAL LETTER S WITH CEDILLA
    LineBreakClass::ULB_AL, // 0x015F  # LATIN SMALL LETTER S WITH CEDILLA
    LineBreakClass::ULB_AL, // 0x0160  # LATIN CAPITAL LETTER S WITH CARON
    LineBreakClass::ULB_AL, // 0x0161  # LATIN SMALL LETTER S WITH CARON
    LineBreakClass::ULB_AL, // 0x0162  # LATIN CAPITAL LETTER T WITH CEDILLA
    LineBreakClass::ULB_AL, // 0x0163  # LATIN SMALL LETTER T WITH CEDILLA
    LineBreakClass::ULB_AL, // 0x0164  # LATIN CAPITAL LETTER T WITH CARON
    LineBreakClass::ULB_AL, // 0x0165  # LATIN SMALL LETTER T WITH CARON
    LineBreakClass::ULB_AL, // 0x0166  # LATIN CAPITAL LETTER T WITH STROKE
    LineBreakClass::ULB_AL, // 0x0167  # LATIN SMALL LETTER T WITH STROKE
    LineBreakClass::ULB_AL, // 0x0168  # LATIN CAPITAL LETTER U WITH TILDE
    LineBreakClass::ULB_AL, // 0x0169  # LATIN SMALL LETTER U WITH TILDE
    LineBreakClass::ULB_AL, // 0x016A  # LATIN CAPITAL LETTER U WITH MACRON
    LineBreakClass::ULB_AL, // 0x016B  # LATIN SMALL LETTER U WITH MACRON
    LineBreakClass::ULB_AL, // 0x016C  # LATIN CAPITAL LETTER U WITH BREVE
    LineBreakClass::ULB_AL, // 0x016D  # LATIN SMALL LETTER U WITH BREVE
    LineBreakClass::ULB_AL, // 0x016E  # LATIN CAPITAL LETTER U WITH RING ABOVE
    LineBreakClass::ULB_AL, // 0x016F  # LATIN SMALL LETTER U WITH RING ABOVE
    LineBreakClass::ULB_AL, // 0x0170  # LATIN CAPITAL LETTER U WITH DOUBLE ACUTE
    LineBreakClass::ULB_AL, // 0x0171  # LATIN SMALL LETTER U WITH DOUBLE ACUTE
    LineBreakClass::ULB_AL, // 0x0172  # LATIN CAPITAL LETTER U WITH OGONEK
    LineBreakClass::ULB_AL, // 0x0173  # LATIN SMALL LETTER U WITH OGONEK
    LineBreakClass::ULB_AL, // 0x0174  # LATIN CAPITAL LETTER W WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x0175  # LATIN SMALL LETTER W WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x0176  # LATIN CAPITAL LETTER Y WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x0177  # LATIN SMALL LETTER Y WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x0178  # LATIN CAPITAL LETTER Y WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x0179  # LATIN CAPITAL LETTER Z WITH ACUTE
    LineBreakClass::ULB_AL, // 0x017A  # LATIN SMALL LETTER Z WITH ACUTE
    LineBreakClass::ULB_AL, // 0x017B  # LATIN CAPITAL LETTER Z WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x017C  # LATIN SMALL LETTER Z WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x017D  # LATIN CAPITAL LETTER Z WITH CARON
    LineBreakClass::ULB_AL, // 0x017E  # LATIN SMALL LETTER Z WITH CARON
    LineBreakClass::ULB_AL, // 0x017F  # LATIN SMALL LETTER LONG S
    LineBreakClass::ULB_AL, // 0x0180  # LATIN SMALL LETTER B WITH STROKE
    LineBreakClass::ULB_AL, // 0x0181  # LATIN CAPITAL LETTER B WITH HOOK
    LineBreakClass::ULB_AL, // 0x0182  # LATIN CAPITAL LETTER B WITH TOPBAR
    LineBreakClass::ULB_AL, // 0x0183  # LATIN SMALL LETTER B WITH TOPBAR
    LineBreakClass::ULB_AL, // 0x0184  # LATIN CAPITAL LETTER TONE SIX
    LineBreakClass::ULB_AL, // 0x0185  # LATIN SMALL LETTER TONE SIX
    LineBreakClass::ULB_AL, // 0x0186  # LATIN CAPITAL LETTER OPEN O
    LineBreakClass::ULB_AL, // 0x0187  # LATIN CAPITAL LETTER C WITH HOOK
    LineBreakClass::ULB_AL, // 0x0188  # LATIN SMALL LETTER C WITH HOOK
    LineBreakClass::ULB_AL, // 0x0189  # LATIN CAPITAL LETTER AFRICAN D
    LineBreakClass::ULB_AL, // 0x018A  # LATIN CAPITAL LETTER D WITH HOOK
    LineBreakClass::ULB_AL, // 0x018B  # LATIN CAPITAL LETTER D WITH TOPBAR
    LineBreakClass::ULB_AL, // 0x018C  # LATIN SMALL LETTER D WITH TOPBAR
    LineBreakClass::ULB_AL, // 0x018D  # LATIN SMALL LETTER TURNED DELTA
    LineBreakClass::ULB_AL, // 0x018E  # LATIN CAPITAL LETTER REVERSED E
    LineBreakClass::ULB_AL, // 0x018F  # LATIN CAPITAL LETTER SCHWA
    LineBreakClass::ULB_AL, // 0x0190  # LATIN CAPITAL LETTER OPEN E
    LineBreakClass::ULB_AL, // 0x0191  # LATIN CAPITAL LETTER F WITH HOOK
    LineBreakClass::ULB_AL, // 0x0192  # LATIN SMALL LETTER F WITH HOOK
    LineBreakClass::ULB_AL, // 0x0193  # LATIN CAPITAL LETTER G WITH HOOK
    LineBreakClass::ULB_AL, // 0x0194  # LATIN CAPITAL LETTER GAMMA
    LineBreakClass::ULB_AL, // 0x0195  # LATIN SMALL LETTER HV
    LineBreakClass::ULB_AL, // 0x0196  # LATIN CAPITAL LETTER IOTA
    LineBreakClass::ULB_AL, // 0x0197  # LATIN CAPITAL LETTER I WITH STROKE
    LineBreakClass::ULB_AL, // 0x0198  # LATIN CAPITAL LETTER K WITH HOOK
    LineBreakClass::ULB_AL, // 0x0199  # LATIN SMALL LETTER K WITH HOOK
    LineBreakClass::ULB_AL, // 0x019A  # LATIN SMALL LETTER L WITH BAR
    LineBreakClass::ULB_AL, // 0x019B  # LATIN SMALL LETTER LAMBDA WITH STROKE
    LineBreakClass::ULB_AL, // 0x019C  # LATIN CAPITAL LETTER TURNED M
    LineBreakClass::ULB_AL, // 0x019D  # LATIN CAPITAL LETTER N WITH LEFT HOOK
    LineBreakClass::ULB_AL, // 0x019E  # LATIN SMALL LETTER N WITH LONG RIGHT LEG
    LineBreakClass::ULB_AL, // 0x019F  # LATIN CAPITAL LETTER O WITH MIDDLE TILDE
    LineBreakClass::ULB_AL, // 0x01A0  # LATIN CAPITAL LETTER O WITH HORN
    LineBreakClass::ULB_AL, // 0x01A1  # LATIN SMALL LETTER O WITH HORN
    LineBreakClass::ULB_AL, // 0x01A2  # LATIN CAPITAL LETTER OI
    LineBreakClass::ULB_AL, // 0x01A3  # LATIN SMALL LETTER OI
    LineBreakClass::ULB_AL, // 0x01A4  # LATIN CAPITAL LETTER P WITH HOOK
    LineBreakClass::ULB_AL, // 0x01A5  # LATIN SMALL LETTER P WITH HOOK
    LineBreakClass::ULB_AL, // 0x01A6  # LATIN LETTER YR
    LineBreakClass::ULB_AL, // 0x01A7  # LATIN CAPITAL LETTER TONE TWO
    LineBreakClass::ULB_AL, // 0x01A8  # LATIN SMALL LETTER TONE TWO
    LineBreakClass::ULB_AL, // 0x01A9  # LATIN CAPITAL LETTER ESH
    LineBreakClass::ULB_AL, // 0x01AA  # LATIN LETTER REVERSED ESH LOOP
    LineBreakClass::ULB_AL, // 0x01AB  # LATIN SMALL LETTER T WITH PALATAL HOOK
    LineBreakClass::ULB_AL, // 0x01AC  # LATIN CAPITAL LETTER T WITH HOOK
    LineBreakClass::ULB_AL, // 0x01AD  # LATIN SMALL LETTER T WITH HOOK
    LineBreakClass::ULB_AL, // 0x01AE  # LATIN CAPITAL LETTER T WITH RETROFLEX HOOK
    LineBreakClass::ULB_AL, // 0x01AF  # LATIN CAPITAL LETTER U WITH HORN
    LineBreakClass::ULB_AL, // 0x01B0  # LATIN SMALL LETTER U WITH HORN
    LineBreakClass::ULB_AL, // 0x01B1  # LATIN CAPITAL LETTER UPSILON
    LineBreakClass::ULB_AL, // 0x01B2  # LATIN CAPITAL LETTER V WITH HOOK
    LineBreakClass::ULB_AL, // 0x01B3  # LATIN CAPITAL LETTER Y WITH HOOK
    LineBreakClass::ULB_AL, // 0x01B4  # LATIN SMALL LETTER Y WITH HOOK
    LineBreakClass::ULB_AL, // 0x01B5  # LATIN CAPITAL LETTER Z WITH STROKE
    LineBreakClass::ULB_AL, // 0x01B6  # LATIN SMALL LETTER Z WITH STROKE
    LineBreakClass::ULB_AL, // 0x01B7  # LATIN CAPITAL LETTER EZH
    LineBreakClass::ULB_AL, // 0x01B8  # LATIN CAPITAL LETTER EZH REVERSED
    LineBreakClass::ULB_AL, // 0x01B9  # LATIN SMALL LETTER EZH REVERSED
    LineBreakClass::ULB_AL, // 0x01BA  # LATIN SMALL LETTER EZH WITH TAIL
    LineBreakClass::ULB_AL, // 0x01BB  # LATIN LETTER TWO WITH STROKE
    LineBreakClass::ULB_AL, // 0x01BC  # LATIN CAPITAL LETTER TONE FIVE
    LineBreakClass::ULB_AL, // 0x01BD  # LATIN SMALL LETTER TONE FIVE
    LineBreakClass::ULB_AL, // 0x01BE  # LATIN LETTER INVERTED GLOTTAL STOP WITH STROKE
    LineBreakClass::ULB_AL, // 0x01BF  # LATIN LETTER WYNN
    LineBreakClass::ULB_AL, // 0x01C0  # LATIN LETTER DENTAL CLICK
    LineBreakClass::ULB_AL, // 0x01C1  # LATIN LETTER LATERAL CLICK
    LineBreakClass::ULB_AL, // 0x01C2  # LATIN LETTER ALVEOLAR CLICK
    LineBreakClass::ULB_AL, // 0x01C3  # LATIN LETTER RETROFLEX CLICK
    LineBreakClass::ULB_AL, // 0x01C4  # LATIN CAPITAL LETTER DZ WITH CARON
    LineBreakClass::ULB_AL, // 0x01C5  # LATIN CAPITAL LETTER D WITH SMALL LETTER Z WITH CARON
    LineBreakClass::ULB_AL, // 0x01C6  # LATIN SMALL LETTER DZ WITH CARON
    LineBreakClass::ULB_AL, // 0x01C7  # LATIN CAPITAL LETTER LJ
    LineBreakClass::ULB_AL, // 0x01C8  # LATIN CAPITAL LETTER L WITH SMALL LETTER J
    LineBreakClass::ULB_AL, // 0x01C9  # LATIN SMALL LETTER LJ
    LineBreakClass::ULB_AL, // 0x01CA  # LATIN CAPITAL LETTER NJ
    LineBreakClass::ULB_AL, // 0x01CB  # LATIN CAPITAL LETTER N WITH SMALL LETTER J
    LineBreakClass::ULB_AL, // 0x01CC  # LATIN SMALL LETTER NJ
    LineBreakClass::ULB_AL, // 0x01CD  # LATIN CAPITAL LETTER A WITH CARON
    LineBreakClass::ULB_AL, // 0x01CE  # LATIN SMALL LETTER A WITH CARON
    LineBreakClass::ULB_AL, // 0x01CF  # LATIN CAPITAL LETTER I WITH CARON
    LineBreakClass::ULB_AL, // 0x01D0  # LATIN SMALL LETTER I WITH CARON
    LineBreakClass::ULB_AL, // 0x01D1  # LATIN CAPITAL LETTER O WITH CARON
    LineBreakClass::ULB_AL, // 0x01D2  # LATIN SMALL LETTER O WITH CARON
    LineBreakClass::ULB_AL, // 0x01D3  # LATIN CAPITAL LETTER U WITH CARON
    LineBreakClass::ULB_AL, // 0x01D4  # LATIN SMALL LETTER U WITH CARON
    LineBreakClass::ULB_AL, // 0x01D5  # LATIN CAPITAL LETTER U WITH DIAERESIS AND MACRON
    LineBreakClass::ULB_AL, // 0x01D6  # LATIN SMALL LETTER U WITH DIAERESIS AND MACRON
    LineBreakClass::ULB_AL, // 0x01D7  # LATIN CAPITAL LETTER U WITH DIAERESIS AND ACUTE
    LineBreakClass::ULB_AL, // 0x01D8  # LATIN SMALL LETTER U WITH DIAERESIS AND ACUTE
    LineBreakClass::ULB_AL, // 0x01D9  # LATIN CAPITAL LETTER U WITH DIAERESIS AND CARON
    LineBreakClass::ULB_AL, // 0x01DA  # LATIN SMALL LETTER U WITH DIAERESIS AND CARON
    LineBreakClass::ULB_AL, // 0x01DB  # LATIN CAPITAL LETTER U WITH DIAERESIS AND GRAVE
    LineBreakClass::ULB_AL, // 0x01DC  # LATIN SMALL LETTER U WITH DIAERESIS AND GRAVE
    LineBreakClass::ULB_AL, // 0x01DD  # LATIN SMALL LETTER TURNED E
    LineBreakClass::ULB_AL, // 0x01DE  # LATIN CAPITAL LETTER A WITH DIAERESIS AND MACRON
    LineBreakClass::ULB_AL, // 0x01DF  # LATIN SMALL LETTER A WITH DIAERESIS AND MACRON
    LineBreakClass::ULB_AL, // 0x01E0  # LATIN CAPITAL LETTER A WITH DOT ABOVE AND MACRON
    LineBreakClass::ULB_AL, // 0x01E1  # LATIN SMALL LETTER A WITH DOT ABOVE AND MACRON
    LineBreakClass::ULB_AL, // 0x01E2  # LATIN CAPITAL LETTER AE WITH MACRON
    LineBreakClass::ULB_AL, // 0x01E3  # LATIN SMALL LETTER AE WITH MACRON
    LineBreakClass::ULB_AL, // 0x01E4  # LATIN CAPITAL LETTER G WITH STROKE
    LineBreakClass::ULB_AL, // 0x01E5  # LATIN SMALL LETTER G WITH STROKE
    LineBreakClass::ULB_AL, // 0x01E6  # LATIN CAPITAL LETTER G WITH CARON
    LineBreakClass::ULB_AL, // 0x01E7  # LATIN SMALL LETTER G WITH CARON
    LineBreakClass::ULB_AL, // 0x01E8  # LATIN CAPITAL LETTER K WITH CARON
    LineBreakClass::ULB_AL, // 0x01E9  # LATIN SMALL LETTER K WITH CARON
    LineBreakClass::ULB_AL, // 0x01EA  # LATIN CAPITAL LETTER O WITH OGONEK
    LineBreakClass::ULB_AL, // 0x01EB  # LATIN SMALL LETTER O WITH OGONEK
    LineBreakClass::ULB_AL, // 0x01EC  # LATIN CAPITAL LETTER O WITH OGONEK AND MACRON
    LineBreakClass::ULB_AL, // 0x01ED  # LATIN SMALL LETTER O WITH OGONEK AND MACRON
    LineBreakClass::ULB_AL, // 0x01EE  # LATIN CAPITAL LETTER EZH WITH CARON
    LineBreakClass::ULB_AL, // 0x01EF  # LATIN SMALL LETTER EZH WITH CARON
    LineBreakClass::ULB_AL, // 0x01F0  # LATIN SMALL LETTER J WITH CARON
    LineBreakClass::ULB_AL, // 0x01F1  # LATIN CAPITAL LETTER DZ
    LineBreakClass::ULB_AL, // 0x01F2  # LATIN CAPITAL LETTER D WITH SMALL LETTER Z
    LineBreakClass::ULB_AL, // 0x01F3  # LATIN SMALL LETTER DZ
    LineBreakClass::ULB_AL, // 0x01F4  # LATIN CAPITAL LETTER G WITH ACUTE
    LineBreakClass::ULB_AL, // 0x01F5  # LATIN SMALL LETTER G WITH ACUTE
    LineBreakClass::ULB_AL, // 0x01F6  # LATIN CAPITAL LETTER HWAIR
    LineBreakClass::ULB_AL, // 0x01F7  # LATIN CAPITAL LETTER WYNN
    LineBreakClass::ULB_AL, // 0x01F8  # LATIN CAPITAL LETTER N WITH GRAVE
    LineBreakClass::ULB_AL, // 0x01F9  # LATIN SMALL LETTER N WITH GRAVE
    LineBreakClass::ULB_AL, // 0x01FA  # LATIN CAPITAL LETTER A WITH RING ABOVE AND ACUTE
    LineBreakClass::ULB_AL, // 0x01FB  # LATIN SMALL LETTER A WITH RING ABOVE AND ACUTE
    LineBreakClass::ULB_AL, // 0x01FC  # LATIN CAPITAL LETTER AE WITH ACUTE
    LineBreakClass::ULB_AL, // 0x01FD  # LATIN SMALL LETTER AE WITH ACUTE
    LineBreakClass::ULB_AL, // 0x01FE  # LATIN CAPITAL LETTER O WITH STROKE AND ACUTE
    LineBreakClass::ULB_AL, // 0x01FF  # LATIN SMALL LETTER O WITH STROKE AND ACUTE
    LineBreakClass::ULB_AL, // 0x0200  # LATIN CAPITAL LETTER A WITH DOUBLE GRAVE
    LineBreakClass::ULB_AL, // 0x0201  # LATIN SMALL LETTER A WITH DOUBLE GRAVE
    LineBreakClass::ULB_AL, // 0x0202  # LATIN CAPITAL LETTER A WITH INVERTED BREVE
    LineBreakClass::ULB_AL, // 0x0203  # LATIN SMALL LETTER A WITH INVERTED BREVE
    LineBreakClass::ULB_AL, // 0x0204  # LATIN CAPITAL LETTER E WITH DOUBLE GRAVE
    LineBreakClass::ULB_AL, // 0x0205  # LATIN SMALL LETTER E WITH DOUBLE GRAVE
    LineBreakClass::ULB_AL, // 0x0206  # LATIN CAPITAL LETTER E WITH INVERTED BREVE
    LineBreakClass::ULB_AL, // 0x0207  # LATIN SMALL LETTER E WITH INVERTED BREVE
    LineBreakClass::ULB_AL, // 0x0208  # LATIN CAPITAL LETTER I WITH DOUBLE GRAVE
    LineBreakClass::ULB_AL, // 0x0209  # LATIN SMALL LETTER I WITH DOUBLE GRAVE
    LineBreakClass::ULB_AL, // 0x020A  # LATIN CAPITAL LETTER I WITH INVERTED BREVE
    LineBreakClass::ULB_AL, // 0x020B  # LATIN SMALL LETTER I WITH INVERTED BREVE
    LineBreakClass::ULB_AL, // 0x020C  # LATIN CAPITAL LETTER O WITH DOUBLE GRAVE
    LineBreakClass::ULB_AL, // 0x020D  # LATIN SMALL LETTER O WITH DOUBLE GRAVE
    LineBreakClass::ULB_AL, // 0x020E  # LATIN CAPITAL LETTER O WITH INVERTED BREVE
    LineBreakClass::ULB_AL, // 0x020F  # LATIN SMALL LETTER O WITH INVERTED BREVE
    LineBreakClass::ULB_AL, // 0x0210  # LATIN CAPITAL LETTER R WITH DOUBLE GRAVE
    LineBreakClass::ULB_AL, // 0x0211  # LATIN SMALL LETTER R WITH DOUBLE GRAVE
    LineBreakClass::ULB_AL, // 0x0212  # LATIN CAPITAL LETTER R WITH INVERTED BREVE
    LineBreakClass::ULB_AL, // 0x0213  # LATIN SMALL LETTER R WITH INVERTED BREVE
    LineBreakClass::ULB_AL, // 0x0214  # LATIN CAPITAL LETTER U WITH DOUBLE GRAVE
    LineBreakClass::ULB_AL, // 0x0215  # LATIN SMALL LETTER U WITH DOUBLE GRAVE
    LineBreakClass::ULB_AL, // 0x0216  # LATIN CAPITAL LETTER U WITH INVERTED BREVE
    LineBreakClass::ULB_AL, // 0x0217  # LATIN SMALL LETTER U WITH INVERTED BREVE
    LineBreakClass::ULB_AL, // 0x0218  # LATIN CAPITAL LETTER S WITH COMMA BELOW
    LineBreakClass::ULB_AL, // 0x0219  # LATIN SMALL LETTER S WITH COMMA BELOW
    LineBreakClass::ULB_AL, // 0x021A  # LATIN CAPITAL LETTER T WITH COMMA BELOW
    LineBreakClass::ULB_AL, // 0x021B  # LATIN SMALL LETTER T WITH COMMA BELOW
    LineBreakClass::ULB_AL, // 0x021C  # LATIN CAPITAL LETTER YOGH
    LineBreakClass::ULB_AL, // 0x021D  # LATIN SMALL LETTER YOGH
    LineBreakClass::ULB_AL, // 0x021E  # LATIN CAPITAL LETTER H WITH CARON
    LineBreakClass::ULB_AL, // 0x021F  # LATIN SMALL LETTER H WITH CARON
    LineBreakClass::ULB_AL, // 0x0220  # LATIN CAPITAL LETTER N WITH LONG RIGHT LEG
    LineBreakClass::ULB_AL, // 0x0221  # LATIN SMALL LETTER D WITH CURL
    LineBreakClass::ULB_AL, // 0x0222  # LATIN CAPITAL LETTER OU
    LineBreakClass::ULB_AL, // 0x0223  # LATIN SMALL LETTER OU
    LineBreakClass::ULB_AL, // 0x0224  # LATIN CAPITAL LETTER Z WITH HOOK
    LineBreakClass::ULB_AL, // 0x0225  # LATIN SMALL LETTER Z WITH HOOK
    LineBreakClass::ULB_AL, // 0x0226  # LATIN CAPITAL LETTER A WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x0227  # LATIN SMALL LETTER A WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x0228  # LATIN CAPITAL LETTER E WITH CEDILLA
    LineBreakClass::ULB_AL, // 0x0229  # LATIN SMALL LETTER E WITH CEDILLA
    LineBreakClass::ULB_AL, // 0x022A  # LATIN CAPITAL LETTER O WITH DIAERESIS AND MACRON
    LineBreakClass::ULB_AL, // 0x022B  # LATIN SMALL LETTER O WITH DIAERESIS AND MACRON
    LineBreakClass::ULB_AL, // 0x022C  # LATIN CAPITAL LETTER O WITH TILDE AND MACRON
    LineBreakClass::ULB_AL, // 0x022D  # LATIN SMALL LETTER O WITH TILDE AND MACRON
    LineBreakClass::ULB_AL, // 0x022E  # LATIN CAPITAL LETTER O WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x022F  # LATIN SMALL LETTER O WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x0230  # LATIN CAPITAL LETTER O WITH DOT ABOVE AND MACRON
    LineBreakClass::ULB_AL, // 0x0231  # LATIN SMALL LETTER O WITH DOT ABOVE AND MACRON
    LineBreakClass::ULB_AL, // 0x0232  # LATIN CAPITAL LETTER Y WITH MACRON
    LineBreakClass::ULB_AL, // 0x0233  # LATIN SMALL LETTER Y WITH MACRON
    LineBreakClass::ULB_AL, // 0x0234  # LATIN SMALL LETTER L WITH CURL
    LineBreakClass::ULB_AL, // 0x0235  # LATIN SMALL LETTER N WITH CURL
    LineBreakClass::ULB_AL, // 0x0236  # LATIN SMALL LETTER T WITH CURL
    LineBreakClass::ULB_AL, // 0x0237  # LATIN SMALL LETTER DOTLESS J
    LineBreakClass::ULB_AL, // 0x0238  # LATIN SMALL LETTER DB DIGRAPH
    LineBreakClass::ULB_AL, // 0x0239  # LATIN SMALL LETTER QP DIGRAPH
    LineBreakClass::ULB_AL, // 0x023A  # LATIN CAPITAL LETTER A WITH STROKE
    LineBreakClass::ULB_AL, // 0x023B  # LATIN CAPITAL LETTER C WITH STROKE
    LineBreakClass::ULB_AL, // 0x023C  # LATIN SMALL LETTER C WITH STROKE
    LineBreakClass::ULB_AL, // 0x023D  # LATIN CAPITAL LETTER L WITH BAR
    LineBreakClass::ULB_AL, // 0x023E  # LATIN CAPITAL LETTER T WITH DIAGONAL STROKE
    LineBreakClass::ULB_AL, // 0x023F  # LATIN SMALL LETTER S WITH SWASH TAIL
    LineBreakClass::ULB_AL, // 0x0240  # LATIN SMALL LETTER Z WITH SWASH TAIL
    LineBreakClass::ULB_AL, // 0x0241  # LATIN CAPITAL LETTER GLOTTAL STOP
    LineBreakClass::ULB_AL, // 0x0242  # LATIN SMALL LETTER GLOTTAL STOP
    LineBreakClass::ULB_AL, // 0x0243  # LATIN CAPITAL LETTER B WITH STROKE
    LineBreakClass::ULB_AL, // 0x0244  # LATIN CAPITAL LETTER U BAR
    LineBreakClass::ULB_AL, // 0x0245  # LATIN CAPITAL LETTER TURNED V
    LineBreakClass::ULB_AL, // 0x0246  # LATIN CAPITAL LETTER E WITH STROKE
    LineBreakClass::ULB_AL, // 0x0247  # LATIN SMALL LETTER E WITH STROKE
    LineBreakClass::ULB_AL, // 0x0248  # LATIN CAPITAL LETTER J WITH STROKE
    LineBreakClass::ULB_AL, // 0x0249  # LATIN SMALL LETTER J WITH STROKE
    LineBreakClass::ULB_AL, // 0x024A  # LATIN CAPITAL LETTER SMALL Q WITH HOOK TAIL
    LineBreakClass::ULB_AL, // 0x024B  # LATIN SMALL LETTER Q WITH HOOK TAIL
    LineBreakClass::ULB_AL, // 0x024C  # LATIN CAPITAL LETTER R WITH STROKE
    LineBreakClass::ULB_AL, // 0x024D  # LATIN SMALL LETTER R WITH STROKE
    LineBreakClass::ULB_AL, // 0x024E  # LATIN CAPITAL LETTER Y WITH STROKE
    LineBreakClass::ULB_AL, // 0x024F  # LATIN SMALL LETTER Y WITH STROKE
    LineBreakClass::ULB_AL, // 0x0250  # LATIN SMALL LETTER TURNED A
    LineBreakClass::ULB_AL, // 0x0251  # LATIN SMALL LETTER ALPHA
    LineBreakClass::ULB_AL, // 0x0252  # LATIN SMALL LETTER TURNED ALPHA
    LineBreakClass::ULB_AL, // 0x0253  # LATIN SMALL LETTER B WITH HOOK
    LineBreakClass::ULB_AL, // 0x0254  # LATIN SMALL LETTER OPEN O
    LineBreakClass::ULB_AL, // 0x0255  # LATIN SMALL LETTER C WITH CURL
    LineBreakClass::ULB_AL, // 0x0256  # LATIN SMALL LETTER D WITH TAIL
    LineBreakClass::ULB_AL, // 0x0257  # LATIN SMALL LETTER D WITH HOOK
    LineBreakClass::ULB_AL, // 0x0258  # LATIN SMALL LETTER REVERSED E
    LineBreakClass::ULB_AL, // 0x0259  # LATIN SMALL LETTER SCHWA
    LineBreakClass::ULB_AL, // 0x025A  # LATIN SMALL LETTER SCHWA WITH HOOK
    LineBreakClass::ULB_AL, // 0x025B  # LATIN SMALL LETTER OPEN E
    LineBreakClass::ULB_AL, // 0x025C  # LATIN SMALL LETTER REVERSED OPEN E
    LineBreakClass::ULB_AL, // 0x025D  # LATIN SMALL LETTER REVERSED OPEN E WITH HOOK
    LineBreakClass::ULB_AL, // 0x025E  # LATIN SMALL LETTER CLOSED REVERSED OPEN E
    LineBreakClass::ULB_AL, // 0x025F  # LATIN SMALL LETTER DOTLESS J WITH STROKE
    LineBreakClass::ULB_AL, // 0x0260  # LATIN SMALL LETTER G WITH HOOK
    LineBreakClass::ULB_AL, // 0x0261  # LATIN SMALL LETTER SCRIPT G
    LineBreakClass::ULB_AL, // 0x0262  # LATIN LETTER SMALL CAPITAL G
    LineBreakClass::ULB_AL, // 0x0263  # LATIN SMALL LETTER GAMMA
    LineBreakClass::ULB_AL, // 0x0264  # LATIN SMALL LETTER RAMS HORN
    LineBreakClass::ULB_AL, // 0x0265  # LATIN SMALL LETTER TURNED H
    LineBreakClass::ULB_AL, // 0x0266  # LATIN SMALL LETTER H WITH HOOK
    LineBreakClass::ULB_AL, // 0x0267  # LATIN SMALL LETTER HENG WITH HOOK
    LineBreakClass::ULB_AL, // 0x0268  # LATIN SMALL LETTER I WITH STROKE
    LineBreakClass::ULB_AL, // 0x0269  # LATIN SMALL LETTER IOTA
    LineBreakClass::ULB_AL, // 0x026A  # LATIN LETTER SMALL CAPITAL I
    LineBreakClass::ULB_AL, // 0x026B  # LATIN SMALL LETTER L WITH MIDDLE TILDE
    LineBreakClass::ULB_AL, // 0x026C  # LATIN SMALL LETTER L WITH BELT
    LineBreakClass::ULB_AL, // 0x026D  # LATIN SMALL LETTER L WITH RETROFLEX HOOK
    LineBreakClass::ULB_AL, // 0x026E  # LATIN SMALL LETTER LEZH
    LineBreakClass::ULB_AL, // 0x026F  # LATIN SMALL LETTER TURNED M
    LineBreakClass::ULB_AL, // 0x0270  # LATIN SMALL LETTER TURNED M WITH LONG LEG
    LineBreakClass::ULB_AL, // 0x0271  # LATIN SMALL LETTER M WITH HOOK
    LineBreakClass::ULB_AL, // 0x0272  # LATIN SMALL LETTER N WITH LEFT HOOK
    LineBreakClass::ULB_AL, // 0x0273  # LATIN SMALL LETTER N WITH RETROFLEX HOOK
    LineBreakClass::ULB_AL, // 0x0274  # LATIN LETTER SMALL CAPITAL N
    LineBreakClass::ULB_AL, // 0x0275  # LATIN SMALL LETTER BARRED O
    LineBreakClass::ULB_AL, // 0x0276  # LATIN LETTER SMALL CAPITAL OE
    LineBreakClass::ULB_AL, // 0x0277  # LATIN SMALL LETTER CLOSED OMEGA
    LineBreakClass::ULB_AL, // 0x0278  # LATIN SMALL LETTER PHI
    LineBreakClass::ULB_AL, // 0x0279  # LATIN SMALL LETTER TURNED R
    LineBreakClass::ULB_AL, // 0x027A  # LATIN SMALL LETTER TURNED R WITH LONG LEG
    LineBreakClass::ULB_AL, // 0x027B  # LATIN SMALL LETTER TURNED R WITH HOOK
    LineBreakClass::ULB_AL, // 0x027C  # LATIN SMALL LETTER R WITH LONG LEG
    LineBreakClass::ULB_AL, // 0x027D  # LATIN SMALL LETTER R WITH TAIL
    LineBreakClass::ULB_AL, // 0x027E  # LATIN SMALL LETTER R WITH FISHHOOK
    LineBreakClass::ULB_AL, // 0x027F  # LATIN SMALL LETTER REVERSED R WITH FISHHOOK
    LineBreakClass::ULB_AL, // 0x0280  # LATIN LETTER SMALL CAPITAL R
    LineBreakClass::ULB_AL, // 0x0281  # LATIN LETTER SMALL CAPITAL INVERTED R
    LineBreakClass::ULB_AL, // 0x0282  # LATIN SMALL LETTER S WITH HOOK
    LineBreakClass::ULB_AL, // 0x0283  # LATIN SMALL LETTER ESH
    LineBreakClass::ULB_AL, // 0x0284  # LATIN SMALL LETTER DOTLESS J WITH STROKE AND HOOK
    LineBreakClass::ULB_AL, // 0x0285  # LATIN SMALL LETTER SQUAT REVERSED ESH
    LineBreakClass::ULB_AL, // 0x0286  # LATIN SMALL LETTER ESH WITH CURL
    LineBreakClass::ULB_AL, // 0x0287  # LATIN SMALL LETTER TURNED T
    LineBreakClass::ULB_AL, // 0x0288  # LATIN SMALL LETTER T WITH RETROFLEX HOOK
    LineBreakClass::ULB_AL, // 0x0289  # LATIN SMALL LETTER U BAR
    LineBreakClass::ULB_AL, // 0x028A  # LATIN SMALL LETTER UPSILON
    LineBreakClass::ULB_AL, // 0x028B  # LATIN SMALL LETTER V WITH HOOK
    LineBreakClass::ULB_AL, // 0x028C  # LATIN SMALL LETTER TURNED V
    LineBreakClass::ULB_AL, // 0x028D  # LATIN SMALL LETTER TURNED W
    LineBreakClass::ULB_AL, // 0x028E  # LATIN SMALL LETTER TURNED Y
    LineBreakClass::ULB_AL, // 0x028F  # LATIN LETTER SMALL CAPITAL Y
    LineBreakClass::ULB_AL, // 0x0290  # LATIN SMALL LETTER Z WITH RETROFLEX HOOK
    LineBreakClass::ULB_AL, // 0x0291  # LATIN SMALL LETTER Z WITH CURL
    LineBreakClass::ULB_AL, // 0x0292  # LATIN SMALL LETTER EZH
    LineBreakClass::ULB_AL, // 0x0293  # LATIN SMALL LETTER EZH WITH CURL
    LineBreakClass::ULB_AL, // 0x0294  # LATIN LETTER GLOTTAL STOP
    LineBreakClass::ULB_AL, // 0x0295  # LATIN LETTER PHARYNGEAL VOICED FRICATIVE
    LineBreakClass::ULB_AL, // 0x0296  # LATIN LETTER INVERTED GLOTTAL STOP
    LineBreakClass::ULB_AL, // 0x0297  # LATIN LETTER STRETCHED C
    LineBreakClass::ULB_AL, // 0x0298  # LATIN LETTER BILABIAL CLICK
    LineBreakClass::ULB_AL, // 0x0299  # LATIN LETTER SMALL CAPITAL B
    LineBreakClass::ULB_AL, // 0x029A  # LATIN SMALL LETTER CLOSED OPEN E
    LineBreakClass::ULB_AL, // 0x029B  # LATIN LETTER SMALL CAPITAL G WITH HOOK
    LineBreakClass::ULB_AL, // 0x029C  # LATIN LETTER SMALL CAPITAL H
    LineBreakClass::ULB_AL, // 0x029D  # LATIN SMALL LETTER J WITH CROSSED-TAIL
    LineBreakClass::ULB_AL, // 0x029E  # LATIN SMALL LETTER TURNED K
    LineBreakClass::ULB_AL, // 0x029F  # LATIN LETTER SMALL CAPITAL L
    LineBreakClass::ULB_AL, // 0x02A0  # LATIN SMALL LETTER Q WITH HOOK
    LineBreakClass::ULB_AL, // 0x02A1  # LATIN LETTER GLOTTAL STOP WITH STROKE
    LineBreakClass::ULB_AL, // 0x02A2  # LATIN LETTER REVERSED GLOTTAL STOP WITH STROKE
    LineBreakClass::ULB_AL, // 0x02A3  # LATIN SMALL LETTER DZ DIGRAPH
    LineBreakClass::ULB_AL, // 0x02A4  # LATIN SMALL LETTER DEZH DIGRAPH
    LineBreakClass::ULB_AL, // 0x02A5  # LATIN SMALL LETTER DZ DIGRAPH WITH CURL
    LineBreakClass::ULB_AL, // 0x02A6  # LATIN SMALL LETTER TS DIGRAPH
    LineBreakClass::ULB_AL, // 0x02A7  # LATIN SMALL LETTER TESH DIGRAPH
    LineBreakClass::ULB_AL, // 0x02A8  # LATIN SMALL LETTER TC DIGRAPH WITH CURL
    LineBreakClass::ULB_AL, // 0x02A9  # LATIN SMALL LETTER FENG DIGRAPH
    LineBreakClass::ULB_AL, // 0x02AA  # LATIN SMALL LETTER LS DIGRAPH
    LineBreakClass::ULB_AL, // 0x02AB  # LATIN SMALL LETTER LZ DIGRAPH
    LineBreakClass::ULB_AL, // 0x02AC  # LATIN LETTER BILABIAL PERCUSSIVE
    LineBreakClass::ULB_AL, // 0x02AD  # LATIN LETTER BIDENTAL PERCUSSIVE
    LineBreakClass::ULB_AL, // 0x02AE  # LATIN SMALL LETTER TURNED H WITH FISHHOOK
    LineBreakClass::ULB_AL, // 0x02AF  # LATIN SMALL LETTER TURNED H WITH FISHHOOK AND TAIL
    LineBreakClass::ULB_AL, // 0x02B0  # MODIFIER LETTER SMALL H
    LineBreakClass::ULB_AL, // 0x02B1  # MODIFIER LETTER SMALL H WITH HOOK
    LineBreakClass::ULB_AL, // 0x02B2  # MODIFIER LETTER SMALL J
    LineBreakClass::ULB_AL, // 0x02B3  # MODIFIER LETTER SMALL R
    LineBreakClass::ULB_AL, // 0x02B4  # MODIFIER LETTER SMALL TURNED R
    LineBreakClass::ULB_AL, // 0x02B5  # MODIFIER LETTER SMALL TURNED R WITH HOOK
    LineBreakClass::ULB_AL, // 0x02B6  # MODIFIER LETTER SMALL CAPITAL INVERTED R
    LineBreakClass::ULB_AL, // 0x02B7  # MODIFIER LETTER SMALL W
    LineBreakClass::ULB_AL, // 0x02B8  # MODIFIER LETTER SMALL Y
    LineBreakClass::ULB_AL, // 0x02B9  # MODIFIER LETTER PRIME
    LineBreakClass::ULB_AL, // 0x02BA  # MODIFIER LETTER DOUBLE PRIME
    LineBreakClass::ULB_AL, // 0x02BB  # MODIFIER LETTER TURNED COMMA
    LineBreakClass::ULB_AL, // 0x02BC  # MODIFIER LETTER APOSTROPHE
    LineBreakClass::ULB_AL, // 0x02BD  # MODIFIER LETTER REVERSED COMMA
    LineBreakClass::ULB_AL, // 0x02BE  # MODIFIER LETTER RIGHT HALF RING
    LineBreakClass::ULB_AL, // 0x02BF  # MODIFIER LETTER LEFT HALF RING
    LineBreakClass::ULB_AL, // 0x02C0  # MODIFIER LETTER GLOTTAL STOP
    LineBreakClass::ULB_AL, // 0x02C1  # MODIFIER LETTER REVERSED GLOTTAL STOP
    LineBreakClass::ULB_AL, // 0x02C2  # MODIFIER LETTER LEFT ARROWHEAD
    LineBreakClass::ULB_AL, // 0x02C3  # MODIFIER LETTER RIGHT ARROWHEAD
    LineBreakClass::ULB_AL, // 0x02C4  # MODIFIER LETTER UP ARROWHEAD
    LineBreakClass::ULB_AL, // 0x02C5  # MODIFIER LETTER DOWN ARROWHEAD
    LineBreakClass::ULB_AL, // 0x02C6  # MODIFIER LETTER CIRCUMFLEX ACCENT
    LineBreakClass::ULB_AI, // 0x02C7  # CARON
    LineBreakClass::ULB_BB, // 0x02C8  # MODIFIER LETTER VERTICAL LINE
    LineBreakClass::ULB_AI, // 0x02C9  # MODIFIER LETTER MACRON
    LineBreakClass::ULB_AI, // 0x02CA  # MODIFIER LETTER ACUTE ACCENT
    LineBreakClass::ULB_AI, // 0x02CB  # MODIFIER LETTER GRAVE ACCENT
    LineBreakClass::ULB_BB, // 0x02CC  # MODIFIER LETTER LOW VERTICAL LINE
    LineBreakClass::ULB_AI, // 0x02CD  # MODIFIER LETTER LOW MACRON
    LineBreakClass::ULB_AL, // 0x02CE  # MODIFIER LETTER LOW GRAVE ACCENT
    LineBreakClass::ULB_AL, // 0x02CF  # MODIFIER LETTER LOW ACUTE ACCENT
    LineBreakClass::ULB_AI, // 0x02D0  # MODIFIER LETTER TRIANGULAR COLON
    LineBreakClass::ULB_AL, // 0x02D1  # MODIFIER LETTER HALF TRIANGULAR COLON
    LineBreakClass::ULB_AL, // 0x02D2  # MODIFIER LETTER CENTRED RIGHT HALF RING
    LineBreakClass::ULB_AL, // 0x02D3  # MODIFIER LETTER CENTRED LEFT HALF RING
    LineBreakClass::ULB_AL, // 0x02D4  # MODIFIER LETTER UP TACK
    LineBreakClass::ULB_AL, // 0x02D5  # MODIFIER LETTER DOWN TACK
    LineBreakClass::ULB_AL, // 0x02D6  # MODIFIER LETTER PLUS SIGN
    LineBreakClass::ULB_AL, // 0x02D7  # MODIFIER LETTER MINUS SIGN
    LineBreakClass::ULB_AI, // 0x02D8  # BREVE
    LineBreakClass::ULB_AI, // 0x02D9  # DOT ABOVE
    LineBreakClass::ULB_AI, // 0x02DA  # RING ABOVE
    LineBreakClass::ULB_AI, // 0x02DB  # OGONEK
    LineBreakClass::ULB_AL, // 0x02DC  # SMALL TILDE
    LineBreakClass::ULB_AI, // 0x02DD  # DOUBLE ACUTE ACCENT
    LineBreakClass::ULB_AL, // 0x02DE  # MODIFIER LETTER RHOTIC HOOK
    LineBreakClass::ULB_AL, // 0x02DF  # MODIFIER LETTER CROSS ACCENT
    LineBreakClass::ULB_AL, // 0x02E0  # MODIFIER LETTER SMALL GAMMA
    LineBreakClass::ULB_AL, // 0x02E1  # MODIFIER LETTER SMALL L
    LineBreakClass::ULB_AL, // 0x02E2  # MODIFIER LETTER SMALL S
    LineBreakClass::ULB_AL, // 0x02E3  # MODIFIER LETTER SMALL X
    LineBreakClass::ULB_AL, // 0x02E4  # MODIFIER LETTER SMALL REVERSED GLOTTAL STOP
    LineBreakClass::ULB_AL, // 0x02E5  # MODIFIER LETTER EXTRA-HIGH TONE BAR
    LineBreakClass::ULB_AL, // 0x02E6  # MODIFIER LETTER HIGH TONE BAR
    LineBreakClass::ULB_AL, // 0x02E7  # MODIFIER LETTER MID TONE BAR
    LineBreakClass::ULB_AL, // 0x02E8  # MODIFIER LETTER LOW TONE BAR
    LineBreakClass::ULB_AL, // 0x02E9  # MODIFIER LETTER EXTRA-LOW TONE BAR
    LineBreakClass::ULB_AL, // 0x02EA  # MODIFIER LETTER YIN DEPARTING TONE MARK
    LineBreakClass::ULB_AL, // 0x02EB  # MODIFIER LETTER YANG DEPARTING TONE MARK
    LineBreakClass::ULB_AL, // 0x02EC  # MODIFIER LETTER VOICING
    LineBreakClass::ULB_AL, // 0x02ED  # MODIFIER LETTER UNASPIRATED
    LineBreakClass::ULB_AL, // 0x02EE  # MODIFIER LETTER DOUBLE APOSTROPHE
    LineBreakClass::ULB_AL, // 0x02EF  # MODIFIER LETTER LOW DOWN ARROWHEAD
    LineBreakClass::ULB_AL, // 0x02F0  # MODIFIER LETTER LOW UP ARROWHEAD
    LineBreakClass::ULB_AL, // 0x02F1  # MODIFIER LETTER LOW LEFT ARROWHEAD
    LineBreakClass::ULB_AL, // 0x02F2  # MODIFIER LETTER LOW RIGHT ARROWHEAD
    LineBreakClass::ULB_AL, // 0x02F3  # MODIFIER LETTER LOW RING
    LineBreakClass::ULB_AL, // 0x02F4  # MODIFIER LETTER MIDDLE GRAVE ACCENT
    LineBreakClass::ULB_AL, // 0x02F5  # MODIFIER LETTER MIDDLE DOUBLE GRAVE ACCENT
    LineBreakClass::ULB_AL, // 0x02F6  # MODIFIER LETTER MIDDLE DOUBLE ACUTE ACCENT
    LineBreakClass::ULB_AL, // 0x02F7  # MODIFIER LETTER LOW TILDE
    LineBreakClass::ULB_AL, // 0x02F8  # MODIFIER LETTER RAISED COLON
    LineBreakClass::ULB_AL, // 0x02F9  # MODIFIER LETTER BEGIN HIGH TONE
    LineBreakClass::ULB_AL, // 0x02FA  # MODIFIER LETTER END HIGH TONE
    LineBreakClass::ULB_AL, // 0x02FB  # MODIFIER LETTER BEGIN LOW TONE
    LineBreakClass::ULB_AL, // 0x02FC  # MODIFIER LETTER END LOW TONE
    LineBreakClass::ULB_AL, // 0x02FD  # MODIFIER LETTER SHELF
    LineBreakClass::ULB_AL, // 0x02FE  # MODIFIER LETTER OPEN SHELF
    LineBreakClass::ULB_AL, // 0x02FF  # MODIFIER LETTER LOW LEFT ARROW
    LineBreakClass::ULB_CM, // 0x0300  # COMBINING GRAVE ACCENT
    LineBreakClass::ULB_CM, // 0x0301  # COMBINING ACUTE ACCENT
    LineBreakClass::ULB_CM, // 0x0302  # COMBINING CIRCUMFLEX ACCENT
    LineBreakClass::ULB_CM, // 0x0303  # COMBINING TILDE
    LineBreakClass::ULB_CM, // 0x0304  # COMBINING MACRON
    LineBreakClass::ULB_CM, // 0x0305  # COMBINING OVERLINE
    LineBreakClass::ULB_CM, // 0x0306  # COMBINING BREVE
    LineBreakClass::ULB_CM, // 0x0307  # COMBINING DOT ABOVE
    LineBreakClass::ULB_CM, // 0x0308  # COMBINING DIAERESIS
    LineBreakClass::ULB_CM, // 0x0309  # COMBINING HOOK ABOVE
    LineBreakClass::ULB_CM, // 0x030A  # COMBINING RING ABOVE
    LineBreakClass::ULB_CM, // 0x030B  # COMBINING DOUBLE ACUTE ACCENT
    LineBreakClass::ULB_CM, // 0x030C  # COMBINING CARON
    LineBreakClass::ULB_CM, // 0x030D  # COMBINING VERTICAL LINE ABOVE
    LineBreakClass::ULB_CM, // 0x030E  # COMBINING DOUBLE VERTICAL LINE ABOVE
    LineBreakClass::ULB_CM, // 0x030F  # COMBINING DOUBLE GRAVE ACCENT
    LineBreakClass::ULB_CM, // 0x0310  # COMBINING CANDRABINDU
    LineBreakClass::ULB_CM, // 0x0311  # COMBINING INVERTED BREVE
    LineBreakClass::ULB_CM, // 0x0312  # COMBINING TURNED COMMA ABOVE
    LineBreakClass::ULB_CM, // 0x0313  # COMBINING COMMA ABOVE
    LineBreakClass::ULB_CM, // 0x0314  # COMBINING REVERSED COMMA ABOVE
    LineBreakClass::ULB_CM, // 0x0315  # COMBINING COMMA ABOVE RIGHT
    LineBreakClass::ULB_CM, // 0x0316  # COMBINING GRAVE ACCENT BELOW
    LineBreakClass::ULB_CM, // 0x0317  # COMBINING ACUTE ACCENT BELOW
    LineBreakClass::ULB_CM, // 0x0318  # COMBINING LEFT TACK BELOW
    LineBreakClass::ULB_CM, // 0x0319  # COMBINING RIGHT TACK BELOW
    LineBreakClass::ULB_CM, // 0x031A  # COMBINING LEFT ANGLE ABOVE
    LineBreakClass::ULB_CM, // 0x031B  # COMBINING HORN
    LineBreakClass::ULB_CM, // 0x031C  # COMBINING LEFT HALF RING BELOW
    LineBreakClass::ULB_CM, // 0x031D  # COMBINING UP TACK BELOW
    LineBreakClass::ULB_CM, // 0x031E  # COMBINING DOWN TACK BELOW
    LineBreakClass::ULB_CM, // 0x031F  # COMBINING PLUS SIGN BELOW
    LineBreakClass::ULB_CM, // 0x0320  # COMBINING MINUS SIGN BELOW
    LineBreakClass::ULB_CM, // 0x0321  # COMBINING PALATALIZED HOOK BELOW
    LineBreakClass::ULB_CM, // 0x0322  # COMBINING RETROFLEX HOOK BELOW
    LineBreakClass::ULB_CM, // 0x0323  # COMBINING DOT BELOW
    LineBreakClass::ULB_CM, // 0x0324  # COMBINING DIAERESIS BELOW
    LineBreakClass::ULB_CM, // 0x0325  # COMBINING RING BELOW
    LineBreakClass::ULB_CM, // 0x0326  # COMBINING COMMA BELOW
    LineBreakClass::ULB_CM, // 0x0327  # COMBINING CEDILLA
    LineBreakClass::ULB_CM, // 0x0328  # COMBINING OGONEK
    LineBreakClass::ULB_CM, // 0x0329  # COMBINING VERTICAL LINE BELOW
    LineBreakClass::ULB_CM, // 0x032A  # COMBINING BRIDGE BELOW
    LineBreakClass::ULB_CM, // 0x032B  # COMBINING INVERTED DOUBLE ARCH BELOW
    LineBreakClass::ULB_CM, // 0x032C  # COMBINING CARON BELOW
    LineBreakClass::ULB_CM, // 0x032D  # COMBINING CIRCUMFLEX ACCENT BELOW
    LineBreakClass::ULB_CM, // 0x032E  # COMBINING BREVE BELOW
    LineBreakClass::ULB_CM, // 0x032F  # COMBINING INVERTED BREVE BELOW
    LineBreakClass::ULB_CM, // 0x0330  # COMBINING TILDE BELOW
    LineBreakClass::ULB_CM, // 0x0331  # COMBINING MACRON BELOW
    LineBreakClass::ULB_CM, // 0x0332  # COMBINING LOW LINE
    LineBreakClass::ULB_CM, // 0x0333  # COMBINING DOUBLE LOW LINE
    LineBreakClass::ULB_CM, // 0x0334  # COMBINING TILDE OVERLAY
    LineBreakClass::ULB_CM, // 0x0335  # COMBINING SHORT STROKE OVERLAY
    LineBreakClass::ULB_CM, // 0x0336  # COMBINING LONG STROKE OVERLAY
    LineBreakClass::ULB_CM, // 0x0337  # COMBINING SHORT SOLIDUS OVERLAY
    LineBreakClass::ULB_CM, // 0x0338  # COMBINING LONG SOLIDUS OVERLAY
    LineBreakClass::ULB_CM, // 0x0339  # COMBINING RIGHT HALF RING BELOW
    LineBreakClass::ULB_CM, // 0x033A  # COMBINING INVERTED BRIDGE BELOW
    LineBreakClass::ULB_CM, // 0x033B  # COMBINING SQUARE BELOW
    LineBreakClass::ULB_CM, // 0x033C  # COMBINING SEAGULL BELOW
    LineBreakClass::ULB_CM, // 0x033D  # COMBINING X ABOVE
    LineBreakClass::ULB_CM, // 0x033E  # COMBINING VERTICAL TILDE
    LineBreakClass::ULB_CM, // 0x033F  # COMBINING DOUBLE OVERLINE
    LineBreakClass::ULB_CM, // 0x0340  # COMBINING GRAVE TONE MARK
    LineBreakClass::ULB_CM, // 0x0341  # COMBINING ACUTE TONE MARK
    LineBreakClass::ULB_CM, // 0x0342  # COMBINING GREEK PERISPOMENI
    LineBreakClass::ULB_CM, // 0x0343  # COMBINING GREEK KORONIS
    LineBreakClass::ULB_CM, // 0x0344  # COMBINING GREEK DIALYTIKA TONOS
    LineBreakClass::ULB_CM, // 0x0345  # COMBINING GREEK YPOGEGRAMMENI
    LineBreakClass::ULB_CM, // 0x0346  # COMBINING BRIDGE ABOVE
    LineBreakClass::ULB_CM, // 0x0347  # COMBINING EQUALS SIGN BELOW
    LineBreakClass::ULB_CM, // 0x0348  # COMBINING DOUBLE VERTICAL LINE BELOW
    LineBreakClass::ULB_CM, // 0x0349  # COMBINING LEFT ANGLE BELOW
    LineBreakClass::ULB_CM, // 0x034A  # COMBINING NOT TILDE ABOVE
    LineBreakClass::ULB_CM, // 0x034B  # COMBINING HOMOTHETIC ABOVE
    LineBreakClass::ULB_CM, // 0x034C  # COMBINING ALMOST EQUAL TO ABOVE
    LineBreakClass::ULB_CM, // 0x034D  # COMBINING LEFT RIGHT ARROW BELOW
    LineBreakClass::ULB_CM, // 0x034E  # COMBINING UPWARDS ARROW BELOW
    LineBreakClass::ULB_GL, // 0x034F  # COMBINING GRAPHEME JOINER
    LineBreakClass::ULB_CM, // 0x0350  # COMBINING RIGHT ARROWHEAD ABOVE
    LineBreakClass::ULB_CM, // 0x0351  # COMBINING LEFT HALF RING ABOVE
    LineBreakClass::ULB_CM, // 0x0352  # COMBINING FERMATA
    LineBreakClass::ULB_CM, // 0x0353  # COMBINING X BELOW
    LineBreakClass::ULB_CM, // 0x0354  # COMBINING LEFT ARROWHEAD BELOW
    LineBreakClass::ULB_CM, // 0x0355  # COMBINING RIGHT ARROWHEAD BELOW
    LineBreakClass::ULB_CM, // 0x0356  # COMBINING RIGHT ARROWHEAD AND UP ARROWHEAD BELOW
    LineBreakClass::ULB_CM, // 0x0357  # COMBINING RIGHT HALF RING ABOVE
    LineBreakClass::ULB_CM, // 0x0358  # COMBINING DOT ABOVE RIGHT
    LineBreakClass::ULB_CM, // 0x0359  # COMBINING ASTERISK BELOW
    LineBreakClass::ULB_CM, // 0x035A  # COMBINING DOUBLE RING BELOW
    LineBreakClass::ULB_CM, // 0x035B  # COMBINING ZIGZAG ABOVE
    LineBreakClass::ULB_GL, // 0x035C  # COMBINING DOUBLE BREVE BELOW
    LineBreakClass::ULB_GL, // 0x035D  # COMBINING DOUBLE BREVE
    LineBreakClass::ULB_GL, // 0x035E  # COMBINING DOUBLE MACRON
    LineBreakClass::ULB_GL, // 0x035F  # COMBINING DOUBLE MACRON BELOW
    LineBreakClass::ULB_GL, // 0x0360  # COMBINING DOUBLE TILDE
    LineBreakClass::ULB_GL, // 0x0361  # COMBINING DOUBLE INVERTED BREVE
    LineBreakClass::ULB_GL, // 0x0362  # COMBINING DOUBLE RIGHTWARDS ARROW BELOW
    LineBreakClass::ULB_CM, // 0x0363  # COMBINING LATIN SMALL LETTER A
    LineBreakClass::ULB_CM, // 0x0364  # COMBINING LATIN SMALL LETTER E
    LineBreakClass::ULB_CM, // 0x0365  # COMBINING LATIN SMALL LETTER I
    LineBreakClass::ULB_CM, // 0x0366  # COMBINING LATIN SMALL LETTER O
    LineBreakClass::ULB_CM, // 0x0367  # COMBINING LATIN SMALL LETTER U
    LineBreakClass::ULB_CM, // 0x0368  # COMBINING LATIN SMALL LETTER C
    LineBreakClass::ULB_CM, // 0x0369  # COMBINING LATIN SMALL LETTER D
    LineBreakClass::ULB_CM, // 0x036A  # COMBINING LATIN SMALL LETTER H
    LineBreakClass::ULB_CM, // 0x036B  # COMBINING LATIN SMALL LETTER M
    LineBreakClass::ULB_CM, // 0x036C  # COMBINING LATIN SMALL LETTER R
    LineBreakClass::ULB_CM, // 0x036D  # COMBINING LATIN SMALL LETTER T
    LineBreakClass::ULB_CM, // 0x036E  # COMBINING LATIN SMALL LETTER V
    LineBreakClass::ULB_CM, // 0x036F  # COMBINING LATIN SMALL LETTER X
    LineBreakClass::ULB_ID, // 0x0370 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0371 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0372 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0373 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0374  # GREEK NUMERAL SIGN
    LineBreakClass::ULB_AL, // 0x0375  # GREEK LOWER NUMERAL SIGN
    LineBreakClass::ULB_ID, // 0x0376 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0377 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0378 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0379 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x037A  # GREEK YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x037B  # GREEK SMALL REVERSED LUNATE SIGMA SYMBOL
    LineBreakClass::ULB_AL, // 0x037C  # GREEK SMALL DOTTED LUNATE SIGMA SYMBOL
    LineBreakClass::ULB_AL, // 0x037D  # GREEK SMALL REVERSED DOTTED LUNATE SIGMA SYMBOL
    LineBreakClass::ULB_IS, // 0x037E  # GREEK QUESTION MARK
    LineBreakClass::ULB_ID, // 0x037F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0380 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0381 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0382 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0383 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0384  # GREEK TONOS
    LineBreakClass::ULB_AL, // 0x0385  # GREEK DIALYTIKA TONOS
    LineBreakClass::ULB_AL, // 0x0386  # GREEK CAPITAL LETTER ALPHA WITH TONOS
    LineBreakClass::ULB_AL, // 0x0387  # GREEK ANO TELEIA
    LineBreakClass::ULB_AL, // 0x0388  # GREEK CAPITAL LETTER EPSILON WITH TONOS
    LineBreakClass::ULB_AL, // 0x0389  # GREEK CAPITAL LETTER ETA WITH TONOS
    LineBreakClass::ULB_AL, // 0x038A  # GREEK CAPITAL LETTER IOTA WITH TONOS
    LineBreakClass::ULB_ID, // 0x038B # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x038C  # GREEK CAPITAL LETTER OMICRON WITH TONOS
    LineBreakClass::ULB_ID, // 0x038D # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x038E  # GREEK CAPITAL LETTER UPSILON WITH TONOS
    LineBreakClass::ULB_AL, // 0x038F  # GREEK CAPITAL LETTER OMEGA WITH TONOS
    LineBreakClass::ULB_AL, // 0x0390  # GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS
    LineBreakClass::ULB_AL, // 0x0391  # GREEK CAPITAL LETTER ALPHA
    LineBreakClass::ULB_AL, // 0x0392  # GREEK CAPITAL LETTER BETA
    LineBreakClass::ULB_AL, // 0x0393  # GREEK CAPITAL LETTER GAMMA
    LineBreakClass::ULB_AL, // 0x0394  # GREEK CAPITAL LETTER DELTA
    LineBreakClass::ULB_AL, // 0x0395  # GREEK CAPITAL LETTER EPSILON
    LineBreakClass::ULB_AL, // 0x0396  # GREEK CAPITAL LETTER ZETA
    LineBreakClass::ULB_AL, // 0x0397  # GREEK CAPITAL LETTER ETA
    LineBreakClass::ULB_AL, // 0x0398  # GREEK CAPITAL LETTER THETA
    LineBreakClass::ULB_AL, // 0x0399  # GREEK CAPITAL LETTER IOTA
    LineBreakClass::ULB_AL, // 0x039A  # GREEK CAPITAL LETTER KAPPA
    LineBreakClass::ULB_AL, // 0x039B  # GREEK CAPITAL LETTER LAMDA
    LineBreakClass::ULB_AL, // 0x039C  # GREEK CAPITAL LETTER MU
    LineBreakClass::ULB_AL, // 0x039D  # GREEK CAPITAL LETTER NU
    LineBreakClass::ULB_AL, // 0x039E  # GREEK CAPITAL LETTER XI
    LineBreakClass::ULB_AL, // 0x039F  # GREEK CAPITAL LETTER OMICRON
    LineBreakClass::ULB_AL, // 0x03A0  # GREEK CAPITAL LETTER PI
    LineBreakClass::ULB_AL, // 0x03A1  # GREEK CAPITAL LETTER RHO
    LineBreakClass::ULB_ID, // 0x03A2 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x03A3  # GREEK CAPITAL LETTER SIGMA
    LineBreakClass::ULB_AL, // 0x03A4  # GREEK CAPITAL LETTER TAU
    LineBreakClass::ULB_AL, // 0x03A5  # GREEK CAPITAL LETTER UPSILON
    LineBreakClass::ULB_AL, // 0x03A6  # GREEK CAPITAL LETTER PHI
    LineBreakClass::ULB_AL, // 0x03A7  # GREEK CAPITAL LETTER CHI
    LineBreakClass::ULB_AL, // 0x03A8  # GREEK CAPITAL LETTER PSI
    LineBreakClass::ULB_AL, // 0x03A9  # GREEK CAPITAL LETTER OMEGA
    LineBreakClass::ULB_AL, // 0x03AA  # GREEK CAPITAL LETTER IOTA WITH DIALYTIKA
    LineBreakClass::ULB_AL, // 0x03AB  # GREEK CAPITAL LETTER UPSILON WITH DIALYTIKA
    LineBreakClass::ULB_AL, // 0x03AC  # GREEK SMALL LETTER ALPHA WITH TONOS
    LineBreakClass::ULB_AL, // 0x03AD  # GREEK SMALL LETTER EPSILON WITH TONOS
    LineBreakClass::ULB_AL, // 0x03AE  # GREEK SMALL LETTER ETA WITH TONOS
    LineBreakClass::ULB_AL, // 0x03AF  # GREEK SMALL LETTER IOTA WITH TONOS
    LineBreakClass::ULB_AL, // 0x03B0  # GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS
    LineBreakClass::ULB_AL, // 0x03B1  # GREEK SMALL LETTER ALPHA
    LineBreakClass::ULB_AL, // 0x03B2  # GREEK SMALL LETTER BETA
    LineBreakClass::ULB_AL, // 0x03B3  # GREEK SMALL LETTER GAMMA
    LineBreakClass::ULB_AL, // 0x03B4  # GREEK SMALL LETTER DELTA
    LineBreakClass::ULB_AL, // 0x03B5  # GREEK SMALL LETTER EPSILON
    LineBreakClass::ULB_AL, // 0x03B6  # GREEK SMALL LETTER ZETA
    LineBreakClass::ULB_AL, // 0x03B7  # GREEK SMALL LETTER ETA
    LineBreakClass::ULB_AL, // 0x03B8  # GREEK SMALL LETTER THETA
    LineBreakClass::ULB_AL, // 0x03B9  # GREEK SMALL LETTER IOTA
    LineBreakClass::ULB_AL, // 0x03BA  # GREEK SMALL LETTER KAPPA
    LineBreakClass::ULB_AL, // 0x03BB  # GREEK SMALL LETTER LAMDA
    LineBreakClass::ULB_AL, // 0x03BC  # GREEK SMALL LETTER MU
    LineBreakClass::ULB_AL, // 0x03BD  # GREEK SMALL LETTER NU
    LineBreakClass::ULB_AL, // 0x03BE  # GREEK SMALL LETTER XI
    LineBreakClass::ULB_AL, // 0x03BF  # GREEK SMALL LETTER OMICRON
    LineBreakClass::ULB_AL, // 0x03C0  # GREEK SMALL LETTER PI
    LineBreakClass::ULB_AL, // 0x03C1  # GREEK SMALL LETTER RHO
    LineBreakClass::ULB_AL, // 0x03C2  # GREEK SMALL LETTER FINAL SIGMA
    LineBreakClass::ULB_AL, // 0x03C3  # GREEK SMALL LETTER SIGMA
    LineBreakClass::ULB_AL, // 0x03C4  # GREEK SMALL LETTER TAU
    LineBreakClass::ULB_AL, // 0x03C5  # GREEK SMALL LETTER UPSILON
    LineBreakClass::ULB_AL, // 0x03C6  # GREEK SMALL LETTER PHI
    LineBreakClass::ULB_AL, // 0x03C7  # GREEK SMALL LETTER CHI
    LineBreakClass::ULB_AL, // 0x03C8  # GREEK SMALL LETTER PSI
    LineBreakClass::ULB_AL, // 0x03C9  # GREEK SMALL LETTER OMEGA
    LineBreakClass::ULB_AL, // 0x03CA  # GREEK SMALL LETTER IOTA WITH DIALYTIKA
    LineBreakClass::ULB_AL, // 0x03CB  # GREEK SMALL LETTER UPSILON WITH DIALYTIKA
    LineBreakClass::ULB_AL, // 0x03CC  # GREEK SMALL LETTER OMICRON WITH TONOS
    LineBreakClass::ULB_AL, // 0x03CD  # GREEK SMALL LETTER UPSILON WITH TONOS
    LineBreakClass::ULB_AL, // 0x03CE  # GREEK SMALL LETTER OMEGA WITH TONOS
    LineBreakClass::ULB_ID, // 0x03CF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x03D0  # GREEK BETA SYMBOL
    LineBreakClass::ULB_AL, // 0x03D1  # GREEK THETA SYMBOL
    LineBreakClass::ULB_AL, // 0x03D2  # GREEK UPSILON WITH HOOK SYMBOL
    LineBreakClass::ULB_AL, // 0x03D3  # GREEK UPSILON WITH ACUTE AND HOOK SYMBOL
    LineBreakClass::ULB_AL, // 0x03D4  # GREEK UPSILON WITH DIAERESIS AND HOOK SYMBOL
    LineBreakClass::ULB_AL, // 0x03D5  # GREEK PHI SYMBOL
    LineBreakClass::ULB_AL, // 0x03D6  # GREEK PI SYMBOL
    LineBreakClass::ULB_AL, // 0x03D7  # GREEK KAI SYMBOL
    LineBreakClass::ULB_AL, // 0x03D8  # GREEK LETTER ARCHAIC KOPPA
    LineBreakClass::ULB_AL, // 0x03D9  # GREEK SMALL LETTER ARCHAIC KOPPA
    LineBreakClass::ULB_AL, // 0x03DA  # GREEK LETTER STIGMA
    LineBreakClass::ULB_AL, // 0x03DB  # GREEK SMALL LETTER STIGMA
    LineBreakClass::ULB_AL, // 0x03DC  # GREEK LETTER DIGAMMA
    LineBreakClass::ULB_AL, // 0x03DD  # GREEK SMALL LETTER DIGAMMA
    LineBreakClass::ULB_AL, // 0x03DE  # GREEK LETTER KOPPA
    LineBreakClass::ULB_AL, // 0x03DF  # GREEK SMALL LETTER KOPPA
    LineBreakClass::ULB_AL, // 0x03E0  # GREEK LETTER SAMPI
    LineBreakClass::ULB_AL, // 0x03E1  # GREEK SMALL LETTER SAMPI
    LineBreakClass::ULB_AL, // 0x03E2  # COPTIC CAPITAL LETTER SHEI
    LineBreakClass::ULB_AL, // 0x03E3  # COPTIC SMALL LETTER SHEI
    LineBreakClass::ULB_AL, // 0x03E4  # COPTIC CAPITAL LETTER FEI
    LineBreakClass::ULB_AL, // 0x03E5  # COPTIC SMALL LETTER FEI
    LineBreakClass::ULB_AL, // 0x03E6  # COPTIC CAPITAL LETTER KHEI
    LineBreakClass::ULB_AL, // 0x03E7  # COPTIC SMALL LETTER KHEI
    LineBreakClass::ULB_AL, // 0x03E8  # COPTIC CAPITAL LETTER HORI
    LineBreakClass::ULB_AL, // 0x03E9  # COPTIC SMALL LETTER HORI
    LineBreakClass::ULB_AL, // 0x03EA  # COPTIC CAPITAL LETTER GANGIA
    LineBreakClass::ULB_AL, // 0x03EB  # COPTIC SMALL LETTER GANGIA
    LineBreakClass::ULB_AL, // 0x03EC  # COPTIC CAPITAL LETTER SHIMA
    LineBreakClass::ULB_AL, // 0x03ED  # COPTIC SMALL LETTER SHIMA
    LineBreakClass::ULB_AL, // 0x03EE  # COPTIC CAPITAL LETTER DEI
    LineBreakClass::ULB_AL, // 0x03EF  # COPTIC SMALL LETTER DEI
    LineBreakClass::ULB_AL, // 0x03F0  # GREEK KAPPA SYMBOL
    LineBreakClass::ULB_AL, // 0x03F1  # GREEK RHO SYMBOL
    LineBreakClass::ULB_AL, // 0x03F2  # GREEK LUNATE SIGMA SYMBOL
    LineBreakClass::ULB_AL, // 0x03F3  # GREEK LETTER YOT
    LineBreakClass::ULB_AL, // 0x03F4  # GREEK CAPITAL THETA SYMBOL
    LineBreakClass::ULB_AL, // 0x03F5  # GREEK LUNATE EPSILON SYMBOL
    LineBreakClass::ULB_AL, // 0x03F6  # GREEK REVERSED LUNATE EPSILON SYMBOL
    LineBreakClass::ULB_AL, // 0x03F7  # GREEK CAPITAL LETTER SHO
    LineBreakClass::ULB_AL, // 0x03F8  # GREEK SMALL LETTER SHO
    LineBreakClass::ULB_AL, // 0x03F9  # GREEK CAPITAL LUNATE SIGMA SYMBOL
    LineBreakClass::ULB_AL, // 0x03FA  # GREEK CAPITAL LETTER SAN
    LineBreakClass::ULB_AL, // 0x03FB  # GREEK SMALL LETTER SAN
    LineBreakClass::ULB_AL, // 0x03FC  # GREEK RHO WITH STROKE SYMBOL
    LineBreakClass::ULB_AL, // 0x03FD  # GREEK CAPITAL REVERSED LUNATE SIGMA SYMBOL
    LineBreakClass::ULB_AL, // 0x03FE  # GREEK CAPITAL DOTTED LUNATE SIGMA SYMBOL
    LineBreakClass::ULB_AL, // 0x03FF  # GREEK CAPITAL REVERSED DOTTED LUNATE SIGMA SYMBOL
    LineBreakClass::ULB_AL, // 0x0400  # CYRILLIC CAPITAL LETTER IE WITH GRAVE
    LineBreakClass::ULB_AL, // 0x0401  # CYRILLIC CAPITAL LETTER IO
    LineBreakClass::ULB_AL, // 0x0402  # CYRILLIC CAPITAL LETTER DJE
    LineBreakClass::ULB_AL, // 0x0403  # CYRILLIC CAPITAL LETTER GJE
    LineBreakClass::ULB_AL, // 0x0404  # CYRILLIC CAPITAL LETTER UKRAINIAN IE
    LineBreakClass::ULB_AL, // 0x0405  # CYRILLIC CAPITAL LETTER DZE
    LineBreakClass::ULB_AL, // 0x0406  # CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I
    LineBreakClass::ULB_AL, // 0x0407  # CYRILLIC CAPITAL LETTER YI
    LineBreakClass::ULB_AL, // 0x0408  # CYRILLIC CAPITAL LETTER JE
    LineBreakClass::ULB_AL, // 0x0409  # CYRILLIC CAPITAL LETTER LJE
    LineBreakClass::ULB_AL, // 0x040A  # CYRILLIC CAPITAL LETTER NJE
    LineBreakClass::ULB_AL, // 0x040B  # CYRILLIC CAPITAL LETTER TSHE
    LineBreakClass::ULB_AL, // 0x040C  # CYRILLIC CAPITAL LETTER KJE
    LineBreakClass::ULB_AL, // 0x040D  # CYRILLIC CAPITAL LETTER I WITH GRAVE
    LineBreakClass::ULB_AL, // 0x040E  # CYRILLIC CAPITAL LETTER SHORT U
    LineBreakClass::ULB_AL, // 0x040F  # CYRILLIC CAPITAL LETTER DZHE
    LineBreakClass::ULB_AL, // 0x0410  # CYRILLIC CAPITAL LETTER A
    LineBreakClass::ULB_AL, // 0x0411  # CYRILLIC CAPITAL LETTER BE
    LineBreakClass::ULB_AL, // 0x0412  # CYRILLIC CAPITAL LETTER VE
    LineBreakClass::ULB_AL, // 0x0413  # CYRILLIC CAPITAL LETTER GHE
    LineBreakClass::ULB_AL, // 0x0414  # CYRILLIC CAPITAL LETTER DE
    LineBreakClass::ULB_AL, // 0x0415  # CYRILLIC CAPITAL LETTER IE
    LineBreakClass::ULB_AL, // 0x0416  # CYRILLIC CAPITAL LETTER ZHE
    LineBreakClass::ULB_AL, // 0x0417  # CYRILLIC CAPITAL LETTER ZE
    LineBreakClass::ULB_AL, // 0x0418  # CYRILLIC CAPITAL LETTER I
    LineBreakClass::ULB_AL, // 0x0419  # CYRILLIC CAPITAL LETTER SHORT I
    LineBreakClass::ULB_AL, // 0x041A  # CYRILLIC CAPITAL LETTER KA
    LineBreakClass::ULB_AL, // 0x041B  # CYRILLIC CAPITAL LETTER EL
    LineBreakClass::ULB_AL, // 0x041C  # CYRILLIC CAPITAL LETTER EM
    LineBreakClass::ULB_AL, // 0x041D  # CYRILLIC CAPITAL LETTER EN
    LineBreakClass::ULB_AL, // 0x041E  # CYRILLIC CAPITAL LETTER O
    LineBreakClass::ULB_AL, // 0x041F  # CYRILLIC CAPITAL LETTER PE
    LineBreakClass::ULB_AL, // 0x0420  # CYRILLIC CAPITAL LETTER ER
    LineBreakClass::ULB_AL, // 0x0421  # CYRILLIC CAPITAL LETTER ES
    LineBreakClass::ULB_AL, // 0x0422  # CYRILLIC CAPITAL LETTER TE
    LineBreakClass::ULB_AL, // 0x0423  # CYRILLIC CAPITAL LETTER U
    LineBreakClass::ULB_AL, // 0x0424  # CYRILLIC CAPITAL LETTER EF
    LineBreakClass::ULB_AL, // 0x0425  # CYRILLIC CAPITAL LETTER HA
    LineBreakClass::ULB_AL, // 0x0426  # CYRILLIC CAPITAL LETTER TSE
    LineBreakClass::ULB_AL, // 0x0427  # CYRILLIC CAPITAL LETTER CHE
    LineBreakClass::ULB_AL, // 0x0428  # CYRILLIC CAPITAL LETTER SHA
    LineBreakClass::ULB_AL, // 0x0429  # CYRILLIC CAPITAL LETTER SHCHA
    LineBreakClass::ULB_AL, // 0x042A  # CYRILLIC CAPITAL LETTER HARD SIGN
    LineBreakClass::ULB_AL, // 0x042B  # CYRILLIC CAPITAL LETTER YERU
    LineBreakClass::ULB_AL, // 0x042C  # CYRILLIC CAPITAL LETTER SOFT SIGN
    LineBreakClass::ULB_AL, // 0x042D  # CYRILLIC CAPITAL LETTER E
    LineBreakClass::ULB_AL, // 0x042E  # CYRILLIC CAPITAL LETTER YU
    LineBreakClass::ULB_AL, // 0x042F  # CYRILLIC CAPITAL LETTER YA
    LineBreakClass::ULB_AL, // 0x0430  # CYRILLIC SMALL LETTER A
    LineBreakClass::ULB_AL, // 0x0431  # CYRILLIC SMALL LETTER BE
    LineBreakClass::ULB_AL, // 0x0432  # CYRILLIC SMALL LETTER VE
    LineBreakClass::ULB_AL, // 0x0433  # CYRILLIC SMALL LETTER GHE
    LineBreakClass::ULB_AL, // 0x0434  # CYRILLIC SMALL LETTER DE
    LineBreakClass::ULB_AL, // 0x0435  # CYRILLIC SMALL LETTER IE
    LineBreakClass::ULB_AL, // 0x0436  # CYRILLIC SMALL LETTER ZHE
    LineBreakClass::ULB_AL, // 0x0437  # CYRILLIC SMALL LETTER ZE
    LineBreakClass::ULB_AL, // 0x0438  # CYRILLIC SMALL LETTER I
    LineBreakClass::ULB_AL, // 0x0439  # CYRILLIC SMALL LETTER SHORT I
    LineBreakClass::ULB_AL, // 0x043A  # CYRILLIC SMALL LETTER KA
    LineBreakClass::ULB_AL, // 0x043B  # CYRILLIC SMALL LETTER EL
    LineBreakClass::ULB_AL, // 0x043C  # CYRILLIC SMALL LETTER EM
    LineBreakClass::ULB_AL, // 0x043D  # CYRILLIC SMALL LETTER EN
    LineBreakClass::ULB_AL, // 0x043E  # CYRILLIC SMALL LETTER O
    LineBreakClass::ULB_AL, // 0x043F  # CYRILLIC SMALL LETTER PE
    LineBreakClass::ULB_AL, // 0x0440  # CYRILLIC SMALL LETTER ER
    LineBreakClass::ULB_AL, // 0x0441  # CYRILLIC SMALL LETTER ES
    LineBreakClass::ULB_AL, // 0x0442  # CYRILLIC SMALL LETTER TE
    LineBreakClass::ULB_AL, // 0x0443  # CYRILLIC SMALL LETTER U
    LineBreakClass::ULB_AL, // 0x0444  # CYRILLIC SMALL LETTER EF
    LineBreakClass::ULB_AL, // 0x0445  # CYRILLIC SMALL LETTER HA
    LineBreakClass::ULB_AL, // 0x0446  # CYRILLIC SMALL LETTER TSE
    LineBreakClass::ULB_AL, // 0x0447  # CYRILLIC SMALL LETTER CHE
    LineBreakClass::ULB_AL, // 0x0448  # CYRILLIC SMALL LETTER SHA
    LineBreakClass::ULB_AL, // 0x0449  # CYRILLIC SMALL LETTER SHCHA
    LineBreakClass::ULB_AL, // 0x044A  # CYRILLIC SMALL LETTER HARD SIGN
    LineBreakClass::ULB_AL, // 0x044B  # CYRILLIC SMALL LETTER YERU
    LineBreakClass::ULB_AL, // 0x044C  # CYRILLIC SMALL LETTER SOFT SIGN
    LineBreakClass::ULB_AL, // 0x044D  # CYRILLIC SMALL LETTER E
    LineBreakClass::ULB_AL, // 0x044E  # CYRILLIC SMALL LETTER YU
    LineBreakClass::ULB_AL, // 0x044F  # CYRILLIC SMALL LETTER YA
    LineBreakClass::ULB_AL, // 0x0450  # CYRILLIC SMALL LETTER IE WITH GRAVE
    LineBreakClass::ULB_AL, // 0x0451  # CYRILLIC SMALL LETTER IO
    LineBreakClass::ULB_AL, // 0x0452  # CYRILLIC SMALL LETTER DJE
    LineBreakClass::ULB_AL, // 0x0453  # CYRILLIC SMALL LETTER GJE
    LineBreakClass::ULB_AL, // 0x0454  # CYRILLIC SMALL LETTER UKRAINIAN IE
    LineBreakClass::ULB_AL, // 0x0455  # CYRILLIC SMALL LETTER DZE
    LineBreakClass::ULB_AL, // 0x0456  # CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I
    LineBreakClass::ULB_AL, // 0x0457  # CYRILLIC SMALL LETTER YI
    LineBreakClass::ULB_AL, // 0x0458  # CYRILLIC SMALL LETTER JE
    LineBreakClass::ULB_AL, // 0x0459  # CYRILLIC SMALL LETTER LJE
    LineBreakClass::ULB_AL, // 0x045A  # CYRILLIC SMALL LETTER NJE
    LineBreakClass::ULB_AL, // 0x045B  # CYRILLIC SMALL LETTER TSHE
    LineBreakClass::ULB_AL, // 0x045C  # CYRILLIC SMALL LETTER KJE
    LineBreakClass::ULB_AL, // 0x045D  # CYRILLIC SMALL LETTER I WITH GRAVE
    LineBreakClass::ULB_AL, // 0x045E  # CYRILLIC SMALL LETTER SHORT U
    LineBreakClass::ULB_AL, // 0x045F  # CYRILLIC SMALL LETTER DZHE
    LineBreakClass::ULB_AL, // 0x0460  # CYRILLIC CAPITAL LETTER OMEGA
    LineBreakClass::ULB_AL, // 0x0461  # CYRILLIC SMALL LETTER OMEGA
    LineBreakClass::ULB_AL, // 0x0462  # CYRILLIC CAPITAL LETTER YAT
    LineBreakClass::ULB_AL, // 0x0463  # CYRILLIC SMALL LETTER YAT
    LineBreakClass::ULB_AL, // 0x0464  # CYRILLIC CAPITAL LETTER IOTIFIED E
    LineBreakClass::ULB_AL, // 0x0465  # CYRILLIC SMALL LETTER IOTIFIED E
    LineBreakClass::ULB_AL, // 0x0466  # CYRILLIC CAPITAL LETTER LITTLE YUS
    LineBreakClass::ULB_AL, // 0x0467  # CYRILLIC SMALL LETTER LITTLE YUS
    LineBreakClass::ULB_AL, // 0x0468  # CYRILLIC CAPITAL LETTER IOTIFIED LITTLE YUS
    LineBreakClass::ULB_AL, // 0x0469  # CYRILLIC SMALL LETTER IOTIFIED LITTLE YUS
    LineBreakClass::ULB_AL, // 0x046A  # CYRILLIC CAPITAL LETTER BIG YUS
    LineBreakClass::ULB_AL, // 0x046B  # CYRILLIC SMALL LETTER BIG YUS
    LineBreakClass::ULB_AL, // 0x046C  # CYRILLIC CAPITAL LETTER IOTIFIED BIG YUS
    LineBreakClass::ULB_AL, // 0x046D  # CYRILLIC SMALL LETTER IOTIFIED BIG YUS
    LineBreakClass::ULB_AL, // 0x046E  # CYRILLIC CAPITAL LETTER KSI
    LineBreakClass::ULB_AL, // 0x046F  # CYRILLIC SMALL LETTER KSI
    LineBreakClass::ULB_AL, // 0x0470  # CYRILLIC CAPITAL LETTER PSI
    LineBreakClass::ULB_AL, // 0x0471  # CYRILLIC SMALL LETTER PSI
    LineBreakClass::ULB_AL, // 0x0472  # CYRILLIC CAPITAL LETTER FITA
    LineBreakClass::ULB_AL, // 0x0473  # CYRILLIC SMALL LETTER FITA
    LineBreakClass::ULB_AL, // 0x0474  # CYRILLIC CAPITAL LETTER IZHITSA
    LineBreakClass::ULB_AL, // 0x0475  # CYRILLIC SMALL LETTER IZHITSA
    LineBreakClass::ULB_AL, // 0x0476  # CYRILLIC CAPITAL LETTER IZHITSA WITH DOUBLE GRAVE ACCENT
    LineBreakClass::ULB_AL, // 0x0477  # CYRILLIC SMALL LETTER IZHITSA WITH DOUBLE GRAVE ACCENT
    LineBreakClass::ULB_AL, // 0x0478  # CYRILLIC CAPITAL LETTER UK
    LineBreakClass::ULB_AL, // 0x0479  # CYRILLIC SMALL LETTER UK
    LineBreakClass::ULB_AL, // 0x047A  # CYRILLIC CAPITAL LETTER ROUND OMEGA
    LineBreakClass::ULB_AL, // 0x047B  # CYRILLIC SMALL LETTER ROUND OMEGA
    LineBreakClass::ULB_AL, // 0x047C  # CYRILLIC CAPITAL LETTER OMEGA WITH TITLO
    LineBreakClass::ULB_AL, // 0x047D  # CYRILLIC SMALL LETTER OMEGA WITH TITLO
    LineBreakClass::ULB_AL, // 0x047E  # CYRILLIC CAPITAL LETTER OT
    LineBreakClass::ULB_AL, // 0x047F  # CYRILLIC SMALL LETTER OT
    LineBreakClass::ULB_AL, // 0x0480  # CYRILLIC CAPITAL LETTER KOPPA
    LineBreakClass::ULB_AL, // 0x0481  # CYRILLIC SMALL LETTER KOPPA
    LineBreakClass::ULB_AL, // 0x0482  # CYRILLIC THOUSANDS SIGN
    LineBreakClass::ULB_CM, // 0x0483  # COMBINING CYRILLIC TITLO
    LineBreakClass::ULB_CM, // 0x0484  # COMBINING CYRILLIC PALATALIZATION
    LineBreakClass::ULB_CM, // 0x0485  # COMBINING CYRILLIC DASIA PNEUMATA
    LineBreakClass::ULB_CM, // 0x0486  # COMBINING CYRILLIC PSILI PNEUMATA
    LineBreakClass::ULB_ID, // 0x0487 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0488  # COMBINING CYRILLIC HUNDRED THOUSANDS SIGN
    LineBreakClass::ULB_CM, // 0x0489  # COMBINING CYRILLIC MILLIONS SIGN
    LineBreakClass::ULB_AL, // 0x048A  # CYRILLIC CAPITAL LETTER SHORT I WITH TAIL
    LineBreakClass::ULB_AL, // 0x048B  # CYRILLIC SMALL LETTER SHORT I WITH TAIL
    LineBreakClass::ULB_AL, // 0x048C  # CYRILLIC CAPITAL LETTER SEMISOFT SIGN
    LineBreakClass::ULB_AL, // 0x048D  # CYRILLIC SMALL LETTER SEMISOFT SIGN
    LineBreakClass::ULB_AL, // 0x048E  # CYRILLIC CAPITAL LETTER ER WITH TICK
    LineBreakClass::ULB_AL, // 0x048F  # CYRILLIC SMALL LETTER ER WITH TICK
    LineBreakClass::ULB_AL, // 0x0490  # CYRILLIC CAPITAL LETTER GHE WITH UPTURN
    LineBreakClass::ULB_AL, // 0x0491  # CYRILLIC SMALL LETTER GHE WITH UPTURN
    LineBreakClass::ULB_AL, // 0x0492  # CYRILLIC CAPITAL LETTER GHE WITH STROKE
    LineBreakClass::ULB_AL, // 0x0493  # CYRILLIC SMALL LETTER GHE WITH STROKE
    LineBreakClass::ULB_AL, // 0x0494  # CYRILLIC CAPITAL LETTER GHE WITH MIDDLE HOOK
    LineBreakClass::ULB_AL, // 0x0495  # CYRILLIC SMALL LETTER GHE WITH MIDDLE HOOK
    LineBreakClass::ULB_AL, // 0x0496  # CYRILLIC CAPITAL LETTER ZHE WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x0497  # CYRILLIC SMALL LETTER ZHE WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x0498  # CYRILLIC CAPITAL LETTER ZE WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x0499  # CYRILLIC SMALL LETTER ZE WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x049A  # CYRILLIC CAPITAL LETTER KA WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x049B  # CYRILLIC SMALL LETTER KA WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x049C  # CYRILLIC CAPITAL LETTER KA WITH VERTICAL STROKE
    LineBreakClass::ULB_AL, // 0x049D  # CYRILLIC SMALL LETTER KA WITH VERTICAL STROKE
    LineBreakClass::ULB_AL, // 0x049E  # CYRILLIC CAPITAL LETTER KA WITH STROKE
    LineBreakClass::ULB_AL, // 0x049F  # CYRILLIC SMALL LETTER KA WITH STROKE
    LineBreakClass::ULB_AL, // 0x04A0  # CYRILLIC CAPITAL LETTER BASHKIR KA
    LineBreakClass::ULB_AL, // 0x04A1  # CYRILLIC SMALL LETTER BASHKIR KA
    LineBreakClass::ULB_AL, // 0x04A2  # CYRILLIC CAPITAL LETTER EN WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x04A3  # CYRILLIC SMALL LETTER EN WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x04A4  # CYRILLIC CAPITAL LIGATURE EN GHE
    LineBreakClass::ULB_AL, // 0x04A5  # CYRILLIC SMALL LIGATURE EN GHE
    LineBreakClass::ULB_AL, // 0x04A6  # CYRILLIC CAPITAL LETTER PE WITH MIDDLE HOOK
    LineBreakClass::ULB_AL, // 0x04A7  # CYRILLIC SMALL LETTER PE WITH MIDDLE HOOK
    LineBreakClass::ULB_AL, // 0x04A8  # CYRILLIC CAPITAL LETTER ABKHASIAN HA
    LineBreakClass::ULB_AL, // 0x04A9  # CYRILLIC SMALL LETTER ABKHASIAN HA
    LineBreakClass::ULB_AL, // 0x04AA  # CYRILLIC CAPITAL LETTER ES WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x04AB  # CYRILLIC SMALL LETTER ES WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x04AC  # CYRILLIC CAPITAL LETTER TE WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x04AD  # CYRILLIC SMALL LETTER TE WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x04AE  # CYRILLIC CAPITAL LETTER STRAIGHT U
    LineBreakClass::ULB_AL, // 0x04AF  # CYRILLIC SMALL LETTER STRAIGHT U
    LineBreakClass::ULB_AL, // 0x04B0  # CYRILLIC CAPITAL LETTER STRAIGHT U WITH STROKE
    LineBreakClass::ULB_AL, // 0x04B1  # CYRILLIC SMALL LETTER STRAIGHT U WITH STROKE
    LineBreakClass::ULB_AL, // 0x04B2  # CYRILLIC CAPITAL LETTER HA WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x04B3  # CYRILLIC SMALL LETTER HA WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x04B4  # CYRILLIC CAPITAL LIGATURE TE TSE
    LineBreakClass::ULB_AL, // 0x04B5  # CYRILLIC SMALL LIGATURE TE TSE
    LineBreakClass::ULB_AL, // 0x04B6  # CYRILLIC CAPITAL LETTER CHE WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x04B7  # CYRILLIC SMALL LETTER CHE WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x04B8  # CYRILLIC CAPITAL LETTER CHE WITH VERTICAL STROKE
    LineBreakClass::ULB_AL, // 0x04B9  # CYRILLIC SMALL LETTER CHE WITH VERTICAL STROKE
    LineBreakClass::ULB_AL, // 0x04BA  # CYRILLIC CAPITAL LETTER SHHA
    LineBreakClass::ULB_AL, // 0x04BB  # CYRILLIC SMALL LETTER SHHA
    LineBreakClass::ULB_AL, // 0x04BC  # CYRILLIC CAPITAL LETTER ABKHASIAN CHE
    LineBreakClass::ULB_AL, // 0x04BD  # CYRILLIC SMALL LETTER ABKHASIAN CHE
    LineBreakClass::ULB_AL, // 0x04BE  # CYRILLIC CAPITAL LETTER ABKHASIAN CHE WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x04BF  # CYRILLIC SMALL LETTER ABKHASIAN CHE WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x04C0  # CYRILLIC LETTER PALOCHKA
    LineBreakClass::ULB_AL, // 0x04C1  # CYRILLIC CAPITAL LETTER ZHE WITH BREVE
    LineBreakClass::ULB_AL, // 0x04C2  # CYRILLIC SMALL LETTER ZHE WITH BREVE
    LineBreakClass::ULB_AL, // 0x04C3  # CYRILLIC CAPITAL LETTER KA WITH HOOK
    LineBreakClass::ULB_AL, // 0x04C4  # CYRILLIC SMALL LETTER KA WITH HOOK
    LineBreakClass::ULB_AL, // 0x04C5  # CYRILLIC CAPITAL LETTER EL WITH TAIL
    LineBreakClass::ULB_AL, // 0x04C6  # CYRILLIC SMALL LETTER EL WITH TAIL
    LineBreakClass::ULB_AL, // 0x04C7  # CYRILLIC CAPITAL LETTER EN WITH HOOK
    LineBreakClass::ULB_AL, // 0x04C8  # CYRILLIC SMALL LETTER EN WITH HOOK
    LineBreakClass::ULB_AL, // 0x04C9  # CYRILLIC CAPITAL LETTER EN WITH TAIL
    LineBreakClass::ULB_AL, // 0x04CA  # CYRILLIC SMALL LETTER EN WITH TAIL
    LineBreakClass::ULB_AL, // 0x04CB  # CYRILLIC CAPITAL LETTER KHAKASSIAN CHE
    LineBreakClass::ULB_AL, // 0x04CC  # CYRILLIC SMALL LETTER KHAKASSIAN CHE
    LineBreakClass::ULB_AL, // 0x04CD  # CYRILLIC CAPITAL LETTER EM WITH TAIL
    LineBreakClass::ULB_AL, // 0x04CE  # CYRILLIC SMALL LETTER EM WITH TAIL
    LineBreakClass::ULB_AL, // 0x04CF  # CYRILLIC SMALL LETTER PALOCHKA
    LineBreakClass::ULB_AL, // 0x04D0  # CYRILLIC CAPITAL LETTER A WITH BREVE
    LineBreakClass::ULB_AL, // 0x04D1  # CYRILLIC SMALL LETTER A WITH BREVE
    LineBreakClass::ULB_AL, // 0x04D2  # CYRILLIC CAPITAL LETTER A WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x04D3  # CYRILLIC SMALL LETTER A WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x04D4  # CYRILLIC CAPITAL LIGATURE A IE
    LineBreakClass::ULB_AL, // 0x04D5  # CYRILLIC SMALL LIGATURE A IE
    LineBreakClass::ULB_AL, // 0x04D6  # CYRILLIC CAPITAL LETTER IE WITH BREVE
    LineBreakClass::ULB_AL, // 0x04D7  # CYRILLIC SMALL LETTER IE WITH BREVE
    LineBreakClass::ULB_AL, // 0x04D8  # CYRILLIC CAPITAL LETTER SCHWA
    LineBreakClass::ULB_AL, // 0x04D9  # CYRILLIC SMALL LETTER SCHWA
    LineBreakClass::ULB_AL, // 0x04DA  # CYRILLIC CAPITAL LETTER SCHWA WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x04DB  # CYRILLIC SMALL LETTER SCHWA WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x04DC  # CYRILLIC CAPITAL LETTER ZHE WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x04DD  # CYRILLIC SMALL LETTER ZHE WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x04DE  # CYRILLIC CAPITAL LETTER ZE WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x04DF  # CYRILLIC SMALL LETTER ZE WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x04E0  # CYRILLIC CAPITAL LETTER ABKHASIAN DZE
    LineBreakClass::ULB_AL, // 0x04E1  # CYRILLIC SMALL LETTER ABKHASIAN DZE
    LineBreakClass::ULB_AL, // 0x04E2  # CYRILLIC CAPITAL LETTER I WITH MACRON
    LineBreakClass::ULB_AL, // 0x04E3  # CYRILLIC SMALL LETTER I WITH MACRON
    LineBreakClass::ULB_AL, // 0x04E4  # CYRILLIC CAPITAL LETTER I WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x04E5  # CYRILLIC SMALL LETTER I WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x04E6  # CYRILLIC CAPITAL LETTER O WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x04E7  # CYRILLIC SMALL LETTER O WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x04E8  # CYRILLIC CAPITAL LETTER BARRED O
    LineBreakClass::ULB_AL, // 0x04E9  # CYRILLIC SMALL LETTER BARRED O
    LineBreakClass::ULB_AL, // 0x04EA  # CYRILLIC CAPITAL LETTER BARRED O WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x04EB  # CYRILLIC SMALL LETTER BARRED O WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x04EC  # CYRILLIC CAPITAL LETTER E WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x04ED  # CYRILLIC SMALL LETTER E WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x04EE  # CYRILLIC CAPITAL LETTER U WITH MACRON
    LineBreakClass::ULB_AL, // 0x04EF  # CYRILLIC SMALL LETTER U WITH MACRON
    LineBreakClass::ULB_AL, // 0x04F0  # CYRILLIC CAPITAL LETTER U WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x04F1  # CYRILLIC SMALL LETTER U WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x04F2  # CYRILLIC CAPITAL LETTER U WITH DOUBLE ACUTE
    LineBreakClass::ULB_AL, // 0x04F3  # CYRILLIC SMALL LETTER U WITH DOUBLE ACUTE
    LineBreakClass::ULB_AL, // 0x04F4  # CYRILLIC CAPITAL LETTER CHE WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x04F5  # CYRILLIC SMALL LETTER CHE WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x04F6  # CYRILLIC CAPITAL LETTER GHE WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x04F7  # CYRILLIC SMALL LETTER GHE WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x04F8  # CYRILLIC CAPITAL LETTER YERU WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x04F9  # CYRILLIC SMALL LETTER YERU WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x04FA  # CYRILLIC CAPITAL LETTER GHE WITH STROKE AND HOOK
    LineBreakClass::ULB_AL, // 0x04FB  # CYRILLIC SMALL LETTER GHE WITH STROKE AND HOOK
    LineBreakClass::ULB_AL, // 0x04FC  # CYRILLIC CAPITAL LETTER HA WITH HOOK
    LineBreakClass::ULB_AL, // 0x04FD  # CYRILLIC SMALL LETTER HA WITH HOOK
    LineBreakClass::ULB_AL, // 0x04FE  # CYRILLIC CAPITAL LETTER HA WITH STROKE
    LineBreakClass::ULB_AL, // 0x04FF  # CYRILLIC SMALL LETTER HA WITH STROKE
    LineBreakClass::ULB_AL, // 0x0500  # CYRILLIC CAPITAL LETTER KOMI DE
    LineBreakClass::ULB_AL, // 0x0501  # CYRILLIC SMALL LETTER KOMI DE
    LineBreakClass::ULB_AL, // 0x0502  # CYRILLIC CAPITAL LETTER KOMI DJE
    LineBreakClass::ULB_AL, // 0x0503  # CYRILLIC SMALL LETTER KOMI DJE
    LineBreakClass::ULB_AL, // 0x0504  # CYRILLIC CAPITAL LETTER KOMI ZJE
    LineBreakClass::ULB_AL, // 0x0505  # CYRILLIC SMALL LETTER KOMI ZJE
    LineBreakClass::ULB_AL, // 0x0506  # CYRILLIC CAPITAL LETTER KOMI DZJE
    LineBreakClass::ULB_AL, // 0x0507  # CYRILLIC SMALL LETTER KOMI DZJE
    LineBreakClass::ULB_AL, // 0x0508  # CYRILLIC CAPITAL LETTER KOMI LJE
    LineBreakClass::ULB_AL, // 0x0509  # CYRILLIC SMALL LETTER KOMI LJE
    LineBreakClass::ULB_AL, // 0x050A  # CYRILLIC CAPITAL LETTER KOMI NJE
    LineBreakClass::ULB_AL, // 0x050B  # CYRILLIC SMALL LETTER KOMI NJE
    LineBreakClass::ULB_AL, // 0x050C  # CYRILLIC CAPITAL LETTER KOMI SJE
    LineBreakClass::ULB_AL, // 0x050D  # CYRILLIC SMALL LETTER KOMI SJE
    LineBreakClass::ULB_AL, // 0x050E  # CYRILLIC CAPITAL LETTER KOMI TJE
    LineBreakClass::ULB_AL, // 0x050F  # CYRILLIC SMALL LETTER KOMI TJE
    LineBreakClass::ULB_AL, // 0x0510  # CYRILLIC CAPITAL LETTER REVERSED ZE
    LineBreakClass::ULB_AL, // 0x0511  # CYRILLIC SMALL LETTER REVERSED ZE
    LineBreakClass::ULB_AL, // 0x0512  # CYRILLIC CAPITAL LETTER EL WITH HOOK
    LineBreakClass::ULB_AL, // 0x0513  # CYRILLIC SMALL LETTER EL WITH HOOK
    LineBreakClass::ULB_ID, // 0x0514 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0515 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0516 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0517 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0518 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0519 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x051A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x051B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x051C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x051D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x051E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x051F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0520 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0521 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0522 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0523 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0524 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0525 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0526 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0527 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0528 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0529 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x052A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x052B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x052C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x052D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x052E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x052F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0530 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0531  # ARMENIAN CAPITAL LETTER AYB
    LineBreakClass::ULB_AL, // 0x0532  # ARMENIAN CAPITAL LETTER BEN
    LineBreakClass::ULB_AL, // 0x0533  # ARMENIAN CAPITAL LETTER GIM
    LineBreakClass::ULB_AL, // 0x0534  # ARMENIAN CAPITAL LETTER DA
    LineBreakClass::ULB_AL, // 0x0535  # ARMENIAN CAPITAL LETTER ECH
    LineBreakClass::ULB_AL, // 0x0536  # ARMENIAN CAPITAL LETTER ZA
    LineBreakClass::ULB_AL, // 0x0537  # ARMENIAN CAPITAL LETTER EH
    LineBreakClass::ULB_AL, // 0x0538  # ARMENIAN CAPITAL LETTER ET
    LineBreakClass::ULB_AL, // 0x0539  # ARMENIAN CAPITAL LETTER TO
    LineBreakClass::ULB_AL, // 0x053A  # ARMENIAN CAPITAL LETTER ZHE
    LineBreakClass::ULB_AL, // 0x053B  # ARMENIAN CAPITAL LETTER INI
    LineBreakClass::ULB_AL, // 0x053C  # ARMENIAN CAPITAL LETTER LIWN
    LineBreakClass::ULB_AL, // 0x053D  # ARMENIAN CAPITAL LETTER XEH
    LineBreakClass::ULB_AL, // 0x053E  # ARMENIAN CAPITAL LETTER CA
    LineBreakClass::ULB_AL, // 0x053F  # ARMENIAN CAPITAL LETTER KEN
    LineBreakClass::ULB_AL, // 0x0540  # ARMENIAN CAPITAL LETTER HO
    LineBreakClass::ULB_AL, // 0x0541  # ARMENIAN CAPITAL LETTER JA
    LineBreakClass::ULB_AL, // 0x0542  # ARMENIAN CAPITAL LETTER GHAD
    LineBreakClass::ULB_AL, // 0x0543  # ARMENIAN CAPITAL LETTER CHEH
    LineBreakClass::ULB_AL, // 0x0544  # ARMENIAN CAPITAL LETTER MEN
    LineBreakClass::ULB_AL, // 0x0545  # ARMENIAN CAPITAL LETTER YI
    LineBreakClass::ULB_AL, // 0x0546  # ARMENIAN CAPITAL LETTER NOW
    LineBreakClass::ULB_AL, // 0x0547  # ARMENIAN CAPITAL LETTER SHA
    LineBreakClass::ULB_AL, // 0x0548  # ARMENIAN CAPITAL LETTER VO
    LineBreakClass::ULB_AL, // 0x0549  # ARMENIAN CAPITAL LETTER CHA
    LineBreakClass::ULB_AL, // 0x054A  # ARMENIAN CAPITAL LETTER PEH
    LineBreakClass::ULB_AL, // 0x054B  # ARMENIAN CAPITAL LETTER JHEH
    LineBreakClass::ULB_AL, // 0x054C  # ARMENIAN CAPITAL LETTER RA
    LineBreakClass::ULB_AL, // 0x054D  # ARMENIAN CAPITAL LETTER SEH
    LineBreakClass::ULB_AL, // 0x054E  # ARMENIAN CAPITAL LETTER VEW
    LineBreakClass::ULB_AL, // 0x054F  # ARMENIAN CAPITAL LETTER TIWN
    LineBreakClass::ULB_AL, // 0x0550  # ARMENIAN CAPITAL LETTER REH
    LineBreakClass::ULB_AL, // 0x0551  # ARMENIAN CAPITAL LETTER CO
    LineBreakClass::ULB_AL, // 0x0552  # ARMENIAN CAPITAL LETTER YIWN
    LineBreakClass::ULB_AL, // 0x0553  # ARMENIAN CAPITAL LETTER PIWR
    LineBreakClass::ULB_AL, // 0x0554  # ARMENIAN CAPITAL LETTER KEH
    LineBreakClass::ULB_AL, // 0x0555  # ARMENIAN CAPITAL LETTER OH
    LineBreakClass::ULB_AL, // 0x0556  # ARMENIAN CAPITAL LETTER FEH
    LineBreakClass::ULB_ID, // 0x0557 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0558 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0559  # ARMENIAN MODIFIER LETTER LEFT HALF RING
    LineBreakClass::ULB_AL, // 0x055A  # ARMENIAN APOSTROPHE
    LineBreakClass::ULB_AL, // 0x055B  # ARMENIAN EMPHASIS MARK
    LineBreakClass::ULB_AL, // 0x055C  # ARMENIAN EXCLAMATION MARK
    LineBreakClass::ULB_AL, // 0x055D  # ARMENIAN COMMA
    LineBreakClass::ULB_AL, // 0x055E  # ARMENIAN QUESTION MARK
    LineBreakClass::ULB_AL, // 0x055F  # ARMENIAN ABBREVIATION MARK
    LineBreakClass::ULB_ID, // 0x0560 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0561  # ARMENIAN SMALL LETTER AYB
    LineBreakClass::ULB_AL, // 0x0562  # ARMENIAN SMALL LETTER BEN
    LineBreakClass::ULB_AL, // 0x0563  # ARMENIAN SMALL LETTER GIM
    LineBreakClass::ULB_AL, // 0x0564  # ARMENIAN SMALL LETTER DA
    LineBreakClass::ULB_AL, // 0x0565  # ARMENIAN SMALL LETTER ECH
    LineBreakClass::ULB_AL, // 0x0566  # ARMENIAN SMALL LETTER ZA
    LineBreakClass::ULB_AL, // 0x0567  # ARMENIAN SMALL LETTER EH
    LineBreakClass::ULB_AL, // 0x0568  # ARMENIAN SMALL LETTER ET
    LineBreakClass::ULB_AL, // 0x0569  # ARMENIAN SMALL LETTER TO
    LineBreakClass::ULB_AL, // 0x056A  # ARMENIAN SMALL LETTER ZHE
    LineBreakClass::ULB_AL, // 0x056B  # ARMENIAN SMALL LETTER INI
    LineBreakClass::ULB_AL, // 0x056C  # ARMENIAN SMALL LETTER LIWN
    LineBreakClass::ULB_AL, // 0x056D  # ARMENIAN SMALL LETTER XEH
    LineBreakClass::ULB_AL, // 0x056E  # ARMENIAN SMALL LETTER CA
    LineBreakClass::ULB_AL, // 0x056F  # ARMENIAN SMALL LETTER KEN
    LineBreakClass::ULB_AL, // 0x0570  # ARMENIAN SMALL LETTER HO
    LineBreakClass::ULB_AL, // 0x0571  # ARMENIAN SMALL LETTER JA
    LineBreakClass::ULB_AL, // 0x0572  # ARMENIAN SMALL LETTER GHAD
    LineBreakClass::ULB_AL, // 0x0573  # ARMENIAN SMALL LETTER CHEH
    LineBreakClass::ULB_AL, // 0x0574  # ARMENIAN SMALL LETTER MEN
    LineBreakClass::ULB_AL, // 0x0575  # ARMENIAN SMALL LETTER YI
    LineBreakClass::ULB_AL, // 0x0576  # ARMENIAN SMALL LETTER NOW
    LineBreakClass::ULB_AL, // 0x0577  # ARMENIAN SMALL LETTER SHA
    LineBreakClass::ULB_AL, // 0x0578  # ARMENIAN SMALL LETTER VO
    LineBreakClass::ULB_AL, // 0x0579  # ARMENIAN SMALL LETTER CHA
    LineBreakClass::ULB_AL, // 0x057A  # ARMENIAN SMALL LETTER PEH
    LineBreakClass::ULB_AL, // 0x057B  # ARMENIAN SMALL LETTER JHEH
    LineBreakClass::ULB_AL, // 0x057C  # ARMENIAN SMALL LETTER RA
    LineBreakClass::ULB_AL, // 0x057D  # ARMENIAN SMALL LETTER SEH
    LineBreakClass::ULB_AL, // 0x057E  # ARMENIAN SMALL LETTER VEW
    LineBreakClass::ULB_AL, // 0x057F  # ARMENIAN SMALL LETTER TIWN
    LineBreakClass::ULB_AL, // 0x0580  # ARMENIAN SMALL LETTER REH
    LineBreakClass::ULB_AL, // 0x0581  # ARMENIAN SMALL LETTER CO
    LineBreakClass::ULB_AL, // 0x0582  # ARMENIAN SMALL LETTER YIWN
    LineBreakClass::ULB_AL, // 0x0583  # ARMENIAN SMALL LETTER PIWR
    LineBreakClass::ULB_AL, // 0x0584  # ARMENIAN SMALL LETTER KEH
    LineBreakClass::ULB_AL, // 0x0585  # ARMENIAN SMALL LETTER OH
    LineBreakClass::ULB_AL, // 0x0586  # ARMENIAN SMALL LETTER FEH
    LineBreakClass::ULB_AL, // 0x0587  # ARMENIAN SMALL LIGATURE ECH YIWN
    LineBreakClass::ULB_ID, // 0x0588 # <UNDEFINED>
    LineBreakClass::ULB_IS, // 0x0589  # ARMENIAN FULL STOP
    LineBreakClass::ULB_BA, // 0x058A  # ARMENIAN HYPHEN
    LineBreakClass::ULB_ID, // 0x058B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x058C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x058D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x058E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x058F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0590 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0591  # HEBREW ACCENT ETNAHTA
    LineBreakClass::ULB_CM, // 0x0592  # HEBREW ACCENT SEGOL
    LineBreakClass::ULB_CM, // 0x0593  # HEBREW ACCENT SHALSHELET
    LineBreakClass::ULB_CM, // 0x0594  # HEBREW ACCENT ZAQEF QATAN
    LineBreakClass::ULB_CM, // 0x0595  # HEBREW ACCENT ZAQEF GADOL
    LineBreakClass::ULB_CM, // 0x0596  # HEBREW ACCENT TIPEHA
    LineBreakClass::ULB_CM, // 0x0597  # HEBREW ACCENT REVIA
    LineBreakClass::ULB_CM, // 0x0598  # HEBREW ACCENT ZARQA
    LineBreakClass::ULB_CM, // 0x0599  # HEBREW ACCENT PASHTA
    LineBreakClass::ULB_CM, // 0x059A  # HEBREW ACCENT YETIV
    LineBreakClass::ULB_CM, // 0x059B  # HEBREW ACCENT TEVIR
    LineBreakClass::ULB_CM, // 0x059C  # HEBREW ACCENT GERESH
    LineBreakClass::ULB_CM, // 0x059D  # HEBREW ACCENT GERESH MUQDAM
    LineBreakClass::ULB_CM, // 0x059E  # HEBREW ACCENT GERSHAYIM
    LineBreakClass::ULB_CM, // 0x059F  # HEBREW ACCENT QARNEY PARA
    LineBreakClass::ULB_CM, // 0x05A0  # HEBREW ACCENT TELISHA GEDOLA
    LineBreakClass::ULB_CM, // 0x05A1  # HEBREW ACCENT PAZER
    LineBreakClass::ULB_CM, // 0x05A2  # HEBREW ACCENT ATNAH HAFUKH
    LineBreakClass::ULB_CM, // 0x05A3  # HEBREW ACCENT MUNAH
    LineBreakClass::ULB_CM, // 0x05A4  # HEBREW ACCENT MAHAPAKH
    LineBreakClass::ULB_CM, // 0x05A5  # HEBREW ACCENT MERKHA
    LineBreakClass::ULB_CM, // 0x05A6  # HEBREW ACCENT MERKHA KEFULA
    LineBreakClass::ULB_CM, // 0x05A7  # HEBREW ACCENT DARGA
    LineBreakClass::ULB_CM, // 0x05A8  # HEBREW ACCENT QADMA
    LineBreakClass::ULB_CM, // 0x05A9  # HEBREW ACCENT TELISHA QETANA
    LineBreakClass::ULB_CM, // 0x05AA  # HEBREW ACCENT YERAH BEN YOMO
    LineBreakClass::ULB_CM, // 0x05AB  # HEBREW ACCENT OLE
    LineBreakClass::ULB_CM, // 0x05AC  # HEBREW ACCENT ILUY
    LineBreakClass::ULB_CM, // 0x05AD  # HEBREW ACCENT DEHI
    LineBreakClass::ULB_CM, // 0x05AE  # HEBREW ACCENT ZINOR
    LineBreakClass::ULB_CM, // 0x05AF  # HEBREW MARK MASORA CIRCLE
    LineBreakClass::ULB_CM, // 0x05B0  # HEBREW POINT SHEVA
    LineBreakClass::ULB_CM, // 0x05B1  # HEBREW POINT HATAF SEGOL
    LineBreakClass::ULB_CM, // 0x05B2  # HEBREW POINT HATAF PATAH
    LineBreakClass::ULB_CM, // 0x05B3  # HEBREW POINT HATAF QAMATS
    LineBreakClass::ULB_CM, // 0x05B4  # HEBREW POINT HIRIQ
    LineBreakClass::ULB_CM, // 0x05B5  # HEBREW POINT TSERE
    LineBreakClass::ULB_CM, // 0x05B6  # HEBREW POINT SEGOL
    LineBreakClass::ULB_CM, // 0x05B7  # HEBREW POINT PATAH
    LineBreakClass::ULB_CM, // 0x05B8  # HEBREW POINT QAMATS
    LineBreakClass::ULB_CM, // 0x05B9  # HEBREW POINT HOLAM
    LineBreakClass::ULB_CM, // 0x05BA  # HEBREW POINT HOLAM HASER FOR VAV
    LineBreakClass::ULB_CM, // 0x05BB  # HEBREW POINT QUBUTS
    LineBreakClass::ULB_CM, // 0x05BC  # HEBREW POINT DAGESH OR MAPIQ
    LineBreakClass::ULB_CM, // 0x05BD  # HEBREW POINT METEG
    LineBreakClass::ULB_BA, // 0x05BE  # HEBREW PUNCTUATION MAQAF
    LineBreakClass::ULB_CM, // 0x05BF  # HEBREW POINT RAFE
    LineBreakClass::ULB_AL, // 0x05C0  # HEBREW PUNCTUATION PASEQ
    LineBreakClass::ULB_CM, // 0x05C1  # HEBREW POINT SHIN DOT
    LineBreakClass::ULB_CM, // 0x05C2  # HEBREW POINT SIN DOT
    LineBreakClass::ULB_AL, // 0x05C3  # HEBREW PUNCTUATION SOF PASUQ
    LineBreakClass::ULB_CM, // 0x05C4  # HEBREW MARK UPPER DOT
    LineBreakClass::ULB_CM, // 0x05C5  # HEBREW MARK LOWER DOT
    LineBreakClass::ULB_EX, // 0x05C6  # HEBREW PUNCTUATION NUN HAFUKHA
    LineBreakClass::ULB_CM, // 0x05C7  # HEBREW POINT QAMATS QATAN
    LineBreakClass::ULB_ID, // 0x05C8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x05C9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x05CA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x05CB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x05CC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x05CD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x05CE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x05CF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x05D0  # HEBREW LETTER ALEF
    LineBreakClass::ULB_AL, // 0x05D1  # HEBREW LETTER BET
    LineBreakClass::ULB_AL, // 0x05D2  # HEBREW LETTER GIMEL
    LineBreakClass::ULB_AL, // 0x05D3  # HEBREW LETTER DALET
    LineBreakClass::ULB_AL, // 0x05D4  # HEBREW LETTER HE
    LineBreakClass::ULB_AL, // 0x05D5  # HEBREW LETTER VAV
    LineBreakClass::ULB_AL, // 0x05D6  # HEBREW LETTER ZAYIN
    LineBreakClass::ULB_AL, // 0x05D7  # HEBREW LETTER HET
    LineBreakClass::ULB_AL, // 0x05D8  # HEBREW LETTER TET
    LineBreakClass::ULB_AL, // 0x05D9  # HEBREW LETTER YOD
    LineBreakClass::ULB_AL, // 0x05DA  # HEBREW LETTER FINAL KAF
    LineBreakClass::ULB_AL, // 0x05DB  # HEBREW LETTER KAF
    LineBreakClass::ULB_AL, // 0x05DC  # HEBREW LETTER LAMED
    LineBreakClass::ULB_AL, // 0x05DD  # HEBREW LETTER FINAL MEM
    LineBreakClass::ULB_AL, // 0x05DE  # HEBREW LETTER MEM
    LineBreakClass::ULB_AL, // 0x05DF  # HEBREW LETTER FINAL NUN
    LineBreakClass::ULB_AL, // 0x05E0  # HEBREW LETTER NUN
    LineBreakClass::ULB_AL, // 0x05E1  # HEBREW LETTER SAMEKH
    LineBreakClass::ULB_AL, // 0x05E2  # HEBREW LETTER AYIN
    LineBreakClass::ULB_AL, // 0x05E3  # HEBREW LETTER FINAL PE
    LineBreakClass::ULB_AL, // 0x05E4  # HEBREW LETTER PE
    LineBreakClass::ULB_AL, // 0x05E5  # HEBREW LETTER FINAL TSADI
    LineBreakClass::ULB_AL, // 0x05E6  # HEBREW LETTER TSADI
    LineBreakClass::ULB_AL, // 0x05E7  # HEBREW LETTER QOF
    LineBreakClass::ULB_AL, // 0x05E8  # HEBREW LETTER RESH
    LineBreakClass::ULB_AL, // 0x05E9  # HEBREW LETTER SHIN
    LineBreakClass::ULB_AL, // 0x05EA  # HEBREW LETTER TAV
    LineBreakClass::ULB_ID, // 0x05EB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x05EC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x05ED # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x05EE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x05EF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x05F0  # HEBREW LIGATURE YIDDISH DOUBLE VAV
    LineBreakClass::ULB_AL, // 0x05F1  # HEBREW LIGATURE YIDDISH VAV YOD
    LineBreakClass::ULB_AL, // 0x05F2  # HEBREW LIGATURE YIDDISH DOUBLE YOD
    LineBreakClass::ULB_AL, // 0x05F3  # HEBREW PUNCTUATION GERESH
    LineBreakClass::ULB_AL, // 0x05F4  # HEBREW PUNCTUATION GERSHAYIM
    LineBreakClass::ULB_ID, // 0x05F5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x05F6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x05F7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x05F8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x05F9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x05FA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x05FB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x05FC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x05FD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x05FE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x05FF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0600  # ARABIC NUMBER SIGN
    LineBreakClass::ULB_AL, // 0x0601  # ARABIC SIGN SANAH
    LineBreakClass::ULB_AL, // 0x0602  # ARABIC FOOTNOTE MARKER
    LineBreakClass::ULB_AL, // 0x0603  # ARABIC SIGN SAFHA
    LineBreakClass::ULB_ID, // 0x0604 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0605 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0606 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0607 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0608 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0609 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x060A # <UNDEFINED>
    LineBreakClass::ULB_PO, // 0x060B  # AFGHANI SIGN
    LineBreakClass::ULB_EX, // 0x060C  # ARABIC COMMA
    LineBreakClass::ULB_IS, // 0x060D  # ARABIC DATE SEPARATOR
    LineBreakClass::ULB_AL, // 0x060E  # ARABIC POETIC VERSE SIGN
    LineBreakClass::ULB_AL, // 0x060F  # ARABIC SIGN MISRA
    LineBreakClass::ULB_CM, // 0x0610  # ARABIC SIGN SALLALLAHOU ALAYHE WASSALLAM
    LineBreakClass::ULB_CM, // 0x0611  # ARABIC SIGN ALAYHE ASSALLAM
    LineBreakClass::ULB_CM, // 0x0612  # ARABIC SIGN RAHMATULLAH ALAYHE
    LineBreakClass::ULB_CM, // 0x0613  # ARABIC SIGN RADI ALLAHOU ANHU
    LineBreakClass::ULB_CM, // 0x0614  # ARABIC SIGN TAKHALLUS
    LineBreakClass::ULB_CM, // 0x0615  # ARABIC SMALL HIGH TAH
    LineBreakClass::ULB_ID, // 0x0616 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0617 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0618 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0619 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x061A # <UNDEFINED>
    LineBreakClass::ULB_EX, // 0x061B  # ARABIC SEMICOLON
    LineBreakClass::ULB_ID, // 0x061C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x061D # <UNDEFINED>
    LineBreakClass::ULB_EX, // 0x061E  # ARABIC TRIPLE DOT PUNCTUATION MARK
    LineBreakClass::ULB_EX, // 0x061F  # ARABIC QUESTION MARK
    LineBreakClass::ULB_ID, // 0x0620 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0621  # ARABIC LETTER HAMZA
    LineBreakClass::ULB_AL, // 0x0622  # ARABIC LETTER ALEF WITH MADDA ABOVE
    LineBreakClass::ULB_AL, // 0x0623  # ARABIC LETTER ALEF WITH HAMZA ABOVE
    LineBreakClass::ULB_AL, // 0x0624  # ARABIC LETTER WAW WITH HAMZA ABOVE
    LineBreakClass::ULB_AL, // 0x0625  # ARABIC LETTER ALEF WITH HAMZA BELOW
    LineBreakClass::ULB_AL, // 0x0626  # ARABIC LETTER YEH WITH HAMZA ABOVE
    LineBreakClass::ULB_AL, // 0x0627  # ARABIC LETTER ALEF
    LineBreakClass::ULB_AL, // 0x0628  # ARABIC LETTER BEH
    LineBreakClass::ULB_AL, // 0x0629  # ARABIC LETTER TEH MARBUTA
    LineBreakClass::ULB_AL, // 0x062A  # ARABIC LETTER TEH
    LineBreakClass::ULB_AL, // 0x062B  # ARABIC LETTER THEH
    LineBreakClass::ULB_AL, // 0x062C  # ARABIC LETTER JEEM
    LineBreakClass::ULB_AL, // 0x062D  # ARABIC LETTER HAH
    LineBreakClass::ULB_AL, // 0x062E  # ARABIC LETTER KHAH
    LineBreakClass::ULB_AL, // 0x062F  # ARABIC LETTER DAL
    LineBreakClass::ULB_AL, // 0x0630  # ARABIC LETTER THAL
    LineBreakClass::ULB_AL, // 0x0631  # ARABIC LETTER REH
    LineBreakClass::ULB_AL, // 0x0632  # ARABIC LETTER ZAIN
    LineBreakClass::ULB_AL, // 0x0633  # ARABIC LETTER SEEN
    LineBreakClass::ULB_AL, // 0x0634  # ARABIC LETTER SHEEN
    LineBreakClass::ULB_AL, // 0x0635  # ARABIC LETTER SAD
    LineBreakClass::ULB_AL, // 0x0636  # ARABIC LETTER DAD
    LineBreakClass::ULB_AL, // 0x0637  # ARABIC LETTER TAH
    LineBreakClass::ULB_AL, // 0x0638  # ARABIC LETTER ZAH
    LineBreakClass::ULB_AL, // 0x0639  # ARABIC LETTER AIN
    LineBreakClass::ULB_AL, // 0x063A  # ARABIC LETTER GHAIN
    LineBreakClass::ULB_ID, // 0x063B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x063C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x063D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x063E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x063F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0640  # ARABIC TATWEEL
    LineBreakClass::ULB_AL, // 0x0641  # ARABIC LETTER FEH
    LineBreakClass::ULB_AL, // 0x0642  # ARABIC LETTER QAF
    LineBreakClass::ULB_AL, // 0x0643  # ARABIC LETTER KAF
    LineBreakClass::ULB_AL, // 0x0644  # ARABIC LETTER LAM
    LineBreakClass::ULB_AL, // 0x0645  # ARABIC LETTER MEEM
    LineBreakClass::ULB_AL, // 0x0646  # ARABIC LETTER NOON
    LineBreakClass::ULB_AL, // 0x0647  # ARABIC LETTER HEH
    LineBreakClass::ULB_AL, // 0x0648  # ARABIC LETTER WAW
    LineBreakClass::ULB_AL, // 0x0649  # ARABIC LETTER ALEF MAKSURA
    LineBreakClass::ULB_AL, // 0x064A  # ARABIC LETTER YEH
    LineBreakClass::ULB_CM, // 0x064B  # ARABIC FATHATAN
    LineBreakClass::ULB_CM, // 0x064C  # ARABIC DAMMATAN
    LineBreakClass::ULB_CM, // 0x064D  # ARABIC KASRATAN
    LineBreakClass::ULB_CM, // 0x064E  # ARABIC FATHA
    LineBreakClass::ULB_CM, // 0x064F  # ARABIC DAMMA
    LineBreakClass::ULB_CM, // 0x0650  # ARABIC KASRA
    LineBreakClass::ULB_CM, // 0x0651  # ARABIC SHADDA
    LineBreakClass::ULB_CM, // 0x0652  # ARABIC SUKUN
    LineBreakClass::ULB_CM, // 0x0653  # ARABIC MADDAH ABOVE
    LineBreakClass::ULB_CM, // 0x0654  # ARABIC HAMZA ABOVE
    LineBreakClass::ULB_CM, // 0x0655  # ARABIC HAMZA BELOW
    LineBreakClass::ULB_CM, // 0x0656  # ARABIC SUBSCRIPT ALEF
    LineBreakClass::ULB_CM, // 0x0657  # ARABIC INVERTED DAMMA
    LineBreakClass::ULB_CM, // 0x0658  # ARABIC MARK NOON GHUNNA
    LineBreakClass::ULB_CM, // 0x0659  # ARABIC ZWARAKAY
    LineBreakClass::ULB_CM, // 0x065A  # ARABIC VOWEL SIGN SMALL V ABOVE
    LineBreakClass::ULB_CM, // 0x065B  # ARABIC VOWEL SIGN INVERTED SMALL V ABOVE
    LineBreakClass::ULB_CM, // 0x065C  # ARABIC VOWEL SIGN DOT BELOW
    LineBreakClass::ULB_CM, // 0x065D  # ARABIC REVERSED DAMMA
    LineBreakClass::ULB_CM, // 0x065E  # ARABIC FATHA WITH TWO DOTS
    LineBreakClass::ULB_ID, // 0x065F # <UNDEFINED>
    LineBreakClass::ULB_NU, // 0x0660  # ARABIC-INDIC DIGIT ZERO
    LineBreakClass::ULB_NU, // 0x0661  # ARABIC-INDIC DIGIT ONE
    LineBreakClass::ULB_NU, // 0x0662  # ARABIC-INDIC DIGIT TWO
    LineBreakClass::ULB_NU, // 0x0663  # ARABIC-INDIC DIGIT THREE
    LineBreakClass::ULB_NU, // 0x0664  # ARABIC-INDIC DIGIT FOUR
    LineBreakClass::ULB_NU, // 0x0665  # ARABIC-INDIC DIGIT FIVE
    LineBreakClass::ULB_NU, // 0x0666  # ARABIC-INDIC DIGIT SIX
    LineBreakClass::ULB_NU, // 0x0667  # ARABIC-INDIC DIGIT SEVEN
    LineBreakClass::ULB_NU, // 0x0668  # ARABIC-INDIC DIGIT EIGHT
    LineBreakClass::ULB_NU, // 0x0669  # ARABIC-INDIC DIGIT NINE
    LineBreakClass::ULB_EX, // 0x066A  # ARABIC PERCENT SIGN
    LineBreakClass::ULB_NU, // 0x066B  # ARABIC DECIMAL SEPARATOR
    LineBreakClass::ULB_NU, // 0x066C  # ARABIC THOUSANDS SEPARATOR
    LineBreakClass::ULB_AL, // 0x066D  # ARABIC FIVE POINTED STAR
    LineBreakClass::ULB_AL, // 0x066E  # ARABIC LETTER DOTLESS BEH
    LineBreakClass::ULB_AL, // 0x066F  # ARABIC LETTER DOTLESS QAF
    LineBreakClass::ULB_CM, // 0x0670  # ARABIC LETTER SUPERSCRIPT ALEF
    LineBreakClass::ULB_AL, // 0x0671  # ARABIC LETTER ALEF WASLA
    LineBreakClass::ULB_AL, // 0x0672  # ARABIC LETTER ALEF WITH WAVY HAMZA ABOVE
    LineBreakClass::ULB_AL, // 0x0673  # ARABIC LETTER ALEF WITH WAVY HAMZA BELOW
    LineBreakClass::ULB_AL, // 0x0674  # ARABIC LETTER HIGH HAMZA
    LineBreakClass::ULB_AL, // 0x0675  # ARABIC LETTER HIGH HAMZA ALEF
    LineBreakClass::ULB_AL, // 0x0676  # ARABIC LETTER HIGH HAMZA WAW
    LineBreakClass::ULB_AL, // 0x0677  # ARABIC LETTER U WITH HAMZA ABOVE
    LineBreakClass::ULB_AL, // 0x0678  # ARABIC LETTER HIGH HAMZA YEH
    LineBreakClass::ULB_AL, // 0x0679  # ARABIC LETTER TTEH
    LineBreakClass::ULB_AL, // 0x067A  # ARABIC LETTER TTEHEH
    LineBreakClass::ULB_AL, // 0x067B  # ARABIC LETTER BEEH
    LineBreakClass::ULB_AL, // 0x067C  # ARABIC LETTER TEH WITH RING
    LineBreakClass::ULB_AL, // 0x067D  # ARABIC LETTER TEH WITH THREE DOTS ABOVE DOWNWARDS
    LineBreakClass::ULB_AL, // 0x067E  # ARABIC LETTER PEH
    LineBreakClass::ULB_AL, // 0x067F  # ARABIC LETTER TEHEH
    LineBreakClass::ULB_AL, // 0x0680  # ARABIC LETTER BEHEH
    LineBreakClass::ULB_AL, // 0x0681  # ARABIC LETTER HAH WITH HAMZA ABOVE
    LineBreakClass::ULB_AL, // 0x0682  # ARABIC LETTER HAH WITH TWO DOTS VERTICAL ABOVE
    LineBreakClass::ULB_AL, // 0x0683  # ARABIC LETTER NYEH
    LineBreakClass::ULB_AL, // 0x0684  # ARABIC LETTER DYEH
    LineBreakClass::ULB_AL, // 0x0685  # ARABIC LETTER HAH WITH THREE DOTS ABOVE
    LineBreakClass::ULB_AL, // 0x0686  # ARABIC LETTER TCHEH
    LineBreakClass::ULB_AL, // 0x0687  # ARABIC LETTER TCHEHEH
    LineBreakClass::ULB_AL, // 0x0688  # ARABIC LETTER DDAL
    LineBreakClass::ULB_AL, // 0x0689  # ARABIC LETTER DAL WITH RING
    LineBreakClass::ULB_AL, // 0x068A  # ARABIC LETTER DAL WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x068B  # ARABIC LETTER DAL WITH DOT BELOW AND SMALL TAH
    LineBreakClass::ULB_AL, // 0x068C  # ARABIC LETTER DAHAL
    LineBreakClass::ULB_AL, // 0x068D  # ARABIC LETTER DDAHAL
    LineBreakClass::ULB_AL, // 0x068E  # ARABIC LETTER DUL
    LineBreakClass::ULB_AL, // 0x068F  # ARABIC LETTER DAL WITH THREE DOTS ABOVE DOWNWARDS
    LineBreakClass::ULB_AL, // 0x0690  # ARABIC LETTER DAL WITH FOUR DOTS ABOVE
    LineBreakClass::ULB_AL, // 0x0691  # ARABIC LETTER RREH
    LineBreakClass::ULB_AL, // 0x0692  # ARABIC LETTER REH WITH SMALL V
    LineBreakClass::ULB_AL, // 0x0693  # ARABIC LETTER REH WITH RING
    LineBreakClass::ULB_AL, // 0x0694  # ARABIC LETTER REH WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x0695  # ARABIC LETTER REH WITH SMALL V BELOW
    LineBreakClass::ULB_AL, // 0x0696  # ARABIC LETTER REH WITH DOT BELOW AND DOT ABOVE
    LineBreakClass::ULB_AL, // 0x0697  # ARABIC LETTER REH WITH TWO DOTS ABOVE
    LineBreakClass::ULB_AL, // 0x0698  # ARABIC LETTER JEH
    LineBreakClass::ULB_AL, // 0x0699  # ARABIC LETTER REH WITH FOUR DOTS ABOVE
    LineBreakClass::ULB_AL, // 0x069A  # ARABIC LETTER SEEN WITH DOT BELOW AND DOT ABOVE
    LineBreakClass::ULB_AL, // 0x069B  # ARABIC LETTER SEEN WITH THREE DOTS BELOW
    LineBreakClass::ULB_AL, // 0x069C  # ARABIC LETTER SEEN WITH THREE DOTS BELOW AND THREE DOTS
                            // ABOVE
    LineBreakClass::ULB_AL, // 0x069D  # ARABIC LETTER SAD WITH TWO DOTS BELOW
    LineBreakClass::ULB_AL, // 0x069E  # ARABIC LETTER SAD WITH THREE DOTS ABOVE
    LineBreakClass::ULB_AL, // 0x069F  # ARABIC LETTER TAH WITH THREE DOTS ABOVE
    LineBreakClass::ULB_AL, // 0x06A0  # ARABIC LETTER AIN WITH THREE DOTS ABOVE
    LineBreakClass::ULB_AL, // 0x06A1  # ARABIC LETTER DOTLESS FEH
    LineBreakClass::ULB_AL, // 0x06A2  # ARABIC LETTER FEH WITH DOT MOVED BELOW
    LineBreakClass::ULB_AL, // 0x06A3  # ARABIC LETTER FEH WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x06A4  # ARABIC LETTER VEH
    LineBreakClass::ULB_AL, // 0x06A5  # ARABIC LETTER FEH WITH THREE DOTS BELOW
    LineBreakClass::ULB_AL, // 0x06A6  # ARABIC LETTER PEHEH
    LineBreakClass::ULB_AL, // 0x06A7  # ARABIC LETTER QAF WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x06A8  # ARABIC LETTER QAF WITH THREE DOTS ABOVE
    LineBreakClass::ULB_AL, // 0x06A9  # ARABIC LETTER KEHEH
    LineBreakClass::ULB_AL, // 0x06AA  # ARABIC LETTER SWASH KAF
    LineBreakClass::ULB_AL, // 0x06AB  # ARABIC LETTER KAF WITH RING
    LineBreakClass::ULB_AL, // 0x06AC  # ARABIC LETTER KAF WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x06AD  # ARABIC LETTER NG
    LineBreakClass::ULB_AL, // 0x06AE  # ARABIC LETTER KAF WITH THREE DOTS BELOW
    LineBreakClass::ULB_AL, // 0x06AF  # ARABIC LETTER GAF
    LineBreakClass::ULB_AL, // 0x06B0  # ARABIC LETTER GAF WITH RING
    LineBreakClass::ULB_AL, // 0x06B1  # ARABIC LETTER NGOEH
    LineBreakClass::ULB_AL, // 0x06B2  # ARABIC LETTER GAF WITH TWO DOTS BELOW
    LineBreakClass::ULB_AL, // 0x06B3  # ARABIC LETTER GUEH
    LineBreakClass::ULB_AL, // 0x06B4  # ARABIC LETTER GAF WITH THREE DOTS ABOVE
    LineBreakClass::ULB_AL, // 0x06B5  # ARABIC LETTER LAM WITH SMALL V
    LineBreakClass::ULB_AL, // 0x06B6  # ARABIC LETTER LAM WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x06B7  # ARABIC LETTER LAM WITH THREE DOTS ABOVE
    LineBreakClass::ULB_AL, // 0x06B8  # ARABIC LETTER LAM WITH THREE DOTS BELOW
    LineBreakClass::ULB_AL, // 0x06B9  # ARABIC LETTER NOON WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x06BA  # ARABIC LETTER NOON GHUNNA
    LineBreakClass::ULB_AL, // 0x06BB  # ARABIC LETTER RNOON
    LineBreakClass::ULB_AL, // 0x06BC  # ARABIC LETTER NOON WITH RING
    LineBreakClass::ULB_AL, // 0x06BD  # ARABIC LETTER NOON WITH THREE DOTS ABOVE
    LineBreakClass::ULB_AL, // 0x06BE  # ARABIC LETTER HEH DOACHASHMEE
    LineBreakClass::ULB_AL, // 0x06BF  # ARABIC LETTER TCHEH WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x06C0  # ARABIC LETTER HEH WITH YEH ABOVE
    LineBreakClass::ULB_AL, // 0x06C1  # ARABIC LETTER HEH GOAL
    LineBreakClass::ULB_AL, // 0x06C2  # ARABIC LETTER HEH GOAL WITH HAMZA ABOVE
    LineBreakClass::ULB_AL, // 0x06C3  # ARABIC LETTER TEH MARBUTA GOAL
    LineBreakClass::ULB_AL, // 0x06C4  # ARABIC LETTER WAW WITH RING
    LineBreakClass::ULB_AL, // 0x06C5  # ARABIC LETTER KIRGHIZ OE
    LineBreakClass::ULB_AL, // 0x06C6  # ARABIC LETTER OE
    LineBreakClass::ULB_AL, // 0x06C7  # ARABIC LETTER U
    LineBreakClass::ULB_AL, // 0x06C8  # ARABIC LETTER YU
    LineBreakClass::ULB_AL, // 0x06C9  # ARABIC LETTER KIRGHIZ YU
    LineBreakClass::ULB_AL, // 0x06CA  # ARABIC LETTER WAW WITH TWO DOTS ABOVE
    LineBreakClass::ULB_AL, // 0x06CB  # ARABIC LETTER VE
    LineBreakClass::ULB_AL, // 0x06CC  # ARABIC LETTER FARSI YEH
    LineBreakClass::ULB_AL, // 0x06CD  # ARABIC LETTER YEH WITH TAIL
    LineBreakClass::ULB_AL, // 0x06CE  # ARABIC LETTER YEH WITH SMALL V
    LineBreakClass::ULB_AL, // 0x06CF  # ARABIC LETTER WAW WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x06D0  # ARABIC LETTER E
    LineBreakClass::ULB_AL, // 0x06D1  # ARABIC LETTER YEH WITH THREE DOTS BELOW
    LineBreakClass::ULB_AL, // 0x06D2  # ARABIC LETTER YEH BARREE
    LineBreakClass::ULB_AL, // 0x06D3  # ARABIC LETTER YEH BARREE WITH HAMZA ABOVE
    LineBreakClass::ULB_EX, // 0x06D4  # ARABIC FULL STOP
    LineBreakClass::ULB_AL, // 0x06D5  # ARABIC LETTER AE
    LineBreakClass::ULB_CM, // 0x06D6  # ARABIC SMALL HIGH LIGATURE SAD WITH LAM WITH ALEF MAKSURA
    LineBreakClass::ULB_CM, // 0x06D7  # ARABIC SMALL HIGH LIGATURE QAF WITH LAM WITH ALEF MAKSURA
    LineBreakClass::ULB_CM, // 0x06D8  # ARABIC SMALL HIGH MEEM INITIAL FORM
    LineBreakClass::ULB_CM, // 0x06D9  # ARABIC SMALL HIGH LAM ALEF
    LineBreakClass::ULB_CM, // 0x06DA  # ARABIC SMALL HIGH JEEM
    LineBreakClass::ULB_CM, // 0x06DB  # ARABIC SMALL HIGH THREE DOTS
    LineBreakClass::ULB_CM, // 0x06DC  # ARABIC SMALL HIGH SEEN
    LineBreakClass::ULB_AL, // 0x06DD  # ARABIC END OF AYAH
    LineBreakClass::ULB_CM, // 0x06DE  # ARABIC START OF RUB EL HIZB
    LineBreakClass::ULB_CM, // 0x06DF  # ARABIC SMALL HIGH ROUNDED ZERO
    LineBreakClass::ULB_CM, // 0x06E0  # ARABIC SMALL HIGH UPRIGHT RECTANGULAR ZERO
    LineBreakClass::ULB_CM, // 0x06E1  # ARABIC SMALL HIGH DOTLESS HEAD OF KHAH
    LineBreakClass::ULB_CM, // 0x06E2  # ARABIC SMALL HIGH MEEM ISOLATED FORM
    LineBreakClass::ULB_CM, // 0x06E3  # ARABIC SMALL LOW SEEN
    LineBreakClass::ULB_CM, // 0x06E4  # ARABIC SMALL HIGH MADDA
    LineBreakClass::ULB_AL, // 0x06E5  # ARABIC SMALL WAW
    LineBreakClass::ULB_AL, // 0x06E6  # ARABIC SMALL YEH
    LineBreakClass::ULB_CM, // 0x06E7  # ARABIC SMALL HIGH YEH
    LineBreakClass::ULB_CM, // 0x06E8  # ARABIC SMALL HIGH NOON
    LineBreakClass::ULB_AL, // 0x06E9  # ARABIC PLACE OF SAJDAH
    LineBreakClass::ULB_CM, // 0x06EA  # ARABIC EMPTY CENTRE LOW STOP
    LineBreakClass::ULB_CM, // 0x06EB  # ARABIC EMPTY CENTRE HIGH STOP
    LineBreakClass::ULB_CM, // 0x06EC  # ARABIC ROUNDED HIGH STOP WITH FILLED CENTRE
    LineBreakClass::ULB_CM, // 0x06ED  # ARABIC SMALL LOW MEEM
    LineBreakClass::ULB_AL, // 0x06EE  # ARABIC LETTER DAL WITH INVERTED V
    LineBreakClass::ULB_AL, // 0x06EF  # ARABIC LETTER REH WITH INVERTED V
    LineBreakClass::ULB_NU, // 0x06F0  # EXTENDED ARABIC-INDIC DIGIT ZERO
    LineBreakClass::ULB_NU, // 0x06F1  # EXTENDED ARABIC-INDIC DIGIT ONE
    LineBreakClass::ULB_NU, // 0x06F2  # EXTENDED ARABIC-INDIC DIGIT TWO
    LineBreakClass::ULB_NU, // 0x06F3  # EXTENDED ARABIC-INDIC DIGIT THREE
    LineBreakClass::ULB_NU, // 0x06F4  # EXTENDED ARABIC-INDIC DIGIT FOUR
    LineBreakClass::ULB_NU, // 0x06F5  # EXTENDED ARABIC-INDIC DIGIT FIVE
    LineBreakClass::ULB_NU, // 0x06F6  # EXTENDED ARABIC-INDIC DIGIT SIX
    LineBreakClass::ULB_NU, // 0x06F7  # EXTENDED ARABIC-INDIC DIGIT SEVEN
    LineBreakClass::ULB_NU, // 0x06F8  # EXTENDED ARABIC-INDIC DIGIT EIGHT
    LineBreakClass::ULB_NU, // 0x06F9  # EXTENDED ARABIC-INDIC DIGIT NINE
    LineBreakClass::ULB_AL, // 0x06FA  # ARABIC LETTER SHEEN WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x06FB  # ARABIC LETTER DAD WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x06FC  # ARABIC LETTER GHAIN WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x06FD  # ARABIC SIGN SINDHI AMPERSAND
    LineBreakClass::ULB_AL, // 0x06FE  # ARABIC SIGN SINDHI POSTPOSITION MEN
    LineBreakClass::ULB_AL, // 0x06FF  # ARABIC LETTER HEH WITH INVERTED V
    LineBreakClass::ULB_AL, // 0x0700  # SYRIAC END OF PARAGRAPH
    LineBreakClass::ULB_AL, // 0x0701  # SYRIAC SUPRALINEAR FULL STOP
    LineBreakClass::ULB_AL, // 0x0702  # SYRIAC SUBLINEAR FULL STOP
    LineBreakClass::ULB_AL, // 0x0703  # SYRIAC SUPRALINEAR COLON
    LineBreakClass::ULB_AL, // 0x0704  # SYRIAC SUBLINEAR COLON
    LineBreakClass::ULB_AL, // 0x0705  # SYRIAC HORIZONTAL COLON
    LineBreakClass::ULB_AL, // 0x0706  # SYRIAC COLON SKEWED LEFT
    LineBreakClass::ULB_AL, // 0x0707  # SYRIAC COLON SKEWED RIGHT
    LineBreakClass::ULB_AL, // 0x0708  # SYRIAC SUPRALINEAR COLON SKEWED LEFT
    LineBreakClass::ULB_AL, // 0x0709  # SYRIAC SUBLINEAR COLON SKEWED RIGHT
    LineBreakClass::ULB_AL, // 0x070A  # SYRIAC CONTRACTION
    LineBreakClass::ULB_AL, // 0x070B  # SYRIAC HARKLEAN OBELUS
    LineBreakClass::ULB_AL, // 0x070C  # SYRIAC HARKLEAN METOBELUS
    LineBreakClass::ULB_AL, // 0x070D  # SYRIAC HARKLEAN ASTERISCUS
    LineBreakClass::ULB_ID, // 0x070E # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x070F  # SYRIAC ABBREVIATION MARK
    LineBreakClass::ULB_AL, // 0x0710  # SYRIAC LETTER ALAPH
    LineBreakClass::ULB_CM, // 0x0711  # SYRIAC LETTER SUPERSCRIPT ALAPH
    LineBreakClass::ULB_AL, // 0x0712  # SYRIAC LETTER BETH
    LineBreakClass::ULB_AL, // 0x0713  # SYRIAC LETTER GAMAL
    LineBreakClass::ULB_AL, // 0x0714  # SYRIAC LETTER GAMAL GARSHUNI
    LineBreakClass::ULB_AL, // 0x0715  # SYRIAC LETTER DALATH
    LineBreakClass::ULB_AL, // 0x0716  # SYRIAC LETTER DOTLESS DALATH RISH
    LineBreakClass::ULB_AL, // 0x0717  # SYRIAC LETTER HE
    LineBreakClass::ULB_AL, // 0x0718  # SYRIAC LETTER WAW
    LineBreakClass::ULB_AL, // 0x0719  # SYRIAC LETTER ZAIN
    LineBreakClass::ULB_AL, // 0x071A  # SYRIAC LETTER HETH
    LineBreakClass::ULB_AL, // 0x071B  # SYRIAC LETTER TETH
    LineBreakClass::ULB_AL, // 0x071C  # SYRIAC LETTER TETH GARSHUNI
    LineBreakClass::ULB_AL, // 0x071D  # SYRIAC LETTER YUDH
    LineBreakClass::ULB_AL, // 0x071E  # SYRIAC LETTER YUDH HE
    LineBreakClass::ULB_AL, // 0x071F  # SYRIAC LETTER KAPH
    LineBreakClass::ULB_AL, // 0x0720  # SYRIAC LETTER LAMADH
    LineBreakClass::ULB_AL, // 0x0721  # SYRIAC LETTER MIM
    LineBreakClass::ULB_AL, // 0x0722  # SYRIAC LETTER NUN
    LineBreakClass::ULB_AL, // 0x0723  # SYRIAC LETTER SEMKATH
    LineBreakClass::ULB_AL, // 0x0724  # SYRIAC LETTER FINAL SEMKATH
    LineBreakClass::ULB_AL, // 0x0725  # SYRIAC LETTER E
    LineBreakClass::ULB_AL, // 0x0726  # SYRIAC LETTER PE
    LineBreakClass::ULB_AL, // 0x0727  # SYRIAC LETTER REVERSED PE
    LineBreakClass::ULB_AL, // 0x0728  # SYRIAC LETTER SADHE
    LineBreakClass::ULB_AL, // 0x0729  # SYRIAC LETTER QAPH
    LineBreakClass::ULB_AL, // 0x072A  # SYRIAC LETTER RISH
    LineBreakClass::ULB_AL, // 0x072B  # SYRIAC LETTER SHIN
    LineBreakClass::ULB_AL, // 0x072C  # SYRIAC LETTER TAW
    LineBreakClass::ULB_AL, // 0x072D  # SYRIAC LETTER PERSIAN BHETH
    LineBreakClass::ULB_AL, // 0x072E  # SYRIAC LETTER PERSIAN GHAMAL
    LineBreakClass::ULB_AL, // 0x072F  # SYRIAC LETTER PERSIAN DHALATH
    LineBreakClass::ULB_CM, // 0x0730  # SYRIAC PTHAHA ABOVE
    LineBreakClass::ULB_CM, // 0x0731  # SYRIAC PTHAHA BELOW
    LineBreakClass::ULB_CM, // 0x0732  # SYRIAC PTHAHA DOTTED
    LineBreakClass::ULB_CM, // 0x0733  # SYRIAC ZQAPHA ABOVE
    LineBreakClass::ULB_CM, // 0x0734  # SYRIAC ZQAPHA BELOW
    LineBreakClass::ULB_CM, // 0x0735  # SYRIAC ZQAPHA DOTTED
    LineBreakClass::ULB_CM, // 0x0736  # SYRIAC RBASA ABOVE
    LineBreakClass::ULB_CM, // 0x0737  # SYRIAC RBASA BELOW
    LineBreakClass::ULB_CM, // 0x0738  # SYRIAC DOTTED ZLAMA HORIZONTAL
    LineBreakClass::ULB_CM, // 0x0739  # SYRIAC DOTTED ZLAMA ANGULAR
    LineBreakClass::ULB_CM, // 0x073A  # SYRIAC HBASA ABOVE
    LineBreakClass::ULB_CM, // 0x073B  # SYRIAC HBASA BELOW
    LineBreakClass::ULB_CM, // 0x073C  # SYRIAC HBASA-ESASA DOTTED
    LineBreakClass::ULB_CM, // 0x073D  # SYRIAC ESASA ABOVE
    LineBreakClass::ULB_CM, // 0x073E  # SYRIAC ESASA BELOW
    LineBreakClass::ULB_CM, // 0x073F  # SYRIAC RWAHA
    LineBreakClass::ULB_CM, // 0x0740  # SYRIAC FEMININE DOT
    LineBreakClass::ULB_CM, // 0x0741  # SYRIAC QUSHSHAYA
    LineBreakClass::ULB_CM, // 0x0742  # SYRIAC RUKKAKHA
    LineBreakClass::ULB_CM, // 0x0743  # SYRIAC TWO VERTICAL DOTS ABOVE
    LineBreakClass::ULB_CM, // 0x0744  # SYRIAC TWO VERTICAL DOTS BELOW
    LineBreakClass::ULB_CM, // 0x0745  # SYRIAC THREE DOTS ABOVE
    LineBreakClass::ULB_CM, // 0x0746  # SYRIAC THREE DOTS BELOW
    LineBreakClass::ULB_CM, // 0x0747  # SYRIAC OBLIQUE LINE ABOVE
    LineBreakClass::ULB_CM, // 0x0748  # SYRIAC OBLIQUE LINE BELOW
    LineBreakClass::ULB_CM, // 0x0749  # SYRIAC MUSIC
    LineBreakClass::ULB_CM, // 0x074A  # SYRIAC BARREKH
    LineBreakClass::ULB_ID, // 0x074B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x074C # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x074D  # SYRIAC LETTER SOGDIAN ZHAIN
    LineBreakClass::ULB_AL, // 0x074E  # SYRIAC LETTER SOGDIAN KHAPH
    LineBreakClass::ULB_AL, // 0x074F  # SYRIAC LETTER SOGDIAN FE
    LineBreakClass::ULB_AL, // 0x0750  # ARABIC LETTER BEH WITH THREE DOTS HORIZONTALLY BELOW
    LineBreakClass::ULB_AL, // 0x0751  # ARABIC LETTER BEH WITH DOT BELOW AND THREE DOTS ABOVE
    LineBreakClass::ULB_AL, // 0x0752  # ARABIC LETTER BEH WITH THREE DOTS POINTING UPWARDS BELOW
    LineBreakClass::ULB_AL, // 0x0753  # ARABIC LETTER BEH WITH THREE DOTS POINTING UPWARDS BELOW
                            // AND TWO DOTS ABOVE
    LineBreakClass::ULB_AL, // 0x0754  # ARABIC LETTER BEH WITH TWO DOTS BELOW AND DOT ABOVE
    LineBreakClass::ULB_AL, // 0x0755  # ARABIC LETTER BEH WITH INVERTED SMALL V BELOW
    LineBreakClass::ULB_AL, // 0x0756  # ARABIC LETTER BEH WITH SMALL V
    LineBreakClass::ULB_AL, // 0x0757  # ARABIC LETTER HAH WITH TWO DOTS ABOVE
    LineBreakClass::ULB_AL, // 0x0758  # ARABIC LETTER HAH WITH THREE DOTS POINTING UPWARDS BELOW
    LineBreakClass::ULB_AL, // 0x0759  # ARABIC LETTER DAL WITH TWO DOTS VERTICALLY BELOW AND SMALL
                            // TAH
    LineBreakClass::ULB_AL, // 0x075A  # ARABIC LETTER DAL WITH INVERTED SMALL V BELOW
    LineBreakClass::ULB_AL, // 0x075B  # ARABIC LETTER REH WITH STROKE
    LineBreakClass::ULB_AL, // 0x075C  # ARABIC LETTER SEEN WITH FOUR DOTS ABOVE
    LineBreakClass::ULB_AL, // 0x075D  # ARABIC LETTER AIN WITH TWO DOTS ABOVE
    LineBreakClass::ULB_AL, // 0x075E  # ARABIC LETTER AIN WITH THREE DOTS POINTING DOWNWARDS ABOVE
    LineBreakClass::ULB_AL, // 0x075F  # ARABIC LETTER AIN WITH TWO DOTS VERTICALLY ABOVE
    LineBreakClass::ULB_AL, // 0x0760  # ARABIC LETTER FEH WITH TWO DOTS BELOW
    LineBreakClass::ULB_AL, // 0x0761  # ARABIC LETTER FEH WITH THREE DOTS POINTING UPWARDS BELOW
    LineBreakClass::ULB_AL, // 0x0762  # ARABIC LETTER KEHEH WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x0763  # ARABIC LETTER KEHEH WITH THREE DOTS ABOVE
    LineBreakClass::ULB_AL, // 0x0764  # ARABIC LETTER KEHEH WITH THREE DOTS POINTING UPWARDS BELOW
    LineBreakClass::ULB_AL, // 0x0765  # ARABIC LETTER MEEM WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x0766  # ARABIC LETTER MEEM WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x0767  # ARABIC LETTER NOON WITH TWO DOTS BELOW
    LineBreakClass::ULB_AL, // 0x0768  # ARABIC LETTER NOON WITH SMALL TAH
    LineBreakClass::ULB_AL, // 0x0769  # ARABIC LETTER NOON WITH SMALL V
    LineBreakClass::ULB_AL, // 0x076A  # ARABIC LETTER LAM WITH BAR
    LineBreakClass::ULB_AL, // 0x076B  # ARABIC LETTER REH WITH TWO DOTS VERTICALLY ABOVE
    LineBreakClass::ULB_AL, // 0x076C  # ARABIC LETTER REH WITH HAMZA ABOVE
    LineBreakClass::ULB_AL, // 0x076D  # ARABIC LETTER SEEN WITH TWO DOTS VERTICALLY ABOVE
    LineBreakClass::ULB_ID, // 0x076E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x076F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0770 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0771 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0772 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0773 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0774 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0775 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0776 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0777 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0778 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0779 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x077A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x077B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x077C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x077D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x077E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x077F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0780  # THAANA LETTER HAA
    LineBreakClass::ULB_AL, // 0x0781  # THAANA LETTER SHAVIYANI
    LineBreakClass::ULB_AL, // 0x0782  # THAANA LETTER NOONU
    LineBreakClass::ULB_AL, // 0x0783  # THAANA LETTER RAA
    LineBreakClass::ULB_AL, // 0x0784  # THAANA LETTER BAA
    LineBreakClass::ULB_AL, // 0x0785  # THAANA LETTER LHAVIYANI
    LineBreakClass::ULB_AL, // 0x0786  # THAANA LETTER KAAFU
    LineBreakClass::ULB_AL, // 0x0787  # THAANA LETTER ALIFU
    LineBreakClass::ULB_AL, // 0x0788  # THAANA LETTER VAAVU
    LineBreakClass::ULB_AL, // 0x0789  # THAANA LETTER MEEMU
    LineBreakClass::ULB_AL, // 0x078A  # THAANA LETTER FAAFU
    LineBreakClass::ULB_AL, // 0x078B  # THAANA LETTER DHAALU
    LineBreakClass::ULB_AL, // 0x078C  # THAANA LETTER THAA
    LineBreakClass::ULB_AL, // 0x078D  # THAANA LETTER LAAMU
    LineBreakClass::ULB_AL, // 0x078E  # THAANA LETTER GAAFU
    LineBreakClass::ULB_AL, // 0x078F  # THAANA LETTER GNAVIYANI
    LineBreakClass::ULB_AL, // 0x0790  # THAANA LETTER SEENU
    LineBreakClass::ULB_AL, // 0x0791  # THAANA LETTER DAVIYANI
    LineBreakClass::ULB_AL, // 0x0792  # THAANA LETTER ZAVIYANI
    LineBreakClass::ULB_AL, // 0x0793  # THAANA LETTER TAVIYANI
    LineBreakClass::ULB_AL, // 0x0794  # THAANA LETTER YAA
    LineBreakClass::ULB_AL, // 0x0795  # THAANA LETTER PAVIYANI
    LineBreakClass::ULB_AL, // 0x0796  # THAANA LETTER JAVIYANI
    LineBreakClass::ULB_AL, // 0x0797  # THAANA LETTER CHAVIYANI
    LineBreakClass::ULB_AL, // 0x0798  # THAANA LETTER TTAA
    LineBreakClass::ULB_AL, // 0x0799  # THAANA LETTER HHAA
    LineBreakClass::ULB_AL, // 0x079A  # THAANA LETTER KHAA
    LineBreakClass::ULB_AL, // 0x079B  # THAANA LETTER THAALU
    LineBreakClass::ULB_AL, // 0x079C  # THAANA LETTER ZAA
    LineBreakClass::ULB_AL, // 0x079D  # THAANA LETTER SHEENU
    LineBreakClass::ULB_AL, // 0x079E  # THAANA LETTER SAADHU
    LineBreakClass::ULB_AL, // 0x079F  # THAANA LETTER DAADHU
    LineBreakClass::ULB_AL, // 0x07A0  # THAANA LETTER TO
    LineBreakClass::ULB_AL, // 0x07A1  # THAANA LETTER ZO
    LineBreakClass::ULB_AL, // 0x07A2  # THAANA LETTER AINU
    LineBreakClass::ULB_AL, // 0x07A3  # THAANA LETTER GHAINU
    LineBreakClass::ULB_AL, // 0x07A4  # THAANA LETTER QAAFU
    LineBreakClass::ULB_AL, // 0x07A5  # THAANA LETTER WAAVU
    LineBreakClass::ULB_CM, // 0x07A6  # THAANA ABAFILI
    LineBreakClass::ULB_CM, // 0x07A7  # THAANA AABAAFILI
    LineBreakClass::ULB_CM, // 0x07A8  # THAANA IBIFILI
    LineBreakClass::ULB_CM, // 0x07A9  # THAANA EEBEEFILI
    LineBreakClass::ULB_CM, // 0x07AA  # THAANA UBUFILI
    LineBreakClass::ULB_CM, // 0x07AB  # THAANA OOBOOFILI
    LineBreakClass::ULB_CM, // 0x07AC  # THAANA EBEFILI
    LineBreakClass::ULB_CM, // 0x07AD  # THAANA EYBEYFILI
    LineBreakClass::ULB_CM, // 0x07AE  # THAANA OBOFILI
    LineBreakClass::ULB_CM, // 0x07AF  # THAANA OABOAFILI
    LineBreakClass::ULB_CM, // 0x07B0  # THAANA SUKUN
    LineBreakClass::ULB_AL, // 0x07B1  # THAANA LETTER NAA
    LineBreakClass::ULB_ID, // 0x07B2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x07B3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x07B4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x07B5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x07B6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x07B7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x07B8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x07B9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x07BA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x07BB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x07BC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x07BD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x07BE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x07BF # <UNDEFINED>
    LineBreakClass::ULB_NU, // 0x07C0  # NKO DIGIT ZERO
    LineBreakClass::ULB_NU, // 0x07C1  # NKO DIGIT ONE
    LineBreakClass::ULB_NU, // 0x07C2  # NKO DIGIT TWO
    LineBreakClass::ULB_NU, // 0x07C3  # NKO DIGIT THREE
    LineBreakClass::ULB_NU, // 0x07C4  # NKO DIGIT FOUR
    LineBreakClass::ULB_NU, // 0x07C5  # NKO DIGIT FIVE
    LineBreakClass::ULB_NU, // 0x07C6  # NKO DIGIT SIX
    LineBreakClass::ULB_NU, // 0x07C7  # NKO DIGIT SEVEN
    LineBreakClass::ULB_NU, // 0x07C8  # NKO DIGIT EIGHT
    LineBreakClass::ULB_NU, // 0x07C9  # NKO DIGIT NINE
    LineBreakClass::ULB_AL, // 0x07CA  # NKO LETTER A
    LineBreakClass::ULB_AL, // 0x07CB  # NKO LETTER EE
    LineBreakClass::ULB_AL, // 0x07CC  # NKO LETTER I
    LineBreakClass::ULB_AL, // 0x07CD  # NKO LETTER E
    LineBreakClass::ULB_AL, // 0x07CE  # NKO LETTER U
    LineBreakClass::ULB_AL, // 0x07CF  # NKO LETTER OO
    LineBreakClass::ULB_AL, // 0x07D0  # NKO LETTER O
    LineBreakClass::ULB_AL, // 0x07D1  # NKO LETTER DAGBASINNA
    LineBreakClass::ULB_AL, // 0x07D2  # NKO LETTER N
    LineBreakClass::ULB_AL, // 0x07D3  # NKO LETTER BA
    LineBreakClass::ULB_AL, // 0x07D4  # NKO LETTER PA
    LineBreakClass::ULB_AL, // 0x07D5  # NKO LETTER TA
    LineBreakClass::ULB_AL, // 0x07D6  # NKO LETTER JA
    LineBreakClass::ULB_AL, // 0x07D7  # NKO LETTER CHA
    LineBreakClass::ULB_AL, // 0x07D8  # NKO LETTER DA
    LineBreakClass::ULB_AL, // 0x07D9  # NKO LETTER RA
    LineBreakClass::ULB_AL, // 0x07DA  # NKO LETTER RRA
    LineBreakClass::ULB_AL, // 0x07DB  # NKO LETTER SA
    LineBreakClass::ULB_AL, // 0x07DC  # NKO LETTER GBA
    LineBreakClass::ULB_AL, // 0x07DD  # NKO LETTER FA
    LineBreakClass::ULB_AL, // 0x07DE  # NKO LETTER KA
    LineBreakClass::ULB_AL, // 0x07DF  # NKO LETTER LA
    LineBreakClass::ULB_AL, // 0x07E0  # NKO LETTER NA WOLOSO
    LineBreakClass::ULB_AL, // 0x07E1  # NKO LETTER MA
    LineBreakClass::ULB_AL, // 0x07E2  # NKO LETTER NYA
    LineBreakClass::ULB_AL, // 0x07E3  # NKO LETTER NA
    LineBreakClass::ULB_AL, // 0x07E4  # NKO LETTER HA
    LineBreakClass::ULB_AL, // 0x07E5  # NKO LETTER WA
    LineBreakClass::ULB_AL, // 0x07E6  # NKO LETTER YA
    LineBreakClass::ULB_AL, // 0x07E7  # NKO LETTER NYA WOLOSO
    LineBreakClass::ULB_AL, // 0x07E8  # NKO LETTER JONA JA
    LineBreakClass::ULB_AL, // 0x07E9  # NKO LETTER JONA CHA
    LineBreakClass::ULB_AL, // 0x07EA  # NKO LETTER JONA RA
    LineBreakClass::ULB_CM, // 0x07EB  # NKO COMBINING SHORT HIGH TONE
    LineBreakClass::ULB_CM, // 0x07EC  # NKO COMBINING SHORT LOW TONE
    LineBreakClass::ULB_CM, // 0x07ED  # NKO COMBINING SHORT RISING TONE
    LineBreakClass::ULB_CM, // 0x07EE  # NKO COMBINING LONG DESCENDING TONE
    LineBreakClass::ULB_CM, // 0x07EF  # NKO COMBINING LONG HIGH TONE
    LineBreakClass::ULB_CM, // 0x07F0  # NKO COMBINING LONG LOW TONE
    LineBreakClass::ULB_CM, // 0x07F1  # NKO COMBINING LONG RISING TONE
    LineBreakClass::ULB_CM, // 0x07F2  # NKO COMBINING NASALIZATION MARK
    LineBreakClass::ULB_CM, // 0x07F3  # NKO COMBINING DOUBLE DOT ABOVE
    LineBreakClass::ULB_AL, // 0x07F4  # NKO HIGH TONE APOSTROPHE
    LineBreakClass::ULB_AL, // 0x07F5  # NKO LOW TONE APOSTROPHE
    LineBreakClass::ULB_AL, // 0x07F6  # NKO SYMBOL OO DENNEN
    LineBreakClass::ULB_AL, // 0x07F7  # NKO SYMBOL GBAKURUNEN
    LineBreakClass::ULB_IS, // 0x07F8  # NKO COMMA
    LineBreakClass::ULB_EX, // 0x07F9  # NKO EXCLAMATION MARK
    LineBreakClass::ULB_AL, // 0x07FA  # NKO LAJANYALAN
    LineBreakClass::ULB_ID, // 0x07FB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x07FC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x07FD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x07FE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x07FF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0800 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0801 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0802 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0803 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0804 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0805 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0806 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0807 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0808 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0809 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x080A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x080B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x080C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x080D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x080E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x080F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0810 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0811 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0812 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0813 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0814 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0815 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0816 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0817 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0818 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0819 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x081A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x081B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x081C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x081D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x081E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x081F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0820 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0821 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0822 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0823 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0824 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0825 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0826 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0827 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0828 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0829 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x082A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x082B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x082C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x082D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x082E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x082F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0830 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0831 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0832 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0833 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0834 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0835 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0836 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0837 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0838 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0839 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x083A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x083B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x083C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x083D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x083E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x083F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0840 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0841 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0842 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0843 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0844 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0845 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0846 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0847 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0848 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0849 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x084A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x084B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x084C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x084D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x084E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x084F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0850 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0851 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0852 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0853 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0854 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0855 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0856 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0857 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0858 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0859 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x085A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x085B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x085C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x085D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x085E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x085F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0860 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0861 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0862 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0863 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0864 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0865 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0866 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0867 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0868 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0869 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x086A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x086B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x086C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x086D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x086E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x086F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0870 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0871 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0872 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0873 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0874 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0875 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0876 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0877 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0878 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0879 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x087A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x087B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x087C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x087D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x087E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x087F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0880 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0881 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0882 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0883 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0884 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0885 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0886 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0887 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0888 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0889 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x088A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x088B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x088C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x088D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x088E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x088F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0890 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0891 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0892 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0893 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0894 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0895 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0896 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0897 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0898 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0899 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x089A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x089B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x089C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x089D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x089E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x089F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08A0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08A1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08A2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08A3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08A4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08A5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08A6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08A7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08A8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08A9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08AA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08AB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08AC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08AD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08AE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08AF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08B0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08B1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08B2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08B3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08B4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08B5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08B6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08B7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08B8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08B9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08BA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08BB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08BC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08BD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08BE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08BF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08C0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08C1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08C2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08C3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08C4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08C5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08C6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08C7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08C8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08C9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08CA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08CB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08CC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08CD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08CE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08CF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08D0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08D1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08D2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08D3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08D4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08D5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08D6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08D7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08D8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08D9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08DA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08DB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08DC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08DD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08DE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08DF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08E0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08E1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08E2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08E3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08E4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08E5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08E6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08E7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08E8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08E9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08EA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08EB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08EC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08ED # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08EE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08EF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08F0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08F1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08F2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08F3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08F4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08F5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08F6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08F7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08F8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08F9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08FA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08FB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08FC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08FD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08FE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x08FF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0900 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0901  # DEVANAGARI SIGN CANDRABINDU
    LineBreakClass::ULB_CM, // 0x0902  # DEVANAGARI SIGN ANUSVARA
    LineBreakClass::ULB_CM, // 0x0903  # DEVANAGARI SIGN VISARGA
    LineBreakClass::ULB_AL, // 0x0904  # DEVANAGARI LETTER SHORT A
    LineBreakClass::ULB_AL, // 0x0905  # DEVANAGARI LETTER A
    LineBreakClass::ULB_AL, // 0x0906  # DEVANAGARI LETTER AA
    LineBreakClass::ULB_AL, // 0x0907  # DEVANAGARI LETTER I
    LineBreakClass::ULB_AL, // 0x0908  # DEVANAGARI LETTER II
    LineBreakClass::ULB_AL, // 0x0909  # DEVANAGARI LETTER U
    LineBreakClass::ULB_AL, // 0x090A  # DEVANAGARI LETTER UU
    LineBreakClass::ULB_AL, // 0x090B  # DEVANAGARI LETTER VOCALIC R
    LineBreakClass::ULB_AL, // 0x090C  # DEVANAGARI LETTER VOCALIC L
    LineBreakClass::ULB_AL, // 0x090D  # DEVANAGARI LETTER CANDRA E
    LineBreakClass::ULB_AL, // 0x090E  # DEVANAGARI LETTER SHORT E
    LineBreakClass::ULB_AL, // 0x090F  # DEVANAGARI LETTER E
    LineBreakClass::ULB_AL, // 0x0910  # DEVANAGARI LETTER AI
    LineBreakClass::ULB_AL, // 0x0911  # DEVANAGARI LETTER CANDRA O
    LineBreakClass::ULB_AL, // 0x0912  # DEVANAGARI LETTER SHORT O
    LineBreakClass::ULB_AL, // 0x0913  # DEVANAGARI LETTER O
    LineBreakClass::ULB_AL, // 0x0914  # DEVANAGARI LETTER AU
    LineBreakClass::ULB_AL, // 0x0915  # DEVANAGARI LETTER KA
    LineBreakClass::ULB_AL, // 0x0916  # DEVANAGARI LETTER KHA
    LineBreakClass::ULB_AL, // 0x0917  # DEVANAGARI LETTER GA
    LineBreakClass::ULB_AL, // 0x0918  # DEVANAGARI LETTER GHA
    LineBreakClass::ULB_AL, // 0x0919  # DEVANAGARI LETTER NGA
    LineBreakClass::ULB_AL, // 0x091A  # DEVANAGARI LETTER CA
    LineBreakClass::ULB_AL, // 0x091B  # DEVANAGARI LETTER CHA
    LineBreakClass::ULB_AL, // 0x091C  # DEVANAGARI LETTER JA
    LineBreakClass::ULB_AL, // 0x091D  # DEVANAGARI LETTER JHA
    LineBreakClass::ULB_AL, // 0x091E  # DEVANAGARI LETTER NYA
    LineBreakClass::ULB_AL, // 0x091F  # DEVANAGARI LETTER TTA
    LineBreakClass::ULB_AL, // 0x0920  # DEVANAGARI LETTER TTHA
    LineBreakClass::ULB_AL, // 0x0921  # DEVANAGARI LETTER DDA
    LineBreakClass::ULB_AL, // 0x0922  # DEVANAGARI LETTER DDHA
    LineBreakClass::ULB_AL, // 0x0923  # DEVANAGARI LETTER NNA
    LineBreakClass::ULB_AL, // 0x0924  # DEVANAGARI LETTER TA
    LineBreakClass::ULB_AL, // 0x0925  # DEVANAGARI LETTER THA
    LineBreakClass::ULB_AL, // 0x0926  # DEVANAGARI LETTER DA
    LineBreakClass::ULB_AL, // 0x0927  # DEVANAGARI LETTER DHA
    LineBreakClass::ULB_AL, // 0x0928  # DEVANAGARI LETTER NA
    LineBreakClass::ULB_AL, // 0x0929  # DEVANAGARI LETTER NNNA
    LineBreakClass::ULB_AL, // 0x092A  # DEVANAGARI LETTER PA
    LineBreakClass::ULB_AL, // 0x092B  # DEVANAGARI LETTER PHA
    LineBreakClass::ULB_AL, // 0x092C  # DEVANAGARI LETTER BA
    LineBreakClass::ULB_AL, // 0x092D  # DEVANAGARI LETTER BHA
    LineBreakClass::ULB_AL, // 0x092E  # DEVANAGARI LETTER MA
    LineBreakClass::ULB_AL, // 0x092F  # DEVANAGARI LETTER YA
    LineBreakClass::ULB_AL, // 0x0930  # DEVANAGARI LETTER RA
    LineBreakClass::ULB_AL, // 0x0931  # DEVANAGARI LETTER RRA
    LineBreakClass::ULB_AL, // 0x0932  # DEVANAGARI LETTER LA
    LineBreakClass::ULB_AL, // 0x0933  # DEVANAGARI LETTER LLA
    LineBreakClass::ULB_AL, // 0x0934  # DEVANAGARI LETTER LLLA
    LineBreakClass::ULB_AL, // 0x0935  # DEVANAGARI LETTER VA
    LineBreakClass::ULB_AL, // 0x0936  # DEVANAGARI LETTER SHA
    LineBreakClass::ULB_AL, // 0x0937  # DEVANAGARI LETTER SSA
    LineBreakClass::ULB_AL, // 0x0938  # DEVANAGARI LETTER SA
    LineBreakClass::ULB_AL, // 0x0939  # DEVANAGARI LETTER HA
    LineBreakClass::ULB_ID, // 0x093A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x093B # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x093C  # DEVANAGARI SIGN NUKTA
    LineBreakClass::ULB_AL, // 0x093D  # DEVANAGARI SIGN AVAGRAHA
    LineBreakClass::ULB_CM, // 0x093E  # DEVANAGARI VOWEL SIGN AA
    LineBreakClass::ULB_CM, // 0x093F  # DEVANAGARI VOWEL SIGN I
    LineBreakClass::ULB_CM, // 0x0940  # DEVANAGARI VOWEL SIGN II
    LineBreakClass::ULB_CM, // 0x0941  # DEVANAGARI VOWEL SIGN U
    LineBreakClass::ULB_CM, // 0x0942  # DEVANAGARI VOWEL SIGN UU
    LineBreakClass::ULB_CM, // 0x0943  # DEVANAGARI VOWEL SIGN VOCALIC R
    LineBreakClass::ULB_CM, // 0x0944  # DEVANAGARI VOWEL SIGN VOCALIC RR
    LineBreakClass::ULB_CM, // 0x0945  # DEVANAGARI VOWEL SIGN CANDRA E
    LineBreakClass::ULB_CM, // 0x0946  # DEVANAGARI VOWEL SIGN SHORT E
    LineBreakClass::ULB_CM, // 0x0947  # DEVANAGARI VOWEL SIGN E
    LineBreakClass::ULB_CM, // 0x0948  # DEVANAGARI VOWEL SIGN AI
    LineBreakClass::ULB_CM, // 0x0949  # DEVANAGARI VOWEL SIGN CANDRA O
    LineBreakClass::ULB_CM, // 0x094A  # DEVANAGARI VOWEL SIGN SHORT O
    LineBreakClass::ULB_CM, // 0x094B  # DEVANAGARI VOWEL SIGN O
    LineBreakClass::ULB_CM, // 0x094C  # DEVANAGARI VOWEL SIGN AU
    LineBreakClass::ULB_CM, // 0x094D  # DEVANAGARI SIGN VIRAMA
    LineBreakClass::ULB_ID, // 0x094E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x094F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0950  # DEVANAGARI OM
    LineBreakClass::ULB_CM, // 0x0951  # DEVANAGARI STRESS SIGN UDATTA
    LineBreakClass::ULB_CM, // 0x0952  # DEVANAGARI STRESS SIGN ANUDATTA
    LineBreakClass::ULB_CM, // 0x0953  # DEVANAGARI GRAVE ACCENT
    LineBreakClass::ULB_CM, // 0x0954  # DEVANAGARI ACUTE ACCENT
    LineBreakClass::ULB_ID, // 0x0955 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0956 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0957 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0958  # DEVANAGARI LETTER QA
    LineBreakClass::ULB_AL, // 0x0959  # DEVANAGARI LETTER KHHA
    LineBreakClass::ULB_AL, // 0x095A  # DEVANAGARI LETTER GHHA
    LineBreakClass::ULB_AL, // 0x095B  # DEVANAGARI LETTER ZA
    LineBreakClass::ULB_AL, // 0x095C  # DEVANAGARI LETTER DDDHA
    LineBreakClass::ULB_AL, // 0x095D  # DEVANAGARI LETTER RHA
    LineBreakClass::ULB_AL, // 0x095E  # DEVANAGARI LETTER FA
    LineBreakClass::ULB_AL, // 0x095F  # DEVANAGARI LETTER YYA
    LineBreakClass::ULB_AL, // 0x0960  # DEVANAGARI LETTER VOCALIC RR
    LineBreakClass::ULB_AL, // 0x0961  # DEVANAGARI LETTER VOCALIC LL
    LineBreakClass::ULB_CM, // 0x0962  # DEVANAGARI VOWEL SIGN VOCALIC L
    LineBreakClass::ULB_CM, // 0x0963  # DEVANAGARI VOWEL SIGN VOCALIC LL
    LineBreakClass::ULB_BA, // 0x0964  # DEVANAGARI DANDA
    LineBreakClass::ULB_BA, // 0x0965  # DEVANAGARI DOUBLE DANDA
    LineBreakClass::ULB_NU, // 0x0966  # DEVANAGARI DIGIT ZERO
    LineBreakClass::ULB_NU, // 0x0967  # DEVANAGARI DIGIT ONE
    LineBreakClass::ULB_NU, // 0x0968  # DEVANAGARI DIGIT TWO
    LineBreakClass::ULB_NU, // 0x0969  # DEVANAGARI DIGIT THREE
    LineBreakClass::ULB_NU, // 0x096A  # DEVANAGARI DIGIT FOUR
    LineBreakClass::ULB_NU, // 0x096B  # DEVANAGARI DIGIT FIVE
    LineBreakClass::ULB_NU, // 0x096C  # DEVANAGARI DIGIT SIX
    LineBreakClass::ULB_NU, // 0x096D  # DEVANAGARI DIGIT SEVEN
    LineBreakClass::ULB_NU, // 0x096E  # DEVANAGARI DIGIT EIGHT
    LineBreakClass::ULB_NU, // 0x096F  # DEVANAGARI DIGIT NINE
    LineBreakClass::ULB_AL, // 0x0970  # DEVANAGARI ABBREVIATION SIGN
    LineBreakClass::ULB_ID, // 0x0971 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0972 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0973 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0974 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0975 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0976 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0977 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0978 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0979 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x097A # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x097B  # DEVANAGARI LETTER GGA
    LineBreakClass::ULB_AL, // 0x097C  # DEVANAGARI LETTER JJA
    LineBreakClass::ULB_AL, // 0x097D  # DEVANAGARI LETTER GLOTTAL STOP
    LineBreakClass::ULB_AL, // 0x097E  # DEVANAGARI LETTER DDDA
    LineBreakClass::ULB_AL, // 0x097F  # DEVANAGARI LETTER BBA
    LineBreakClass::ULB_ID, // 0x0980 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0981  # BENGALI SIGN CANDRABINDU
    LineBreakClass::ULB_CM, // 0x0982  # BENGALI SIGN ANUSVARA
    LineBreakClass::ULB_CM, // 0x0983  # BENGALI SIGN VISARGA
    LineBreakClass::ULB_ID, // 0x0984 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0985  # BENGALI LETTER A
    LineBreakClass::ULB_AL, // 0x0986  # BENGALI LETTER AA
    LineBreakClass::ULB_AL, // 0x0987  # BENGALI LETTER I
    LineBreakClass::ULB_AL, // 0x0988  # BENGALI LETTER II
    LineBreakClass::ULB_AL, // 0x0989  # BENGALI LETTER U
    LineBreakClass::ULB_AL, // 0x098A  # BENGALI LETTER UU
    LineBreakClass::ULB_AL, // 0x098B  # BENGALI LETTER VOCALIC R
    LineBreakClass::ULB_AL, // 0x098C  # BENGALI LETTER VOCALIC L
    LineBreakClass::ULB_ID, // 0x098D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x098E # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x098F  # BENGALI LETTER E
    LineBreakClass::ULB_AL, // 0x0990  # BENGALI LETTER AI
    LineBreakClass::ULB_ID, // 0x0991 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0992 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0993  # BENGALI LETTER O
    LineBreakClass::ULB_AL, // 0x0994  # BENGALI LETTER AU
    LineBreakClass::ULB_AL, // 0x0995  # BENGALI LETTER KA
    LineBreakClass::ULB_AL, // 0x0996  # BENGALI LETTER KHA
    LineBreakClass::ULB_AL, // 0x0997  # BENGALI LETTER GA
    LineBreakClass::ULB_AL, // 0x0998  # BENGALI LETTER GHA
    LineBreakClass::ULB_AL, // 0x0999  # BENGALI LETTER NGA
    LineBreakClass::ULB_AL, // 0x099A  # BENGALI LETTER CA
    LineBreakClass::ULB_AL, // 0x099B  # BENGALI LETTER CHA
    LineBreakClass::ULB_AL, // 0x099C  # BENGALI LETTER JA
    LineBreakClass::ULB_AL, // 0x099D  # BENGALI LETTER JHA
    LineBreakClass::ULB_AL, // 0x099E  # BENGALI LETTER NYA
    LineBreakClass::ULB_AL, // 0x099F  # BENGALI LETTER TTA
    LineBreakClass::ULB_AL, // 0x09A0  # BENGALI LETTER TTHA
    LineBreakClass::ULB_AL, // 0x09A1  # BENGALI LETTER DDA
    LineBreakClass::ULB_AL, // 0x09A2  # BENGALI LETTER DDHA
    LineBreakClass::ULB_AL, // 0x09A3  # BENGALI LETTER NNA
    LineBreakClass::ULB_AL, // 0x09A4  # BENGALI LETTER TA
    LineBreakClass::ULB_AL, // 0x09A5  # BENGALI LETTER THA
    LineBreakClass::ULB_AL, // 0x09A6  # BENGALI LETTER DA
    LineBreakClass::ULB_AL, // 0x09A7  # BENGALI LETTER DHA
    LineBreakClass::ULB_AL, // 0x09A8  # BENGALI LETTER NA
    LineBreakClass::ULB_ID, // 0x09A9 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x09AA  # BENGALI LETTER PA
    LineBreakClass::ULB_AL, // 0x09AB  # BENGALI LETTER PHA
    LineBreakClass::ULB_AL, // 0x09AC  # BENGALI LETTER BA
    LineBreakClass::ULB_AL, // 0x09AD  # BENGALI LETTER BHA
    LineBreakClass::ULB_AL, // 0x09AE  # BENGALI LETTER MA
    LineBreakClass::ULB_AL, // 0x09AF  # BENGALI LETTER YA
    LineBreakClass::ULB_AL, // 0x09B0  # BENGALI LETTER RA
    LineBreakClass::ULB_ID, // 0x09B1 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x09B2  # BENGALI LETTER LA
    LineBreakClass::ULB_ID, // 0x09B3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x09B4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x09B5 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x09B6  # BENGALI LETTER SHA
    LineBreakClass::ULB_AL, // 0x09B7  # BENGALI LETTER SSA
    LineBreakClass::ULB_AL, // 0x09B8  # BENGALI LETTER SA
    LineBreakClass::ULB_AL, // 0x09B9  # BENGALI LETTER HA
    LineBreakClass::ULB_ID, // 0x09BA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x09BB # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x09BC  # BENGALI SIGN NUKTA
    LineBreakClass::ULB_AL, // 0x09BD  # BENGALI SIGN AVAGRAHA
    LineBreakClass::ULB_CM, // 0x09BE  # BENGALI VOWEL SIGN AA
    LineBreakClass::ULB_CM, // 0x09BF  # BENGALI VOWEL SIGN I
    LineBreakClass::ULB_CM, // 0x09C0  # BENGALI VOWEL SIGN II
    LineBreakClass::ULB_CM, // 0x09C1  # BENGALI VOWEL SIGN U
    LineBreakClass::ULB_CM, // 0x09C2  # BENGALI VOWEL SIGN UU
    LineBreakClass::ULB_CM, // 0x09C3  # BENGALI VOWEL SIGN VOCALIC R
    LineBreakClass::ULB_CM, // 0x09C4  # BENGALI VOWEL SIGN VOCALIC RR
    LineBreakClass::ULB_ID, // 0x09C5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x09C6 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x09C7  # BENGALI VOWEL SIGN E
    LineBreakClass::ULB_CM, // 0x09C8  # BENGALI VOWEL SIGN AI
    LineBreakClass::ULB_ID, // 0x09C9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x09CA # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x09CB  # BENGALI VOWEL SIGN O
    LineBreakClass::ULB_CM, // 0x09CC  # BENGALI VOWEL SIGN AU
    LineBreakClass::ULB_CM, // 0x09CD  # BENGALI SIGN VIRAMA
    LineBreakClass::ULB_AL, // 0x09CE  # BENGALI LETTER KHANDA TA
    LineBreakClass::ULB_ID, // 0x09CF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x09D0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x09D1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x09D2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x09D3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x09D4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x09D5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x09D6 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x09D7  # BENGALI AU LENGTH MARK
    LineBreakClass::ULB_ID, // 0x09D8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x09D9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x09DA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x09DB # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x09DC  # BENGALI LETTER RRA
    LineBreakClass::ULB_AL, // 0x09DD  # BENGALI LETTER RHA
    LineBreakClass::ULB_ID, // 0x09DE # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x09DF  # BENGALI LETTER YYA
    LineBreakClass::ULB_AL, // 0x09E0  # BENGALI LETTER VOCALIC RR
    LineBreakClass::ULB_AL, // 0x09E1  # BENGALI LETTER VOCALIC LL
    LineBreakClass::ULB_CM, // 0x09E2  # BENGALI VOWEL SIGN VOCALIC L
    LineBreakClass::ULB_CM, // 0x09E3  # BENGALI VOWEL SIGN VOCALIC LL
    LineBreakClass::ULB_ID, // 0x09E4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x09E5 # <UNDEFINED>
    LineBreakClass::ULB_NU, // 0x09E6  # BENGALI DIGIT ZERO
    LineBreakClass::ULB_NU, // 0x09E7  # BENGALI DIGIT ONE
    LineBreakClass::ULB_NU, // 0x09E8  # BENGALI DIGIT TWO
    LineBreakClass::ULB_NU, // 0x09E9  # BENGALI DIGIT THREE
    LineBreakClass::ULB_NU, // 0x09EA  # BENGALI DIGIT FOUR
    LineBreakClass::ULB_NU, // 0x09EB  # BENGALI DIGIT FIVE
    LineBreakClass::ULB_NU, // 0x09EC  # BENGALI DIGIT SIX
    LineBreakClass::ULB_NU, // 0x09ED  # BENGALI DIGIT SEVEN
    LineBreakClass::ULB_NU, // 0x09EE  # BENGALI DIGIT EIGHT
    LineBreakClass::ULB_NU, // 0x09EF  # BENGALI DIGIT NINE
    LineBreakClass::ULB_AL, // 0x09F0  # BENGALI LETTER RA WITH MIDDLE DIAGONAL
    LineBreakClass::ULB_AL, // 0x09F1  # BENGALI LETTER RA WITH LOWER DIAGONAL
    LineBreakClass::ULB_PR, // 0x09F2  # BENGALI RUPEE MARK
    LineBreakClass::ULB_PR, // 0x09F3  # BENGALI RUPEE SIGN
    LineBreakClass::ULB_AL, // 0x09F4  # BENGALI CURRENCY NUMERATOR ONE
    LineBreakClass::ULB_AL, // 0x09F5  # BENGALI CURRENCY NUMERATOR TWO
    LineBreakClass::ULB_AL, // 0x09F6  # BENGALI CURRENCY NUMERATOR THREE
    LineBreakClass::ULB_AL, // 0x09F7  # BENGALI CURRENCY NUMERATOR FOUR
    LineBreakClass::ULB_AL, // 0x09F8  # BENGALI CURRENCY NUMERATOR ONE LESS THAN THE DENOMINATOR
    LineBreakClass::ULB_AL, // 0x09F9  # BENGALI CURRENCY DENOMINATOR SIXTEEN
    LineBreakClass::ULB_AL, // 0x09FA  # BENGALI ISSHAR
    LineBreakClass::ULB_ID, // 0x09FB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x09FC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x09FD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x09FE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x09FF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A00 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0A01  # GURMUKHI SIGN ADAK BINDI
    LineBreakClass::ULB_CM, // 0x0A02  # GURMUKHI SIGN BINDI
    LineBreakClass::ULB_CM, // 0x0A03  # GURMUKHI SIGN VISARGA
    LineBreakClass::ULB_ID, // 0x0A04 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0A05  # GURMUKHI LETTER A
    LineBreakClass::ULB_AL, // 0x0A06  # GURMUKHI LETTER AA
    LineBreakClass::ULB_AL, // 0x0A07  # GURMUKHI LETTER I
    LineBreakClass::ULB_AL, // 0x0A08  # GURMUKHI LETTER II
    LineBreakClass::ULB_AL, // 0x0A09  # GURMUKHI LETTER U
    LineBreakClass::ULB_AL, // 0x0A0A  # GURMUKHI LETTER UU
    LineBreakClass::ULB_ID, // 0x0A0B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A0C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A0D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A0E # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0A0F  # GURMUKHI LETTER EE
    LineBreakClass::ULB_AL, // 0x0A10  # GURMUKHI LETTER AI
    LineBreakClass::ULB_ID, // 0x0A11 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A12 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0A13  # GURMUKHI LETTER OO
    LineBreakClass::ULB_AL, // 0x0A14  # GURMUKHI LETTER AU
    LineBreakClass::ULB_AL, // 0x0A15  # GURMUKHI LETTER KA
    LineBreakClass::ULB_AL, // 0x0A16  # GURMUKHI LETTER KHA
    LineBreakClass::ULB_AL, // 0x0A17  # GURMUKHI LETTER GA
    LineBreakClass::ULB_AL, // 0x0A18  # GURMUKHI LETTER GHA
    LineBreakClass::ULB_AL, // 0x0A19  # GURMUKHI LETTER NGA
    LineBreakClass::ULB_AL, // 0x0A1A  # GURMUKHI LETTER CA
    LineBreakClass::ULB_AL, // 0x0A1B  # GURMUKHI LETTER CHA
    LineBreakClass::ULB_AL, // 0x0A1C  # GURMUKHI LETTER JA
    LineBreakClass::ULB_AL, // 0x0A1D  # GURMUKHI LETTER JHA
    LineBreakClass::ULB_AL, // 0x0A1E  # GURMUKHI LETTER NYA
    LineBreakClass::ULB_AL, // 0x0A1F  # GURMUKHI LETTER TTA
    LineBreakClass::ULB_AL, // 0x0A20  # GURMUKHI LETTER TTHA
    LineBreakClass::ULB_AL, // 0x0A21  # GURMUKHI LETTER DDA
    LineBreakClass::ULB_AL, // 0x0A22  # GURMUKHI LETTER DDHA
    LineBreakClass::ULB_AL, // 0x0A23  # GURMUKHI LETTER NNA
    LineBreakClass::ULB_AL, // 0x0A24  # GURMUKHI LETTER TA
    LineBreakClass::ULB_AL, // 0x0A25  # GURMUKHI LETTER THA
    LineBreakClass::ULB_AL, // 0x0A26  # GURMUKHI LETTER DA
    LineBreakClass::ULB_AL, // 0x0A27  # GURMUKHI LETTER DHA
    LineBreakClass::ULB_AL, // 0x0A28  # GURMUKHI LETTER NA
    LineBreakClass::ULB_ID, // 0x0A29 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0A2A  # GURMUKHI LETTER PA
    LineBreakClass::ULB_AL, // 0x0A2B  # GURMUKHI LETTER PHA
    LineBreakClass::ULB_AL, // 0x0A2C  # GURMUKHI LETTER BA
    LineBreakClass::ULB_AL, // 0x0A2D  # GURMUKHI LETTER BHA
    LineBreakClass::ULB_AL, // 0x0A2E  # GURMUKHI LETTER MA
    LineBreakClass::ULB_AL, // 0x0A2F  # GURMUKHI LETTER YA
    LineBreakClass::ULB_AL, // 0x0A30  # GURMUKHI LETTER RA
    LineBreakClass::ULB_ID, // 0x0A31 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0A32  # GURMUKHI LETTER LA
    LineBreakClass::ULB_AL, // 0x0A33  # GURMUKHI LETTER LLA
    LineBreakClass::ULB_ID, // 0x0A34 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0A35  # GURMUKHI LETTER VA
    LineBreakClass::ULB_AL, // 0x0A36  # GURMUKHI LETTER SHA
    LineBreakClass::ULB_ID, // 0x0A37 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0A38  # GURMUKHI LETTER SA
    LineBreakClass::ULB_AL, // 0x0A39  # GURMUKHI LETTER HA
    LineBreakClass::ULB_ID, // 0x0A3A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A3B # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0A3C  # GURMUKHI SIGN NUKTA
    LineBreakClass::ULB_ID, // 0x0A3D # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0A3E  # GURMUKHI VOWEL SIGN AA
    LineBreakClass::ULB_CM, // 0x0A3F  # GURMUKHI VOWEL SIGN I
    LineBreakClass::ULB_CM, // 0x0A40  # GURMUKHI VOWEL SIGN II
    LineBreakClass::ULB_CM, // 0x0A41  # GURMUKHI VOWEL SIGN U
    LineBreakClass::ULB_CM, // 0x0A42  # GURMUKHI VOWEL SIGN UU
    LineBreakClass::ULB_ID, // 0x0A43 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A44 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A45 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A46 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0A47  # GURMUKHI VOWEL SIGN EE
    LineBreakClass::ULB_CM, // 0x0A48  # GURMUKHI VOWEL SIGN AI
    LineBreakClass::ULB_ID, // 0x0A49 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A4A # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0A4B  # GURMUKHI VOWEL SIGN OO
    LineBreakClass::ULB_CM, // 0x0A4C  # GURMUKHI VOWEL SIGN AU
    LineBreakClass::ULB_CM, // 0x0A4D  # GURMUKHI SIGN VIRAMA
    LineBreakClass::ULB_ID, // 0x0A4E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A4F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A50 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A51 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A52 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A53 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A54 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A55 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A56 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A57 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A58 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0A59  # GURMUKHI LETTER KHHA
    LineBreakClass::ULB_AL, // 0x0A5A  # GURMUKHI LETTER GHHA
    LineBreakClass::ULB_AL, // 0x0A5B  # GURMUKHI LETTER ZA
    LineBreakClass::ULB_AL, // 0x0A5C  # GURMUKHI LETTER RRA
    LineBreakClass::ULB_ID, // 0x0A5D # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0A5E  # GURMUKHI LETTER FA
    LineBreakClass::ULB_ID, // 0x0A5F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A60 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A61 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A62 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A63 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A64 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A65 # <UNDEFINED>
    LineBreakClass::ULB_NU, // 0x0A66  # GURMUKHI DIGIT ZERO
    LineBreakClass::ULB_NU, // 0x0A67  # GURMUKHI DIGIT ONE
    LineBreakClass::ULB_NU, // 0x0A68  # GURMUKHI DIGIT TWO
    LineBreakClass::ULB_NU, // 0x0A69  # GURMUKHI DIGIT THREE
    LineBreakClass::ULB_NU, // 0x0A6A  # GURMUKHI DIGIT FOUR
    LineBreakClass::ULB_NU, // 0x0A6B  # GURMUKHI DIGIT FIVE
    LineBreakClass::ULB_NU, // 0x0A6C  # GURMUKHI DIGIT SIX
    LineBreakClass::ULB_NU, // 0x0A6D  # GURMUKHI DIGIT SEVEN
    LineBreakClass::ULB_NU, // 0x0A6E  # GURMUKHI DIGIT EIGHT
    LineBreakClass::ULB_NU, // 0x0A6F  # GURMUKHI DIGIT NINE
    LineBreakClass::ULB_CM, // 0x0A70  # GURMUKHI TIPPI
    LineBreakClass::ULB_CM, // 0x0A71  # GURMUKHI ADDAK
    LineBreakClass::ULB_AL, // 0x0A72  # GURMUKHI IRI
    LineBreakClass::ULB_AL, // 0x0A73  # GURMUKHI URA
    LineBreakClass::ULB_AL, // 0x0A74  # GURMUKHI EK ONKAR
    LineBreakClass::ULB_ID, // 0x0A75 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A76 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A77 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A78 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A79 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A7A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A7B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A7C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A7D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A7E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A7F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0A80 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0A81  # GUJARATI SIGN CANDRABINDU
    LineBreakClass::ULB_CM, // 0x0A82  # GUJARATI SIGN ANUSVARA
    LineBreakClass::ULB_CM, // 0x0A83  # GUJARATI SIGN VISARGA
    LineBreakClass::ULB_ID, // 0x0A84 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0A85  # GUJARATI LETTER A
    LineBreakClass::ULB_AL, // 0x0A86  # GUJARATI LETTER AA
    LineBreakClass::ULB_AL, // 0x0A87  # GUJARATI LETTER I
    LineBreakClass::ULB_AL, // 0x0A88  # GUJARATI LETTER II
    LineBreakClass::ULB_AL, // 0x0A89  # GUJARATI LETTER U
    LineBreakClass::ULB_AL, // 0x0A8A  # GUJARATI LETTER UU
    LineBreakClass::ULB_AL, // 0x0A8B  # GUJARATI LETTER VOCALIC R
    LineBreakClass::ULB_AL, // 0x0A8C  # GUJARATI LETTER VOCALIC L
    LineBreakClass::ULB_AL, // 0x0A8D  # GUJARATI VOWEL CANDRA E
    LineBreakClass::ULB_ID, // 0x0A8E # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0A8F  # GUJARATI LETTER E
    LineBreakClass::ULB_AL, // 0x0A90  # GUJARATI LETTER AI
    LineBreakClass::ULB_AL, // 0x0A91  # GUJARATI VOWEL CANDRA O
    LineBreakClass::ULB_ID, // 0x0A92 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0A93  # GUJARATI LETTER O
    LineBreakClass::ULB_AL, // 0x0A94  # GUJARATI LETTER AU
    LineBreakClass::ULB_AL, // 0x0A95  # GUJARATI LETTER KA
    LineBreakClass::ULB_AL, // 0x0A96  # GUJARATI LETTER KHA
    LineBreakClass::ULB_AL, // 0x0A97  # GUJARATI LETTER GA
    LineBreakClass::ULB_AL, // 0x0A98  # GUJARATI LETTER GHA
    LineBreakClass::ULB_AL, // 0x0A99  # GUJARATI LETTER NGA
    LineBreakClass::ULB_AL, // 0x0A9A  # GUJARATI LETTER CA
    LineBreakClass::ULB_AL, // 0x0A9B  # GUJARATI LETTER CHA
    LineBreakClass::ULB_AL, // 0x0A9C  # GUJARATI LETTER JA
    LineBreakClass::ULB_AL, // 0x0A9D  # GUJARATI LETTER JHA
    LineBreakClass::ULB_AL, // 0x0A9E  # GUJARATI LETTER NYA
    LineBreakClass::ULB_AL, // 0x0A9F  # GUJARATI LETTER TTA
    LineBreakClass::ULB_AL, // 0x0AA0  # GUJARATI LETTER TTHA
    LineBreakClass::ULB_AL, // 0x0AA1  # GUJARATI LETTER DDA
    LineBreakClass::ULB_AL, // 0x0AA2  # GUJARATI LETTER DDHA
    LineBreakClass::ULB_AL, // 0x0AA3  # GUJARATI LETTER NNA
    LineBreakClass::ULB_AL, // 0x0AA4  # GUJARATI LETTER TA
    LineBreakClass::ULB_AL, // 0x0AA5  # GUJARATI LETTER THA
    LineBreakClass::ULB_AL, // 0x0AA6  # GUJARATI LETTER DA
    LineBreakClass::ULB_AL, // 0x0AA7  # GUJARATI LETTER DHA
    LineBreakClass::ULB_AL, // 0x0AA8  # GUJARATI LETTER NA
    LineBreakClass::ULB_ID, // 0x0AA9 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0AAA  # GUJARATI LETTER PA
    LineBreakClass::ULB_AL, // 0x0AAB  # GUJARATI LETTER PHA
    LineBreakClass::ULB_AL, // 0x0AAC  # GUJARATI LETTER BA
    LineBreakClass::ULB_AL, // 0x0AAD  # GUJARATI LETTER BHA
    LineBreakClass::ULB_AL, // 0x0AAE  # GUJARATI LETTER MA
    LineBreakClass::ULB_AL, // 0x0AAF  # GUJARATI LETTER YA
    LineBreakClass::ULB_AL, // 0x0AB0  # GUJARATI LETTER RA
    LineBreakClass::ULB_ID, // 0x0AB1 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0AB2  # GUJARATI LETTER LA
    LineBreakClass::ULB_AL, // 0x0AB3  # GUJARATI LETTER LLA
    LineBreakClass::ULB_ID, // 0x0AB4 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0AB5  # GUJARATI LETTER VA
    LineBreakClass::ULB_AL, // 0x0AB6  # GUJARATI LETTER SHA
    LineBreakClass::ULB_AL, // 0x0AB7  # GUJARATI LETTER SSA
    LineBreakClass::ULB_AL, // 0x0AB8  # GUJARATI LETTER SA
    LineBreakClass::ULB_AL, // 0x0AB9  # GUJARATI LETTER HA
    LineBreakClass::ULB_ID, // 0x0ABA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0ABB # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0ABC  # GUJARATI SIGN NUKTA
    LineBreakClass::ULB_AL, // 0x0ABD  # GUJARATI SIGN AVAGRAHA
    LineBreakClass::ULB_CM, // 0x0ABE  # GUJARATI VOWEL SIGN AA
    LineBreakClass::ULB_CM, // 0x0ABF  # GUJARATI VOWEL SIGN I
    LineBreakClass::ULB_CM, // 0x0AC0  # GUJARATI VOWEL SIGN II
    LineBreakClass::ULB_CM, // 0x0AC1  # GUJARATI VOWEL SIGN U
    LineBreakClass::ULB_CM, // 0x0AC2  # GUJARATI VOWEL SIGN UU
    LineBreakClass::ULB_CM, // 0x0AC3  # GUJARATI VOWEL SIGN VOCALIC R
    LineBreakClass::ULB_CM, // 0x0AC4  # GUJARATI VOWEL SIGN VOCALIC RR
    LineBreakClass::ULB_CM, // 0x0AC5  # GUJARATI VOWEL SIGN CANDRA E
    LineBreakClass::ULB_ID, // 0x0AC6 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0AC7  # GUJARATI VOWEL SIGN E
    LineBreakClass::ULB_CM, // 0x0AC8  # GUJARATI VOWEL SIGN AI
    LineBreakClass::ULB_CM, // 0x0AC9  # GUJARATI VOWEL SIGN CANDRA O
    LineBreakClass::ULB_ID, // 0x0ACA # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0ACB  # GUJARATI VOWEL SIGN O
    LineBreakClass::ULB_CM, // 0x0ACC  # GUJARATI VOWEL SIGN AU
    LineBreakClass::ULB_CM, // 0x0ACD  # GUJARATI SIGN VIRAMA
    LineBreakClass::ULB_ID, // 0x0ACE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0ACF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0AD0  # GUJARATI OM
    LineBreakClass::ULB_ID, // 0x0AD1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0AD2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0AD3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0AD4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0AD5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0AD6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0AD7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0AD8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0AD9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0ADA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0ADB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0ADC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0ADD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0ADE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0ADF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0AE0  # GUJARATI LETTER VOCALIC RR
    LineBreakClass::ULB_AL, // 0x0AE1  # GUJARATI LETTER VOCALIC LL
    LineBreakClass::ULB_CM, // 0x0AE2  # GUJARATI VOWEL SIGN VOCALIC L
    LineBreakClass::ULB_CM, // 0x0AE3  # GUJARATI VOWEL SIGN VOCALIC LL
    LineBreakClass::ULB_ID, // 0x0AE4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0AE5 # <UNDEFINED>
    LineBreakClass::ULB_NU, // 0x0AE6  # GUJARATI DIGIT ZERO
    LineBreakClass::ULB_NU, // 0x0AE7  # GUJARATI DIGIT ONE
    LineBreakClass::ULB_NU, // 0x0AE8  # GUJARATI DIGIT TWO
    LineBreakClass::ULB_NU, // 0x0AE9  # GUJARATI DIGIT THREE
    LineBreakClass::ULB_NU, // 0x0AEA  # GUJARATI DIGIT FOUR
    LineBreakClass::ULB_NU, // 0x0AEB  # GUJARATI DIGIT FIVE
    LineBreakClass::ULB_NU, // 0x0AEC  # GUJARATI DIGIT SIX
    LineBreakClass::ULB_NU, // 0x0AED  # GUJARATI DIGIT SEVEN
    LineBreakClass::ULB_NU, // 0x0AEE  # GUJARATI DIGIT EIGHT
    LineBreakClass::ULB_NU, // 0x0AEF  # GUJARATI DIGIT NINE
    LineBreakClass::ULB_ID, // 0x0AF0 # <UNDEFINED>
    LineBreakClass::ULB_PR, // 0x0AF1  # GUJARATI RUPEE SIGN
    LineBreakClass::ULB_ID, // 0x0AF2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0AF3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0AF4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0AF5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0AF6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0AF7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0AF8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0AF9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0AFA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0AFB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0AFC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0AFD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0AFE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0AFF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B00 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0B01  # ORIYA SIGN CANDRABINDU
    LineBreakClass::ULB_CM, // 0x0B02  # ORIYA SIGN ANUSVARA
    LineBreakClass::ULB_CM, // 0x0B03  # ORIYA SIGN VISARGA
    LineBreakClass::ULB_ID, // 0x0B04 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0B05  # ORIYA LETTER A
    LineBreakClass::ULB_AL, // 0x0B06  # ORIYA LETTER AA
    LineBreakClass::ULB_AL, // 0x0B07  # ORIYA LETTER I
    LineBreakClass::ULB_AL, // 0x0B08  # ORIYA LETTER II
    LineBreakClass::ULB_AL, // 0x0B09  # ORIYA LETTER U
    LineBreakClass::ULB_AL, // 0x0B0A  # ORIYA LETTER UU
    LineBreakClass::ULB_AL, // 0x0B0B  # ORIYA LETTER VOCALIC R
    LineBreakClass::ULB_AL, // 0x0B0C  # ORIYA LETTER VOCALIC L
    LineBreakClass::ULB_ID, // 0x0B0D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B0E # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0B0F  # ORIYA LETTER E
    LineBreakClass::ULB_AL, // 0x0B10  # ORIYA LETTER AI
    LineBreakClass::ULB_ID, // 0x0B11 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B12 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0B13  # ORIYA LETTER O
    LineBreakClass::ULB_AL, // 0x0B14  # ORIYA LETTER AU
    LineBreakClass::ULB_AL, // 0x0B15  # ORIYA LETTER KA
    LineBreakClass::ULB_AL, // 0x0B16  # ORIYA LETTER KHA
    LineBreakClass::ULB_AL, // 0x0B17  # ORIYA LETTER GA
    LineBreakClass::ULB_AL, // 0x0B18  # ORIYA LETTER GHA
    LineBreakClass::ULB_AL, // 0x0B19  # ORIYA LETTER NGA
    LineBreakClass::ULB_AL, // 0x0B1A  # ORIYA LETTER CA
    LineBreakClass::ULB_AL, // 0x0B1B  # ORIYA LETTER CHA
    LineBreakClass::ULB_AL, // 0x0B1C  # ORIYA LETTER JA
    LineBreakClass::ULB_AL, // 0x0B1D  # ORIYA LETTER JHA
    LineBreakClass::ULB_AL, // 0x0B1E  # ORIYA LETTER NYA
    LineBreakClass::ULB_AL, // 0x0B1F  # ORIYA LETTER TTA
    LineBreakClass::ULB_AL, // 0x0B20  # ORIYA LETTER TTHA
    LineBreakClass::ULB_AL, // 0x0B21  # ORIYA LETTER DDA
    LineBreakClass::ULB_AL, // 0x0B22  # ORIYA LETTER DDHA
    LineBreakClass::ULB_AL, // 0x0B23  # ORIYA LETTER NNA
    LineBreakClass::ULB_AL, // 0x0B24  # ORIYA LETTER TA
    LineBreakClass::ULB_AL, // 0x0B25  # ORIYA LETTER THA
    LineBreakClass::ULB_AL, // 0x0B26  # ORIYA LETTER DA
    LineBreakClass::ULB_AL, // 0x0B27  # ORIYA LETTER DHA
    LineBreakClass::ULB_AL, // 0x0B28  # ORIYA LETTER NA
    LineBreakClass::ULB_ID, // 0x0B29 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0B2A  # ORIYA LETTER PA
    LineBreakClass::ULB_AL, // 0x0B2B  # ORIYA LETTER PHA
    LineBreakClass::ULB_AL, // 0x0B2C  # ORIYA LETTER BA
    LineBreakClass::ULB_AL, // 0x0B2D  # ORIYA LETTER BHA
    LineBreakClass::ULB_AL, // 0x0B2E  # ORIYA LETTER MA
    LineBreakClass::ULB_AL, // 0x0B2F  # ORIYA LETTER YA
    LineBreakClass::ULB_AL, // 0x0B30  # ORIYA LETTER RA
    LineBreakClass::ULB_ID, // 0x0B31 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0B32  # ORIYA LETTER LA
    LineBreakClass::ULB_AL, // 0x0B33  # ORIYA LETTER LLA
    LineBreakClass::ULB_ID, // 0x0B34 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0B35  # ORIYA LETTER VA
    LineBreakClass::ULB_AL, // 0x0B36  # ORIYA LETTER SHA
    LineBreakClass::ULB_AL, // 0x0B37  # ORIYA LETTER SSA
    LineBreakClass::ULB_AL, // 0x0B38  # ORIYA LETTER SA
    LineBreakClass::ULB_AL, // 0x0B39  # ORIYA LETTER HA
    LineBreakClass::ULB_ID, // 0x0B3A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B3B # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0B3C  # ORIYA SIGN NUKTA
    LineBreakClass::ULB_AL, // 0x0B3D  # ORIYA SIGN AVAGRAHA
    LineBreakClass::ULB_CM, // 0x0B3E  # ORIYA VOWEL SIGN AA
    LineBreakClass::ULB_CM, // 0x0B3F  # ORIYA VOWEL SIGN I
    LineBreakClass::ULB_CM, // 0x0B40  # ORIYA VOWEL SIGN II
    LineBreakClass::ULB_CM, // 0x0B41  # ORIYA VOWEL SIGN U
    LineBreakClass::ULB_CM, // 0x0B42  # ORIYA VOWEL SIGN UU
    LineBreakClass::ULB_CM, // 0x0B43  # ORIYA VOWEL SIGN VOCALIC R
    LineBreakClass::ULB_ID, // 0x0B44 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B45 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B46 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0B47  # ORIYA VOWEL SIGN E
    LineBreakClass::ULB_CM, // 0x0B48  # ORIYA VOWEL SIGN AI
    LineBreakClass::ULB_ID, // 0x0B49 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B4A # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0B4B  # ORIYA VOWEL SIGN O
    LineBreakClass::ULB_CM, // 0x0B4C  # ORIYA VOWEL SIGN AU
    LineBreakClass::ULB_CM, // 0x0B4D  # ORIYA SIGN VIRAMA
    LineBreakClass::ULB_ID, // 0x0B4E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B4F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B50 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B51 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B52 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B53 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B54 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B55 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0B56  # ORIYA AI LENGTH MARK
    LineBreakClass::ULB_CM, // 0x0B57  # ORIYA AU LENGTH MARK
    LineBreakClass::ULB_ID, // 0x0B58 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B59 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B5A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B5B # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0B5C  # ORIYA LETTER RRA
    LineBreakClass::ULB_AL, // 0x0B5D  # ORIYA LETTER RHA
    LineBreakClass::ULB_ID, // 0x0B5E # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0B5F  # ORIYA LETTER YYA
    LineBreakClass::ULB_AL, // 0x0B60  # ORIYA LETTER VOCALIC RR
    LineBreakClass::ULB_AL, // 0x0B61  # ORIYA LETTER VOCALIC LL
    LineBreakClass::ULB_ID, // 0x0B62 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B63 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B64 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B65 # <UNDEFINED>
    LineBreakClass::ULB_NU, // 0x0B66  # ORIYA DIGIT ZERO
    LineBreakClass::ULB_NU, // 0x0B67  # ORIYA DIGIT ONE
    LineBreakClass::ULB_NU, // 0x0B68  # ORIYA DIGIT TWO
    LineBreakClass::ULB_NU, // 0x0B69  # ORIYA DIGIT THREE
    LineBreakClass::ULB_NU, // 0x0B6A  # ORIYA DIGIT FOUR
    LineBreakClass::ULB_NU, // 0x0B6B  # ORIYA DIGIT FIVE
    LineBreakClass::ULB_NU, // 0x0B6C  # ORIYA DIGIT SIX
    LineBreakClass::ULB_NU, // 0x0B6D  # ORIYA DIGIT SEVEN
    LineBreakClass::ULB_NU, // 0x0B6E  # ORIYA DIGIT EIGHT
    LineBreakClass::ULB_NU, // 0x0B6F  # ORIYA DIGIT NINE
    LineBreakClass::ULB_AL, // 0x0B70  # ORIYA ISSHAR
    LineBreakClass::ULB_AL, // 0x0B71  # ORIYA LETTER WA
    LineBreakClass::ULB_ID, // 0x0B72 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B73 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B74 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B75 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B76 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B77 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B78 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B79 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B7A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B7B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B7C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B7D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B7E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B7F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B80 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B81 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0B82  # TAMIL SIGN ANUSVARA
    LineBreakClass::ULB_AL, // 0x0B83  # TAMIL SIGN VISARGA
    LineBreakClass::ULB_ID, // 0x0B84 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0B85  # TAMIL LETTER A
    LineBreakClass::ULB_AL, // 0x0B86  # TAMIL LETTER AA
    LineBreakClass::ULB_AL, // 0x0B87  # TAMIL LETTER I
    LineBreakClass::ULB_AL, // 0x0B88  # TAMIL LETTER II
    LineBreakClass::ULB_AL, // 0x0B89  # TAMIL LETTER U
    LineBreakClass::ULB_AL, // 0x0B8A  # TAMIL LETTER UU
    LineBreakClass::ULB_ID, // 0x0B8B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B8C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B8D # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0B8E  # TAMIL LETTER E
    LineBreakClass::ULB_AL, // 0x0B8F  # TAMIL LETTER EE
    LineBreakClass::ULB_AL, // 0x0B90  # TAMIL LETTER AI
    LineBreakClass::ULB_ID, // 0x0B91 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0B92  # TAMIL LETTER O
    LineBreakClass::ULB_AL, // 0x0B93  # TAMIL LETTER OO
    LineBreakClass::ULB_AL, // 0x0B94  # TAMIL LETTER AU
    LineBreakClass::ULB_AL, // 0x0B95  # TAMIL LETTER KA
    LineBreakClass::ULB_ID, // 0x0B96 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B97 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0B98 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0B99  # TAMIL LETTER NGA
    LineBreakClass::ULB_AL, // 0x0B9A  # TAMIL LETTER CA
    LineBreakClass::ULB_ID, // 0x0B9B # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0B9C  # TAMIL LETTER JA
    LineBreakClass::ULB_ID, // 0x0B9D # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0B9E  # TAMIL LETTER NYA
    LineBreakClass::ULB_AL, // 0x0B9F  # TAMIL LETTER TTA
    LineBreakClass::ULB_ID, // 0x0BA0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BA1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BA2 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0BA3  # TAMIL LETTER NNA
    LineBreakClass::ULB_AL, // 0x0BA4  # TAMIL LETTER TA
    LineBreakClass::ULB_ID, // 0x0BA5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BA6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BA7 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0BA8  # TAMIL LETTER NA
    LineBreakClass::ULB_AL, // 0x0BA9  # TAMIL LETTER NNNA
    LineBreakClass::ULB_AL, // 0x0BAA  # TAMIL LETTER PA
    LineBreakClass::ULB_ID, // 0x0BAB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BAC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BAD # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0BAE  # TAMIL LETTER MA
    LineBreakClass::ULB_AL, // 0x0BAF  # TAMIL LETTER YA
    LineBreakClass::ULB_AL, // 0x0BB0  # TAMIL LETTER RA
    LineBreakClass::ULB_AL, // 0x0BB1  # TAMIL LETTER RRA
    LineBreakClass::ULB_AL, // 0x0BB2  # TAMIL LETTER LA
    LineBreakClass::ULB_AL, // 0x0BB3  # TAMIL LETTER LLA
    LineBreakClass::ULB_AL, // 0x0BB4  # TAMIL LETTER LLLA
    LineBreakClass::ULB_AL, // 0x0BB5  # TAMIL LETTER VA
    LineBreakClass::ULB_AL, // 0x0BB6  # TAMIL LETTER SHA
    LineBreakClass::ULB_AL, // 0x0BB7  # TAMIL LETTER SSA
    LineBreakClass::ULB_AL, // 0x0BB8  # TAMIL LETTER SA
    LineBreakClass::ULB_AL, // 0x0BB9  # TAMIL LETTER HA
    LineBreakClass::ULB_ID, // 0x0BBA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BBB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BBC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BBD # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0BBE  # TAMIL VOWEL SIGN AA
    LineBreakClass::ULB_CM, // 0x0BBF  # TAMIL VOWEL SIGN I
    LineBreakClass::ULB_CM, // 0x0BC0  # TAMIL VOWEL SIGN II
    LineBreakClass::ULB_CM, // 0x0BC1  # TAMIL VOWEL SIGN U
    LineBreakClass::ULB_CM, // 0x0BC2  # TAMIL VOWEL SIGN UU
    LineBreakClass::ULB_ID, // 0x0BC3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BC4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BC5 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0BC6  # TAMIL VOWEL SIGN E
    LineBreakClass::ULB_CM, // 0x0BC7  # TAMIL VOWEL SIGN EE
    LineBreakClass::ULB_CM, // 0x0BC8  # TAMIL VOWEL SIGN AI
    LineBreakClass::ULB_ID, // 0x0BC9 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0BCA  # TAMIL VOWEL SIGN O
    LineBreakClass::ULB_CM, // 0x0BCB  # TAMIL VOWEL SIGN OO
    LineBreakClass::ULB_CM, // 0x0BCC  # TAMIL VOWEL SIGN AU
    LineBreakClass::ULB_CM, // 0x0BCD  # TAMIL SIGN VIRAMA
    LineBreakClass::ULB_ID, // 0x0BCE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BCF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BD0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BD1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BD2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BD3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BD4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BD5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BD6 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0BD7  # TAMIL AU LENGTH MARK
    LineBreakClass::ULB_ID, // 0x0BD8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BD9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BDA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BDB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BDC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BDD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BDE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BDF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BE0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BE1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BE2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BE3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BE4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BE5 # <UNDEFINED>
    LineBreakClass::ULB_NU, // 0x0BE6  # TAMIL DIGIT ZERO
    LineBreakClass::ULB_NU, // 0x0BE7  # TAMIL DIGIT ONE
    LineBreakClass::ULB_NU, // 0x0BE8  # TAMIL DIGIT TWO
    LineBreakClass::ULB_NU, // 0x0BE9  # TAMIL DIGIT THREE
    LineBreakClass::ULB_NU, // 0x0BEA  # TAMIL DIGIT FOUR
    LineBreakClass::ULB_NU, // 0x0BEB  # TAMIL DIGIT FIVE
    LineBreakClass::ULB_NU, // 0x0BEC  # TAMIL DIGIT SIX
    LineBreakClass::ULB_NU, // 0x0BED  # TAMIL DIGIT SEVEN
    LineBreakClass::ULB_NU, // 0x0BEE  # TAMIL DIGIT EIGHT
    LineBreakClass::ULB_NU, // 0x0BEF  # TAMIL DIGIT NINE
    LineBreakClass::ULB_AL, // 0x0BF0  # TAMIL NUMBER TEN
    LineBreakClass::ULB_AL, // 0x0BF1  # TAMIL NUMBER ONE HUNDRED
    LineBreakClass::ULB_AL, // 0x0BF2  # TAMIL NUMBER ONE THOUSAND
    LineBreakClass::ULB_AL, // 0x0BF3  # TAMIL DAY SIGN
    LineBreakClass::ULB_AL, // 0x0BF4  # TAMIL MONTH SIGN
    LineBreakClass::ULB_AL, // 0x0BF5  # TAMIL YEAR SIGN
    LineBreakClass::ULB_AL, // 0x0BF6  # TAMIL DEBIT SIGN
    LineBreakClass::ULB_AL, // 0x0BF7  # TAMIL CREDIT SIGN
    LineBreakClass::ULB_AL, // 0x0BF8  # TAMIL AS ABOVE SIGN
    LineBreakClass::ULB_PR, // 0x0BF9  # TAMIL RUPEE SIGN
    LineBreakClass::ULB_AL, // 0x0BFA  # TAMIL NUMBER SIGN
    LineBreakClass::ULB_ID, // 0x0BFB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BFC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BFD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BFE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0BFF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C00 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0C01  # TELUGU SIGN CANDRABINDU
    LineBreakClass::ULB_CM, // 0x0C02  # TELUGU SIGN ANUSVARA
    LineBreakClass::ULB_CM, // 0x0C03  # TELUGU SIGN VISARGA
    LineBreakClass::ULB_ID, // 0x0C04 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0C05  # TELUGU LETTER A
    LineBreakClass::ULB_AL, // 0x0C06  # TELUGU LETTER AA
    LineBreakClass::ULB_AL, // 0x0C07  # TELUGU LETTER I
    LineBreakClass::ULB_AL, // 0x0C08  # TELUGU LETTER II
    LineBreakClass::ULB_AL, // 0x0C09  # TELUGU LETTER U
    LineBreakClass::ULB_AL, // 0x0C0A  # TELUGU LETTER UU
    LineBreakClass::ULB_AL, // 0x0C0B  # TELUGU LETTER VOCALIC R
    LineBreakClass::ULB_AL, // 0x0C0C  # TELUGU LETTER VOCALIC L
    LineBreakClass::ULB_ID, // 0x0C0D # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0C0E  # TELUGU LETTER E
    LineBreakClass::ULB_AL, // 0x0C0F  # TELUGU LETTER EE
    LineBreakClass::ULB_AL, // 0x0C10  # TELUGU LETTER AI
    LineBreakClass::ULB_ID, // 0x0C11 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0C12  # TELUGU LETTER O
    LineBreakClass::ULB_AL, // 0x0C13  # TELUGU LETTER OO
    LineBreakClass::ULB_AL, // 0x0C14  # TELUGU LETTER AU
    LineBreakClass::ULB_AL, // 0x0C15  # TELUGU LETTER KA
    LineBreakClass::ULB_AL, // 0x0C16  # TELUGU LETTER KHA
    LineBreakClass::ULB_AL, // 0x0C17  # TELUGU LETTER GA
    LineBreakClass::ULB_AL, // 0x0C18  # TELUGU LETTER GHA
    LineBreakClass::ULB_AL, // 0x0C19  # TELUGU LETTER NGA
    LineBreakClass::ULB_AL, // 0x0C1A  # TELUGU LETTER CA
    LineBreakClass::ULB_AL, // 0x0C1B  # TELUGU LETTER CHA
    LineBreakClass::ULB_AL, // 0x0C1C  # TELUGU LETTER JA
    LineBreakClass::ULB_AL, // 0x0C1D  # TELUGU LETTER JHA
    LineBreakClass::ULB_AL, // 0x0C1E  # TELUGU LETTER NYA
    LineBreakClass::ULB_AL, // 0x0C1F  # TELUGU LETTER TTA
    LineBreakClass::ULB_AL, // 0x0C20  # TELUGU LETTER TTHA
    LineBreakClass::ULB_AL, // 0x0C21  # TELUGU LETTER DDA
    LineBreakClass::ULB_AL, // 0x0C22  # TELUGU LETTER DDHA
    LineBreakClass::ULB_AL, // 0x0C23  # TELUGU LETTER NNA
    LineBreakClass::ULB_AL, // 0x0C24  # TELUGU LETTER TA
    LineBreakClass::ULB_AL, // 0x0C25  # TELUGU LETTER THA
    LineBreakClass::ULB_AL, // 0x0C26  # TELUGU LETTER DA
    LineBreakClass::ULB_AL, // 0x0C27  # TELUGU LETTER DHA
    LineBreakClass::ULB_AL, // 0x0C28  # TELUGU LETTER NA
    LineBreakClass::ULB_ID, // 0x0C29 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0C2A  # TELUGU LETTER PA
    LineBreakClass::ULB_AL, // 0x0C2B  # TELUGU LETTER PHA
    LineBreakClass::ULB_AL, // 0x0C2C  # TELUGU LETTER BA
    LineBreakClass::ULB_AL, // 0x0C2D  # TELUGU LETTER BHA
    LineBreakClass::ULB_AL, // 0x0C2E  # TELUGU LETTER MA
    LineBreakClass::ULB_AL, // 0x0C2F  # TELUGU LETTER YA
    LineBreakClass::ULB_AL, // 0x0C30  # TELUGU LETTER RA
    LineBreakClass::ULB_AL, // 0x0C31  # TELUGU LETTER RRA
    LineBreakClass::ULB_AL, // 0x0C32  # TELUGU LETTER LA
    LineBreakClass::ULB_AL, // 0x0C33  # TELUGU LETTER LLA
    LineBreakClass::ULB_ID, // 0x0C34 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0C35  # TELUGU LETTER VA
    LineBreakClass::ULB_AL, // 0x0C36  # TELUGU LETTER SHA
    LineBreakClass::ULB_AL, // 0x0C37  # TELUGU LETTER SSA
    LineBreakClass::ULB_AL, // 0x0C38  # TELUGU LETTER SA
    LineBreakClass::ULB_AL, // 0x0C39  # TELUGU LETTER HA
    LineBreakClass::ULB_ID, // 0x0C3A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C3B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C3C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C3D # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0C3E  # TELUGU VOWEL SIGN AA
    LineBreakClass::ULB_CM, // 0x0C3F  # TELUGU VOWEL SIGN I
    LineBreakClass::ULB_CM, // 0x0C40  # TELUGU VOWEL SIGN II
    LineBreakClass::ULB_CM, // 0x0C41  # TELUGU VOWEL SIGN U
    LineBreakClass::ULB_CM, // 0x0C42  # TELUGU VOWEL SIGN UU
    LineBreakClass::ULB_CM, // 0x0C43  # TELUGU VOWEL SIGN VOCALIC R
    LineBreakClass::ULB_CM, // 0x0C44  # TELUGU VOWEL SIGN VOCALIC RR
    LineBreakClass::ULB_ID, // 0x0C45 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0C46  # TELUGU VOWEL SIGN E
    LineBreakClass::ULB_CM, // 0x0C47  # TELUGU VOWEL SIGN EE
    LineBreakClass::ULB_CM, // 0x0C48  # TELUGU VOWEL SIGN AI
    LineBreakClass::ULB_ID, // 0x0C49 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0C4A  # TELUGU VOWEL SIGN O
    LineBreakClass::ULB_CM, // 0x0C4B  # TELUGU VOWEL SIGN OO
    LineBreakClass::ULB_CM, // 0x0C4C  # TELUGU VOWEL SIGN AU
    LineBreakClass::ULB_CM, // 0x0C4D  # TELUGU SIGN VIRAMA
    LineBreakClass::ULB_ID, // 0x0C4E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C4F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C50 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C51 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C52 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C53 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C54 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0C55  # TELUGU LENGTH MARK
    LineBreakClass::ULB_CM, // 0x0C56  # TELUGU AI LENGTH MARK
    LineBreakClass::ULB_ID, // 0x0C57 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C58 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C59 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C5A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C5B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C5C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C5D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C5E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C5F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0C60  # TELUGU LETTER VOCALIC RR
    LineBreakClass::ULB_AL, // 0x0C61  # TELUGU LETTER VOCALIC LL
    LineBreakClass::ULB_ID, // 0x0C62 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C63 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C64 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C65 # <UNDEFINED>
    LineBreakClass::ULB_NU, // 0x0C66  # TELUGU DIGIT ZERO
    LineBreakClass::ULB_NU, // 0x0C67  # TELUGU DIGIT ONE
    LineBreakClass::ULB_NU, // 0x0C68  # TELUGU DIGIT TWO
    LineBreakClass::ULB_NU, // 0x0C69  # TELUGU DIGIT THREE
    LineBreakClass::ULB_NU, // 0x0C6A  # TELUGU DIGIT FOUR
    LineBreakClass::ULB_NU, // 0x0C6B  # TELUGU DIGIT FIVE
    LineBreakClass::ULB_NU, // 0x0C6C  # TELUGU DIGIT SIX
    LineBreakClass::ULB_NU, // 0x0C6D  # TELUGU DIGIT SEVEN
    LineBreakClass::ULB_NU, // 0x0C6E  # TELUGU DIGIT EIGHT
    LineBreakClass::ULB_NU, // 0x0C6F  # TELUGU DIGIT NINE
    LineBreakClass::ULB_ID, // 0x0C70 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C71 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C72 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C73 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C74 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C75 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C76 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C77 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C78 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C79 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C7A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C7B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C7C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C7D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C7E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C7F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C80 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0C81 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0C82  # KANNADA SIGN ANUSVARA
    LineBreakClass::ULB_CM, // 0x0C83  # KANNADA SIGN VISARGA
    LineBreakClass::ULB_ID, // 0x0C84 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0C85  # KANNADA LETTER A
    LineBreakClass::ULB_AL, // 0x0C86  # KANNADA LETTER AA
    LineBreakClass::ULB_AL, // 0x0C87  # KANNADA LETTER I
    LineBreakClass::ULB_AL, // 0x0C88  # KANNADA LETTER II
    LineBreakClass::ULB_AL, // 0x0C89  # KANNADA LETTER U
    LineBreakClass::ULB_AL, // 0x0C8A  # KANNADA LETTER UU
    LineBreakClass::ULB_AL, // 0x0C8B  # KANNADA LETTER VOCALIC R
    LineBreakClass::ULB_AL, // 0x0C8C  # KANNADA LETTER VOCALIC L
    LineBreakClass::ULB_ID, // 0x0C8D # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0C8E  # KANNADA LETTER E
    LineBreakClass::ULB_AL, // 0x0C8F  # KANNADA LETTER EE
    LineBreakClass::ULB_AL, // 0x0C90  # KANNADA LETTER AI
    LineBreakClass::ULB_ID, // 0x0C91 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0C92  # KANNADA LETTER O
    LineBreakClass::ULB_AL, // 0x0C93  # KANNADA LETTER OO
    LineBreakClass::ULB_AL, // 0x0C94  # KANNADA LETTER AU
    LineBreakClass::ULB_AL, // 0x0C95  # KANNADA LETTER KA
    LineBreakClass::ULB_AL, // 0x0C96  # KANNADA LETTER KHA
    LineBreakClass::ULB_AL, // 0x0C97  # KANNADA LETTER GA
    LineBreakClass::ULB_AL, // 0x0C98  # KANNADA LETTER GHA
    LineBreakClass::ULB_AL, // 0x0C99  # KANNADA LETTER NGA
    LineBreakClass::ULB_AL, // 0x0C9A  # KANNADA LETTER CA
    LineBreakClass::ULB_AL, // 0x0C9B  # KANNADA LETTER CHA
    LineBreakClass::ULB_AL, // 0x0C9C  # KANNADA LETTER JA
    LineBreakClass::ULB_AL, // 0x0C9D  # KANNADA LETTER JHA
    LineBreakClass::ULB_AL, // 0x0C9E  # KANNADA LETTER NYA
    LineBreakClass::ULB_AL, // 0x0C9F  # KANNADA LETTER TTA
    LineBreakClass::ULB_AL, // 0x0CA0  # KANNADA LETTER TTHA
    LineBreakClass::ULB_AL, // 0x0CA1  # KANNADA LETTER DDA
    LineBreakClass::ULB_AL, // 0x0CA2  # KANNADA LETTER DDHA
    LineBreakClass::ULB_AL, // 0x0CA3  # KANNADA LETTER NNA
    LineBreakClass::ULB_AL, // 0x0CA4  # KANNADA LETTER TA
    LineBreakClass::ULB_AL, // 0x0CA5  # KANNADA LETTER THA
    LineBreakClass::ULB_AL, // 0x0CA6  # KANNADA LETTER DA
    LineBreakClass::ULB_AL, // 0x0CA7  # KANNADA LETTER DHA
    LineBreakClass::ULB_AL, // 0x0CA8  # KANNADA LETTER NA
    LineBreakClass::ULB_ID, // 0x0CA9 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0CAA  # KANNADA LETTER PA
    LineBreakClass::ULB_AL, // 0x0CAB  # KANNADA LETTER PHA
    LineBreakClass::ULB_AL, // 0x0CAC  # KANNADA LETTER BA
    LineBreakClass::ULB_AL, // 0x0CAD  # KANNADA LETTER BHA
    LineBreakClass::ULB_AL, // 0x0CAE  # KANNADA LETTER MA
    LineBreakClass::ULB_AL, // 0x0CAF  # KANNADA LETTER YA
    LineBreakClass::ULB_AL, // 0x0CB0  # KANNADA LETTER RA
    LineBreakClass::ULB_AL, // 0x0CB1  # KANNADA LETTER RRA
    LineBreakClass::ULB_AL, // 0x0CB2  # KANNADA LETTER LA
    LineBreakClass::ULB_AL, // 0x0CB3  # KANNADA LETTER LLA
    LineBreakClass::ULB_ID, // 0x0CB4 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0CB5  # KANNADA LETTER VA
    LineBreakClass::ULB_AL, // 0x0CB6  # KANNADA LETTER SHA
    LineBreakClass::ULB_AL, // 0x0CB7  # KANNADA LETTER SSA
    LineBreakClass::ULB_AL, // 0x0CB8  # KANNADA LETTER SA
    LineBreakClass::ULB_AL, // 0x0CB9  # KANNADA LETTER HA
    LineBreakClass::ULB_ID, // 0x0CBA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CBB # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0CBC  # KANNADA SIGN NUKTA
    LineBreakClass::ULB_AL, // 0x0CBD  # KANNADA SIGN AVAGRAHA
    LineBreakClass::ULB_CM, // 0x0CBE  # KANNADA VOWEL SIGN AA
    LineBreakClass::ULB_CM, // 0x0CBF  # KANNADA VOWEL SIGN I
    LineBreakClass::ULB_CM, // 0x0CC0  # KANNADA VOWEL SIGN II
    LineBreakClass::ULB_CM, // 0x0CC1  # KANNADA VOWEL SIGN U
    LineBreakClass::ULB_CM, // 0x0CC2  # KANNADA VOWEL SIGN UU
    LineBreakClass::ULB_CM, // 0x0CC3  # KANNADA VOWEL SIGN VOCALIC R
    LineBreakClass::ULB_CM, // 0x0CC4  # KANNADA VOWEL SIGN VOCALIC RR
    LineBreakClass::ULB_ID, // 0x0CC5 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0CC6  # KANNADA VOWEL SIGN E
    LineBreakClass::ULB_CM, // 0x0CC7  # KANNADA VOWEL SIGN EE
    LineBreakClass::ULB_CM, // 0x0CC8  # KANNADA VOWEL SIGN AI
    LineBreakClass::ULB_ID, // 0x0CC9 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0CCA  # KANNADA VOWEL SIGN O
    LineBreakClass::ULB_CM, // 0x0CCB  # KANNADA VOWEL SIGN OO
    LineBreakClass::ULB_CM, // 0x0CCC  # KANNADA VOWEL SIGN AU
    LineBreakClass::ULB_CM, // 0x0CCD  # KANNADA SIGN VIRAMA
    LineBreakClass::ULB_ID, // 0x0CCE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CCF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CD0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CD1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CD2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CD3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CD4 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0CD5  # KANNADA LENGTH MARK
    LineBreakClass::ULB_CM, // 0x0CD6  # KANNADA AI LENGTH MARK
    LineBreakClass::ULB_ID, // 0x0CD7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CD8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CD9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CDA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CDB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CDC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CDD # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0CDE  # KANNADA LETTER FA
    LineBreakClass::ULB_ID, // 0x0CDF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0CE0  # KANNADA LETTER VOCALIC RR
    LineBreakClass::ULB_AL, // 0x0CE1  # KANNADA LETTER VOCALIC LL
    LineBreakClass::ULB_CM, // 0x0CE2  # KANNADA VOWEL SIGN VOCALIC L
    LineBreakClass::ULB_CM, // 0x0CE3  # KANNADA VOWEL SIGN VOCALIC LL
    LineBreakClass::ULB_ID, // 0x0CE4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CE5 # <UNDEFINED>
    LineBreakClass::ULB_NU, // 0x0CE6  # KANNADA DIGIT ZERO
    LineBreakClass::ULB_NU, // 0x0CE7  # KANNADA DIGIT ONE
    LineBreakClass::ULB_NU, // 0x0CE8  # KANNADA DIGIT TWO
    LineBreakClass::ULB_NU, // 0x0CE9  # KANNADA DIGIT THREE
    LineBreakClass::ULB_NU, // 0x0CEA  # KANNADA DIGIT FOUR
    LineBreakClass::ULB_NU, // 0x0CEB  # KANNADA DIGIT FIVE
    LineBreakClass::ULB_NU, // 0x0CEC  # KANNADA DIGIT SIX
    LineBreakClass::ULB_NU, // 0x0CED  # KANNADA DIGIT SEVEN
    LineBreakClass::ULB_NU, // 0x0CEE  # KANNADA DIGIT EIGHT
    LineBreakClass::ULB_NU, // 0x0CEF  # KANNADA DIGIT NINE
    LineBreakClass::ULB_ID, // 0x0CF0 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0CF1  # KANNADA SIGN JIHVAMULIYA
    LineBreakClass::ULB_AL, // 0x0CF2  # KANNADA SIGN UPADHMANIYA
    LineBreakClass::ULB_ID, // 0x0CF3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CF4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CF5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CF6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CF7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CF8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CF9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CFA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CFB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CFC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CFD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CFE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0CFF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D00 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D01 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0D02  # MALAYALAM SIGN ANUSVARA
    LineBreakClass::ULB_CM, // 0x0D03  # MALAYALAM SIGN VISARGA
    LineBreakClass::ULB_ID, // 0x0D04 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0D05  # MALAYALAM LETTER A
    LineBreakClass::ULB_AL, // 0x0D06  # MALAYALAM LETTER AA
    LineBreakClass::ULB_AL, // 0x0D07  # MALAYALAM LETTER I
    LineBreakClass::ULB_AL, // 0x0D08  # MALAYALAM LETTER II
    LineBreakClass::ULB_AL, // 0x0D09  # MALAYALAM LETTER U
    LineBreakClass::ULB_AL, // 0x0D0A  # MALAYALAM LETTER UU
    LineBreakClass::ULB_AL, // 0x0D0B  # MALAYALAM LETTER VOCALIC R
    LineBreakClass::ULB_AL, // 0x0D0C  # MALAYALAM LETTER VOCALIC L
    LineBreakClass::ULB_ID, // 0x0D0D # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0D0E  # MALAYALAM LETTER E
    LineBreakClass::ULB_AL, // 0x0D0F  # MALAYALAM LETTER EE
    LineBreakClass::ULB_AL, // 0x0D10  # MALAYALAM LETTER AI
    LineBreakClass::ULB_ID, // 0x0D11 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0D12  # MALAYALAM LETTER O
    LineBreakClass::ULB_AL, // 0x0D13  # MALAYALAM LETTER OO
    LineBreakClass::ULB_AL, // 0x0D14  # MALAYALAM LETTER AU
    LineBreakClass::ULB_AL, // 0x0D15  # MALAYALAM LETTER KA
    LineBreakClass::ULB_AL, // 0x0D16  # MALAYALAM LETTER KHA
    LineBreakClass::ULB_AL, // 0x0D17  # MALAYALAM LETTER GA
    LineBreakClass::ULB_AL, // 0x0D18  # MALAYALAM LETTER GHA
    LineBreakClass::ULB_AL, // 0x0D19  # MALAYALAM LETTER NGA
    LineBreakClass::ULB_AL, // 0x0D1A  # MALAYALAM LETTER CA
    LineBreakClass::ULB_AL, // 0x0D1B  # MALAYALAM LETTER CHA
    LineBreakClass::ULB_AL, // 0x0D1C  # MALAYALAM LETTER JA
    LineBreakClass::ULB_AL, // 0x0D1D  # MALAYALAM LETTER JHA
    LineBreakClass::ULB_AL, // 0x0D1E  # MALAYALAM LETTER NYA
    LineBreakClass::ULB_AL, // 0x0D1F  # MALAYALAM LETTER TTA
    LineBreakClass::ULB_AL, // 0x0D20  # MALAYALAM LETTER TTHA
    LineBreakClass::ULB_AL, // 0x0D21  # MALAYALAM LETTER DDA
    LineBreakClass::ULB_AL, // 0x0D22  # MALAYALAM LETTER DDHA
    LineBreakClass::ULB_AL, // 0x0D23  # MALAYALAM LETTER NNA
    LineBreakClass::ULB_AL, // 0x0D24  # MALAYALAM LETTER TA
    LineBreakClass::ULB_AL, // 0x0D25  # MALAYALAM LETTER THA
    LineBreakClass::ULB_AL, // 0x0D26  # MALAYALAM LETTER DA
    LineBreakClass::ULB_AL, // 0x0D27  # MALAYALAM LETTER DHA
    LineBreakClass::ULB_AL, // 0x0D28  # MALAYALAM LETTER NA
    LineBreakClass::ULB_ID, // 0x0D29 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0D2A  # MALAYALAM LETTER PA
    LineBreakClass::ULB_AL, // 0x0D2B  # MALAYALAM LETTER PHA
    LineBreakClass::ULB_AL, // 0x0D2C  # MALAYALAM LETTER BA
    LineBreakClass::ULB_AL, // 0x0D2D  # MALAYALAM LETTER BHA
    LineBreakClass::ULB_AL, // 0x0D2E  # MALAYALAM LETTER MA
    LineBreakClass::ULB_AL, // 0x0D2F  # MALAYALAM LETTER YA
    LineBreakClass::ULB_AL, // 0x0D30  # MALAYALAM LETTER RA
    LineBreakClass::ULB_AL, // 0x0D31  # MALAYALAM LETTER RRA
    LineBreakClass::ULB_AL, // 0x0D32  # MALAYALAM LETTER LA
    LineBreakClass::ULB_AL, // 0x0D33  # MALAYALAM LETTER LLA
    LineBreakClass::ULB_AL, // 0x0D34  # MALAYALAM LETTER LLLA
    LineBreakClass::ULB_AL, // 0x0D35  # MALAYALAM LETTER VA
    LineBreakClass::ULB_AL, // 0x0D36  # MALAYALAM LETTER SHA
    LineBreakClass::ULB_AL, // 0x0D37  # MALAYALAM LETTER SSA
    LineBreakClass::ULB_AL, // 0x0D38  # MALAYALAM LETTER SA
    LineBreakClass::ULB_AL, // 0x0D39  # MALAYALAM LETTER HA
    LineBreakClass::ULB_ID, // 0x0D3A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D3B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D3C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D3D # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0D3E  # MALAYALAM VOWEL SIGN AA
    LineBreakClass::ULB_CM, // 0x0D3F  # MALAYALAM VOWEL SIGN I
    LineBreakClass::ULB_CM, // 0x0D40  # MALAYALAM VOWEL SIGN II
    LineBreakClass::ULB_CM, // 0x0D41  # MALAYALAM VOWEL SIGN U
    LineBreakClass::ULB_CM, // 0x0D42  # MALAYALAM VOWEL SIGN UU
    LineBreakClass::ULB_CM, // 0x0D43  # MALAYALAM VOWEL SIGN VOCALIC R
    LineBreakClass::ULB_ID, // 0x0D44 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D45 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0D46  # MALAYALAM VOWEL SIGN E
    LineBreakClass::ULB_CM, // 0x0D47  # MALAYALAM VOWEL SIGN EE
    LineBreakClass::ULB_CM, // 0x0D48  # MALAYALAM VOWEL SIGN AI
    LineBreakClass::ULB_ID, // 0x0D49 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0D4A  # MALAYALAM VOWEL SIGN O
    LineBreakClass::ULB_CM, // 0x0D4B  # MALAYALAM VOWEL SIGN OO
    LineBreakClass::ULB_CM, // 0x0D4C  # MALAYALAM VOWEL SIGN AU
    LineBreakClass::ULB_CM, // 0x0D4D  # MALAYALAM SIGN VIRAMA
    LineBreakClass::ULB_ID, // 0x0D4E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D4F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D50 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D51 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D52 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D53 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D54 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D55 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D56 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0D57  # MALAYALAM AU LENGTH MARK
    LineBreakClass::ULB_ID, // 0x0D58 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D59 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D5A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D5B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D5C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D5D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D5E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D5F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0D60  # MALAYALAM LETTER VOCALIC RR
    LineBreakClass::ULB_AL, // 0x0D61  # MALAYALAM LETTER VOCALIC LL
    LineBreakClass::ULB_ID, // 0x0D62 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D63 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D64 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D65 # <UNDEFINED>
    LineBreakClass::ULB_NU, // 0x0D66  # MALAYALAM DIGIT ZERO
    LineBreakClass::ULB_NU, // 0x0D67  # MALAYALAM DIGIT ONE
    LineBreakClass::ULB_NU, // 0x0D68  # MALAYALAM DIGIT TWO
    LineBreakClass::ULB_NU, // 0x0D69  # MALAYALAM DIGIT THREE
    LineBreakClass::ULB_NU, // 0x0D6A  # MALAYALAM DIGIT FOUR
    LineBreakClass::ULB_NU, // 0x0D6B  # MALAYALAM DIGIT FIVE
    LineBreakClass::ULB_NU, // 0x0D6C  # MALAYALAM DIGIT SIX
    LineBreakClass::ULB_NU, // 0x0D6D  # MALAYALAM DIGIT SEVEN
    LineBreakClass::ULB_NU, // 0x0D6E  # MALAYALAM DIGIT EIGHT
    LineBreakClass::ULB_NU, // 0x0D6F  # MALAYALAM DIGIT NINE
    LineBreakClass::ULB_ID, // 0x0D70 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D71 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D72 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D73 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D74 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D75 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D76 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D77 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D78 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D79 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D7A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D7B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D7C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D7D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D7E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D7F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D80 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D81 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0D82  # SINHALA SIGN ANUSVARAYA
    LineBreakClass::ULB_CM, // 0x0D83  # SINHALA SIGN VISARGAYA
    LineBreakClass::ULB_ID, // 0x0D84 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0D85  # SINHALA LETTER AYANNA
    LineBreakClass::ULB_AL, // 0x0D86  # SINHALA LETTER AAYANNA
    LineBreakClass::ULB_AL, // 0x0D87  # SINHALA LETTER AEYANNA
    LineBreakClass::ULB_AL, // 0x0D88  # SINHALA LETTER AEEYANNA
    LineBreakClass::ULB_AL, // 0x0D89  # SINHALA LETTER IYANNA
    LineBreakClass::ULB_AL, // 0x0D8A  # SINHALA LETTER IIYANNA
    LineBreakClass::ULB_AL, // 0x0D8B  # SINHALA LETTER UYANNA
    LineBreakClass::ULB_AL, // 0x0D8C  # SINHALA LETTER UUYANNA
    LineBreakClass::ULB_AL, // 0x0D8D  # SINHALA LETTER IRUYANNA
    LineBreakClass::ULB_AL, // 0x0D8E  # SINHALA LETTER IRUUYANNA
    LineBreakClass::ULB_AL, // 0x0D8F  # SINHALA LETTER ILUYANNA
    LineBreakClass::ULB_AL, // 0x0D90  # SINHALA LETTER ILUUYANNA
    LineBreakClass::ULB_AL, // 0x0D91  # SINHALA LETTER EYANNA
    LineBreakClass::ULB_AL, // 0x0D92  # SINHALA LETTER EEYANNA
    LineBreakClass::ULB_AL, // 0x0D93  # SINHALA LETTER AIYANNA
    LineBreakClass::ULB_AL, // 0x0D94  # SINHALA LETTER OYANNA
    LineBreakClass::ULB_AL, // 0x0D95  # SINHALA LETTER OOYANNA
    LineBreakClass::ULB_AL, // 0x0D96  # SINHALA LETTER AUYANNA
    LineBreakClass::ULB_ID, // 0x0D97 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D98 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0D99 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0D9A  # SINHALA LETTER ALPAPRAANA KAYANNA
    LineBreakClass::ULB_AL, // 0x0D9B  # SINHALA LETTER MAHAAPRAANA KAYANNA
    LineBreakClass::ULB_AL, // 0x0D9C  # SINHALA LETTER ALPAPRAANA GAYANNA
    LineBreakClass::ULB_AL, // 0x0D9D  # SINHALA LETTER MAHAAPRAANA GAYANNA
    LineBreakClass::ULB_AL, // 0x0D9E  # SINHALA LETTER KANTAJA NAASIKYAYA
    LineBreakClass::ULB_AL, // 0x0D9F  # SINHALA LETTER SANYAKA GAYANNA
    LineBreakClass::ULB_AL, // 0x0DA0  # SINHALA LETTER ALPAPRAANA CAYANNA
    LineBreakClass::ULB_AL, // 0x0DA1  # SINHALA LETTER MAHAAPRAANA CAYANNA
    LineBreakClass::ULB_AL, // 0x0DA2  # SINHALA LETTER ALPAPRAANA JAYANNA
    LineBreakClass::ULB_AL, // 0x0DA3  # SINHALA LETTER MAHAAPRAANA JAYANNA
    LineBreakClass::ULB_AL, // 0x0DA4  # SINHALA LETTER TAALUJA NAASIKYAYA
    LineBreakClass::ULB_AL, // 0x0DA5  # SINHALA LETTER TAALUJA SANYOOGA NAAKSIKYAYA
    LineBreakClass::ULB_AL, // 0x0DA6  # SINHALA LETTER SANYAKA JAYANNA
    LineBreakClass::ULB_AL, // 0x0DA7  # SINHALA LETTER ALPAPRAANA TTAYANNA
    LineBreakClass::ULB_AL, // 0x0DA8  # SINHALA LETTER MAHAAPRAANA TTAYANNA
    LineBreakClass::ULB_AL, // 0x0DA9  # SINHALA LETTER ALPAPRAANA DDAYANNA
    LineBreakClass::ULB_AL, // 0x0DAA  # SINHALA LETTER MAHAAPRAANA DDAYANNA
    LineBreakClass::ULB_AL, // 0x0DAB  # SINHALA LETTER MUURDHAJA NAYANNA
    LineBreakClass::ULB_AL, // 0x0DAC  # SINHALA LETTER SANYAKA DDAYANNA
    LineBreakClass::ULB_AL, // 0x0DAD  # SINHALA LETTER ALPAPRAANA TAYANNA
    LineBreakClass::ULB_AL, // 0x0DAE  # SINHALA LETTER MAHAAPRAANA TAYANNA
    LineBreakClass::ULB_AL, // 0x0DAF  # SINHALA LETTER ALPAPRAANA DAYANNA
    LineBreakClass::ULB_AL, // 0x0DB0  # SINHALA LETTER MAHAAPRAANA DAYANNA
    LineBreakClass::ULB_AL, // 0x0DB1  # SINHALA LETTER DANTAJA NAYANNA
    LineBreakClass::ULB_ID, // 0x0DB2 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0DB3  # SINHALA LETTER SANYAKA DAYANNA
    LineBreakClass::ULB_AL, // 0x0DB4  # SINHALA LETTER ALPAPRAANA PAYANNA
    LineBreakClass::ULB_AL, // 0x0DB5  # SINHALA LETTER MAHAAPRAANA PAYANNA
    LineBreakClass::ULB_AL, // 0x0DB6  # SINHALA LETTER ALPAPRAANA BAYANNA
    LineBreakClass::ULB_AL, // 0x0DB7  # SINHALA LETTER MAHAAPRAANA BAYANNA
    LineBreakClass::ULB_AL, // 0x0DB8  # SINHALA LETTER MAYANNA
    LineBreakClass::ULB_AL, // 0x0DB9  # SINHALA LETTER AMBA BAYANNA
    LineBreakClass::ULB_AL, // 0x0DBA  # SINHALA LETTER YAYANNA
    LineBreakClass::ULB_AL, // 0x0DBB  # SINHALA LETTER RAYANNA
    LineBreakClass::ULB_ID, // 0x0DBC # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0DBD  # SINHALA LETTER DANTAJA LAYANNA
    LineBreakClass::ULB_ID, // 0x0DBE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DBF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0DC0  # SINHALA LETTER VAYANNA
    LineBreakClass::ULB_AL, // 0x0DC1  # SINHALA LETTER TAALUJA SAYANNA
    LineBreakClass::ULB_AL, // 0x0DC2  # SINHALA LETTER MUURDHAJA SAYANNA
    LineBreakClass::ULB_AL, // 0x0DC3  # SINHALA LETTER DANTAJA SAYANNA
    LineBreakClass::ULB_AL, // 0x0DC4  # SINHALA LETTER HAYANNA
    LineBreakClass::ULB_AL, // 0x0DC5  # SINHALA LETTER MUURDHAJA LAYANNA
    LineBreakClass::ULB_AL, // 0x0DC6  # SINHALA LETTER FAYANNA
    LineBreakClass::ULB_ID, // 0x0DC7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DC8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DC9 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0DCA  # SINHALA SIGN AL-LAKUNA
    LineBreakClass::ULB_ID, // 0x0DCB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DCC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DCD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DCE # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0DCF  # SINHALA VOWEL SIGN AELA-PILLA
    LineBreakClass::ULB_CM, // 0x0DD0  # SINHALA VOWEL SIGN KETTI AEDA-PILLA
    LineBreakClass::ULB_CM, // 0x0DD1  # SINHALA VOWEL SIGN DIGA AEDA-PILLA
    LineBreakClass::ULB_CM, // 0x0DD2  # SINHALA VOWEL SIGN KETTI IS-PILLA
    LineBreakClass::ULB_CM, // 0x0DD3  # SINHALA VOWEL SIGN DIGA IS-PILLA
    LineBreakClass::ULB_CM, // 0x0DD4  # SINHALA VOWEL SIGN KETTI PAA-PILLA
    LineBreakClass::ULB_ID, // 0x0DD5 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0DD6  # SINHALA VOWEL SIGN DIGA PAA-PILLA
    LineBreakClass::ULB_ID, // 0x0DD7 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0DD8  # SINHALA VOWEL SIGN GAETTA-PILLA
    LineBreakClass::ULB_CM, // 0x0DD9  # SINHALA VOWEL SIGN KOMBUVA
    LineBreakClass::ULB_CM, // 0x0DDA  # SINHALA VOWEL SIGN DIGA KOMBUVA
    LineBreakClass::ULB_CM, // 0x0DDB  # SINHALA VOWEL SIGN KOMBU DEKA
    LineBreakClass::ULB_CM, // 0x0DDC  # SINHALA VOWEL SIGN KOMBUVA HAA AELA-PILLA
    LineBreakClass::ULB_CM, // 0x0DDD  # SINHALA VOWEL SIGN KOMBUVA HAA DIGA AELA-PILLA
    LineBreakClass::ULB_CM, // 0x0DDE  # SINHALA VOWEL SIGN KOMBUVA HAA GAYANUKITTA
    LineBreakClass::ULB_CM, // 0x0DDF  # SINHALA VOWEL SIGN GAYANUKITTA
    LineBreakClass::ULB_ID, // 0x0DE0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DE1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DE2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DE3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DE4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DE5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DE6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DE7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DE8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DE9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DEA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DEB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DEC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DED # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DEE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DEF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DF0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DF1 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0DF2  # SINHALA VOWEL SIGN DIGA GAETTA-PILLA
    LineBreakClass::ULB_CM, // 0x0DF3  # SINHALA VOWEL SIGN DIGA GAYANUKITTA
    LineBreakClass::ULB_AL, // 0x0DF4  # SINHALA PUNCTUATION KUNDDALIYA
    LineBreakClass::ULB_ID, // 0x0DF5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DF6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DF7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DF8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DF9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DFA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DFB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DFC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DFD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DFE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0DFF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E00 # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x0E01  # THAI CHARACTER KO KAI
    LineBreakClass::ULB_SA, // 0x0E02  # THAI CHARACTER KHO KHAI
    LineBreakClass::ULB_SA, // 0x0E03  # THAI CHARACTER KHO KHUAT
    LineBreakClass::ULB_SA, // 0x0E04  # THAI CHARACTER KHO KHWAI
    LineBreakClass::ULB_SA, // 0x0E05  # THAI CHARACTER KHO KHON
    LineBreakClass::ULB_SA, // 0x0E06  # THAI CHARACTER KHO RAKHANG
    LineBreakClass::ULB_SA, // 0x0E07  # THAI CHARACTER NGO NGU
    LineBreakClass::ULB_SA, // 0x0E08  # THAI CHARACTER CHO CHAN
    LineBreakClass::ULB_SA, // 0x0E09  # THAI CHARACTER CHO CHING
    LineBreakClass::ULB_SA, // 0x0E0A  # THAI CHARACTER CHO CHANG
    LineBreakClass::ULB_SA, // 0x0E0B  # THAI CHARACTER SO SO
    LineBreakClass::ULB_SA, // 0x0E0C  # THAI CHARACTER CHO CHOE
    LineBreakClass::ULB_SA, // 0x0E0D  # THAI CHARACTER YO YING
    LineBreakClass::ULB_SA, // 0x0E0E  # THAI CHARACTER DO CHADA
    LineBreakClass::ULB_SA, // 0x0E0F  # THAI CHARACTER TO PATAK
    LineBreakClass::ULB_SA, // 0x0E10  # THAI CHARACTER THO THAN
    LineBreakClass::ULB_SA, // 0x0E11  # THAI CHARACTER THO NANGMONTHO
    LineBreakClass::ULB_SA, // 0x0E12  # THAI CHARACTER THO PHUTHAO
    LineBreakClass::ULB_SA, // 0x0E13  # THAI CHARACTER NO NEN
    LineBreakClass::ULB_SA, // 0x0E14  # THAI CHARACTER DO DEK
    LineBreakClass::ULB_SA, // 0x0E15  # THAI CHARACTER TO TAO
    LineBreakClass::ULB_SA, // 0x0E16  # THAI CHARACTER THO THUNG
    LineBreakClass::ULB_SA, // 0x0E17  # THAI CHARACTER THO THAHAN
    LineBreakClass::ULB_SA, // 0x0E18  # THAI CHARACTER THO THONG
    LineBreakClass::ULB_SA, // 0x0E19  # THAI CHARACTER NO NU
    LineBreakClass::ULB_SA, // 0x0E1A  # THAI CHARACTER BO BAIMAI
    LineBreakClass::ULB_SA, // 0x0E1B  # THAI CHARACTER PO PLA
    LineBreakClass::ULB_SA, // 0x0E1C  # THAI CHARACTER PHO PHUNG
    LineBreakClass::ULB_SA, // 0x0E1D  # THAI CHARACTER FO FA
    LineBreakClass::ULB_SA, // 0x0E1E  # THAI CHARACTER PHO PHAN
    LineBreakClass::ULB_SA, // 0x0E1F  # THAI CHARACTER FO FAN
    LineBreakClass::ULB_SA, // 0x0E20  # THAI CHARACTER PHO SAMPHAO
    LineBreakClass::ULB_SA, // 0x0E21  # THAI CHARACTER MO MA
    LineBreakClass::ULB_SA, // 0x0E22  # THAI CHARACTER YO YAK
    LineBreakClass::ULB_SA, // 0x0E23  # THAI CHARACTER RO RUA
    LineBreakClass::ULB_SA, // 0x0E24  # THAI CHARACTER RU
    LineBreakClass::ULB_SA, // 0x0E25  # THAI CHARACTER LO LING
    LineBreakClass::ULB_SA, // 0x0E26  # THAI CHARACTER LU
    LineBreakClass::ULB_SA, // 0x0E27  # THAI CHARACTER WO WAEN
    LineBreakClass::ULB_SA, // 0x0E28  # THAI CHARACTER SO SALA
    LineBreakClass::ULB_SA, // 0x0E29  # THAI CHARACTER SO RUSI
    LineBreakClass::ULB_SA, // 0x0E2A  # THAI CHARACTER SO SUA
    LineBreakClass::ULB_SA, // 0x0E2B  # THAI CHARACTER HO HIP
    LineBreakClass::ULB_SA, // 0x0E2C  # THAI CHARACTER LO CHULA
    LineBreakClass::ULB_SA, // 0x0E2D  # THAI CHARACTER O ANG
    LineBreakClass::ULB_SA, // 0x0E2E  # THAI CHARACTER HO NOKHUK
    LineBreakClass::ULB_SA, // 0x0E2F  # THAI CHARACTER PAIYANNOI
    LineBreakClass::ULB_SA, // 0x0E30  # THAI CHARACTER SARA A
    LineBreakClass::ULB_SA, // 0x0E31  # THAI CHARACTER MAI HAN-AKAT
    LineBreakClass::ULB_SA, // 0x0E32  # THAI CHARACTER SARA AA
    LineBreakClass::ULB_SA, // 0x0E33  # THAI CHARACTER SARA AM
    LineBreakClass::ULB_SA, // 0x0E34  # THAI CHARACTER SARA I
    LineBreakClass::ULB_SA, // 0x0E35  # THAI CHARACTER SARA II
    LineBreakClass::ULB_SA, // 0x0E36  # THAI CHARACTER SARA UE
    LineBreakClass::ULB_SA, // 0x0E37  # THAI CHARACTER SARA UEE
    LineBreakClass::ULB_SA, // 0x0E38  # THAI CHARACTER SARA U
    LineBreakClass::ULB_SA, // 0x0E39  # THAI CHARACTER SARA UU
    LineBreakClass::ULB_SA, // 0x0E3A  # THAI CHARACTER PHINTHU
    LineBreakClass::ULB_ID, // 0x0E3B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E3C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E3D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E3E # <UNDEFINED>
    LineBreakClass::ULB_PR, // 0x0E3F  # THAI CURRENCY SYMBOL BAHT
    LineBreakClass::ULB_SA, // 0x0E40  # THAI CHARACTER SARA E
    LineBreakClass::ULB_SA, // 0x0E41  # THAI CHARACTER SARA AE
    LineBreakClass::ULB_SA, // 0x0E42  # THAI CHARACTER SARA O
    LineBreakClass::ULB_SA, // 0x0E43  # THAI CHARACTER SARA AI MAIMUAN
    LineBreakClass::ULB_SA, // 0x0E44  # THAI CHARACTER SARA AI MAIMALAI
    LineBreakClass::ULB_SA, // 0x0E45  # THAI CHARACTER LAKKHANGYAO
    LineBreakClass::ULB_SA, // 0x0E46  # THAI CHARACTER MAIYAMOK
    LineBreakClass::ULB_SA, // 0x0E47  # THAI CHARACTER MAITAIKHU
    LineBreakClass::ULB_SA, // 0x0E48  # THAI CHARACTER MAI EK
    LineBreakClass::ULB_SA, // 0x0E49  # THAI CHARACTER MAI THO
    LineBreakClass::ULB_SA, // 0x0E4A  # THAI CHARACTER MAI TRI
    LineBreakClass::ULB_SA, // 0x0E4B  # THAI CHARACTER MAI CHATTAWA
    LineBreakClass::ULB_SA, // 0x0E4C  # THAI CHARACTER THANTHAKHAT
    LineBreakClass::ULB_SA, // 0x0E4D  # THAI CHARACTER NIKHAHIT
    LineBreakClass::ULB_SA, // 0x0E4E  # THAI CHARACTER YAMAKKAN
    LineBreakClass::ULB_AL, // 0x0E4F  # THAI CHARACTER FONGMAN
    LineBreakClass::ULB_NU, // 0x0E50  # THAI DIGIT ZERO
    LineBreakClass::ULB_NU, // 0x0E51  # THAI DIGIT ONE
    LineBreakClass::ULB_NU, // 0x0E52  # THAI DIGIT TWO
    LineBreakClass::ULB_NU, // 0x0E53  # THAI DIGIT THREE
    LineBreakClass::ULB_NU, // 0x0E54  # THAI DIGIT FOUR
    LineBreakClass::ULB_NU, // 0x0E55  # THAI DIGIT FIVE
    LineBreakClass::ULB_NU, // 0x0E56  # THAI DIGIT SIX
    LineBreakClass::ULB_NU, // 0x0E57  # THAI DIGIT SEVEN
    LineBreakClass::ULB_NU, // 0x0E58  # THAI DIGIT EIGHT
    LineBreakClass::ULB_NU, // 0x0E59  # THAI DIGIT NINE
    LineBreakClass::ULB_BA, // 0x0E5A  # THAI CHARACTER ANGKHANKHU
    LineBreakClass::ULB_BA, // 0x0E5B  # THAI CHARACTER KHOMUT
    LineBreakClass::ULB_ID, // 0x0E5C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E5D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E5E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E5F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E60 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E61 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E62 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E63 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E64 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E65 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E66 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E67 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E68 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E69 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E6A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E6B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E6C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E6D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E6E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E6F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E70 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E71 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E72 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E73 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E74 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E75 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E76 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E77 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E78 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E79 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E7A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E7B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E7C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E7D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E7E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E7F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E80 # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x0E81  # LAO LETTER KO
    LineBreakClass::ULB_SA, // 0x0E82  # LAO LETTER KHO SUNG
    LineBreakClass::ULB_ID, // 0x0E83 # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x0E84  # LAO LETTER KHO TAM
    LineBreakClass::ULB_ID, // 0x0E85 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E86 # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x0E87  # LAO LETTER NGO
    LineBreakClass::ULB_SA, // 0x0E88  # LAO LETTER CO
    LineBreakClass::ULB_ID, // 0x0E89 # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x0E8A  # LAO LETTER SO TAM
    LineBreakClass::ULB_ID, // 0x0E8B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E8C # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x0E8D  # LAO LETTER NYO
    LineBreakClass::ULB_ID, // 0x0E8E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E8F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E90 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E91 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E92 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0E93 # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x0E94  # LAO LETTER DO
    LineBreakClass::ULB_SA, // 0x0E95  # LAO LETTER TO
    LineBreakClass::ULB_SA, // 0x0E96  # LAO LETTER THO SUNG
    LineBreakClass::ULB_SA, // 0x0E97  # LAO LETTER THO TAM
    LineBreakClass::ULB_ID, // 0x0E98 # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x0E99  # LAO LETTER NO
    LineBreakClass::ULB_SA, // 0x0E9A  # LAO LETTER BO
    LineBreakClass::ULB_SA, // 0x0E9B  # LAO LETTER PO
    LineBreakClass::ULB_SA, // 0x0E9C  # LAO LETTER PHO SUNG
    LineBreakClass::ULB_SA, // 0x0E9D  # LAO LETTER FO TAM
    LineBreakClass::ULB_SA, // 0x0E9E  # LAO LETTER PHO TAM
    LineBreakClass::ULB_SA, // 0x0E9F  # LAO LETTER FO SUNG
    LineBreakClass::ULB_ID, // 0x0EA0 # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x0EA1  # LAO LETTER MO
    LineBreakClass::ULB_SA, // 0x0EA2  # LAO LETTER YO
    LineBreakClass::ULB_SA, // 0x0EA3  # LAO LETTER LO LING
    LineBreakClass::ULB_ID, // 0x0EA4 # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x0EA5  # LAO LETTER LO LOOT
    LineBreakClass::ULB_ID, // 0x0EA6 # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x0EA7  # LAO LETTER WO
    LineBreakClass::ULB_ID, // 0x0EA8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EA9 # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x0EAA  # LAO LETTER SO SUNG
    LineBreakClass::ULB_SA, // 0x0EAB  # LAO LETTER HO SUNG
    LineBreakClass::ULB_ID, // 0x0EAC # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x0EAD  # LAO LETTER O
    LineBreakClass::ULB_SA, // 0x0EAE  # LAO LETTER HO TAM
    LineBreakClass::ULB_SA, // 0x0EAF  # LAO ELLIPSIS
    LineBreakClass::ULB_SA, // 0x0EB0  # LAO VOWEL SIGN A
    LineBreakClass::ULB_SA, // 0x0EB1  # LAO VOWEL SIGN MAI KAN
    LineBreakClass::ULB_SA, // 0x0EB2  # LAO VOWEL SIGN AA
    LineBreakClass::ULB_SA, // 0x0EB3  # LAO VOWEL SIGN AM
    LineBreakClass::ULB_SA, // 0x0EB4  # LAO VOWEL SIGN I
    LineBreakClass::ULB_SA, // 0x0EB5  # LAO VOWEL SIGN II
    LineBreakClass::ULB_SA, // 0x0EB6  # LAO VOWEL SIGN Y
    LineBreakClass::ULB_SA, // 0x0EB7  # LAO VOWEL SIGN YY
    LineBreakClass::ULB_SA, // 0x0EB8  # LAO VOWEL SIGN U
    LineBreakClass::ULB_SA, // 0x0EB9  # LAO VOWEL SIGN UU
    LineBreakClass::ULB_ID, // 0x0EBA # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x0EBB  # LAO VOWEL SIGN MAI KON
    LineBreakClass::ULB_SA, // 0x0EBC  # LAO SEMIVOWEL SIGN LO
    LineBreakClass::ULB_SA, // 0x0EBD  # LAO SEMIVOWEL SIGN NYO
    LineBreakClass::ULB_ID, // 0x0EBE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EBF # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x0EC0  # LAO VOWEL SIGN E
    LineBreakClass::ULB_SA, // 0x0EC1  # LAO VOWEL SIGN EI
    LineBreakClass::ULB_SA, // 0x0EC2  # LAO VOWEL SIGN O
    LineBreakClass::ULB_SA, // 0x0EC3  # LAO VOWEL SIGN AY
    LineBreakClass::ULB_SA, // 0x0EC4  # LAO VOWEL SIGN AI
    LineBreakClass::ULB_ID, // 0x0EC5 # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x0EC6  # LAO KO LA
    LineBreakClass::ULB_ID, // 0x0EC7 # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x0EC8  # LAO TONE MAI EK
    LineBreakClass::ULB_SA, // 0x0EC9  # LAO TONE MAI THO
    LineBreakClass::ULB_SA, // 0x0ECA  # LAO TONE MAI TI
    LineBreakClass::ULB_SA, // 0x0ECB  # LAO TONE MAI CATAWA
    LineBreakClass::ULB_SA, // 0x0ECC  # LAO CANCELLATION MARK
    LineBreakClass::ULB_SA, // 0x0ECD  # LAO NIGGAHITA
    LineBreakClass::ULB_ID, // 0x0ECE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0ECF # <UNDEFINED>
    LineBreakClass::ULB_NU, // 0x0ED0  # LAO DIGIT ZERO
    LineBreakClass::ULB_NU, // 0x0ED1  # LAO DIGIT ONE
    LineBreakClass::ULB_NU, // 0x0ED2  # LAO DIGIT TWO
    LineBreakClass::ULB_NU, // 0x0ED3  # LAO DIGIT THREE
    LineBreakClass::ULB_NU, // 0x0ED4  # LAO DIGIT FOUR
    LineBreakClass::ULB_NU, // 0x0ED5  # LAO DIGIT FIVE
    LineBreakClass::ULB_NU, // 0x0ED6  # LAO DIGIT SIX
    LineBreakClass::ULB_NU, // 0x0ED7  # LAO DIGIT SEVEN
    LineBreakClass::ULB_NU, // 0x0ED8  # LAO DIGIT EIGHT
    LineBreakClass::ULB_NU, // 0x0ED9  # LAO DIGIT NINE
    LineBreakClass::ULB_ID, // 0x0EDA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EDB # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x0EDC  # LAO HO NO
    LineBreakClass::ULB_SA, // 0x0EDD  # LAO HO MO
    LineBreakClass::ULB_ID, // 0x0EDE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EDF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EE0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EE1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EE2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EE3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EE4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EE5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EE6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EE7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EE8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EE9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EEA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EEB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EEC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EED # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EEE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EEF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EF0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EF1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EF2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EF3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EF4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EF5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EF6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EF7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EF8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EF9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EFA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EFB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EFC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EFD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EFE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0EFF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0F00  # TIBETAN SYLLABLE OM
    LineBreakClass::ULB_BB, // 0x0F01  # TIBETAN MARK GTER YIG MGO TRUNCATED A
    LineBreakClass::ULB_BB, // 0x0F02  # TIBETAN MARK GTER YIG MGO -UM RNAM BCAD MA
    LineBreakClass::ULB_BB, // 0x0F03  # TIBETAN MARK GTER YIG MGO -UM GTER TSHEG MA
    LineBreakClass::ULB_BB, // 0x0F04  # TIBETAN MARK INITIAL YIG MGO MDUN MA
    LineBreakClass::ULB_AL, // 0x0F05  # TIBETAN MARK CLOSING YIG MGO SGAB MA
    LineBreakClass::ULB_BB, // 0x0F06  # TIBETAN MARK CARET YIG MGO PHUR SHAD MA
    LineBreakClass::ULB_BB, // 0x0F07  # TIBETAN MARK YIG MGO TSHEG SHAD MA
    LineBreakClass::ULB_GL, // 0x0F08  # TIBETAN MARK SBRUL SHAD
    LineBreakClass::ULB_BB, // 0x0F09  # TIBETAN MARK BSKUR YIG MGO
    LineBreakClass::ULB_BB, // 0x0F0A  # TIBETAN MARK BKA- SHOG YIG MGO
    LineBreakClass::ULB_BA, // 0x0F0B  # TIBETAN MARK INTERSYLLABIC TSHEG
    LineBreakClass::ULB_GL, // 0x0F0C  # TIBETAN MARK DELIMITER TSHEG BSTAR
    LineBreakClass::ULB_EX, // 0x0F0D  # TIBETAN MARK SHAD
    LineBreakClass::ULB_EX, // 0x0F0E  # TIBETAN MARK NYIS SHAD
    LineBreakClass::ULB_EX, // 0x0F0F  # TIBETAN MARK TSHEG SHAD
    LineBreakClass::ULB_EX, // 0x0F10  # TIBETAN MARK NYIS TSHEG SHAD
    LineBreakClass::ULB_EX, // 0x0F11  # TIBETAN MARK RIN CHEN SPUNGS SHAD
    LineBreakClass::ULB_GL, // 0x0F12  # TIBETAN MARK RGYA GRAM SHAD
    LineBreakClass::ULB_AL, // 0x0F13  # TIBETAN MARK CARET -DZUD RTAGS ME LONG CAN
    LineBreakClass::ULB_EX, // 0x0F14  # TIBETAN MARK GTER TSHEG
    LineBreakClass::ULB_AL, // 0x0F15  # TIBETAN LOGOTYPE SIGN CHAD RTAGS
    LineBreakClass::ULB_AL, // 0x0F16  # TIBETAN LOGOTYPE SIGN LHAG RTAGS
    LineBreakClass::ULB_AL, // 0x0F17  # TIBETAN ASTROLOGICAL SIGN SGRA GCAN -CHAR RTAGS
    LineBreakClass::ULB_CM, // 0x0F18  # TIBETAN ASTROLOGICAL SIGN -KHYUD PA
    LineBreakClass::ULB_CM, // 0x0F19  # TIBETAN ASTROLOGICAL SIGN SDONG TSHUGS
    LineBreakClass::ULB_AL, // 0x0F1A  # TIBETAN SIGN RDEL DKAR GCIG
    LineBreakClass::ULB_AL, // 0x0F1B  # TIBETAN SIGN RDEL DKAR GNYIS
    LineBreakClass::ULB_AL, // 0x0F1C  # TIBETAN SIGN RDEL DKAR GSUM
    LineBreakClass::ULB_AL, // 0x0F1D  # TIBETAN SIGN RDEL NAG GCIG
    LineBreakClass::ULB_AL, // 0x0F1E  # TIBETAN SIGN RDEL NAG GNYIS
    LineBreakClass::ULB_AL, // 0x0F1F  # TIBETAN SIGN RDEL DKAR RDEL NAG
    LineBreakClass::ULB_NU, // 0x0F20  # TIBETAN DIGIT ZERO
    LineBreakClass::ULB_NU, // 0x0F21  # TIBETAN DIGIT ONE
    LineBreakClass::ULB_NU, // 0x0F22  # TIBETAN DIGIT TWO
    LineBreakClass::ULB_NU, // 0x0F23  # TIBETAN DIGIT THREE
    LineBreakClass::ULB_NU, // 0x0F24  # TIBETAN DIGIT FOUR
    LineBreakClass::ULB_NU, // 0x0F25  # TIBETAN DIGIT FIVE
    LineBreakClass::ULB_NU, // 0x0F26  # TIBETAN DIGIT SIX
    LineBreakClass::ULB_NU, // 0x0F27  # TIBETAN DIGIT SEVEN
    LineBreakClass::ULB_NU, // 0x0F28  # TIBETAN DIGIT EIGHT
    LineBreakClass::ULB_NU, // 0x0F29  # TIBETAN DIGIT NINE
    LineBreakClass::ULB_AL, // 0x0F2A  # TIBETAN DIGIT HALF ONE
    LineBreakClass::ULB_AL, // 0x0F2B  # TIBETAN DIGIT HALF TWO
    LineBreakClass::ULB_AL, // 0x0F2C  # TIBETAN DIGIT HALF THREE
    LineBreakClass::ULB_AL, // 0x0F2D  # TIBETAN DIGIT HALF FOUR
    LineBreakClass::ULB_AL, // 0x0F2E  # TIBETAN DIGIT HALF FIVE
    LineBreakClass::ULB_AL, // 0x0F2F  # TIBETAN DIGIT HALF SIX
    LineBreakClass::ULB_AL, // 0x0F30  # TIBETAN DIGIT HALF SEVEN
    LineBreakClass::ULB_AL, // 0x0F31  # TIBETAN DIGIT HALF EIGHT
    LineBreakClass::ULB_AL, // 0x0F32  # TIBETAN DIGIT HALF NINE
    LineBreakClass::ULB_AL, // 0x0F33  # TIBETAN DIGIT HALF ZERO
    LineBreakClass::ULB_BA, // 0x0F34  # TIBETAN MARK BSDUS RTAGS
    LineBreakClass::ULB_CM, // 0x0F35  # TIBETAN MARK NGAS BZUNG NYI ZLA
    LineBreakClass::ULB_AL, // 0x0F36  # TIBETAN MARK CARET -DZUD RTAGS BZHI MIG CAN
    LineBreakClass::ULB_CM, // 0x0F37  # TIBETAN MARK NGAS BZUNG SGOR RTAGS
    LineBreakClass::ULB_AL, // 0x0F38  # TIBETAN MARK CHE MGO
    LineBreakClass::ULB_CM, // 0x0F39  # TIBETAN MARK TSA -PHRU
    LineBreakClass::ULB_OP, // 0x0F3A  # TIBETAN MARK GUG RTAGS GYON
    LineBreakClass::ULB_CL, // 0x0F3B  # TIBETAN MARK GUG RTAGS GYAS
    LineBreakClass::ULB_OP, // 0x0F3C  # TIBETAN MARK ANG KHANG GYON
    LineBreakClass::ULB_CL, // 0x0F3D  # TIBETAN MARK ANG KHANG GYAS
    LineBreakClass::ULB_CM, // 0x0F3E  # TIBETAN SIGN YAR TSHES
    LineBreakClass::ULB_CM, // 0x0F3F  # TIBETAN SIGN MAR TSHES
    LineBreakClass::ULB_AL, // 0x0F40  # TIBETAN LETTER KA
    LineBreakClass::ULB_AL, // 0x0F41  # TIBETAN LETTER KHA
    LineBreakClass::ULB_AL, // 0x0F42  # TIBETAN LETTER GA
    LineBreakClass::ULB_AL, // 0x0F43  # TIBETAN LETTER GHA
    LineBreakClass::ULB_AL, // 0x0F44  # TIBETAN LETTER NGA
    LineBreakClass::ULB_AL, // 0x0F45  # TIBETAN LETTER CA
    LineBreakClass::ULB_AL, // 0x0F46  # TIBETAN LETTER CHA
    LineBreakClass::ULB_AL, // 0x0F47  # TIBETAN LETTER JA
    LineBreakClass::ULB_ID, // 0x0F48 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0F49  # TIBETAN LETTER NYA
    LineBreakClass::ULB_AL, // 0x0F4A  # TIBETAN LETTER TTA
    LineBreakClass::ULB_AL, // 0x0F4B  # TIBETAN LETTER TTHA
    LineBreakClass::ULB_AL, // 0x0F4C  # TIBETAN LETTER DDA
    LineBreakClass::ULB_AL, // 0x0F4D  # TIBETAN LETTER DDHA
    LineBreakClass::ULB_AL, // 0x0F4E  # TIBETAN LETTER NNA
    LineBreakClass::ULB_AL, // 0x0F4F  # TIBETAN LETTER TA
    LineBreakClass::ULB_AL, // 0x0F50  # TIBETAN LETTER THA
    LineBreakClass::ULB_AL, // 0x0F51  # TIBETAN LETTER DA
    LineBreakClass::ULB_AL, // 0x0F52  # TIBETAN LETTER DHA
    LineBreakClass::ULB_AL, // 0x0F53  # TIBETAN LETTER NA
    LineBreakClass::ULB_AL, // 0x0F54  # TIBETAN LETTER PA
    LineBreakClass::ULB_AL, // 0x0F55  # TIBETAN LETTER PHA
    LineBreakClass::ULB_AL, // 0x0F56  # TIBETAN LETTER BA
    LineBreakClass::ULB_AL, // 0x0F57  # TIBETAN LETTER BHA
    LineBreakClass::ULB_AL, // 0x0F58  # TIBETAN LETTER MA
    LineBreakClass::ULB_AL, // 0x0F59  # TIBETAN LETTER TSA
    LineBreakClass::ULB_AL, // 0x0F5A  # TIBETAN LETTER TSHA
    LineBreakClass::ULB_AL, // 0x0F5B  # TIBETAN LETTER DZA
    LineBreakClass::ULB_AL, // 0x0F5C  # TIBETAN LETTER DZHA
    LineBreakClass::ULB_AL, // 0x0F5D  # TIBETAN LETTER WA
    LineBreakClass::ULB_AL, // 0x0F5E  # TIBETAN LETTER ZHA
    LineBreakClass::ULB_AL, // 0x0F5F  # TIBETAN LETTER ZA
    LineBreakClass::ULB_AL, // 0x0F60  # TIBETAN LETTER -A
    LineBreakClass::ULB_AL, // 0x0F61  # TIBETAN LETTER YA
    LineBreakClass::ULB_AL, // 0x0F62  # TIBETAN LETTER RA
    LineBreakClass::ULB_AL, // 0x0F63  # TIBETAN LETTER LA
    LineBreakClass::ULB_AL, // 0x0F64  # TIBETAN LETTER SHA
    LineBreakClass::ULB_AL, // 0x0F65  # TIBETAN LETTER SSA
    LineBreakClass::ULB_AL, // 0x0F66  # TIBETAN LETTER SA
    LineBreakClass::ULB_AL, // 0x0F67  # TIBETAN LETTER HA
    LineBreakClass::ULB_AL, // 0x0F68  # TIBETAN LETTER A
    LineBreakClass::ULB_AL, // 0x0F69  # TIBETAN LETTER KSSA
    LineBreakClass::ULB_AL, // 0x0F6A  # TIBETAN LETTER FIXED-FORM RA
    LineBreakClass::ULB_ID, // 0x0F6B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0F6C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0F6D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0F6E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0F6F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0F70 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0F71  # TIBETAN VOWEL SIGN AA
    LineBreakClass::ULB_CM, // 0x0F72  # TIBETAN VOWEL SIGN I
    LineBreakClass::ULB_CM, // 0x0F73  # TIBETAN VOWEL SIGN II
    LineBreakClass::ULB_CM, // 0x0F74  # TIBETAN VOWEL SIGN U
    LineBreakClass::ULB_CM, // 0x0F75  # TIBETAN VOWEL SIGN UU
    LineBreakClass::ULB_CM, // 0x0F76  # TIBETAN VOWEL SIGN VOCALIC R
    LineBreakClass::ULB_CM, // 0x0F77  # TIBETAN VOWEL SIGN VOCALIC RR
    LineBreakClass::ULB_CM, // 0x0F78  # TIBETAN VOWEL SIGN VOCALIC L
    LineBreakClass::ULB_CM, // 0x0F79  # TIBETAN VOWEL SIGN VOCALIC LL
    LineBreakClass::ULB_CM, // 0x0F7A  # TIBETAN VOWEL SIGN E
    LineBreakClass::ULB_CM, // 0x0F7B  # TIBETAN VOWEL SIGN EE
    LineBreakClass::ULB_CM, // 0x0F7C  # TIBETAN VOWEL SIGN O
    LineBreakClass::ULB_CM, // 0x0F7D  # TIBETAN VOWEL SIGN OO
    LineBreakClass::ULB_CM, // 0x0F7E  # TIBETAN SIGN RJES SU NGA RO
    LineBreakClass::ULB_BA, // 0x0F7F  # TIBETAN SIGN RNAM BCAD
    LineBreakClass::ULB_CM, // 0x0F80  # TIBETAN VOWEL SIGN REVERSED I
    LineBreakClass::ULB_CM, // 0x0F81  # TIBETAN VOWEL SIGN REVERSED II
    LineBreakClass::ULB_CM, // 0x0F82  # TIBETAN SIGN NYI ZLA NAA DA
    LineBreakClass::ULB_CM, // 0x0F83  # TIBETAN SIGN SNA LDAN
    LineBreakClass::ULB_CM, // 0x0F84  # TIBETAN MARK HALANTA
    LineBreakClass::ULB_BA, // 0x0F85  # TIBETAN MARK PALUTA
    LineBreakClass::ULB_CM, // 0x0F86  # TIBETAN SIGN LCI RTAGS
    LineBreakClass::ULB_CM, // 0x0F87  # TIBETAN SIGN YANG RTAGS
    LineBreakClass::ULB_AL, // 0x0F88  # TIBETAN SIGN LCE TSA CAN
    LineBreakClass::ULB_AL, // 0x0F89  # TIBETAN SIGN MCHU CAN
    LineBreakClass::ULB_AL, // 0x0F8A  # TIBETAN SIGN GRU CAN RGYINGS
    LineBreakClass::ULB_AL, // 0x0F8B  # TIBETAN SIGN GRU MED RGYINGS
    LineBreakClass::ULB_ID, // 0x0F8C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0F8D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0F8E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0F8F # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0F90  # TIBETAN SUBJOINED LETTER KA
    LineBreakClass::ULB_CM, // 0x0F91  # TIBETAN SUBJOINED LETTER KHA
    LineBreakClass::ULB_CM, // 0x0F92  # TIBETAN SUBJOINED LETTER GA
    LineBreakClass::ULB_CM, // 0x0F93  # TIBETAN SUBJOINED LETTER GHA
    LineBreakClass::ULB_CM, // 0x0F94  # TIBETAN SUBJOINED LETTER NGA
    LineBreakClass::ULB_CM, // 0x0F95  # TIBETAN SUBJOINED LETTER CA
    LineBreakClass::ULB_CM, // 0x0F96  # TIBETAN SUBJOINED LETTER CHA
    LineBreakClass::ULB_CM, // 0x0F97  # TIBETAN SUBJOINED LETTER JA
    LineBreakClass::ULB_ID, // 0x0F98 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x0F99  # TIBETAN SUBJOINED LETTER NYA
    LineBreakClass::ULB_CM, // 0x0F9A  # TIBETAN SUBJOINED LETTER TTA
    LineBreakClass::ULB_CM, // 0x0F9B  # TIBETAN SUBJOINED LETTER TTHA
    LineBreakClass::ULB_CM, // 0x0F9C  # TIBETAN SUBJOINED LETTER DDA
    LineBreakClass::ULB_CM, // 0x0F9D  # TIBETAN SUBJOINED LETTER DDHA
    LineBreakClass::ULB_CM, // 0x0F9E  # TIBETAN SUBJOINED LETTER NNA
    LineBreakClass::ULB_CM, // 0x0F9F  # TIBETAN SUBJOINED LETTER TA
    LineBreakClass::ULB_CM, // 0x0FA0  # TIBETAN SUBJOINED LETTER THA
    LineBreakClass::ULB_CM, // 0x0FA1  # TIBETAN SUBJOINED LETTER DA
    LineBreakClass::ULB_CM, // 0x0FA2  # TIBETAN SUBJOINED LETTER DHA
    LineBreakClass::ULB_CM, // 0x0FA3  # TIBETAN SUBJOINED LETTER NA
    LineBreakClass::ULB_CM, // 0x0FA4  # TIBETAN SUBJOINED LETTER PA
    LineBreakClass::ULB_CM, // 0x0FA5  # TIBETAN SUBJOINED LETTER PHA
    LineBreakClass::ULB_CM, // 0x0FA6  # TIBETAN SUBJOINED LETTER BA
    LineBreakClass::ULB_CM, // 0x0FA7  # TIBETAN SUBJOINED LETTER BHA
    LineBreakClass::ULB_CM, // 0x0FA8  # TIBETAN SUBJOINED LETTER MA
    LineBreakClass::ULB_CM, // 0x0FA9  # TIBETAN SUBJOINED LETTER TSA
    LineBreakClass::ULB_CM, // 0x0FAA  # TIBETAN SUBJOINED LETTER TSHA
    LineBreakClass::ULB_CM, // 0x0FAB  # TIBETAN SUBJOINED LETTER DZA
    LineBreakClass::ULB_CM, // 0x0FAC  # TIBETAN SUBJOINED LETTER DZHA
    LineBreakClass::ULB_CM, // 0x0FAD  # TIBETAN SUBJOINED LETTER WA
    LineBreakClass::ULB_CM, // 0x0FAE  # TIBETAN SUBJOINED LETTER ZHA
    LineBreakClass::ULB_CM, // 0x0FAF  # TIBETAN SUBJOINED LETTER ZA
    LineBreakClass::ULB_CM, // 0x0FB0  # TIBETAN SUBJOINED LETTER -A
    LineBreakClass::ULB_CM, // 0x0FB1  # TIBETAN SUBJOINED LETTER YA
    LineBreakClass::ULB_CM, // 0x0FB2  # TIBETAN SUBJOINED LETTER RA
    LineBreakClass::ULB_CM, // 0x0FB3  # TIBETAN SUBJOINED LETTER LA
    LineBreakClass::ULB_CM, // 0x0FB4  # TIBETAN SUBJOINED LETTER SHA
    LineBreakClass::ULB_CM, // 0x0FB5  # TIBETAN SUBJOINED LETTER SSA
    LineBreakClass::ULB_CM, // 0x0FB6  # TIBETAN SUBJOINED LETTER SA
    LineBreakClass::ULB_CM, // 0x0FB7  # TIBETAN SUBJOINED LETTER HA
    LineBreakClass::ULB_CM, // 0x0FB8  # TIBETAN SUBJOINED LETTER A
    LineBreakClass::ULB_CM, // 0x0FB9  # TIBETAN SUBJOINED LETTER KSSA
    LineBreakClass::ULB_CM, // 0x0FBA  # TIBETAN SUBJOINED LETTER FIXED-FORM WA
    LineBreakClass::ULB_CM, // 0x0FBB  # TIBETAN SUBJOINED LETTER FIXED-FORM YA
    LineBreakClass::ULB_CM, // 0x0FBC  # TIBETAN SUBJOINED LETTER FIXED-FORM RA
    LineBreakClass::ULB_ID, // 0x0FBD # <UNDEFINED>
    LineBreakClass::ULB_BA, // 0x0FBE  # TIBETAN KU RU KHA
    LineBreakClass::ULB_BA, // 0x0FBF  # TIBETAN KU RU KHA BZHI MIG CAN
    LineBreakClass::ULB_AL, // 0x0FC0  # TIBETAN CANTILLATION SIGN HEAVY BEAT
    LineBreakClass::ULB_AL, // 0x0FC1  # TIBETAN CANTILLATION SIGN LIGHT BEAT
    LineBreakClass::ULB_AL, // 0x0FC2  # TIBETAN CANTILLATION SIGN CANG TE-U
    LineBreakClass::ULB_AL, // 0x0FC3  # TIBETAN CANTILLATION SIGN SBUB -CHAL
    LineBreakClass::ULB_AL, // 0x0FC4  # TIBETAN SYMBOL DRIL BU
    LineBreakClass::ULB_AL, // 0x0FC5  # TIBETAN SYMBOL RDO RJE
    LineBreakClass::ULB_CM, // 0x0FC6  # TIBETAN SYMBOL PADMA GDAN
    LineBreakClass::ULB_AL, // 0x0FC7  # TIBETAN SYMBOL RDO RJE RGYA GRAM
    LineBreakClass::ULB_AL, // 0x0FC8  # TIBETAN SYMBOL PHUR PA
    LineBreakClass::ULB_AL, // 0x0FC9  # TIBETAN SYMBOL NOR BU
    LineBreakClass::ULB_AL, // 0x0FCA  # TIBETAN SYMBOL NOR BU NYIS -KHYIL
    LineBreakClass::ULB_AL, // 0x0FCB  # TIBETAN SYMBOL NOR BU GSUM -KHYIL
    LineBreakClass::ULB_AL, // 0x0FCC  # TIBETAN SYMBOL NOR BU BZHI -KHYIL
    LineBreakClass::ULB_ID, // 0x0FCD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FCE # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x0FCF  # TIBETAN SIGN RDEL NAG GSUM
    LineBreakClass::ULB_BB, // 0x0FD0  # TIBETAN MARK BSKA- SHOG GI MGO RGYAN
    LineBreakClass::ULB_BB, // 0x0FD1  # TIBETAN MARK MNYAM YIG GI MGO RGYAN
    LineBreakClass::ULB_ID, // 0x0FD2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FD3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FD4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FD5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FD6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FD7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FD8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FD9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FDA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FDB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FDC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FDD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FDE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FDF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FE0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FE1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FE2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FE3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FE4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FE5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FE6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FE7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FE8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FE9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FEA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FEB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FEC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FED # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FEE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FEF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FF0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FF1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FF2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FF3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FF4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FF5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FF6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FF7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FF8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FF9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FFA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FFB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FFC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FFD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FFE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x0FFF # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x1000  # MYANMAR LETTER KA
    LineBreakClass::ULB_SA, // 0x1001  # MYANMAR LETTER KHA
    LineBreakClass::ULB_SA, // 0x1002  # MYANMAR LETTER GA
    LineBreakClass::ULB_SA, // 0x1003  # MYANMAR LETTER GHA
    LineBreakClass::ULB_SA, // 0x1004  # MYANMAR LETTER NGA
    LineBreakClass::ULB_SA, // 0x1005  # MYANMAR LETTER CA
    LineBreakClass::ULB_SA, // 0x1006  # MYANMAR LETTER CHA
    LineBreakClass::ULB_SA, // 0x1007  # MYANMAR LETTER JA
    LineBreakClass::ULB_SA, // 0x1008  # MYANMAR LETTER JHA
    LineBreakClass::ULB_SA, // 0x1009  # MYANMAR LETTER NYA
    LineBreakClass::ULB_SA, // 0x100A  # MYANMAR LETTER NNYA
    LineBreakClass::ULB_SA, // 0x100B  # MYANMAR LETTER TTA
    LineBreakClass::ULB_SA, // 0x100C  # MYANMAR LETTER TTHA
    LineBreakClass::ULB_SA, // 0x100D  # MYANMAR LETTER DDA
    LineBreakClass::ULB_SA, // 0x100E  # MYANMAR LETTER DDHA
    LineBreakClass::ULB_SA, // 0x100F  # MYANMAR LETTER NNA
    LineBreakClass::ULB_SA, // 0x1010  # MYANMAR LETTER TA
    LineBreakClass::ULB_SA, // 0x1011  # MYANMAR LETTER THA
    LineBreakClass::ULB_SA, // 0x1012  # MYANMAR LETTER DA
    LineBreakClass::ULB_SA, // 0x1013  # MYANMAR LETTER DHA
    LineBreakClass::ULB_SA, // 0x1014  # MYANMAR LETTER NA
    LineBreakClass::ULB_SA, // 0x1015  # MYANMAR LETTER PA
    LineBreakClass::ULB_SA, // 0x1016  # MYANMAR LETTER PHA
    LineBreakClass::ULB_SA, // 0x1017  # MYANMAR LETTER BA
    LineBreakClass::ULB_SA, // 0x1018  # MYANMAR LETTER BHA
    LineBreakClass::ULB_SA, // 0x1019  # MYANMAR LETTER MA
    LineBreakClass::ULB_SA, // 0x101A  # MYANMAR LETTER YA
    LineBreakClass::ULB_SA, // 0x101B  # MYANMAR LETTER RA
    LineBreakClass::ULB_SA, // 0x101C  # MYANMAR LETTER LA
    LineBreakClass::ULB_SA, // 0x101D  # MYANMAR LETTER WA
    LineBreakClass::ULB_SA, // 0x101E  # MYANMAR LETTER SA
    LineBreakClass::ULB_SA, // 0x101F  # MYANMAR LETTER HA
    LineBreakClass::ULB_SA, // 0x1020  # MYANMAR LETTER LLA
    LineBreakClass::ULB_SA, // 0x1021  # MYANMAR LETTER A
    LineBreakClass::ULB_ID, // 0x1022 # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x1023  # MYANMAR LETTER I
    LineBreakClass::ULB_SA, // 0x1024  # MYANMAR LETTER II
    LineBreakClass::ULB_SA, // 0x1025  # MYANMAR LETTER U
    LineBreakClass::ULB_SA, // 0x1026  # MYANMAR LETTER UU
    LineBreakClass::ULB_SA, // 0x1027  # MYANMAR LETTER E
    LineBreakClass::ULB_ID, // 0x1028 # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x1029  # MYANMAR LETTER O
    LineBreakClass::ULB_SA, // 0x102A  # MYANMAR LETTER AU
    LineBreakClass::ULB_ID, // 0x102B # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x102C  # MYANMAR VOWEL SIGN AA
    LineBreakClass::ULB_SA, // 0x102D  # MYANMAR VOWEL SIGN I
    LineBreakClass::ULB_SA, // 0x102E  # MYANMAR VOWEL SIGN II
    LineBreakClass::ULB_SA, // 0x102F  # MYANMAR VOWEL SIGN U
    LineBreakClass::ULB_SA, // 0x1030  # MYANMAR VOWEL SIGN UU
    LineBreakClass::ULB_SA, // 0x1031  # MYANMAR VOWEL SIGN E
    LineBreakClass::ULB_SA, // 0x1032  # MYANMAR VOWEL SIGN AI
    LineBreakClass::ULB_ID, // 0x1033 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1034 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1035 # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x1036  # MYANMAR SIGN ANUSVARA
    LineBreakClass::ULB_SA, // 0x1037  # MYANMAR SIGN DOT BELOW
    LineBreakClass::ULB_SA, // 0x1038  # MYANMAR SIGN VISARGA
    LineBreakClass::ULB_SA, // 0x1039  # MYANMAR SIGN VIRAMA
    LineBreakClass::ULB_ID, // 0x103A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x103B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x103C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x103D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x103E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x103F # <UNDEFINED>
    LineBreakClass::ULB_NU, // 0x1040  # MYANMAR DIGIT ZERO
    LineBreakClass::ULB_NU, // 0x1041  # MYANMAR DIGIT ONE
    LineBreakClass::ULB_NU, // 0x1042  # MYANMAR DIGIT TWO
    LineBreakClass::ULB_NU, // 0x1043  # MYANMAR DIGIT THREE
    LineBreakClass::ULB_NU, // 0x1044  # MYANMAR DIGIT FOUR
    LineBreakClass::ULB_NU, // 0x1045  # MYANMAR DIGIT FIVE
    LineBreakClass::ULB_NU, // 0x1046  # MYANMAR DIGIT SIX
    LineBreakClass::ULB_NU, // 0x1047  # MYANMAR DIGIT SEVEN
    LineBreakClass::ULB_NU, // 0x1048  # MYANMAR DIGIT EIGHT
    LineBreakClass::ULB_NU, // 0x1049  # MYANMAR DIGIT NINE
    LineBreakClass::ULB_BA, // 0x104A  # MYANMAR SIGN LITTLE SECTION
    LineBreakClass::ULB_BA, // 0x104B  # MYANMAR SIGN SECTION
    LineBreakClass::ULB_AL, // 0x104C  # MYANMAR SYMBOL LOCATIVE
    LineBreakClass::ULB_AL, // 0x104D  # MYANMAR SYMBOL COMPLETED
    LineBreakClass::ULB_AL, // 0x104E  # MYANMAR SYMBOL AFOREMENTIONED
    LineBreakClass::ULB_AL, // 0x104F  # MYANMAR SYMBOL GENITIVE
    LineBreakClass::ULB_SA, // 0x1050  # MYANMAR LETTER SHA
    LineBreakClass::ULB_SA, // 0x1051  # MYANMAR LETTER SSA
    LineBreakClass::ULB_SA, // 0x1052  # MYANMAR LETTER VOCALIC R
    LineBreakClass::ULB_SA, // 0x1053  # MYANMAR LETTER VOCALIC RR
    LineBreakClass::ULB_SA, // 0x1054  # MYANMAR LETTER VOCALIC L
    LineBreakClass::ULB_SA, // 0x1055  # MYANMAR LETTER VOCALIC LL
    LineBreakClass::ULB_SA, // 0x1056  # MYANMAR VOWEL SIGN VOCALIC R
    LineBreakClass::ULB_SA, // 0x1057  # MYANMAR VOWEL SIGN VOCALIC RR
    LineBreakClass::ULB_SA, // 0x1058  # MYANMAR VOWEL SIGN VOCALIC L
    LineBreakClass::ULB_SA, // 0x1059  # MYANMAR VOWEL SIGN VOCALIC LL
    LineBreakClass::ULB_ID, // 0x105A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x105B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x105C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x105D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x105E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x105F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1060 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1061 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1062 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1063 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1064 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1065 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1066 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1067 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1068 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1069 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x106A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x106B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x106C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x106D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x106E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x106F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1070 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1071 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1072 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1073 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1074 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1075 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1076 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1077 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1078 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1079 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x107A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x107B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x107C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x107D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x107E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x107F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1080 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1081 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1082 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1083 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1084 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1085 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1086 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1087 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1088 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1089 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x108A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x108B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x108C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x108D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x108E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x108F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1090 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1091 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1092 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1093 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1094 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1095 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1096 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1097 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1098 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1099 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x109A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x109B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x109C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x109D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x109E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x109F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x10A0  # GEORGIAN CAPITAL LETTER AN
    LineBreakClass::ULB_AL, // 0x10A1  # GEORGIAN CAPITAL LETTER BAN
    LineBreakClass::ULB_AL, // 0x10A2  # GEORGIAN CAPITAL LETTER GAN
    LineBreakClass::ULB_AL, // 0x10A3  # GEORGIAN CAPITAL LETTER DON
    LineBreakClass::ULB_AL, // 0x10A4  # GEORGIAN CAPITAL LETTER EN
    LineBreakClass::ULB_AL, // 0x10A5  # GEORGIAN CAPITAL LETTER VIN
    LineBreakClass::ULB_AL, // 0x10A6  # GEORGIAN CAPITAL LETTER ZEN
    LineBreakClass::ULB_AL, // 0x10A7  # GEORGIAN CAPITAL LETTER TAN
    LineBreakClass::ULB_AL, // 0x10A8  # GEORGIAN CAPITAL LETTER IN
    LineBreakClass::ULB_AL, // 0x10A9  # GEORGIAN CAPITAL LETTER KAN
    LineBreakClass::ULB_AL, // 0x10AA  # GEORGIAN CAPITAL LETTER LAS
    LineBreakClass::ULB_AL, // 0x10AB  # GEORGIAN CAPITAL LETTER MAN
    LineBreakClass::ULB_AL, // 0x10AC  # GEORGIAN CAPITAL LETTER NAR
    LineBreakClass::ULB_AL, // 0x10AD  # GEORGIAN CAPITAL LETTER ON
    LineBreakClass::ULB_AL, // 0x10AE  # GEORGIAN CAPITAL LETTER PAR
    LineBreakClass::ULB_AL, // 0x10AF  # GEORGIAN CAPITAL LETTER ZHAR
    LineBreakClass::ULB_AL, // 0x10B0  # GEORGIAN CAPITAL LETTER RAE
    LineBreakClass::ULB_AL, // 0x10B1  # GEORGIAN CAPITAL LETTER SAN
    LineBreakClass::ULB_AL, // 0x10B2  # GEORGIAN CAPITAL LETTER TAR
    LineBreakClass::ULB_AL, // 0x10B3  # GEORGIAN CAPITAL LETTER UN
    LineBreakClass::ULB_AL, // 0x10B4  # GEORGIAN CAPITAL LETTER PHAR
    LineBreakClass::ULB_AL, // 0x10B5  # GEORGIAN CAPITAL LETTER KHAR
    LineBreakClass::ULB_AL, // 0x10B6  # GEORGIAN CAPITAL LETTER GHAN
    LineBreakClass::ULB_AL, // 0x10B7  # GEORGIAN CAPITAL LETTER QAR
    LineBreakClass::ULB_AL, // 0x10B8  # GEORGIAN CAPITAL LETTER SHIN
    LineBreakClass::ULB_AL, // 0x10B9  # GEORGIAN CAPITAL LETTER CHIN
    LineBreakClass::ULB_AL, // 0x10BA  # GEORGIAN CAPITAL LETTER CAN
    LineBreakClass::ULB_AL, // 0x10BB  # GEORGIAN CAPITAL LETTER JIL
    LineBreakClass::ULB_AL, // 0x10BC  # GEORGIAN CAPITAL LETTER CIL
    LineBreakClass::ULB_AL, // 0x10BD  # GEORGIAN CAPITAL LETTER CHAR
    LineBreakClass::ULB_AL, // 0x10BE  # GEORGIAN CAPITAL LETTER XAN
    LineBreakClass::ULB_AL, // 0x10BF  # GEORGIAN CAPITAL LETTER JHAN
    LineBreakClass::ULB_AL, // 0x10C0  # GEORGIAN CAPITAL LETTER HAE
    LineBreakClass::ULB_AL, // 0x10C1  # GEORGIAN CAPITAL LETTER HE
    LineBreakClass::ULB_AL, // 0x10C2  # GEORGIAN CAPITAL LETTER HIE
    LineBreakClass::ULB_AL, // 0x10C3  # GEORGIAN CAPITAL LETTER WE
    LineBreakClass::ULB_AL, // 0x10C4  # GEORGIAN CAPITAL LETTER HAR
    LineBreakClass::ULB_AL, // 0x10C5  # GEORGIAN CAPITAL LETTER HOE
    LineBreakClass::ULB_ID, // 0x10C6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x10C7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x10C8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x10C9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x10CA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x10CB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x10CC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x10CD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x10CE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x10CF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x10D0  # GEORGIAN LETTER AN
    LineBreakClass::ULB_AL, // 0x10D1  # GEORGIAN LETTER BAN
    LineBreakClass::ULB_AL, // 0x10D2  # GEORGIAN LETTER GAN
    LineBreakClass::ULB_AL, // 0x10D3  # GEORGIAN LETTER DON
    LineBreakClass::ULB_AL, // 0x10D4  # GEORGIAN LETTER EN
    LineBreakClass::ULB_AL, // 0x10D5  # GEORGIAN LETTER VIN
    LineBreakClass::ULB_AL, // 0x10D6  # GEORGIAN LETTER ZEN
    LineBreakClass::ULB_AL, // 0x10D7  # GEORGIAN LETTER TAN
    LineBreakClass::ULB_AL, // 0x10D8  # GEORGIAN LETTER IN
    LineBreakClass::ULB_AL, // 0x10D9  # GEORGIAN LETTER KAN
    LineBreakClass::ULB_AL, // 0x10DA  # GEORGIAN LETTER LAS
    LineBreakClass::ULB_AL, // 0x10DB  # GEORGIAN LETTER MAN
    LineBreakClass::ULB_AL, // 0x10DC  # GEORGIAN LETTER NAR
    LineBreakClass::ULB_AL, // 0x10DD  # GEORGIAN LETTER ON
    LineBreakClass::ULB_AL, // 0x10DE  # GEORGIAN LETTER PAR
    LineBreakClass::ULB_AL, // 0x10DF  # GEORGIAN LETTER ZHAR
    LineBreakClass::ULB_AL, // 0x10E0  # GEORGIAN LETTER RAE
    LineBreakClass::ULB_AL, // 0x10E1  # GEORGIAN LETTER SAN
    LineBreakClass::ULB_AL, // 0x10E2  # GEORGIAN LETTER TAR
    LineBreakClass::ULB_AL, // 0x10E3  # GEORGIAN LETTER UN
    LineBreakClass::ULB_AL, // 0x10E4  # GEORGIAN LETTER PHAR
    LineBreakClass::ULB_AL, // 0x10E5  # GEORGIAN LETTER KHAR
    LineBreakClass::ULB_AL, // 0x10E6  # GEORGIAN LETTER GHAN
    LineBreakClass::ULB_AL, // 0x10E7  # GEORGIAN LETTER QAR
    LineBreakClass::ULB_AL, // 0x10E8  # GEORGIAN LETTER SHIN
    LineBreakClass::ULB_AL, // 0x10E9  # GEORGIAN LETTER CHIN
    LineBreakClass::ULB_AL, // 0x10EA  # GEORGIAN LETTER CAN
    LineBreakClass::ULB_AL, // 0x10EB  # GEORGIAN LETTER JIL
    LineBreakClass::ULB_AL, // 0x10EC  # GEORGIAN LETTER CIL
    LineBreakClass::ULB_AL, // 0x10ED  # GEORGIAN LETTER CHAR
    LineBreakClass::ULB_AL, // 0x10EE  # GEORGIAN LETTER XAN
    LineBreakClass::ULB_AL, // 0x10EF  # GEORGIAN LETTER JHAN
    LineBreakClass::ULB_AL, // 0x10F0  # GEORGIAN LETTER HAE
    LineBreakClass::ULB_AL, // 0x10F1  # GEORGIAN LETTER HE
    LineBreakClass::ULB_AL, // 0x10F2  # GEORGIAN LETTER HIE
    LineBreakClass::ULB_AL, // 0x10F3  # GEORGIAN LETTER WE
    LineBreakClass::ULB_AL, // 0x10F4  # GEORGIAN LETTER HAR
    LineBreakClass::ULB_AL, // 0x10F5  # GEORGIAN LETTER HOE
    LineBreakClass::ULB_AL, // 0x10F6  # GEORGIAN LETTER FI
    LineBreakClass::ULB_AL, // 0x10F7  # GEORGIAN LETTER YN
    LineBreakClass::ULB_AL, // 0x10F8  # GEORGIAN LETTER ELIFI
    LineBreakClass::ULB_AL, // 0x10F9  # GEORGIAN LETTER TURNED GAN
    LineBreakClass::ULB_AL, // 0x10FA  # GEORGIAN LETTER AIN
    LineBreakClass::ULB_AL, // 0x10FB  # GEORGIAN PARAGRAPH SEPARATOR
    LineBreakClass::ULB_AL, // 0x10FC  # MODIFIER LETTER GEORGIAN NAR
    LineBreakClass::ULB_ID, // 0x10FD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x10FE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x10FF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1100  # HANGUL CHOSEONG KIYEOK
    LineBreakClass::ULB_ID, // 0x1101  # HANGUL CHOSEONG SSANGKIYEOK
    LineBreakClass::ULB_ID, // 0x1102  # HANGUL CHOSEONG NIEUN
    LineBreakClass::ULB_ID, // 0x1103  # HANGUL CHOSEONG TIKEUT
    LineBreakClass::ULB_ID, // 0x1104  # HANGUL CHOSEONG SSANGTIKEUT
    LineBreakClass::ULB_ID, // 0x1105  # HANGUL CHOSEONG RIEUL
    LineBreakClass::ULB_ID, // 0x1106  # HANGUL CHOSEONG MIEUM
    LineBreakClass::ULB_ID, // 0x1107  # HANGUL CHOSEONG PIEUP
    LineBreakClass::ULB_ID, // 0x1108  # HANGUL CHOSEONG SSANGPIEUP
    LineBreakClass::ULB_ID, // 0x1109  # HANGUL CHOSEONG SIOS
    LineBreakClass::ULB_ID, // 0x110A  # HANGUL CHOSEONG SSANGSIOS
    LineBreakClass::ULB_ID, // 0x110B  # HANGUL CHOSEONG IEUNG
    LineBreakClass::ULB_ID, // 0x110C  # HANGUL CHOSEONG CIEUC
    LineBreakClass::ULB_ID, // 0x110D  # HANGUL CHOSEONG SSANGCIEUC
    LineBreakClass::ULB_ID, // 0x110E  # HANGUL CHOSEONG CHIEUCH
    LineBreakClass::ULB_ID, // 0x110F  # HANGUL CHOSEONG KHIEUKH
    LineBreakClass::ULB_ID, // 0x1110  # HANGUL CHOSEONG THIEUTH
    LineBreakClass::ULB_ID, // 0x1111  # HANGUL CHOSEONG PHIEUPH
    LineBreakClass::ULB_ID, // 0x1112  # HANGUL CHOSEONG HIEUH
    LineBreakClass::ULB_ID, // 0x1113  # HANGUL CHOSEONG NIEUN-KIYEOK
    LineBreakClass::ULB_ID, // 0x1114  # HANGUL CHOSEONG SSANGNIEUN
    LineBreakClass::ULB_ID, // 0x1115  # HANGUL CHOSEONG NIEUN-TIKEUT
    LineBreakClass::ULB_ID, // 0x1116  # HANGUL CHOSEONG NIEUN-PIEUP
    LineBreakClass::ULB_ID, // 0x1117  # HANGUL CHOSEONG TIKEUT-KIYEOK
    LineBreakClass::ULB_ID, // 0x1118  # HANGUL CHOSEONG RIEUL-NIEUN
    LineBreakClass::ULB_ID, // 0x1119  # HANGUL CHOSEONG SSANGRIEUL
    LineBreakClass::ULB_ID, // 0x111A  # HANGUL CHOSEONG RIEUL-HIEUH
    LineBreakClass::ULB_ID, // 0x111B  # HANGUL CHOSEONG KAPYEOUNRIEUL
    LineBreakClass::ULB_ID, // 0x111C  # HANGUL CHOSEONG MIEUM-PIEUP
    LineBreakClass::ULB_ID, // 0x111D  # HANGUL CHOSEONG KAPYEOUNMIEUM
    LineBreakClass::ULB_ID, // 0x111E  # HANGUL CHOSEONG PIEUP-KIYEOK
    LineBreakClass::ULB_ID, // 0x111F  # HANGUL CHOSEONG PIEUP-NIEUN
    LineBreakClass::ULB_ID, // 0x1120  # HANGUL CHOSEONG PIEUP-TIKEUT
    LineBreakClass::ULB_ID, // 0x1121  # HANGUL CHOSEONG PIEUP-SIOS
    LineBreakClass::ULB_ID, // 0x1122  # HANGUL CHOSEONG PIEUP-SIOS-KIYEOK
    LineBreakClass::ULB_ID, // 0x1123  # HANGUL CHOSEONG PIEUP-SIOS-TIKEUT
    LineBreakClass::ULB_ID, // 0x1124  # HANGUL CHOSEONG PIEUP-SIOS-PIEUP
    LineBreakClass::ULB_ID, // 0x1125  # HANGUL CHOSEONG PIEUP-SSANGSIOS
    LineBreakClass::ULB_ID, // 0x1126  # HANGUL CHOSEONG PIEUP-SIOS-CIEUC
    LineBreakClass::ULB_ID, // 0x1127  # HANGUL CHOSEONG PIEUP-CIEUC
    LineBreakClass::ULB_ID, // 0x1128  # HANGUL CHOSEONG PIEUP-CHIEUCH
    LineBreakClass::ULB_ID, // 0x1129  # HANGUL CHOSEONG PIEUP-THIEUTH
    LineBreakClass::ULB_ID, // 0x112A  # HANGUL CHOSEONG PIEUP-PHIEUPH
    LineBreakClass::ULB_ID, // 0x112B  # HANGUL CHOSEONG KAPYEOUNPIEUP
    LineBreakClass::ULB_ID, // 0x112C  # HANGUL CHOSEONG KAPYEOUNSSANGPIEUP
    LineBreakClass::ULB_ID, // 0x112D  # HANGUL CHOSEONG SIOS-KIYEOK
    LineBreakClass::ULB_ID, // 0x112E  # HANGUL CHOSEONG SIOS-NIEUN
    LineBreakClass::ULB_ID, // 0x112F  # HANGUL CHOSEONG SIOS-TIKEUT
    LineBreakClass::ULB_ID, // 0x1130  # HANGUL CHOSEONG SIOS-RIEUL
    LineBreakClass::ULB_ID, // 0x1131  # HANGUL CHOSEONG SIOS-MIEUM
    LineBreakClass::ULB_ID, // 0x1132  # HANGUL CHOSEONG SIOS-PIEUP
    LineBreakClass::ULB_ID, // 0x1133  # HANGUL CHOSEONG SIOS-PIEUP-KIYEOK
    LineBreakClass::ULB_ID, // 0x1134  # HANGUL CHOSEONG SIOS-SSANGSIOS
    LineBreakClass::ULB_ID, // 0x1135  # HANGUL CHOSEONG SIOS-IEUNG
    LineBreakClass::ULB_ID, // 0x1136  # HANGUL CHOSEONG SIOS-CIEUC
    LineBreakClass::ULB_ID, // 0x1137  # HANGUL CHOSEONG SIOS-CHIEUCH
    LineBreakClass::ULB_ID, // 0x1138  # HANGUL CHOSEONG SIOS-KHIEUKH
    LineBreakClass::ULB_ID, // 0x1139  # HANGUL CHOSEONG SIOS-THIEUTH
    LineBreakClass::ULB_ID, // 0x113A  # HANGUL CHOSEONG SIOS-PHIEUPH
    LineBreakClass::ULB_ID, // 0x113B  # HANGUL CHOSEONG SIOS-HIEUH
    LineBreakClass::ULB_ID, // 0x113C  # HANGUL CHOSEONG CHITUEUMSIOS
    LineBreakClass::ULB_ID, // 0x113D  # HANGUL CHOSEONG CHITUEUMSSANGSIOS
    LineBreakClass::ULB_ID, // 0x113E  # HANGUL CHOSEONG CEONGCHIEUMSIOS
    LineBreakClass::ULB_ID, // 0x113F  # HANGUL CHOSEONG CEONGCHIEUMSSANGSIOS
    LineBreakClass::ULB_ID, // 0x1140  # HANGUL CHOSEONG PANSIOS
    LineBreakClass::ULB_ID, // 0x1141  # HANGUL CHOSEONG IEUNG-KIYEOK
    LineBreakClass::ULB_ID, // 0x1142  # HANGUL CHOSEONG IEUNG-TIKEUT
    LineBreakClass::ULB_ID, // 0x1143  # HANGUL CHOSEONG IEUNG-MIEUM
    LineBreakClass::ULB_ID, // 0x1144  # HANGUL CHOSEONG IEUNG-PIEUP
    LineBreakClass::ULB_ID, // 0x1145  # HANGUL CHOSEONG IEUNG-SIOS
    LineBreakClass::ULB_ID, // 0x1146  # HANGUL CHOSEONG IEUNG-PANSIOS
    LineBreakClass::ULB_ID, // 0x1147  # HANGUL CHOSEONG SSANGIEUNG
    LineBreakClass::ULB_ID, // 0x1148  # HANGUL CHOSEONG IEUNG-CIEUC
    LineBreakClass::ULB_ID, // 0x1149  # HANGUL CHOSEONG IEUNG-CHIEUCH
    LineBreakClass::ULB_ID, // 0x114A  # HANGUL CHOSEONG IEUNG-THIEUTH
    LineBreakClass::ULB_ID, // 0x114B  # HANGUL CHOSEONG IEUNG-PHIEUPH
    LineBreakClass::ULB_ID, // 0x114C  # HANGUL CHOSEONG YESIEUNG
    LineBreakClass::ULB_ID, // 0x114D  # HANGUL CHOSEONG CIEUC-IEUNG
    LineBreakClass::ULB_ID, // 0x114E  # HANGUL CHOSEONG CHITUEUMCIEUC
    LineBreakClass::ULB_ID, // 0x114F  # HANGUL CHOSEONG CHITUEUMSSANGCIEUC
    LineBreakClass::ULB_ID, // 0x1150  # HANGUL CHOSEONG CEONGCHIEUMCIEUC
    LineBreakClass::ULB_ID, // 0x1151  # HANGUL CHOSEONG CEONGCHIEUMSSANGCIEUC
    LineBreakClass::ULB_ID, // 0x1152  # HANGUL CHOSEONG CHIEUCH-KHIEUKH
    LineBreakClass::ULB_ID, // 0x1153  # HANGUL CHOSEONG CHIEUCH-HIEUH
    LineBreakClass::ULB_ID, // 0x1154  # HANGUL CHOSEONG CHITUEUMCHIEUCH
    LineBreakClass::ULB_ID, // 0x1155  # HANGUL CHOSEONG CEONGCHIEUMCHIEUCH
    LineBreakClass::ULB_ID, // 0x1156  # HANGUL CHOSEONG PHIEUPH-PIEUP
    LineBreakClass::ULB_ID, // 0x1157  # HANGUL CHOSEONG KAPYEOUNPHIEUPH
    LineBreakClass::ULB_ID, // 0x1158  # HANGUL CHOSEONG SSANGHIEUH
    LineBreakClass::ULB_ID, // 0x1159  # HANGUL CHOSEONG YEORINHIEUH
    LineBreakClass::ULB_ID, // 0x115A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x115B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x115C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x115D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x115E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x115F  # HANGUL CHOSEONG FILLER
    LineBreakClass::ULB_ID, // 0x1160  # HANGUL JUNGSEONG FILLER
    LineBreakClass::ULB_ID, // 0x1161  # HANGUL JUNGSEONG A
    LineBreakClass::ULB_ID, // 0x1162  # HANGUL JUNGSEONG AE
    LineBreakClass::ULB_ID, // 0x1163  # HANGUL JUNGSEONG YA
    LineBreakClass::ULB_ID, // 0x1164  # HANGUL JUNGSEONG YAE
    LineBreakClass::ULB_ID, // 0x1165  # HANGUL JUNGSEONG EO
    LineBreakClass::ULB_ID, // 0x1166  # HANGUL JUNGSEONG E
    LineBreakClass::ULB_ID, // 0x1167  # HANGUL JUNGSEONG YEO
    LineBreakClass::ULB_ID, // 0x1168  # HANGUL JUNGSEONG YE
    LineBreakClass::ULB_ID, // 0x1169  # HANGUL JUNGSEONG O
    LineBreakClass::ULB_ID, // 0x116A  # HANGUL JUNGSEONG WA
    LineBreakClass::ULB_ID, // 0x116B  # HANGUL JUNGSEONG WAE
    LineBreakClass::ULB_ID, // 0x116C  # HANGUL JUNGSEONG OE
    LineBreakClass::ULB_ID, // 0x116D  # HANGUL JUNGSEONG YO
    LineBreakClass::ULB_ID, // 0x116E  # HANGUL JUNGSEONG U
    LineBreakClass::ULB_ID, // 0x116F  # HANGUL JUNGSEONG WEO
    LineBreakClass::ULB_ID, // 0x1170  # HANGUL JUNGSEONG WE
    LineBreakClass::ULB_ID, // 0x1171  # HANGUL JUNGSEONG WI
    LineBreakClass::ULB_ID, // 0x1172  # HANGUL JUNGSEONG YU
    LineBreakClass::ULB_ID, // 0x1173  # HANGUL JUNGSEONG EU
    LineBreakClass::ULB_ID, // 0x1174  # HANGUL JUNGSEONG YI
    LineBreakClass::ULB_ID, // 0x1175  # HANGUL JUNGSEONG I
    LineBreakClass::ULB_ID, // 0x1176  # HANGUL JUNGSEONG A-O
    LineBreakClass::ULB_ID, // 0x1177  # HANGUL JUNGSEONG A-U
    LineBreakClass::ULB_ID, // 0x1178  # HANGUL JUNGSEONG YA-O
    LineBreakClass::ULB_ID, // 0x1179  # HANGUL JUNGSEONG YA-YO
    LineBreakClass::ULB_ID, // 0x117A  # HANGUL JUNGSEONG EO-O
    LineBreakClass::ULB_ID, // 0x117B  # HANGUL JUNGSEONG EO-U
    LineBreakClass::ULB_ID, // 0x117C  # HANGUL JUNGSEONG EO-EU
    LineBreakClass::ULB_ID, // 0x117D  # HANGUL JUNGSEONG YEO-O
    LineBreakClass::ULB_ID, // 0x117E  # HANGUL JUNGSEONG YEO-U
    LineBreakClass::ULB_ID, // 0x117F  # HANGUL JUNGSEONG O-EO
    LineBreakClass::ULB_ID, // 0x1180  # HANGUL JUNGSEONG O-E
    LineBreakClass::ULB_ID, // 0x1181  # HANGUL JUNGSEONG O-YE
    LineBreakClass::ULB_ID, // 0x1182  # HANGUL JUNGSEONG O-O
    LineBreakClass::ULB_ID, // 0x1183  # HANGUL JUNGSEONG O-U
    LineBreakClass::ULB_ID, // 0x1184  # HANGUL JUNGSEONG YO-YA
    LineBreakClass::ULB_ID, // 0x1185  # HANGUL JUNGSEONG YO-YAE
    LineBreakClass::ULB_ID, // 0x1186  # HANGUL JUNGSEONG YO-YEO
    LineBreakClass::ULB_ID, // 0x1187  # HANGUL JUNGSEONG YO-O
    LineBreakClass::ULB_ID, // 0x1188  # HANGUL JUNGSEONG YO-I
    LineBreakClass::ULB_ID, // 0x1189  # HANGUL JUNGSEONG U-A
    LineBreakClass::ULB_ID, // 0x118A  # HANGUL JUNGSEONG U-AE
    LineBreakClass::ULB_ID, // 0x118B  # HANGUL JUNGSEONG U-EO-EU
    LineBreakClass::ULB_ID, // 0x118C  # HANGUL JUNGSEONG U-YE
    LineBreakClass::ULB_ID, // 0x118D  # HANGUL JUNGSEONG U-U
    LineBreakClass::ULB_ID, // 0x118E  # HANGUL JUNGSEONG YU-A
    LineBreakClass::ULB_ID, // 0x118F  # HANGUL JUNGSEONG YU-EO
    LineBreakClass::ULB_ID, // 0x1190  # HANGUL JUNGSEONG YU-E
    LineBreakClass::ULB_ID, // 0x1191  # HANGUL JUNGSEONG YU-YEO
    LineBreakClass::ULB_ID, // 0x1192  # HANGUL JUNGSEONG YU-YE
    LineBreakClass::ULB_ID, // 0x1193  # HANGUL JUNGSEONG YU-U
    LineBreakClass::ULB_ID, // 0x1194  # HANGUL JUNGSEONG YU-I
    LineBreakClass::ULB_ID, // 0x1195  # HANGUL JUNGSEONG EU-U
    LineBreakClass::ULB_ID, // 0x1196  # HANGUL JUNGSEONG EU-EU
    LineBreakClass::ULB_ID, // 0x1197  # HANGUL JUNGSEONG YI-U
    LineBreakClass::ULB_ID, // 0x1198  # HANGUL JUNGSEONG I-A
    LineBreakClass::ULB_ID, // 0x1199  # HANGUL JUNGSEONG I-YA
    LineBreakClass::ULB_ID, // 0x119A  # HANGUL JUNGSEONG I-O
    LineBreakClass::ULB_ID, // 0x119B  # HANGUL JUNGSEONG I-U
    LineBreakClass::ULB_ID, // 0x119C  # HANGUL JUNGSEONG I-EU
    LineBreakClass::ULB_ID, // 0x119D  # HANGUL JUNGSEONG I-ARAEA
    LineBreakClass::ULB_ID, // 0x119E  # HANGUL JUNGSEONG ARAEA
    LineBreakClass::ULB_ID, // 0x119F  # HANGUL JUNGSEONG ARAEA-EO
    LineBreakClass::ULB_ID, // 0x11A0  # HANGUL JUNGSEONG ARAEA-U
    LineBreakClass::ULB_ID, // 0x11A1  # HANGUL JUNGSEONG ARAEA-I
    LineBreakClass::ULB_ID, // 0x11A2  # HANGUL JUNGSEONG SSANGARAEA
    LineBreakClass::ULB_ID, // 0x11A3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x11A4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x11A5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x11A6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x11A7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x11A8  # HANGUL JONGSEONG KIYEOK
    LineBreakClass::ULB_ID, // 0x11A9  # HANGUL JONGSEONG SSANGKIYEOK
    LineBreakClass::ULB_ID, // 0x11AA  # HANGUL JONGSEONG KIYEOK-SIOS
    LineBreakClass::ULB_ID, // 0x11AB  # HANGUL JONGSEONG NIEUN
    LineBreakClass::ULB_ID, // 0x11AC  # HANGUL JONGSEONG NIEUN-CIEUC
    LineBreakClass::ULB_ID, // 0x11AD  # HANGUL JONGSEONG NIEUN-HIEUH
    LineBreakClass::ULB_ID, // 0x11AE  # HANGUL JONGSEONG TIKEUT
    LineBreakClass::ULB_ID, // 0x11AF  # HANGUL JONGSEONG RIEUL
    LineBreakClass::ULB_ID, // 0x11B0  # HANGUL JONGSEONG RIEUL-KIYEOK
    LineBreakClass::ULB_ID, // 0x11B1  # HANGUL JONGSEONG RIEUL-MIEUM
    LineBreakClass::ULB_ID, // 0x11B2  # HANGUL JONGSEONG RIEUL-PIEUP
    LineBreakClass::ULB_ID, // 0x11B3  # HANGUL JONGSEONG RIEUL-SIOS
    LineBreakClass::ULB_ID, // 0x11B4  # HANGUL JONGSEONG RIEUL-THIEUTH
    LineBreakClass::ULB_ID, // 0x11B5  # HANGUL JONGSEONG RIEUL-PHIEUPH
    LineBreakClass::ULB_ID, // 0x11B6  # HANGUL JONGSEONG RIEUL-HIEUH
    LineBreakClass::ULB_ID, // 0x11B7  # HANGUL JONGSEONG MIEUM
    LineBreakClass::ULB_ID, // 0x11B8  # HANGUL JONGSEONG PIEUP
    LineBreakClass::ULB_ID, // 0x11B9  # HANGUL JONGSEONG PIEUP-SIOS
    LineBreakClass::ULB_ID, // 0x11BA  # HANGUL JONGSEONG SIOS
    LineBreakClass::ULB_ID, // 0x11BB  # HANGUL JONGSEONG SSANGSIOS
    LineBreakClass::ULB_ID, // 0x11BC  # HANGUL JONGSEONG IEUNG
    LineBreakClass::ULB_ID, // 0x11BD  # HANGUL JONGSEONG CIEUC
    LineBreakClass::ULB_ID, // 0x11BE  # HANGUL JONGSEONG CHIEUCH
    LineBreakClass::ULB_ID, // 0x11BF  # HANGUL JONGSEONG KHIEUKH
    LineBreakClass::ULB_ID, // 0x11C0  # HANGUL JONGSEONG THIEUTH
    LineBreakClass::ULB_ID, // 0x11C1  # HANGUL JONGSEONG PHIEUPH
    LineBreakClass::ULB_ID, // 0x11C2  # HANGUL JONGSEONG HIEUH
    LineBreakClass::ULB_ID, // 0x11C3  # HANGUL JONGSEONG KIYEOK-RIEUL
    LineBreakClass::ULB_ID, // 0x11C4  # HANGUL JONGSEONG KIYEOK-SIOS-KIYEOK
    LineBreakClass::ULB_ID, // 0x11C5  # HANGUL JONGSEONG NIEUN-KIYEOK
    LineBreakClass::ULB_ID, // 0x11C6  # HANGUL JONGSEONG NIEUN-TIKEUT
    LineBreakClass::ULB_ID, // 0x11C7  # HANGUL JONGSEONG NIEUN-SIOS
    LineBreakClass::ULB_ID, // 0x11C8  # HANGUL JONGSEONG NIEUN-PANSIOS
    LineBreakClass::ULB_ID, // 0x11C9  # HANGUL JONGSEONG NIEUN-THIEUTH
    LineBreakClass::ULB_ID, // 0x11CA  # HANGUL JONGSEONG TIKEUT-KIYEOK
    LineBreakClass::ULB_ID, // 0x11CB  # HANGUL JONGSEONG TIKEUT-RIEUL
    LineBreakClass::ULB_ID, // 0x11CC  # HANGUL JONGSEONG RIEUL-KIYEOK-SIOS
    LineBreakClass::ULB_ID, // 0x11CD  # HANGUL JONGSEONG RIEUL-NIEUN
    LineBreakClass::ULB_ID, // 0x11CE  # HANGUL JONGSEONG RIEUL-TIKEUT
    LineBreakClass::ULB_ID, // 0x11CF  # HANGUL JONGSEONG RIEUL-TIKEUT-HIEUH
    LineBreakClass::ULB_ID, // 0x11D0  # HANGUL JONGSEONG SSANGRIEUL
    LineBreakClass::ULB_ID, // 0x11D1  # HANGUL JONGSEONG RIEUL-MIEUM-KIYEOK
    LineBreakClass::ULB_ID, // 0x11D2  # HANGUL JONGSEONG RIEUL-MIEUM-SIOS
    LineBreakClass::ULB_ID, // 0x11D3  # HANGUL JONGSEONG RIEUL-PIEUP-SIOS
    LineBreakClass::ULB_ID, // 0x11D4  # HANGUL JONGSEONG RIEUL-PIEUP-HIEUH
    LineBreakClass::ULB_ID, // 0x11D5  # HANGUL JONGSEONG RIEUL-KAPYEOUNPIEUP
    LineBreakClass::ULB_ID, // 0x11D6  # HANGUL JONGSEONG RIEUL-SSANGSIOS
    LineBreakClass::ULB_ID, // 0x11D7  # HANGUL JONGSEONG RIEUL-PANSIOS
    LineBreakClass::ULB_ID, // 0x11D8  # HANGUL JONGSEONG RIEUL-KHIEUKH
    LineBreakClass::ULB_ID, // 0x11D9  # HANGUL JONGSEONG RIEUL-YEORINHIEUH
    LineBreakClass::ULB_ID, // 0x11DA  # HANGUL JONGSEONG MIEUM-KIYEOK
    LineBreakClass::ULB_ID, // 0x11DB  # HANGUL JONGSEONG MIEUM-RIEUL
    LineBreakClass::ULB_ID, // 0x11DC  # HANGUL JONGSEONG MIEUM-PIEUP
    LineBreakClass::ULB_ID, // 0x11DD  # HANGUL JONGSEONG MIEUM-SIOS
    LineBreakClass::ULB_ID, // 0x11DE  # HANGUL JONGSEONG MIEUM-SSANGSIOS
    LineBreakClass::ULB_ID, // 0x11DF  # HANGUL JONGSEONG MIEUM-PANSIOS
    LineBreakClass::ULB_ID, // 0x11E0  # HANGUL JONGSEONG MIEUM-CHIEUCH
    LineBreakClass::ULB_ID, // 0x11E1  # HANGUL JONGSEONG MIEUM-HIEUH
    LineBreakClass::ULB_ID, // 0x11E2  # HANGUL JONGSEONG KAPYEOUNMIEUM
    LineBreakClass::ULB_ID, // 0x11E3  # HANGUL JONGSEONG PIEUP-RIEUL
    LineBreakClass::ULB_ID, // 0x11E4  # HANGUL JONGSEONG PIEUP-PHIEUPH
    LineBreakClass::ULB_ID, // 0x11E5  # HANGUL JONGSEONG PIEUP-HIEUH
    LineBreakClass::ULB_ID, // 0x11E6  # HANGUL JONGSEONG KAPYEOUNPIEUP
    LineBreakClass::ULB_ID, // 0x11E7  # HANGUL JONGSEONG SIOS-KIYEOK
    LineBreakClass::ULB_ID, // 0x11E8  # HANGUL JONGSEONG SIOS-TIKEUT
    LineBreakClass::ULB_ID, // 0x11E9  # HANGUL JONGSEONG SIOS-RIEUL
    LineBreakClass::ULB_ID, // 0x11EA  # HANGUL JONGSEONG SIOS-PIEUP
    LineBreakClass::ULB_ID, // 0x11EB  # HANGUL JONGSEONG PANSIOS
    LineBreakClass::ULB_ID, // 0x11EC  # HANGUL JONGSEONG IEUNG-KIYEOK
    LineBreakClass::ULB_ID, // 0x11ED  # HANGUL JONGSEONG IEUNG-SSANGKIYEOK
    LineBreakClass::ULB_ID, // 0x11EE  # HANGUL JONGSEONG SSANGIEUNG
    LineBreakClass::ULB_ID, // 0x11EF  # HANGUL JONGSEONG IEUNG-KHIEUKH
    LineBreakClass::ULB_ID, // 0x11F0  # HANGUL JONGSEONG YESIEUNG
    LineBreakClass::ULB_ID, // 0x11F1  # HANGUL JONGSEONG YESIEUNG-SIOS
    LineBreakClass::ULB_ID, // 0x11F2  # HANGUL JONGSEONG YESIEUNG-PANSIOS
    LineBreakClass::ULB_ID, // 0x11F3  # HANGUL JONGSEONG PHIEUPH-PIEUP
    LineBreakClass::ULB_ID, // 0x11F4  # HANGUL JONGSEONG KAPYEOUNPHIEUPH
    LineBreakClass::ULB_ID, // 0x11F5  # HANGUL JONGSEONG HIEUH-NIEUN
    LineBreakClass::ULB_ID, // 0x11F6  # HANGUL JONGSEONG HIEUH-RIEUL
    LineBreakClass::ULB_ID, // 0x11F7  # HANGUL JONGSEONG HIEUH-MIEUM
    LineBreakClass::ULB_ID, // 0x11F8  # HANGUL JONGSEONG HIEUH-PIEUP
    LineBreakClass::ULB_ID, // 0x11F9  # HANGUL JONGSEONG YEORINHIEUH
    LineBreakClass::ULB_ID, // 0x11FA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x11FB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x11FC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x11FD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x11FE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x11FF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1200  # ETHIOPIC SYLLABLE HA
    LineBreakClass::ULB_AL, // 0x1201  # ETHIOPIC SYLLABLE HU
    LineBreakClass::ULB_AL, // 0x1202  # ETHIOPIC SYLLABLE HI
    LineBreakClass::ULB_AL, // 0x1203  # ETHIOPIC SYLLABLE HAA
    LineBreakClass::ULB_AL, // 0x1204  # ETHIOPIC SYLLABLE HEE
    LineBreakClass::ULB_AL, // 0x1205  # ETHIOPIC SYLLABLE HE
    LineBreakClass::ULB_AL, // 0x1206  # ETHIOPIC SYLLABLE HO
    LineBreakClass::ULB_AL, // 0x1207  # ETHIOPIC SYLLABLE HOA
    LineBreakClass::ULB_AL, // 0x1208  # ETHIOPIC SYLLABLE LA
    LineBreakClass::ULB_AL, // 0x1209  # ETHIOPIC SYLLABLE LU
    LineBreakClass::ULB_AL, // 0x120A  # ETHIOPIC SYLLABLE LI
    LineBreakClass::ULB_AL, // 0x120B  # ETHIOPIC SYLLABLE LAA
    LineBreakClass::ULB_AL, // 0x120C  # ETHIOPIC SYLLABLE LEE
    LineBreakClass::ULB_AL, // 0x120D  # ETHIOPIC SYLLABLE LE
    LineBreakClass::ULB_AL, // 0x120E  # ETHIOPIC SYLLABLE LO
    LineBreakClass::ULB_AL, // 0x120F  # ETHIOPIC SYLLABLE LWA
    LineBreakClass::ULB_AL, // 0x1210  # ETHIOPIC SYLLABLE HHA
    LineBreakClass::ULB_AL, // 0x1211  # ETHIOPIC SYLLABLE HHU
    LineBreakClass::ULB_AL, // 0x1212  # ETHIOPIC SYLLABLE HHI
    LineBreakClass::ULB_AL, // 0x1213  # ETHIOPIC SYLLABLE HHAA
    LineBreakClass::ULB_AL, // 0x1214  # ETHIOPIC SYLLABLE HHEE
    LineBreakClass::ULB_AL, // 0x1215  # ETHIOPIC SYLLABLE HHE
    LineBreakClass::ULB_AL, // 0x1216  # ETHIOPIC SYLLABLE HHO
    LineBreakClass::ULB_AL, // 0x1217  # ETHIOPIC SYLLABLE HHWA
    LineBreakClass::ULB_AL, // 0x1218  # ETHIOPIC SYLLABLE MA
    LineBreakClass::ULB_AL, // 0x1219  # ETHIOPIC SYLLABLE MU
    LineBreakClass::ULB_AL, // 0x121A  # ETHIOPIC SYLLABLE MI
    LineBreakClass::ULB_AL, // 0x121B  # ETHIOPIC SYLLABLE MAA
    LineBreakClass::ULB_AL, // 0x121C  # ETHIOPIC SYLLABLE MEE
    LineBreakClass::ULB_AL, // 0x121D  # ETHIOPIC SYLLABLE ME
    LineBreakClass::ULB_AL, // 0x121E  # ETHIOPIC SYLLABLE MO
    LineBreakClass::ULB_AL, // 0x121F  # ETHIOPIC SYLLABLE MWA
    LineBreakClass::ULB_AL, // 0x1220  # ETHIOPIC SYLLABLE SZA
    LineBreakClass::ULB_AL, // 0x1221  # ETHIOPIC SYLLABLE SZU
    LineBreakClass::ULB_AL, // 0x1222  # ETHIOPIC SYLLABLE SZI
    LineBreakClass::ULB_AL, // 0x1223  # ETHIOPIC SYLLABLE SZAA
    LineBreakClass::ULB_AL, // 0x1224  # ETHIOPIC SYLLABLE SZEE
    LineBreakClass::ULB_AL, // 0x1225  # ETHIOPIC SYLLABLE SZE
    LineBreakClass::ULB_AL, // 0x1226  # ETHIOPIC SYLLABLE SZO
    LineBreakClass::ULB_AL, // 0x1227  # ETHIOPIC SYLLABLE SZWA
    LineBreakClass::ULB_AL, // 0x1228  # ETHIOPIC SYLLABLE RA
    LineBreakClass::ULB_AL, // 0x1229  # ETHIOPIC SYLLABLE RU
    LineBreakClass::ULB_AL, // 0x122A  # ETHIOPIC SYLLABLE RI
    LineBreakClass::ULB_AL, // 0x122B  # ETHIOPIC SYLLABLE RAA
    LineBreakClass::ULB_AL, // 0x122C  # ETHIOPIC SYLLABLE REE
    LineBreakClass::ULB_AL, // 0x122D  # ETHIOPIC SYLLABLE RE
    LineBreakClass::ULB_AL, // 0x122E  # ETHIOPIC SYLLABLE RO
    LineBreakClass::ULB_AL, // 0x122F  # ETHIOPIC SYLLABLE RWA
    LineBreakClass::ULB_AL, // 0x1230  # ETHIOPIC SYLLABLE SA
    LineBreakClass::ULB_AL, // 0x1231  # ETHIOPIC SYLLABLE SU
    LineBreakClass::ULB_AL, // 0x1232  # ETHIOPIC SYLLABLE SI
    LineBreakClass::ULB_AL, // 0x1233  # ETHIOPIC SYLLABLE SAA
    LineBreakClass::ULB_AL, // 0x1234  # ETHIOPIC SYLLABLE SEE
    LineBreakClass::ULB_AL, // 0x1235  # ETHIOPIC SYLLABLE SE
    LineBreakClass::ULB_AL, // 0x1236  # ETHIOPIC SYLLABLE SO
    LineBreakClass::ULB_AL, // 0x1237  # ETHIOPIC SYLLABLE SWA
    LineBreakClass::ULB_AL, // 0x1238  # ETHIOPIC SYLLABLE SHA
    LineBreakClass::ULB_AL, // 0x1239  # ETHIOPIC SYLLABLE SHU
    LineBreakClass::ULB_AL, // 0x123A  # ETHIOPIC SYLLABLE SHI
    LineBreakClass::ULB_AL, // 0x123B  # ETHIOPIC SYLLABLE SHAA
    LineBreakClass::ULB_AL, // 0x123C  # ETHIOPIC SYLLABLE SHEE
    LineBreakClass::ULB_AL, // 0x123D  # ETHIOPIC SYLLABLE SHE
    LineBreakClass::ULB_AL, // 0x123E  # ETHIOPIC SYLLABLE SHO
    LineBreakClass::ULB_AL, // 0x123F  # ETHIOPIC SYLLABLE SHWA
    LineBreakClass::ULB_AL, // 0x1240  # ETHIOPIC SYLLABLE QA
    LineBreakClass::ULB_AL, // 0x1241  # ETHIOPIC SYLLABLE QU
    LineBreakClass::ULB_AL, // 0x1242  # ETHIOPIC SYLLABLE QI
    LineBreakClass::ULB_AL, // 0x1243  # ETHIOPIC SYLLABLE QAA
    LineBreakClass::ULB_AL, // 0x1244  # ETHIOPIC SYLLABLE QEE
    LineBreakClass::ULB_AL, // 0x1245  # ETHIOPIC SYLLABLE QE
    LineBreakClass::ULB_AL, // 0x1246  # ETHIOPIC SYLLABLE QO
    LineBreakClass::ULB_AL, // 0x1247  # ETHIOPIC SYLLABLE QOA
    LineBreakClass::ULB_AL, // 0x1248  # ETHIOPIC SYLLABLE QWA
    LineBreakClass::ULB_ID, // 0x1249 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x124A  # ETHIOPIC SYLLABLE QWI
    LineBreakClass::ULB_AL, // 0x124B  # ETHIOPIC SYLLABLE QWAA
    LineBreakClass::ULB_AL, // 0x124C  # ETHIOPIC SYLLABLE QWEE
    LineBreakClass::ULB_AL, // 0x124D  # ETHIOPIC SYLLABLE QWE
    LineBreakClass::ULB_ID, // 0x124E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x124F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1250  # ETHIOPIC SYLLABLE QHA
    LineBreakClass::ULB_AL, // 0x1251  # ETHIOPIC SYLLABLE QHU
    LineBreakClass::ULB_AL, // 0x1252  # ETHIOPIC SYLLABLE QHI
    LineBreakClass::ULB_AL, // 0x1253  # ETHIOPIC SYLLABLE QHAA
    LineBreakClass::ULB_AL, // 0x1254  # ETHIOPIC SYLLABLE QHEE
    LineBreakClass::ULB_AL, // 0x1255  # ETHIOPIC SYLLABLE QHE
    LineBreakClass::ULB_AL, // 0x1256  # ETHIOPIC SYLLABLE QHO
    LineBreakClass::ULB_ID, // 0x1257 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1258  # ETHIOPIC SYLLABLE QHWA
    LineBreakClass::ULB_ID, // 0x1259 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x125A  # ETHIOPIC SYLLABLE QHWI
    LineBreakClass::ULB_AL, // 0x125B  # ETHIOPIC SYLLABLE QHWAA
    LineBreakClass::ULB_AL, // 0x125C  # ETHIOPIC SYLLABLE QHWEE
    LineBreakClass::ULB_AL, // 0x125D  # ETHIOPIC SYLLABLE QHWE
    LineBreakClass::ULB_ID, // 0x125E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x125F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1260  # ETHIOPIC SYLLABLE BA
    LineBreakClass::ULB_AL, // 0x1261  # ETHIOPIC SYLLABLE BU
    LineBreakClass::ULB_AL, // 0x1262  # ETHIOPIC SYLLABLE BI
    LineBreakClass::ULB_AL, // 0x1263  # ETHIOPIC SYLLABLE BAA
    LineBreakClass::ULB_AL, // 0x1264  # ETHIOPIC SYLLABLE BEE
    LineBreakClass::ULB_AL, // 0x1265  # ETHIOPIC SYLLABLE BE
    LineBreakClass::ULB_AL, // 0x1266  # ETHIOPIC SYLLABLE BO
    LineBreakClass::ULB_AL, // 0x1267  # ETHIOPIC SYLLABLE BWA
    LineBreakClass::ULB_AL, // 0x1268  # ETHIOPIC SYLLABLE VA
    LineBreakClass::ULB_AL, // 0x1269  # ETHIOPIC SYLLABLE VU
    LineBreakClass::ULB_AL, // 0x126A  # ETHIOPIC SYLLABLE VI
    LineBreakClass::ULB_AL, // 0x126B  # ETHIOPIC SYLLABLE VAA
    LineBreakClass::ULB_AL, // 0x126C  # ETHIOPIC SYLLABLE VEE
    LineBreakClass::ULB_AL, // 0x126D  # ETHIOPIC SYLLABLE VE
    LineBreakClass::ULB_AL, // 0x126E  # ETHIOPIC SYLLABLE VO
    LineBreakClass::ULB_AL, // 0x126F  # ETHIOPIC SYLLABLE VWA
    LineBreakClass::ULB_AL, // 0x1270  # ETHIOPIC SYLLABLE TA
    LineBreakClass::ULB_AL, // 0x1271  # ETHIOPIC SYLLABLE TU
    LineBreakClass::ULB_AL, // 0x1272  # ETHIOPIC SYLLABLE TI
    LineBreakClass::ULB_AL, // 0x1273  # ETHIOPIC SYLLABLE TAA
    LineBreakClass::ULB_AL, // 0x1274  # ETHIOPIC SYLLABLE TEE
    LineBreakClass::ULB_AL, // 0x1275  # ETHIOPIC SYLLABLE TE
    LineBreakClass::ULB_AL, // 0x1276  # ETHIOPIC SYLLABLE TO
    LineBreakClass::ULB_AL, // 0x1277  # ETHIOPIC SYLLABLE TWA
    LineBreakClass::ULB_AL, // 0x1278  # ETHIOPIC SYLLABLE CA
    LineBreakClass::ULB_AL, // 0x1279  # ETHIOPIC SYLLABLE CU
    LineBreakClass::ULB_AL, // 0x127A  # ETHIOPIC SYLLABLE CI
    LineBreakClass::ULB_AL, // 0x127B  # ETHIOPIC SYLLABLE CAA
    LineBreakClass::ULB_AL, // 0x127C  # ETHIOPIC SYLLABLE CEE
    LineBreakClass::ULB_AL, // 0x127D  # ETHIOPIC SYLLABLE CE
    LineBreakClass::ULB_AL, // 0x127E  # ETHIOPIC SYLLABLE CO
    LineBreakClass::ULB_AL, // 0x127F  # ETHIOPIC SYLLABLE CWA
    LineBreakClass::ULB_AL, // 0x1280  # ETHIOPIC SYLLABLE XA
    LineBreakClass::ULB_AL, // 0x1281  # ETHIOPIC SYLLABLE XU
    LineBreakClass::ULB_AL, // 0x1282  # ETHIOPIC SYLLABLE XI
    LineBreakClass::ULB_AL, // 0x1283  # ETHIOPIC SYLLABLE XAA
    LineBreakClass::ULB_AL, // 0x1284  # ETHIOPIC SYLLABLE XEE
    LineBreakClass::ULB_AL, // 0x1285  # ETHIOPIC SYLLABLE XE
    LineBreakClass::ULB_AL, // 0x1286  # ETHIOPIC SYLLABLE XO
    LineBreakClass::ULB_AL, // 0x1287  # ETHIOPIC SYLLABLE XOA
    LineBreakClass::ULB_AL, // 0x1288  # ETHIOPIC SYLLABLE XWA
    LineBreakClass::ULB_ID, // 0x1289 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x128A  # ETHIOPIC SYLLABLE XWI
    LineBreakClass::ULB_AL, // 0x128B  # ETHIOPIC SYLLABLE XWAA
    LineBreakClass::ULB_AL, // 0x128C  # ETHIOPIC SYLLABLE XWEE
    LineBreakClass::ULB_AL, // 0x128D  # ETHIOPIC SYLLABLE XWE
    LineBreakClass::ULB_ID, // 0x128E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x128F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1290  # ETHIOPIC SYLLABLE NA
    LineBreakClass::ULB_AL, // 0x1291  # ETHIOPIC SYLLABLE NU
    LineBreakClass::ULB_AL, // 0x1292  # ETHIOPIC SYLLABLE NI
    LineBreakClass::ULB_AL, // 0x1293  # ETHIOPIC SYLLABLE NAA
    LineBreakClass::ULB_AL, // 0x1294  # ETHIOPIC SYLLABLE NEE
    LineBreakClass::ULB_AL, // 0x1295  # ETHIOPIC SYLLABLE NE
    LineBreakClass::ULB_AL, // 0x1296  # ETHIOPIC SYLLABLE NO
    LineBreakClass::ULB_AL, // 0x1297  # ETHIOPIC SYLLABLE NWA
    LineBreakClass::ULB_AL, // 0x1298  # ETHIOPIC SYLLABLE NYA
    LineBreakClass::ULB_AL, // 0x1299  # ETHIOPIC SYLLABLE NYU
    LineBreakClass::ULB_AL, // 0x129A  # ETHIOPIC SYLLABLE NYI
    LineBreakClass::ULB_AL, // 0x129B  # ETHIOPIC SYLLABLE NYAA
    LineBreakClass::ULB_AL, // 0x129C  # ETHIOPIC SYLLABLE NYEE
    LineBreakClass::ULB_AL, // 0x129D  # ETHIOPIC SYLLABLE NYE
    LineBreakClass::ULB_AL, // 0x129E  # ETHIOPIC SYLLABLE NYO
    LineBreakClass::ULB_AL, // 0x129F  # ETHIOPIC SYLLABLE NYWA
    LineBreakClass::ULB_AL, // 0x12A0  # ETHIOPIC SYLLABLE GLOTTAL A
    LineBreakClass::ULB_AL, // 0x12A1  # ETHIOPIC SYLLABLE GLOTTAL U
    LineBreakClass::ULB_AL, // 0x12A2  # ETHIOPIC SYLLABLE GLOTTAL I
    LineBreakClass::ULB_AL, // 0x12A3  # ETHIOPIC SYLLABLE GLOTTAL AA
    LineBreakClass::ULB_AL, // 0x12A4  # ETHIOPIC SYLLABLE GLOTTAL EE
    LineBreakClass::ULB_AL, // 0x12A5  # ETHIOPIC SYLLABLE GLOTTAL E
    LineBreakClass::ULB_AL, // 0x12A6  # ETHIOPIC SYLLABLE GLOTTAL O
    LineBreakClass::ULB_AL, // 0x12A7  # ETHIOPIC SYLLABLE GLOTTAL WA
    LineBreakClass::ULB_AL, // 0x12A8  # ETHIOPIC SYLLABLE KA
    LineBreakClass::ULB_AL, // 0x12A9  # ETHIOPIC SYLLABLE KU
    LineBreakClass::ULB_AL, // 0x12AA  # ETHIOPIC SYLLABLE KI
    LineBreakClass::ULB_AL, // 0x12AB  # ETHIOPIC SYLLABLE KAA
    LineBreakClass::ULB_AL, // 0x12AC  # ETHIOPIC SYLLABLE KEE
    LineBreakClass::ULB_AL, // 0x12AD  # ETHIOPIC SYLLABLE KE
    LineBreakClass::ULB_AL, // 0x12AE  # ETHIOPIC SYLLABLE KO
    LineBreakClass::ULB_AL, // 0x12AF  # ETHIOPIC SYLLABLE KOA
    LineBreakClass::ULB_AL, // 0x12B0  # ETHIOPIC SYLLABLE KWA
    LineBreakClass::ULB_ID, // 0x12B1 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x12B2  # ETHIOPIC SYLLABLE KWI
    LineBreakClass::ULB_AL, // 0x12B3  # ETHIOPIC SYLLABLE KWAA
    LineBreakClass::ULB_AL, // 0x12B4  # ETHIOPIC SYLLABLE KWEE
    LineBreakClass::ULB_AL, // 0x12B5  # ETHIOPIC SYLLABLE KWE
    LineBreakClass::ULB_ID, // 0x12B6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x12B7 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x12B8  # ETHIOPIC SYLLABLE KXA
    LineBreakClass::ULB_AL, // 0x12B9  # ETHIOPIC SYLLABLE KXU
    LineBreakClass::ULB_AL, // 0x12BA  # ETHIOPIC SYLLABLE KXI
    LineBreakClass::ULB_AL, // 0x12BB  # ETHIOPIC SYLLABLE KXAA
    LineBreakClass::ULB_AL, // 0x12BC  # ETHIOPIC SYLLABLE KXEE
    LineBreakClass::ULB_AL, // 0x12BD  # ETHIOPIC SYLLABLE KXE
    LineBreakClass::ULB_AL, // 0x12BE  # ETHIOPIC SYLLABLE KXO
    LineBreakClass::ULB_ID, // 0x12BF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x12C0  # ETHIOPIC SYLLABLE KXWA
    LineBreakClass::ULB_ID, // 0x12C1 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x12C2  # ETHIOPIC SYLLABLE KXWI
    LineBreakClass::ULB_AL, // 0x12C3  # ETHIOPIC SYLLABLE KXWAA
    LineBreakClass::ULB_AL, // 0x12C4  # ETHIOPIC SYLLABLE KXWEE
    LineBreakClass::ULB_AL, // 0x12C5  # ETHIOPIC SYLLABLE KXWE
    LineBreakClass::ULB_ID, // 0x12C6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x12C7 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x12C8  # ETHIOPIC SYLLABLE WA
    LineBreakClass::ULB_AL, // 0x12C9  # ETHIOPIC SYLLABLE WU
    LineBreakClass::ULB_AL, // 0x12CA  # ETHIOPIC SYLLABLE WI
    LineBreakClass::ULB_AL, // 0x12CB  # ETHIOPIC SYLLABLE WAA
    LineBreakClass::ULB_AL, // 0x12CC  # ETHIOPIC SYLLABLE WEE
    LineBreakClass::ULB_AL, // 0x12CD  # ETHIOPIC SYLLABLE WE
    LineBreakClass::ULB_AL, // 0x12CE  # ETHIOPIC SYLLABLE WO
    LineBreakClass::ULB_AL, // 0x12CF  # ETHIOPIC SYLLABLE WOA
    LineBreakClass::ULB_AL, // 0x12D0  # ETHIOPIC SYLLABLE PHARYNGEAL A
    LineBreakClass::ULB_AL, // 0x12D1  # ETHIOPIC SYLLABLE PHARYNGEAL U
    LineBreakClass::ULB_AL, // 0x12D2  # ETHIOPIC SYLLABLE PHARYNGEAL I
    LineBreakClass::ULB_AL, // 0x12D3  # ETHIOPIC SYLLABLE PHARYNGEAL AA
    LineBreakClass::ULB_AL, // 0x12D4  # ETHIOPIC SYLLABLE PHARYNGEAL EE
    LineBreakClass::ULB_AL, // 0x12D5  # ETHIOPIC SYLLABLE PHARYNGEAL E
    LineBreakClass::ULB_AL, // 0x12D6  # ETHIOPIC SYLLABLE PHARYNGEAL O
    LineBreakClass::ULB_ID, // 0x12D7 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x12D8  # ETHIOPIC SYLLABLE ZA
    LineBreakClass::ULB_AL, // 0x12D9  # ETHIOPIC SYLLABLE ZU
    LineBreakClass::ULB_AL, // 0x12DA  # ETHIOPIC SYLLABLE ZI
    LineBreakClass::ULB_AL, // 0x12DB  # ETHIOPIC SYLLABLE ZAA
    LineBreakClass::ULB_AL, // 0x12DC  # ETHIOPIC SYLLABLE ZEE
    LineBreakClass::ULB_AL, // 0x12DD  # ETHIOPIC SYLLABLE ZE
    LineBreakClass::ULB_AL, // 0x12DE  # ETHIOPIC SYLLABLE ZO
    LineBreakClass::ULB_AL, // 0x12DF  # ETHIOPIC SYLLABLE ZWA
    LineBreakClass::ULB_AL, // 0x12E0  # ETHIOPIC SYLLABLE ZHA
    LineBreakClass::ULB_AL, // 0x12E1  # ETHIOPIC SYLLABLE ZHU
    LineBreakClass::ULB_AL, // 0x12E2  # ETHIOPIC SYLLABLE ZHI
    LineBreakClass::ULB_AL, // 0x12E3  # ETHIOPIC SYLLABLE ZHAA
    LineBreakClass::ULB_AL, // 0x12E4  # ETHIOPIC SYLLABLE ZHEE
    LineBreakClass::ULB_AL, // 0x12E5  # ETHIOPIC SYLLABLE ZHE
    LineBreakClass::ULB_AL, // 0x12E6  # ETHIOPIC SYLLABLE ZHO
    LineBreakClass::ULB_AL, // 0x12E7  # ETHIOPIC SYLLABLE ZHWA
    LineBreakClass::ULB_AL, // 0x12E8  # ETHIOPIC SYLLABLE YA
    LineBreakClass::ULB_AL, // 0x12E9  # ETHIOPIC SYLLABLE YU
    LineBreakClass::ULB_AL, // 0x12EA  # ETHIOPIC SYLLABLE YI
    LineBreakClass::ULB_AL, // 0x12EB  # ETHIOPIC SYLLABLE YAA
    LineBreakClass::ULB_AL, // 0x12EC  # ETHIOPIC SYLLABLE YEE
    LineBreakClass::ULB_AL, // 0x12ED  # ETHIOPIC SYLLABLE YE
    LineBreakClass::ULB_AL, // 0x12EE  # ETHIOPIC SYLLABLE YO
    LineBreakClass::ULB_AL, // 0x12EF  # ETHIOPIC SYLLABLE YOA
    LineBreakClass::ULB_AL, // 0x12F0  # ETHIOPIC SYLLABLE DA
    LineBreakClass::ULB_AL, // 0x12F1  # ETHIOPIC SYLLABLE DU
    LineBreakClass::ULB_AL, // 0x12F2  # ETHIOPIC SYLLABLE DI
    LineBreakClass::ULB_AL, // 0x12F3  # ETHIOPIC SYLLABLE DAA
    LineBreakClass::ULB_AL, // 0x12F4  # ETHIOPIC SYLLABLE DEE
    LineBreakClass::ULB_AL, // 0x12F5  # ETHIOPIC SYLLABLE DE
    LineBreakClass::ULB_AL, // 0x12F6  # ETHIOPIC SYLLABLE DO
    LineBreakClass::ULB_AL, // 0x12F7  # ETHIOPIC SYLLABLE DWA
    LineBreakClass::ULB_AL, // 0x12F8  # ETHIOPIC SYLLABLE DDA
    LineBreakClass::ULB_AL, // 0x12F9  # ETHIOPIC SYLLABLE DDU
    LineBreakClass::ULB_AL, // 0x12FA  # ETHIOPIC SYLLABLE DDI
    LineBreakClass::ULB_AL, // 0x12FB  # ETHIOPIC SYLLABLE DDAA
    LineBreakClass::ULB_AL, // 0x12FC  # ETHIOPIC SYLLABLE DDEE
    LineBreakClass::ULB_AL, // 0x12FD  # ETHIOPIC SYLLABLE DDE
    LineBreakClass::ULB_AL, // 0x12FE  # ETHIOPIC SYLLABLE DDO
    LineBreakClass::ULB_AL, // 0x12FF  # ETHIOPIC SYLLABLE DDWA
    LineBreakClass::ULB_AL, // 0x1300  # ETHIOPIC SYLLABLE JA
    LineBreakClass::ULB_AL, // 0x1301  # ETHIOPIC SYLLABLE JU
    LineBreakClass::ULB_AL, // 0x1302  # ETHIOPIC SYLLABLE JI
    LineBreakClass::ULB_AL, // 0x1303  # ETHIOPIC SYLLABLE JAA
    LineBreakClass::ULB_AL, // 0x1304  # ETHIOPIC SYLLABLE JEE
    LineBreakClass::ULB_AL, // 0x1305  # ETHIOPIC SYLLABLE JE
    LineBreakClass::ULB_AL, // 0x1306  # ETHIOPIC SYLLABLE JO
    LineBreakClass::ULB_AL, // 0x1307  # ETHIOPIC SYLLABLE JWA
    LineBreakClass::ULB_AL, // 0x1308  # ETHIOPIC SYLLABLE GA
    LineBreakClass::ULB_AL, // 0x1309  # ETHIOPIC SYLLABLE GU
    LineBreakClass::ULB_AL, // 0x130A  # ETHIOPIC SYLLABLE GI
    LineBreakClass::ULB_AL, // 0x130B  # ETHIOPIC SYLLABLE GAA
    LineBreakClass::ULB_AL, // 0x130C  # ETHIOPIC SYLLABLE GEE
    LineBreakClass::ULB_AL, // 0x130D  # ETHIOPIC SYLLABLE GE
    LineBreakClass::ULB_AL, // 0x130E  # ETHIOPIC SYLLABLE GO
    LineBreakClass::ULB_AL, // 0x130F  # ETHIOPIC SYLLABLE GOA
    LineBreakClass::ULB_AL, // 0x1310  # ETHIOPIC SYLLABLE GWA
    LineBreakClass::ULB_ID, // 0x1311 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1312  # ETHIOPIC SYLLABLE GWI
    LineBreakClass::ULB_AL, // 0x1313  # ETHIOPIC SYLLABLE GWAA
    LineBreakClass::ULB_AL, // 0x1314  # ETHIOPIC SYLLABLE GWEE
    LineBreakClass::ULB_AL, // 0x1315  # ETHIOPIC SYLLABLE GWE
    LineBreakClass::ULB_ID, // 0x1316 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1317 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1318  # ETHIOPIC SYLLABLE GGA
    LineBreakClass::ULB_AL, // 0x1319  # ETHIOPIC SYLLABLE GGU
    LineBreakClass::ULB_AL, // 0x131A  # ETHIOPIC SYLLABLE GGI
    LineBreakClass::ULB_AL, // 0x131B  # ETHIOPIC SYLLABLE GGAA
    LineBreakClass::ULB_AL, // 0x131C  # ETHIOPIC SYLLABLE GGEE
    LineBreakClass::ULB_AL, // 0x131D  # ETHIOPIC SYLLABLE GGE
    LineBreakClass::ULB_AL, // 0x131E  # ETHIOPIC SYLLABLE GGO
    LineBreakClass::ULB_AL, // 0x131F  # ETHIOPIC SYLLABLE GGWAA
    LineBreakClass::ULB_AL, // 0x1320  # ETHIOPIC SYLLABLE THA
    LineBreakClass::ULB_AL, // 0x1321  # ETHIOPIC SYLLABLE THU
    LineBreakClass::ULB_AL, // 0x1322  # ETHIOPIC SYLLABLE THI
    LineBreakClass::ULB_AL, // 0x1323  # ETHIOPIC SYLLABLE THAA
    LineBreakClass::ULB_AL, // 0x1324  # ETHIOPIC SYLLABLE THEE
    LineBreakClass::ULB_AL, // 0x1325  # ETHIOPIC SYLLABLE THE
    LineBreakClass::ULB_AL, // 0x1326  # ETHIOPIC SYLLABLE THO
    LineBreakClass::ULB_AL, // 0x1327  # ETHIOPIC SYLLABLE THWA
    LineBreakClass::ULB_AL, // 0x1328  # ETHIOPIC SYLLABLE CHA
    LineBreakClass::ULB_AL, // 0x1329  # ETHIOPIC SYLLABLE CHU
    LineBreakClass::ULB_AL, // 0x132A  # ETHIOPIC SYLLABLE CHI
    LineBreakClass::ULB_AL, // 0x132B  # ETHIOPIC SYLLABLE CHAA
    LineBreakClass::ULB_AL, // 0x132C  # ETHIOPIC SYLLABLE CHEE
    LineBreakClass::ULB_AL, // 0x132D  # ETHIOPIC SYLLABLE CHE
    LineBreakClass::ULB_AL, // 0x132E  # ETHIOPIC SYLLABLE CHO
    LineBreakClass::ULB_AL, // 0x132F  # ETHIOPIC SYLLABLE CHWA
    LineBreakClass::ULB_AL, // 0x1330  # ETHIOPIC SYLLABLE PHA
    LineBreakClass::ULB_AL, // 0x1331  # ETHIOPIC SYLLABLE PHU
    LineBreakClass::ULB_AL, // 0x1332  # ETHIOPIC SYLLABLE PHI
    LineBreakClass::ULB_AL, // 0x1333  # ETHIOPIC SYLLABLE PHAA
    LineBreakClass::ULB_AL, // 0x1334  # ETHIOPIC SYLLABLE PHEE
    LineBreakClass::ULB_AL, // 0x1335  # ETHIOPIC SYLLABLE PHE
    LineBreakClass::ULB_AL, // 0x1336  # ETHIOPIC SYLLABLE PHO
    LineBreakClass::ULB_AL, // 0x1337  # ETHIOPIC SYLLABLE PHWA
    LineBreakClass::ULB_AL, // 0x1338  # ETHIOPIC SYLLABLE TSA
    LineBreakClass::ULB_AL, // 0x1339  # ETHIOPIC SYLLABLE TSU
    LineBreakClass::ULB_AL, // 0x133A  # ETHIOPIC SYLLABLE TSI
    LineBreakClass::ULB_AL, // 0x133B  # ETHIOPIC SYLLABLE TSAA
    LineBreakClass::ULB_AL, // 0x133C  # ETHIOPIC SYLLABLE TSEE
    LineBreakClass::ULB_AL, // 0x133D  # ETHIOPIC SYLLABLE TSE
    LineBreakClass::ULB_AL, // 0x133E  # ETHIOPIC SYLLABLE TSO
    LineBreakClass::ULB_AL, // 0x133F  # ETHIOPIC SYLLABLE TSWA
    LineBreakClass::ULB_AL, // 0x1340  # ETHIOPIC SYLLABLE TZA
    LineBreakClass::ULB_AL, // 0x1341  # ETHIOPIC SYLLABLE TZU
    LineBreakClass::ULB_AL, // 0x1342  # ETHIOPIC SYLLABLE TZI
    LineBreakClass::ULB_AL, // 0x1343  # ETHIOPIC SYLLABLE TZAA
    LineBreakClass::ULB_AL, // 0x1344  # ETHIOPIC SYLLABLE TZEE
    LineBreakClass::ULB_AL, // 0x1345  # ETHIOPIC SYLLABLE TZE
    LineBreakClass::ULB_AL, // 0x1346  # ETHIOPIC SYLLABLE TZO
    LineBreakClass::ULB_AL, // 0x1347  # ETHIOPIC SYLLABLE TZOA
    LineBreakClass::ULB_AL, // 0x1348  # ETHIOPIC SYLLABLE FA
    LineBreakClass::ULB_AL, // 0x1349  # ETHIOPIC SYLLABLE FU
    LineBreakClass::ULB_AL, // 0x134A  # ETHIOPIC SYLLABLE FI
    LineBreakClass::ULB_AL, // 0x134B  # ETHIOPIC SYLLABLE FAA
    LineBreakClass::ULB_AL, // 0x134C  # ETHIOPIC SYLLABLE FEE
    LineBreakClass::ULB_AL, // 0x134D  # ETHIOPIC SYLLABLE FE
    LineBreakClass::ULB_AL, // 0x134E  # ETHIOPIC SYLLABLE FO
    LineBreakClass::ULB_AL, // 0x134F  # ETHIOPIC SYLLABLE FWA
    LineBreakClass::ULB_AL, // 0x1350  # ETHIOPIC SYLLABLE PA
    LineBreakClass::ULB_AL, // 0x1351  # ETHIOPIC SYLLABLE PU
    LineBreakClass::ULB_AL, // 0x1352  # ETHIOPIC SYLLABLE PI
    LineBreakClass::ULB_AL, // 0x1353  # ETHIOPIC SYLLABLE PAA
    LineBreakClass::ULB_AL, // 0x1354  # ETHIOPIC SYLLABLE PEE
    LineBreakClass::ULB_AL, // 0x1355  # ETHIOPIC SYLLABLE PE
    LineBreakClass::ULB_AL, // 0x1356  # ETHIOPIC SYLLABLE PO
    LineBreakClass::ULB_AL, // 0x1357  # ETHIOPIC SYLLABLE PWA
    LineBreakClass::ULB_AL, // 0x1358  # ETHIOPIC SYLLABLE RYA
    LineBreakClass::ULB_AL, // 0x1359  # ETHIOPIC SYLLABLE MYA
    LineBreakClass::ULB_AL, // 0x135A  # ETHIOPIC SYLLABLE FYA
    LineBreakClass::ULB_ID, // 0x135B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x135C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x135D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x135E # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x135F  # ETHIOPIC COMBINING GEMINATION MARK
    LineBreakClass::ULB_AL, // 0x1360  # ETHIOPIC SECTION MARK
    LineBreakClass::ULB_BA, // 0x1361  # ETHIOPIC WORDSPACE
    LineBreakClass::ULB_AL, // 0x1362  # ETHIOPIC FULL STOP
    LineBreakClass::ULB_AL, // 0x1363  # ETHIOPIC COMMA
    LineBreakClass::ULB_AL, // 0x1364  # ETHIOPIC SEMICOLON
    LineBreakClass::ULB_AL, // 0x1365  # ETHIOPIC COLON
    LineBreakClass::ULB_AL, // 0x1366  # ETHIOPIC PREFACE COLON
    LineBreakClass::ULB_AL, // 0x1367  # ETHIOPIC QUESTION MARK
    LineBreakClass::ULB_AL, // 0x1368  # ETHIOPIC PARAGRAPH SEPARATOR
    LineBreakClass::ULB_AL, // 0x1369  # ETHIOPIC DIGIT ONE
    LineBreakClass::ULB_AL, // 0x136A  # ETHIOPIC DIGIT TWO
    LineBreakClass::ULB_AL, // 0x136B  # ETHIOPIC DIGIT THREE
    LineBreakClass::ULB_AL, // 0x136C  # ETHIOPIC DIGIT FOUR
    LineBreakClass::ULB_AL, // 0x136D  # ETHIOPIC DIGIT FIVE
    LineBreakClass::ULB_AL, // 0x136E  # ETHIOPIC DIGIT SIX
    LineBreakClass::ULB_AL, // 0x136F  # ETHIOPIC DIGIT SEVEN
    LineBreakClass::ULB_AL, // 0x1370  # ETHIOPIC DIGIT EIGHT
    LineBreakClass::ULB_AL, // 0x1371  # ETHIOPIC DIGIT NINE
    LineBreakClass::ULB_AL, // 0x1372  # ETHIOPIC NUMBER TEN
    LineBreakClass::ULB_AL, // 0x1373  # ETHIOPIC NUMBER TWENTY
    LineBreakClass::ULB_AL, // 0x1374  # ETHIOPIC NUMBER THIRTY
    LineBreakClass::ULB_AL, // 0x1375  # ETHIOPIC NUMBER FORTY
    LineBreakClass::ULB_AL, // 0x1376  # ETHIOPIC NUMBER FIFTY
    LineBreakClass::ULB_AL, // 0x1377  # ETHIOPIC NUMBER SIXTY
    LineBreakClass::ULB_AL, // 0x1378  # ETHIOPIC NUMBER SEVENTY
    LineBreakClass::ULB_AL, // 0x1379  # ETHIOPIC NUMBER EIGHTY
    LineBreakClass::ULB_AL, // 0x137A  # ETHIOPIC NUMBER NINETY
    LineBreakClass::ULB_AL, // 0x137B  # ETHIOPIC NUMBER HUNDRED
    LineBreakClass::ULB_AL, // 0x137C  # ETHIOPIC NUMBER TEN THOUSAND
    LineBreakClass::ULB_ID, // 0x137D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x137E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x137F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1380  # ETHIOPIC SYLLABLE SEBATBEIT MWA
    LineBreakClass::ULB_AL, // 0x1381  # ETHIOPIC SYLLABLE MWI
    LineBreakClass::ULB_AL, // 0x1382  # ETHIOPIC SYLLABLE MWEE
    LineBreakClass::ULB_AL, // 0x1383  # ETHIOPIC SYLLABLE MWE
    LineBreakClass::ULB_AL, // 0x1384  # ETHIOPIC SYLLABLE SEBATBEIT BWA
    LineBreakClass::ULB_AL, // 0x1385  # ETHIOPIC SYLLABLE BWI
    LineBreakClass::ULB_AL, // 0x1386  # ETHIOPIC SYLLABLE BWEE
    LineBreakClass::ULB_AL, // 0x1387  # ETHIOPIC SYLLABLE BWE
    LineBreakClass::ULB_AL, // 0x1388  # ETHIOPIC SYLLABLE SEBATBEIT FWA
    LineBreakClass::ULB_AL, // 0x1389  # ETHIOPIC SYLLABLE FWI
    LineBreakClass::ULB_AL, // 0x138A  # ETHIOPIC SYLLABLE FWEE
    LineBreakClass::ULB_AL, // 0x138B  # ETHIOPIC SYLLABLE FWE
    LineBreakClass::ULB_AL, // 0x138C  # ETHIOPIC SYLLABLE SEBATBEIT PWA
    LineBreakClass::ULB_AL, // 0x138D  # ETHIOPIC SYLLABLE PWI
    LineBreakClass::ULB_AL, // 0x138E  # ETHIOPIC SYLLABLE PWEE
    LineBreakClass::ULB_AL, // 0x138F  # ETHIOPIC SYLLABLE PWE
    LineBreakClass::ULB_AL, // 0x1390  # ETHIOPIC TONAL MARK YIZET
    LineBreakClass::ULB_AL, // 0x1391  # ETHIOPIC TONAL MARK DERET
    LineBreakClass::ULB_AL, // 0x1392  # ETHIOPIC TONAL MARK RIKRIK
    LineBreakClass::ULB_AL, // 0x1393  # ETHIOPIC TONAL MARK SHORT RIKRIK
    LineBreakClass::ULB_AL, // 0x1394  # ETHIOPIC TONAL MARK DIFAT
    LineBreakClass::ULB_AL, // 0x1395  # ETHIOPIC TONAL MARK KENAT
    LineBreakClass::ULB_AL, // 0x1396  # ETHIOPIC TONAL MARK CHIRET
    LineBreakClass::ULB_AL, // 0x1397  # ETHIOPIC TONAL MARK HIDET
    LineBreakClass::ULB_AL, // 0x1398  # ETHIOPIC TONAL MARK DERET-HIDET
    LineBreakClass::ULB_AL, // 0x1399  # ETHIOPIC TONAL MARK KURT
    LineBreakClass::ULB_ID, // 0x139A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x139B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x139C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x139D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x139E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x139F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x13A0  # CHEROKEE LETTER A
    LineBreakClass::ULB_AL, // 0x13A1  # CHEROKEE LETTER E
    LineBreakClass::ULB_AL, // 0x13A2  # CHEROKEE LETTER I
    LineBreakClass::ULB_AL, // 0x13A3  # CHEROKEE LETTER O
    LineBreakClass::ULB_AL, // 0x13A4  # CHEROKEE LETTER U
    LineBreakClass::ULB_AL, // 0x13A5  # CHEROKEE LETTER V
    LineBreakClass::ULB_AL, // 0x13A6  # CHEROKEE LETTER GA
    LineBreakClass::ULB_AL, // 0x13A7  # CHEROKEE LETTER KA
    LineBreakClass::ULB_AL, // 0x13A8  # CHEROKEE LETTER GE
    LineBreakClass::ULB_AL, // 0x13A9  # CHEROKEE LETTER GI
    LineBreakClass::ULB_AL, // 0x13AA  # CHEROKEE LETTER GO
    LineBreakClass::ULB_AL, // 0x13AB  # CHEROKEE LETTER GU
    LineBreakClass::ULB_AL, // 0x13AC  # CHEROKEE LETTER GV
    LineBreakClass::ULB_AL, // 0x13AD  # CHEROKEE LETTER HA
    LineBreakClass::ULB_AL, // 0x13AE  # CHEROKEE LETTER HE
    LineBreakClass::ULB_AL, // 0x13AF  # CHEROKEE LETTER HI
    LineBreakClass::ULB_AL, // 0x13B0  # CHEROKEE LETTER HO
    LineBreakClass::ULB_AL, // 0x13B1  # CHEROKEE LETTER HU
    LineBreakClass::ULB_AL, // 0x13B2  # CHEROKEE LETTER HV
    LineBreakClass::ULB_AL, // 0x13B3  # CHEROKEE LETTER LA
    LineBreakClass::ULB_AL, // 0x13B4  # CHEROKEE LETTER LE
    LineBreakClass::ULB_AL, // 0x13B5  # CHEROKEE LETTER LI
    LineBreakClass::ULB_AL, // 0x13B6  # CHEROKEE LETTER LO
    LineBreakClass::ULB_AL, // 0x13B7  # CHEROKEE LETTER LU
    LineBreakClass::ULB_AL, // 0x13B8  # CHEROKEE LETTER LV
    LineBreakClass::ULB_AL, // 0x13B9  # CHEROKEE LETTER MA
    LineBreakClass::ULB_AL, // 0x13BA  # CHEROKEE LETTER ME
    LineBreakClass::ULB_AL, // 0x13BB  # CHEROKEE LETTER MI
    LineBreakClass::ULB_AL, // 0x13BC  # CHEROKEE LETTER MO
    LineBreakClass::ULB_AL, // 0x13BD  # CHEROKEE LETTER MU
    LineBreakClass::ULB_AL, // 0x13BE  # CHEROKEE LETTER NA
    LineBreakClass::ULB_AL, // 0x13BF  # CHEROKEE LETTER HNA
    LineBreakClass::ULB_AL, // 0x13C0  # CHEROKEE LETTER NAH
    LineBreakClass::ULB_AL, // 0x13C1  # CHEROKEE LETTER NE
    LineBreakClass::ULB_AL, // 0x13C2  # CHEROKEE LETTER NI
    LineBreakClass::ULB_AL, // 0x13C3  # CHEROKEE LETTER NO
    LineBreakClass::ULB_AL, // 0x13C4  # CHEROKEE LETTER NU
    LineBreakClass::ULB_AL, // 0x13C5  # CHEROKEE LETTER NV
    LineBreakClass::ULB_AL, // 0x13C6  # CHEROKEE LETTER QUA
    LineBreakClass::ULB_AL, // 0x13C7  # CHEROKEE LETTER QUE
    LineBreakClass::ULB_AL, // 0x13C8  # CHEROKEE LETTER QUI
    LineBreakClass::ULB_AL, // 0x13C9  # CHEROKEE LETTER QUO
    LineBreakClass::ULB_AL, // 0x13CA  # CHEROKEE LETTER QUU
    LineBreakClass::ULB_AL, // 0x13CB  # CHEROKEE LETTER QUV
    LineBreakClass::ULB_AL, // 0x13CC  # CHEROKEE LETTER SA
    LineBreakClass::ULB_AL, // 0x13CD  # CHEROKEE LETTER S
    LineBreakClass::ULB_AL, // 0x13CE  # CHEROKEE LETTER SE
    LineBreakClass::ULB_AL, // 0x13CF  # CHEROKEE LETTER SI
    LineBreakClass::ULB_AL, // 0x13D0  # CHEROKEE LETTER SO
    LineBreakClass::ULB_AL, // 0x13D1  # CHEROKEE LETTER SU
    LineBreakClass::ULB_AL, // 0x13D2  # CHEROKEE LETTER SV
    LineBreakClass::ULB_AL, // 0x13D3  # CHEROKEE LETTER DA
    LineBreakClass::ULB_AL, // 0x13D4  # CHEROKEE LETTER TA
    LineBreakClass::ULB_AL, // 0x13D5  # CHEROKEE LETTER DE
    LineBreakClass::ULB_AL, // 0x13D6  # CHEROKEE LETTER TE
    LineBreakClass::ULB_AL, // 0x13D7  # CHEROKEE LETTER DI
    LineBreakClass::ULB_AL, // 0x13D8  # CHEROKEE LETTER TI
    LineBreakClass::ULB_AL, // 0x13D9  # CHEROKEE LETTER DO
    LineBreakClass::ULB_AL, // 0x13DA  # CHEROKEE LETTER DU
    LineBreakClass::ULB_AL, // 0x13DB  # CHEROKEE LETTER DV
    LineBreakClass::ULB_AL, // 0x13DC  # CHEROKEE LETTER DLA
    LineBreakClass::ULB_AL, // 0x13DD  # CHEROKEE LETTER TLA
    LineBreakClass::ULB_AL, // 0x13DE  # CHEROKEE LETTER TLE
    LineBreakClass::ULB_AL, // 0x13DF  # CHEROKEE LETTER TLI
    LineBreakClass::ULB_AL, // 0x13E0  # CHEROKEE LETTER TLO
    LineBreakClass::ULB_AL, // 0x13E1  # CHEROKEE LETTER TLU
    LineBreakClass::ULB_AL, // 0x13E2  # CHEROKEE LETTER TLV
    LineBreakClass::ULB_AL, // 0x13E3  # CHEROKEE LETTER TSA
    LineBreakClass::ULB_AL, // 0x13E4  # CHEROKEE LETTER TSE
    LineBreakClass::ULB_AL, // 0x13E5  # CHEROKEE LETTER TSI
    LineBreakClass::ULB_AL, // 0x13E6  # CHEROKEE LETTER TSO
    LineBreakClass::ULB_AL, // 0x13E7  # CHEROKEE LETTER TSU
    LineBreakClass::ULB_AL, // 0x13E8  # CHEROKEE LETTER TSV
    LineBreakClass::ULB_AL, // 0x13E9  # CHEROKEE LETTER WA
    LineBreakClass::ULB_AL, // 0x13EA  # CHEROKEE LETTER WE
    LineBreakClass::ULB_AL, // 0x13EB  # CHEROKEE LETTER WI
    LineBreakClass::ULB_AL, // 0x13EC  # CHEROKEE LETTER WO
    LineBreakClass::ULB_AL, // 0x13ED  # CHEROKEE LETTER WU
    LineBreakClass::ULB_AL, // 0x13EE  # CHEROKEE LETTER WV
    LineBreakClass::ULB_AL, // 0x13EF  # CHEROKEE LETTER YA
    LineBreakClass::ULB_AL, // 0x13F0  # CHEROKEE LETTER YE
    LineBreakClass::ULB_AL, // 0x13F1  # CHEROKEE LETTER YI
    LineBreakClass::ULB_AL, // 0x13F2  # CHEROKEE LETTER YO
    LineBreakClass::ULB_AL, // 0x13F3  # CHEROKEE LETTER YU
    LineBreakClass::ULB_AL, // 0x13F4  # CHEROKEE LETTER YV
    LineBreakClass::ULB_ID, // 0x13F5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x13F6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x13F7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x13F8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x13F9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x13FA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x13FB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x13FC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x13FD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x13FE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x13FF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1400 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1401  # CANADIAN SYLLABICS E
    LineBreakClass::ULB_AL, // 0x1402  # CANADIAN SYLLABICS AAI
    LineBreakClass::ULB_AL, // 0x1403  # CANADIAN SYLLABICS I
    LineBreakClass::ULB_AL, // 0x1404  # CANADIAN SYLLABICS II
    LineBreakClass::ULB_AL, // 0x1405  # CANADIAN SYLLABICS O
    LineBreakClass::ULB_AL, // 0x1406  # CANADIAN SYLLABICS OO
    LineBreakClass::ULB_AL, // 0x1407  # CANADIAN SYLLABICS Y-CREE OO
    LineBreakClass::ULB_AL, // 0x1408  # CANADIAN SYLLABICS CARRIER EE
    LineBreakClass::ULB_AL, // 0x1409  # CANADIAN SYLLABICS CARRIER I
    LineBreakClass::ULB_AL, // 0x140A  # CANADIAN SYLLABICS A
    LineBreakClass::ULB_AL, // 0x140B  # CANADIAN SYLLABICS AA
    LineBreakClass::ULB_AL, // 0x140C  # CANADIAN SYLLABICS WE
    LineBreakClass::ULB_AL, // 0x140D  # CANADIAN SYLLABICS WEST-CREE WE
    LineBreakClass::ULB_AL, // 0x140E  # CANADIAN SYLLABICS WI
    LineBreakClass::ULB_AL, // 0x140F  # CANADIAN SYLLABICS WEST-CREE WI
    LineBreakClass::ULB_AL, // 0x1410  # CANADIAN SYLLABICS WII
    LineBreakClass::ULB_AL, // 0x1411  # CANADIAN SYLLABICS WEST-CREE WII
    LineBreakClass::ULB_AL, // 0x1412  # CANADIAN SYLLABICS WO
    LineBreakClass::ULB_AL, // 0x1413  # CANADIAN SYLLABICS WEST-CREE WO
    LineBreakClass::ULB_AL, // 0x1414  # CANADIAN SYLLABICS WOO
    LineBreakClass::ULB_AL, // 0x1415  # CANADIAN SYLLABICS WEST-CREE WOO
    LineBreakClass::ULB_AL, // 0x1416  # CANADIAN SYLLABICS NASKAPI WOO
    LineBreakClass::ULB_AL, // 0x1417  # CANADIAN SYLLABICS WA
    LineBreakClass::ULB_AL, // 0x1418  # CANADIAN SYLLABICS WEST-CREE WA
    LineBreakClass::ULB_AL, // 0x1419  # CANADIAN SYLLABICS WAA
    LineBreakClass::ULB_AL, // 0x141A  # CANADIAN SYLLABICS WEST-CREE WAA
    LineBreakClass::ULB_AL, // 0x141B  # CANADIAN SYLLABICS NASKAPI WAA
    LineBreakClass::ULB_AL, // 0x141C  # CANADIAN SYLLABICS AI
    LineBreakClass::ULB_AL, // 0x141D  # CANADIAN SYLLABICS Y-CREE W
    LineBreakClass::ULB_AL, // 0x141E  # CANADIAN SYLLABICS GLOTTAL STOP
    LineBreakClass::ULB_AL, // 0x141F  # CANADIAN SYLLABICS FINAL ACUTE
    LineBreakClass::ULB_AL, // 0x1420  # CANADIAN SYLLABICS FINAL GRAVE
    LineBreakClass::ULB_AL, // 0x1421  # CANADIAN SYLLABICS FINAL BOTTOM HALF RING
    LineBreakClass::ULB_AL, // 0x1422  # CANADIAN SYLLABICS FINAL TOP HALF RING
    LineBreakClass::ULB_AL, // 0x1423  # CANADIAN SYLLABICS FINAL RIGHT HALF RING
    LineBreakClass::ULB_AL, // 0x1424  # CANADIAN SYLLABICS FINAL RING
    LineBreakClass::ULB_AL, // 0x1425  # CANADIAN SYLLABICS FINAL DOUBLE ACUTE
    LineBreakClass::ULB_AL, // 0x1426  # CANADIAN SYLLABICS FINAL DOUBLE SHORT VERTICAL STROKES
    LineBreakClass::ULB_AL, // 0x1427  # CANADIAN SYLLABICS FINAL MIDDLE DOT
    LineBreakClass::ULB_AL, // 0x1428  # CANADIAN SYLLABICS FINAL SHORT HORIZONTAL STROKE
    LineBreakClass::ULB_AL, // 0x1429  # CANADIAN SYLLABICS FINAL PLUS
    LineBreakClass::ULB_AL, // 0x142A  # CANADIAN SYLLABICS FINAL DOWN TACK
    LineBreakClass::ULB_AL, // 0x142B  # CANADIAN SYLLABICS EN
    LineBreakClass::ULB_AL, // 0x142C  # CANADIAN SYLLABICS IN
    LineBreakClass::ULB_AL, // 0x142D  # CANADIAN SYLLABICS ON
    LineBreakClass::ULB_AL, // 0x142E  # CANADIAN SYLLABICS AN
    LineBreakClass::ULB_AL, // 0x142F  # CANADIAN SYLLABICS PE
    LineBreakClass::ULB_AL, // 0x1430  # CANADIAN SYLLABICS PAAI
    LineBreakClass::ULB_AL, // 0x1431  # CANADIAN SYLLABICS PI
    LineBreakClass::ULB_AL, // 0x1432  # CANADIAN SYLLABICS PII
    LineBreakClass::ULB_AL, // 0x1433  # CANADIAN SYLLABICS PO
    LineBreakClass::ULB_AL, // 0x1434  # CANADIAN SYLLABICS POO
    LineBreakClass::ULB_AL, // 0x1435  # CANADIAN SYLLABICS Y-CREE POO
    LineBreakClass::ULB_AL, // 0x1436  # CANADIAN SYLLABICS CARRIER HEE
    LineBreakClass::ULB_AL, // 0x1437  # CANADIAN SYLLABICS CARRIER HI
    LineBreakClass::ULB_AL, // 0x1438  # CANADIAN SYLLABICS PA
    LineBreakClass::ULB_AL, // 0x1439  # CANADIAN SYLLABICS PAA
    LineBreakClass::ULB_AL, // 0x143A  # CANADIAN SYLLABICS PWE
    LineBreakClass::ULB_AL, // 0x143B  # CANADIAN SYLLABICS WEST-CREE PWE
    LineBreakClass::ULB_AL, // 0x143C  # CANADIAN SYLLABICS PWI
    LineBreakClass::ULB_AL, // 0x143D  # CANADIAN SYLLABICS WEST-CREE PWI
    LineBreakClass::ULB_AL, // 0x143E  # CANADIAN SYLLABICS PWII
    LineBreakClass::ULB_AL, // 0x143F  # CANADIAN SYLLABICS WEST-CREE PWII
    LineBreakClass::ULB_AL, // 0x1440  # CANADIAN SYLLABICS PWO
    LineBreakClass::ULB_AL, // 0x1441  # CANADIAN SYLLABICS WEST-CREE PWO
    LineBreakClass::ULB_AL, // 0x1442  # CANADIAN SYLLABICS PWOO
    LineBreakClass::ULB_AL, // 0x1443  # CANADIAN SYLLABICS WEST-CREE PWOO
    LineBreakClass::ULB_AL, // 0x1444  # CANADIAN SYLLABICS PWA
    LineBreakClass::ULB_AL, // 0x1445  # CANADIAN SYLLABICS WEST-CREE PWA
    LineBreakClass::ULB_AL, // 0x1446  # CANADIAN SYLLABICS PWAA
    LineBreakClass::ULB_AL, // 0x1447  # CANADIAN SYLLABICS WEST-CREE PWAA
    LineBreakClass::ULB_AL, // 0x1448  # CANADIAN SYLLABICS Y-CREE PWAA
    LineBreakClass::ULB_AL, // 0x1449  # CANADIAN SYLLABICS P
    LineBreakClass::ULB_AL, // 0x144A  # CANADIAN SYLLABICS WEST-CREE P
    LineBreakClass::ULB_AL, // 0x144B  # CANADIAN SYLLABICS CARRIER H
    LineBreakClass::ULB_AL, // 0x144C  # CANADIAN SYLLABICS TE
    LineBreakClass::ULB_AL, // 0x144D  # CANADIAN SYLLABICS TAAI
    LineBreakClass::ULB_AL, // 0x144E  # CANADIAN SYLLABICS TI
    LineBreakClass::ULB_AL, // 0x144F  # CANADIAN SYLLABICS TII
    LineBreakClass::ULB_AL, // 0x1450  # CANADIAN SYLLABICS TO
    LineBreakClass::ULB_AL, // 0x1451  # CANADIAN SYLLABICS TOO
    LineBreakClass::ULB_AL, // 0x1452  # CANADIAN SYLLABICS Y-CREE TOO
    LineBreakClass::ULB_AL, // 0x1453  # CANADIAN SYLLABICS CARRIER DEE
    LineBreakClass::ULB_AL, // 0x1454  # CANADIAN SYLLABICS CARRIER DI
    LineBreakClass::ULB_AL, // 0x1455  # CANADIAN SYLLABICS TA
    LineBreakClass::ULB_AL, // 0x1456  # CANADIAN SYLLABICS TAA
    LineBreakClass::ULB_AL, // 0x1457  # CANADIAN SYLLABICS TWE
    LineBreakClass::ULB_AL, // 0x1458  # CANADIAN SYLLABICS WEST-CREE TWE
    LineBreakClass::ULB_AL, // 0x1459  # CANADIAN SYLLABICS TWI
    LineBreakClass::ULB_AL, // 0x145A  # CANADIAN SYLLABICS WEST-CREE TWI
    LineBreakClass::ULB_AL, // 0x145B  # CANADIAN SYLLABICS TWII
    LineBreakClass::ULB_AL, // 0x145C  # CANADIAN SYLLABICS WEST-CREE TWII
    LineBreakClass::ULB_AL, // 0x145D  # CANADIAN SYLLABICS TWO
    LineBreakClass::ULB_AL, // 0x145E  # CANADIAN SYLLABICS WEST-CREE TWO
    LineBreakClass::ULB_AL, // 0x145F  # CANADIAN SYLLABICS TWOO
    LineBreakClass::ULB_AL, // 0x1460  # CANADIAN SYLLABICS WEST-CREE TWOO
    LineBreakClass::ULB_AL, // 0x1461  # CANADIAN SYLLABICS TWA
    LineBreakClass::ULB_AL, // 0x1462  # CANADIAN SYLLABICS WEST-CREE TWA
    LineBreakClass::ULB_AL, // 0x1463  # CANADIAN SYLLABICS TWAA
    LineBreakClass::ULB_AL, // 0x1464  # CANADIAN SYLLABICS WEST-CREE TWAA
    LineBreakClass::ULB_AL, // 0x1465  # CANADIAN SYLLABICS NASKAPI TWAA
    LineBreakClass::ULB_AL, // 0x1466  # CANADIAN SYLLABICS T
    LineBreakClass::ULB_AL, // 0x1467  # CANADIAN SYLLABICS TTE
    LineBreakClass::ULB_AL, // 0x1468  # CANADIAN SYLLABICS TTI
    LineBreakClass::ULB_AL, // 0x1469  # CANADIAN SYLLABICS TTO
    LineBreakClass::ULB_AL, // 0x146A  # CANADIAN SYLLABICS TTA
    LineBreakClass::ULB_AL, // 0x146B  # CANADIAN SYLLABICS KE
    LineBreakClass::ULB_AL, // 0x146C  # CANADIAN SYLLABICS KAAI
    LineBreakClass::ULB_AL, // 0x146D  # CANADIAN SYLLABICS KI
    LineBreakClass::ULB_AL, // 0x146E  # CANADIAN SYLLABICS KII
    LineBreakClass::ULB_AL, // 0x146F  # CANADIAN SYLLABICS KO
    LineBreakClass::ULB_AL, // 0x1470  # CANADIAN SYLLABICS KOO
    LineBreakClass::ULB_AL, // 0x1471  # CANADIAN SYLLABICS Y-CREE KOO
    LineBreakClass::ULB_AL, // 0x1472  # CANADIAN SYLLABICS KA
    LineBreakClass::ULB_AL, // 0x1473  # CANADIAN SYLLABICS KAA
    LineBreakClass::ULB_AL, // 0x1474  # CANADIAN SYLLABICS KWE
    LineBreakClass::ULB_AL, // 0x1475  # CANADIAN SYLLABICS WEST-CREE KWE
    LineBreakClass::ULB_AL, // 0x1476  # CANADIAN SYLLABICS KWI
    LineBreakClass::ULB_AL, // 0x1477  # CANADIAN SYLLABICS WEST-CREE KWI
    LineBreakClass::ULB_AL, // 0x1478  # CANADIAN SYLLABICS KWII
    LineBreakClass::ULB_AL, // 0x1479  # CANADIAN SYLLABICS WEST-CREE KWII
    LineBreakClass::ULB_AL, // 0x147A  # CANADIAN SYLLABICS KWO
    LineBreakClass::ULB_AL, // 0x147B  # CANADIAN SYLLABICS WEST-CREE KWO
    LineBreakClass::ULB_AL, // 0x147C  # CANADIAN SYLLABICS KWOO
    LineBreakClass::ULB_AL, // 0x147D  # CANADIAN SYLLABICS WEST-CREE KWOO
    LineBreakClass::ULB_AL, // 0x147E  # CANADIAN SYLLABICS KWA
    LineBreakClass::ULB_AL, // 0x147F  # CANADIAN SYLLABICS WEST-CREE KWA
    LineBreakClass::ULB_AL, // 0x1480  # CANADIAN SYLLABICS KWAA
    LineBreakClass::ULB_AL, // 0x1481  # CANADIAN SYLLABICS WEST-CREE KWAA
    LineBreakClass::ULB_AL, // 0x1482  # CANADIAN SYLLABICS NASKAPI KWAA
    LineBreakClass::ULB_AL, // 0x1483  # CANADIAN SYLLABICS K
    LineBreakClass::ULB_AL, // 0x1484  # CANADIAN SYLLABICS KW
    LineBreakClass::ULB_AL, // 0x1485  # CANADIAN SYLLABICS SOUTH-SLAVEY KEH
    LineBreakClass::ULB_AL, // 0x1486  # CANADIAN SYLLABICS SOUTH-SLAVEY KIH
    LineBreakClass::ULB_AL, // 0x1487  # CANADIAN SYLLABICS SOUTH-SLAVEY KOH
    LineBreakClass::ULB_AL, // 0x1488  # CANADIAN SYLLABICS SOUTH-SLAVEY KAH
    LineBreakClass::ULB_AL, // 0x1489  # CANADIAN SYLLABICS CE
    LineBreakClass::ULB_AL, // 0x148A  # CANADIAN SYLLABICS CAAI
    LineBreakClass::ULB_AL, // 0x148B  # CANADIAN SYLLABICS CI
    LineBreakClass::ULB_AL, // 0x148C  # CANADIAN SYLLABICS CII
    LineBreakClass::ULB_AL, // 0x148D  # CANADIAN SYLLABICS CO
    LineBreakClass::ULB_AL, // 0x148E  # CANADIAN SYLLABICS COO
    LineBreakClass::ULB_AL, // 0x148F  # CANADIAN SYLLABICS Y-CREE COO
    LineBreakClass::ULB_AL, // 0x1490  # CANADIAN SYLLABICS CA
    LineBreakClass::ULB_AL, // 0x1491  # CANADIAN SYLLABICS CAA
    LineBreakClass::ULB_AL, // 0x1492  # CANADIAN SYLLABICS CWE
    LineBreakClass::ULB_AL, // 0x1493  # CANADIAN SYLLABICS WEST-CREE CWE
    LineBreakClass::ULB_AL, // 0x1494  # CANADIAN SYLLABICS CWI
    LineBreakClass::ULB_AL, // 0x1495  # CANADIAN SYLLABICS WEST-CREE CWI
    LineBreakClass::ULB_AL, // 0x1496  # CANADIAN SYLLABICS CWII
    LineBreakClass::ULB_AL, // 0x1497  # CANADIAN SYLLABICS WEST-CREE CWII
    LineBreakClass::ULB_AL, // 0x1498  # CANADIAN SYLLABICS CWO
    LineBreakClass::ULB_AL, // 0x1499  # CANADIAN SYLLABICS WEST-CREE CWO
    LineBreakClass::ULB_AL, // 0x149A  # CANADIAN SYLLABICS CWOO
    LineBreakClass::ULB_AL, // 0x149B  # CANADIAN SYLLABICS WEST-CREE CWOO
    LineBreakClass::ULB_AL, // 0x149C  # CANADIAN SYLLABICS CWA
    LineBreakClass::ULB_AL, // 0x149D  # CANADIAN SYLLABICS WEST-CREE CWA
    LineBreakClass::ULB_AL, // 0x149E  # CANADIAN SYLLABICS CWAA
    LineBreakClass::ULB_AL, // 0x149F  # CANADIAN SYLLABICS WEST-CREE CWAA
    LineBreakClass::ULB_AL, // 0x14A0  # CANADIAN SYLLABICS NASKAPI CWAA
    LineBreakClass::ULB_AL, // 0x14A1  # CANADIAN SYLLABICS C
    LineBreakClass::ULB_AL, // 0x14A2  # CANADIAN SYLLABICS SAYISI TH
    LineBreakClass::ULB_AL, // 0x14A3  # CANADIAN SYLLABICS ME
    LineBreakClass::ULB_AL, // 0x14A4  # CANADIAN SYLLABICS MAAI
    LineBreakClass::ULB_AL, // 0x14A5  # CANADIAN SYLLABICS MI
    LineBreakClass::ULB_AL, // 0x14A6  # CANADIAN SYLLABICS MII
    LineBreakClass::ULB_AL, // 0x14A7  # CANADIAN SYLLABICS MO
    LineBreakClass::ULB_AL, // 0x14A8  # CANADIAN SYLLABICS MOO
    LineBreakClass::ULB_AL, // 0x14A9  # CANADIAN SYLLABICS Y-CREE MOO
    LineBreakClass::ULB_AL, // 0x14AA  # CANADIAN SYLLABICS MA
    LineBreakClass::ULB_AL, // 0x14AB  # CANADIAN SYLLABICS MAA
    LineBreakClass::ULB_AL, // 0x14AC  # CANADIAN SYLLABICS MWE
    LineBreakClass::ULB_AL, // 0x14AD  # CANADIAN SYLLABICS WEST-CREE MWE
    LineBreakClass::ULB_AL, // 0x14AE  # CANADIAN SYLLABICS MWI
    LineBreakClass::ULB_AL, // 0x14AF  # CANADIAN SYLLABICS WEST-CREE MWI
    LineBreakClass::ULB_AL, // 0x14B0  # CANADIAN SYLLABICS MWII
    LineBreakClass::ULB_AL, // 0x14B1  # CANADIAN SYLLABICS WEST-CREE MWII
    LineBreakClass::ULB_AL, // 0x14B2  # CANADIAN SYLLABICS MWO
    LineBreakClass::ULB_AL, // 0x14B3  # CANADIAN SYLLABICS WEST-CREE MWO
    LineBreakClass::ULB_AL, // 0x14B4  # CANADIAN SYLLABICS MWOO
    LineBreakClass::ULB_AL, // 0x14B5  # CANADIAN SYLLABICS WEST-CREE MWOO
    LineBreakClass::ULB_AL, // 0x14B6  # CANADIAN SYLLABICS MWA
    LineBreakClass::ULB_AL, // 0x14B7  # CANADIAN SYLLABICS WEST-CREE MWA
    LineBreakClass::ULB_AL, // 0x14B8  # CANADIAN SYLLABICS MWAA
    LineBreakClass::ULB_AL, // 0x14B9  # CANADIAN SYLLABICS WEST-CREE MWAA
    LineBreakClass::ULB_AL, // 0x14BA  # CANADIAN SYLLABICS NASKAPI MWAA
    LineBreakClass::ULB_AL, // 0x14BB  # CANADIAN SYLLABICS M
    LineBreakClass::ULB_AL, // 0x14BC  # CANADIAN SYLLABICS WEST-CREE M
    LineBreakClass::ULB_AL, // 0x14BD  # CANADIAN SYLLABICS MH
    LineBreakClass::ULB_AL, // 0x14BE  # CANADIAN SYLLABICS ATHAPASCAN M
    LineBreakClass::ULB_AL, // 0x14BF  # CANADIAN SYLLABICS SAYISI M
    LineBreakClass::ULB_AL, // 0x14C0  # CANADIAN SYLLABICS NE
    LineBreakClass::ULB_AL, // 0x14C1  # CANADIAN SYLLABICS NAAI
    LineBreakClass::ULB_AL, // 0x14C2  # CANADIAN SYLLABICS NI
    LineBreakClass::ULB_AL, // 0x14C3  # CANADIAN SYLLABICS NII
    LineBreakClass::ULB_AL, // 0x14C4  # CANADIAN SYLLABICS NO
    LineBreakClass::ULB_AL, // 0x14C5  # CANADIAN SYLLABICS NOO
    LineBreakClass::ULB_AL, // 0x14C6  # CANADIAN SYLLABICS Y-CREE NOO
    LineBreakClass::ULB_AL, // 0x14C7  # CANADIAN SYLLABICS NA
    LineBreakClass::ULB_AL, // 0x14C8  # CANADIAN SYLLABICS NAA
    LineBreakClass::ULB_AL, // 0x14C9  # CANADIAN SYLLABICS NWE
    LineBreakClass::ULB_AL, // 0x14CA  # CANADIAN SYLLABICS WEST-CREE NWE
    LineBreakClass::ULB_AL, // 0x14CB  # CANADIAN SYLLABICS NWA
    LineBreakClass::ULB_AL, // 0x14CC  # CANADIAN SYLLABICS WEST-CREE NWA
    LineBreakClass::ULB_AL, // 0x14CD  # CANADIAN SYLLABICS NWAA
    LineBreakClass::ULB_AL, // 0x14CE  # CANADIAN SYLLABICS WEST-CREE NWAA
    LineBreakClass::ULB_AL, // 0x14CF  # CANADIAN SYLLABICS NASKAPI NWAA
    LineBreakClass::ULB_AL, // 0x14D0  # CANADIAN SYLLABICS N
    LineBreakClass::ULB_AL, // 0x14D1  # CANADIAN SYLLABICS CARRIER NG
    LineBreakClass::ULB_AL, // 0x14D2  # CANADIAN SYLLABICS NH
    LineBreakClass::ULB_AL, // 0x14D3  # CANADIAN SYLLABICS LE
    LineBreakClass::ULB_AL, // 0x14D4  # CANADIAN SYLLABICS LAAI
    LineBreakClass::ULB_AL, // 0x14D5  # CANADIAN SYLLABICS LI
    LineBreakClass::ULB_AL, // 0x14D6  # CANADIAN SYLLABICS LII
    LineBreakClass::ULB_AL, // 0x14D7  # CANADIAN SYLLABICS LO
    LineBreakClass::ULB_AL, // 0x14D8  # CANADIAN SYLLABICS LOO
    LineBreakClass::ULB_AL, // 0x14D9  # CANADIAN SYLLABICS Y-CREE LOO
    LineBreakClass::ULB_AL, // 0x14DA  # CANADIAN SYLLABICS LA
    LineBreakClass::ULB_AL, // 0x14DB  # CANADIAN SYLLABICS LAA
    LineBreakClass::ULB_AL, // 0x14DC  # CANADIAN SYLLABICS LWE
    LineBreakClass::ULB_AL, // 0x14DD  # CANADIAN SYLLABICS WEST-CREE LWE
    LineBreakClass::ULB_AL, // 0x14DE  # CANADIAN SYLLABICS LWI
    LineBreakClass::ULB_AL, // 0x14DF  # CANADIAN SYLLABICS WEST-CREE LWI
    LineBreakClass::ULB_AL, // 0x14E0  # CANADIAN SYLLABICS LWII
    LineBreakClass::ULB_AL, // 0x14E1  # CANADIAN SYLLABICS WEST-CREE LWII
    LineBreakClass::ULB_AL, // 0x14E2  # CANADIAN SYLLABICS LWO
    LineBreakClass::ULB_AL, // 0x14E3  # CANADIAN SYLLABICS WEST-CREE LWO
    LineBreakClass::ULB_AL, // 0x14E4  # CANADIAN SYLLABICS LWOO
    LineBreakClass::ULB_AL, // 0x14E5  # CANADIAN SYLLABICS WEST-CREE LWOO
    LineBreakClass::ULB_AL, // 0x14E6  # CANADIAN SYLLABICS LWA
    LineBreakClass::ULB_AL, // 0x14E7  # CANADIAN SYLLABICS WEST-CREE LWA
    LineBreakClass::ULB_AL, // 0x14E8  # CANADIAN SYLLABICS LWAA
    LineBreakClass::ULB_AL, // 0x14E9  # CANADIAN SYLLABICS WEST-CREE LWAA
    LineBreakClass::ULB_AL, // 0x14EA  # CANADIAN SYLLABICS L
    LineBreakClass::ULB_AL, // 0x14EB  # CANADIAN SYLLABICS WEST-CREE L
    LineBreakClass::ULB_AL, // 0x14EC  # CANADIAN SYLLABICS MEDIAL L
    LineBreakClass::ULB_AL, // 0x14ED  # CANADIAN SYLLABICS SE
    LineBreakClass::ULB_AL, // 0x14EE  # CANADIAN SYLLABICS SAAI
    LineBreakClass::ULB_AL, // 0x14EF  # CANADIAN SYLLABICS SI
    LineBreakClass::ULB_AL, // 0x14F0  # CANADIAN SYLLABICS SII
    LineBreakClass::ULB_AL, // 0x14F1  # CANADIAN SYLLABICS SO
    LineBreakClass::ULB_AL, // 0x14F2  # CANADIAN SYLLABICS SOO
    LineBreakClass::ULB_AL, // 0x14F3  # CANADIAN SYLLABICS Y-CREE SOO
    LineBreakClass::ULB_AL, // 0x14F4  # CANADIAN SYLLABICS SA
    LineBreakClass::ULB_AL, // 0x14F5  # CANADIAN SYLLABICS SAA
    LineBreakClass::ULB_AL, // 0x14F6  # CANADIAN SYLLABICS SWE
    LineBreakClass::ULB_AL, // 0x14F7  # CANADIAN SYLLABICS WEST-CREE SWE
    LineBreakClass::ULB_AL, // 0x14F8  # CANADIAN SYLLABICS SWI
    LineBreakClass::ULB_AL, // 0x14F9  # CANADIAN SYLLABICS WEST-CREE SWI
    LineBreakClass::ULB_AL, // 0x14FA  # CANADIAN SYLLABICS SWII
    LineBreakClass::ULB_AL, // 0x14FB  # CANADIAN SYLLABICS WEST-CREE SWII
    LineBreakClass::ULB_AL, // 0x14FC  # CANADIAN SYLLABICS SWO
    LineBreakClass::ULB_AL, // 0x14FD  # CANADIAN SYLLABICS WEST-CREE SWO
    LineBreakClass::ULB_AL, // 0x14FE  # CANADIAN SYLLABICS SWOO
    LineBreakClass::ULB_AL, // 0x14FF  # CANADIAN SYLLABICS WEST-CREE SWOO
    LineBreakClass::ULB_AL, // 0x1500  # CANADIAN SYLLABICS SWA
    LineBreakClass::ULB_AL, // 0x1501  # CANADIAN SYLLABICS WEST-CREE SWA
    LineBreakClass::ULB_AL, // 0x1502  # CANADIAN SYLLABICS SWAA
    LineBreakClass::ULB_AL, // 0x1503  # CANADIAN SYLLABICS WEST-CREE SWAA
    LineBreakClass::ULB_AL, // 0x1504  # CANADIAN SYLLABICS NASKAPI SWAA
    LineBreakClass::ULB_AL, // 0x1505  # CANADIAN SYLLABICS S
    LineBreakClass::ULB_AL, // 0x1506  # CANADIAN SYLLABICS ATHAPASCAN S
    LineBreakClass::ULB_AL, // 0x1507  # CANADIAN SYLLABICS SW
    LineBreakClass::ULB_AL, // 0x1508  # CANADIAN SYLLABICS BLACKFOOT S
    LineBreakClass::ULB_AL, // 0x1509  # CANADIAN SYLLABICS MOOSE-CREE SK
    LineBreakClass::ULB_AL, // 0x150A  # CANADIAN SYLLABICS NASKAPI SKW
    LineBreakClass::ULB_AL, // 0x150B  # CANADIAN SYLLABICS NASKAPI S-W
    LineBreakClass::ULB_AL, // 0x150C  # CANADIAN SYLLABICS NASKAPI SPWA
    LineBreakClass::ULB_AL, // 0x150D  # CANADIAN SYLLABICS NASKAPI STWA
    LineBreakClass::ULB_AL, // 0x150E  # CANADIAN SYLLABICS NASKAPI SKWA
    LineBreakClass::ULB_AL, // 0x150F  # CANADIAN SYLLABICS NASKAPI SCWA
    LineBreakClass::ULB_AL, // 0x1510  # CANADIAN SYLLABICS SHE
    LineBreakClass::ULB_AL, // 0x1511  # CANADIAN SYLLABICS SHI
    LineBreakClass::ULB_AL, // 0x1512  # CANADIAN SYLLABICS SHII
    LineBreakClass::ULB_AL, // 0x1513  # CANADIAN SYLLABICS SHO
    LineBreakClass::ULB_AL, // 0x1514  # CANADIAN SYLLABICS SHOO
    LineBreakClass::ULB_AL, // 0x1515  # CANADIAN SYLLABICS SHA
    LineBreakClass::ULB_AL, // 0x1516  # CANADIAN SYLLABICS SHAA
    LineBreakClass::ULB_AL, // 0x1517  # CANADIAN SYLLABICS SHWE
    LineBreakClass::ULB_AL, // 0x1518  # CANADIAN SYLLABICS WEST-CREE SHWE
    LineBreakClass::ULB_AL, // 0x1519  # CANADIAN SYLLABICS SHWI
    LineBreakClass::ULB_AL, // 0x151A  # CANADIAN SYLLABICS WEST-CREE SHWI
    LineBreakClass::ULB_AL, // 0x151B  # CANADIAN SYLLABICS SHWII
    LineBreakClass::ULB_AL, // 0x151C  # CANADIAN SYLLABICS WEST-CREE SHWII
    LineBreakClass::ULB_AL, // 0x151D  # CANADIAN SYLLABICS SHWO
    LineBreakClass::ULB_AL, // 0x151E  # CANADIAN SYLLABICS WEST-CREE SHWO
    LineBreakClass::ULB_AL, // 0x151F  # CANADIAN SYLLABICS SHWOO
    LineBreakClass::ULB_AL, // 0x1520  # CANADIAN SYLLABICS WEST-CREE SHWOO
    LineBreakClass::ULB_AL, // 0x1521  # CANADIAN SYLLABICS SHWA
    LineBreakClass::ULB_AL, // 0x1522  # CANADIAN SYLLABICS WEST-CREE SHWA
    LineBreakClass::ULB_AL, // 0x1523  # CANADIAN SYLLABICS SHWAA
    LineBreakClass::ULB_AL, // 0x1524  # CANADIAN SYLLABICS WEST-CREE SHWAA
    LineBreakClass::ULB_AL, // 0x1525  # CANADIAN SYLLABICS SH
    LineBreakClass::ULB_AL, // 0x1526  # CANADIAN SYLLABICS YE
    LineBreakClass::ULB_AL, // 0x1527  # CANADIAN SYLLABICS YAAI
    LineBreakClass::ULB_AL, // 0x1528  # CANADIAN SYLLABICS YI
    LineBreakClass::ULB_AL, // 0x1529  # CANADIAN SYLLABICS YII
    LineBreakClass::ULB_AL, // 0x152A  # CANADIAN SYLLABICS YO
    LineBreakClass::ULB_AL, // 0x152B  # CANADIAN SYLLABICS YOO
    LineBreakClass::ULB_AL, // 0x152C  # CANADIAN SYLLABICS Y-CREE YOO
    LineBreakClass::ULB_AL, // 0x152D  # CANADIAN SYLLABICS YA
    LineBreakClass::ULB_AL, // 0x152E  # CANADIAN SYLLABICS YAA
    LineBreakClass::ULB_AL, // 0x152F  # CANADIAN SYLLABICS YWE
    LineBreakClass::ULB_AL, // 0x1530  # CANADIAN SYLLABICS WEST-CREE YWE
    LineBreakClass::ULB_AL, // 0x1531  # CANADIAN SYLLABICS YWI
    LineBreakClass::ULB_AL, // 0x1532  # CANADIAN SYLLABICS WEST-CREE YWI
    LineBreakClass::ULB_AL, // 0x1533  # CANADIAN SYLLABICS YWII
    LineBreakClass::ULB_AL, // 0x1534  # CANADIAN SYLLABICS WEST-CREE YWII
    LineBreakClass::ULB_AL, // 0x1535  # CANADIAN SYLLABICS YWO
    LineBreakClass::ULB_AL, // 0x1536  # CANADIAN SYLLABICS WEST-CREE YWO
    LineBreakClass::ULB_AL, // 0x1537  # CANADIAN SYLLABICS YWOO
    LineBreakClass::ULB_AL, // 0x1538  # CANADIAN SYLLABICS WEST-CREE YWOO
    LineBreakClass::ULB_AL, // 0x1539  # CANADIAN SYLLABICS YWA
    LineBreakClass::ULB_AL, // 0x153A  # CANADIAN SYLLABICS WEST-CREE YWA
    LineBreakClass::ULB_AL, // 0x153B  # CANADIAN SYLLABICS YWAA
    LineBreakClass::ULB_AL, // 0x153C  # CANADIAN SYLLABICS WEST-CREE YWAA
    LineBreakClass::ULB_AL, // 0x153D  # CANADIAN SYLLABICS NASKAPI YWAA
    LineBreakClass::ULB_AL, // 0x153E  # CANADIAN SYLLABICS Y
    LineBreakClass::ULB_AL, // 0x153F  # CANADIAN SYLLABICS BIBLE-CREE Y
    LineBreakClass::ULB_AL, // 0x1540  # CANADIAN SYLLABICS WEST-CREE Y
    LineBreakClass::ULB_AL, // 0x1541  # CANADIAN SYLLABICS SAYISI YI
    LineBreakClass::ULB_AL, // 0x1542  # CANADIAN SYLLABICS RE
    LineBreakClass::ULB_AL, // 0x1543  # CANADIAN SYLLABICS R-CREE RE
    LineBreakClass::ULB_AL, // 0x1544  # CANADIAN SYLLABICS WEST-CREE LE
    LineBreakClass::ULB_AL, // 0x1545  # CANADIAN SYLLABICS RAAI
    LineBreakClass::ULB_AL, // 0x1546  # CANADIAN SYLLABICS RI
    LineBreakClass::ULB_AL, // 0x1547  # CANADIAN SYLLABICS RII
    LineBreakClass::ULB_AL, // 0x1548  # CANADIAN SYLLABICS RO
    LineBreakClass::ULB_AL, // 0x1549  # CANADIAN SYLLABICS ROO
    LineBreakClass::ULB_AL, // 0x154A  # CANADIAN SYLLABICS WEST-CREE LO
    LineBreakClass::ULB_AL, // 0x154B  # CANADIAN SYLLABICS RA
    LineBreakClass::ULB_AL, // 0x154C  # CANADIAN SYLLABICS RAA
    LineBreakClass::ULB_AL, // 0x154D  # CANADIAN SYLLABICS WEST-CREE LA
    LineBreakClass::ULB_AL, // 0x154E  # CANADIAN SYLLABICS RWAA
    LineBreakClass::ULB_AL, // 0x154F  # CANADIAN SYLLABICS WEST-CREE RWAA
    LineBreakClass::ULB_AL, // 0x1550  # CANADIAN SYLLABICS R
    LineBreakClass::ULB_AL, // 0x1551  # CANADIAN SYLLABICS WEST-CREE R
    LineBreakClass::ULB_AL, // 0x1552  # CANADIAN SYLLABICS MEDIAL R
    LineBreakClass::ULB_AL, // 0x1553  # CANADIAN SYLLABICS FE
    LineBreakClass::ULB_AL, // 0x1554  # CANADIAN SYLLABICS FAAI
    LineBreakClass::ULB_AL, // 0x1555  # CANADIAN SYLLABICS FI
    LineBreakClass::ULB_AL, // 0x1556  # CANADIAN SYLLABICS FII
    LineBreakClass::ULB_AL, // 0x1557  # CANADIAN SYLLABICS FO
    LineBreakClass::ULB_AL, // 0x1558  # CANADIAN SYLLABICS FOO
    LineBreakClass::ULB_AL, // 0x1559  # CANADIAN SYLLABICS FA
    LineBreakClass::ULB_AL, // 0x155A  # CANADIAN SYLLABICS FAA
    LineBreakClass::ULB_AL, // 0x155B  # CANADIAN SYLLABICS FWAA
    LineBreakClass::ULB_AL, // 0x155C  # CANADIAN SYLLABICS WEST-CREE FWAA
    LineBreakClass::ULB_AL, // 0x155D  # CANADIAN SYLLABICS F
    LineBreakClass::ULB_AL, // 0x155E  # CANADIAN SYLLABICS THE
    LineBreakClass::ULB_AL, // 0x155F  # CANADIAN SYLLABICS N-CREE THE
    LineBreakClass::ULB_AL, // 0x1560  # CANADIAN SYLLABICS THI
    LineBreakClass::ULB_AL, // 0x1561  # CANADIAN SYLLABICS N-CREE THI
    LineBreakClass::ULB_AL, // 0x1562  # CANADIAN SYLLABICS THII
    LineBreakClass::ULB_AL, // 0x1563  # CANADIAN SYLLABICS N-CREE THII
    LineBreakClass::ULB_AL, // 0x1564  # CANADIAN SYLLABICS THO
    LineBreakClass::ULB_AL, // 0x1565  # CANADIAN SYLLABICS THOO
    LineBreakClass::ULB_AL, // 0x1566  # CANADIAN SYLLABICS THA
    LineBreakClass::ULB_AL, // 0x1567  # CANADIAN SYLLABICS THAA
    LineBreakClass::ULB_AL, // 0x1568  # CANADIAN SYLLABICS THWAA
    LineBreakClass::ULB_AL, // 0x1569  # CANADIAN SYLLABICS WEST-CREE THWAA
    LineBreakClass::ULB_AL, // 0x156A  # CANADIAN SYLLABICS TH
    LineBreakClass::ULB_AL, // 0x156B  # CANADIAN SYLLABICS TTHE
    LineBreakClass::ULB_AL, // 0x156C  # CANADIAN SYLLABICS TTHI
    LineBreakClass::ULB_AL, // 0x156D  # CANADIAN SYLLABICS TTHO
    LineBreakClass::ULB_AL, // 0x156E  # CANADIAN SYLLABICS TTHA
    LineBreakClass::ULB_AL, // 0x156F  # CANADIAN SYLLABICS TTH
    LineBreakClass::ULB_AL, // 0x1570  # CANADIAN SYLLABICS TYE
    LineBreakClass::ULB_AL, // 0x1571  # CANADIAN SYLLABICS TYI
    LineBreakClass::ULB_AL, // 0x1572  # CANADIAN SYLLABICS TYO
    LineBreakClass::ULB_AL, // 0x1573  # CANADIAN SYLLABICS TYA
    LineBreakClass::ULB_AL, // 0x1574  # CANADIAN SYLLABICS NUNAVIK HE
    LineBreakClass::ULB_AL, // 0x1575  # CANADIAN SYLLABICS NUNAVIK HI
    LineBreakClass::ULB_AL, // 0x1576  # CANADIAN SYLLABICS NUNAVIK HII
    LineBreakClass::ULB_AL, // 0x1577  # CANADIAN SYLLABICS NUNAVIK HO
    LineBreakClass::ULB_AL, // 0x1578  # CANADIAN SYLLABICS NUNAVIK HOO
    LineBreakClass::ULB_AL, // 0x1579  # CANADIAN SYLLABICS NUNAVIK HA
    LineBreakClass::ULB_AL, // 0x157A  # CANADIAN SYLLABICS NUNAVIK HAA
    LineBreakClass::ULB_AL, // 0x157B  # CANADIAN SYLLABICS NUNAVIK H
    LineBreakClass::ULB_AL, // 0x157C  # CANADIAN SYLLABICS NUNAVUT H
    LineBreakClass::ULB_AL, // 0x157D  # CANADIAN SYLLABICS HK
    LineBreakClass::ULB_AL, // 0x157E  # CANADIAN SYLLABICS QAAI
    LineBreakClass::ULB_AL, // 0x157F  # CANADIAN SYLLABICS QI
    LineBreakClass::ULB_AL, // 0x1580  # CANADIAN SYLLABICS QII
    LineBreakClass::ULB_AL, // 0x1581  # CANADIAN SYLLABICS QO
    LineBreakClass::ULB_AL, // 0x1582  # CANADIAN SYLLABICS QOO
    LineBreakClass::ULB_AL, // 0x1583  # CANADIAN SYLLABICS QA
    LineBreakClass::ULB_AL, // 0x1584  # CANADIAN SYLLABICS QAA
    LineBreakClass::ULB_AL, // 0x1585  # CANADIAN SYLLABICS Q
    LineBreakClass::ULB_AL, // 0x1586  # CANADIAN SYLLABICS TLHE
    LineBreakClass::ULB_AL, // 0x1587  # CANADIAN SYLLABICS TLHI
    LineBreakClass::ULB_AL, // 0x1588  # CANADIAN SYLLABICS TLHO
    LineBreakClass::ULB_AL, // 0x1589  # CANADIAN SYLLABICS TLHA
    LineBreakClass::ULB_AL, // 0x158A  # CANADIAN SYLLABICS WEST-CREE RE
    LineBreakClass::ULB_AL, // 0x158B  # CANADIAN SYLLABICS WEST-CREE RI
    LineBreakClass::ULB_AL, // 0x158C  # CANADIAN SYLLABICS WEST-CREE RO
    LineBreakClass::ULB_AL, // 0x158D  # CANADIAN SYLLABICS WEST-CREE RA
    LineBreakClass::ULB_AL, // 0x158E  # CANADIAN SYLLABICS NGAAI
    LineBreakClass::ULB_AL, // 0x158F  # CANADIAN SYLLABICS NGI
    LineBreakClass::ULB_AL, // 0x1590  # CANADIAN SYLLABICS NGII
    LineBreakClass::ULB_AL, // 0x1591  # CANADIAN SYLLABICS NGO
    LineBreakClass::ULB_AL, // 0x1592  # CANADIAN SYLLABICS NGOO
    LineBreakClass::ULB_AL, // 0x1593  # CANADIAN SYLLABICS NGA
    LineBreakClass::ULB_AL, // 0x1594  # CANADIAN SYLLABICS NGAA
    LineBreakClass::ULB_AL, // 0x1595  # CANADIAN SYLLABICS NG
    LineBreakClass::ULB_AL, // 0x1596  # CANADIAN SYLLABICS NNG
    LineBreakClass::ULB_AL, // 0x1597  # CANADIAN SYLLABICS SAYISI SHE
    LineBreakClass::ULB_AL, // 0x1598  # CANADIAN SYLLABICS SAYISI SHI
    LineBreakClass::ULB_AL, // 0x1599  # CANADIAN SYLLABICS SAYISI SHO
    LineBreakClass::ULB_AL, // 0x159A  # CANADIAN SYLLABICS SAYISI SHA
    LineBreakClass::ULB_AL, // 0x159B  # CANADIAN SYLLABICS WOODS-CREE THE
    LineBreakClass::ULB_AL, // 0x159C  # CANADIAN SYLLABICS WOODS-CREE THI
    LineBreakClass::ULB_AL, // 0x159D  # CANADIAN SYLLABICS WOODS-CREE THO
    LineBreakClass::ULB_AL, // 0x159E  # CANADIAN SYLLABICS WOODS-CREE THA
    LineBreakClass::ULB_AL, // 0x159F  # CANADIAN SYLLABICS WOODS-CREE TH
    LineBreakClass::ULB_AL, // 0x15A0  # CANADIAN SYLLABICS LHI
    LineBreakClass::ULB_AL, // 0x15A1  # CANADIAN SYLLABICS LHII
    LineBreakClass::ULB_AL, // 0x15A2  # CANADIAN SYLLABICS LHO
    LineBreakClass::ULB_AL, // 0x15A3  # CANADIAN SYLLABICS LHOO
    LineBreakClass::ULB_AL, // 0x15A4  # CANADIAN SYLLABICS LHA
    LineBreakClass::ULB_AL, // 0x15A5  # CANADIAN SYLLABICS LHAA
    LineBreakClass::ULB_AL, // 0x15A6  # CANADIAN SYLLABICS LH
    LineBreakClass::ULB_AL, // 0x15A7  # CANADIAN SYLLABICS TH-CREE THE
    LineBreakClass::ULB_AL, // 0x15A8  # CANADIAN SYLLABICS TH-CREE THI
    LineBreakClass::ULB_AL, // 0x15A9  # CANADIAN SYLLABICS TH-CREE THII
    LineBreakClass::ULB_AL, // 0x15AA  # CANADIAN SYLLABICS TH-CREE THO
    LineBreakClass::ULB_AL, // 0x15AB  # CANADIAN SYLLABICS TH-CREE THOO
    LineBreakClass::ULB_AL, // 0x15AC  # CANADIAN SYLLABICS TH-CREE THA
    LineBreakClass::ULB_AL, // 0x15AD  # CANADIAN SYLLABICS TH-CREE THAA
    LineBreakClass::ULB_AL, // 0x15AE  # CANADIAN SYLLABICS TH-CREE TH
    LineBreakClass::ULB_AL, // 0x15AF  # CANADIAN SYLLABICS AIVILIK B
    LineBreakClass::ULB_AL, // 0x15B0  # CANADIAN SYLLABICS BLACKFOOT E
    LineBreakClass::ULB_AL, // 0x15B1  # CANADIAN SYLLABICS BLACKFOOT I
    LineBreakClass::ULB_AL, // 0x15B2  # CANADIAN SYLLABICS BLACKFOOT O
    LineBreakClass::ULB_AL, // 0x15B3  # CANADIAN SYLLABICS BLACKFOOT A
    LineBreakClass::ULB_AL, // 0x15B4  # CANADIAN SYLLABICS BLACKFOOT WE
    LineBreakClass::ULB_AL, // 0x15B5  # CANADIAN SYLLABICS BLACKFOOT WI
    LineBreakClass::ULB_AL, // 0x15B6  # CANADIAN SYLLABICS BLACKFOOT WO
    LineBreakClass::ULB_AL, // 0x15B7  # CANADIAN SYLLABICS BLACKFOOT WA
    LineBreakClass::ULB_AL, // 0x15B8  # CANADIAN SYLLABICS BLACKFOOT NE
    LineBreakClass::ULB_AL, // 0x15B9  # CANADIAN SYLLABICS BLACKFOOT NI
    LineBreakClass::ULB_AL, // 0x15BA  # CANADIAN SYLLABICS BLACKFOOT NO
    LineBreakClass::ULB_AL, // 0x15BB  # CANADIAN SYLLABICS BLACKFOOT NA
    LineBreakClass::ULB_AL, // 0x15BC  # CANADIAN SYLLABICS BLACKFOOT KE
    LineBreakClass::ULB_AL, // 0x15BD  # CANADIAN SYLLABICS BLACKFOOT KI
    LineBreakClass::ULB_AL, // 0x15BE  # CANADIAN SYLLABICS BLACKFOOT KO
    LineBreakClass::ULB_AL, // 0x15BF  # CANADIAN SYLLABICS BLACKFOOT KA
    LineBreakClass::ULB_AL, // 0x15C0  # CANADIAN SYLLABICS SAYISI HE
    LineBreakClass::ULB_AL, // 0x15C1  # CANADIAN SYLLABICS SAYISI HI
    LineBreakClass::ULB_AL, // 0x15C2  # CANADIAN SYLLABICS SAYISI HO
    LineBreakClass::ULB_AL, // 0x15C3  # CANADIAN SYLLABICS SAYISI HA
    LineBreakClass::ULB_AL, // 0x15C4  # CANADIAN SYLLABICS CARRIER GHU
    LineBreakClass::ULB_AL, // 0x15C5  # CANADIAN SYLLABICS CARRIER GHO
    LineBreakClass::ULB_AL, // 0x15C6  # CANADIAN SYLLABICS CARRIER GHE
    LineBreakClass::ULB_AL, // 0x15C7  # CANADIAN SYLLABICS CARRIER GHEE
    LineBreakClass::ULB_AL, // 0x15C8  # CANADIAN SYLLABICS CARRIER GHI
    LineBreakClass::ULB_AL, // 0x15C9  # CANADIAN SYLLABICS CARRIER GHA
    LineBreakClass::ULB_AL, // 0x15CA  # CANADIAN SYLLABICS CARRIER RU
    LineBreakClass::ULB_AL, // 0x15CB  # CANADIAN SYLLABICS CARRIER RO
    LineBreakClass::ULB_AL, // 0x15CC  # CANADIAN SYLLABICS CARRIER RE
    LineBreakClass::ULB_AL, // 0x15CD  # CANADIAN SYLLABICS CARRIER REE
    LineBreakClass::ULB_AL, // 0x15CE  # CANADIAN SYLLABICS CARRIER RI
    LineBreakClass::ULB_AL, // 0x15CF  # CANADIAN SYLLABICS CARRIER RA
    LineBreakClass::ULB_AL, // 0x15D0  # CANADIAN SYLLABICS CARRIER WU
    LineBreakClass::ULB_AL, // 0x15D1  # CANADIAN SYLLABICS CARRIER WO
    LineBreakClass::ULB_AL, // 0x15D2  # CANADIAN SYLLABICS CARRIER WE
    LineBreakClass::ULB_AL, // 0x15D3  # CANADIAN SYLLABICS CARRIER WEE
    LineBreakClass::ULB_AL, // 0x15D4  # CANADIAN SYLLABICS CARRIER WI
    LineBreakClass::ULB_AL, // 0x15D5  # CANADIAN SYLLABICS CARRIER WA
    LineBreakClass::ULB_AL, // 0x15D6  # CANADIAN SYLLABICS CARRIER HWU
    LineBreakClass::ULB_AL, // 0x15D7  # CANADIAN SYLLABICS CARRIER HWO
    LineBreakClass::ULB_AL, // 0x15D8  # CANADIAN SYLLABICS CARRIER HWE
    LineBreakClass::ULB_AL, // 0x15D9  # CANADIAN SYLLABICS CARRIER HWEE
    LineBreakClass::ULB_AL, // 0x15DA  # CANADIAN SYLLABICS CARRIER HWI
    LineBreakClass::ULB_AL, // 0x15DB  # CANADIAN SYLLABICS CARRIER HWA
    LineBreakClass::ULB_AL, // 0x15DC  # CANADIAN SYLLABICS CARRIER THU
    LineBreakClass::ULB_AL, // 0x15DD  # CANADIAN SYLLABICS CARRIER THO
    LineBreakClass::ULB_AL, // 0x15DE  # CANADIAN SYLLABICS CARRIER THE
    LineBreakClass::ULB_AL, // 0x15DF  # CANADIAN SYLLABICS CARRIER THEE
    LineBreakClass::ULB_AL, // 0x15E0  # CANADIAN SYLLABICS CARRIER THI
    LineBreakClass::ULB_AL, // 0x15E1  # CANADIAN SYLLABICS CARRIER THA
    LineBreakClass::ULB_AL, // 0x15E2  # CANADIAN SYLLABICS CARRIER TTU
    LineBreakClass::ULB_AL, // 0x15E3  # CANADIAN SYLLABICS CARRIER TTO
    LineBreakClass::ULB_AL, // 0x15E4  # CANADIAN SYLLABICS CARRIER TTE
    LineBreakClass::ULB_AL, // 0x15E5  # CANADIAN SYLLABICS CARRIER TTEE
    LineBreakClass::ULB_AL, // 0x15E6  # CANADIAN SYLLABICS CARRIER TTI
    LineBreakClass::ULB_AL, // 0x15E7  # CANADIAN SYLLABICS CARRIER TTA
    LineBreakClass::ULB_AL, // 0x15E8  # CANADIAN SYLLABICS CARRIER PU
    LineBreakClass::ULB_AL, // 0x15E9  # CANADIAN SYLLABICS CARRIER PO
    LineBreakClass::ULB_AL, // 0x15EA  # CANADIAN SYLLABICS CARRIER PE
    LineBreakClass::ULB_AL, // 0x15EB  # CANADIAN SYLLABICS CARRIER PEE
    LineBreakClass::ULB_AL, // 0x15EC  # CANADIAN SYLLABICS CARRIER PI
    LineBreakClass::ULB_AL, // 0x15ED  # CANADIAN SYLLABICS CARRIER PA
    LineBreakClass::ULB_AL, // 0x15EE  # CANADIAN SYLLABICS CARRIER P
    LineBreakClass::ULB_AL, // 0x15EF  # CANADIAN SYLLABICS CARRIER GU
    LineBreakClass::ULB_AL, // 0x15F0  # CANADIAN SYLLABICS CARRIER GO
    LineBreakClass::ULB_AL, // 0x15F1  # CANADIAN SYLLABICS CARRIER GE
    LineBreakClass::ULB_AL, // 0x15F2  # CANADIAN SYLLABICS CARRIER GEE
    LineBreakClass::ULB_AL, // 0x15F3  # CANADIAN SYLLABICS CARRIER GI
    LineBreakClass::ULB_AL, // 0x15F4  # CANADIAN SYLLABICS CARRIER GA
    LineBreakClass::ULB_AL, // 0x15F5  # CANADIAN SYLLABICS CARRIER KHU
    LineBreakClass::ULB_AL, // 0x15F6  # CANADIAN SYLLABICS CARRIER KHO
    LineBreakClass::ULB_AL, // 0x15F7  # CANADIAN SYLLABICS CARRIER KHE
    LineBreakClass::ULB_AL, // 0x15F8  # CANADIAN SYLLABICS CARRIER KHEE
    LineBreakClass::ULB_AL, // 0x15F9  # CANADIAN SYLLABICS CARRIER KHI
    LineBreakClass::ULB_AL, // 0x15FA  # CANADIAN SYLLABICS CARRIER KHA
    LineBreakClass::ULB_AL, // 0x15FB  # CANADIAN SYLLABICS CARRIER KKU
    LineBreakClass::ULB_AL, // 0x15FC  # CANADIAN SYLLABICS CARRIER KKO
    LineBreakClass::ULB_AL, // 0x15FD  # CANADIAN SYLLABICS CARRIER KKE
    LineBreakClass::ULB_AL, // 0x15FE  # CANADIAN SYLLABICS CARRIER KKEE
    LineBreakClass::ULB_AL, // 0x15FF  # CANADIAN SYLLABICS CARRIER KKI
    LineBreakClass::ULB_AL, // 0x1600  # CANADIAN SYLLABICS CARRIER KKA
    LineBreakClass::ULB_AL, // 0x1601  # CANADIAN SYLLABICS CARRIER KK
    LineBreakClass::ULB_AL, // 0x1602  # CANADIAN SYLLABICS CARRIER NU
    LineBreakClass::ULB_AL, // 0x1603  # CANADIAN SYLLABICS CARRIER NO
    LineBreakClass::ULB_AL, // 0x1604  # CANADIAN SYLLABICS CARRIER NE
    LineBreakClass::ULB_AL, // 0x1605  # CANADIAN SYLLABICS CARRIER NEE
    LineBreakClass::ULB_AL, // 0x1606  # CANADIAN SYLLABICS CARRIER NI
    LineBreakClass::ULB_AL, // 0x1607  # CANADIAN SYLLABICS CARRIER NA
    LineBreakClass::ULB_AL, // 0x1608  # CANADIAN SYLLABICS CARRIER MU
    LineBreakClass::ULB_AL, // 0x1609  # CANADIAN SYLLABICS CARRIER MO
    LineBreakClass::ULB_AL, // 0x160A  # CANADIAN SYLLABICS CARRIER ME
    LineBreakClass::ULB_AL, // 0x160B  # CANADIAN SYLLABICS CARRIER MEE
    LineBreakClass::ULB_AL, // 0x160C  # CANADIAN SYLLABICS CARRIER MI
    LineBreakClass::ULB_AL, // 0x160D  # CANADIAN SYLLABICS CARRIER MA
    LineBreakClass::ULB_AL, // 0x160E  # CANADIAN SYLLABICS CARRIER YU
    LineBreakClass::ULB_AL, // 0x160F  # CANADIAN SYLLABICS CARRIER YO
    LineBreakClass::ULB_AL, // 0x1610  # CANADIAN SYLLABICS CARRIER YE
    LineBreakClass::ULB_AL, // 0x1611  # CANADIAN SYLLABICS CARRIER YEE
    LineBreakClass::ULB_AL, // 0x1612  # CANADIAN SYLLABICS CARRIER YI
    LineBreakClass::ULB_AL, // 0x1613  # CANADIAN SYLLABICS CARRIER YA
    LineBreakClass::ULB_AL, // 0x1614  # CANADIAN SYLLABICS CARRIER JU
    LineBreakClass::ULB_AL, // 0x1615  # CANADIAN SYLLABICS SAYISI JU
    LineBreakClass::ULB_AL, // 0x1616  # CANADIAN SYLLABICS CARRIER JO
    LineBreakClass::ULB_AL, // 0x1617  # CANADIAN SYLLABICS CARRIER JE
    LineBreakClass::ULB_AL, // 0x1618  # CANADIAN SYLLABICS CARRIER JEE
    LineBreakClass::ULB_AL, // 0x1619  # CANADIAN SYLLABICS CARRIER JI
    LineBreakClass::ULB_AL, // 0x161A  # CANADIAN SYLLABICS SAYISI JI
    LineBreakClass::ULB_AL, // 0x161B  # CANADIAN SYLLABICS CARRIER JA
    LineBreakClass::ULB_AL, // 0x161C  # CANADIAN SYLLABICS CARRIER JJU
    LineBreakClass::ULB_AL, // 0x161D  # CANADIAN SYLLABICS CARRIER JJO
    LineBreakClass::ULB_AL, // 0x161E  # CANADIAN SYLLABICS CARRIER JJE
    LineBreakClass::ULB_AL, // 0x161F  # CANADIAN SYLLABICS CARRIER JJEE
    LineBreakClass::ULB_AL, // 0x1620  # CANADIAN SYLLABICS CARRIER JJI
    LineBreakClass::ULB_AL, // 0x1621  # CANADIAN SYLLABICS CARRIER JJA
    LineBreakClass::ULB_AL, // 0x1622  # CANADIAN SYLLABICS CARRIER LU
    LineBreakClass::ULB_AL, // 0x1623  # CANADIAN SYLLABICS CARRIER LO
    LineBreakClass::ULB_AL, // 0x1624  # CANADIAN SYLLABICS CARRIER LE
    LineBreakClass::ULB_AL, // 0x1625  # CANADIAN SYLLABICS CARRIER LEE
    LineBreakClass::ULB_AL, // 0x1626  # CANADIAN SYLLABICS CARRIER LI
    LineBreakClass::ULB_AL, // 0x1627  # CANADIAN SYLLABICS CARRIER LA
    LineBreakClass::ULB_AL, // 0x1628  # CANADIAN SYLLABICS CARRIER DLU
    LineBreakClass::ULB_AL, // 0x1629  # CANADIAN SYLLABICS CARRIER DLO
    LineBreakClass::ULB_AL, // 0x162A  # CANADIAN SYLLABICS CARRIER DLE
    LineBreakClass::ULB_AL, // 0x162B  # CANADIAN SYLLABICS CARRIER DLEE
    LineBreakClass::ULB_AL, // 0x162C  # CANADIAN SYLLABICS CARRIER DLI
    LineBreakClass::ULB_AL, // 0x162D  # CANADIAN SYLLABICS CARRIER DLA
    LineBreakClass::ULB_AL, // 0x162E  # CANADIAN SYLLABICS CARRIER LHU
    LineBreakClass::ULB_AL, // 0x162F  # CANADIAN SYLLABICS CARRIER LHO
    LineBreakClass::ULB_AL, // 0x1630  # CANADIAN SYLLABICS CARRIER LHE
    LineBreakClass::ULB_AL, // 0x1631  # CANADIAN SYLLABICS CARRIER LHEE
    LineBreakClass::ULB_AL, // 0x1632  # CANADIAN SYLLABICS CARRIER LHI
    LineBreakClass::ULB_AL, // 0x1633  # CANADIAN SYLLABICS CARRIER LHA
    LineBreakClass::ULB_AL, // 0x1634  # CANADIAN SYLLABICS CARRIER TLHU
    LineBreakClass::ULB_AL, // 0x1635  # CANADIAN SYLLABICS CARRIER TLHO
    LineBreakClass::ULB_AL, // 0x1636  # CANADIAN SYLLABICS CARRIER TLHE
    LineBreakClass::ULB_AL, // 0x1637  # CANADIAN SYLLABICS CARRIER TLHEE
    LineBreakClass::ULB_AL, // 0x1638  # CANADIAN SYLLABICS CARRIER TLHI
    LineBreakClass::ULB_AL, // 0x1639  # CANADIAN SYLLABICS CARRIER TLHA
    LineBreakClass::ULB_AL, // 0x163A  # CANADIAN SYLLABICS CARRIER TLU
    LineBreakClass::ULB_AL, // 0x163B  # CANADIAN SYLLABICS CARRIER TLO
    LineBreakClass::ULB_AL, // 0x163C  # CANADIAN SYLLABICS CARRIER TLE
    LineBreakClass::ULB_AL, // 0x163D  # CANADIAN SYLLABICS CARRIER TLEE
    LineBreakClass::ULB_AL, // 0x163E  # CANADIAN SYLLABICS CARRIER TLI
    LineBreakClass::ULB_AL, // 0x163F  # CANADIAN SYLLABICS CARRIER TLA
    LineBreakClass::ULB_AL, // 0x1640  # CANADIAN SYLLABICS CARRIER ZU
    LineBreakClass::ULB_AL, // 0x1641  # CANADIAN SYLLABICS CARRIER ZO
    LineBreakClass::ULB_AL, // 0x1642  # CANADIAN SYLLABICS CARRIER ZE
    LineBreakClass::ULB_AL, // 0x1643  # CANADIAN SYLLABICS CARRIER ZEE
    LineBreakClass::ULB_AL, // 0x1644  # CANADIAN SYLLABICS CARRIER ZI
    LineBreakClass::ULB_AL, // 0x1645  # CANADIAN SYLLABICS CARRIER ZA
    LineBreakClass::ULB_AL, // 0x1646  # CANADIAN SYLLABICS CARRIER Z
    LineBreakClass::ULB_AL, // 0x1647  # CANADIAN SYLLABICS CARRIER INITIAL Z
    LineBreakClass::ULB_AL, // 0x1648  # CANADIAN SYLLABICS CARRIER DZU
    LineBreakClass::ULB_AL, // 0x1649  # CANADIAN SYLLABICS CARRIER DZO
    LineBreakClass::ULB_AL, // 0x164A  # CANADIAN SYLLABICS CARRIER DZE
    LineBreakClass::ULB_AL, // 0x164B  # CANADIAN SYLLABICS CARRIER DZEE
    LineBreakClass::ULB_AL, // 0x164C  # CANADIAN SYLLABICS CARRIER DZI
    LineBreakClass::ULB_AL, // 0x164D  # CANADIAN SYLLABICS CARRIER DZA
    LineBreakClass::ULB_AL, // 0x164E  # CANADIAN SYLLABICS CARRIER SU
    LineBreakClass::ULB_AL, // 0x164F  # CANADIAN SYLLABICS CARRIER SO
    LineBreakClass::ULB_AL, // 0x1650  # CANADIAN SYLLABICS CARRIER SE
    LineBreakClass::ULB_AL, // 0x1651  # CANADIAN SYLLABICS CARRIER SEE
    LineBreakClass::ULB_AL, // 0x1652  # CANADIAN SYLLABICS CARRIER SI
    LineBreakClass::ULB_AL, // 0x1653  # CANADIAN SYLLABICS CARRIER SA
    LineBreakClass::ULB_AL, // 0x1654  # CANADIAN SYLLABICS CARRIER SHU
    LineBreakClass::ULB_AL, // 0x1655  # CANADIAN SYLLABICS CARRIER SHO
    LineBreakClass::ULB_AL, // 0x1656  # CANADIAN SYLLABICS CARRIER SHE
    LineBreakClass::ULB_AL, // 0x1657  # CANADIAN SYLLABICS CARRIER SHEE
    LineBreakClass::ULB_AL, // 0x1658  # CANADIAN SYLLABICS CARRIER SHI
    LineBreakClass::ULB_AL, // 0x1659  # CANADIAN SYLLABICS CARRIER SHA
    LineBreakClass::ULB_AL, // 0x165A  # CANADIAN SYLLABICS CARRIER SH
    LineBreakClass::ULB_AL, // 0x165B  # CANADIAN SYLLABICS CARRIER TSU
    LineBreakClass::ULB_AL, // 0x165C  # CANADIAN SYLLABICS CARRIER TSO
    LineBreakClass::ULB_AL, // 0x165D  # CANADIAN SYLLABICS CARRIER TSE
    LineBreakClass::ULB_AL, // 0x165E  # CANADIAN SYLLABICS CARRIER TSEE
    LineBreakClass::ULB_AL, // 0x165F  # CANADIAN SYLLABICS CARRIER TSI
    LineBreakClass::ULB_AL, // 0x1660  # CANADIAN SYLLABICS CARRIER TSA
    LineBreakClass::ULB_AL, // 0x1661  # CANADIAN SYLLABICS CARRIER CHU
    LineBreakClass::ULB_AL, // 0x1662  # CANADIAN SYLLABICS CARRIER CHO
    LineBreakClass::ULB_AL, // 0x1663  # CANADIAN SYLLABICS CARRIER CHE
    LineBreakClass::ULB_AL, // 0x1664  # CANADIAN SYLLABICS CARRIER CHEE
    LineBreakClass::ULB_AL, // 0x1665  # CANADIAN SYLLABICS CARRIER CHI
    LineBreakClass::ULB_AL, // 0x1666  # CANADIAN SYLLABICS CARRIER CHA
    LineBreakClass::ULB_AL, // 0x1667  # CANADIAN SYLLABICS CARRIER TTSU
    LineBreakClass::ULB_AL, // 0x1668  # CANADIAN SYLLABICS CARRIER TTSO
    LineBreakClass::ULB_AL, // 0x1669  # CANADIAN SYLLABICS CARRIER TTSE
    LineBreakClass::ULB_AL, // 0x166A  # CANADIAN SYLLABICS CARRIER TTSEE
    LineBreakClass::ULB_AL, // 0x166B  # CANADIAN SYLLABICS CARRIER TTSI
    LineBreakClass::ULB_AL, // 0x166C  # CANADIAN SYLLABICS CARRIER TTSA
    LineBreakClass::ULB_AL, // 0x166D  # CANADIAN SYLLABICS CHI SIGN
    LineBreakClass::ULB_AL, // 0x166E  # CANADIAN SYLLABICS FULL STOP
    LineBreakClass::ULB_AL, // 0x166F  # CANADIAN SYLLABICS QAI
    LineBreakClass::ULB_AL, // 0x1670  # CANADIAN SYLLABICS NGAI
    LineBreakClass::ULB_AL, // 0x1671  # CANADIAN SYLLABICS NNGI
    LineBreakClass::ULB_AL, // 0x1672  # CANADIAN SYLLABICS NNGII
    LineBreakClass::ULB_AL, // 0x1673  # CANADIAN SYLLABICS NNGO
    LineBreakClass::ULB_AL, // 0x1674  # CANADIAN SYLLABICS NNGOO
    LineBreakClass::ULB_AL, // 0x1675  # CANADIAN SYLLABICS NNGA
    LineBreakClass::ULB_AL, // 0x1676  # CANADIAN SYLLABICS NNGAA
    LineBreakClass::ULB_ID, // 0x1677 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1678 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1679 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x167A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x167B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x167C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x167D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x167E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x167F # <UNDEFINED>
    LineBreakClass::ULB_BA, // 0x1680  # OGHAM SPACE MARK
    LineBreakClass::ULB_AL, // 0x1681  # OGHAM LETTER BEITH
    LineBreakClass::ULB_AL, // 0x1682  # OGHAM LETTER LUIS
    LineBreakClass::ULB_AL, // 0x1683  # OGHAM LETTER FEARN
    LineBreakClass::ULB_AL, // 0x1684  # OGHAM LETTER SAIL
    LineBreakClass::ULB_AL, // 0x1685  # OGHAM LETTER NION
    LineBreakClass::ULB_AL, // 0x1686  # OGHAM LETTER UATH
    LineBreakClass::ULB_AL, // 0x1687  # OGHAM LETTER DAIR
    LineBreakClass::ULB_AL, // 0x1688  # OGHAM LETTER TINNE
    LineBreakClass::ULB_AL, // 0x1689  # OGHAM LETTER COLL
    LineBreakClass::ULB_AL, // 0x168A  # OGHAM LETTER CEIRT
    LineBreakClass::ULB_AL, // 0x168B  # OGHAM LETTER MUIN
    LineBreakClass::ULB_AL, // 0x168C  # OGHAM LETTER GORT
    LineBreakClass::ULB_AL, // 0x168D  # OGHAM LETTER NGEADAL
    LineBreakClass::ULB_AL, // 0x168E  # OGHAM LETTER STRAIF
    LineBreakClass::ULB_AL, // 0x168F  # OGHAM LETTER RUIS
    LineBreakClass::ULB_AL, // 0x1690  # OGHAM LETTER AILM
    LineBreakClass::ULB_AL, // 0x1691  # OGHAM LETTER ONN
    LineBreakClass::ULB_AL, // 0x1692  # OGHAM LETTER UR
    LineBreakClass::ULB_AL, // 0x1693  # OGHAM LETTER EADHADH
    LineBreakClass::ULB_AL, // 0x1694  # OGHAM LETTER IODHADH
    LineBreakClass::ULB_AL, // 0x1695  # OGHAM LETTER EABHADH
    LineBreakClass::ULB_AL, // 0x1696  # OGHAM LETTER OR
    LineBreakClass::ULB_AL, // 0x1697  # OGHAM LETTER UILLEANN
    LineBreakClass::ULB_AL, // 0x1698  # OGHAM LETTER IFIN
    LineBreakClass::ULB_AL, // 0x1699  # OGHAM LETTER EAMHANCHOLL
    LineBreakClass::ULB_AL, // 0x169A  # OGHAM LETTER PEITH
    LineBreakClass::ULB_OP, // 0x169B  # OGHAM FEATHER MARK
    LineBreakClass::ULB_CL, // 0x169C  # OGHAM REVERSED FEATHER MARK
    LineBreakClass::ULB_ID, // 0x169D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x169E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x169F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x16A0  # RUNIC LETTER FEHU FEOH FE F
    LineBreakClass::ULB_AL, // 0x16A1  # RUNIC LETTER V
    LineBreakClass::ULB_AL, // 0x16A2  # RUNIC LETTER URUZ UR U
    LineBreakClass::ULB_AL, // 0x16A3  # RUNIC LETTER YR
    LineBreakClass::ULB_AL, // 0x16A4  # RUNIC LETTER Y
    LineBreakClass::ULB_AL, // 0x16A5  # RUNIC LETTER W
    LineBreakClass::ULB_AL, // 0x16A6  # RUNIC LETTER THURISAZ THURS THORN
    LineBreakClass::ULB_AL, // 0x16A7  # RUNIC LETTER ETH
    LineBreakClass::ULB_AL, // 0x16A8  # RUNIC LETTER ANSUZ A
    LineBreakClass::ULB_AL, // 0x16A9  # RUNIC LETTER OS O
    LineBreakClass::ULB_AL, // 0x16AA  # RUNIC LETTER AC A
    LineBreakClass::ULB_AL, // 0x16AB  # RUNIC LETTER AESC
    LineBreakClass::ULB_AL, // 0x16AC  # RUNIC LETTER LONG-BRANCH-OSS O
    LineBreakClass::ULB_AL, // 0x16AD  # RUNIC LETTER SHORT-TWIG-OSS O
    LineBreakClass::ULB_AL, // 0x16AE  # RUNIC LETTER O
    LineBreakClass::ULB_AL, // 0x16AF  # RUNIC LETTER OE
    LineBreakClass::ULB_AL, // 0x16B0  # RUNIC LETTER ON
    LineBreakClass::ULB_AL, // 0x16B1  # RUNIC LETTER RAIDO RAD REID R
    LineBreakClass::ULB_AL, // 0x16B2  # RUNIC LETTER KAUNA
    LineBreakClass::ULB_AL, // 0x16B3  # RUNIC LETTER CEN
    LineBreakClass::ULB_AL, // 0x16B4  # RUNIC LETTER KAUN K
    LineBreakClass::ULB_AL, // 0x16B5  # RUNIC LETTER G
    LineBreakClass::ULB_AL, // 0x16B6  # RUNIC LETTER ENG
    LineBreakClass::ULB_AL, // 0x16B7  # RUNIC LETTER GEBO GYFU G
    LineBreakClass::ULB_AL, // 0x16B8  # RUNIC LETTER GAR
    LineBreakClass::ULB_AL, // 0x16B9  # RUNIC LETTER WUNJO WYNN W
    LineBreakClass::ULB_AL, // 0x16BA  # RUNIC LETTER HAGLAZ H
    LineBreakClass::ULB_AL, // 0x16BB  # RUNIC LETTER HAEGL H
    LineBreakClass::ULB_AL, // 0x16BC  # RUNIC LETTER LONG-BRANCH-HAGALL H
    LineBreakClass::ULB_AL, // 0x16BD  # RUNIC LETTER SHORT-TWIG-HAGALL H
    LineBreakClass::ULB_AL, // 0x16BE  # RUNIC LETTER NAUDIZ NYD NAUD N
    LineBreakClass::ULB_AL, // 0x16BF  # RUNIC LETTER SHORT-TWIG-NAUD N
    LineBreakClass::ULB_AL, // 0x16C0  # RUNIC LETTER DOTTED-N
    LineBreakClass::ULB_AL, // 0x16C1  # RUNIC LETTER ISAZ IS ISS I
    LineBreakClass::ULB_AL, // 0x16C2  # RUNIC LETTER E
    LineBreakClass::ULB_AL, // 0x16C3  # RUNIC LETTER JERAN J
    LineBreakClass::ULB_AL, // 0x16C4  # RUNIC LETTER GER
    LineBreakClass::ULB_AL, // 0x16C5  # RUNIC LETTER LONG-BRANCH-AR AE
    LineBreakClass::ULB_AL, // 0x16C6  # RUNIC LETTER SHORT-TWIG-AR A
    LineBreakClass::ULB_AL, // 0x16C7  # RUNIC LETTER IWAZ EOH
    LineBreakClass::ULB_AL, // 0x16C8  # RUNIC LETTER PERTHO PEORTH P
    LineBreakClass::ULB_AL, // 0x16C9  # RUNIC LETTER ALGIZ EOLHX
    LineBreakClass::ULB_AL, // 0x16CA  # RUNIC LETTER SOWILO S
    LineBreakClass::ULB_AL, // 0x16CB  # RUNIC LETTER SIGEL LONG-BRANCH-SOL S
    LineBreakClass::ULB_AL, // 0x16CC  # RUNIC LETTER SHORT-TWIG-SOL S
    LineBreakClass::ULB_AL, // 0x16CD  # RUNIC LETTER C
    LineBreakClass::ULB_AL, // 0x16CE  # RUNIC LETTER Z
    LineBreakClass::ULB_AL, // 0x16CF  # RUNIC LETTER TIWAZ TIR TYR T
    LineBreakClass::ULB_AL, // 0x16D0  # RUNIC LETTER SHORT-TWIG-TYR T
    LineBreakClass::ULB_AL, // 0x16D1  # RUNIC LETTER D
    LineBreakClass::ULB_AL, // 0x16D2  # RUNIC LETTER BERKANAN BEORC BJARKAN B
    LineBreakClass::ULB_AL, // 0x16D3  # RUNIC LETTER SHORT-TWIG-BJARKAN B
    LineBreakClass::ULB_AL, // 0x16D4  # RUNIC LETTER DOTTED-P
    LineBreakClass::ULB_AL, // 0x16D5  # RUNIC LETTER OPEN-P
    LineBreakClass::ULB_AL, // 0x16D6  # RUNIC LETTER EHWAZ EH E
    LineBreakClass::ULB_AL, // 0x16D7  # RUNIC LETTER MANNAZ MAN M
    LineBreakClass::ULB_AL, // 0x16D8  # RUNIC LETTER LONG-BRANCH-MADR M
    LineBreakClass::ULB_AL, // 0x16D9  # RUNIC LETTER SHORT-TWIG-MADR M
    LineBreakClass::ULB_AL, // 0x16DA  # RUNIC LETTER LAUKAZ LAGU LOGR L
    LineBreakClass::ULB_AL, // 0x16DB  # RUNIC LETTER DOTTED-L
    LineBreakClass::ULB_AL, // 0x16DC  # RUNIC LETTER INGWAZ
    LineBreakClass::ULB_AL, // 0x16DD  # RUNIC LETTER ING
    LineBreakClass::ULB_AL, // 0x16DE  # RUNIC LETTER DAGAZ DAEG D
    LineBreakClass::ULB_AL, // 0x16DF  # RUNIC LETTER OTHALAN ETHEL O
    LineBreakClass::ULB_AL, // 0x16E0  # RUNIC LETTER EAR
    LineBreakClass::ULB_AL, // 0x16E1  # RUNIC LETTER IOR
    LineBreakClass::ULB_AL, // 0x16E2  # RUNIC LETTER CWEORTH
    LineBreakClass::ULB_AL, // 0x16E3  # RUNIC LETTER CALC
    LineBreakClass::ULB_AL, // 0x16E4  # RUNIC LETTER CEALC
    LineBreakClass::ULB_AL, // 0x16E5  # RUNIC LETTER STAN
    LineBreakClass::ULB_AL, // 0x16E6  # RUNIC LETTER LONG-BRANCH-YR
    LineBreakClass::ULB_AL, // 0x16E7  # RUNIC LETTER SHORT-TWIG-YR
    LineBreakClass::ULB_AL, // 0x16E8  # RUNIC LETTER ICELANDIC-YR
    LineBreakClass::ULB_AL, // 0x16E9  # RUNIC LETTER Q
    LineBreakClass::ULB_AL, // 0x16EA  # RUNIC LETTER X
    LineBreakClass::ULB_BA, // 0x16EB  # RUNIC SINGLE PUNCTUATION
    LineBreakClass::ULB_BA, // 0x16EC  # RUNIC MULTIPLE PUNCTUATION
    LineBreakClass::ULB_BA, // 0x16ED  # RUNIC CROSS PUNCTUATION
    LineBreakClass::ULB_AL, // 0x16EE  # RUNIC ARLAUG SYMBOL
    LineBreakClass::ULB_AL, // 0x16EF  # RUNIC TVIMADUR SYMBOL
    LineBreakClass::ULB_AL, // 0x16F0  # RUNIC BELGTHOR SYMBOL
    LineBreakClass::ULB_ID, // 0x16F1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x16F2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x16F3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x16F4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x16F5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x16F6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x16F7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x16F8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x16F9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x16FA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x16FB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x16FC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x16FD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x16FE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x16FF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1700  # TAGALOG LETTER A
    LineBreakClass::ULB_AL, // 0x1701  # TAGALOG LETTER I
    LineBreakClass::ULB_AL, // 0x1702  # TAGALOG LETTER U
    LineBreakClass::ULB_AL, // 0x1703  # TAGALOG LETTER KA
    LineBreakClass::ULB_AL, // 0x1704  # TAGALOG LETTER GA
    LineBreakClass::ULB_AL, // 0x1705  # TAGALOG LETTER NGA
    LineBreakClass::ULB_AL, // 0x1706  # TAGALOG LETTER TA
    LineBreakClass::ULB_AL, // 0x1707  # TAGALOG LETTER DA
    LineBreakClass::ULB_AL, // 0x1708  # TAGALOG LETTER NA
    LineBreakClass::ULB_AL, // 0x1709  # TAGALOG LETTER PA
    LineBreakClass::ULB_AL, // 0x170A  # TAGALOG LETTER BA
    LineBreakClass::ULB_AL, // 0x170B  # TAGALOG LETTER MA
    LineBreakClass::ULB_AL, // 0x170C  # TAGALOG LETTER YA
    LineBreakClass::ULB_ID, // 0x170D # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x170E  # TAGALOG LETTER LA
    LineBreakClass::ULB_AL, // 0x170F  # TAGALOG LETTER WA
    LineBreakClass::ULB_AL, // 0x1710  # TAGALOG LETTER SA
    LineBreakClass::ULB_AL, // 0x1711  # TAGALOG LETTER HA
    LineBreakClass::ULB_CM, // 0x1712  # TAGALOG VOWEL SIGN I
    LineBreakClass::ULB_CM, // 0x1713  # TAGALOG VOWEL SIGN U
    LineBreakClass::ULB_CM, // 0x1714  # TAGALOG SIGN VIRAMA
    LineBreakClass::ULB_ID, // 0x1715 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1716 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1717 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1718 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1719 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x171A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x171B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x171C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x171D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x171E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x171F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1720  # HANUNOO LETTER A
    LineBreakClass::ULB_AL, // 0x1721  # HANUNOO LETTER I
    LineBreakClass::ULB_AL, // 0x1722  # HANUNOO LETTER U
    LineBreakClass::ULB_AL, // 0x1723  # HANUNOO LETTER KA
    LineBreakClass::ULB_AL, // 0x1724  # HANUNOO LETTER GA
    LineBreakClass::ULB_AL, // 0x1725  # HANUNOO LETTER NGA
    LineBreakClass::ULB_AL, // 0x1726  # HANUNOO LETTER TA
    LineBreakClass::ULB_AL, // 0x1727  # HANUNOO LETTER DA
    LineBreakClass::ULB_AL, // 0x1728  # HANUNOO LETTER NA
    LineBreakClass::ULB_AL, // 0x1729  # HANUNOO LETTER PA
    LineBreakClass::ULB_AL, // 0x172A  # HANUNOO LETTER BA
    LineBreakClass::ULB_AL, // 0x172B  # HANUNOO LETTER MA
    LineBreakClass::ULB_AL, // 0x172C  # HANUNOO LETTER YA
    LineBreakClass::ULB_AL, // 0x172D  # HANUNOO LETTER RA
    LineBreakClass::ULB_AL, // 0x172E  # HANUNOO LETTER LA
    LineBreakClass::ULB_AL, // 0x172F  # HANUNOO LETTER WA
    LineBreakClass::ULB_AL, // 0x1730  # HANUNOO LETTER SA
    LineBreakClass::ULB_AL, // 0x1731  # HANUNOO LETTER HA
    LineBreakClass::ULB_CM, // 0x1732  # HANUNOO VOWEL SIGN I
    LineBreakClass::ULB_CM, // 0x1733  # HANUNOO VOWEL SIGN U
    LineBreakClass::ULB_CM, // 0x1734  # HANUNOO SIGN PAMUDPOD
    LineBreakClass::ULB_BA, // 0x1735  # PHILIPPINE SINGLE PUNCTUATION
    LineBreakClass::ULB_BA, // 0x1736  # PHILIPPINE DOUBLE PUNCTUATION
    LineBreakClass::ULB_ID, // 0x1737 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1738 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1739 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x173A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x173B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x173C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x173D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x173E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x173F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1740  # BUHID LETTER A
    LineBreakClass::ULB_AL, // 0x1741  # BUHID LETTER I
    LineBreakClass::ULB_AL, // 0x1742  # BUHID LETTER U
    LineBreakClass::ULB_AL, // 0x1743  # BUHID LETTER KA
    LineBreakClass::ULB_AL, // 0x1744  # BUHID LETTER GA
    LineBreakClass::ULB_AL, // 0x1745  # BUHID LETTER NGA
    LineBreakClass::ULB_AL, // 0x1746  # BUHID LETTER TA
    LineBreakClass::ULB_AL, // 0x1747  # BUHID LETTER DA
    LineBreakClass::ULB_AL, // 0x1748  # BUHID LETTER NA
    LineBreakClass::ULB_AL, // 0x1749  # BUHID LETTER PA
    LineBreakClass::ULB_AL, // 0x174A  # BUHID LETTER BA
    LineBreakClass::ULB_AL, // 0x174B  # BUHID LETTER MA
    LineBreakClass::ULB_AL, // 0x174C  # BUHID LETTER YA
    LineBreakClass::ULB_AL, // 0x174D  # BUHID LETTER RA
    LineBreakClass::ULB_AL, // 0x174E  # BUHID LETTER LA
    LineBreakClass::ULB_AL, // 0x174F  # BUHID LETTER WA
    LineBreakClass::ULB_AL, // 0x1750  # BUHID LETTER SA
    LineBreakClass::ULB_AL, // 0x1751  # BUHID LETTER HA
    LineBreakClass::ULB_CM, // 0x1752  # BUHID VOWEL SIGN I
    LineBreakClass::ULB_CM, // 0x1753  # BUHID VOWEL SIGN U
    LineBreakClass::ULB_ID, // 0x1754 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1755 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1756 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1757 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1758 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1759 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x175A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x175B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x175C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x175D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x175E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x175F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1760  # TAGBANWA LETTER A
    LineBreakClass::ULB_AL, // 0x1761  # TAGBANWA LETTER I
    LineBreakClass::ULB_AL, // 0x1762  # TAGBANWA LETTER U
    LineBreakClass::ULB_AL, // 0x1763  # TAGBANWA LETTER KA
    LineBreakClass::ULB_AL, // 0x1764  # TAGBANWA LETTER GA
    LineBreakClass::ULB_AL, // 0x1765  # TAGBANWA LETTER NGA
    LineBreakClass::ULB_AL, // 0x1766  # TAGBANWA LETTER TA
    LineBreakClass::ULB_AL, // 0x1767  # TAGBANWA LETTER DA
    LineBreakClass::ULB_AL, // 0x1768  # TAGBANWA LETTER NA
    LineBreakClass::ULB_AL, // 0x1769  # TAGBANWA LETTER PA
    LineBreakClass::ULB_AL, // 0x176A  # TAGBANWA LETTER BA
    LineBreakClass::ULB_AL, // 0x176B  # TAGBANWA LETTER MA
    LineBreakClass::ULB_AL, // 0x176C  # TAGBANWA LETTER YA
    LineBreakClass::ULB_ID, // 0x176D # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x176E  # TAGBANWA LETTER LA
    LineBreakClass::ULB_AL, // 0x176F  # TAGBANWA LETTER WA
    LineBreakClass::ULB_AL, // 0x1770  # TAGBANWA LETTER SA
    LineBreakClass::ULB_ID, // 0x1771 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x1772  # TAGBANWA VOWEL SIGN I
    LineBreakClass::ULB_CM, // 0x1773  # TAGBANWA VOWEL SIGN U
    LineBreakClass::ULB_ID, // 0x1774 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1775 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1776 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1777 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1778 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1779 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x177A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x177B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x177C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x177D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x177E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x177F # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x1780  # KHMER LETTER KA
    LineBreakClass::ULB_SA, // 0x1781  # KHMER LETTER KHA
    LineBreakClass::ULB_SA, // 0x1782  # KHMER LETTER KO
    LineBreakClass::ULB_SA, // 0x1783  # KHMER LETTER KHO
    LineBreakClass::ULB_SA, // 0x1784  # KHMER LETTER NGO
    LineBreakClass::ULB_SA, // 0x1785  # KHMER LETTER CA
    LineBreakClass::ULB_SA, // 0x1786  # KHMER LETTER CHA
    LineBreakClass::ULB_SA, // 0x1787  # KHMER LETTER CO
    LineBreakClass::ULB_SA, // 0x1788  # KHMER LETTER CHO
    LineBreakClass::ULB_SA, // 0x1789  # KHMER LETTER NYO
    LineBreakClass::ULB_SA, // 0x178A  # KHMER LETTER DA
    LineBreakClass::ULB_SA, // 0x178B  # KHMER LETTER TTHA
    LineBreakClass::ULB_SA, // 0x178C  # KHMER LETTER DO
    LineBreakClass::ULB_SA, // 0x178D  # KHMER LETTER TTHO
    LineBreakClass::ULB_SA, // 0x178E  # KHMER LETTER NNO
    LineBreakClass::ULB_SA, // 0x178F  # KHMER LETTER TA
    LineBreakClass::ULB_SA, // 0x1790  # KHMER LETTER THA
    LineBreakClass::ULB_SA, // 0x1791  # KHMER LETTER TO
    LineBreakClass::ULB_SA, // 0x1792  # KHMER LETTER THO
    LineBreakClass::ULB_SA, // 0x1793  # KHMER LETTER NO
    LineBreakClass::ULB_SA, // 0x1794  # KHMER LETTER BA
    LineBreakClass::ULB_SA, // 0x1795  # KHMER LETTER PHA
    LineBreakClass::ULB_SA, // 0x1796  # KHMER LETTER PO
    LineBreakClass::ULB_SA, // 0x1797  # KHMER LETTER PHO
    LineBreakClass::ULB_SA, // 0x1798  # KHMER LETTER MO
    LineBreakClass::ULB_SA, // 0x1799  # KHMER LETTER YO
    LineBreakClass::ULB_SA, // 0x179A  # KHMER LETTER RO
    LineBreakClass::ULB_SA, // 0x179B  # KHMER LETTER LO
    LineBreakClass::ULB_SA, // 0x179C  # KHMER LETTER VO
    LineBreakClass::ULB_SA, // 0x179D  # KHMER LETTER SHA
    LineBreakClass::ULB_SA, // 0x179E  # KHMER LETTER SSO
    LineBreakClass::ULB_SA, // 0x179F  # KHMER LETTER SA
    LineBreakClass::ULB_SA, // 0x17A0  # KHMER LETTER HA
    LineBreakClass::ULB_SA, // 0x17A1  # KHMER LETTER LA
    LineBreakClass::ULB_SA, // 0x17A2  # KHMER LETTER QA
    LineBreakClass::ULB_SA, // 0x17A3  # KHMER INDEPENDENT VOWEL QAQ
    LineBreakClass::ULB_SA, // 0x17A4  # KHMER INDEPENDENT VOWEL QAA
    LineBreakClass::ULB_SA, // 0x17A5  # KHMER INDEPENDENT VOWEL QI
    LineBreakClass::ULB_SA, // 0x17A6  # KHMER INDEPENDENT VOWEL QII
    LineBreakClass::ULB_SA, // 0x17A7  # KHMER INDEPENDENT VOWEL QU
    LineBreakClass::ULB_SA, // 0x17A8  # KHMER INDEPENDENT VOWEL QUK
    LineBreakClass::ULB_SA, // 0x17A9  # KHMER INDEPENDENT VOWEL QUU
    LineBreakClass::ULB_SA, // 0x17AA  # KHMER INDEPENDENT VOWEL QUUV
    LineBreakClass::ULB_SA, // 0x17AB  # KHMER INDEPENDENT VOWEL RY
    LineBreakClass::ULB_SA, // 0x17AC  # KHMER INDEPENDENT VOWEL RYY
    LineBreakClass::ULB_SA, // 0x17AD  # KHMER INDEPENDENT VOWEL LY
    LineBreakClass::ULB_SA, // 0x17AE  # KHMER INDEPENDENT VOWEL LYY
    LineBreakClass::ULB_SA, // 0x17AF  # KHMER INDEPENDENT VOWEL QE
    LineBreakClass::ULB_SA, // 0x17B0  # KHMER INDEPENDENT VOWEL QAI
    LineBreakClass::ULB_SA, // 0x17B1  # KHMER INDEPENDENT VOWEL QOO TYPE ONE
    LineBreakClass::ULB_SA, // 0x17B2  # KHMER INDEPENDENT VOWEL QOO TYPE TWO
    LineBreakClass::ULB_SA, // 0x17B3  # KHMER INDEPENDENT VOWEL QAU
    LineBreakClass::ULB_SA, // 0x17B4  # KHMER VOWEL INHERENT AQ
    LineBreakClass::ULB_SA, // 0x17B5  # KHMER VOWEL INHERENT AA
    LineBreakClass::ULB_SA, // 0x17B6  # KHMER VOWEL SIGN AA
    LineBreakClass::ULB_SA, // 0x17B7  # KHMER VOWEL SIGN I
    LineBreakClass::ULB_SA, // 0x17B8  # KHMER VOWEL SIGN II
    LineBreakClass::ULB_SA, // 0x17B9  # KHMER VOWEL SIGN Y
    LineBreakClass::ULB_SA, // 0x17BA  # KHMER VOWEL SIGN YY
    LineBreakClass::ULB_SA, // 0x17BB  # KHMER VOWEL SIGN U
    LineBreakClass::ULB_SA, // 0x17BC  # KHMER VOWEL SIGN UU
    LineBreakClass::ULB_SA, // 0x17BD  # KHMER VOWEL SIGN UA
    LineBreakClass::ULB_SA, // 0x17BE  # KHMER VOWEL SIGN OE
    LineBreakClass::ULB_SA, // 0x17BF  # KHMER VOWEL SIGN YA
    LineBreakClass::ULB_SA, // 0x17C0  # KHMER VOWEL SIGN IE
    LineBreakClass::ULB_SA, // 0x17C1  # KHMER VOWEL SIGN E
    LineBreakClass::ULB_SA, // 0x17C2  # KHMER VOWEL SIGN AE
    LineBreakClass::ULB_SA, // 0x17C3  # KHMER VOWEL SIGN AI
    LineBreakClass::ULB_SA, // 0x17C4  # KHMER VOWEL SIGN OO
    LineBreakClass::ULB_SA, // 0x17C5  # KHMER VOWEL SIGN AU
    LineBreakClass::ULB_SA, // 0x17C6  # KHMER SIGN NIKAHIT
    LineBreakClass::ULB_SA, // 0x17C7  # KHMER SIGN REAHMUK
    LineBreakClass::ULB_SA, // 0x17C8  # KHMER SIGN YUUKALEAPINTU
    LineBreakClass::ULB_SA, // 0x17C9  # KHMER SIGN MUUSIKATOAN
    LineBreakClass::ULB_SA, // 0x17CA  # KHMER SIGN TRIISAP
    LineBreakClass::ULB_SA, // 0x17CB  # KHMER SIGN BANTOC
    LineBreakClass::ULB_SA, // 0x17CC  # KHMER SIGN ROBAT
    LineBreakClass::ULB_SA, // 0x17CD  # KHMER SIGN TOANDAKHIAT
    LineBreakClass::ULB_SA, // 0x17CE  # KHMER SIGN KAKABAT
    LineBreakClass::ULB_SA, // 0x17CF  # KHMER SIGN AHSDA
    LineBreakClass::ULB_SA, // 0x17D0  # KHMER SIGN SAMYOK SANNYA
    LineBreakClass::ULB_SA, // 0x17D1  # KHMER SIGN VIRIAM
    LineBreakClass::ULB_SA, // 0x17D2  # KHMER SIGN COENG
    LineBreakClass::ULB_SA, // 0x17D3  # KHMER SIGN BATHAMASAT
    LineBreakClass::ULB_BA, // 0x17D4  # KHMER SIGN KHAN
    LineBreakClass::ULB_BA, // 0x17D5  # KHMER SIGN BARIYOOSAN
    LineBreakClass::ULB_NS, // 0x17D6  # KHMER SIGN CAMNUC PII KUUH
    LineBreakClass::ULB_SA, // 0x17D7  # KHMER SIGN LEK TOO
    LineBreakClass::ULB_BA, // 0x17D8  # KHMER SIGN BEYYAL
    LineBreakClass::ULB_AL, // 0x17D9  # KHMER SIGN PHNAEK MUAN
    LineBreakClass::ULB_BA, // 0x17DA  # KHMER SIGN KOOMUUT
    LineBreakClass::ULB_PR, // 0x17DB  # KHMER CURRENCY SYMBOL RIEL
    LineBreakClass::ULB_SA, // 0x17DC  # KHMER SIGN AVAKRAHASANYA
    LineBreakClass::ULB_SA, // 0x17DD  # KHMER SIGN ATTHACAN
    LineBreakClass::ULB_ID, // 0x17DE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x17DF # <UNDEFINED>
    LineBreakClass::ULB_NU, // 0x17E0  # KHMER DIGIT ZERO
    LineBreakClass::ULB_NU, // 0x17E1  # KHMER DIGIT ONE
    LineBreakClass::ULB_NU, // 0x17E2  # KHMER DIGIT TWO
    LineBreakClass::ULB_NU, // 0x17E3  # KHMER DIGIT THREE
    LineBreakClass::ULB_NU, // 0x17E4  # KHMER DIGIT FOUR
    LineBreakClass::ULB_NU, // 0x17E5  # KHMER DIGIT FIVE
    LineBreakClass::ULB_NU, // 0x17E6  # KHMER DIGIT SIX
    LineBreakClass::ULB_NU, // 0x17E7  # KHMER DIGIT SEVEN
    LineBreakClass::ULB_NU, // 0x17E8  # KHMER DIGIT EIGHT
    LineBreakClass::ULB_NU, // 0x17E9  # KHMER DIGIT NINE
    LineBreakClass::ULB_ID, // 0x17EA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x17EB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x17EC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x17ED # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x17EE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x17EF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x17F0  # KHMER SYMBOL LEK ATTAK SON
    LineBreakClass::ULB_AL, // 0x17F1  # KHMER SYMBOL LEK ATTAK MUOY
    LineBreakClass::ULB_AL, // 0x17F2  # KHMER SYMBOL LEK ATTAK PII
    LineBreakClass::ULB_AL, // 0x17F3  # KHMER SYMBOL LEK ATTAK BEI
    LineBreakClass::ULB_AL, // 0x17F4  # KHMER SYMBOL LEK ATTAK BUON
    LineBreakClass::ULB_AL, // 0x17F5  # KHMER SYMBOL LEK ATTAK PRAM
    LineBreakClass::ULB_AL, // 0x17F6  # KHMER SYMBOL LEK ATTAK PRAM-MUOY
    LineBreakClass::ULB_AL, // 0x17F7  # KHMER SYMBOL LEK ATTAK PRAM-PII
    LineBreakClass::ULB_AL, // 0x17F8  # KHMER SYMBOL LEK ATTAK PRAM-BEI
    LineBreakClass::ULB_AL, // 0x17F9  # KHMER SYMBOL LEK ATTAK PRAM-BUON
    LineBreakClass::ULB_ID, // 0x17FA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x17FB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x17FC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x17FD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x17FE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x17FF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1800  # MONGOLIAN BIRGA
    LineBreakClass::ULB_AL, // 0x1801  # MONGOLIAN ELLIPSIS
    LineBreakClass::ULB_BA, // 0x1802  # MONGOLIAN COMMA
    LineBreakClass::ULB_BA, // 0x1803  # MONGOLIAN FULL STOP
    LineBreakClass::ULB_BA, // 0x1804  # MONGOLIAN COLON
    LineBreakClass::ULB_BA, // 0x1805  # MONGOLIAN FOUR DOTS
    LineBreakClass::ULB_BB, // 0x1806  # MONGOLIAN TODO SOFT HYPHEN
    LineBreakClass::ULB_AL, // 0x1807  # MONGOLIAN SIBE SYLLABLE BOUNDARY MARKER
    LineBreakClass::ULB_BA, // 0x1808  # MONGOLIAN MANCHU COMMA
    LineBreakClass::ULB_BA, // 0x1809  # MONGOLIAN MANCHU FULL STOP
    LineBreakClass::ULB_AL, // 0x180A  # MONGOLIAN NIRUGU
    LineBreakClass::ULB_CM, // 0x180B  # MONGOLIAN FREE VARIATION SELECTOR ONE
    LineBreakClass::ULB_CM, // 0x180C  # MONGOLIAN FREE VARIATION SELECTOR TWO
    LineBreakClass::ULB_CM, // 0x180D  # MONGOLIAN FREE VARIATION SELECTOR THREE
    LineBreakClass::ULB_GL, // 0x180E  # MONGOLIAN VOWEL SEPARATOR
    LineBreakClass::ULB_ID, // 0x180F # <UNDEFINED>
    LineBreakClass::ULB_NU, // 0x1810  # MONGOLIAN DIGIT ZERO
    LineBreakClass::ULB_NU, // 0x1811  # MONGOLIAN DIGIT ONE
    LineBreakClass::ULB_NU, // 0x1812  # MONGOLIAN DIGIT TWO
    LineBreakClass::ULB_NU, // 0x1813  # MONGOLIAN DIGIT THREE
    LineBreakClass::ULB_NU, // 0x1814  # MONGOLIAN DIGIT FOUR
    LineBreakClass::ULB_NU, // 0x1815  # MONGOLIAN DIGIT FIVE
    LineBreakClass::ULB_NU, // 0x1816  # MONGOLIAN DIGIT SIX
    LineBreakClass::ULB_NU, // 0x1817  # MONGOLIAN DIGIT SEVEN
    LineBreakClass::ULB_NU, // 0x1818  # MONGOLIAN DIGIT EIGHT
    LineBreakClass::ULB_NU, // 0x1819  # MONGOLIAN DIGIT NINE
    LineBreakClass::ULB_ID, // 0x181A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x181B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x181C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x181D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x181E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x181F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1820  # MONGOLIAN LETTER A
    LineBreakClass::ULB_AL, // 0x1821  # MONGOLIAN LETTER E
    LineBreakClass::ULB_AL, // 0x1822  # MONGOLIAN LETTER I
    LineBreakClass::ULB_AL, // 0x1823  # MONGOLIAN LETTER O
    LineBreakClass::ULB_AL, // 0x1824  # MONGOLIAN LETTER U
    LineBreakClass::ULB_AL, // 0x1825  # MONGOLIAN LETTER OE
    LineBreakClass::ULB_AL, // 0x1826  # MONGOLIAN LETTER UE
    LineBreakClass::ULB_AL, // 0x1827  # MONGOLIAN LETTER EE
    LineBreakClass::ULB_AL, // 0x1828  # MONGOLIAN LETTER NA
    LineBreakClass::ULB_AL, // 0x1829  # MONGOLIAN LETTER ANG
    LineBreakClass::ULB_AL, // 0x182A  # MONGOLIAN LETTER BA
    LineBreakClass::ULB_AL, // 0x182B  # MONGOLIAN LETTER PA
    LineBreakClass::ULB_AL, // 0x182C  # MONGOLIAN LETTER QA
    LineBreakClass::ULB_AL, // 0x182D  # MONGOLIAN LETTER GA
    LineBreakClass::ULB_AL, // 0x182E  # MONGOLIAN LETTER MA
    LineBreakClass::ULB_AL, // 0x182F  # MONGOLIAN LETTER LA
    LineBreakClass::ULB_AL, // 0x1830  # MONGOLIAN LETTER SA
    LineBreakClass::ULB_AL, // 0x1831  # MONGOLIAN LETTER SHA
    LineBreakClass::ULB_AL, // 0x1832  # MONGOLIAN LETTER TA
    LineBreakClass::ULB_AL, // 0x1833  # MONGOLIAN LETTER DA
    LineBreakClass::ULB_AL, // 0x1834  # MONGOLIAN LETTER CHA
    LineBreakClass::ULB_AL, // 0x1835  # MONGOLIAN LETTER JA
    LineBreakClass::ULB_AL, // 0x1836  # MONGOLIAN LETTER YA
    LineBreakClass::ULB_AL, // 0x1837  # MONGOLIAN LETTER RA
    LineBreakClass::ULB_AL, // 0x1838  # MONGOLIAN LETTER WA
    LineBreakClass::ULB_AL, // 0x1839  # MONGOLIAN LETTER FA
    LineBreakClass::ULB_AL, // 0x183A  # MONGOLIAN LETTER KA
    LineBreakClass::ULB_AL, // 0x183B  # MONGOLIAN LETTER KHA
    LineBreakClass::ULB_AL, // 0x183C  # MONGOLIAN LETTER TSA
    LineBreakClass::ULB_AL, // 0x183D  # MONGOLIAN LETTER ZA
    LineBreakClass::ULB_AL, // 0x183E  # MONGOLIAN LETTER HAA
    LineBreakClass::ULB_AL, // 0x183F  # MONGOLIAN LETTER ZRA
    LineBreakClass::ULB_AL, // 0x1840  # MONGOLIAN LETTER LHA
    LineBreakClass::ULB_AL, // 0x1841  # MONGOLIAN LETTER ZHI
    LineBreakClass::ULB_AL, // 0x1842  # MONGOLIAN LETTER CHI
    LineBreakClass::ULB_AL, // 0x1843  # MONGOLIAN LETTER TODO LONG VOWEL SIGN
    LineBreakClass::ULB_AL, // 0x1844  # MONGOLIAN LETTER TODO E
    LineBreakClass::ULB_AL, // 0x1845  # MONGOLIAN LETTER TODO I
    LineBreakClass::ULB_AL, // 0x1846  # MONGOLIAN LETTER TODO O
    LineBreakClass::ULB_AL, // 0x1847  # MONGOLIAN LETTER TODO U
    LineBreakClass::ULB_AL, // 0x1848  # MONGOLIAN LETTER TODO OE
    LineBreakClass::ULB_AL, // 0x1849  # MONGOLIAN LETTER TODO UE
    LineBreakClass::ULB_AL, // 0x184A  # MONGOLIAN LETTER TODO ANG
    LineBreakClass::ULB_AL, // 0x184B  # MONGOLIAN LETTER TODO BA
    LineBreakClass::ULB_AL, // 0x184C  # MONGOLIAN LETTER TODO PA
    LineBreakClass::ULB_AL, // 0x184D  # MONGOLIAN LETTER TODO QA
    LineBreakClass::ULB_AL, // 0x184E  # MONGOLIAN LETTER TODO GA
    LineBreakClass::ULB_AL, // 0x184F  # MONGOLIAN LETTER TODO MA
    LineBreakClass::ULB_AL, // 0x1850  # MONGOLIAN LETTER TODO TA
    LineBreakClass::ULB_AL, // 0x1851  # MONGOLIAN LETTER TODO DA
    LineBreakClass::ULB_AL, // 0x1852  # MONGOLIAN LETTER TODO CHA
    LineBreakClass::ULB_AL, // 0x1853  # MONGOLIAN LETTER TODO JA
    LineBreakClass::ULB_AL, // 0x1854  # MONGOLIAN LETTER TODO TSA
    LineBreakClass::ULB_AL, // 0x1855  # MONGOLIAN LETTER TODO YA
    LineBreakClass::ULB_AL, // 0x1856  # MONGOLIAN LETTER TODO WA
    LineBreakClass::ULB_AL, // 0x1857  # MONGOLIAN LETTER TODO KA
    LineBreakClass::ULB_AL, // 0x1858  # MONGOLIAN LETTER TODO GAA
    LineBreakClass::ULB_AL, // 0x1859  # MONGOLIAN LETTER TODO HAA
    LineBreakClass::ULB_AL, // 0x185A  # MONGOLIAN LETTER TODO JIA
    LineBreakClass::ULB_AL, // 0x185B  # MONGOLIAN LETTER TODO NIA
    LineBreakClass::ULB_AL, // 0x185C  # MONGOLIAN LETTER TODO DZA
    LineBreakClass::ULB_AL, // 0x185D  # MONGOLIAN LETTER SIBE E
    LineBreakClass::ULB_AL, // 0x185E  # MONGOLIAN LETTER SIBE I
    LineBreakClass::ULB_AL, // 0x185F  # MONGOLIAN LETTER SIBE IY
    LineBreakClass::ULB_AL, // 0x1860  # MONGOLIAN LETTER SIBE UE
    LineBreakClass::ULB_AL, // 0x1861  # MONGOLIAN LETTER SIBE U
    LineBreakClass::ULB_AL, // 0x1862  # MONGOLIAN LETTER SIBE ANG
    LineBreakClass::ULB_AL, // 0x1863  # MONGOLIAN LETTER SIBE KA
    LineBreakClass::ULB_AL, // 0x1864  # MONGOLIAN LETTER SIBE GA
    LineBreakClass::ULB_AL, // 0x1865  # MONGOLIAN LETTER SIBE HA
    LineBreakClass::ULB_AL, // 0x1866  # MONGOLIAN LETTER SIBE PA
    LineBreakClass::ULB_AL, // 0x1867  # MONGOLIAN LETTER SIBE SHA
    LineBreakClass::ULB_AL, // 0x1868  # MONGOLIAN LETTER SIBE TA
    LineBreakClass::ULB_AL, // 0x1869  # MONGOLIAN LETTER SIBE DA
    LineBreakClass::ULB_AL, // 0x186A  # MONGOLIAN LETTER SIBE JA
    LineBreakClass::ULB_AL, // 0x186B  # MONGOLIAN LETTER SIBE FA
    LineBreakClass::ULB_AL, // 0x186C  # MONGOLIAN LETTER SIBE GAA
    LineBreakClass::ULB_AL, // 0x186D  # MONGOLIAN LETTER SIBE HAA
    LineBreakClass::ULB_AL, // 0x186E  # MONGOLIAN LETTER SIBE TSA
    LineBreakClass::ULB_AL, // 0x186F  # MONGOLIAN LETTER SIBE ZA
    LineBreakClass::ULB_AL, // 0x1870  # MONGOLIAN LETTER SIBE RAA
    LineBreakClass::ULB_AL, // 0x1871  # MONGOLIAN LETTER SIBE CHA
    LineBreakClass::ULB_AL, // 0x1872  # MONGOLIAN LETTER SIBE ZHA
    LineBreakClass::ULB_AL, // 0x1873  # MONGOLIAN LETTER MANCHU I
    LineBreakClass::ULB_AL, // 0x1874  # MONGOLIAN LETTER MANCHU KA
    LineBreakClass::ULB_AL, // 0x1875  # MONGOLIAN LETTER MANCHU RA
    LineBreakClass::ULB_AL, // 0x1876  # MONGOLIAN LETTER MANCHU FA
    LineBreakClass::ULB_AL, // 0x1877  # MONGOLIAN LETTER MANCHU ZHA
    LineBreakClass::ULB_ID, // 0x1878 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1879 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x187A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x187B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x187C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x187D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x187E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x187F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1880  # MONGOLIAN LETTER ALI GALI ANUSVARA ONE
    LineBreakClass::ULB_AL, // 0x1881  # MONGOLIAN LETTER ALI GALI VISARGA ONE
    LineBreakClass::ULB_AL, // 0x1882  # MONGOLIAN LETTER ALI GALI DAMARU
    LineBreakClass::ULB_AL, // 0x1883  # MONGOLIAN LETTER ALI GALI UBADAMA
    LineBreakClass::ULB_AL, // 0x1884  # MONGOLIAN LETTER ALI GALI INVERTED UBADAMA
    LineBreakClass::ULB_AL, // 0x1885  # MONGOLIAN LETTER ALI GALI BALUDA
    LineBreakClass::ULB_AL, // 0x1886  # MONGOLIAN LETTER ALI GALI THREE BALUDA
    LineBreakClass::ULB_AL, // 0x1887  # MONGOLIAN LETTER ALI GALI A
    LineBreakClass::ULB_AL, // 0x1888  # MONGOLIAN LETTER ALI GALI I
    LineBreakClass::ULB_AL, // 0x1889  # MONGOLIAN LETTER ALI GALI KA
    LineBreakClass::ULB_AL, // 0x188A  # MONGOLIAN LETTER ALI GALI NGA
    LineBreakClass::ULB_AL, // 0x188B  # MONGOLIAN LETTER ALI GALI CA
    LineBreakClass::ULB_AL, // 0x188C  # MONGOLIAN LETTER ALI GALI TTA
    LineBreakClass::ULB_AL, // 0x188D  # MONGOLIAN LETTER ALI GALI TTHA
    LineBreakClass::ULB_AL, // 0x188E  # MONGOLIAN LETTER ALI GALI DDA
    LineBreakClass::ULB_AL, // 0x188F  # MONGOLIAN LETTER ALI GALI NNA
    LineBreakClass::ULB_AL, // 0x1890  # MONGOLIAN LETTER ALI GALI TA
    LineBreakClass::ULB_AL, // 0x1891  # MONGOLIAN LETTER ALI GALI DA
    LineBreakClass::ULB_AL, // 0x1892  # MONGOLIAN LETTER ALI GALI PA
    LineBreakClass::ULB_AL, // 0x1893  # MONGOLIAN LETTER ALI GALI PHA
    LineBreakClass::ULB_AL, // 0x1894  # MONGOLIAN LETTER ALI GALI SSA
    LineBreakClass::ULB_AL, // 0x1895  # MONGOLIAN LETTER ALI GALI ZHA
    LineBreakClass::ULB_AL, // 0x1896  # MONGOLIAN LETTER ALI GALI ZA
    LineBreakClass::ULB_AL, // 0x1897  # MONGOLIAN LETTER ALI GALI AH
    LineBreakClass::ULB_AL, // 0x1898  # MONGOLIAN LETTER TODO ALI GALI TA
    LineBreakClass::ULB_AL, // 0x1899  # MONGOLIAN LETTER TODO ALI GALI ZHA
    LineBreakClass::ULB_AL, // 0x189A  # MONGOLIAN LETTER MANCHU ALI GALI GHA
    LineBreakClass::ULB_AL, // 0x189B  # MONGOLIAN LETTER MANCHU ALI GALI NGA
    LineBreakClass::ULB_AL, // 0x189C  # MONGOLIAN LETTER MANCHU ALI GALI CA
    LineBreakClass::ULB_AL, // 0x189D  # MONGOLIAN LETTER MANCHU ALI GALI JHA
    LineBreakClass::ULB_AL, // 0x189E  # MONGOLIAN LETTER MANCHU ALI GALI TTA
    LineBreakClass::ULB_AL, // 0x189F  # MONGOLIAN LETTER MANCHU ALI GALI DDHA
    LineBreakClass::ULB_AL, // 0x18A0  # MONGOLIAN LETTER MANCHU ALI GALI TA
    LineBreakClass::ULB_AL, // 0x18A1  # MONGOLIAN LETTER MANCHU ALI GALI DHA
    LineBreakClass::ULB_AL, // 0x18A2  # MONGOLIAN LETTER MANCHU ALI GALI SSA
    LineBreakClass::ULB_AL, // 0x18A3  # MONGOLIAN LETTER MANCHU ALI GALI CYA
    LineBreakClass::ULB_AL, // 0x18A4  # MONGOLIAN LETTER MANCHU ALI GALI ZHA
    LineBreakClass::ULB_AL, // 0x18A5  # MONGOLIAN LETTER MANCHU ALI GALI ZA
    LineBreakClass::ULB_AL, // 0x18A6  # MONGOLIAN LETTER ALI GALI HALF U
    LineBreakClass::ULB_AL, // 0x18A7  # MONGOLIAN LETTER ALI GALI HALF YA
    LineBreakClass::ULB_AL, // 0x18A8  # MONGOLIAN LETTER MANCHU ALI GALI BHA
    LineBreakClass::ULB_CM, // 0x18A9  # MONGOLIAN LETTER ALI GALI DAGALGA
    LineBreakClass::ULB_ID, // 0x18AA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18AB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18AC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18AD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18AE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18AF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18B0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18B1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18B2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18B3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18B4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18B5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18B6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18B7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18B8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18B9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18BA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18BB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18BC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18BD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18BE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18BF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18C0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18C1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18C2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18C3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18C4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18C5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18C6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18C7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18C8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18C9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18CA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18CB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18CC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18CD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18CE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18CF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18D0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18D1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18D2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18D3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18D4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18D5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18D6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18D7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18D8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18D9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18DA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18DB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18DC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18DD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18DE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18DF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18E0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18E1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18E2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18E3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18E4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18E5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18E6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18E7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18E8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18E9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18EA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18EB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18EC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18ED # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18EE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18EF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18F0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18F1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18F2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18F3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18F4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18F5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18F6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18F7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18F8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18F9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18FA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18FB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18FC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18FD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18FE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x18FF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1900  # LIMBU VOWEL-CARRIER LETTER
    LineBreakClass::ULB_AL, // 0x1901  # LIMBU LETTER KA
    LineBreakClass::ULB_AL, // 0x1902  # LIMBU LETTER KHA
    LineBreakClass::ULB_AL, // 0x1903  # LIMBU LETTER GA
    LineBreakClass::ULB_AL, // 0x1904  # LIMBU LETTER GHA
    LineBreakClass::ULB_AL, // 0x1905  # LIMBU LETTER NGA
    LineBreakClass::ULB_AL, // 0x1906  # LIMBU LETTER CA
    LineBreakClass::ULB_AL, // 0x1907  # LIMBU LETTER CHA
    LineBreakClass::ULB_AL, // 0x1908  # LIMBU LETTER JA
    LineBreakClass::ULB_AL, // 0x1909  # LIMBU LETTER JHA
    LineBreakClass::ULB_AL, // 0x190A  # LIMBU LETTER YAN
    LineBreakClass::ULB_AL, // 0x190B  # LIMBU LETTER TA
    LineBreakClass::ULB_AL, // 0x190C  # LIMBU LETTER THA
    LineBreakClass::ULB_AL, // 0x190D  # LIMBU LETTER DA
    LineBreakClass::ULB_AL, // 0x190E  # LIMBU LETTER DHA
    LineBreakClass::ULB_AL, // 0x190F  # LIMBU LETTER NA
    LineBreakClass::ULB_AL, // 0x1910  # LIMBU LETTER PA
    LineBreakClass::ULB_AL, // 0x1911  # LIMBU LETTER PHA
    LineBreakClass::ULB_AL, // 0x1912  # LIMBU LETTER BA
    LineBreakClass::ULB_AL, // 0x1913  # LIMBU LETTER BHA
    LineBreakClass::ULB_AL, // 0x1914  # LIMBU LETTER MA
    LineBreakClass::ULB_AL, // 0x1915  # LIMBU LETTER YA
    LineBreakClass::ULB_AL, // 0x1916  # LIMBU LETTER RA
    LineBreakClass::ULB_AL, // 0x1917  # LIMBU LETTER LA
    LineBreakClass::ULB_AL, // 0x1918  # LIMBU LETTER WA
    LineBreakClass::ULB_AL, // 0x1919  # LIMBU LETTER SHA
    LineBreakClass::ULB_AL, // 0x191A  # LIMBU LETTER SSA
    LineBreakClass::ULB_AL, // 0x191B  # LIMBU LETTER SA
    LineBreakClass::ULB_AL, // 0x191C  # LIMBU LETTER HA
    LineBreakClass::ULB_ID, // 0x191D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x191E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x191F # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x1920  # LIMBU VOWEL SIGN A
    LineBreakClass::ULB_CM, // 0x1921  # LIMBU VOWEL SIGN I
    LineBreakClass::ULB_CM, // 0x1922  # LIMBU VOWEL SIGN U
    LineBreakClass::ULB_CM, // 0x1923  # LIMBU VOWEL SIGN EE
    LineBreakClass::ULB_CM, // 0x1924  # LIMBU VOWEL SIGN AI
    LineBreakClass::ULB_CM, // 0x1925  # LIMBU VOWEL SIGN OO
    LineBreakClass::ULB_CM, // 0x1926  # LIMBU VOWEL SIGN AU
    LineBreakClass::ULB_CM, // 0x1927  # LIMBU VOWEL SIGN E
    LineBreakClass::ULB_CM, // 0x1928  # LIMBU VOWEL SIGN O
    LineBreakClass::ULB_CM, // 0x1929  # LIMBU SUBJOINED LETTER YA
    LineBreakClass::ULB_CM, // 0x192A  # LIMBU SUBJOINED LETTER RA
    LineBreakClass::ULB_CM, // 0x192B  # LIMBU SUBJOINED LETTER WA
    LineBreakClass::ULB_ID, // 0x192C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x192D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x192E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x192F # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x1930  # LIMBU SMALL LETTER KA
    LineBreakClass::ULB_CM, // 0x1931  # LIMBU SMALL LETTER NGA
    LineBreakClass::ULB_CM, // 0x1932  # LIMBU SMALL LETTER ANUSVARA
    LineBreakClass::ULB_CM, // 0x1933  # LIMBU SMALL LETTER TA
    LineBreakClass::ULB_CM, // 0x1934  # LIMBU SMALL LETTER NA
    LineBreakClass::ULB_CM, // 0x1935  # LIMBU SMALL LETTER PA
    LineBreakClass::ULB_CM, // 0x1936  # LIMBU SMALL LETTER MA
    LineBreakClass::ULB_CM, // 0x1937  # LIMBU SMALL LETTER RA
    LineBreakClass::ULB_CM, // 0x1938  # LIMBU SMALL LETTER LA
    LineBreakClass::ULB_CM, // 0x1939  # LIMBU SIGN MUKPHRENG
    LineBreakClass::ULB_CM, // 0x193A  # LIMBU SIGN KEMPHRENG
    LineBreakClass::ULB_CM, // 0x193B  # LIMBU SIGN SA-I
    LineBreakClass::ULB_ID, // 0x193C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x193D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x193E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x193F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1940  # LIMBU SIGN LOO
    LineBreakClass::ULB_ID, // 0x1941 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1942 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1943 # <UNDEFINED>
    LineBreakClass::ULB_EX, // 0x1944  # LIMBU EXCLAMATION MARK
    LineBreakClass::ULB_EX, // 0x1945  # LIMBU QUESTION MARK
    LineBreakClass::ULB_NU, // 0x1946  # LIMBU DIGIT ZERO
    LineBreakClass::ULB_NU, // 0x1947  # LIMBU DIGIT ONE
    LineBreakClass::ULB_NU, // 0x1948  # LIMBU DIGIT TWO
    LineBreakClass::ULB_NU, // 0x1949  # LIMBU DIGIT THREE
    LineBreakClass::ULB_NU, // 0x194A  # LIMBU DIGIT FOUR
    LineBreakClass::ULB_NU, // 0x194B  # LIMBU DIGIT FIVE
    LineBreakClass::ULB_NU, // 0x194C  # LIMBU DIGIT SIX
    LineBreakClass::ULB_NU, // 0x194D  # LIMBU DIGIT SEVEN
    LineBreakClass::ULB_NU, // 0x194E  # LIMBU DIGIT EIGHT
    LineBreakClass::ULB_NU, // 0x194F  # LIMBU DIGIT NINE
    LineBreakClass::ULB_SA, // 0x1950  # TAI LE LETTER KA
    LineBreakClass::ULB_SA, // 0x1951  # TAI LE LETTER XA
    LineBreakClass::ULB_SA, // 0x1952  # TAI LE LETTER NGA
    LineBreakClass::ULB_SA, // 0x1953  # TAI LE LETTER TSA
    LineBreakClass::ULB_SA, // 0x1954  # TAI LE LETTER SA
    LineBreakClass::ULB_SA, // 0x1955  # TAI LE LETTER YA
    LineBreakClass::ULB_SA, // 0x1956  # TAI LE LETTER TA
    LineBreakClass::ULB_SA, // 0x1957  # TAI LE LETTER THA
    LineBreakClass::ULB_SA, // 0x1958  # TAI LE LETTER LA
    LineBreakClass::ULB_SA, // 0x1959  # TAI LE LETTER PA
    LineBreakClass::ULB_SA, // 0x195A  # TAI LE LETTER PHA
    LineBreakClass::ULB_SA, // 0x195B  # TAI LE LETTER MA
    LineBreakClass::ULB_SA, // 0x195C  # TAI LE LETTER FA
    LineBreakClass::ULB_SA, // 0x195D  # TAI LE LETTER VA
    LineBreakClass::ULB_SA, // 0x195E  # TAI LE LETTER HA
    LineBreakClass::ULB_SA, // 0x195F  # TAI LE LETTER QA
    LineBreakClass::ULB_SA, // 0x1960  # TAI LE LETTER KHA
    LineBreakClass::ULB_SA, // 0x1961  # TAI LE LETTER TSHA
    LineBreakClass::ULB_SA, // 0x1962  # TAI LE LETTER NA
    LineBreakClass::ULB_SA, // 0x1963  # TAI LE LETTER A
    LineBreakClass::ULB_SA, // 0x1964  # TAI LE LETTER I
    LineBreakClass::ULB_SA, // 0x1965  # TAI LE LETTER EE
    LineBreakClass::ULB_SA, // 0x1966  # TAI LE LETTER EH
    LineBreakClass::ULB_SA, // 0x1967  # TAI LE LETTER U
    LineBreakClass::ULB_SA, // 0x1968  # TAI LE LETTER OO
    LineBreakClass::ULB_SA, // 0x1969  # TAI LE LETTER O
    LineBreakClass::ULB_SA, // 0x196A  # TAI LE LETTER UE
    LineBreakClass::ULB_SA, // 0x196B  # TAI LE LETTER E
    LineBreakClass::ULB_SA, // 0x196C  # TAI LE LETTER AUE
    LineBreakClass::ULB_SA, // 0x196D  # TAI LE LETTER AI
    LineBreakClass::ULB_ID, // 0x196E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x196F # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x1970  # TAI LE LETTER TONE-2
    LineBreakClass::ULB_SA, // 0x1971  # TAI LE LETTER TONE-3
    LineBreakClass::ULB_SA, // 0x1972  # TAI LE LETTER TONE-4
    LineBreakClass::ULB_SA, // 0x1973  # TAI LE LETTER TONE-5
    LineBreakClass::ULB_SA, // 0x1974  # TAI LE LETTER TONE-6
    LineBreakClass::ULB_ID, // 0x1975 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1976 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1977 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1978 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1979 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x197A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x197B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x197C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x197D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x197E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x197F # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x1980  # NEW TAI LUE LETTER HIGH QA
    LineBreakClass::ULB_SA, // 0x1981  # NEW TAI LUE LETTER LOW QA
    LineBreakClass::ULB_SA, // 0x1982  # NEW TAI LUE LETTER HIGH KA
    LineBreakClass::ULB_SA, // 0x1983  # NEW TAI LUE LETTER HIGH XA
    LineBreakClass::ULB_SA, // 0x1984  # NEW TAI LUE LETTER HIGH NGA
    LineBreakClass::ULB_SA, // 0x1985  # NEW TAI LUE LETTER LOW KA
    LineBreakClass::ULB_SA, // 0x1986  # NEW TAI LUE LETTER LOW XA
    LineBreakClass::ULB_SA, // 0x1987  # NEW TAI LUE LETTER LOW NGA
    LineBreakClass::ULB_SA, // 0x1988  # NEW TAI LUE LETTER HIGH TSA
    LineBreakClass::ULB_SA, // 0x1989  # NEW TAI LUE LETTER HIGH SA
    LineBreakClass::ULB_SA, // 0x198A  # NEW TAI LUE LETTER HIGH YA
    LineBreakClass::ULB_SA, // 0x198B  # NEW TAI LUE LETTER LOW TSA
    LineBreakClass::ULB_SA, // 0x198C  # NEW TAI LUE LETTER LOW SA
    LineBreakClass::ULB_SA, // 0x198D  # NEW TAI LUE LETTER LOW YA
    LineBreakClass::ULB_SA, // 0x198E  # NEW TAI LUE LETTER HIGH TA
    LineBreakClass::ULB_SA, // 0x198F  # NEW TAI LUE LETTER HIGH THA
    LineBreakClass::ULB_SA, // 0x1990  # NEW TAI LUE LETTER HIGH NA
    LineBreakClass::ULB_SA, // 0x1991  # NEW TAI LUE LETTER LOW TA
    LineBreakClass::ULB_SA, // 0x1992  # NEW TAI LUE LETTER LOW THA
    LineBreakClass::ULB_SA, // 0x1993  # NEW TAI LUE LETTER LOW NA
    LineBreakClass::ULB_SA, // 0x1994  # NEW TAI LUE LETTER HIGH PA
    LineBreakClass::ULB_SA, // 0x1995  # NEW TAI LUE LETTER HIGH PHA
    LineBreakClass::ULB_SA, // 0x1996  # NEW TAI LUE LETTER HIGH MA
    LineBreakClass::ULB_SA, // 0x1997  # NEW TAI LUE LETTER LOW PA
    LineBreakClass::ULB_SA, // 0x1998  # NEW TAI LUE LETTER LOW PHA
    LineBreakClass::ULB_SA, // 0x1999  # NEW TAI LUE LETTER LOW MA
    LineBreakClass::ULB_SA, // 0x199A  # NEW TAI LUE LETTER HIGH FA
    LineBreakClass::ULB_SA, // 0x199B  # NEW TAI LUE LETTER HIGH VA
    LineBreakClass::ULB_SA, // 0x199C  # NEW TAI LUE LETTER HIGH LA
    LineBreakClass::ULB_SA, // 0x199D  # NEW TAI LUE LETTER LOW FA
    LineBreakClass::ULB_SA, // 0x199E  # NEW TAI LUE LETTER LOW VA
    LineBreakClass::ULB_SA, // 0x199F  # NEW TAI LUE LETTER LOW LA
    LineBreakClass::ULB_SA, // 0x19A0  # NEW TAI LUE LETTER HIGH HA
    LineBreakClass::ULB_SA, // 0x19A1  # NEW TAI LUE LETTER HIGH DA
    LineBreakClass::ULB_SA, // 0x19A2  # NEW TAI LUE LETTER HIGH BA
    LineBreakClass::ULB_SA, // 0x19A3  # NEW TAI LUE LETTER LOW HA
    LineBreakClass::ULB_SA, // 0x19A4  # NEW TAI LUE LETTER LOW DA
    LineBreakClass::ULB_SA, // 0x19A5  # NEW TAI LUE LETTER LOW BA
    LineBreakClass::ULB_SA, // 0x19A6  # NEW TAI LUE LETTER HIGH KVA
    LineBreakClass::ULB_SA, // 0x19A7  # NEW TAI LUE LETTER HIGH XVA
    LineBreakClass::ULB_SA, // 0x19A8  # NEW TAI LUE LETTER LOW KVA
    LineBreakClass::ULB_SA, // 0x19A9  # NEW TAI LUE LETTER LOW XVA
    LineBreakClass::ULB_ID, // 0x19AA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x19AB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x19AC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x19AD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x19AE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x19AF # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x19B0  # NEW TAI LUE VOWEL SIGN VOWEL SHORTENER
    LineBreakClass::ULB_SA, // 0x19B1  # NEW TAI LUE VOWEL SIGN AA
    LineBreakClass::ULB_SA, // 0x19B2  # NEW TAI LUE VOWEL SIGN II
    LineBreakClass::ULB_SA, // 0x19B3  # NEW TAI LUE VOWEL SIGN U
    LineBreakClass::ULB_SA, // 0x19B4  # NEW TAI LUE VOWEL SIGN UU
    LineBreakClass::ULB_SA, // 0x19B5  # NEW TAI LUE VOWEL SIGN E
    LineBreakClass::ULB_SA, // 0x19B6  # NEW TAI LUE VOWEL SIGN AE
    LineBreakClass::ULB_SA, // 0x19B7  # NEW TAI LUE VOWEL SIGN O
    LineBreakClass::ULB_SA, // 0x19B8  # NEW TAI LUE VOWEL SIGN OA
    LineBreakClass::ULB_SA, // 0x19B9  # NEW TAI LUE VOWEL SIGN UE
    LineBreakClass::ULB_SA, // 0x19BA  # NEW TAI LUE VOWEL SIGN AY
    LineBreakClass::ULB_SA, // 0x19BB  # NEW TAI LUE VOWEL SIGN AAY
    LineBreakClass::ULB_SA, // 0x19BC  # NEW TAI LUE VOWEL SIGN UY
    LineBreakClass::ULB_SA, // 0x19BD  # NEW TAI LUE VOWEL SIGN OY
    LineBreakClass::ULB_SA, // 0x19BE  # NEW TAI LUE VOWEL SIGN OAY
    LineBreakClass::ULB_SA, // 0x19BF  # NEW TAI LUE VOWEL SIGN UEY
    LineBreakClass::ULB_SA, // 0x19C0  # NEW TAI LUE VOWEL SIGN IY
    LineBreakClass::ULB_SA, // 0x19C1  # NEW TAI LUE LETTER FINAL V
    LineBreakClass::ULB_SA, // 0x19C2  # NEW TAI LUE LETTER FINAL NG
    LineBreakClass::ULB_SA, // 0x19C3  # NEW TAI LUE LETTER FINAL N
    LineBreakClass::ULB_SA, // 0x19C4  # NEW TAI LUE LETTER FINAL M
    LineBreakClass::ULB_SA, // 0x19C5  # NEW TAI LUE LETTER FINAL K
    LineBreakClass::ULB_SA, // 0x19C6  # NEW TAI LUE LETTER FINAL D
    LineBreakClass::ULB_SA, // 0x19C7  # NEW TAI LUE LETTER FINAL B
    LineBreakClass::ULB_SA, // 0x19C8  # NEW TAI LUE TONE MARK-1
    LineBreakClass::ULB_SA, // 0x19C9  # NEW TAI LUE TONE MARK-2
    LineBreakClass::ULB_ID, // 0x19CA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x19CB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x19CC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x19CD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x19CE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x19CF # <UNDEFINED>
    LineBreakClass::ULB_NU, // 0x19D0  # NEW TAI LUE DIGIT ZERO
    LineBreakClass::ULB_NU, // 0x19D1  # NEW TAI LUE DIGIT ONE
    LineBreakClass::ULB_NU, // 0x19D2  # NEW TAI LUE DIGIT TWO
    LineBreakClass::ULB_NU, // 0x19D3  # NEW TAI LUE DIGIT THREE
    LineBreakClass::ULB_NU, // 0x19D4  # NEW TAI LUE DIGIT FOUR
    LineBreakClass::ULB_NU, // 0x19D5  # NEW TAI LUE DIGIT FIVE
    LineBreakClass::ULB_NU, // 0x19D6  # NEW TAI LUE DIGIT SIX
    LineBreakClass::ULB_NU, // 0x19D7  # NEW TAI LUE DIGIT SEVEN
    LineBreakClass::ULB_NU, // 0x19D8  # NEW TAI LUE DIGIT EIGHT
    LineBreakClass::ULB_NU, // 0x19D9  # NEW TAI LUE DIGIT NINE
    LineBreakClass::ULB_ID, // 0x19DA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x19DB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x19DC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x19DD # <UNDEFINED>
    LineBreakClass::ULB_SA, // 0x19DE  # NEW TAI LUE SIGN LAE
    LineBreakClass::ULB_SA, // 0x19DF  # NEW TAI LUE SIGN LAEV
    LineBreakClass::ULB_AL, // 0x19E0  # KHMER SYMBOL PATHAMASAT
    LineBreakClass::ULB_AL, // 0x19E1  # KHMER SYMBOL MUOY KOET
    LineBreakClass::ULB_AL, // 0x19E2  # KHMER SYMBOL PII KOET
    LineBreakClass::ULB_AL, // 0x19E3  # KHMER SYMBOL BEI KOET
    LineBreakClass::ULB_AL, // 0x19E4  # KHMER SYMBOL BUON KOET
    LineBreakClass::ULB_AL, // 0x19E5  # KHMER SYMBOL PRAM KOET
    LineBreakClass::ULB_AL, // 0x19E6  # KHMER SYMBOL PRAM-MUOY KOET
    LineBreakClass::ULB_AL, // 0x19E7  # KHMER SYMBOL PRAM-PII KOET
    LineBreakClass::ULB_AL, // 0x19E8  # KHMER SYMBOL PRAM-BEI KOET
    LineBreakClass::ULB_AL, // 0x19E9  # KHMER SYMBOL PRAM-BUON KOET
    LineBreakClass::ULB_AL, // 0x19EA  # KHMER SYMBOL DAP KOET
    LineBreakClass::ULB_AL, // 0x19EB  # KHMER SYMBOL DAP-MUOY KOET
    LineBreakClass::ULB_AL, // 0x19EC  # KHMER SYMBOL DAP-PII KOET
    LineBreakClass::ULB_AL, // 0x19ED  # KHMER SYMBOL DAP-BEI KOET
    LineBreakClass::ULB_AL, // 0x19EE  # KHMER SYMBOL DAP-BUON KOET
    LineBreakClass::ULB_AL, // 0x19EF  # KHMER SYMBOL DAP-PRAM KOET
    LineBreakClass::ULB_AL, // 0x19F0  # KHMER SYMBOL TUTEYASAT
    LineBreakClass::ULB_AL, // 0x19F1  # KHMER SYMBOL MUOY ROC
    LineBreakClass::ULB_AL, // 0x19F2  # KHMER SYMBOL PII ROC
    LineBreakClass::ULB_AL, // 0x19F3  # KHMER SYMBOL BEI ROC
    LineBreakClass::ULB_AL, // 0x19F4  # KHMER SYMBOL BUON ROC
    LineBreakClass::ULB_AL, // 0x19F5  # KHMER SYMBOL PRAM ROC
    LineBreakClass::ULB_AL, // 0x19F6  # KHMER SYMBOL PRAM-MUOY ROC
    LineBreakClass::ULB_AL, // 0x19F7  # KHMER SYMBOL PRAM-PII ROC
    LineBreakClass::ULB_AL, // 0x19F8  # KHMER SYMBOL PRAM-BEI ROC
    LineBreakClass::ULB_AL, // 0x19F9  # KHMER SYMBOL PRAM-BUON ROC
    LineBreakClass::ULB_AL, // 0x19FA  # KHMER SYMBOL DAP ROC
    LineBreakClass::ULB_AL, // 0x19FB  # KHMER SYMBOL DAP-MUOY ROC
    LineBreakClass::ULB_AL, // 0x19FC  # KHMER SYMBOL DAP-PII ROC
    LineBreakClass::ULB_AL, // 0x19FD  # KHMER SYMBOL DAP-BEI ROC
    LineBreakClass::ULB_AL, // 0x19FE  # KHMER SYMBOL DAP-BUON ROC
    LineBreakClass::ULB_AL, // 0x19FF  # KHMER SYMBOL DAP-PRAM ROC
    LineBreakClass::ULB_AL, // 0x1A00  # BUGINESE LETTER KA
    LineBreakClass::ULB_AL, // 0x1A01  # BUGINESE LETTER GA
    LineBreakClass::ULB_AL, // 0x1A02  # BUGINESE LETTER NGA
    LineBreakClass::ULB_AL, // 0x1A03  # BUGINESE LETTER NGKA
    LineBreakClass::ULB_AL, // 0x1A04  # BUGINESE LETTER PA
    LineBreakClass::ULB_AL, // 0x1A05  # BUGINESE LETTER BA
    LineBreakClass::ULB_AL, // 0x1A06  # BUGINESE LETTER MA
    LineBreakClass::ULB_AL, // 0x1A07  # BUGINESE LETTER MPA
    LineBreakClass::ULB_AL, // 0x1A08  # BUGINESE LETTER TA
    LineBreakClass::ULB_AL, // 0x1A09  # BUGINESE LETTER DA
    LineBreakClass::ULB_AL, // 0x1A0A  # BUGINESE LETTER NA
    LineBreakClass::ULB_AL, // 0x1A0B  # BUGINESE LETTER NRA
    LineBreakClass::ULB_AL, // 0x1A0C  # BUGINESE LETTER CA
    LineBreakClass::ULB_AL, // 0x1A0D  # BUGINESE LETTER JA
    LineBreakClass::ULB_AL, // 0x1A0E  # BUGINESE LETTER NYA
    LineBreakClass::ULB_AL, // 0x1A0F  # BUGINESE LETTER NYCA
    LineBreakClass::ULB_AL, // 0x1A10  # BUGINESE LETTER YA
    LineBreakClass::ULB_AL, // 0x1A11  # BUGINESE LETTER RA
    LineBreakClass::ULB_AL, // 0x1A12  # BUGINESE LETTER LA
    LineBreakClass::ULB_AL, // 0x1A13  # BUGINESE LETTER VA
    LineBreakClass::ULB_AL, // 0x1A14  # BUGINESE LETTER SA
    LineBreakClass::ULB_AL, // 0x1A15  # BUGINESE LETTER A
    LineBreakClass::ULB_AL, // 0x1A16  # BUGINESE LETTER HA
    LineBreakClass::ULB_CM, // 0x1A17  # BUGINESE VOWEL SIGN I
    LineBreakClass::ULB_CM, // 0x1A18  # BUGINESE VOWEL SIGN U
    LineBreakClass::ULB_CM, // 0x1A19  # BUGINESE VOWEL SIGN E
    LineBreakClass::ULB_CM, // 0x1A1A  # BUGINESE VOWEL SIGN O
    LineBreakClass::ULB_CM, // 0x1A1B  # BUGINESE VOWEL SIGN AE
    LineBreakClass::ULB_ID, // 0x1A1C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A1D # <UNDEFINED>
    LineBreakClass::ULB_BA, // 0x1A1E  # BUGINESE PALLAWA
    LineBreakClass::ULB_AL, // 0x1A1F  # BUGINESE END OF SECTION
    LineBreakClass::ULB_ID, // 0x1A20 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A21 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A22 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A23 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A24 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A25 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A26 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A27 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A28 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A29 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A2A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A2B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A2C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A2D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A2E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A2F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A30 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A31 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A32 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A33 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A34 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A35 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A36 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A37 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A38 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A39 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A3A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A3B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A3C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A3D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A3E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A3F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A40 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A41 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A42 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A43 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A44 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A45 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A46 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A47 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A48 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A49 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A4A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A4B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A4C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A4D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A4E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A4F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A50 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A51 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A52 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A53 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A54 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A55 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A56 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A57 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A58 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A59 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A5A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A5B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A5C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A5D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A5E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A5F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A60 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A61 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A62 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A63 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A64 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A65 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A66 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A67 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A68 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A69 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A6A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A6B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A6C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A6D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A6E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A6F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A70 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A71 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A72 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A73 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A74 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A75 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A76 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A77 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A78 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A79 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A7A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A7B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A7C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A7D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A7E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A7F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A80 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A81 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A82 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A83 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A84 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A85 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A86 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A87 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A88 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A89 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A8A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A8B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A8C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A8D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A8E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A8F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A90 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A91 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A92 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A93 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A94 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A95 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A96 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A97 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A98 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A99 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A9A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A9B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A9C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A9D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A9E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1A9F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AA0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AA1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AA2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AA3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AA4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AA5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AA6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AA7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AA8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AA9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AAA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AAB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AAC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AAD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AAE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AAF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AB0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AB1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AB2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AB3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AB4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AB5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AB6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AB7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AB8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AB9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1ABA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1ABB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1ABC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1ABD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1ABE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1ABF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AC0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AC1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AC2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AC3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AC4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AC5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AC6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AC7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AC8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AC9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1ACA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1ACB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1ACC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1ACD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1ACE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1ACF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AD0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AD1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AD2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AD3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AD4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AD5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AD6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AD7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AD8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AD9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1ADA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1ADB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1ADC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1ADD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1ADE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1ADF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AE0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AE1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AE2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AE3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AE4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AE5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AE6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AE7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AE8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AE9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AEA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AEB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AEC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AED # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AEE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AEF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AF0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AF1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AF2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AF3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AF4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AF5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AF6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AF7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AF8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AF9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AFA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AFB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AFC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AFD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AFE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1AFF # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x1B00  # BALINESE SIGN ULU RICEM
    LineBreakClass::ULB_CM, // 0x1B01  # BALINESE SIGN ULU CANDRA
    LineBreakClass::ULB_CM, // 0x1B02  # BALINESE SIGN CECEK
    LineBreakClass::ULB_CM, // 0x1B03  # BALINESE SIGN SURANG
    LineBreakClass::ULB_CM, // 0x1B04  # BALINESE SIGN BISAH
    LineBreakClass::ULB_AL, // 0x1B05  # BALINESE LETTER AKARA
    LineBreakClass::ULB_AL, // 0x1B06  # BALINESE LETTER AKARA TEDUNG
    LineBreakClass::ULB_AL, // 0x1B07  # BALINESE LETTER IKARA
    LineBreakClass::ULB_AL, // 0x1B08  # BALINESE LETTER IKARA TEDUNG
    LineBreakClass::ULB_AL, // 0x1B09  # BALINESE LETTER UKARA
    LineBreakClass::ULB_AL, // 0x1B0A  # BALINESE LETTER UKARA TEDUNG
    LineBreakClass::ULB_AL, // 0x1B0B  # BALINESE LETTER RA REPA
    LineBreakClass::ULB_AL, // 0x1B0C  # BALINESE LETTER RA REPA TEDUNG
    LineBreakClass::ULB_AL, // 0x1B0D  # BALINESE LETTER LA LENGA
    LineBreakClass::ULB_AL, // 0x1B0E  # BALINESE LETTER LA LENGA TEDUNG
    LineBreakClass::ULB_AL, // 0x1B0F  # BALINESE LETTER EKARA
    LineBreakClass::ULB_AL, // 0x1B10  # BALINESE LETTER AIKARA
    LineBreakClass::ULB_AL, // 0x1B11  # BALINESE LETTER OKARA
    LineBreakClass::ULB_AL, // 0x1B12  # BALINESE LETTER OKARA TEDUNG
    LineBreakClass::ULB_AL, // 0x1B13  # BALINESE LETTER KA
    LineBreakClass::ULB_AL, // 0x1B14  # BALINESE LETTER KA MAHAPRANA
    LineBreakClass::ULB_AL, // 0x1B15  # BALINESE LETTER GA
    LineBreakClass::ULB_AL, // 0x1B16  # BALINESE LETTER GA GORA
    LineBreakClass::ULB_AL, // 0x1B17  # BALINESE LETTER NGA
    LineBreakClass::ULB_AL, // 0x1B18  # BALINESE LETTER CA
    LineBreakClass::ULB_AL, // 0x1B19  # BALINESE LETTER CA LACA
    LineBreakClass::ULB_AL, // 0x1B1A  # BALINESE LETTER JA
    LineBreakClass::ULB_AL, // 0x1B1B  # BALINESE LETTER JA JERA
    LineBreakClass::ULB_AL, // 0x1B1C  # BALINESE LETTER NYA
    LineBreakClass::ULB_AL, // 0x1B1D  # BALINESE LETTER TA LATIK
    LineBreakClass::ULB_AL, // 0x1B1E  # BALINESE LETTER TA MURDA MAHAPRANA
    LineBreakClass::ULB_AL, // 0x1B1F  # BALINESE LETTER DA MURDA ALPAPRANA
    LineBreakClass::ULB_AL, // 0x1B20  # BALINESE LETTER DA MURDA MAHAPRANA
    LineBreakClass::ULB_AL, // 0x1B21  # BALINESE LETTER NA RAMBAT
    LineBreakClass::ULB_AL, // 0x1B22  # BALINESE LETTER TA
    LineBreakClass::ULB_AL, // 0x1B23  # BALINESE LETTER TA TAWA
    LineBreakClass::ULB_AL, // 0x1B24  # BALINESE LETTER DA
    LineBreakClass::ULB_AL, // 0x1B25  # BALINESE LETTER DA MADU
    LineBreakClass::ULB_AL, // 0x1B26  # BALINESE LETTER NA
    LineBreakClass::ULB_AL, // 0x1B27  # BALINESE LETTER PA
    LineBreakClass::ULB_AL, // 0x1B28  # BALINESE LETTER PA KAPAL
    LineBreakClass::ULB_AL, // 0x1B29  # BALINESE LETTER BA
    LineBreakClass::ULB_AL, // 0x1B2A  # BALINESE LETTER BA KEMBANG
    LineBreakClass::ULB_AL, // 0x1B2B  # BALINESE LETTER MA
    LineBreakClass::ULB_AL, // 0x1B2C  # BALINESE LETTER YA
    LineBreakClass::ULB_AL, // 0x1B2D  # BALINESE LETTER RA
    LineBreakClass::ULB_AL, // 0x1B2E  # BALINESE LETTER LA
    LineBreakClass::ULB_AL, // 0x1B2F  # BALINESE LETTER WA
    LineBreakClass::ULB_AL, // 0x1B30  # BALINESE LETTER SA SAGA
    LineBreakClass::ULB_AL, // 0x1B31  # BALINESE LETTER SA SAPA
    LineBreakClass::ULB_AL, // 0x1B32  # BALINESE LETTER SA
    LineBreakClass::ULB_AL, // 0x1B33  # BALINESE LETTER HA
    LineBreakClass::ULB_CM, // 0x1B34  # BALINESE SIGN REREKAN
    LineBreakClass::ULB_CM, // 0x1B35  # BALINESE VOWEL SIGN TEDUNG
    LineBreakClass::ULB_CM, // 0x1B36  # BALINESE VOWEL SIGN ULU
    LineBreakClass::ULB_CM, // 0x1B37  # BALINESE VOWEL SIGN ULU SARI
    LineBreakClass::ULB_CM, // 0x1B38  # BALINESE VOWEL SIGN SUKU
    LineBreakClass::ULB_CM, // 0x1B39  # BALINESE VOWEL SIGN SUKU ILUT
    LineBreakClass::ULB_CM, // 0x1B3A  # BALINESE VOWEL SIGN RA REPA
    LineBreakClass::ULB_CM, // 0x1B3B  # BALINESE VOWEL SIGN RA REPA TEDUNG
    LineBreakClass::ULB_CM, // 0x1B3C  # BALINESE VOWEL SIGN LA LENGA
    LineBreakClass::ULB_CM, // 0x1B3D  # BALINESE VOWEL SIGN LA LENGA TEDUNG
    LineBreakClass::ULB_CM, // 0x1B3E  # BALINESE VOWEL SIGN TALING
    LineBreakClass::ULB_CM, // 0x1B3F  # BALINESE VOWEL SIGN TALING REPA
    LineBreakClass::ULB_CM, // 0x1B40  # BALINESE VOWEL SIGN TALING TEDUNG
    LineBreakClass::ULB_CM, // 0x1B41  # BALINESE VOWEL SIGN TALING REPA TEDUNG
    LineBreakClass::ULB_CM, // 0x1B42  # BALINESE VOWEL SIGN PEPET
    LineBreakClass::ULB_CM, // 0x1B43  # BALINESE VOWEL SIGN PEPET TEDUNG
    LineBreakClass::ULB_CM, // 0x1B44  # BALINESE ADEG ADEG
    LineBreakClass::ULB_AL, // 0x1B45  # BALINESE LETTER KAF SASAK
    LineBreakClass::ULB_AL, // 0x1B46  # BALINESE LETTER KHOT SASAK
    LineBreakClass::ULB_AL, // 0x1B47  # BALINESE LETTER TZIR SASAK
    LineBreakClass::ULB_AL, // 0x1B48  # BALINESE LETTER EF SASAK
    LineBreakClass::ULB_AL, // 0x1B49  # BALINESE LETTER VE SASAK
    LineBreakClass::ULB_AL, // 0x1B4A  # BALINESE LETTER ZAL SASAK
    LineBreakClass::ULB_AL, // 0x1B4B  # BALINESE LETTER ASYURA SASAK
    LineBreakClass::ULB_ID, // 0x1B4C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B4D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B4E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B4F # <UNDEFINED>
    LineBreakClass::ULB_NU, // 0x1B50  # BALINESE DIGIT ZERO
    LineBreakClass::ULB_NU, // 0x1B51  # BALINESE DIGIT ONE
    LineBreakClass::ULB_NU, // 0x1B52  # BALINESE DIGIT TWO
    LineBreakClass::ULB_NU, // 0x1B53  # BALINESE DIGIT THREE
    LineBreakClass::ULB_NU, // 0x1B54  # BALINESE DIGIT FOUR
    LineBreakClass::ULB_NU, // 0x1B55  # BALINESE DIGIT FIVE
    LineBreakClass::ULB_NU, // 0x1B56  # BALINESE DIGIT SIX
    LineBreakClass::ULB_NU, // 0x1B57  # BALINESE DIGIT SEVEN
    LineBreakClass::ULB_NU, // 0x1B58  # BALINESE DIGIT EIGHT
    LineBreakClass::ULB_NU, // 0x1B59  # BALINESE DIGIT NINE
    LineBreakClass::ULB_BA, // 0x1B5A  # BALINESE PANTI
    LineBreakClass::ULB_BA, // 0x1B5B  # BALINESE PAMADA
    LineBreakClass::ULB_BA, // 0x1B5C  # BALINESE WINDU
    LineBreakClass::ULB_BA, // 0x1B5D  # BALINESE CARIK PAMUNGKAH
    LineBreakClass::ULB_BA, // 0x1B5E  # BALINESE CARIK SIKI
    LineBreakClass::ULB_BA, // 0x1B5F  # BALINESE CARIK PAREREN
    LineBreakClass::ULB_BA, // 0x1B60  # BALINESE PAMENENG
    LineBreakClass::ULB_AL, // 0x1B61  # BALINESE MUSICAL SYMBOL DONG
    LineBreakClass::ULB_AL, // 0x1B62  # BALINESE MUSICAL SYMBOL DENG
    LineBreakClass::ULB_AL, // 0x1B63  # BALINESE MUSICAL SYMBOL DUNG
    LineBreakClass::ULB_AL, // 0x1B64  # BALINESE MUSICAL SYMBOL DANG
    LineBreakClass::ULB_AL, // 0x1B65  # BALINESE MUSICAL SYMBOL DANG SURANG
    LineBreakClass::ULB_AL, // 0x1B66  # BALINESE MUSICAL SYMBOL DING
    LineBreakClass::ULB_AL, // 0x1B67  # BALINESE MUSICAL SYMBOL DAENG
    LineBreakClass::ULB_AL, // 0x1B68  # BALINESE MUSICAL SYMBOL DEUNG
    LineBreakClass::ULB_AL, // 0x1B69  # BALINESE MUSICAL SYMBOL DAING
    LineBreakClass::ULB_AL, // 0x1B6A  # BALINESE MUSICAL SYMBOL DANG GEDE
    LineBreakClass::ULB_CM, // 0x1B6B  # BALINESE MUSICAL SYMBOL COMBINING TEGEH
    LineBreakClass::ULB_CM, // 0x1B6C  # BALINESE MUSICAL SYMBOL COMBINING ENDEP
    LineBreakClass::ULB_CM, // 0x1B6D  # BALINESE MUSICAL SYMBOL COMBINING KEMPUL
    LineBreakClass::ULB_CM, // 0x1B6E  # BALINESE MUSICAL SYMBOL COMBINING KEMPLI
    LineBreakClass::ULB_CM, // 0x1B6F  # BALINESE MUSICAL SYMBOL COMBINING JEGOGAN
    LineBreakClass::ULB_CM, // 0x1B70  # BALINESE MUSICAL SYMBOL COMBINING KEMPUL WITH JEGOGAN
    LineBreakClass::ULB_CM, // 0x1B71  # BALINESE MUSICAL SYMBOL COMBINING KEMPLI WITH JEGOGAN
    LineBreakClass::ULB_CM, // 0x1B72  # BALINESE MUSICAL SYMBOL COMBINING BENDE
    LineBreakClass::ULB_CM, // 0x1B73  # BALINESE MUSICAL SYMBOL COMBINING GONG
    LineBreakClass::ULB_AL, // 0x1B74  # BALINESE MUSICAL SYMBOL RIGHT-HAND OPEN DUG
    LineBreakClass::ULB_AL, // 0x1B75  # BALINESE MUSICAL SYMBOL RIGHT-HAND OPEN DAG
    LineBreakClass::ULB_AL, // 0x1B76  # BALINESE MUSICAL SYMBOL RIGHT-HAND CLOSED TUK
    LineBreakClass::ULB_AL, // 0x1B77  # BALINESE MUSICAL SYMBOL RIGHT-HAND CLOSED TAK
    LineBreakClass::ULB_AL, // 0x1B78  # BALINESE MUSICAL SYMBOL LEFT-HAND OPEN PANG
    LineBreakClass::ULB_AL, // 0x1B79  # BALINESE MUSICAL SYMBOL LEFT-HAND OPEN PUNG
    LineBreakClass::ULB_AL, // 0x1B7A  # BALINESE MUSICAL SYMBOL LEFT-HAND CLOSED PLAK
    LineBreakClass::ULB_AL, // 0x1B7B  # BALINESE MUSICAL SYMBOL LEFT-HAND CLOSED PLUK
    LineBreakClass::ULB_AL, // 0x1B7C  # BALINESE MUSICAL SYMBOL LEFT-HAND OPEN PING
    LineBreakClass::ULB_ID, // 0x1B7D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B7E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B7F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B80 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B81 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B82 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B83 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B84 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B85 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B86 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B87 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B88 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B89 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B8A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B8B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B8C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B8D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B8E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B8F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B90 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B91 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B92 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B93 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B94 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B95 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B96 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B97 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B98 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B99 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B9A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B9B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B9C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B9D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B9E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1B9F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BA0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BA1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BA2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BA3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BA4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BA5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BA6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BA7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BA8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BA9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BAA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BAB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BAC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BAD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BAE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BAF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BB0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BB1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BB2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BB3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BB4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BB5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BB6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BB7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BB8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BB9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BBA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BBB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BBC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BBD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BBE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BBF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BC0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BC1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BC2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BC3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BC4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BC5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BC6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BC7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BC8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BC9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BCA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BCB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BCC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BCD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BCE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BCF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BD0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BD1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BD2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BD3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BD4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BD5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BD6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BD7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BD8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BD9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BDA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BDB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BDC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BDD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BDE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BDF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BE0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BE1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BE2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BE3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BE4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BE5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BE6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BE7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BE8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BE9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BEA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BEB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BEC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BED # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BEE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BEF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BF0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BF1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BF2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BF3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BF4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BF5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BF6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BF7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BF8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BF9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BFA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BFB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BFC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BFD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BFE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1BFF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C00 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C01 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C02 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C03 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C04 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C05 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C06 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C07 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C08 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C09 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C0A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C0B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C0C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C0D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C0E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C0F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C10 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C11 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C12 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C13 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C14 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C15 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C16 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C17 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C18 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C19 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C1A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C1B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C1C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C1D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C1E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C1F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C20 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C21 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C22 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C23 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C24 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C25 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C26 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C27 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C28 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C29 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C2A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C2B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C2C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C2D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C2E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C2F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C30 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C31 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C32 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C33 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C34 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C35 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C36 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C37 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C38 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C39 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C3A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C3B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C3C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C3D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C3E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C3F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C40 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C41 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C42 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C43 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C44 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C45 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C46 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C47 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C48 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C49 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C4A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C4B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C4C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C4D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C4E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C4F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C50 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C51 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C52 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C53 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C54 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C55 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C56 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C57 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C58 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C59 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C5A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C5B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C5C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C5D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C5E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C5F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C60 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C61 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C62 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C63 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C64 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C65 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C66 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C67 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C68 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C69 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C6A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C6B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C6C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C6D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C6E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C6F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C70 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C71 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C72 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C73 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C74 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C75 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C76 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C77 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C78 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C79 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C7A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C7B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C7C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C7D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C7E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C7F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C80 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C81 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C82 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C83 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C84 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C85 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C86 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C87 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C88 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C89 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C8A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C8B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C8C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C8D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C8E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C8F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C90 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C91 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C92 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C93 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C94 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C95 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C96 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C97 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C98 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C99 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C9A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C9B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C9C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C9D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C9E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1C9F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CA0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CA1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CA2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CA3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CA4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CA5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CA6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CA7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CA8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CA9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CAA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CAB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CAC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CAD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CAE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CAF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CB0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CB1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CB2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CB3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CB4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CB5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CB6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CB7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CB8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CB9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CBA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CBB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CBC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CBD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CBE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CBF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CC0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CC1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CC2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CC3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CC4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CC5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CC6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CC7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CC8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CC9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CCA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CCB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CCC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CCD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CCE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CCF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CD0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CD1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CD2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CD3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CD4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CD5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CD6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CD7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CD8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CD9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CDA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CDB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CDC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CDD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CDE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CDF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CE0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CE1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CE2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CE3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CE4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CE5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CE6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CE7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CE8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CE9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CEA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CEB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CEC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CED # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CEE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CEF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CF0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CF1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CF2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CF3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CF4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CF5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CF6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CF7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CF8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CF9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CFA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CFB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CFC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CFD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CFE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1CFF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1D00  # LATIN LETTER SMALL CAPITAL A
    LineBreakClass::ULB_AL, // 0x1D01  # LATIN LETTER SMALL CAPITAL AE
    LineBreakClass::ULB_AL, // 0x1D02  # LATIN SMALL LETTER TURNED AE
    LineBreakClass::ULB_AL, // 0x1D03  # LATIN LETTER SMALL CAPITAL BARRED B
    LineBreakClass::ULB_AL, // 0x1D04  # LATIN LETTER SMALL CAPITAL C
    LineBreakClass::ULB_AL, // 0x1D05  # LATIN LETTER SMALL CAPITAL D
    LineBreakClass::ULB_AL, // 0x1D06  # LATIN LETTER SMALL CAPITAL ETH
    LineBreakClass::ULB_AL, // 0x1D07  # LATIN LETTER SMALL CAPITAL E
    LineBreakClass::ULB_AL, // 0x1D08  # LATIN SMALL LETTER TURNED OPEN E
    LineBreakClass::ULB_AL, // 0x1D09  # LATIN SMALL LETTER TURNED I
    LineBreakClass::ULB_AL, // 0x1D0A  # LATIN LETTER SMALL CAPITAL J
    LineBreakClass::ULB_AL, // 0x1D0B  # LATIN LETTER SMALL CAPITAL K
    LineBreakClass::ULB_AL, // 0x1D0C  # LATIN LETTER SMALL CAPITAL L WITH STROKE
    LineBreakClass::ULB_AL, // 0x1D0D  # LATIN LETTER SMALL CAPITAL M
    LineBreakClass::ULB_AL, // 0x1D0E  # LATIN LETTER SMALL CAPITAL REVERSED N
    LineBreakClass::ULB_AL, // 0x1D0F  # LATIN LETTER SMALL CAPITAL O
    LineBreakClass::ULB_AL, // 0x1D10  # LATIN LETTER SMALL CAPITAL OPEN O
    LineBreakClass::ULB_AL, // 0x1D11  # LATIN SMALL LETTER SIDEWAYS O
    LineBreakClass::ULB_AL, // 0x1D12  # LATIN SMALL LETTER SIDEWAYS OPEN O
    LineBreakClass::ULB_AL, // 0x1D13  # LATIN SMALL LETTER SIDEWAYS O WITH STROKE
    LineBreakClass::ULB_AL, // 0x1D14  # LATIN SMALL LETTER TURNED OE
    LineBreakClass::ULB_AL, // 0x1D15  # LATIN LETTER SMALL CAPITAL OU
    LineBreakClass::ULB_AL, // 0x1D16  # LATIN SMALL LETTER TOP HALF O
    LineBreakClass::ULB_AL, // 0x1D17  # LATIN SMALL LETTER BOTTOM HALF O
    LineBreakClass::ULB_AL, // 0x1D18  # LATIN LETTER SMALL CAPITAL P
    LineBreakClass::ULB_AL, // 0x1D19  # LATIN LETTER SMALL CAPITAL REVERSED R
    LineBreakClass::ULB_AL, // 0x1D1A  # LATIN LETTER SMALL CAPITAL TURNED R
    LineBreakClass::ULB_AL, // 0x1D1B  # LATIN LETTER SMALL CAPITAL T
    LineBreakClass::ULB_AL, // 0x1D1C  # LATIN LETTER SMALL CAPITAL U
    LineBreakClass::ULB_AL, // 0x1D1D  # LATIN SMALL LETTER SIDEWAYS U
    LineBreakClass::ULB_AL, // 0x1D1E  # LATIN SMALL LETTER SIDEWAYS DIAERESIZED U
    LineBreakClass::ULB_AL, // 0x1D1F  # LATIN SMALL LETTER SIDEWAYS TURNED M
    LineBreakClass::ULB_AL, // 0x1D20  # LATIN LETTER SMALL CAPITAL V
    LineBreakClass::ULB_AL, // 0x1D21  # LATIN LETTER SMALL CAPITAL W
    LineBreakClass::ULB_AL, // 0x1D22  # LATIN LETTER SMALL CAPITAL Z
    LineBreakClass::ULB_AL, // 0x1D23  # LATIN LETTER SMALL CAPITAL EZH
    LineBreakClass::ULB_AL, // 0x1D24  # LATIN LETTER VOICED LARYNGEAL SPIRANT
    LineBreakClass::ULB_AL, // 0x1D25  # LATIN LETTER AIN
    LineBreakClass::ULB_AL, // 0x1D26  # GREEK LETTER SMALL CAPITAL GAMMA
    LineBreakClass::ULB_AL, // 0x1D27  # GREEK LETTER SMALL CAPITAL LAMDA
    LineBreakClass::ULB_AL, // 0x1D28  # GREEK LETTER SMALL CAPITAL PI
    LineBreakClass::ULB_AL, // 0x1D29  # GREEK LETTER SMALL CAPITAL RHO
    LineBreakClass::ULB_AL, // 0x1D2A  # GREEK LETTER SMALL CAPITAL PSI
    LineBreakClass::ULB_AL, // 0x1D2B  # CYRILLIC LETTER SMALL CAPITAL EL
    LineBreakClass::ULB_AL, // 0x1D2C  # MODIFIER LETTER CAPITAL A
    LineBreakClass::ULB_AL, // 0x1D2D  # MODIFIER LETTER CAPITAL AE
    LineBreakClass::ULB_AL, // 0x1D2E  # MODIFIER LETTER CAPITAL B
    LineBreakClass::ULB_AL, // 0x1D2F  # MODIFIER LETTER CAPITAL BARRED B
    LineBreakClass::ULB_AL, // 0x1D30  # MODIFIER LETTER CAPITAL D
    LineBreakClass::ULB_AL, // 0x1D31  # MODIFIER LETTER CAPITAL E
    LineBreakClass::ULB_AL, // 0x1D32  # MODIFIER LETTER CAPITAL REVERSED E
    LineBreakClass::ULB_AL, // 0x1D33  # MODIFIER LETTER CAPITAL G
    LineBreakClass::ULB_AL, // 0x1D34  # MODIFIER LETTER CAPITAL H
    LineBreakClass::ULB_AL, // 0x1D35  # MODIFIER LETTER CAPITAL I
    LineBreakClass::ULB_AL, // 0x1D36  # MODIFIER LETTER CAPITAL J
    LineBreakClass::ULB_AL, // 0x1D37  # MODIFIER LETTER CAPITAL K
    LineBreakClass::ULB_AL, // 0x1D38  # MODIFIER LETTER CAPITAL L
    LineBreakClass::ULB_AL, // 0x1D39  # MODIFIER LETTER CAPITAL M
    LineBreakClass::ULB_AL, // 0x1D3A  # MODIFIER LETTER CAPITAL N
    LineBreakClass::ULB_AL, // 0x1D3B  # MODIFIER LETTER CAPITAL REVERSED N
    LineBreakClass::ULB_AL, // 0x1D3C  # MODIFIER LETTER CAPITAL O
    LineBreakClass::ULB_AL, // 0x1D3D  # MODIFIER LETTER CAPITAL OU
    LineBreakClass::ULB_AL, // 0x1D3E  # MODIFIER LETTER CAPITAL P
    LineBreakClass::ULB_AL, // 0x1D3F  # MODIFIER LETTER CAPITAL R
    LineBreakClass::ULB_AL, // 0x1D40  # MODIFIER LETTER CAPITAL T
    LineBreakClass::ULB_AL, // 0x1D41  # MODIFIER LETTER CAPITAL U
    LineBreakClass::ULB_AL, // 0x1D42  # MODIFIER LETTER CAPITAL W
    LineBreakClass::ULB_AL, // 0x1D43  # MODIFIER LETTER SMALL A
    LineBreakClass::ULB_AL, // 0x1D44  # MODIFIER LETTER SMALL TURNED A
    LineBreakClass::ULB_AL, // 0x1D45  # MODIFIER LETTER SMALL ALPHA
    LineBreakClass::ULB_AL, // 0x1D46  # MODIFIER LETTER SMALL TURNED AE
    LineBreakClass::ULB_AL, // 0x1D47  # MODIFIER LETTER SMALL B
    LineBreakClass::ULB_AL, // 0x1D48  # MODIFIER LETTER SMALL D
    LineBreakClass::ULB_AL, // 0x1D49  # MODIFIER LETTER SMALL E
    LineBreakClass::ULB_AL, // 0x1D4A  # MODIFIER LETTER SMALL SCHWA
    LineBreakClass::ULB_AL, // 0x1D4B  # MODIFIER LETTER SMALL OPEN E
    LineBreakClass::ULB_AL, // 0x1D4C  # MODIFIER LETTER SMALL TURNED OPEN E
    LineBreakClass::ULB_AL, // 0x1D4D  # MODIFIER LETTER SMALL G
    LineBreakClass::ULB_AL, // 0x1D4E  # MODIFIER LETTER SMALL TURNED I
    LineBreakClass::ULB_AL, // 0x1D4F  # MODIFIER LETTER SMALL K
    LineBreakClass::ULB_AL, // 0x1D50  # MODIFIER LETTER SMALL M
    LineBreakClass::ULB_AL, // 0x1D51  # MODIFIER LETTER SMALL ENG
    LineBreakClass::ULB_AL, // 0x1D52  # MODIFIER LETTER SMALL O
    LineBreakClass::ULB_AL, // 0x1D53  # MODIFIER LETTER SMALL OPEN O
    LineBreakClass::ULB_AL, // 0x1D54  # MODIFIER LETTER SMALL TOP HALF O
    LineBreakClass::ULB_AL, // 0x1D55  # MODIFIER LETTER SMALL BOTTOM HALF O
    LineBreakClass::ULB_AL, // 0x1D56  # MODIFIER LETTER SMALL P
    LineBreakClass::ULB_AL, // 0x1D57  # MODIFIER LETTER SMALL T
    LineBreakClass::ULB_AL, // 0x1D58  # MODIFIER LETTER SMALL U
    LineBreakClass::ULB_AL, // 0x1D59  # MODIFIER LETTER SMALL SIDEWAYS U
    LineBreakClass::ULB_AL, // 0x1D5A  # MODIFIER LETTER SMALL TURNED M
    LineBreakClass::ULB_AL, // 0x1D5B  # MODIFIER LETTER SMALL V
    LineBreakClass::ULB_AL, // 0x1D5C  # MODIFIER LETTER SMALL AIN
    LineBreakClass::ULB_AL, // 0x1D5D  # MODIFIER LETTER SMALL BETA
    LineBreakClass::ULB_AL, // 0x1D5E  # MODIFIER LETTER SMALL GREEK GAMMA
    LineBreakClass::ULB_AL, // 0x1D5F  # MODIFIER LETTER SMALL DELTA
    LineBreakClass::ULB_AL, // 0x1D60  # MODIFIER LETTER SMALL GREEK PHI
    LineBreakClass::ULB_AL, // 0x1D61  # MODIFIER LETTER SMALL CHI
    LineBreakClass::ULB_AL, // 0x1D62  # LATIN SUBSCRIPT SMALL LETTER I
    LineBreakClass::ULB_AL, // 0x1D63  # LATIN SUBSCRIPT SMALL LETTER R
    LineBreakClass::ULB_AL, // 0x1D64  # LATIN SUBSCRIPT SMALL LETTER U
    LineBreakClass::ULB_AL, // 0x1D65  # LATIN SUBSCRIPT SMALL LETTER V
    LineBreakClass::ULB_AL, // 0x1D66  # GREEK SUBSCRIPT SMALL LETTER BETA
    LineBreakClass::ULB_AL, // 0x1D67  # GREEK SUBSCRIPT SMALL LETTER GAMMA
    LineBreakClass::ULB_AL, // 0x1D68  # GREEK SUBSCRIPT SMALL LETTER RHO
    LineBreakClass::ULB_AL, // 0x1D69  # GREEK SUBSCRIPT SMALL LETTER PHI
    LineBreakClass::ULB_AL, // 0x1D6A  # GREEK SUBSCRIPT SMALL LETTER CHI
    LineBreakClass::ULB_AL, // 0x1D6B  # LATIN SMALL LETTER UE
    LineBreakClass::ULB_AL, // 0x1D6C  # LATIN SMALL LETTER B WITH MIDDLE TILDE
    LineBreakClass::ULB_AL, // 0x1D6D  # LATIN SMALL LETTER D WITH MIDDLE TILDE
    LineBreakClass::ULB_AL, // 0x1D6E  # LATIN SMALL LETTER F WITH MIDDLE TILDE
    LineBreakClass::ULB_AL, // 0x1D6F  # LATIN SMALL LETTER M WITH MIDDLE TILDE
    LineBreakClass::ULB_AL, // 0x1D70  # LATIN SMALL LETTER N WITH MIDDLE TILDE
    LineBreakClass::ULB_AL, // 0x1D71  # LATIN SMALL LETTER P WITH MIDDLE TILDE
    LineBreakClass::ULB_AL, // 0x1D72  # LATIN SMALL LETTER R WITH MIDDLE TILDE
    LineBreakClass::ULB_AL, // 0x1D73  # LATIN SMALL LETTER R WITH FISHHOOK AND MIDDLE TILDE
    LineBreakClass::ULB_AL, // 0x1D74  # LATIN SMALL LETTER S WITH MIDDLE TILDE
    LineBreakClass::ULB_AL, // 0x1D75  # LATIN SMALL LETTER T WITH MIDDLE TILDE
    LineBreakClass::ULB_AL, // 0x1D76  # LATIN SMALL LETTER Z WITH MIDDLE TILDE
    LineBreakClass::ULB_AL, // 0x1D77  # LATIN SMALL LETTER TURNED G
    LineBreakClass::ULB_AL, // 0x1D78  # MODIFIER LETTER CYRILLIC EN
    LineBreakClass::ULB_AL, // 0x1D79  # LATIN SMALL LETTER INSULAR G
    LineBreakClass::ULB_AL, // 0x1D7A  # LATIN SMALL LETTER TH WITH STRIKETHROUGH
    LineBreakClass::ULB_AL, // 0x1D7B  # LATIN SMALL CAPITAL LETTER I WITH STROKE
    LineBreakClass::ULB_AL, // 0x1D7C  # LATIN SMALL LETTER IOTA WITH STROKE
    LineBreakClass::ULB_AL, // 0x1D7D  # LATIN SMALL LETTER P WITH STROKE
    LineBreakClass::ULB_AL, // 0x1D7E  # LATIN SMALL CAPITAL LETTER U WITH STROKE
    LineBreakClass::ULB_AL, // 0x1D7F  # LATIN SMALL LETTER UPSILON WITH STROKE
    LineBreakClass::ULB_AL, // 0x1D80  # LATIN SMALL LETTER B WITH PALATAL HOOK
    LineBreakClass::ULB_AL, // 0x1D81  # LATIN SMALL LETTER D WITH PALATAL HOOK
    LineBreakClass::ULB_AL, // 0x1D82  # LATIN SMALL LETTER F WITH PALATAL HOOK
    LineBreakClass::ULB_AL, // 0x1D83  # LATIN SMALL LETTER G WITH PALATAL HOOK
    LineBreakClass::ULB_AL, // 0x1D84  # LATIN SMALL LETTER K WITH PALATAL HOOK
    LineBreakClass::ULB_AL, // 0x1D85  # LATIN SMALL LETTER L WITH PALATAL HOOK
    LineBreakClass::ULB_AL, // 0x1D86  # LATIN SMALL LETTER M WITH PALATAL HOOK
    LineBreakClass::ULB_AL, // 0x1D87  # LATIN SMALL LETTER N WITH PALATAL HOOK
    LineBreakClass::ULB_AL, // 0x1D88  # LATIN SMALL LETTER P WITH PALATAL HOOK
    LineBreakClass::ULB_AL, // 0x1D89  # LATIN SMALL LETTER R WITH PALATAL HOOK
    LineBreakClass::ULB_AL, // 0x1D8A  # LATIN SMALL LETTER S WITH PALATAL HOOK
    LineBreakClass::ULB_AL, // 0x1D8B  # LATIN SMALL LETTER ESH WITH PALATAL HOOK
    LineBreakClass::ULB_AL, // 0x1D8C  # LATIN SMALL LETTER V WITH PALATAL HOOK
    LineBreakClass::ULB_AL, // 0x1D8D  # LATIN SMALL LETTER X WITH PALATAL HOOK
    LineBreakClass::ULB_AL, // 0x1D8E  # LATIN SMALL LETTER Z WITH PALATAL HOOK
    LineBreakClass::ULB_AL, // 0x1D8F  # LATIN SMALL LETTER A WITH RETROFLEX HOOK
    LineBreakClass::ULB_AL, // 0x1D90  # LATIN SMALL LETTER ALPHA WITH RETROFLEX HOOK
    LineBreakClass::ULB_AL, // 0x1D91  # LATIN SMALL LETTER D WITH HOOK AND TAIL
    LineBreakClass::ULB_AL, // 0x1D92  # LATIN SMALL LETTER E WITH RETROFLEX HOOK
    LineBreakClass::ULB_AL, // 0x1D93  # LATIN SMALL LETTER OPEN E WITH RETROFLEX HOOK
    LineBreakClass::ULB_AL, // 0x1D94  # LATIN SMALL LETTER REVERSED OPEN E WITH RETROFLEX HOOK
    LineBreakClass::ULB_AL, // 0x1D95  # LATIN SMALL LETTER SCHWA WITH RETROFLEX HOOK
    LineBreakClass::ULB_AL, // 0x1D96  # LATIN SMALL LETTER I WITH RETROFLEX HOOK
    LineBreakClass::ULB_AL, // 0x1D97  # LATIN SMALL LETTER OPEN O WITH RETROFLEX HOOK
    LineBreakClass::ULB_AL, // 0x1D98  # LATIN SMALL LETTER ESH WITH RETROFLEX HOOK
    LineBreakClass::ULB_AL, // 0x1D99  # LATIN SMALL LETTER U WITH RETROFLEX HOOK
    LineBreakClass::ULB_AL, // 0x1D9A  # LATIN SMALL LETTER EZH WITH RETROFLEX HOOK
    LineBreakClass::ULB_AL, // 0x1D9B  # MODIFIER LETTER SMALL TURNED ALPHA
    LineBreakClass::ULB_AL, // 0x1D9C  # MODIFIER LETTER SMALL C
    LineBreakClass::ULB_AL, // 0x1D9D  # MODIFIER LETTER SMALL C WITH CURL
    LineBreakClass::ULB_AL, // 0x1D9E  # MODIFIER LETTER SMALL ETH
    LineBreakClass::ULB_AL, // 0x1D9F  # MODIFIER LETTER SMALL REVERSED OPEN E
    LineBreakClass::ULB_AL, // 0x1DA0  # MODIFIER LETTER SMALL F
    LineBreakClass::ULB_AL, // 0x1DA1  # MODIFIER LETTER SMALL DOTLESS J WITH STROKE
    LineBreakClass::ULB_AL, // 0x1DA2  # MODIFIER LETTER SMALL SCRIPT G
    LineBreakClass::ULB_AL, // 0x1DA3  # MODIFIER LETTER SMALL TURNED H
    LineBreakClass::ULB_AL, // 0x1DA4  # MODIFIER LETTER SMALL I WITH STROKE
    LineBreakClass::ULB_AL, // 0x1DA5  # MODIFIER LETTER SMALL IOTA
    LineBreakClass::ULB_AL, // 0x1DA6  # MODIFIER LETTER SMALL CAPITAL I
    LineBreakClass::ULB_AL, // 0x1DA7  # MODIFIER LETTER SMALL CAPITAL I WITH STROKE
    LineBreakClass::ULB_AL, // 0x1DA8  # MODIFIER LETTER SMALL J WITH CROSSED-TAIL
    LineBreakClass::ULB_AL, // 0x1DA9  # MODIFIER LETTER SMALL L WITH RETROFLEX HOOK
    LineBreakClass::ULB_AL, // 0x1DAA  # MODIFIER LETTER SMALL L WITH PALATAL HOOK
    LineBreakClass::ULB_AL, // 0x1DAB  # MODIFIER LETTER SMALL CAPITAL L
    LineBreakClass::ULB_AL, // 0x1DAC  # MODIFIER LETTER SMALL M WITH HOOK
    LineBreakClass::ULB_AL, // 0x1DAD  # MODIFIER LETTER SMALL TURNED M WITH LONG LEG
    LineBreakClass::ULB_AL, // 0x1DAE  # MODIFIER LETTER SMALL N WITH LEFT HOOK
    LineBreakClass::ULB_AL, // 0x1DAF  # MODIFIER LETTER SMALL N WITH RETROFLEX HOOK
    LineBreakClass::ULB_AL, // 0x1DB0  # MODIFIER LETTER SMALL CAPITAL N
    LineBreakClass::ULB_AL, // 0x1DB1  # MODIFIER LETTER SMALL BARRED O
    LineBreakClass::ULB_AL, // 0x1DB2  # MODIFIER LETTER SMALL PHI
    LineBreakClass::ULB_AL, // 0x1DB3  # MODIFIER LETTER SMALL S WITH HOOK
    LineBreakClass::ULB_AL, // 0x1DB4  # MODIFIER LETTER SMALL ESH
    LineBreakClass::ULB_AL, // 0x1DB5  # MODIFIER LETTER SMALL T WITH PALATAL HOOK
    LineBreakClass::ULB_AL, // 0x1DB6  # MODIFIER LETTER SMALL U BAR
    LineBreakClass::ULB_AL, // 0x1DB7  # MODIFIER LETTER SMALL UPSILON
    LineBreakClass::ULB_AL, // 0x1DB8  # MODIFIER LETTER SMALL CAPITAL U
    LineBreakClass::ULB_AL, // 0x1DB9  # MODIFIER LETTER SMALL V WITH HOOK
    LineBreakClass::ULB_AL, // 0x1DBA  # MODIFIER LETTER SMALL TURNED V
    LineBreakClass::ULB_AL, // 0x1DBB  # MODIFIER LETTER SMALL Z
    LineBreakClass::ULB_AL, // 0x1DBC  # MODIFIER LETTER SMALL Z WITH RETROFLEX HOOK
    LineBreakClass::ULB_AL, // 0x1DBD  # MODIFIER LETTER SMALL Z WITH CURL
    LineBreakClass::ULB_AL, // 0x1DBE  # MODIFIER LETTER SMALL EZH
    LineBreakClass::ULB_AL, // 0x1DBF  # MODIFIER LETTER SMALL THETA
    LineBreakClass::ULB_CM, // 0x1DC0  # COMBINING DOTTED GRAVE ACCENT
    LineBreakClass::ULB_CM, // 0x1DC1  # COMBINING DOTTED ACUTE ACCENT
    LineBreakClass::ULB_CM, // 0x1DC2  # COMBINING SNAKE BELOW
    LineBreakClass::ULB_CM, // 0x1DC3  # COMBINING SUSPENSION MARK
    LineBreakClass::ULB_CM, // 0x1DC4  # COMBINING MACRON-ACUTE
    LineBreakClass::ULB_CM, // 0x1DC5  # COMBINING GRAVE-MACRON
    LineBreakClass::ULB_CM, // 0x1DC6  # COMBINING MACRON-GRAVE
    LineBreakClass::ULB_CM, // 0x1DC7  # COMBINING ACUTE-MACRON
    LineBreakClass::ULB_CM, // 0x1DC8  # COMBINING GRAVE-ACUTE-GRAVE
    LineBreakClass::ULB_CM, // 0x1DC9  # COMBINING ACUTE-GRAVE-ACUTE
    LineBreakClass::ULB_CM, // 0x1DCA  # COMBINING LATIN SMALL LETTER R BELOW
    LineBreakClass::ULB_ID, // 0x1DCB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DCC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DCD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DCE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DCF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DD0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DD1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DD2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DD3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DD4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DD5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DD6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DD7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DD8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DD9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DDA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DDB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DDC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DDD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DDE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DDF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DE0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DE1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DE2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DE3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DE4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DE5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DE6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DE7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DE8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DE9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DEA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DEB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DEC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DED # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DEE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DEF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DF0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DF1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DF2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DF3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DF4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DF5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DF6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DF7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DF8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DF9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DFA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DFB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DFC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1DFD # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x1DFE  # COMBINING LEFT ARROWHEAD ABOVE
    LineBreakClass::ULB_CM, // 0x1DFF  # COMBINING RIGHT ARROWHEAD AND DOWN ARROWHEAD BELOW
    LineBreakClass::ULB_AL, // 0x1E00  # LATIN CAPITAL LETTER A WITH RING BELOW
    LineBreakClass::ULB_AL, // 0x1E01  # LATIN SMALL LETTER A WITH RING BELOW
    LineBreakClass::ULB_AL, // 0x1E02  # LATIN CAPITAL LETTER B WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E03  # LATIN SMALL LETTER B WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E04  # LATIN CAPITAL LETTER B WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E05  # LATIN SMALL LETTER B WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E06  # LATIN CAPITAL LETTER B WITH LINE BELOW
    LineBreakClass::ULB_AL, // 0x1E07  # LATIN SMALL LETTER B WITH LINE BELOW
    LineBreakClass::ULB_AL, // 0x1E08  # LATIN CAPITAL LETTER C WITH CEDILLA AND ACUTE
    LineBreakClass::ULB_AL, // 0x1E09  # LATIN SMALL LETTER C WITH CEDILLA AND ACUTE
    LineBreakClass::ULB_AL, // 0x1E0A  # LATIN CAPITAL LETTER D WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E0B  # LATIN SMALL LETTER D WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E0C  # LATIN CAPITAL LETTER D WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E0D  # LATIN SMALL LETTER D WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E0E  # LATIN CAPITAL LETTER D WITH LINE BELOW
    LineBreakClass::ULB_AL, // 0x1E0F  # LATIN SMALL LETTER D WITH LINE BELOW
    LineBreakClass::ULB_AL, // 0x1E10  # LATIN CAPITAL LETTER D WITH CEDILLA
    LineBreakClass::ULB_AL, // 0x1E11  # LATIN SMALL LETTER D WITH CEDILLA
    LineBreakClass::ULB_AL, // 0x1E12  # LATIN CAPITAL LETTER D WITH CIRCUMFLEX BELOW
    LineBreakClass::ULB_AL, // 0x1E13  # LATIN SMALL LETTER D WITH CIRCUMFLEX BELOW
    LineBreakClass::ULB_AL, // 0x1E14  # LATIN CAPITAL LETTER E WITH MACRON AND GRAVE
    LineBreakClass::ULB_AL, // 0x1E15  # LATIN SMALL LETTER E WITH MACRON AND GRAVE
    LineBreakClass::ULB_AL, // 0x1E16  # LATIN CAPITAL LETTER E WITH MACRON AND ACUTE
    LineBreakClass::ULB_AL, // 0x1E17  # LATIN SMALL LETTER E WITH MACRON AND ACUTE
    LineBreakClass::ULB_AL, // 0x1E18  # LATIN CAPITAL LETTER E WITH CIRCUMFLEX BELOW
    LineBreakClass::ULB_AL, // 0x1E19  # LATIN SMALL LETTER E WITH CIRCUMFLEX BELOW
    LineBreakClass::ULB_AL, // 0x1E1A  # LATIN CAPITAL LETTER E WITH TILDE BELOW
    LineBreakClass::ULB_AL, // 0x1E1B  # LATIN SMALL LETTER E WITH TILDE BELOW
    LineBreakClass::ULB_AL, // 0x1E1C  # LATIN CAPITAL LETTER E WITH CEDILLA AND BREVE
    LineBreakClass::ULB_AL, // 0x1E1D  # LATIN SMALL LETTER E WITH CEDILLA AND BREVE
    LineBreakClass::ULB_AL, // 0x1E1E  # LATIN CAPITAL LETTER F WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E1F  # LATIN SMALL LETTER F WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E20  # LATIN CAPITAL LETTER G WITH MACRON
    LineBreakClass::ULB_AL, // 0x1E21  # LATIN SMALL LETTER G WITH MACRON
    LineBreakClass::ULB_AL, // 0x1E22  # LATIN CAPITAL LETTER H WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E23  # LATIN SMALL LETTER H WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E24  # LATIN CAPITAL LETTER H WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E25  # LATIN SMALL LETTER H WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E26  # LATIN CAPITAL LETTER H WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x1E27  # LATIN SMALL LETTER H WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x1E28  # LATIN CAPITAL LETTER H WITH CEDILLA
    LineBreakClass::ULB_AL, // 0x1E29  # LATIN SMALL LETTER H WITH CEDILLA
    LineBreakClass::ULB_AL, // 0x1E2A  # LATIN CAPITAL LETTER H WITH BREVE BELOW
    LineBreakClass::ULB_AL, // 0x1E2B  # LATIN SMALL LETTER H WITH BREVE BELOW
    LineBreakClass::ULB_AL, // 0x1E2C  # LATIN CAPITAL LETTER I WITH TILDE BELOW
    LineBreakClass::ULB_AL, // 0x1E2D  # LATIN SMALL LETTER I WITH TILDE BELOW
    LineBreakClass::ULB_AL, // 0x1E2E  # LATIN CAPITAL LETTER I WITH DIAERESIS AND ACUTE
    LineBreakClass::ULB_AL, // 0x1E2F  # LATIN SMALL LETTER I WITH DIAERESIS AND ACUTE
    LineBreakClass::ULB_AL, // 0x1E30  # LATIN CAPITAL LETTER K WITH ACUTE
    LineBreakClass::ULB_AL, // 0x1E31  # LATIN SMALL LETTER K WITH ACUTE
    LineBreakClass::ULB_AL, // 0x1E32  # LATIN CAPITAL LETTER K WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E33  # LATIN SMALL LETTER K WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E34  # LATIN CAPITAL LETTER K WITH LINE BELOW
    LineBreakClass::ULB_AL, // 0x1E35  # LATIN SMALL LETTER K WITH LINE BELOW
    LineBreakClass::ULB_AL, // 0x1E36  # LATIN CAPITAL LETTER L WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E37  # LATIN SMALL LETTER L WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E38  # LATIN CAPITAL LETTER L WITH DOT BELOW AND MACRON
    LineBreakClass::ULB_AL, // 0x1E39  # LATIN SMALL LETTER L WITH DOT BELOW AND MACRON
    LineBreakClass::ULB_AL, // 0x1E3A  # LATIN CAPITAL LETTER L WITH LINE BELOW
    LineBreakClass::ULB_AL, // 0x1E3B  # LATIN SMALL LETTER L WITH LINE BELOW
    LineBreakClass::ULB_AL, // 0x1E3C  # LATIN CAPITAL LETTER L WITH CIRCUMFLEX BELOW
    LineBreakClass::ULB_AL, // 0x1E3D  # LATIN SMALL LETTER L WITH CIRCUMFLEX BELOW
    LineBreakClass::ULB_AL, // 0x1E3E  # LATIN CAPITAL LETTER M WITH ACUTE
    LineBreakClass::ULB_AL, // 0x1E3F  # LATIN SMALL LETTER M WITH ACUTE
    LineBreakClass::ULB_AL, // 0x1E40  # LATIN CAPITAL LETTER M WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E41  # LATIN SMALL LETTER M WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E42  # LATIN CAPITAL LETTER M WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E43  # LATIN SMALL LETTER M WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E44  # LATIN CAPITAL LETTER N WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E45  # LATIN SMALL LETTER N WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E46  # LATIN CAPITAL LETTER N WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E47  # LATIN SMALL LETTER N WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E48  # LATIN CAPITAL LETTER N WITH LINE BELOW
    LineBreakClass::ULB_AL, // 0x1E49  # LATIN SMALL LETTER N WITH LINE BELOW
    LineBreakClass::ULB_AL, // 0x1E4A  # LATIN CAPITAL LETTER N WITH CIRCUMFLEX BELOW
    LineBreakClass::ULB_AL, // 0x1E4B  # LATIN SMALL LETTER N WITH CIRCUMFLEX BELOW
    LineBreakClass::ULB_AL, // 0x1E4C  # LATIN CAPITAL LETTER O WITH TILDE AND ACUTE
    LineBreakClass::ULB_AL, // 0x1E4D  # LATIN SMALL LETTER O WITH TILDE AND ACUTE
    LineBreakClass::ULB_AL, // 0x1E4E  # LATIN CAPITAL LETTER O WITH TILDE AND DIAERESIS
    LineBreakClass::ULB_AL, // 0x1E4F  # LATIN SMALL LETTER O WITH TILDE AND DIAERESIS
    LineBreakClass::ULB_AL, // 0x1E50  # LATIN CAPITAL LETTER O WITH MACRON AND GRAVE
    LineBreakClass::ULB_AL, // 0x1E51  # LATIN SMALL LETTER O WITH MACRON AND GRAVE
    LineBreakClass::ULB_AL, // 0x1E52  # LATIN CAPITAL LETTER O WITH MACRON AND ACUTE
    LineBreakClass::ULB_AL, // 0x1E53  # LATIN SMALL LETTER O WITH MACRON AND ACUTE
    LineBreakClass::ULB_AL, // 0x1E54  # LATIN CAPITAL LETTER P WITH ACUTE
    LineBreakClass::ULB_AL, // 0x1E55  # LATIN SMALL LETTER P WITH ACUTE
    LineBreakClass::ULB_AL, // 0x1E56  # LATIN CAPITAL LETTER P WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E57  # LATIN SMALL LETTER P WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E58  # LATIN CAPITAL LETTER R WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E59  # LATIN SMALL LETTER R WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E5A  # LATIN CAPITAL LETTER R WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E5B  # LATIN SMALL LETTER R WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E5C  # LATIN CAPITAL LETTER R WITH DOT BELOW AND MACRON
    LineBreakClass::ULB_AL, // 0x1E5D  # LATIN SMALL LETTER R WITH DOT BELOW AND MACRON
    LineBreakClass::ULB_AL, // 0x1E5E  # LATIN CAPITAL LETTER R WITH LINE BELOW
    LineBreakClass::ULB_AL, // 0x1E5F  # LATIN SMALL LETTER R WITH LINE BELOW
    LineBreakClass::ULB_AL, // 0x1E60  # LATIN CAPITAL LETTER S WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E61  # LATIN SMALL LETTER S WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E62  # LATIN CAPITAL LETTER S WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E63  # LATIN SMALL LETTER S WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E64  # LATIN CAPITAL LETTER S WITH ACUTE AND DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E65  # LATIN SMALL LETTER S WITH ACUTE AND DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E66  # LATIN CAPITAL LETTER S WITH CARON AND DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E67  # LATIN SMALL LETTER S WITH CARON AND DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E68  # LATIN CAPITAL LETTER S WITH DOT BELOW AND DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E69  # LATIN SMALL LETTER S WITH DOT BELOW AND DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E6A  # LATIN CAPITAL LETTER T WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E6B  # LATIN SMALL LETTER T WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E6C  # LATIN CAPITAL LETTER T WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E6D  # LATIN SMALL LETTER T WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E6E  # LATIN CAPITAL LETTER T WITH LINE BELOW
    LineBreakClass::ULB_AL, // 0x1E6F  # LATIN SMALL LETTER T WITH LINE BELOW
    LineBreakClass::ULB_AL, // 0x1E70  # LATIN CAPITAL LETTER T WITH CIRCUMFLEX BELOW
    LineBreakClass::ULB_AL, // 0x1E71  # LATIN SMALL LETTER T WITH CIRCUMFLEX BELOW
    LineBreakClass::ULB_AL, // 0x1E72  # LATIN CAPITAL LETTER U WITH DIAERESIS BELOW
    LineBreakClass::ULB_AL, // 0x1E73  # LATIN SMALL LETTER U WITH DIAERESIS BELOW
    LineBreakClass::ULB_AL, // 0x1E74  # LATIN CAPITAL LETTER U WITH TILDE BELOW
    LineBreakClass::ULB_AL, // 0x1E75  # LATIN SMALL LETTER U WITH TILDE BELOW
    LineBreakClass::ULB_AL, // 0x1E76  # LATIN CAPITAL LETTER U WITH CIRCUMFLEX BELOW
    LineBreakClass::ULB_AL, // 0x1E77  # LATIN SMALL LETTER U WITH CIRCUMFLEX BELOW
    LineBreakClass::ULB_AL, // 0x1E78  # LATIN CAPITAL LETTER U WITH TILDE AND ACUTE
    LineBreakClass::ULB_AL, // 0x1E79  # LATIN SMALL LETTER U WITH TILDE AND ACUTE
    LineBreakClass::ULB_AL, // 0x1E7A  # LATIN CAPITAL LETTER U WITH MACRON AND DIAERESIS
    LineBreakClass::ULB_AL, // 0x1E7B  # LATIN SMALL LETTER U WITH MACRON AND DIAERESIS
    LineBreakClass::ULB_AL, // 0x1E7C  # LATIN CAPITAL LETTER V WITH TILDE
    LineBreakClass::ULB_AL, // 0x1E7D  # LATIN SMALL LETTER V WITH TILDE
    LineBreakClass::ULB_AL, // 0x1E7E  # LATIN CAPITAL LETTER V WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E7F  # LATIN SMALL LETTER V WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E80  # LATIN CAPITAL LETTER W WITH GRAVE
    LineBreakClass::ULB_AL, // 0x1E81  # LATIN SMALL LETTER W WITH GRAVE
    LineBreakClass::ULB_AL, // 0x1E82  # LATIN CAPITAL LETTER W WITH ACUTE
    LineBreakClass::ULB_AL, // 0x1E83  # LATIN SMALL LETTER W WITH ACUTE
    LineBreakClass::ULB_AL, // 0x1E84  # LATIN CAPITAL LETTER W WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x1E85  # LATIN SMALL LETTER W WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x1E86  # LATIN CAPITAL LETTER W WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E87  # LATIN SMALL LETTER W WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E88  # LATIN CAPITAL LETTER W WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E89  # LATIN SMALL LETTER W WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E8A  # LATIN CAPITAL LETTER X WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E8B  # LATIN SMALL LETTER X WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E8C  # LATIN CAPITAL LETTER X WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x1E8D  # LATIN SMALL LETTER X WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x1E8E  # LATIN CAPITAL LETTER Y WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E8F  # LATIN SMALL LETTER Y WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x1E90  # LATIN CAPITAL LETTER Z WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x1E91  # LATIN SMALL LETTER Z WITH CIRCUMFLEX
    LineBreakClass::ULB_AL, // 0x1E92  # LATIN CAPITAL LETTER Z WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E93  # LATIN SMALL LETTER Z WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1E94  # LATIN CAPITAL LETTER Z WITH LINE BELOW
    LineBreakClass::ULB_AL, // 0x1E95  # LATIN SMALL LETTER Z WITH LINE BELOW
    LineBreakClass::ULB_AL, // 0x1E96  # LATIN SMALL LETTER H WITH LINE BELOW
    LineBreakClass::ULB_AL, // 0x1E97  # LATIN SMALL LETTER T WITH DIAERESIS
    LineBreakClass::ULB_AL, // 0x1E98  # LATIN SMALL LETTER W WITH RING ABOVE
    LineBreakClass::ULB_AL, // 0x1E99  # LATIN SMALL LETTER Y WITH RING ABOVE
    LineBreakClass::ULB_AL, // 0x1E9A  # LATIN SMALL LETTER A WITH RIGHT HALF RING
    LineBreakClass::ULB_AL, // 0x1E9B  # LATIN SMALL LETTER LONG S WITH DOT ABOVE
    LineBreakClass::ULB_ID, // 0x1E9C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1E9D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1E9E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1E9F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1EA0  # LATIN CAPITAL LETTER A WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1EA1  # LATIN SMALL LETTER A WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1EA2  # LATIN CAPITAL LETTER A WITH HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1EA3  # LATIN SMALL LETTER A WITH HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1EA4  # LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND ACUTE
    LineBreakClass::ULB_AL, // 0x1EA5  # LATIN SMALL LETTER A WITH CIRCUMFLEX AND ACUTE
    LineBreakClass::ULB_AL, // 0x1EA6  # LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND GRAVE
    LineBreakClass::ULB_AL, // 0x1EA7  # LATIN SMALL LETTER A WITH CIRCUMFLEX AND GRAVE
    LineBreakClass::ULB_AL, // 0x1EA8  # LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1EA9  # LATIN SMALL LETTER A WITH CIRCUMFLEX AND HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1EAA  # LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND TILDE
    LineBreakClass::ULB_AL, // 0x1EAB  # LATIN SMALL LETTER A WITH CIRCUMFLEX AND TILDE
    LineBreakClass::ULB_AL, // 0x1EAC  # LATIN CAPITAL LETTER A WITH CIRCUMFLEX AND DOT BELOW
    LineBreakClass::ULB_AL, // 0x1EAD  # LATIN SMALL LETTER A WITH CIRCUMFLEX AND DOT BELOW
    LineBreakClass::ULB_AL, // 0x1EAE  # LATIN CAPITAL LETTER A WITH BREVE AND ACUTE
    LineBreakClass::ULB_AL, // 0x1EAF  # LATIN SMALL LETTER A WITH BREVE AND ACUTE
    LineBreakClass::ULB_AL, // 0x1EB0  # LATIN CAPITAL LETTER A WITH BREVE AND GRAVE
    LineBreakClass::ULB_AL, // 0x1EB1  # LATIN SMALL LETTER A WITH BREVE AND GRAVE
    LineBreakClass::ULB_AL, // 0x1EB2  # LATIN CAPITAL LETTER A WITH BREVE AND HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1EB3  # LATIN SMALL LETTER A WITH BREVE AND HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1EB4  # LATIN CAPITAL LETTER A WITH BREVE AND TILDE
    LineBreakClass::ULB_AL, // 0x1EB5  # LATIN SMALL LETTER A WITH BREVE AND TILDE
    LineBreakClass::ULB_AL, // 0x1EB6  # LATIN CAPITAL LETTER A WITH BREVE AND DOT BELOW
    LineBreakClass::ULB_AL, // 0x1EB7  # LATIN SMALL LETTER A WITH BREVE AND DOT BELOW
    LineBreakClass::ULB_AL, // 0x1EB8  # LATIN CAPITAL LETTER E WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1EB9  # LATIN SMALL LETTER E WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1EBA  # LATIN CAPITAL LETTER E WITH HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1EBB  # LATIN SMALL LETTER E WITH HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1EBC  # LATIN CAPITAL LETTER E WITH TILDE
    LineBreakClass::ULB_AL, // 0x1EBD  # LATIN SMALL LETTER E WITH TILDE
    LineBreakClass::ULB_AL, // 0x1EBE  # LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND ACUTE
    LineBreakClass::ULB_AL, // 0x1EBF  # LATIN SMALL LETTER E WITH CIRCUMFLEX AND ACUTE
    LineBreakClass::ULB_AL, // 0x1EC0  # LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND GRAVE
    LineBreakClass::ULB_AL, // 0x1EC1  # LATIN SMALL LETTER E WITH CIRCUMFLEX AND GRAVE
    LineBreakClass::ULB_AL, // 0x1EC2  # LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1EC3  # LATIN SMALL LETTER E WITH CIRCUMFLEX AND HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1EC4  # LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND TILDE
    LineBreakClass::ULB_AL, // 0x1EC5  # LATIN SMALL LETTER E WITH CIRCUMFLEX AND TILDE
    LineBreakClass::ULB_AL, // 0x1EC6  # LATIN CAPITAL LETTER E WITH CIRCUMFLEX AND DOT BELOW
    LineBreakClass::ULB_AL, // 0x1EC7  # LATIN SMALL LETTER E WITH CIRCUMFLEX AND DOT BELOW
    LineBreakClass::ULB_AL, // 0x1EC8  # LATIN CAPITAL LETTER I WITH HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1EC9  # LATIN SMALL LETTER I WITH HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1ECA  # LATIN CAPITAL LETTER I WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1ECB  # LATIN SMALL LETTER I WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1ECC  # LATIN CAPITAL LETTER O WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1ECD  # LATIN SMALL LETTER O WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1ECE  # LATIN CAPITAL LETTER O WITH HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1ECF  # LATIN SMALL LETTER O WITH HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1ED0  # LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND ACUTE
    LineBreakClass::ULB_AL, // 0x1ED1  # LATIN SMALL LETTER O WITH CIRCUMFLEX AND ACUTE
    LineBreakClass::ULB_AL, // 0x1ED2  # LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND GRAVE
    LineBreakClass::ULB_AL, // 0x1ED3  # LATIN SMALL LETTER O WITH CIRCUMFLEX AND GRAVE
    LineBreakClass::ULB_AL, // 0x1ED4  # LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1ED5  # LATIN SMALL LETTER O WITH CIRCUMFLEX AND HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1ED6  # LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND TILDE
    LineBreakClass::ULB_AL, // 0x1ED7  # LATIN SMALL LETTER O WITH CIRCUMFLEX AND TILDE
    LineBreakClass::ULB_AL, // 0x1ED8  # LATIN CAPITAL LETTER O WITH CIRCUMFLEX AND DOT BELOW
    LineBreakClass::ULB_AL, // 0x1ED9  # LATIN SMALL LETTER O WITH CIRCUMFLEX AND DOT BELOW
    LineBreakClass::ULB_AL, // 0x1EDA  # LATIN CAPITAL LETTER O WITH HORN AND ACUTE
    LineBreakClass::ULB_AL, // 0x1EDB  # LATIN SMALL LETTER O WITH HORN AND ACUTE
    LineBreakClass::ULB_AL, // 0x1EDC  # LATIN CAPITAL LETTER O WITH HORN AND GRAVE
    LineBreakClass::ULB_AL, // 0x1EDD  # LATIN SMALL LETTER O WITH HORN AND GRAVE
    LineBreakClass::ULB_AL, // 0x1EDE  # LATIN CAPITAL LETTER O WITH HORN AND HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1EDF  # LATIN SMALL LETTER O WITH HORN AND HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1EE0  # LATIN CAPITAL LETTER O WITH HORN AND TILDE
    LineBreakClass::ULB_AL, // 0x1EE1  # LATIN SMALL LETTER O WITH HORN AND TILDE
    LineBreakClass::ULB_AL, // 0x1EE2  # LATIN CAPITAL LETTER O WITH HORN AND DOT BELOW
    LineBreakClass::ULB_AL, // 0x1EE3  # LATIN SMALL LETTER O WITH HORN AND DOT BELOW
    LineBreakClass::ULB_AL, // 0x1EE4  # LATIN CAPITAL LETTER U WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1EE5  # LATIN SMALL LETTER U WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1EE6  # LATIN CAPITAL LETTER U WITH HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1EE7  # LATIN SMALL LETTER U WITH HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1EE8  # LATIN CAPITAL LETTER U WITH HORN AND ACUTE
    LineBreakClass::ULB_AL, // 0x1EE9  # LATIN SMALL LETTER U WITH HORN AND ACUTE
    LineBreakClass::ULB_AL, // 0x1EEA  # LATIN CAPITAL LETTER U WITH HORN AND GRAVE
    LineBreakClass::ULB_AL, // 0x1EEB  # LATIN SMALL LETTER U WITH HORN AND GRAVE
    LineBreakClass::ULB_AL, // 0x1EEC  # LATIN CAPITAL LETTER U WITH HORN AND HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1EED  # LATIN SMALL LETTER U WITH HORN AND HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1EEE  # LATIN CAPITAL LETTER U WITH HORN AND TILDE
    LineBreakClass::ULB_AL, // 0x1EEF  # LATIN SMALL LETTER U WITH HORN AND TILDE
    LineBreakClass::ULB_AL, // 0x1EF0  # LATIN CAPITAL LETTER U WITH HORN AND DOT BELOW
    LineBreakClass::ULB_AL, // 0x1EF1  # LATIN SMALL LETTER U WITH HORN AND DOT BELOW
    LineBreakClass::ULB_AL, // 0x1EF2  # LATIN CAPITAL LETTER Y WITH GRAVE
    LineBreakClass::ULB_AL, // 0x1EF3  # LATIN SMALL LETTER Y WITH GRAVE
    LineBreakClass::ULB_AL, // 0x1EF4  # LATIN CAPITAL LETTER Y WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1EF5  # LATIN SMALL LETTER Y WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x1EF6  # LATIN CAPITAL LETTER Y WITH HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1EF7  # LATIN SMALL LETTER Y WITH HOOK ABOVE
    LineBreakClass::ULB_AL, // 0x1EF8  # LATIN CAPITAL LETTER Y WITH TILDE
    LineBreakClass::ULB_AL, // 0x1EF9  # LATIN SMALL LETTER Y WITH TILDE
    LineBreakClass::ULB_ID, // 0x1EFA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1EFB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1EFC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1EFD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1EFE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1EFF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1F00  # GREEK SMALL LETTER ALPHA WITH PSILI
    LineBreakClass::ULB_AL, // 0x1F01  # GREEK SMALL LETTER ALPHA WITH DASIA
    LineBreakClass::ULB_AL, // 0x1F02  # GREEK SMALL LETTER ALPHA WITH PSILI AND VARIA
    LineBreakClass::ULB_AL, // 0x1F03  # GREEK SMALL LETTER ALPHA WITH DASIA AND VARIA
    LineBreakClass::ULB_AL, // 0x1F04  # GREEK SMALL LETTER ALPHA WITH PSILI AND OXIA
    LineBreakClass::ULB_AL, // 0x1F05  # GREEK SMALL LETTER ALPHA WITH DASIA AND OXIA
    LineBreakClass::ULB_AL, // 0x1F06  # GREEK SMALL LETTER ALPHA WITH PSILI AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1F07  # GREEK SMALL LETTER ALPHA WITH DASIA AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1F08  # GREEK CAPITAL LETTER ALPHA WITH PSILI
    LineBreakClass::ULB_AL, // 0x1F09  # GREEK CAPITAL LETTER ALPHA WITH DASIA
    LineBreakClass::ULB_AL, // 0x1F0A  # GREEK CAPITAL LETTER ALPHA WITH PSILI AND VARIA
    LineBreakClass::ULB_AL, // 0x1F0B  # GREEK CAPITAL LETTER ALPHA WITH DASIA AND VARIA
    LineBreakClass::ULB_AL, // 0x1F0C  # GREEK CAPITAL LETTER ALPHA WITH PSILI AND OXIA
    LineBreakClass::ULB_AL, // 0x1F0D  # GREEK CAPITAL LETTER ALPHA WITH DASIA AND OXIA
    LineBreakClass::ULB_AL, // 0x1F0E  # GREEK CAPITAL LETTER ALPHA WITH PSILI AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1F0F  # GREEK CAPITAL LETTER ALPHA WITH DASIA AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1F10  # GREEK SMALL LETTER EPSILON WITH PSILI
    LineBreakClass::ULB_AL, // 0x1F11  # GREEK SMALL LETTER EPSILON WITH DASIA
    LineBreakClass::ULB_AL, // 0x1F12  # GREEK SMALL LETTER EPSILON WITH PSILI AND VARIA
    LineBreakClass::ULB_AL, // 0x1F13  # GREEK SMALL LETTER EPSILON WITH DASIA AND VARIA
    LineBreakClass::ULB_AL, // 0x1F14  # GREEK SMALL LETTER EPSILON WITH PSILI AND OXIA
    LineBreakClass::ULB_AL, // 0x1F15  # GREEK SMALL LETTER EPSILON WITH DASIA AND OXIA
    LineBreakClass::ULB_ID, // 0x1F16 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1F17 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1F18  # GREEK CAPITAL LETTER EPSILON WITH PSILI
    LineBreakClass::ULB_AL, // 0x1F19  # GREEK CAPITAL LETTER EPSILON WITH DASIA
    LineBreakClass::ULB_AL, // 0x1F1A  # GREEK CAPITAL LETTER EPSILON WITH PSILI AND VARIA
    LineBreakClass::ULB_AL, // 0x1F1B  # GREEK CAPITAL LETTER EPSILON WITH DASIA AND VARIA
    LineBreakClass::ULB_AL, // 0x1F1C  # GREEK CAPITAL LETTER EPSILON WITH PSILI AND OXIA
    LineBreakClass::ULB_AL, // 0x1F1D  # GREEK CAPITAL LETTER EPSILON WITH DASIA AND OXIA
    LineBreakClass::ULB_ID, // 0x1F1E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1F1F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1F20  # GREEK SMALL LETTER ETA WITH PSILI
    LineBreakClass::ULB_AL, // 0x1F21  # GREEK SMALL LETTER ETA WITH DASIA
    LineBreakClass::ULB_AL, // 0x1F22  # GREEK SMALL LETTER ETA WITH PSILI AND VARIA
    LineBreakClass::ULB_AL, // 0x1F23  # GREEK SMALL LETTER ETA WITH DASIA AND VARIA
    LineBreakClass::ULB_AL, // 0x1F24  # GREEK SMALL LETTER ETA WITH PSILI AND OXIA
    LineBreakClass::ULB_AL, // 0x1F25  # GREEK SMALL LETTER ETA WITH DASIA AND OXIA
    LineBreakClass::ULB_AL, // 0x1F26  # GREEK SMALL LETTER ETA WITH PSILI AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1F27  # GREEK SMALL LETTER ETA WITH DASIA AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1F28  # GREEK CAPITAL LETTER ETA WITH PSILI
    LineBreakClass::ULB_AL, // 0x1F29  # GREEK CAPITAL LETTER ETA WITH DASIA
    LineBreakClass::ULB_AL, // 0x1F2A  # GREEK CAPITAL LETTER ETA WITH PSILI AND VARIA
    LineBreakClass::ULB_AL, // 0x1F2B  # GREEK CAPITAL LETTER ETA WITH DASIA AND VARIA
    LineBreakClass::ULB_AL, // 0x1F2C  # GREEK CAPITAL LETTER ETA WITH PSILI AND OXIA
    LineBreakClass::ULB_AL, // 0x1F2D  # GREEK CAPITAL LETTER ETA WITH DASIA AND OXIA
    LineBreakClass::ULB_AL, // 0x1F2E  # GREEK CAPITAL LETTER ETA WITH PSILI AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1F2F  # GREEK CAPITAL LETTER ETA WITH DASIA AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1F30  # GREEK SMALL LETTER IOTA WITH PSILI
    LineBreakClass::ULB_AL, // 0x1F31  # GREEK SMALL LETTER IOTA WITH DASIA
    LineBreakClass::ULB_AL, // 0x1F32  # GREEK SMALL LETTER IOTA WITH PSILI AND VARIA
    LineBreakClass::ULB_AL, // 0x1F33  # GREEK SMALL LETTER IOTA WITH DASIA AND VARIA
    LineBreakClass::ULB_AL, // 0x1F34  # GREEK SMALL LETTER IOTA WITH PSILI AND OXIA
    LineBreakClass::ULB_AL, // 0x1F35  # GREEK SMALL LETTER IOTA WITH DASIA AND OXIA
    LineBreakClass::ULB_AL, // 0x1F36  # GREEK SMALL LETTER IOTA WITH PSILI AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1F37  # GREEK SMALL LETTER IOTA WITH DASIA AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1F38  # GREEK CAPITAL LETTER IOTA WITH PSILI
    LineBreakClass::ULB_AL, // 0x1F39  # GREEK CAPITAL LETTER IOTA WITH DASIA
    LineBreakClass::ULB_AL, // 0x1F3A  # GREEK CAPITAL LETTER IOTA WITH PSILI AND VARIA
    LineBreakClass::ULB_AL, // 0x1F3B  # GREEK CAPITAL LETTER IOTA WITH DASIA AND VARIA
    LineBreakClass::ULB_AL, // 0x1F3C  # GREEK CAPITAL LETTER IOTA WITH PSILI AND OXIA
    LineBreakClass::ULB_AL, // 0x1F3D  # GREEK CAPITAL LETTER IOTA WITH DASIA AND OXIA
    LineBreakClass::ULB_AL, // 0x1F3E  # GREEK CAPITAL LETTER IOTA WITH PSILI AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1F3F  # GREEK CAPITAL LETTER IOTA WITH DASIA AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1F40  # GREEK SMALL LETTER OMICRON WITH PSILI
    LineBreakClass::ULB_AL, // 0x1F41  # GREEK SMALL LETTER OMICRON WITH DASIA
    LineBreakClass::ULB_AL, // 0x1F42  # GREEK SMALL LETTER OMICRON WITH PSILI AND VARIA
    LineBreakClass::ULB_AL, // 0x1F43  # GREEK SMALL LETTER OMICRON WITH DASIA AND VARIA
    LineBreakClass::ULB_AL, // 0x1F44  # GREEK SMALL LETTER OMICRON WITH PSILI AND OXIA
    LineBreakClass::ULB_AL, // 0x1F45  # GREEK SMALL LETTER OMICRON WITH DASIA AND OXIA
    LineBreakClass::ULB_ID, // 0x1F46 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1F47 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1F48  # GREEK CAPITAL LETTER OMICRON WITH PSILI
    LineBreakClass::ULB_AL, // 0x1F49  # GREEK CAPITAL LETTER OMICRON WITH DASIA
    LineBreakClass::ULB_AL, // 0x1F4A  # GREEK CAPITAL LETTER OMICRON WITH PSILI AND VARIA
    LineBreakClass::ULB_AL, // 0x1F4B  # GREEK CAPITAL LETTER OMICRON WITH DASIA AND VARIA
    LineBreakClass::ULB_AL, // 0x1F4C  # GREEK CAPITAL LETTER OMICRON WITH PSILI AND OXIA
    LineBreakClass::ULB_AL, // 0x1F4D  # GREEK CAPITAL LETTER OMICRON WITH DASIA AND OXIA
    LineBreakClass::ULB_ID, // 0x1F4E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1F4F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1F50  # GREEK SMALL LETTER UPSILON WITH PSILI
    LineBreakClass::ULB_AL, // 0x1F51  # GREEK SMALL LETTER UPSILON WITH DASIA
    LineBreakClass::ULB_AL, // 0x1F52  # GREEK SMALL LETTER UPSILON WITH PSILI AND VARIA
    LineBreakClass::ULB_AL, // 0x1F53  # GREEK SMALL LETTER UPSILON WITH DASIA AND VARIA
    LineBreakClass::ULB_AL, // 0x1F54  # GREEK SMALL LETTER UPSILON WITH PSILI AND OXIA
    LineBreakClass::ULB_AL, // 0x1F55  # GREEK SMALL LETTER UPSILON WITH DASIA AND OXIA
    LineBreakClass::ULB_AL, // 0x1F56  # GREEK SMALL LETTER UPSILON WITH PSILI AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1F57  # GREEK SMALL LETTER UPSILON WITH DASIA AND PERISPOMENI
    LineBreakClass::ULB_ID, // 0x1F58 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1F59  # GREEK CAPITAL LETTER UPSILON WITH DASIA
    LineBreakClass::ULB_ID, // 0x1F5A # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1F5B  # GREEK CAPITAL LETTER UPSILON WITH DASIA AND VARIA
    LineBreakClass::ULB_ID, // 0x1F5C # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1F5D  # GREEK CAPITAL LETTER UPSILON WITH DASIA AND OXIA
    LineBreakClass::ULB_ID, // 0x1F5E # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1F5F  # GREEK CAPITAL LETTER UPSILON WITH DASIA AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1F60  # GREEK SMALL LETTER OMEGA WITH PSILI
    LineBreakClass::ULB_AL, // 0x1F61  # GREEK SMALL LETTER OMEGA WITH DASIA
    LineBreakClass::ULB_AL, // 0x1F62  # GREEK SMALL LETTER OMEGA WITH PSILI AND VARIA
    LineBreakClass::ULB_AL, // 0x1F63  # GREEK SMALL LETTER OMEGA WITH DASIA AND VARIA
    LineBreakClass::ULB_AL, // 0x1F64  # GREEK SMALL LETTER OMEGA WITH PSILI AND OXIA
    LineBreakClass::ULB_AL, // 0x1F65  # GREEK SMALL LETTER OMEGA WITH DASIA AND OXIA
    LineBreakClass::ULB_AL, // 0x1F66  # GREEK SMALL LETTER OMEGA WITH PSILI AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1F67  # GREEK SMALL LETTER OMEGA WITH DASIA AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1F68  # GREEK CAPITAL LETTER OMEGA WITH PSILI
    LineBreakClass::ULB_AL, // 0x1F69  # GREEK CAPITAL LETTER OMEGA WITH DASIA
    LineBreakClass::ULB_AL, // 0x1F6A  # GREEK CAPITAL LETTER OMEGA WITH PSILI AND VARIA
    LineBreakClass::ULB_AL, // 0x1F6B  # GREEK CAPITAL LETTER OMEGA WITH DASIA AND VARIA
    LineBreakClass::ULB_AL, // 0x1F6C  # GREEK CAPITAL LETTER OMEGA WITH PSILI AND OXIA
    LineBreakClass::ULB_AL, // 0x1F6D  # GREEK CAPITAL LETTER OMEGA WITH DASIA AND OXIA
    LineBreakClass::ULB_AL, // 0x1F6E  # GREEK CAPITAL LETTER OMEGA WITH PSILI AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1F6F  # GREEK CAPITAL LETTER OMEGA WITH DASIA AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1F70  # GREEK SMALL LETTER ALPHA WITH VARIA
    LineBreakClass::ULB_AL, // 0x1F71  # GREEK SMALL LETTER ALPHA WITH OXIA
    LineBreakClass::ULB_AL, // 0x1F72  # GREEK SMALL LETTER EPSILON WITH VARIA
    LineBreakClass::ULB_AL, // 0x1F73  # GREEK SMALL LETTER EPSILON WITH OXIA
    LineBreakClass::ULB_AL, // 0x1F74  # GREEK SMALL LETTER ETA WITH VARIA
    LineBreakClass::ULB_AL, // 0x1F75  # GREEK SMALL LETTER ETA WITH OXIA
    LineBreakClass::ULB_AL, // 0x1F76  # GREEK SMALL LETTER IOTA WITH VARIA
    LineBreakClass::ULB_AL, // 0x1F77  # GREEK SMALL LETTER IOTA WITH OXIA
    LineBreakClass::ULB_AL, // 0x1F78  # GREEK SMALL LETTER OMICRON WITH VARIA
    LineBreakClass::ULB_AL, // 0x1F79  # GREEK SMALL LETTER OMICRON WITH OXIA
    LineBreakClass::ULB_AL, // 0x1F7A  # GREEK SMALL LETTER UPSILON WITH VARIA
    LineBreakClass::ULB_AL, // 0x1F7B  # GREEK SMALL LETTER UPSILON WITH OXIA
    LineBreakClass::ULB_AL, // 0x1F7C  # GREEK SMALL LETTER OMEGA WITH VARIA
    LineBreakClass::ULB_AL, // 0x1F7D  # GREEK SMALL LETTER OMEGA WITH OXIA
    LineBreakClass::ULB_ID, // 0x1F7E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1F7F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1F80  # GREEK SMALL LETTER ALPHA WITH PSILI AND YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F81  # GREEK SMALL LETTER ALPHA WITH DASIA AND YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F82  # GREEK SMALL LETTER ALPHA WITH PSILI AND VARIA AND
                            // YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F83  # GREEK SMALL LETTER ALPHA WITH DASIA AND VARIA AND
                            // YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F84  # GREEK SMALL LETTER ALPHA WITH PSILI AND OXIA AND
                            // YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F85  # GREEK SMALL LETTER ALPHA WITH DASIA AND OXIA AND
                            // YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F86  # GREEK SMALL LETTER ALPHA WITH PSILI AND PERISPOMENI AND
                            // YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F87  # GREEK SMALL LETTER ALPHA WITH DASIA AND PERISPOMENI AND
                            // YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F88  # GREEK CAPITAL LETTER ALPHA WITH PSILI AND PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F89  # GREEK CAPITAL LETTER ALPHA WITH DASIA AND PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F8A  # GREEK CAPITAL LETTER ALPHA WITH PSILI AND VARIA AND
                            // PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F8B  # GREEK CAPITAL LETTER ALPHA WITH DASIA AND VARIA AND
                            // PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F8C  # GREEK CAPITAL LETTER ALPHA WITH PSILI AND OXIA AND
                            // PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F8D  # GREEK CAPITAL LETTER ALPHA WITH DASIA AND OXIA AND
                            // PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F8E  # GREEK CAPITAL LETTER ALPHA WITH PSILI AND PERISPOMENI AND
                            // PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F8F  # GREEK CAPITAL LETTER ALPHA WITH DASIA AND PERISPOMENI AND
                            // PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F90  # GREEK SMALL LETTER ETA WITH PSILI AND YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F91  # GREEK SMALL LETTER ETA WITH DASIA AND YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F92  # GREEK SMALL LETTER ETA WITH PSILI AND VARIA AND
                            // YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F93  # GREEK SMALL LETTER ETA WITH DASIA AND VARIA AND
                            // YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F94  # GREEK SMALL LETTER ETA WITH PSILI AND OXIA AND
                            // YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F95  # GREEK SMALL LETTER ETA WITH DASIA AND OXIA AND
                            // YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F96  # GREEK SMALL LETTER ETA WITH PSILI AND PERISPOMENI AND
                            // YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F97  # GREEK SMALL LETTER ETA WITH DASIA AND PERISPOMENI AND
                            // YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F98  # GREEK CAPITAL LETTER ETA WITH PSILI AND PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F99  # GREEK CAPITAL LETTER ETA WITH DASIA AND PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F9A  # GREEK CAPITAL LETTER ETA WITH PSILI AND VARIA AND
                            // PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F9B  # GREEK CAPITAL LETTER ETA WITH DASIA AND VARIA AND
                            // PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F9C  # GREEK CAPITAL LETTER ETA WITH PSILI AND OXIA AND
                            // PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F9D  # GREEK CAPITAL LETTER ETA WITH DASIA AND OXIA AND
                            // PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F9E  # GREEK CAPITAL LETTER ETA WITH PSILI AND PERISPOMENI AND
                            // PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1F9F  # GREEK CAPITAL LETTER ETA WITH DASIA AND PERISPOMENI AND
                            // PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FA0  # GREEK SMALL LETTER OMEGA WITH PSILI AND YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FA1  # GREEK SMALL LETTER OMEGA WITH DASIA AND YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FA2  # GREEK SMALL LETTER OMEGA WITH PSILI AND VARIA AND
                            // YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FA3  # GREEK SMALL LETTER OMEGA WITH DASIA AND VARIA AND
                            // YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FA4  # GREEK SMALL LETTER OMEGA WITH PSILI AND OXIA AND
                            // YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FA5  # GREEK SMALL LETTER OMEGA WITH DASIA AND OXIA AND
                            // YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FA6  # GREEK SMALL LETTER OMEGA WITH PSILI AND PERISPOMENI AND
                            // YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FA7  # GREEK SMALL LETTER OMEGA WITH DASIA AND PERISPOMENI AND
                            // YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FA8  # GREEK CAPITAL LETTER OMEGA WITH PSILI AND PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FA9  # GREEK CAPITAL LETTER OMEGA WITH DASIA AND PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FAA  # GREEK CAPITAL LETTER OMEGA WITH PSILI AND VARIA AND
                            // PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FAB  # GREEK CAPITAL LETTER OMEGA WITH DASIA AND VARIA AND
                            // PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FAC  # GREEK CAPITAL LETTER OMEGA WITH PSILI AND OXIA AND
                            // PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FAD  # GREEK CAPITAL LETTER OMEGA WITH DASIA AND OXIA AND
                            // PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FAE  # GREEK CAPITAL LETTER OMEGA WITH PSILI AND PERISPOMENI AND
                            // PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FAF  # GREEK CAPITAL LETTER OMEGA WITH DASIA AND PERISPOMENI AND
                            // PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FB0  # GREEK SMALL LETTER ALPHA WITH VRACHY
    LineBreakClass::ULB_AL, // 0x1FB1  # GREEK SMALL LETTER ALPHA WITH MACRON
    LineBreakClass::ULB_AL, // 0x1FB2  # GREEK SMALL LETTER ALPHA WITH VARIA AND YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FB3  # GREEK SMALL LETTER ALPHA WITH YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FB4  # GREEK SMALL LETTER ALPHA WITH OXIA AND YPOGEGRAMMENI
    LineBreakClass::ULB_ID, // 0x1FB5 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1FB6  # GREEK SMALL LETTER ALPHA WITH PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1FB7  # GREEK SMALL LETTER ALPHA WITH PERISPOMENI AND
                            // YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FB8  # GREEK CAPITAL LETTER ALPHA WITH VRACHY
    LineBreakClass::ULB_AL, // 0x1FB9  # GREEK CAPITAL LETTER ALPHA WITH MACRON
    LineBreakClass::ULB_AL, // 0x1FBA  # GREEK CAPITAL LETTER ALPHA WITH VARIA
    LineBreakClass::ULB_AL, // 0x1FBB  # GREEK CAPITAL LETTER ALPHA WITH OXIA
    LineBreakClass::ULB_AL, // 0x1FBC  # GREEK CAPITAL LETTER ALPHA WITH PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FBD  # GREEK KORONIS
    LineBreakClass::ULB_AL, // 0x1FBE  # GREEK PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FBF  # GREEK PSILI
    LineBreakClass::ULB_AL, // 0x1FC0  # GREEK PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1FC1  # GREEK DIALYTIKA AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1FC2  # GREEK SMALL LETTER ETA WITH VARIA AND YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FC3  # GREEK SMALL LETTER ETA WITH YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FC4  # GREEK SMALL LETTER ETA WITH OXIA AND YPOGEGRAMMENI
    LineBreakClass::ULB_ID, // 0x1FC5 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1FC6  # GREEK SMALL LETTER ETA WITH PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1FC7  # GREEK SMALL LETTER ETA WITH PERISPOMENI AND YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FC8  # GREEK CAPITAL LETTER EPSILON WITH VARIA
    LineBreakClass::ULB_AL, // 0x1FC9  # GREEK CAPITAL LETTER EPSILON WITH OXIA
    LineBreakClass::ULB_AL, // 0x1FCA  # GREEK CAPITAL LETTER ETA WITH VARIA
    LineBreakClass::ULB_AL, // 0x1FCB  # GREEK CAPITAL LETTER ETA WITH OXIA
    LineBreakClass::ULB_AL, // 0x1FCC  # GREEK CAPITAL LETTER ETA WITH PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FCD  # GREEK PSILI AND VARIA
    LineBreakClass::ULB_AL, // 0x1FCE  # GREEK PSILI AND OXIA
    LineBreakClass::ULB_AL, // 0x1FCF  # GREEK PSILI AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1FD0  # GREEK SMALL LETTER IOTA WITH VRACHY
    LineBreakClass::ULB_AL, // 0x1FD1  # GREEK SMALL LETTER IOTA WITH MACRON
    LineBreakClass::ULB_AL, // 0x1FD2  # GREEK SMALL LETTER IOTA WITH DIALYTIKA AND VARIA
    LineBreakClass::ULB_AL, // 0x1FD3  # GREEK SMALL LETTER IOTA WITH DIALYTIKA AND OXIA
    LineBreakClass::ULB_ID, // 0x1FD4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1FD5 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1FD6  # GREEK SMALL LETTER IOTA WITH PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1FD7  # GREEK SMALL LETTER IOTA WITH DIALYTIKA AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1FD8  # GREEK CAPITAL LETTER IOTA WITH VRACHY
    LineBreakClass::ULB_AL, // 0x1FD9  # GREEK CAPITAL LETTER IOTA WITH MACRON
    LineBreakClass::ULB_AL, // 0x1FDA  # GREEK CAPITAL LETTER IOTA WITH VARIA
    LineBreakClass::ULB_AL, // 0x1FDB  # GREEK CAPITAL LETTER IOTA WITH OXIA
    LineBreakClass::ULB_ID, // 0x1FDC # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1FDD  # GREEK DASIA AND VARIA
    LineBreakClass::ULB_AL, // 0x1FDE  # GREEK DASIA AND OXIA
    LineBreakClass::ULB_AL, // 0x1FDF  # GREEK DASIA AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1FE0  # GREEK SMALL LETTER UPSILON WITH VRACHY
    LineBreakClass::ULB_AL, // 0x1FE1  # GREEK SMALL LETTER UPSILON WITH MACRON
    LineBreakClass::ULB_AL, // 0x1FE2  # GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND VARIA
    LineBreakClass::ULB_AL, // 0x1FE3  # GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND OXIA
    LineBreakClass::ULB_AL, // 0x1FE4  # GREEK SMALL LETTER RHO WITH PSILI
    LineBreakClass::ULB_AL, // 0x1FE5  # GREEK SMALL LETTER RHO WITH DASIA
    LineBreakClass::ULB_AL, // 0x1FE6  # GREEK SMALL LETTER UPSILON WITH PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1FE7  # GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1FE8  # GREEK CAPITAL LETTER UPSILON WITH VRACHY
    LineBreakClass::ULB_AL, // 0x1FE9  # GREEK CAPITAL LETTER UPSILON WITH MACRON
    LineBreakClass::ULB_AL, // 0x1FEA  # GREEK CAPITAL LETTER UPSILON WITH VARIA
    LineBreakClass::ULB_AL, // 0x1FEB  # GREEK CAPITAL LETTER UPSILON WITH OXIA
    LineBreakClass::ULB_AL, // 0x1FEC  # GREEK CAPITAL LETTER RHO WITH DASIA
    LineBreakClass::ULB_AL, // 0x1FED  # GREEK DIALYTIKA AND VARIA
    LineBreakClass::ULB_AL, // 0x1FEE  # GREEK DIALYTIKA AND OXIA
    LineBreakClass::ULB_AL, // 0x1FEF  # GREEK VARIA
    LineBreakClass::ULB_ID, // 0x1FF0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x1FF1 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1FF2  # GREEK SMALL LETTER OMEGA WITH VARIA AND YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FF3  # GREEK SMALL LETTER OMEGA WITH YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FF4  # GREEK SMALL LETTER OMEGA WITH OXIA AND YPOGEGRAMMENI
    LineBreakClass::ULB_ID, // 0x1FF5 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x1FF6  # GREEK SMALL LETTER OMEGA WITH PERISPOMENI
    LineBreakClass::ULB_AL, // 0x1FF7  # GREEK SMALL LETTER OMEGA WITH PERISPOMENI AND
                            // YPOGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FF8  # GREEK CAPITAL LETTER OMICRON WITH VARIA
    LineBreakClass::ULB_AL, // 0x1FF9  # GREEK CAPITAL LETTER OMICRON WITH OXIA
    LineBreakClass::ULB_AL, // 0x1FFA  # GREEK CAPITAL LETTER OMEGA WITH VARIA
    LineBreakClass::ULB_AL, // 0x1FFB  # GREEK CAPITAL LETTER OMEGA WITH OXIA
    LineBreakClass::ULB_AL, // 0x1FFC  # GREEK CAPITAL LETTER OMEGA WITH PROSGEGRAMMENI
    LineBreakClass::ULB_AL, // 0x1FFD  # GREEK OXIA
    LineBreakClass::ULB_AL, // 0x1FFE  # GREEK DASIA
    LineBreakClass::ULB_ID, // 0x1FFF # <UNDEFINED>
    LineBreakClass::ULB_BA, // 0x2000  # EN QUAD
    LineBreakClass::ULB_BA, // 0x2001  # EM QUAD
    LineBreakClass::ULB_BA, // 0x2002  # EN SPACE
    LineBreakClass::ULB_BA, // 0x2003  # EM SPACE
    LineBreakClass::ULB_BA, // 0x2004  # THREE-PER-EM SPACE
    LineBreakClass::ULB_BA, // 0x2005  # FOUR-PER-EM SPACE
    LineBreakClass::ULB_BA, // 0x2006  # SIX-PER-EM SPACE
    LineBreakClass::ULB_GL, // 0x2007  # FIGURE SPACE
    LineBreakClass::ULB_BA, // 0x2008  # PUNCTUATION SPACE
    LineBreakClass::ULB_BA, // 0x2009  # THIN SPACE
    LineBreakClass::ULB_BA, // 0x200A  # HAIR SPACE
    LineBreakClass::ULB_ZW, // 0x200B  # ZERO WIDTH SPACE
    LineBreakClass::ULB_CM, // 0x200C  # ZERO WIDTH NON-JOINER
    LineBreakClass::ULB_CM, // 0x200D  # ZERO WIDTH JOINER
    LineBreakClass::ULB_CM, // 0x200E  # LEFT-TO-RIGHT MARK
    LineBreakClass::ULB_CM, // 0x200F  # RIGHT-TO-LEFT MARK
    LineBreakClass::ULB_BA, // 0x2010  # HYPHEN
    LineBreakClass::ULB_GL, // 0x2011  # NON-BREAKING HYPHEN
    LineBreakClass::ULB_BA, // 0x2012  # FIGURE DASH
    LineBreakClass::ULB_BA, // 0x2013  # EN DASH
    LineBreakClass::ULB_B2, // 0x2014  # EM DASH
    LineBreakClass::ULB_AI, // 0x2015  # HORIZONTAL BAR
    LineBreakClass::ULB_AI, // 0x2016  # DOUBLE VERTICAL LINE
    LineBreakClass::ULB_AL, // 0x2017  # DOUBLE LOW LINE
    LineBreakClass::ULB_QU, // 0x2018  # LEFT SINGLE QUOTATION MARK
    LineBreakClass::ULB_QU, // 0x2019  # RIGHT SINGLE QUOTATION MARK
    LineBreakClass::ULB_OP, // 0x201A  # SINGLE LOW-9 QUOTATION MARK
    LineBreakClass::ULB_QU, // 0x201B  # SINGLE HIGH-REVERSED-9 QUOTATION MARK
    LineBreakClass::ULB_QU, // 0x201C  # LEFT DOUBLE QUOTATION MARK
    LineBreakClass::ULB_QU, // 0x201D  # RIGHT DOUBLE QUOTATION MARK
    LineBreakClass::ULB_OP, // 0x201E  # DOUBLE LOW-9 QUOTATION MARK
    LineBreakClass::ULB_QU, // 0x201F  # DOUBLE HIGH-REVERSED-9 QUOTATION MARK
    LineBreakClass::ULB_AI, // 0x2020  # DAGGER
    LineBreakClass::ULB_AI, // 0x2021  # DOUBLE DAGGER
    LineBreakClass::ULB_AL, // 0x2022  # BULLET
    LineBreakClass::ULB_AL, // 0x2023  # TRIANGULAR BULLET
    LineBreakClass::ULB_IN, // 0x2024  # ONE DOT LEADER
    LineBreakClass::ULB_IN, // 0x2025  # TWO DOT LEADER
    LineBreakClass::ULB_IN, // 0x2026  # HORIZONTAL ELLIPSIS
    LineBreakClass::ULB_BA, // 0x2027  # HYPHENATION POINT
    LineBreakClass::ULB_BK, // 0x2028  # LINE SEPARATOR
    LineBreakClass::ULB_BK, // 0x2029  # PARAGRAPH SEPARATOR
    LineBreakClass::ULB_CM, // 0x202A  # LEFT-TO-RIGHT EMBEDDING
    LineBreakClass::ULB_CM, // 0x202B  # RIGHT-TO-LEFT EMBEDDING
    LineBreakClass::ULB_CM, // 0x202C  # POP DIRECTIONAL FORMATTING
    LineBreakClass::ULB_CM, // 0x202D  # LEFT-TO-RIGHT OVERRIDE
    LineBreakClass::ULB_CM, // 0x202E  # RIGHT-TO-LEFT OVERRIDE
    LineBreakClass::ULB_GL, // 0x202F  # NARROW NO-BREAK SPACE
    LineBreakClass::ULB_PO, // 0x2030  # PER MILLE SIGN
    LineBreakClass::ULB_PO, // 0x2031  # PER TEN THOUSAND SIGN
    LineBreakClass::ULB_PO, // 0x2032  # PRIME
    LineBreakClass::ULB_PO, // 0x2033  # DOUBLE PRIME
    LineBreakClass::ULB_PO, // 0x2034  # TRIPLE PRIME
    LineBreakClass::ULB_PO, // 0x2035  # REVERSED PRIME
    LineBreakClass::ULB_PO, // 0x2036  # REVERSED DOUBLE PRIME
    LineBreakClass::ULB_PO, // 0x2037  # REVERSED TRIPLE PRIME
    LineBreakClass::ULB_AL, // 0x2038  # CARET
    LineBreakClass::ULB_QU, // 0x2039  # SINGLE LEFT-POINTING ANGLE QUOTATION MARK
    LineBreakClass::ULB_QU, // 0x203A  # SINGLE RIGHT-POINTING ANGLE QUOTATION MARK
    LineBreakClass::ULB_AI, // 0x203B  # REFERENCE MARK
    LineBreakClass::ULB_NS, // 0x203C  # DOUBLE EXCLAMATION MARK
    LineBreakClass::ULB_NS, // 0x203D  # INTERROBANG
    LineBreakClass::ULB_AL, // 0x203E  # OVERLINE
    LineBreakClass::ULB_AL, // 0x203F  # UNDERTIE
    LineBreakClass::ULB_AL, // 0x2040  # CHARACTER TIE
    LineBreakClass::ULB_AL, // 0x2041  # CARET INSERTION POINT
    LineBreakClass::ULB_AL, // 0x2042  # ASTERISM
    LineBreakClass::ULB_AL, // 0x2043  # HYPHEN BULLET
    LineBreakClass::ULB_IS, // 0x2044  # FRACTION SLASH
    LineBreakClass::ULB_OP, // 0x2045  # LEFT SQUARE BRACKET WITH QUILL
    LineBreakClass::ULB_CL, // 0x2046  # RIGHT SQUARE BRACKET WITH QUILL
    LineBreakClass::ULB_NS, // 0x2047  # DOUBLE QUESTION MARK
    LineBreakClass::ULB_NS, // 0x2048  # QUESTION EXCLAMATION MARK
    LineBreakClass::ULB_NS, // 0x2049  # EXCLAMATION QUESTION MARK
    LineBreakClass::ULB_AL, // 0x204A  # TIRONIAN SIGN ET
    LineBreakClass::ULB_AL, // 0x204B  # REVERSED PILCROW SIGN
    LineBreakClass::ULB_AL, // 0x204C  # BLACK LEFTWARDS BULLET
    LineBreakClass::ULB_AL, // 0x204D  # BLACK RIGHTWARDS BULLET
    LineBreakClass::ULB_AL, // 0x204E  # LOW ASTERISK
    LineBreakClass::ULB_AL, // 0x204F  # REVERSED SEMICOLON
    LineBreakClass::ULB_AL, // 0x2050  # CLOSE UP
    LineBreakClass::ULB_AL, // 0x2051  # TWO ASTERISKS ALIGNED VERTICALLY
    LineBreakClass::ULB_AL, // 0x2052  # COMMERCIAL MINUS SIGN
    LineBreakClass::ULB_AL, // 0x2053  # SWUNG DASH
    LineBreakClass::ULB_AL, // 0x2054  # INVERTED UNDERTIE
    LineBreakClass::ULB_AL, // 0x2055  # FLOWER PUNCTUATION MARK
    LineBreakClass::ULB_BA, // 0x2056  # THREE DOT PUNCTUATION
    LineBreakClass::ULB_AL, // 0x2057  # QUADRUPLE PRIME
    LineBreakClass::ULB_BA, // 0x2058  # FOUR DOT PUNCTUATION
    LineBreakClass::ULB_BA, // 0x2059  # FIVE DOT PUNCTUATION
    LineBreakClass::ULB_BA, // 0x205A  # TWO DOT PUNCTUATION
    LineBreakClass::ULB_BA, // 0x205B  # FOUR DOT MARK
    LineBreakClass::ULB_AL, // 0x205C  # DOTTED CROSS
    LineBreakClass::ULB_BA, // 0x205D  # TRICOLON
    LineBreakClass::ULB_BA, // 0x205E  # VERTICAL FOUR DOTS
    LineBreakClass::ULB_BA, // 0x205F  # MEDIUM MATHEMATICAL SPACE
    LineBreakClass::ULB_WJ, // 0x2060  # WORD JOINER
    LineBreakClass::ULB_AL, // 0x2061  # FUNCTION APPLICATION
    LineBreakClass::ULB_AL, // 0x2062  # INVISIBLE TIMES
    LineBreakClass::ULB_AL, // 0x2063  # INVISIBLE SEPARATOR
    LineBreakClass::ULB_ID, // 0x2064 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2065 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2066 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2067 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2068 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2069 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x206A  # INHIBIT SYMMETRIC SWAPPING
    LineBreakClass::ULB_CM, // 0x206B  # ACTIVATE SYMMETRIC SWAPPING
    LineBreakClass::ULB_CM, // 0x206C  # INHIBIT ARABIC FORM SHAPING
    LineBreakClass::ULB_CM, // 0x206D  # ACTIVATE ARABIC FORM SHAPING
    LineBreakClass::ULB_CM, // 0x206E  # NATIONAL DIGIT SHAPES
    LineBreakClass::ULB_CM, // 0x206F  # NOMINAL DIGIT SHAPES
    LineBreakClass::ULB_AL, // 0x2070  # SUPERSCRIPT ZERO
    LineBreakClass::ULB_AL, // 0x2071  # SUPERSCRIPT LATIN SMALL LETTER I
    LineBreakClass::ULB_ID, // 0x2072 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2073 # <UNDEFINED>
    LineBreakClass::ULB_AI, // 0x2074  # SUPERSCRIPT FOUR
    LineBreakClass::ULB_AL, // 0x2075  # SUPERSCRIPT FIVE
    LineBreakClass::ULB_AL, // 0x2076  # SUPERSCRIPT SIX
    LineBreakClass::ULB_AL, // 0x2077  # SUPERSCRIPT SEVEN
    LineBreakClass::ULB_AL, // 0x2078  # SUPERSCRIPT EIGHT
    LineBreakClass::ULB_AL, // 0x2079  # SUPERSCRIPT NINE
    LineBreakClass::ULB_AL, // 0x207A  # SUPERSCRIPT PLUS SIGN
    LineBreakClass::ULB_AL, // 0x207B  # SUPERSCRIPT MINUS
    LineBreakClass::ULB_AL, // 0x207C  # SUPERSCRIPT EQUALS SIGN
    LineBreakClass::ULB_OP, // 0x207D  # SUPERSCRIPT LEFT PARENTHESIS
    LineBreakClass::ULB_CL, // 0x207E  # SUPERSCRIPT RIGHT PARENTHESIS
    LineBreakClass::ULB_AI, // 0x207F  # SUPERSCRIPT LATIN SMALL LETTER N
    LineBreakClass::ULB_AL, // 0x2080  # SUBSCRIPT ZERO
    LineBreakClass::ULB_AI, // 0x2081  # SUBSCRIPT ONE
    LineBreakClass::ULB_AI, // 0x2082  # SUBSCRIPT TWO
    LineBreakClass::ULB_AI, // 0x2083  # SUBSCRIPT THREE
    LineBreakClass::ULB_AI, // 0x2084  # SUBSCRIPT FOUR
    LineBreakClass::ULB_AL, // 0x2085  # SUBSCRIPT FIVE
    LineBreakClass::ULB_AL, // 0x2086  # SUBSCRIPT SIX
    LineBreakClass::ULB_AL, // 0x2087  # SUBSCRIPT SEVEN
    LineBreakClass::ULB_AL, // 0x2088  # SUBSCRIPT EIGHT
    LineBreakClass::ULB_AL, // 0x2089  # SUBSCRIPT NINE
    LineBreakClass::ULB_AL, // 0x208A  # SUBSCRIPT PLUS SIGN
    LineBreakClass::ULB_AL, // 0x208B  # SUBSCRIPT MINUS
    LineBreakClass::ULB_AL, // 0x208C  # SUBSCRIPT EQUALS SIGN
    LineBreakClass::ULB_OP, // 0x208D  # SUBSCRIPT LEFT PARENTHESIS
    LineBreakClass::ULB_CL, // 0x208E  # SUBSCRIPT RIGHT PARENTHESIS
    LineBreakClass::ULB_ID, // 0x208F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2090  # LATIN SUBSCRIPT SMALL LETTER A
    LineBreakClass::ULB_AL, // 0x2091  # LATIN SUBSCRIPT SMALL LETTER E
    LineBreakClass::ULB_AL, // 0x2092  # LATIN SUBSCRIPT SMALL LETTER O
    LineBreakClass::ULB_AL, // 0x2093  # LATIN SUBSCRIPT SMALL LETTER X
    LineBreakClass::ULB_AL, // 0x2094  # LATIN SUBSCRIPT SMALL LETTER SCHWA
    LineBreakClass::ULB_ID, // 0x2095 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2096 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2097 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2098 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2099 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x209A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x209B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x209C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x209D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x209E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x209F # <UNDEFINED>
    LineBreakClass::ULB_PR, // 0x20A0  # EURO-CURRENCY SIGN
    LineBreakClass::ULB_PR, // 0x20A1  # COLON SIGN
    LineBreakClass::ULB_PR, // 0x20A2  # CRUZEIRO SIGN
    LineBreakClass::ULB_PR, // 0x20A3  # FRENCH FRANC SIGN
    LineBreakClass::ULB_PR, // 0x20A4  # LIRA SIGN
    LineBreakClass::ULB_PR, // 0x20A5  # MILL SIGN
    LineBreakClass::ULB_PR, // 0x20A6  # NAIRA SIGN
    LineBreakClass::ULB_PO, // 0x20A7  # PESETA SIGN
    LineBreakClass::ULB_PR, // 0x20A8  # RUPEE SIGN
    LineBreakClass::ULB_PR, // 0x20A9  # WON SIGN
    LineBreakClass::ULB_PR, // 0x20AA  # NEW SHEQEL SIGN
    LineBreakClass::ULB_PR, // 0x20AB  # DONG SIGN
    LineBreakClass::ULB_PR, // 0x20AC  # EURO SIGN
    LineBreakClass::ULB_PR, // 0x20AD  # KIP SIGN
    LineBreakClass::ULB_PR, // 0x20AE  # TUGRIK SIGN
    LineBreakClass::ULB_PR, // 0x20AF  # DRACHMA SIGN
    LineBreakClass::ULB_PR, // 0x20B0  # GERMAN PENNY SIGN
    LineBreakClass::ULB_PR, // 0x20B1  # PESO SIGN
    LineBreakClass::ULB_PR, // 0x20B2  # GUARANI SIGN
    LineBreakClass::ULB_PR, // 0x20B3  # AUSTRAL SIGN
    LineBreakClass::ULB_PR, // 0x20B4  # HRYVNIA SIGN
    LineBreakClass::ULB_PR, // 0x20B5  # CEDI SIGN
    LineBreakClass::ULB_ID, // 0x20B6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20B7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20B8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20B9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20BA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20BB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20BC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20BD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20BE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20BF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20C0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20C1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20C2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20C3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20C4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20C5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20C6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20C7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20C8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20C9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20CA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20CB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20CC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20CD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20CE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20CF # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x20D0  # COMBINING LEFT HARPOON ABOVE
    LineBreakClass::ULB_CM, // 0x20D1  # COMBINING RIGHT HARPOON ABOVE
    LineBreakClass::ULB_CM, // 0x20D2  # COMBINING LONG VERTICAL LINE OVERLAY
    LineBreakClass::ULB_CM, // 0x20D3  # COMBINING SHORT VERTICAL LINE OVERLAY
    LineBreakClass::ULB_CM, // 0x20D4  # COMBINING ANTICLOCKWISE ARROW ABOVE
    LineBreakClass::ULB_CM, // 0x20D5  # COMBINING CLOCKWISE ARROW ABOVE
    LineBreakClass::ULB_CM, // 0x20D6  # COMBINING LEFT ARROW ABOVE
    LineBreakClass::ULB_CM, // 0x20D7  # COMBINING RIGHT ARROW ABOVE
    LineBreakClass::ULB_CM, // 0x20D8  # COMBINING RING OVERLAY
    LineBreakClass::ULB_CM, // 0x20D9  # COMBINING CLOCKWISE RING OVERLAY
    LineBreakClass::ULB_CM, // 0x20DA  # COMBINING ANTICLOCKWISE RING OVERLAY
    LineBreakClass::ULB_CM, // 0x20DB  # COMBINING THREE DOTS ABOVE
    LineBreakClass::ULB_CM, // 0x20DC  # COMBINING FOUR DOTS ABOVE
    LineBreakClass::ULB_CM, // 0x20DD  # COMBINING ENCLOSING CIRCLE
    LineBreakClass::ULB_CM, // 0x20DE  # COMBINING ENCLOSING SQUARE
    LineBreakClass::ULB_CM, // 0x20DF  # COMBINING ENCLOSING DIAMOND
    LineBreakClass::ULB_CM, // 0x20E0  # COMBINING ENCLOSING CIRCLE BACKSLASH
    LineBreakClass::ULB_CM, // 0x20E1  # COMBINING LEFT RIGHT ARROW ABOVE
    LineBreakClass::ULB_CM, // 0x20E2  # COMBINING ENCLOSING SCREEN
    LineBreakClass::ULB_CM, // 0x20E3  # COMBINING ENCLOSING KEYCAP
    LineBreakClass::ULB_CM, // 0x20E4  # COMBINING ENCLOSING UPWARD POINTING TRIANGLE
    LineBreakClass::ULB_CM, // 0x20E5  # COMBINING REVERSE SOLIDUS OVERLAY
    LineBreakClass::ULB_CM, // 0x20E6  # COMBINING DOUBLE VERTICAL STROKE OVERLAY
    LineBreakClass::ULB_CM, // 0x20E7  # COMBINING ANNUITY SYMBOL
    LineBreakClass::ULB_CM, // 0x20E8  # COMBINING TRIPLE UNDERDOT
    LineBreakClass::ULB_CM, // 0x20E9  # COMBINING WIDE BRIDGE ABOVE
    LineBreakClass::ULB_CM, // 0x20EA  # COMBINING LEFTWARDS ARROW OVERLAY
    LineBreakClass::ULB_CM, // 0x20EB  # COMBINING LONG DOUBLE SOLIDUS OVERLAY
    LineBreakClass::ULB_CM, // 0x20EC  # COMBINING RIGHTWARDS HARPOON WITH BARB DOWNWARDS
    LineBreakClass::ULB_CM, // 0x20ED  # COMBINING LEFTWARDS HARPOON WITH BARB DOWNWARDS
    LineBreakClass::ULB_CM, // 0x20EE  # COMBINING LEFT ARROW BELOW
    LineBreakClass::ULB_CM, // 0x20EF  # COMBINING RIGHT ARROW BELOW
    LineBreakClass::ULB_ID, // 0x20F0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20F1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20F2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20F3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20F4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20F5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20F6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20F7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20F8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20F9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20FA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20FB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20FC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20FD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20FE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x20FF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2100  # ACCOUNT OF
    LineBreakClass::ULB_AL, // 0x2101  # ADDRESSED TO THE SUBJECT
    LineBreakClass::ULB_AL, // 0x2102  # DOUBLE-STRUCK CAPITAL C
    LineBreakClass::ULB_PO, // 0x2103  # DEGREE CELSIUS
    LineBreakClass::ULB_AL, // 0x2104  # CENTRE LINE SYMBOL
    LineBreakClass::ULB_AI, // 0x2105  # CARE OF
    LineBreakClass::ULB_AL, // 0x2106  # CADA UNA
    LineBreakClass::ULB_AL, // 0x2107  # EULER CONSTANT
    LineBreakClass::ULB_AL, // 0x2108  # SCRUPLE
    LineBreakClass::ULB_PO, // 0x2109  # DEGREE FAHRENHEIT
    LineBreakClass::ULB_AL, // 0x210A  # SCRIPT SMALL G
    LineBreakClass::ULB_AL, // 0x210B  # SCRIPT CAPITAL H
    LineBreakClass::ULB_AL, // 0x210C  # BLACK-LETTER CAPITAL H
    LineBreakClass::ULB_AL, // 0x210D  # DOUBLE-STRUCK CAPITAL H
    LineBreakClass::ULB_AL, // 0x210E  # PLANCK CONSTANT
    LineBreakClass::ULB_AL, // 0x210F  # PLANCK CONSTANT OVER TWO PI
    LineBreakClass::ULB_AL, // 0x2110  # SCRIPT CAPITAL I
    LineBreakClass::ULB_AL, // 0x2111  # BLACK-LETTER CAPITAL I
    LineBreakClass::ULB_AL, // 0x2112  # SCRIPT CAPITAL L
    LineBreakClass::ULB_AI, // 0x2113  # SCRIPT SMALL L
    LineBreakClass::ULB_AL, // 0x2114  # L B BAR SYMBOL
    LineBreakClass::ULB_AL, // 0x2115  # DOUBLE-STRUCK CAPITAL N
    LineBreakClass::ULB_PR, // 0x2116  # NUMERO SIGN
    LineBreakClass::ULB_AL, // 0x2117  # SOUND RECORDING COPYRIGHT
    LineBreakClass::ULB_AL, // 0x2118  # SCRIPT CAPITAL P
    LineBreakClass::ULB_AL, // 0x2119  # DOUBLE-STRUCK CAPITAL P
    LineBreakClass::ULB_AL, // 0x211A  # DOUBLE-STRUCK CAPITAL Q
    LineBreakClass::ULB_AL, // 0x211B  # SCRIPT CAPITAL R
    LineBreakClass::ULB_AL, // 0x211C  # BLACK-LETTER CAPITAL R
    LineBreakClass::ULB_AL, // 0x211D  # DOUBLE-STRUCK CAPITAL R
    LineBreakClass::ULB_AL, // 0x211E  # PRESCRIPTION TAKE
    LineBreakClass::ULB_AL, // 0x211F  # RESPONSE
    LineBreakClass::ULB_AL, // 0x2120  # SERVICE MARK
    LineBreakClass::ULB_AI, // 0x2121  # TELEPHONE SIGN
    LineBreakClass::ULB_AI, // 0x2122  # TRADE MARK SIGN
    LineBreakClass::ULB_AL, // 0x2123  # VERSICLE
    LineBreakClass::ULB_AL, // 0x2124  # DOUBLE-STRUCK CAPITAL Z
    LineBreakClass::ULB_AL, // 0x2125  # OUNCE SIGN
    LineBreakClass::ULB_AL, // 0x2126  # OHM SIGN
    LineBreakClass::ULB_AL, // 0x2127  # INVERTED OHM SIGN
    LineBreakClass::ULB_AL, // 0x2128  # BLACK-LETTER CAPITAL Z
    LineBreakClass::ULB_AL, // 0x2129  # TURNED GREEK SMALL LETTER IOTA
    LineBreakClass::ULB_AL, // 0x212A  # KELVIN SIGN
    LineBreakClass::ULB_AI, // 0x212B  # ANGSTROM SIGN
    LineBreakClass::ULB_AL, // 0x212C  # SCRIPT CAPITAL B
    LineBreakClass::ULB_AL, // 0x212D  # BLACK-LETTER CAPITAL C
    LineBreakClass::ULB_AL, // 0x212E  # ESTIMATED SYMBOL
    LineBreakClass::ULB_AL, // 0x212F  # SCRIPT SMALL E
    LineBreakClass::ULB_AL, // 0x2130  # SCRIPT CAPITAL E
    LineBreakClass::ULB_AL, // 0x2131  # SCRIPT CAPITAL F
    LineBreakClass::ULB_AL, // 0x2132  # TURNED CAPITAL F
    LineBreakClass::ULB_AL, // 0x2133  # SCRIPT CAPITAL M
    LineBreakClass::ULB_AL, // 0x2134  # SCRIPT SMALL O
    LineBreakClass::ULB_AL, // 0x2135  # ALEF SYMBOL
    LineBreakClass::ULB_AL, // 0x2136  # BET SYMBOL
    LineBreakClass::ULB_AL, // 0x2137  # GIMEL SYMBOL
    LineBreakClass::ULB_AL, // 0x2138  # DALET SYMBOL
    LineBreakClass::ULB_AL, // 0x2139  # INFORMATION SOURCE
    LineBreakClass::ULB_AL, // 0x213A  # ROTATED CAPITAL Q
    LineBreakClass::ULB_AL, // 0x213B  # FACSIMILE SIGN
    LineBreakClass::ULB_AL, // 0x213C  # DOUBLE-STRUCK SMALL PI
    LineBreakClass::ULB_AL, // 0x213D  # DOUBLE-STRUCK SMALL GAMMA
    LineBreakClass::ULB_AL, // 0x213E  # DOUBLE-STRUCK CAPITAL GAMMA
    LineBreakClass::ULB_AL, // 0x213F  # DOUBLE-STRUCK CAPITAL PI
    LineBreakClass::ULB_AL, // 0x2140  # DOUBLE-STRUCK N-ARY SUMMATION
    LineBreakClass::ULB_AL, // 0x2141  # TURNED SANS-SERIF CAPITAL G
    LineBreakClass::ULB_AL, // 0x2142  # TURNED SANS-SERIF CAPITAL L
    LineBreakClass::ULB_AL, // 0x2143  # REVERSED SANS-SERIF CAPITAL L
    LineBreakClass::ULB_AL, // 0x2144  # TURNED SANS-SERIF CAPITAL Y
    LineBreakClass::ULB_AL, // 0x2145  # DOUBLE-STRUCK ITALIC CAPITAL D
    LineBreakClass::ULB_AL, // 0x2146  # DOUBLE-STRUCK ITALIC SMALL D
    LineBreakClass::ULB_AL, // 0x2147  # DOUBLE-STRUCK ITALIC SMALL E
    LineBreakClass::ULB_AL, // 0x2148  # DOUBLE-STRUCK ITALIC SMALL I
    LineBreakClass::ULB_AL, // 0x2149  # DOUBLE-STRUCK ITALIC SMALL J
    LineBreakClass::ULB_AL, // 0x214A  # PROPERTY LINE
    LineBreakClass::ULB_AL, // 0x214B  # TURNED AMPERSAND
    LineBreakClass::ULB_AL, // 0x214C  # PER SIGN
    LineBreakClass::ULB_AL, // 0x214D  # AKTIESELSKAB
    LineBreakClass::ULB_AL, // 0x214E  # TURNED SMALL F
    LineBreakClass::ULB_ID, // 0x214F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2150 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2151 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2152 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2153  # VULGAR FRACTION ONE THIRD
    LineBreakClass::ULB_AI, // 0x2154  # VULGAR FRACTION TWO THIRDS
    LineBreakClass::ULB_AI, // 0x2155  # VULGAR FRACTION ONE FIFTH
    LineBreakClass::ULB_AL, // 0x2156  # VULGAR FRACTION TWO FIFTHS
    LineBreakClass::ULB_AL, // 0x2157  # VULGAR FRACTION THREE FIFTHS
    LineBreakClass::ULB_AL, // 0x2158  # VULGAR FRACTION FOUR FIFTHS
    LineBreakClass::ULB_AL, // 0x2159  # VULGAR FRACTION ONE SIXTH
    LineBreakClass::ULB_AL, // 0x215A  # VULGAR FRACTION FIVE SIXTHS
    LineBreakClass::ULB_AI, // 0x215B  # VULGAR FRACTION ONE EIGHTH
    LineBreakClass::ULB_AL, // 0x215C  # VULGAR FRACTION THREE EIGHTHS
    LineBreakClass::ULB_AL, // 0x215D  # VULGAR FRACTION FIVE EIGHTHS
    LineBreakClass::ULB_AI, // 0x215E  # VULGAR FRACTION SEVEN EIGHTHS
    LineBreakClass::ULB_AL, // 0x215F  # FRACTION NUMERATOR ONE
    LineBreakClass::ULB_AI, // 0x2160  # ROMAN NUMERAL ONE
    LineBreakClass::ULB_AI, // 0x2161  # ROMAN NUMERAL TWO
    LineBreakClass::ULB_AI, // 0x2162  # ROMAN NUMERAL THREE
    LineBreakClass::ULB_AI, // 0x2163  # ROMAN NUMERAL FOUR
    LineBreakClass::ULB_AI, // 0x2164  # ROMAN NUMERAL FIVE
    LineBreakClass::ULB_AI, // 0x2165  # ROMAN NUMERAL SIX
    LineBreakClass::ULB_AI, // 0x2166  # ROMAN NUMERAL SEVEN
    LineBreakClass::ULB_AI, // 0x2167  # ROMAN NUMERAL EIGHT
    LineBreakClass::ULB_AI, // 0x2168  # ROMAN NUMERAL NINE
    LineBreakClass::ULB_AI, // 0x2169  # ROMAN NUMERAL TEN
    LineBreakClass::ULB_AI, // 0x216A  # ROMAN NUMERAL ELEVEN
    LineBreakClass::ULB_AI, // 0x216B  # ROMAN NUMERAL TWELVE
    LineBreakClass::ULB_AL, // 0x216C  # ROMAN NUMERAL FIFTY
    LineBreakClass::ULB_AL, // 0x216D  # ROMAN NUMERAL ONE HUNDRED
    LineBreakClass::ULB_AL, // 0x216E  # ROMAN NUMERAL FIVE HUNDRED
    LineBreakClass::ULB_AL, // 0x216F  # ROMAN NUMERAL ONE THOUSAND
    LineBreakClass::ULB_AI, // 0x2170  # SMALL ROMAN NUMERAL ONE
    LineBreakClass::ULB_AI, // 0x2171  # SMALL ROMAN NUMERAL TWO
    LineBreakClass::ULB_AI, // 0x2172  # SMALL ROMAN NUMERAL THREE
    LineBreakClass::ULB_AI, // 0x2173  # SMALL ROMAN NUMERAL FOUR
    LineBreakClass::ULB_AI, // 0x2174  # SMALL ROMAN NUMERAL FIVE
    LineBreakClass::ULB_AI, // 0x2175  # SMALL ROMAN NUMERAL SIX
    LineBreakClass::ULB_AI, // 0x2176  # SMALL ROMAN NUMERAL SEVEN
    LineBreakClass::ULB_AI, // 0x2177  # SMALL ROMAN NUMERAL EIGHT
    LineBreakClass::ULB_AI, // 0x2178  # SMALL ROMAN NUMERAL NINE
    LineBreakClass::ULB_AI, // 0x2179  # SMALL ROMAN NUMERAL TEN
    LineBreakClass::ULB_AL, // 0x217A  # SMALL ROMAN NUMERAL ELEVEN
    LineBreakClass::ULB_AL, // 0x217B  # SMALL ROMAN NUMERAL TWELVE
    LineBreakClass::ULB_AL, // 0x217C  # SMALL ROMAN NUMERAL FIFTY
    LineBreakClass::ULB_AL, // 0x217D  # SMALL ROMAN NUMERAL ONE HUNDRED
    LineBreakClass::ULB_AL, // 0x217E  # SMALL ROMAN NUMERAL FIVE HUNDRED
    LineBreakClass::ULB_AL, // 0x217F  # SMALL ROMAN NUMERAL ONE THOUSAND
    LineBreakClass::ULB_AL, // 0x2180  # ROMAN NUMERAL ONE THOUSAND C D
    LineBreakClass::ULB_AL, // 0x2181  # ROMAN NUMERAL FIVE THOUSAND
    LineBreakClass::ULB_AL, // 0x2182  # ROMAN NUMERAL TEN THOUSAND
    LineBreakClass::ULB_AL, // 0x2183  # ROMAN NUMERAL REVERSED ONE HUNDRED
    LineBreakClass::ULB_AL, // 0x2184  # LATIN SMALL LETTER REVERSED C
    LineBreakClass::ULB_ID, // 0x2185 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2186 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2187 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2188 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2189 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x218A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x218B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x218C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x218D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x218E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x218F # <UNDEFINED>
    LineBreakClass::ULB_AI, // 0x2190  # LEFTWARDS ARROW
    LineBreakClass::ULB_AI, // 0x2191  # UPWARDS ARROW
    LineBreakClass::ULB_AI, // 0x2192  # RIGHTWARDS ARROW
    LineBreakClass::ULB_AI, // 0x2193  # DOWNWARDS ARROW
    LineBreakClass::ULB_AI, // 0x2194  # LEFT RIGHT ARROW
    LineBreakClass::ULB_AI, // 0x2195  # UP DOWN ARROW
    LineBreakClass::ULB_AI, // 0x2196  # NORTH WEST ARROW
    LineBreakClass::ULB_AI, // 0x2197  # NORTH EAST ARROW
    LineBreakClass::ULB_AI, // 0x2198  # SOUTH EAST ARROW
    LineBreakClass::ULB_AI, // 0x2199  # SOUTH WEST ARROW
    LineBreakClass::ULB_AL, // 0x219A  # LEFTWARDS ARROW WITH STROKE
    LineBreakClass::ULB_AL, // 0x219B  # RIGHTWARDS ARROW WITH STROKE
    LineBreakClass::ULB_AL, // 0x219C  # LEFTWARDS WAVE ARROW
    LineBreakClass::ULB_AL, // 0x219D  # RIGHTWARDS WAVE ARROW
    LineBreakClass::ULB_AL, // 0x219E  # LEFTWARDS TWO HEADED ARROW
    LineBreakClass::ULB_AL, // 0x219F  # UPWARDS TWO HEADED ARROW
    LineBreakClass::ULB_AL, // 0x21A0  # RIGHTWARDS TWO HEADED ARROW
    LineBreakClass::ULB_AL, // 0x21A1  # DOWNWARDS TWO HEADED ARROW
    LineBreakClass::ULB_AL, // 0x21A2  # LEFTWARDS ARROW WITH TAIL
    LineBreakClass::ULB_AL, // 0x21A3  # RIGHTWARDS ARROW WITH TAIL
    LineBreakClass::ULB_AL, // 0x21A4  # LEFTWARDS ARROW FROM BAR
    LineBreakClass::ULB_AL, // 0x21A5  # UPWARDS ARROW FROM BAR
    LineBreakClass::ULB_AL, // 0x21A6  # RIGHTWARDS ARROW FROM BAR
    LineBreakClass::ULB_AL, // 0x21A7  # DOWNWARDS ARROW FROM BAR
    LineBreakClass::ULB_AL, // 0x21A8  # UP DOWN ARROW WITH BASE
    LineBreakClass::ULB_AL, // 0x21A9  # LEFTWARDS ARROW WITH HOOK
    LineBreakClass::ULB_AL, // 0x21AA  # RIGHTWARDS ARROW WITH HOOK
    LineBreakClass::ULB_AL, // 0x21AB  # LEFTWARDS ARROW WITH LOOP
    LineBreakClass::ULB_AL, // 0x21AC  # RIGHTWARDS ARROW WITH LOOP
    LineBreakClass::ULB_AL, // 0x21AD  # LEFT RIGHT WAVE ARROW
    LineBreakClass::ULB_AL, // 0x21AE  # LEFT RIGHT ARROW WITH STROKE
    LineBreakClass::ULB_AL, // 0x21AF  # DOWNWARDS ZIGZAG ARROW
    LineBreakClass::ULB_AL, // 0x21B0  # UPWARDS ARROW WITH TIP LEFTWARDS
    LineBreakClass::ULB_AL, // 0x21B1  # UPWARDS ARROW WITH TIP RIGHTWARDS
    LineBreakClass::ULB_AL, // 0x21B2  # DOWNWARDS ARROW WITH TIP LEFTWARDS
    LineBreakClass::ULB_AL, // 0x21B3  # DOWNWARDS ARROW WITH TIP RIGHTWARDS
    LineBreakClass::ULB_AL, // 0x21B4  # RIGHTWARDS ARROW WITH CORNER DOWNWARDS
    LineBreakClass::ULB_AL, // 0x21B5  # DOWNWARDS ARROW WITH CORNER LEFTWARDS
    LineBreakClass::ULB_AL, // 0x21B6  # ANTICLOCKWISE TOP SEMICIRCLE ARROW
    LineBreakClass::ULB_AL, // 0x21B7  # CLOCKWISE TOP SEMICIRCLE ARROW
    LineBreakClass::ULB_AL, // 0x21B8  # NORTH WEST ARROW TO LONG BAR
    LineBreakClass::ULB_AL, // 0x21B9  # LEFTWARDS ARROW TO BAR OVER RIGHTWARDS ARROW TO BAR
    LineBreakClass::ULB_AL, // 0x21BA  # ANTICLOCKWISE OPEN CIRCLE ARROW
    LineBreakClass::ULB_AL, // 0x21BB  # CLOCKWISE OPEN CIRCLE ARROW
    LineBreakClass::ULB_AL, // 0x21BC  # LEFTWARDS HARPOON WITH BARB UPWARDS
    LineBreakClass::ULB_AL, // 0x21BD  # LEFTWARDS HARPOON WITH BARB DOWNWARDS
    LineBreakClass::ULB_AL, // 0x21BE  # UPWARDS HARPOON WITH BARB RIGHTWARDS
    LineBreakClass::ULB_AL, // 0x21BF  # UPWARDS HARPOON WITH BARB LEFTWARDS
    LineBreakClass::ULB_AL, // 0x21C0  # RIGHTWARDS HARPOON WITH BARB UPWARDS
    LineBreakClass::ULB_AL, // 0x21C1  # RIGHTWARDS HARPOON WITH BARB DOWNWARDS
    LineBreakClass::ULB_AL, // 0x21C2  # DOWNWARDS HARPOON WITH BARB RIGHTWARDS
    LineBreakClass::ULB_AL, // 0x21C3  # DOWNWARDS HARPOON WITH BARB LEFTWARDS
    LineBreakClass::ULB_AL, // 0x21C4  # RIGHTWARDS ARROW OVER LEFTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x21C5  # UPWARDS ARROW LEFTWARDS OF DOWNWARDS ARROW
    LineBreakClass::ULB_AL, // 0x21C6  # LEFTWARDS ARROW OVER RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x21C7  # LEFTWARDS PAIRED ARROWS
    LineBreakClass::ULB_AL, // 0x21C8  # UPWARDS PAIRED ARROWS
    LineBreakClass::ULB_AL, // 0x21C9  # RIGHTWARDS PAIRED ARROWS
    LineBreakClass::ULB_AL, // 0x21CA  # DOWNWARDS PAIRED ARROWS
    LineBreakClass::ULB_AL, // 0x21CB  # LEFTWARDS HARPOON OVER RIGHTWARDS HARPOON
    LineBreakClass::ULB_AL, // 0x21CC  # RIGHTWARDS HARPOON OVER LEFTWARDS HARPOON
    LineBreakClass::ULB_AL, // 0x21CD  # LEFTWARDS DOUBLE ARROW WITH STROKE
    LineBreakClass::ULB_AL, // 0x21CE  # LEFT RIGHT DOUBLE ARROW WITH STROKE
    LineBreakClass::ULB_AL, // 0x21CF  # RIGHTWARDS DOUBLE ARROW WITH STROKE
    LineBreakClass::ULB_AL, // 0x21D0  # LEFTWARDS DOUBLE ARROW
    LineBreakClass::ULB_AL, // 0x21D1  # UPWARDS DOUBLE ARROW
    LineBreakClass::ULB_AI, // 0x21D2  # RIGHTWARDS DOUBLE ARROW
    LineBreakClass::ULB_AL, // 0x21D3  # DOWNWARDS DOUBLE ARROW
    LineBreakClass::ULB_AI, // 0x21D4  # LEFT RIGHT DOUBLE ARROW
    LineBreakClass::ULB_AL, // 0x21D5  # UP DOWN DOUBLE ARROW
    LineBreakClass::ULB_AL, // 0x21D6  # NORTH WEST DOUBLE ARROW
    LineBreakClass::ULB_AL, // 0x21D7  # NORTH EAST DOUBLE ARROW
    LineBreakClass::ULB_AL, // 0x21D8  # SOUTH EAST DOUBLE ARROW
    LineBreakClass::ULB_AL, // 0x21D9  # SOUTH WEST DOUBLE ARROW
    LineBreakClass::ULB_AL, // 0x21DA  # LEFTWARDS TRIPLE ARROW
    LineBreakClass::ULB_AL, // 0x21DB  # RIGHTWARDS TRIPLE ARROW
    LineBreakClass::ULB_AL, // 0x21DC  # LEFTWARDS SQUIGGLE ARROW
    LineBreakClass::ULB_AL, // 0x21DD  # RIGHTWARDS SQUIGGLE ARROW
    LineBreakClass::ULB_AL, // 0x21DE  # UPWARDS ARROW WITH DOUBLE STROKE
    LineBreakClass::ULB_AL, // 0x21DF  # DOWNWARDS ARROW WITH DOUBLE STROKE
    LineBreakClass::ULB_AL, // 0x21E0  # LEFTWARDS DASHED ARROW
    LineBreakClass::ULB_AL, // 0x21E1  # UPWARDS DASHED ARROW
    LineBreakClass::ULB_AL, // 0x21E2  # RIGHTWARDS DASHED ARROW
    LineBreakClass::ULB_AL, // 0x21E3  # DOWNWARDS DASHED ARROW
    LineBreakClass::ULB_AL, // 0x21E4  # LEFTWARDS ARROW TO BAR
    LineBreakClass::ULB_AL, // 0x21E5  # RIGHTWARDS ARROW TO BAR
    LineBreakClass::ULB_AL, // 0x21E6  # LEFTWARDS WHITE ARROW
    LineBreakClass::ULB_AL, // 0x21E7  # UPWARDS WHITE ARROW
    LineBreakClass::ULB_AL, // 0x21E8  # RIGHTWARDS WHITE ARROW
    LineBreakClass::ULB_AL, // 0x21E9  # DOWNWARDS WHITE ARROW
    LineBreakClass::ULB_AL, // 0x21EA  # UPWARDS WHITE ARROW FROM BAR
    LineBreakClass::ULB_AL, // 0x21EB  # UPWARDS WHITE ARROW ON PEDESTAL
    LineBreakClass::ULB_AL, // 0x21EC  # UPWARDS WHITE ARROW ON PEDESTAL WITH HORIZONTAL BAR
    LineBreakClass::ULB_AL, // 0x21ED  # UPWARDS WHITE ARROW ON PEDESTAL WITH VERTICAL BAR
    LineBreakClass::ULB_AL, // 0x21EE  # UPWARDS WHITE DOUBLE ARROW
    LineBreakClass::ULB_AL, // 0x21EF  # UPWARDS WHITE DOUBLE ARROW ON PEDESTAL
    LineBreakClass::ULB_AL, // 0x21F0  # RIGHTWARDS WHITE ARROW FROM WALL
    LineBreakClass::ULB_AL, // 0x21F1  # NORTH WEST ARROW TO CORNER
    LineBreakClass::ULB_AL, // 0x21F2  # SOUTH EAST ARROW TO CORNER
    LineBreakClass::ULB_AL, // 0x21F3  # UP DOWN WHITE ARROW
    LineBreakClass::ULB_AL, // 0x21F4  # RIGHT ARROW WITH SMALL CIRCLE
    LineBreakClass::ULB_AL, // 0x21F5  # DOWNWARDS ARROW LEFTWARDS OF UPWARDS ARROW
    LineBreakClass::ULB_AL, // 0x21F6  # THREE RIGHTWARDS ARROWS
    LineBreakClass::ULB_AL, // 0x21F7  # LEFTWARDS ARROW WITH VERTICAL STROKE
    LineBreakClass::ULB_AL, // 0x21F8  # RIGHTWARDS ARROW WITH VERTICAL STROKE
    LineBreakClass::ULB_AL, // 0x21F9  # LEFT RIGHT ARROW WITH VERTICAL STROKE
    LineBreakClass::ULB_AL, // 0x21FA  # LEFTWARDS ARROW WITH DOUBLE VERTICAL STROKE
    LineBreakClass::ULB_AL, // 0x21FB  # RIGHTWARDS ARROW WITH DOUBLE VERTICAL STROKE
    LineBreakClass::ULB_AL, // 0x21FC  # LEFT RIGHT ARROW WITH DOUBLE VERTICAL STROKE
    LineBreakClass::ULB_AL, // 0x21FD  # LEFTWARDS OPEN-HEADED ARROW
    LineBreakClass::ULB_AL, // 0x21FE  # RIGHTWARDS OPEN-HEADED ARROW
    LineBreakClass::ULB_AL, // 0x21FF  # LEFT RIGHT OPEN-HEADED ARROW
    LineBreakClass::ULB_AI, // 0x2200  # FOR ALL
    LineBreakClass::ULB_AL, // 0x2201  # COMPLEMENT
    LineBreakClass::ULB_AI, // 0x2202  # PARTIAL DIFFERENTIAL
    LineBreakClass::ULB_AI, // 0x2203  # THERE EXISTS
    LineBreakClass::ULB_AL, // 0x2204  # THERE DOES NOT EXIST
    LineBreakClass::ULB_AL, // 0x2205  # EMPTY SET
    LineBreakClass::ULB_AL, // 0x2206  # INCREMENT
    LineBreakClass::ULB_AI, // 0x2207  # NABLA
    LineBreakClass::ULB_AI, // 0x2208  # ELEMENT OF
    LineBreakClass::ULB_AL, // 0x2209  # NOT AN ELEMENT OF
    LineBreakClass::ULB_AL, // 0x220A  # SMALL ELEMENT OF
    LineBreakClass::ULB_AI, // 0x220B  # CONTAINS AS MEMBER
    LineBreakClass::ULB_AL, // 0x220C  # DOES NOT CONTAIN AS MEMBER
    LineBreakClass::ULB_AL, // 0x220D  # SMALL CONTAINS AS MEMBER
    LineBreakClass::ULB_AL, // 0x220E  # END OF PROOF
    LineBreakClass::ULB_AI, // 0x220F  # N-ARY PRODUCT
    LineBreakClass::ULB_AL, // 0x2210  # N-ARY COPRODUCT
    LineBreakClass::ULB_AI, // 0x2211  # N-ARY SUMMATION
    LineBreakClass::ULB_PR, // 0x2212  # MINUS SIGN
    LineBreakClass::ULB_PR, // 0x2213  # MINUS-OR-PLUS SIGN
    LineBreakClass::ULB_AL, // 0x2214  # DOT PLUS
    LineBreakClass::ULB_AI, // 0x2215  # DIVISION SLASH
    LineBreakClass::ULB_AL, // 0x2216  # SET MINUS
    LineBreakClass::ULB_AL, // 0x2217  # ASTERISK OPERATOR
    LineBreakClass::ULB_AL, // 0x2218  # RING OPERATOR
    LineBreakClass::ULB_AL, // 0x2219  # BULLET OPERATOR
    LineBreakClass::ULB_AI, // 0x221A  # SQUARE ROOT
    LineBreakClass::ULB_AL, // 0x221B  # CUBE ROOT
    LineBreakClass::ULB_AL, // 0x221C  # FOURTH ROOT
    LineBreakClass::ULB_AI, // 0x221D  # PROPORTIONAL TO
    LineBreakClass::ULB_AI, // 0x221E  # INFINITY
    LineBreakClass::ULB_AI, // 0x221F  # RIGHT ANGLE
    LineBreakClass::ULB_AI, // 0x2220  # ANGLE
    LineBreakClass::ULB_AL, // 0x2221  # MEASURED ANGLE
    LineBreakClass::ULB_AL, // 0x2222  # SPHERICAL ANGLE
    LineBreakClass::ULB_AI, // 0x2223  # DIVIDES
    LineBreakClass::ULB_AL, // 0x2224  # DOES NOT DIVIDE
    LineBreakClass::ULB_AI, // 0x2225  # PARALLEL TO
    LineBreakClass::ULB_AL, // 0x2226  # NOT PARALLEL TO
    LineBreakClass::ULB_AI, // 0x2227  # LOGICAL AND
    LineBreakClass::ULB_AI, // 0x2228  # LOGICAL OR
    LineBreakClass::ULB_AI, // 0x2229  # INTERSECTION
    LineBreakClass::ULB_AI, // 0x222A  # UNION
    LineBreakClass::ULB_AI, // 0x222B  # INTEGRAL
    LineBreakClass::ULB_AI, // 0x222C  # DOUBLE INTEGRAL
    LineBreakClass::ULB_AL, // 0x222D  # TRIPLE INTEGRAL
    LineBreakClass::ULB_AI, // 0x222E  # CONTOUR INTEGRAL
    LineBreakClass::ULB_AL, // 0x222F  # SURFACE INTEGRAL
    LineBreakClass::ULB_AL, // 0x2230  # VOLUME INTEGRAL
    LineBreakClass::ULB_AL, // 0x2231  # CLOCKWISE INTEGRAL
    LineBreakClass::ULB_AL, // 0x2232  # CLOCKWISE CONTOUR INTEGRAL
    LineBreakClass::ULB_AL, // 0x2233  # ANTICLOCKWISE CONTOUR INTEGRAL
    LineBreakClass::ULB_AI, // 0x2234  # THEREFORE
    LineBreakClass::ULB_AI, // 0x2235  # BECAUSE
    LineBreakClass::ULB_AI, // 0x2236  # RATIO
    LineBreakClass::ULB_AI, // 0x2237  # PROPORTION
    LineBreakClass::ULB_AL, // 0x2238  # DOT MINUS
    LineBreakClass::ULB_AL, // 0x2239  # EXCESS
    LineBreakClass::ULB_AL, // 0x223A  # GEOMETRIC PROPORTION
    LineBreakClass::ULB_AL, // 0x223B  # HOMOTHETIC
    LineBreakClass::ULB_AI, // 0x223C  # TILDE OPERATOR
    LineBreakClass::ULB_AI, // 0x223D  # REVERSED TILDE
    LineBreakClass::ULB_AL, // 0x223E  # INVERTED LAZY S
    LineBreakClass::ULB_AL, // 0x223F  # SINE WAVE
    LineBreakClass::ULB_AL, // 0x2240  # WREATH PRODUCT
    LineBreakClass::ULB_AL, // 0x2241  # NOT TILDE
    LineBreakClass::ULB_AL, // 0x2242  # MINUS TILDE
    LineBreakClass::ULB_AL, // 0x2243  # ASYMPTOTICALLY EQUAL TO
    LineBreakClass::ULB_AL, // 0x2244  # NOT ASYMPTOTICALLY EQUAL TO
    LineBreakClass::ULB_AL, // 0x2245  # APPROXIMATELY EQUAL TO
    LineBreakClass::ULB_AL, // 0x2246  # APPROXIMATELY BUT NOT ACTUALLY EQUAL TO
    LineBreakClass::ULB_AL, // 0x2247  # NEITHER APPROXIMATELY NOR ACTUALLY EQUAL TO
    LineBreakClass::ULB_AI, // 0x2248  # ALMOST EQUAL TO
    LineBreakClass::ULB_AL, // 0x2249  # NOT ALMOST EQUAL TO
    LineBreakClass::ULB_AL, // 0x224A  # ALMOST EQUAL OR EQUAL TO
    LineBreakClass::ULB_AL, // 0x224B  # TRIPLE TILDE
    LineBreakClass::ULB_AI, // 0x224C  # ALL EQUAL TO
    LineBreakClass::ULB_AL, // 0x224D  # EQUIVALENT TO
    LineBreakClass::ULB_AL, // 0x224E  # GEOMETRICALLY EQUIVALENT TO
    LineBreakClass::ULB_AL, // 0x224F  # DIFFERENCE BETWEEN
    LineBreakClass::ULB_AL, // 0x2250  # APPROACHES THE LIMIT
    LineBreakClass::ULB_AL, // 0x2251  # GEOMETRICALLY EQUAL TO
    LineBreakClass::ULB_AI, // 0x2252  # APPROXIMATELY EQUAL TO OR THE IMAGE OF
    LineBreakClass::ULB_AL, // 0x2253  # IMAGE OF OR APPROXIMATELY EQUAL TO
    LineBreakClass::ULB_AL, // 0x2254  # COLON EQUALS
    LineBreakClass::ULB_AL, // 0x2255  # EQUALS COLON
    LineBreakClass::ULB_AL, // 0x2256  # RING IN EQUAL TO
    LineBreakClass::ULB_AL, // 0x2257  # RING EQUAL TO
    LineBreakClass::ULB_AL, // 0x2258  # CORRESPONDS TO
    LineBreakClass::ULB_AL, // 0x2259  # ESTIMATES
    LineBreakClass::ULB_AL, // 0x225A  # EQUIANGULAR TO
    LineBreakClass::ULB_AL, // 0x225B  # STAR EQUALS
    LineBreakClass::ULB_AL, // 0x225C  # DELTA EQUAL TO
    LineBreakClass::ULB_AL, // 0x225D  # EQUAL TO BY DEFINITION
    LineBreakClass::ULB_AL, // 0x225E  # MEASURED BY
    LineBreakClass::ULB_AL, // 0x225F  # QUESTIONED EQUAL TO
    LineBreakClass::ULB_AI, // 0x2260  # NOT EQUAL TO
    LineBreakClass::ULB_AI, // 0x2261  # IDENTICAL TO
    LineBreakClass::ULB_AL, // 0x2262  # NOT IDENTICAL TO
    LineBreakClass::ULB_AL, // 0x2263  # STRICTLY EQUIVALENT TO
    LineBreakClass::ULB_AI, // 0x2264  # LESS-THAN OR EQUAL TO
    LineBreakClass::ULB_AI, // 0x2265  # GREATER-THAN OR EQUAL TO
    LineBreakClass::ULB_AI, // 0x2266  # LESS-THAN OVER EQUAL TO
    LineBreakClass::ULB_AI, // 0x2267  # GREATER-THAN OVER EQUAL TO
    LineBreakClass::ULB_AL, // 0x2268  # LESS-THAN BUT NOT EQUAL TO
    LineBreakClass::ULB_AL, // 0x2269  # GREATER-THAN BUT NOT EQUAL TO
    LineBreakClass::ULB_AI, // 0x226A  # MUCH LESS-THAN
    LineBreakClass::ULB_AI, // 0x226B  # MUCH GREATER-THAN
    LineBreakClass::ULB_AL, // 0x226C  # BETWEEN
    LineBreakClass::ULB_AL, // 0x226D  # NOT EQUIVALENT TO
    LineBreakClass::ULB_AI, // 0x226E  # NOT LESS-THAN
    LineBreakClass::ULB_AI, // 0x226F  # NOT GREATER-THAN
    LineBreakClass::ULB_AL, // 0x2270  # NEITHER LESS-THAN NOR EQUAL TO
    LineBreakClass::ULB_AL, // 0x2271  # NEITHER GREATER-THAN NOR EQUAL TO
    LineBreakClass::ULB_AL, // 0x2272  # LESS-THAN OR EQUIVALENT TO
    LineBreakClass::ULB_AL, // 0x2273  # GREATER-THAN OR EQUIVALENT TO
    LineBreakClass::ULB_AL, // 0x2274  # NEITHER LESS-THAN NOR EQUIVALENT TO
    LineBreakClass::ULB_AL, // 0x2275  # NEITHER GREATER-THAN NOR EQUIVALENT TO
    LineBreakClass::ULB_AL, // 0x2276  # LESS-THAN OR GREATER-THAN
    LineBreakClass::ULB_AL, // 0x2277  # GREATER-THAN OR LESS-THAN
    LineBreakClass::ULB_AL, // 0x2278  # NEITHER LESS-THAN NOR GREATER-THAN
    LineBreakClass::ULB_AL, // 0x2279  # NEITHER GREATER-THAN NOR LESS-THAN
    LineBreakClass::ULB_AL, // 0x227A  # PRECEDES
    LineBreakClass::ULB_AL, // 0x227B  # SUCCEEDS
    LineBreakClass::ULB_AL, // 0x227C  # PRECEDES OR EQUAL TO
    LineBreakClass::ULB_AL, // 0x227D  # SUCCEEDS OR EQUAL TO
    LineBreakClass::ULB_AL, // 0x227E  # PRECEDES OR EQUIVALENT TO
    LineBreakClass::ULB_AL, // 0x227F  # SUCCEEDS OR EQUIVALENT TO
    LineBreakClass::ULB_AL, // 0x2280  # DOES NOT PRECEDE
    LineBreakClass::ULB_AL, // 0x2281  # DOES NOT SUCCEED
    LineBreakClass::ULB_AI, // 0x2282  # SUBSET OF
    LineBreakClass::ULB_AI, // 0x2283  # SUPERSET OF
    LineBreakClass::ULB_AL, // 0x2284  # NOT A SUBSET OF
    LineBreakClass::ULB_AL, // 0x2285  # NOT A SUPERSET OF
    LineBreakClass::ULB_AI, // 0x2286  # SUBSET OF OR EQUAL TO
    LineBreakClass::ULB_AI, // 0x2287  # SUPERSET OF OR EQUAL TO
    LineBreakClass::ULB_AL, // 0x2288  # NEITHER A SUBSET OF NOR EQUAL TO
    LineBreakClass::ULB_AL, // 0x2289  # NEITHER A SUPERSET OF NOR EQUAL TO
    LineBreakClass::ULB_AL, // 0x228A  # SUBSET OF WITH NOT EQUAL TO
    LineBreakClass::ULB_AL, // 0x228B  # SUPERSET OF WITH NOT EQUAL TO
    LineBreakClass::ULB_AL, // 0x228C  # MULTISET
    LineBreakClass::ULB_AL, // 0x228D  # MULTISET MULTIPLICATION
    LineBreakClass::ULB_AL, // 0x228E  # MULTISET UNION
    LineBreakClass::ULB_AL, // 0x228F  # SQUARE IMAGE OF
    LineBreakClass::ULB_AL, // 0x2290  # SQUARE ORIGINAL OF
    LineBreakClass::ULB_AL, // 0x2291  # SQUARE IMAGE OF OR EQUAL TO
    LineBreakClass::ULB_AL, // 0x2292  # SQUARE ORIGINAL OF OR EQUAL TO
    LineBreakClass::ULB_AL, // 0x2293  # SQUARE CAP
    LineBreakClass::ULB_AL, // 0x2294  # SQUARE CUP
    LineBreakClass::ULB_AI, // 0x2295  # CIRCLED PLUS
    LineBreakClass::ULB_AL, // 0x2296  # CIRCLED MINUS
    LineBreakClass::ULB_AL, // 0x2297  # CIRCLED TIMES
    LineBreakClass::ULB_AL, // 0x2298  # CIRCLED DIVISION SLASH
    LineBreakClass::ULB_AI, // 0x2299  # CIRCLED DOT OPERATOR
    LineBreakClass::ULB_AL, // 0x229A  # CIRCLED RING OPERATOR
    LineBreakClass::ULB_AL, // 0x229B  # CIRCLED ASTERISK OPERATOR
    LineBreakClass::ULB_AL, // 0x229C  # CIRCLED EQUALS
    LineBreakClass::ULB_AL, // 0x229D  # CIRCLED DASH
    LineBreakClass::ULB_AL, // 0x229E  # SQUARED PLUS
    LineBreakClass::ULB_AL, // 0x229F  # SQUARED MINUS
    LineBreakClass::ULB_AL, // 0x22A0  # SQUARED TIMES
    LineBreakClass::ULB_AL, // 0x22A1  # SQUARED DOT OPERATOR
    LineBreakClass::ULB_AL, // 0x22A2  # RIGHT TACK
    LineBreakClass::ULB_AL, // 0x22A3  # LEFT TACK
    LineBreakClass::ULB_AL, // 0x22A4  # DOWN TACK
    LineBreakClass::ULB_AI, // 0x22A5  # UP TACK
    LineBreakClass::ULB_AL, // 0x22A6  # ASSERTION
    LineBreakClass::ULB_AL, // 0x22A7  # MODELS
    LineBreakClass::ULB_AL, // 0x22A8  # TRUE
    LineBreakClass::ULB_AL, // 0x22A9  # FORCES
    LineBreakClass::ULB_AL, // 0x22AA  # TRIPLE VERTICAL BAR RIGHT TURNSTILE
    LineBreakClass::ULB_AL, // 0x22AB  # DOUBLE VERTICAL BAR DOUBLE RIGHT TURNSTILE
    LineBreakClass::ULB_AL, // 0x22AC  # DOES NOT PROVE
    LineBreakClass::ULB_AL, // 0x22AD  # NOT TRUE
    LineBreakClass::ULB_AL, // 0x22AE  # DOES NOT FORCE
    LineBreakClass::ULB_AL, // 0x22AF  # NEGATED DOUBLE VERTICAL BAR DOUBLE RIGHT TURNSTILE
    LineBreakClass::ULB_AL, // 0x22B0  # PRECEDES UNDER RELATION
    LineBreakClass::ULB_AL, // 0x22B1  # SUCCEEDS UNDER RELATION
    LineBreakClass::ULB_AL, // 0x22B2  # NORMAL SUBGROUP OF
    LineBreakClass::ULB_AL, // 0x22B3  # CONTAINS AS NORMAL SUBGROUP
    LineBreakClass::ULB_AL, // 0x22B4  # NORMAL SUBGROUP OF OR EQUAL TO
    LineBreakClass::ULB_AL, // 0x22B5  # CONTAINS AS NORMAL SUBGROUP OR EQUAL TO
    LineBreakClass::ULB_AL, // 0x22B6  # ORIGINAL OF
    LineBreakClass::ULB_AL, // 0x22B7  # IMAGE OF
    LineBreakClass::ULB_AL, // 0x22B8  # MULTIMAP
    LineBreakClass::ULB_AL, // 0x22B9  # HERMITIAN CONJUGATE MATRIX
    LineBreakClass::ULB_AL, // 0x22BA  # INTERCALATE
    LineBreakClass::ULB_AL, // 0x22BB  # XOR
    LineBreakClass::ULB_AL, // 0x22BC  # NAND
    LineBreakClass::ULB_AL, // 0x22BD  # NOR
    LineBreakClass::ULB_AL, // 0x22BE  # RIGHT ANGLE WITH ARC
    LineBreakClass::ULB_AI, // 0x22BF  # RIGHT TRIANGLE
    LineBreakClass::ULB_AL, // 0x22C0  # N-ARY LOGICAL AND
    LineBreakClass::ULB_AL, // 0x22C1  # N-ARY LOGICAL OR
    LineBreakClass::ULB_AL, // 0x22C2  # N-ARY INTERSECTION
    LineBreakClass::ULB_AL, // 0x22C3  # N-ARY UNION
    LineBreakClass::ULB_AL, // 0x22C4  # DIAMOND OPERATOR
    LineBreakClass::ULB_AL, // 0x22C5  # DOT OPERATOR
    LineBreakClass::ULB_AL, // 0x22C6  # STAR OPERATOR
    LineBreakClass::ULB_AL, // 0x22C7  # DIVISION TIMES
    LineBreakClass::ULB_AL, // 0x22C8  # BOWTIE
    LineBreakClass::ULB_AL, // 0x22C9  # LEFT NORMAL FACTOR SEMIDIRECT PRODUCT
    LineBreakClass::ULB_AL, // 0x22CA  # RIGHT NORMAL FACTOR SEMIDIRECT PRODUCT
    LineBreakClass::ULB_AL, // 0x22CB  # LEFT SEMIDIRECT PRODUCT
    LineBreakClass::ULB_AL, // 0x22CC  # RIGHT SEMIDIRECT PRODUCT
    LineBreakClass::ULB_AL, // 0x22CD  # REVERSED TILDE EQUALS
    LineBreakClass::ULB_AL, // 0x22CE  # CURLY LOGICAL OR
    LineBreakClass::ULB_AL, // 0x22CF  # CURLY LOGICAL AND
    LineBreakClass::ULB_AL, // 0x22D0  # DOUBLE SUBSET
    LineBreakClass::ULB_AL, // 0x22D1  # DOUBLE SUPERSET
    LineBreakClass::ULB_AL, // 0x22D2  # DOUBLE INTERSECTION
    LineBreakClass::ULB_AL, // 0x22D3  # DOUBLE UNION
    LineBreakClass::ULB_AL, // 0x22D4  # PITCHFORK
    LineBreakClass::ULB_AL, // 0x22D5  # EQUAL AND PARALLEL TO
    LineBreakClass::ULB_AL, // 0x22D6  # LESS-THAN WITH DOT
    LineBreakClass::ULB_AL, // 0x22D7  # GREATER-THAN WITH DOT
    LineBreakClass::ULB_AL, // 0x22D8  # VERY MUCH LESS-THAN
    LineBreakClass::ULB_AL, // 0x22D9  # VERY MUCH GREATER-THAN
    LineBreakClass::ULB_AL, // 0x22DA  # LESS-THAN EQUAL TO OR GREATER-THAN
    LineBreakClass::ULB_AL, // 0x22DB  # GREATER-THAN EQUAL TO OR LESS-THAN
    LineBreakClass::ULB_AL, // 0x22DC  # EQUAL TO OR LESS-THAN
    LineBreakClass::ULB_AL, // 0x22DD  # EQUAL TO OR GREATER-THAN
    LineBreakClass::ULB_AL, // 0x22DE  # EQUAL TO OR PRECEDES
    LineBreakClass::ULB_AL, // 0x22DF  # EQUAL TO OR SUCCEEDS
    LineBreakClass::ULB_AL, // 0x22E0  # DOES NOT PRECEDE OR EQUAL
    LineBreakClass::ULB_AL, // 0x22E1  # DOES NOT SUCCEED OR EQUAL
    LineBreakClass::ULB_AL, // 0x22E2  # NOT SQUARE IMAGE OF OR EQUAL TO
    LineBreakClass::ULB_AL, // 0x22E3  # NOT SQUARE ORIGINAL OF OR EQUAL TO
    LineBreakClass::ULB_AL, // 0x22E4  # SQUARE IMAGE OF OR NOT EQUAL TO
    LineBreakClass::ULB_AL, // 0x22E5  # SQUARE ORIGINAL OF OR NOT EQUAL TO
    LineBreakClass::ULB_AL, // 0x22E6  # LESS-THAN BUT NOT EQUIVALENT TO
    LineBreakClass::ULB_AL, // 0x22E7  # GREATER-THAN BUT NOT EQUIVALENT TO
    LineBreakClass::ULB_AL, // 0x22E8  # PRECEDES BUT NOT EQUIVALENT TO
    LineBreakClass::ULB_AL, // 0x22E9  # SUCCEEDS BUT NOT EQUIVALENT TO
    LineBreakClass::ULB_AL, // 0x22EA  # NOT NORMAL SUBGROUP OF
    LineBreakClass::ULB_AL, // 0x22EB  # DOES NOT CONTAIN AS NORMAL SUBGROUP
    LineBreakClass::ULB_AL, // 0x22EC  # NOT NORMAL SUBGROUP OF OR EQUAL TO
    LineBreakClass::ULB_AL, // 0x22ED  # DOES NOT CONTAIN AS NORMAL SUBGROUP OR EQUAL
    LineBreakClass::ULB_AL, // 0x22EE  # VERTICAL ELLIPSIS
    LineBreakClass::ULB_AL, // 0x22EF  # MIDLINE HORIZONTAL ELLIPSIS
    LineBreakClass::ULB_AL, // 0x22F0  # UP RIGHT DIAGONAL ELLIPSIS
    LineBreakClass::ULB_AL, // 0x22F1  # DOWN RIGHT DIAGONAL ELLIPSIS
    LineBreakClass::ULB_AL, // 0x22F2  # ELEMENT OF WITH LONG HORIZONTAL STROKE
    LineBreakClass::ULB_AL, // 0x22F3  # ELEMENT OF WITH VERTICAL BAR AT END OF HORIZONTAL STROKE
    LineBreakClass::ULB_AL, // 0x22F4  # SMALL ELEMENT OF WITH VERTICAL BAR AT END OF HORIZONTAL
                            // STROKE
    LineBreakClass::ULB_AL, // 0x22F5  # ELEMENT OF WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x22F6  # ELEMENT OF WITH OVERBAR
    LineBreakClass::ULB_AL, // 0x22F7  # SMALL ELEMENT OF WITH OVERBAR
    LineBreakClass::ULB_AL, // 0x22F8  # ELEMENT OF WITH UNDERBAR
    LineBreakClass::ULB_AL, // 0x22F9  # ELEMENT OF WITH TWO HORIZONTAL STROKES
    LineBreakClass::ULB_AL, // 0x22FA  # CONTAINS WITH LONG HORIZONTAL STROKE
    LineBreakClass::ULB_AL, // 0x22FB  # CONTAINS WITH VERTICAL BAR AT END OF HORIZONTAL STROKE
    LineBreakClass::ULB_AL, // 0x22FC  # SMALL CONTAINS WITH VERTICAL BAR AT END OF HORIZONTAL
                            // STROKE
    LineBreakClass::ULB_AL, // 0x22FD  # CONTAINS WITH OVERBAR
    LineBreakClass::ULB_AL, // 0x22FE  # SMALL CONTAINS WITH OVERBAR
    LineBreakClass::ULB_AL, // 0x22FF  # Z NOTATION BAG MEMBERSHIP
    LineBreakClass::ULB_AL, // 0x2300  # DIAMETER SIGN
    LineBreakClass::ULB_AL, // 0x2301  # ELECTRIC ARROW
    LineBreakClass::ULB_AL, // 0x2302  # HOUSE
    LineBreakClass::ULB_AL, // 0x2303  # UP ARROWHEAD
    LineBreakClass::ULB_AL, // 0x2304  # DOWN ARROWHEAD
    LineBreakClass::ULB_AL, // 0x2305  # PROJECTIVE
    LineBreakClass::ULB_AL, // 0x2306  # PERSPECTIVE
    LineBreakClass::ULB_AL, // 0x2307  # WAVY LINE
    LineBreakClass::ULB_AL, // 0x2308  # LEFT CEILING
    LineBreakClass::ULB_AL, // 0x2309  # RIGHT CEILING
    LineBreakClass::ULB_AL, // 0x230A  # LEFT FLOOR
    LineBreakClass::ULB_AL, // 0x230B  # RIGHT FLOOR
    LineBreakClass::ULB_AL, // 0x230C  # BOTTOM RIGHT CROP
    LineBreakClass::ULB_AL, // 0x230D  # BOTTOM LEFT CROP
    LineBreakClass::ULB_AL, // 0x230E  # TOP RIGHT CROP
    LineBreakClass::ULB_AL, // 0x230F  # TOP LEFT CROP
    LineBreakClass::ULB_AL, // 0x2310  # REVERSED NOT SIGN
    LineBreakClass::ULB_AL, // 0x2311  # SQUARE LOZENGE
    LineBreakClass::ULB_AI, // 0x2312  # ARC
    LineBreakClass::ULB_AL, // 0x2313  # SEGMENT
    LineBreakClass::ULB_AL, // 0x2314  # SECTOR
    LineBreakClass::ULB_AL, // 0x2315  # TELEPHONE RECORDER
    LineBreakClass::ULB_AL, // 0x2316  # POSITION INDICATOR
    LineBreakClass::ULB_AL, // 0x2317  # VIEWDATA SQUARE
    LineBreakClass::ULB_AL, // 0x2318  # PLACE OF INTEREST SIGN
    LineBreakClass::ULB_AL, // 0x2319  # TURNED NOT SIGN
    LineBreakClass::ULB_AL, // 0x231A  # WATCH
    LineBreakClass::ULB_AL, // 0x231B  # HOURGLASS
    LineBreakClass::ULB_AL, // 0x231C  # TOP LEFT CORNER
    LineBreakClass::ULB_AL, // 0x231D  # TOP RIGHT CORNER
    LineBreakClass::ULB_AL, // 0x231E  # BOTTOM LEFT CORNER
    LineBreakClass::ULB_AL, // 0x231F  # BOTTOM RIGHT CORNER
    LineBreakClass::ULB_AL, // 0x2320  # TOP HALF INTEGRAL
    LineBreakClass::ULB_AL, // 0x2321  # BOTTOM HALF INTEGRAL
    LineBreakClass::ULB_AL, // 0x2322  # FROWN
    LineBreakClass::ULB_AL, // 0x2323  # SMILE
    LineBreakClass::ULB_AL, // 0x2324  # UP ARROWHEAD BETWEEN TWO HORIZONTAL BARS
    LineBreakClass::ULB_AL, // 0x2325  # OPTION KEY
    LineBreakClass::ULB_AL, // 0x2326  # ERASE TO THE RIGHT
    LineBreakClass::ULB_AL, // 0x2327  # X IN A RECTANGLE BOX
    LineBreakClass::ULB_AL, // 0x2328  # KEYBOARD
    LineBreakClass::ULB_OP, // 0x2329  # LEFT-POINTING ANGLE BRACKET
    LineBreakClass::ULB_CL, // 0x232A  # RIGHT-POINTING ANGLE BRACKET
    LineBreakClass::ULB_AL, // 0x232B  # ERASE TO THE LEFT
    LineBreakClass::ULB_AL, // 0x232C  # BENZENE RING
    LineBreakClass::ULB_AL, // 0x232D  # CYLINDRICITY
    LineBreakClass::ULB_AL, // 0x232E  # ALL AROUND-PROFILE
    LineBreakClass::ULB_AL, // 0x232F  # SYMMETRY
    LineBreakClass::ULB_AL, // 0x2330  # TOTAL RUNOUT
    LineBreakClass::ULB_AL, // 0x2331  # DIMENSION ORIGIN
    LineBreakClass::ULB_AL, // 0x2332  # CONICAL TAPER
    LineBreakClass::ULB_AL, // 0x2333  # SLOPE
    LineBreakClass::ULB_AL, // 0x2334  # COUNTERBORE
    LineBreakClass::ULB_AL, // 0x2335  # COUNTERSINK
    LineBreakClass::ULB_AL, // 0x2336  # APL FUNCTIONAL SYMBOL I-BEAM
    LineBreakClass::ULB_AL, // 0x2337  # APL FUNCTIONAL SYMBOL SQUISH QUAD
    LineBreakClass::ULB_AL, // 0x2338  # APL FUNCTIONAL SYMBOL QUAD EQUAL
    LineBreakClass::ULB_AL, // 0x2339  # APL FUNCTIONAL SYMBOL QUAD DIVIDE
    LineBreakClass::ULB_AL, // 0x233A  # APL FUNCTIONAL SYMBOL QUAD DIAMOND
    LineBreakClass::ULB_AL, // 0x233B  # APL FUNCTIONAL SYMBOL QUAD JOT
    LineBreakClass::ULB_AL, // 0x233C  # APL FUNCTIONAL SYMBOL QUAD CIRCLE
    LineBreakClass::ULB_AL, // 0x233D  # APL FUNCTIONAL SYMBOL CIRCLE STILE
    LineBreakClass::ULB_AL, // 0x233E  # APL FUNCTIONAL SYMBOL CIRCLE JOT
    LineBreakClass::ULB_AL, // 0x233F  # APL FUNCTIONAL SYMBOL SLASH BAR
    LineBreakClass::ULB_AL, // 0x2340  # APL FUNCTIONAL SYMBOL BACKSLASH BAR
    LineBreakClass::ULB_AL, // 0x2341  # APL FUNCTIONAL SYMBOL QUAD SLASH
    LineBreakClass::ULB_AL, // 0x2342  # APL FUNCTIONAL SYMBOL QUAD BACKSLASH
    LineBreakClass::ULB_AL, // 0x2343  # APL FUNCTIONAL SYMBOL QUAD LESS-THAN
    LineBreakClass::ULB_AL, // 0x2344  # APL FUNCTIONAL SYMBOL QUAD GREATER-THAN
    LineBreakClass::ULB_AL, // 0x2345  # APL FUNCTIONAL SYMBOL LEFTWARDS VANE
    LineBreakClass::ULB_AL, // 0x2346  # APL FUNCTIONAL SYMBOL RIGHTWARDS VANE
    LineBreakClass::ULB_AL, // 0x2347  # APL FUNCTIONAL SYMBOL QUAD LEFTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x2348  # APL FUNCTIONAL SYMBOL QUAD RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x2349  # APL FUNCTIONAL SYMBOL CIRCLE BACKSLASH
    LineBreakClass::ULB_AL, // 0x234A  # APL FUNCTIONAL SYMBOL DOWN TACK UNDERBAR
    LineBreakClass::ULB_AL, // 0x234B  # APL FUNCTIONAL SYMBOL DELTA STILE
    LineBreakClass::ULB_AL, // 0x234C  # APL FUNCTIONAL SYMBOL QUAD DOWN CARET
    LineBreakClass::ULB_AL, // 0x234D  # APL FUNCTIONAL SYMBOL QUAD DELTA
    LineBreakClass::ULB_AL, // 0x234E  # APL FUNCTIONAL SYMBOL DOWN TACK JOT
    LineBreakClass::ULB_AL, // 0x234F  # APL FUNCTIONAL SYMBOL UPWARDS VANE
    LineBreakClass::ULB_AL, // 0x2350  # APL FUNCTIONAL SYMBOL QUAD UPWARDS ARROW
    LineBreakClass::ULB_AL, // 0x2351  # APL FUNCTIONAL SYMBOL UP TACK OVERBAR
    LineBreakClass::ULB_AL, // 0x2352  # APL FUNCTIONAL SYMBOL DEL STILE
    LineBreakClass::ULB_AL, // 0x2353  # APL FUNCTIONAL SYMBOL QUAD UP CARET
    LineBreakClass::ULB_AL, // 0x2354  # APL FUNCTIONAL SYMBOL QUAD DEL
    LineBreakClass::ULB_AL, // 0x2355  # APL FUNCTIONAL SYMBOL UP TACK JOT
    LineBreakClass::ULB_AL, // 0x2356  # APL FUNCTIONAL SYMBOL DOWNWARDS VANE
    LineBreakClass::ULB_AL, // 0x2357  # APL FUNCTIONAL SYMBOL QUAD DOWNWARDS ARROW
    LineBreakClass::ULB_AL, // 0x2358  # APL FUNCTIONAL SYMBOL QUOTE UNDERBAR
    LineBreakClass::ULB_AL, // 0x2359  # APL FUNCTIONAL SYMBOL DELTA UNDERBAR
    LineBreakClass::ULB_AL, // 0x235A  # APL FUNCTIONAL SYMBOL DIAMOND UNDERBAR
    LineBreakClass::ULB_AL, // 0x235B  # APL FUNCTIONAL SYMBOL JOT UNDERBAR
    LineBreakClass::ULB_AL, // 0x235C  # APL FUNCTIONAL SYMBOL CIRCLE UNDERBAR
    LineBreakClass::ULB_AL, // 0x235D  # APL FUNCTIONAL SYMBOL UP SHOE JOT
    LineBreakClass::ULB_AL, // 0x235E  # APL FUNCTIONAL SYMBOL QUOTE QUAD
    LineBreakClass::ULB_AL, // 0x235F  # APL FUNCTIONAL SYMBOL CIRCLE STAR
    LineBreakClass::ULB_AL, // 0x2360  # APL FUNCTIONAL SYMBOL QUAD COLON
    LineBreakClass::ULB_AL, // 0x2361  # APL FUNCTIONAL SYMBOL UP TACK DIAERESIS
    LineBreakClass::ULB_AL, // 0x2362  # APL FUNCTIONAL SYMBOL DEL DIAERESIS
    LineBreakClass::ULB_AL, // 0x2363  # APL FUNCTIONAL SYMBOL STAR DIAERESIS
    LineBreakClass::ULB_AL, // 0x2364  # APL FUNCTIONAL SYMBOL JOT DIAERESIS
    LineBreakClass::ULB_AL, // 0x2365  # APL FUNCTIONAL SYMBOL CIRCLE DIAERESIS
    LineBreakClass::ULB_AL, // 0x2366  # APL FUNCTIONAL SYMBOL DOWN SHOE STILE
    LineBreakClass::ULB_AL, // 0x2367  # APL FUNCTIONAL SYMBOL LEFT SHOE STILE
    LineBreakClass::ULB_AL, // 0x2368  # APL FUNCTIONAL SYMBOL TILDE DIAERESIS
    LineBreakClass::ULB_AL, // 0x2369  # APL FUNCTIONAL SYMBOL GREATER-THAN DIAERESIS
    LineBreakClass::ULB_AL, // 0x236A  # APL FUNCTIONAL SYMBOL COMMA BAR
    LineBreakClass::ULB_AL, // 0x236B  # APL FUNCTIONAL SYMBOL DEL TILDE
    LineBreakClass::ULB_AL, // 0x236C  # APL FUNCTIONAL SYMBOL ZILDE
    LineBreakClass::ULB_AL, // 0x236D  # APL FUNCTIONAL SYMBOL STILE TILDE
    LineBreakClass::ULB_AL, // 0x236E  # APL FUNCTIONAL SYMBOL SEMICOLON UNDERBAR
    LineBreakClass::ULB_AL, // 0x236F  # APL FUNCTIONAL SYMBOL QUAD NOT EQUAL
    LineBreakClass::ULB_AL, // 0x2370  # APL FUNCTIONAL SYMBOL QUAD QUESTION
    LineBreakClass::ULB_AL, // 0x2371  # APL FUNCTIONAL SYMBOL DOWN CARET TILDE
    LineBreakClass::ULB_AL, // 0x2372  # APL FUNCTIONAL SYMBOL UP CARET TILDE
    LineBreakClass::ULB_AL, // 0x2373  # APL FUNCTIONAL SYMBOL IOTA
    LineBreakClass::ULB_AL, // 0x2374  # APL FUNCTIONAL SYMBOL RHO
    LineBreakClass::ULB_AL, // 0x2375  # APL FUNCTIONAL SYMBOL OMEGA
    LineBreakClass::ULB_AL, // 0x2376  # APL FUNCTIONAL SYMBOL ALPHA UNDERBAR
    LineBreakClass::ULB_AL, // 0x2377  # APL FUNCTIONAL SYMBOL EPSILON UNDERBAR
    LineBreakClass::ULB_AL, // 0x2378  # APL FUNCTIONAL SYMBOL IOTA UNDERBAR
    LineBreakClass::ULB_AL, // 0x2379  # APL FUNCTIONAL SYMBOL OMEGA UNDERBAR
    LineBreakClass::ULB_AL, // 0x237A  # APL FUNCTIONAL SYMBOL ALPHA
    LineBreakClass::ULB_AL, // 0x237B  # NOT CHECK MARK
    LineBreakClass::ULB_AL, // 0x237C  # RIGHT ANGLE WITH DOWNWARDS ZIGZAG ARROW
    LineBreakClass::ULB_AL, // 0x237D  # SHOULDERED OPEN BOX
    LineBreakClass::ULB_AL, // 0x237E  # BELL SYMBOL
    LineBreakClass::ULB_AL, // 0x237F  # VERTICAL LINE WITH MIDDLE DOT
    LineBreakClass::ULB_AL, // 0x2380  # INSERTION SYMBOL
    LineBreakClass::ULB_AL, // 0x2381  # CONTINUOUS UNDERLINE SYMBOL
    LineBreakClass::ULB_AL, // 0x2382  # DISCONTINUOUS UNDERLINE SYMBOL
    LineBreakClass::ULB_AL, // 0x2383  # EMPHASIS SYMBOL
    LineBreakClass::ULB_AL, // 0x2384  # COMPOSITION SYMBOL
    LineBreakClass::ULB_AL, // 0x2385  # WHITE SQUARE WITH CENTRE VERTICAL LINE
    LineBreakClass::ULB_AL, // 0x2386  # ENTER SYMBOL
    LineBreakClass::ULB_AL, // 0x2387  # ALTERNATIVE KEY SYMBOL
    LineBreakClass::ULB_AL, // 0x2388  # HELM SYMBOL
    LineBreakClass::ULB_AL, // 0x2389  # CIRCLED HORIZONTAL BAR WITH NOTCH
    LineBreakClass::ULB_AL, // 0x238A  # CIRCLED TRIANGLE DOWN
    LineBreakClass::ULB_AL, // 0x238B  # BROKEN CIRCLE WITH NORTHWEST ARROW
    LineBreakClass::ULB_AL, // 0x238C  # UNDO SYMBOL
    LineBreakClass::ULB_AL, // 0x238D  # MONOSTABLE SYMBOL
    LineBreakClass::ULB_AL, // 0x238E  # HYSTERESIS SYMBOL
    LineBreakClass::ULB_AL, // 0x238F  # OPEN-CIRCUIT-OUTPUT H-TYPE SYMBOL
    LineBreakClass::ULB_AL, // 0x2390  # OPEN-CIRCUIT-OUTPUT L-TYPE SYMBOL
    LineBreakClass::ULB_AL, // 0x2391  # PASSIVE-PULL-DOWN-OUTPUT SYMBOL
    LineBreakClass::ULB_AL, // 0x2392  # PASSIVE-PULL-UP-OUTPUT SYMBOL
    LineBreakClass::ULB_AL, // 0x2393  # DIRECT CURRENT SYMBOL FORM TWO
    LineBreakClass::ULB_AL, // 0x2394  # SOFTWARE-FUNCTION SYMBOL
    LineBreakClass::ULB_AL, // 0x2395  # APL FUNCTIONAL SYMBOL QUAD
    LineBreakClass::ULB_AL, // 0x2396  # DECIMAL SEPARATOR KEY SYMBOL
    LineBreakClass::ULB_AL, // 0x2397  # PREVIOUS PAGE
    LineBreakClass::ULB_AL, // 0x2398  # NEXT PAGE
    LineBreakClass::ULB_AL, // 0x2399  # PRINT SCREEN SYMBOL
    LineBreakClass::ULB_AL, // 0x239A  # CLEAR SCREEN SYMBOL
    LineBreakClass::ULB_AL, // 0x239B  # LEFT PARENTHESIS UPPER HOOK
    LineBreakClass::ULB_AL, // 0x239C  # LEFT PARENTHESIS EXTENSION
    LineBreakClass::ULB_AL, // 0x239D  # LEFT PARENTHESIS LOWER HOOK
    LineBreakClass::ULB_AL, // 0x239E  # RIGHT PARENTHESIS UPPER HOOK
    LineBreakClass::ULB_AL, // 0x239F  # RIGHT PARENTHESIS EXTENSION
    LineBreakClass::ULB_AL, // 0x23A0  # RIGHT PARENTHESIS LOWER HOOK
    LineBreakClass::ULB_AL, // 0x23A1  # LEFT SQUARE BRACKET UPPER CORNER
    LineBreakClass::ULB_AL, // 0x23A2  # LEFT SQUARE BRACKET EXTENSION
    LineBreakClass::ULB_AL, // 0x23A3  # LEFT SQUARE BRACKET LOWER CORNER
    LineBreakClass::ULB_AL, // 0x23A4  # RIGHT SQUARE BRACKET UPPER CORNER
    LineBreakClass::ULB_AL, // 0x23A5  # RIGHT SQUARE BRACKET EXTENSION
    LineBreakClass::ULB_AL, // 0x23A6  # RIGHT SQUARE BRACKET LOWER CORNER
    LineBreakClass::ULB_AL, // 0x23A7  # LEFT CURLY BRACKET UPPER HOOK
    LineBreakClass::ULB_AL, // 0x23A8  # LEFT CURLY BRACKET MIDDLE PIECE
    LineBreakClass::ULB_AL, // 0x23A9  # LEFT CURLY BRACKET LOWER HOOK
    LineBreakClass::ULB_AL, // 0x23AA  # CURLY BRACKET EXTENSION
    LineBreakClass::ULB_AL, // 0x23AB  # RIGHT CURLY BRACKET UPPER HOOK
    LineBreakClass::ULB_AL, // 0x23AC  # RIGHT CURLY BRACKET MIDDLE PIECE
    LineBreakClass::ULB_AL, // 0x23AD  # RIGHT CURLY BRACKET LOWER HOOK
    LineBreakClass::ULB_AL, // 0x23AE  # INTEGRAL EXTENSION
    LineBreakClass::ULB_AL, // 0x23AF  # HORIZONTAL LINE EXTENSION
    LineBreakClass::ULB_AL, // 0x23B0  # UPPER LEFT OR LOWER RIGHT CURLY BRACKET SECTION
    LineBreakClass::ULB_AL, // 0x23B1  # UPPER RIGHT OR LOWER LEFT CURLY BRACKET SECTION
    LineBreakClass::ULB_AL, // 0x23B2  # SUMMATION TOP
    LineBreakClass::ULB_AL, // 0x23B3  # SUMMATION BOTTOM
    LineBreakClass::ULB_AL, // 0x23B4  # TOP SQUARE BRACKET
    LineBreakClass::ULB_AL, // 0x23B5  # BOTTOM SQUARE BRACKET
    LineBreakClass::ULB_AL, // 0x23B6  # BOTTOM SQUARE BRACKET OVER TOP SQUARE BRACKET
    LineBreakClass::ULB_AL, // 0x23B7  # RADICAL SYMBOL BOTTOM
    LineBreakClass::ULB_AL, // 0x23B8  # LEFT VERTICAL BOX LINE
    LineBreakClass::ULB_AL, // 0x23B9  # RIGHT VERTICAL BOX LINE
    LineBreakClass::ULB_AL, // 0x23BA  # HORIZONTAL SCAN LINE-1
    LineBreakClass::ULB_AL, // 0x23BB  # HORIZONTAL SCAN LINE-3
    LineBreakClass::ULB_AL, // 0x23BC  # HORIZONTAL SCAN LINE-7
    LineBreakClass::ULB_AL, // 0x23BD  # HORIZONTAL SCAN LINE-9
    LineBreakClass::ULB_AL, // 0x23BE  # DENTISTRY SYMBOL LIGHT VERTICAL AND TOP RIGHT
    LineBreakClass::ULB_AL, // 0x23BF  # DENTISTRY SYMBOL LIGHT VERTICAL AND BOTTOM RIGHT
    LineBreakClass::ULB_AL, // 0x23C0  # DENTISTRY SYMBOL LIGHT VERTICAL WITH CIRCLE
    LineBreakClass::ULB_AL, // 0x23C1  # DENTISTRY SYMBOL LIGHT DOWN AND HORIZONTAL WITH CIRCLE
    LineBreakClass::ULB_AL, // 0x23C2  # DENTISTRY SYMBOL LIGHT UP AND HORIZONTAL WITH CIRCLE
    LineBreakClass::ULB_AL, // 0x23C3  # DENTISTRY SYMBOL LIGHT VERTICAL WITH TRIANGLE
    LineBreakClass::ULB_AL, // 0x23C4  # DENTISTRY SYMBOL LIGHT DOWN AND HORIZONTAL WITH TRIANGLE
    LineBreakClass::ULB_AL, // 0x23C5  # DENTISTRY SYMBOL LIGHT UP AND HORIZONTAL WITH TRIANGLE
    LineBreakClass::ULB_AL, // 0x23C6  # DENTISTRY SYMBOL LIGHT VERTICAL AND WAVE
    LineBreakClass::ULB_AL, // 0x23C7  # DENTISTRY SYMBOL LIGHT DOWN AND HORIZONTAL WITH WAVE
    LineBreakClass::ULB_AL, // 0x23C8  # DENTISTRY SYMBOL LIGHT UP AND HORIZONTAL WITH WAVE
    LineBreakClass::ULB_AL, // 0x23C9  # DENTISTRY SYMBOL LIGHT DOWN AND HORIZONTAL
    LineBreakClass::ULB_AL, // 0x23CA  # DENTISTRY SYMBOL LIGHT UP AND HORIZONTAL
    LineBreakClass::ULB_AL, // 0x23CB  # DENTISTRY SYMBOL LIGHT VERTICAL AND TOP LEFT
    LineBreakClass::ULB_AL, // 0x23CC  # DENTISTRY SYMBOL LIGHT VERTICAL AND BOTTOM LEFT
    LineBreakClass::ULB_AL, // 0x23CD  # SQUARE FOOT
    LineBreakClass::ULB_AL, // 0x23CE  # RETURN SYMBOL
    LineBreakClass::ULB_AL, // 0x23CF  # EJECT SYMBOL
    LineBreakClass::ULB_AL, // 0x23D0  # VERTICAL LINE EXTENSION
    LineBreakClass::ULB_AL, // 0x23D1  # METRICAL BREVE
    LineBreakClass::ULB_AL, // 0x23D2  # METRICAL LONG OVER SHORT
    LineBreakClass::ULB_AL, // 0x23D3  # METRICAL SHORT OVER LONG
    LineBreakClass::ULB_AL, // 0x23D4  # METRICAL LONG OVER TWO SHORTS
    LineBreakClass::ULB_AL, // 0x23D5  # METRICAL TWO SHORTS OVER LONG
    LineBreakClass::ULB_AL, // 0x23D6  # METRICAL TWO SHORTS JOINED
    LineBreakClass::ULB_AL, // 0x23D7  # METRICAL TRISEME
    LineBreakClass::ULB_AL, // 0x23D8  # METRICAL TETRASEME
    LineBreakClass::ULB_AL, // 0x23D9  # METRICAL PENTASEME
    LineBreakClass::ULB_AL, // 0x23DA  # EARTH GROUND
    LineBreakClass::ULB_AL, // 0x23DB  # FUSE
    LineBreakClass::ULB_AL, // 0x23DC  # TOP PARENTHESIS
    LineBreakClass::ULB_AL, // 0x23DD  # BOTTOM PARENTHESIS
    LineBreakClass::ULB_AL, // 0x23DE  # TOP CURLY BRACKET
    LineBreakClass::ULB_AL, // 0x23DF  # BOTTOM CURLY BRACKET
    LineBreakClass::ULB_AL, // 0x23E0  # TOP TORTOISE SHELL BRACKET
    LineBreakClass::ULB_AL, // 0x23E1  # BOTTOM TORTOISE SHELL BRACKET
    LineBreakClass::ULB_AL, // 0x23E2  # WHITE TRAPEZIUM
    LineBreakClass::ULB_AL, // 0x23E3  # BENZENE RING WITH CIRCLE
    LineBreakClass::ULB_AL, // 0x23E4  # STRAIGHTNESS
    LineBreakClass::ULB_AL, // 0x23E5  # FLATNESS
    LineBreakClass::ULB_AL, // 0x23E6  # AC CURRENT
    LineBreakClass::ULB_AL, // 0x23E7  # ELECTRICAL INTERSECTION
    LineBreakClass::ULB_ID, // 0x23E8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23E9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23EA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23EB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23EC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23ED # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23EE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23EF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23F0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23F1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23F2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23F3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23F4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23F5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23F6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23F7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23F8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23F9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23FA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23FB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23FC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23FD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23FE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x23FF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2400  # SYMBOL FOR NULL
    LineBreakClass::ULB_AL, // 0x2401  # SYMBOL FOR START OF HEADING
    LineBreakClass::ULB_AL, // 0x2402  # SYMBOL FOR START OF TEXT
    LineBreakClass::ULB_AL, // 0x2403  # SYMBOL FOR END OF TEXT
    LineBreakClass::ULB_AL, // 0x2404  # SYMBOL FOR END OF TRANSMISSION
    LineBreakClass::ULB_AL, // 0x2405  # SYMBOL FOR ENQUIRY
    LineBreakClass::ULB_AL, // 0x2406  # SYMBOL FOR ACKNOWLEDGE
    LineBreakClass::ULB_AL, // 0x2407  # SYMBOL FOR BELL
    LineBreakClass::ULB_AL, // 0x2408  # SYMBOL FOR BACKSPACE
    LineBreakClass::ULB_AL, // 0x2409  # SYMBOL FOR HORIZONTAL TABULATION
    LineBreakClass::ULB_AL, // 0x240A  # SYMBOL FOR LINE FEED
    LineBreakClass::ULB_AL, // 0x240B  # SYMBOL FOR VERTICAL TABULATION
    LineBreakClass::ULB_AL, // 0x240C  # SYMBOL FOR FORM FEED
    LineBreakClass::ULB_AL, // 0x240D  # SYMBOL FOR CARRIAGE RETURN
    LineBreakClass::ULB_AL, // 0x240E  # SYMBOL FOR SHIFT OUT
    LineBreakClass::ULB_AL, // 0x240F  # SYMBOL FOR SHIFT IN
    LineBreakClass::ULB_AL, // 0x2410  # SYMBOL FOR DATA LINK ESCAPE
    LineBreakClass::ULB_AL, // 0x2411  # SYMBOL FOR DEVICE CONTROL ONE
    LineBreakClass::ULB_AL, // 0x2412  # SYMBOL FOR DEVICE CONTROL TWO
    LineBreakClass::ULB_AL, // 0x2413  # SYMBOL FOR DEVICE CONTROL THREE
    LineBreakClass::ULB_AL, // 0x2414  # SYMBOL FOR DEVICE CONTROL FOUR
    LineBreakClass::ULB_AL, // 0x2415  # SYMBOL FOR NEGATIVE ACKNOWLEDGE
    LineBreakClass::ULB_AL, // 0x2416  # SYMBOL FOR SYNCHRONOUS IDLE
    LineBreakClass::ULB_AL, // 0x2417  # SYMBOL FOR END OF TRANSMISSION BLOCK
    LineBreakClass::ULB_AL, // 0x2418  # SYMBOL FOR CANCEL
    LineBreakClass::ULB_AL, // 0x2419  # SYMBOL FOR END OF MEDIUM
    LineBreakClass::ULB_AL, // 0x241A  # SYMBOL FOR SUBSTITUTE
    LineBreakClass::ULB_AL, // 0x241B  # SYMBOL FOR ESCAPE
    LineBreakClass::ULB_AL, // 0x241C  # SYMBOL FOR FILE SEPARATOR
    LineBreakClass::ULB_AL, // 0x241D  # SYMBOL FOR GROUP SEPARATOR
    LineBreakClass::ULB_AL, // 0x241E  # SYMBOL FOR RECORD SEPARATOR
    LineBreakClass::ULB_AL, // 0x241F  # SYMBOL FOR UNIT SEPARATOR
    LineBreakClass::ULB_AL, // 0x2420  # SYMBOL FOR SPACE
    LineBreakClass::ULB_AL, // 0x2421  # SYMBOL FOR DELETE
    LineBreakClass::ULB_AL, // 0x2422  # BLANK SYMBOL
    LineBreakClass::ULB_AL, // 0x2423  # OPEN BOX
    LineBreakClass::ULB_AL, // 0x2424  # SYMBOL FOR NEWLINE
    LineBreakClass::ULB_AL, // 0x2425  # SYMBOL FOR DELETE FORM TWO
    LineBreakClass::ULB_AL, // 0x2426  # SYMBOL FOR SUBSTITUTE FORM TWO
    LineBreakClass::ULB_ID, // 0x2427 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2428 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2429 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x242A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x242B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x242C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x242D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x242E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x242F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2430 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2431 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2432 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2433 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2434 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2435 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2436 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2437 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2438 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2439 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x243A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x243B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x243C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x243D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x243E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x243F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2440  # OCR HOOK
    LineBreakClass::ULB_AL, // 0x2441  # OCR CHAIR
    LineBreakClass::ULB_AL, // 0x2442  # OCR FORK
    LineBreakClass::ULB_AL, // 0x2443  # OCR INVERTED FORK
    LineBreakClass::ULB_AL, // 0x2444  # OCR BELT BUCKLE
    LineBreakClass::ULB_AL, // 0x2445  # OCR BOW TIE
    LineBreakClass::ULB_AL, // 0x2446  # OCR BRANCH BANK IDENTIFICATION
    LineBreakClass::ULB_AL, // 0x2447  # OCR AMOUNT OF CHECK
    LineBreakClass::ULB_AL, // 0x2448  # OCR DASH
    LineBreakClass::ULB_AL, // 0x2449  # OCR CUSTOMER ACCOUNT NUMBER
    LineBreakClass::ULB_AL, // 0x244A  # OCR DOUBLE BACKSLASH
    LineBreakClass::ULB_ID, // 0x244B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x244C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x244D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x244E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x244F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2450 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2451 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2452 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2453 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2454 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2455 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2456 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2457 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2458 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2459 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x245A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x245B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x245C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x245D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x245E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x245F # <UNDEFINED>
    LineBreakClass::ULB_AI, // 0x2460  # CIRCLED DIGIT ONE
    LineBreakClass::ULB_AI, // 0x2461  # CIRCLED DIGIT TWO
    LineBreakClass::ULB_AI, // 0x2462  # CIRCLED DIGIT THREE
    LineBreakClass::ULB_AI, // 0x2463  # CIRCLED DIGIT FOUR
    LineBreakClass::ULB_AI, // 0x2464  # CIRCLED DIGIT FIVE
    LineBreakClass::ULB_AI, // 0x2465  # CIRCLED DIGIT SIX
    LineBreakClass::ULB_AI, // 0x2466  # CIRCLED DIGIT SEVEN
    LineBreakClass::ULB_AI, // 0x2467  # CIRCLED DIGIT EIGHT
    LineBreakClass::ULB_AI, // 0x2468  # CIRCLED DIGIT NINE
    LineBreakClass::ULB_AI, // 0x2469  # CIRCLED NUMBER TEN
    LineBreakClass::ULB_AI, // 0x246A  # CIRCLED NUMBER ELEVEN
    LineBreakClass::ULB_AI, // 0x246B  # CIRCLED NUMBER TWELVE
    LineBreakClass::ULB_AI, // 0x246C  # CIRCLED NUMBER THIRTEEN
    LineBreakClass::ULB_AI, // 0x246D  # CIRCLED NUMBER FOURTEEN
    LineBreakClass::ULB_AI, // 0x246E  # CIRCLED NUMBER FIFTEEN
    LineBreakClass::ULB_AI, // 0x246F  # CIRCLED NUMBER SIXTEEN
    LineBreakClass::ULB_AI, // 0x2470  # CIRCLED NUMBER SEVENTEEN
    LineBreakClass::ULB_AI, // 0x2471  # CIRCLED NUMBER EIGHTEEN
    LineBreakClass::ULB_AI, // 0x2472  # CIRCLED NUMBER NINETEEN
    LineBreakClass::ULB_AI, // 0x2473  # CIRCLED NUMBER TWENTY
    LineBreakClass::ULB_AI, // 0x2474  # PARENTHESIZED DIGIT ONE
    LineBreakClass::ULB_AI, // 0x2475  # PARENTHESIZED DIGIT TWO
    LineBreakClass::ULB_AI, // 0x2476  # PARENTHESIZED DIGIT THREE
    LineBreakClass::ULB_AI, // 0x2477  # PARENTHESIZED DIGIT FOUR
    LineBreakClass::ULB_AI, // 0x2478  # PARENTHESIZED DIGIT FIVE
    LineBreakClass::ULB_AI, // 0x2479  # PARENTHESIZED DIGIT SIX
    LineBreakClass::ULB_AI, // 0x247A  # PARENTHESIZED DIGIT SEVEN
    LineBreakClass::ULB_AI, // 0x247B  # PARENTHESIZED DIGIT EIGHT
    LineBreakClass::ULB_AI, // 0x247C  # PARENTHESIZED DIGIT NINE
    LineBreakClass::ULB_AI, // 0x247D  # PARENTHESIZED NUMBER TEN
    LineBreakClass::ULB_AI, // 0x247E  # PARENTHESIZED NUMBER ELEVEN
    LineBreakClass::ULB_AI, // 0x247F  # PARENTHESIZED NUMBER TWELVE
    LineBreakClass::ULB_AI, // 0x2480  # PARENTHESIZED NUMBER THIRTEEN
    LineBreakClass::ULB_AI, // 0x2481  # PARENTHESIZED NUMBER FOURTEEN
    LineBreakClass::ULB_AI, // 0x2482  # PARENTHESIZED NUMBER FIFTEEN
    LineBreakClass::ULB_AI, // 0x2483  # PARENTHESIZED NUMBER SIXTEEN
    LineBreakClass::ULB_AI, // 0x2484  # PARENTHESIZED NUMBER SEVENTEEN
    LineBreakClass::ULB_AI, // 0x2485  # PARENTHESIZED NUMBER EIGHTEEN
    LineBreakClass::ULB_AI, // 0x2486  # PARENTHESIZED NUMBER NINETEEN
    LineBreakClass::ULB_AI, // 0x2487  # PARENTHESIZED NUMBER TWENTY
    LineBreakClass::ULB_AI, // 0x2488  # DIGIT ONE FULL STOP
    LineBreakClass::ULB_AI, // 0x2489  # DIGIT TWO FULL STOP
    LineBreakClass::ULB_AI, // 0x248A  # DIGIT THREE FULL STOP
    LineBreakClass::ULB_AI, // 0x248B  # DIGIT FOUR FULL STOP
    LineBreakClass::ULB_AI, // 0x248C  # DIGIT FIVE FULL STOP
    LineBreakClass::ULB_AI, // 0x248D  # DIGIT SIX FULL STOP
    LineBreakClass::ULB_AI, // 0x248E  # DIGIT SEVEN FULL STOP
    LineBreakClass::ULB_AI, // 0x248F  # DIGIT EIGHT FULL STOP
    LineBreakClass::ULB_AI, // 0x2490  # DIGIT NINE FULL STOP
    LineBreakClass::ULB_AI, // 0x2491  # NUMBER TEN FULL STOP
    LineBreakClass::ULB_AI, // 0x2492  # NUMBER ELEVEN FULL STOP
    LineBreakClass::ULB_AI, // 0x2493  # NUMBER TWELVE FULL STOP
    LineBreakClass::ULB_AI, // 0x2494  # NUMBER THIRTEEN FULL STOP
    LineBreakClass::ULB_AI, // 0x2495  # NUMBER FOURTEEN FULL STOP
    LineBreakClass::ULB_AI, // 0x2496  # NUMBER FIFTEEN FULL STOP
    LineBreakClass::ULB_AI, // 0x2497  # NUMBER SIXTEEN FULL STOP
    LineBreakClass::ULB_AI, // 0x2498  # NUMBER SEVENTEEN FULL STOP
    LineBreakClass::ULB_AI, // 0x2499  # NUMBER EIGHTEEN FULL STOP
    LineBreakClass::ULB_AI, // 0x249A  # NUMBER NINETEEN FULL STOP
    LineBreakClass::ULB_AI, // 0x249B  # NUMBER TWENTY FULL STOP
    LineBreakClass::ULB_AI, // 0x249C  # PARENTHESIZED LATIN SMALL LETTER A
    LineBreakClass::ULB_AI, // 0x249D  # PARENTHESIZED LATIN SMALL LETTER B
    LineBreakClass::ULB_AI, // 0x249E  # PARENTHESIZED LATIN SMALL LETTER C
    LineBreakClass::ULB_AI, // 0x249F  # PARENTHESIZED LATIN SMALL LETTER D
    LineBreakClass::ULB_AI, // 0x24A0  # PARENTHESIZED LATIN SMALL LETTER E
    LineBreakClass::ULB_AI, // 0x24A1  # PARENTHESIZED LATIN SMALL LETTER F
    LineBreakClass::ULB_AI, // 0x24A2  # PARENTHESIZED LATIN SMALL LETTER G
    LineBreakClass::ULB_AI, // 0x24A3  # PARENTHESIZED LATIN SMALL LETTER H
    LineBreakClass::ULB_AI, // 0x24A4  # PARENTHESIZED LATIN SMALL LETTER I
    LineBreakClass::ULB_AI, // 0x24A5  # PARENTHESIZED LATIN SMALL LETTER J
    LineBreakClass::ULB_AI, // 0x24A6  # PARENTHESIZED LATIN SMALL LETTER K
    LineBreakClass::ULB_AI, // 0x24A7  # PARENTHESIZED LATIN SMALL LETTER L
    LineBreakClass::ULB_AI, // 0x24A8  # PARENTHESIZED LATIN SMALL LETTER M
    LineBreakClass::ULB_AI, // 0x24A9  # PARENTHESIZED LATIN SMALL LETTER N
    LineBreakClass::ULB_AI, // 0x24AA  # PARENTHESIZED LATIN SMALL LETTER O
    LineBreakClass::ULB_AI, // 0x24AB  # PARENTHESIZED LATIN SMALL LETTER P
    LineBreakClass::ULB_AI, // 0x24AC  # PARENTHESIZED LATIN SMALL LETTER Q
    LineBreakClass::ULB_AI, // 0x24AD  # PARENTHESIZED LATIN SMALL LETTER R
    LineBreakClass::ULB_AI, // 0x24AE  # PARENTHESIZED LATIN SMALL LETTER S
    LineBreakClass::ULB_AI, // 0x24AF  # PARENTHESIZED LATIN SMALL LETTER T
    LineBreakClass::ULB_AI, // 0x24B0  # PARENTHESIZED LATIN SMALL LETTER U
    LineBreakClass::ULB_AI, // 0x24B1  # PARENTHESIZED LATIN SMALL LETTER V
    LineBreakClass::ULB_AI, // 0x24B2  # PARENTHESIZED LATIN SMALL LETTER W
    LineBreakClass::ULB_AI, // 0x24B3  # PARENTHESIZED LATIN SMALL LETTER X
    LineBreakClass::ULB_AI, // 0x24B4  # PARENTHESIZED LATIN SMALL LETTER Y
    LineBreakClass::ULB_AI, // 0x24B5  # PARENTHESIZED LATIN SMALL LETTER Z
    LineBreakClass::ULB_AI, // 0x24B6  # CIRCLED LATIN CAPITAL LETTER A
    LineBreakClass::ULB_AI, // 0x24B7  # CIRCLED LATIN CAPITAL LETTER B
    LineBreakClass::ULB_AI, // 0x24B8  # CIRCLED LATIN CAPITAL LETTER C
    LineBreakClass::ULB_AI, // 0x24B9  # CIRCLED LATIN CAPITAL LETTER D
    LineBreakClass::ULB_AI, // 0x24BA  # CIRCLED LATIN CAPITAL LETTER E
    LineBreakClass::ULB_AI, // 0x24BB  # CIRCLED LATIN CAPITAL LETTER F
    LineBreakClass::ULB_AI, // 0x24BC  # CIRCLED LATIN CAPITAL LETTER G
    LineBreakClass::ULB_AI, // 0x24BD  # CIRCLED LATIN CAPITAL LETTER H
    LineBreakClass::ULB_AI, // 0x24BE  # CIRCLED LATIN CAPITAL LETTER I
    LineBreakClass::ULB_AI, // 0x24BF  # CIRCLED LATIN CAPITAL LETTER J
    LineBreakClass::ULB_AI, // 0x24C0  # CIRCLED LATIN CAPITAL LETTER K
    LineBreakClass::ULB_AI, // 0x24C1  # CIRCLED LATIN CAPITAL LETTER L
    LineBreakClass::ULB_AI, // 0x24C2  # CIRCLED LATIN CAPITAL LETTER M
    LineBreakClass::ULB_AI, // 0x24C3  # CIRCLED LATIN CAPITAL LETTER N
    LineBreakClass::ULB_AI, // 0x24C4  # CIRCLED LATIN CAPITAL LETTER O
    LineBreakClass::ULB_AI, // 0x24C5  # CIRCLED LATIN CAPITAL LETTER P
    LineBreakClass::ULB_AI, // 0x24C6  # CIRCLED LATIN CAPITAL LETTER Q
    LineBreakClass::ULB_AI, // 0x24C7  # CIRCLED LATIN CAPITAL LETTER R
    LineBreakClass::ULB_AI, // 0x24C8  # CIRCLED LATIN CAPITAL LETTER S
    LineBreakClass::ULB_AI, // 0x24C9  # CIRCLED LATIN CAPITAL LETTER T
    LineBreakClass::ULB_AI, // 0x24CA  # CIRCLED LATIN CAPITAL LETTER U
    LineBreakClass::ULB_AI, // 0x24CB  # CIRCLED LATIN CAPITAL LETTER V
    LineBreakClass::ULB_AI, // 0x24CC  # CIRCLED LATIN CAPITAL LETTER W
    LineBreakClass::ULB_AI, // 0x24CD  # CIRCLED LATIN CAPITAL LETTER X
    LineBreakClass::ULB_AI, // 0x24CE  # CIRCLED LATIN CAPITAL LETTER Y
    LineBreakClass::ULB_AI, // 0x24CF  # CIRCLED LATIN CAPITAL LETTER Z
    LineBreakClass::ULB_AI, // 0x24D0  # CIRCLED LATIN SMALL LETTER A
    LineBreakClass::ULB_AI, // 0x24D1  # CIRCLED LATIN SMALL LETTER B
    LineBreakClass::ULB_AI, // 0x24D2  # CIRCLED LATIN SMALL LETTER C
    LineBreakClass::ULB_AI, // 0x24D3  # CIRCLED LATIN SMALL LETTER D
    LineBreakClass::ULB_AI, // 0x24D4  # CIRCLED LATIN SMALL LETTER E
    LineBreakClass::ULB_AI, // 0x24D5  # CIRCLED LATIN SMALL LETTER F
    LineBreakClass::ULB_AI, // 0x24D6  # CIRCLED LATIN SMALL LETTER G
    LineBreakClass::ULB_AI, // 0x24D7  # CIRCLED LATIN SMALL LETTER H
    LineBreakClass::ULB_AI, // 0x24D8  # CIRCLED LATIN SMALL LETTER I
    LineBreakClass::ULB_AI, // 0x24D9  # CIRCLED LATIN SMALL LETTER J
    LineBreakClass::ULB_AI, // 0x24DA  # CIRCLED LATIN SMALL LETTER K
    LineBreakClass::ULB_AI, // 0x24DB  # CIRCLED LATIN SMALL LETTER L
    LineBreakClass::ULB_AI, // 0x24DC  # CIRCLED LATIN SMALL LETTER M
    LineBreakClass::ULB_AI, // 0x24DD  # CIRCLED LATIN SMALL LETTER N
    LineBreakClass::ULB_AI, // 0x24DE  # CIRCLED LATIN SMALL LETTER O
    LineBreakClass::ULB_AI, // 0x24DF  # CIRCLED LATIN SMALL LETTER P
    LineBreakClass::ULB_AI, // 0x24E0  # CIRCLED LATIN SMALL LETTER Q
    LineBreakClass::ULB_AI, // 0x24E1  # CIRCLED LATIN SMALL LETTER R
    LineBreakClass::ULB_AI, // 0x24E2  # CIRCLED LATIN SMALL LETTER S
    LineBreakClass::ULB_AI, // 0x24E3  # CIRCLED LATIN SMALL LETTER T
    LineBreakClass::ULB_AI, // 0x24E4  # CIRCLED LATIN SMALL LETTER U
    LineBreakClass::ULB_AI, // 0x24E5  # CIRCLED LATIN SMALL LETTER V
    LineBreakClass::ULB_AI, // 0x24E6  # CIRCLED LATIN SMALL LETTER W
    LineBreakClass::ULB_AI, // 0x24E7  # CIRCLED LATIN SMALL LETTER X
    LineBreakClass::ULB_AI, // 0x24E8  # CIRCLED LATIN SMALL LETTER Y
    LineBreakClass::ULB_AI, // 0x24E9  # CIRCLED LATIN SMALL LETTER Z
    LineBreakClass::ULB_AI, // 0x24EA  # CIRCLED DIGIT ZERO
    LineBreakClass::ULB_AI, // 0x24EB  # NEGATIVE CIRCLED NUMBER ELEVEN
    LineBreakClass::ULB_AI, // 0x24EC  # NEGATIVE CIRCLED NUMBER TWELVE
    LineBreakClass::ULB_AI, // 0x24ED  # NEGATIVE CIRCLED NUMBER THIRTEEN
    LineBreakClass::ULB_AI, // 0x24EE  # NEGATIVE CIRCLED NUMBER FOURTEEN
    LineBreakClass::ULB_AI, // 0x24EF  # NEGATIVE CIRCLED NUMBER FIFTEEN
    LineBreakClass::ULB_AI, // 0x24F0  # NEGATIVE CIRCLED NUMBER SIXTEEN
    LineBreakClass::ULB_AI, // 0x24F1  # NEGATIVE CIRCLED NUMBER SEVENTEEN
    LineBreakClass::ULB_AI, // 0x24F2  # NEGATIVE CIRCLED NUMBER EIGHTEEN
    LineBreakClass::ULB_AI, // 0x24F3  # NEGATIVE CIRCLED NUMBER NINETEEN
    LineBreakClass::ULB_AI, // 0x24F4  # NEGATIVE CIRCLED NUMBER TWENTY
    LineBreakClass::ULB_AI, // 0x24F5  # DOUBLE CIRCLED DIGIT ONE
    LineBreakClass::ULB_AI, // 0x24F6  # DOUBLE CIRCLED DIGIT TWO
    LineBreakClass::ULB_AI, // 0x24F7  # DOUBLE CIRCLED DIGIT THREE
    LineBreakClass::ULB_AI, // 0x24F8  # DOUBLE CIRCLED DIGIT FOUR
    LineBreakClass::ULB_AI, // 0x24F9  # DOUBLE CIRCLED DIGIT FIVE
    LineBreakClass::ULB_AI, // 0x24FA  # DOUBLE CIRCLED DIGIT SIX
    LineBreakClass::ULB_AI, // 0x24FB  # DOUBLE CIRCLED DIGIT SEVEN
    LineBreakClass::ULB_AI, // 0x24FC  # DOUBLE CIRCLED DIGIT EIGHT
    LineBreakClass::ULB_AI, // 0x24FD  # DOUBLE CIRCLED DIGIT NINE
    LineBreakClass::ULB_AI, // 0x24FE  # DOUBLE CIRCLED NUMBER TEN
    LineBreakClass::ULB_AL, // 0x24FF  # NEGATIVE CIRCLED DIGIT ZERO
    LineBreakClass::ULB_AI, // 0x2500  # BOX DRAWINGS LIGHT HORIZONTAL
    LineBreakClass::ULB_AI, // 0x2501  # BOX DRAWINGS HEAVY HORIZONTAL
    LineBreakClass::ULB_AI, // 0x2502  # BOX DRAWINGS LIGHT VERTICAL
    LineBreakClass::ULB_AI, // 0x2503  # BOX DRAWINGS HEAVY VERTICAL
    LineBreakClass::ULB_AI, // 0x2504  # BOX DRAWINGS LIGHT TRIPLE DASH HORIZONTAL
    LineBreakClass::ULB_AI, // 0x2505  # BOX DRAWINGS HEAVY TRIPLE DASH HORIZONTAL
    LineBreakClass::ULB_AI, // 0x2506  # BOX DRAWINGS LIGHT TRIPLE DASH VERTICAL
    LineBreakClass::ULB_AI, // 0x2507  # BOX DRAWINGS HEAVY TRIPLE DASH VERTICAL
    LineBreakClass::ULB_AI, // 0x2508  # BOX DRAWINGS LIGHT QUADRUPLE DASH HORIZONTAL
    LineBreakClass::ULB_AI, // 0x2509  # BOX DRAWINGS HEAVY QUADRUPLE DASH HORIZONTAL
    LineBreakClass::ULB_AI, // 0x250A  # BOX DRAWINGS LIGHT QUADRUPLE DASH VERTICAL
    LineBreakClass::ULB_AI, // 0x250B  # BOX DRAWINGS HEAVY QUADRUPLE DASH VERTICAL
    LineBreakClass::ULB_AI, // 0x250C  # BOX DRAWINGS LIGHT DOWN AND RIGHT
    LineBreakClass::ULB_AI, // 0x250D  # BOX DRAWINGS DOWN LIGHT AND RIGHT HEAVY
    LineBreakClass::ULB_AI, // 0x250E  # BOX DRAWINGS DOWN HEAVY AND RIGHT LIGHT
    LineBreakClass::ULB_AI, // 0x250F  # BOX DRAWINGS HEAVY DOWN AND RIGHT
    LineBreakClass::ULB_AI, // 0x2510  # BOX DRAWINGS LIGHT DOWN AND LEFT
    LineBreakClass::ULB_AI, // 0x2511  # BOX DRAWINGS DOWN LIGHT AND LEFT HEAVY
    LineBreakClass::ULB_AI, // 0x2512  # BOX DRAWINGS DOWN HEAVY AND LEFT LIGHT
    LineBreakClass::ULB_AI, // 0x2513  # BOX DRAWINGS HEAVY DOWN AND LEFT
    LineBreakClass::ULB_AI, // 0x2514  # BOX DRAWINGS LIGHT UP AND RIGHT
    LineBreakClass::ULB_AI, // 0x2515  # BOX DRAWINGS UP LIGHT AND RIGHT HEAVY
    LineBreakClass::ULB_AI, // 0x2516  # BOX DRAWINGS UP HEAVY AND RIGHT LIGHT
    LineBreakClass::ULB_AI, // 0x2517  # BOX DRAWINGS HEAVY UP AND RIGHT
    LineBreakClass::ULB_AI, // 0x2518  # BOX DRAWINGS LIGHT UP AND LEFT
    LineBreakClass::ULB_AI, // 0x2519  # BOX DRAWINGS UP LIGHT AND LEFT HEAVY
    LineBreakClass::ULB_AI, // 0x251A  # BOX DRAWINGS UP HEAVY AND LEFT LIGHT
    LineBreakClass::ULB_AI, // 0x251B  # BOX DRAWINGS HEAVY UP AND LEFT
    LineBreakClass::ULB_AI, // 0x251C  # BOX DRAWINGS LIGHT VERTICAL AND RIGHT
    LineBreakClass::ULB_AI, // 0x251D  # BOX DRAWINGS VERTICAL LIGHT AND RIGHT HEAVY
    LineBreakClass::ULB_AI, // 0x251E  # BOX DRAWINGS UP HEAVY AND RIGHT DOWN LIGHT
    LineBreakClass::ULB_AI, // 0x251F  # BOX DRAWINGS DOWN HEAVY AND RIGHT UP LIGHT
    LineBreakClass::ULB_AI, // 0x2520  # BOX DRAWINGS VERTICAL HEAVY AND RIGHT LIGHT
    LineBreakClass::ULB_AI, // 0x2521  # BOX DRAWINGS DOWN LIGHT AND RIGHT UP HEAVY
    LineBreakClass::ULB_AI, // 0x2522  # BOX DRAWINGS UP LIGHT AND RIGHT DOWN HEAVY
    LineBreakClass::ULB_AI, // 0x2523  # BOX DRAWINGS HEAVY VERTICAL AND RIGHT
    LineBreakClass::ULB_AI, // 0x2524  # BOX DRAWINGS LIGHT VERTICAL AND LEFT
    LineBreakClass::ULB_AI, // 0x2525  # BOX DRAWINGS VERTICAL LIGHT AND LEFT HEAVY
    LineBreakClass::ULB_AI, // 0x2526  # BOX DRAWINGS UP HEAVY AND LEFT DOWN LIGHT
    LineBreakClass::ULB_AI, // 0x2527  # BOX DRAWINGS DOWN HEAVY AND LEFT UP LIGHT
    LineBreakClass::ULB_AI, // 0x2528  # BOX DRAWINGS VERTICAL HEAVY AND LEFT LIGHT
    LineBreakClass::ULB_AI, // 0x2529  # BOX DRAWINGS DOWN LIGHT AND LEFT UP HEAVY
    LineBreakClass::ULB_AI, // 0x252A  # BOX DRAWINGS UP LIGHT AND LEFT DOWN HEAVY
    LineBreakClass::ULB_AI, // 0x252B  # BOX DRAWINGS HEAVY VERTICAL AND LEFT
    LineBreakClass::ULB_AI, // 0x252C  # BOX DRAWINGS LIGHT DOWN AND HORIZONTAL
    LineBreakClass::ULB_AI, // 0x252D  # BOX DRAWINGS LEFT HEAVY AND RIGHT DOWN LIGHT
    LineBreakClass::ULB_AI, // 0x252E  # BOX DRAWINGS RIGHT HEAVY AND LEFT DOWN LIGHT
    LineBreakClass::ULB_AI, // 0x252F  # BOX DRAWINGS DOWN LIGHT AND HORIZONTAL HEAVY
    LineBreakClass::ULB_AI, // 0x2530  # BOX DRAWINGS DOWN HEAVY AND HORIZONTAL LIGHT
    LineBreakClass::ULB_AI, // 0x2531  # BOX DRAWINGS RIGHT LIGHT AND LEFT DOWN HEAVY
    LineBreakClass::ULB_AI, // 0x2532  # BOX DRAWINGS LEFT LIGHT AND RIGHT DOWN HEAVY
    LineBreakClass::ULB_AI, // 0x2533  # BOX DRAWINGS HEAVY DOWN AND HORIZONTAL
    LineBreakClass::ULB_AI, // 0x2534  # BOX DRAWINGS LIGHT UP AND HORIZONTAL
    LineBreakClass::ULB_AI, // 0x2535  # BOX DRAWINGS LEFT HEAVY AND RIGHT UP LIGHT
    LineBreakClass::ULB_AI, // 0x2536  # BOX DRAWINGS RIGHT HEAVY AND LEFT UP LIGHT
    LineBreakClass::ULB_AI, // 0x2537  # BOX DRAWINGS UP LIGHT AND HORIZONTAL HEAVY
    LineBreakClass::ULB_AI, // 0x2538  # BOX DRAWINGS UP HEAVY AND HORIZONTAL LIGHT
    LineBreakClass::ULB_AI, // 0x2539  # BOX DRAWINGS RIGHT LIGHT AND LEFT UP HEAVY
    LineBreakClass::ULB_AI, // 0x253A  # BOX DRAWINGS LEFT LIGHT AND RIGHT UP HEAVY
    LineBreakClass::ULB_AI, // 0x253B  # BOX DRAWINGS HEAVY UP AND HORIZONTAL
    LineBreakClass::ULB_AI, // 0x253C  # BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL
    LineBreakClass::ULB_AI, // 0x253D  # BOX DRAWINGS LEFT HEAVY AND RIGHT VERTICAL LIGHT
    LineBreakClass::ULB_AI, // 0x253E  # BOX DRAWINGS RIGHT HEAVY AND LEFT VERTICAL LIGHT
    LineBreakClass::ULB_AI, // 0x253F  # BOX DRAWINGS VERTICAL LIGHT AND HORIZONTAL HEAVY
    LineBreakClass::ULB_AI, // 0x2540  # BOX DRAWINGS UP HEAVY AND DOWN HORIZONTAL LIGHT
    LineBreakClass::ULB_AI, // 0x2541  # BOX DRAWINGS DOWN HEAVY AND UP HORIZONTAL LIGHT
    LineBreakClass::ULB_AI, // 0x2542  # BOX DRAWINGS VERTICAL HEAVY AND HORIZONTAL LIGHT
    LineBreakClass::ULB_AI, // 0x2543  # BOX DRAWINGS LEFT UP HEAVY AND RIGHT DOWN LIGHT
    LineBreakClass::ULB_AI, // 0x2544  # BOX DRAWINGS RIGHT UP HEAVY AND LEFT DOWN LIGHT
    LineBreakClass::ULB_AI, // 0x2545  # BOX DRAWINGS LEFT DOWN HEAVY AND RIGHT UP LIGHT
    LineBreakClass::ULB_AI, // 0x2546  # BOX DRAWINGS RIGHT DOWN HEAVY AND LEFT UP LIGHT
    LineBreakClass::ULB_AI, // 0x2547  # BOX DRAWINGS DOWN LIGHT AND UP HORIZONTAL HEAVY
    LineBreakClass::ULB_AI, // 0x2548  # BOX DRAWINGS UP LIGHT AND DOWN HORIZONTAL HEAVY
    LineBreakClass::ULB_AI, // 0x2549  # BOX DRAWINGS RIGHT LIGHT AND LEFT VERTICAL HEAVY
    LineBreakClass::ULB_AI, // 0x254A  # BOX DRAWINGS LEFT LIGHT AND RIGHT VERTICAL HEAVY
    LineBreakClass::ULB_AI, // 0x254B  # BOX DRAWINGS HEAVY VERTICAL AND HORIZONTAL
    LineBreakClass::ULB_AL, // 0x254C  # BOX DRAWINGS LIGHT DOUBLE DASH HORIZONTAL
    LineBreakClass::ULB_AL, // 0x254D  # BOX DRAWINGS HEAVY DOUBLE DASH HORIZONTAL
    LineBreakClass::ULB_AL, // 0x254E  # BOX DRAWINGS LIGHT DOUBLE DASH VERTICAL
    LineBreakClass::ULB_AL, // 0x254F  # BOX DRAWINGS HEAVY DOUBLE DASH VERTICAL
    LineBreakClass::ULB_AI, // 0x2550  # BOX DRAWINGS DOUBLE HORIZONTAL
    LineBreakClass::ULB_AI, // 0x2551  # BOX DRAWINGS DOUBLE VERTICAL
    LineBreakClass::ULB_AI, // 0x2552  # BOX DRAWINGS DOWN SINGLE AND RIGHT DOUBLE
    LineBreakClass::ULB_AI, // 0x2553  # BOX DRAWINGS DOWN DOUBLE AND RIGHT SINGLE
    LineBreakClass::ULB_AI, // 0x2554  # BOX DRAWINGS DOUBLE DOWN AND RIGHT
    LineBreakClass::ULB_AI, // 0x2555  # BOX DRAWINGS DOWN SINGLE AND LEFT DOUBLE
    LineBreakClass::ULB_AI, // 0x2556  # BOX DRAWINGS DOWN DOUBLE AND LEFT SINGLE
    LineBreakClass::ULB_AI, // 0x2557  # BOX DRAWINGS DOUBLE DOWN AND LEFT
    LineBreakClass::ULB_AI, // 0x2558  # BOX DRAWINGS UP SINGLE AND RIGHT DOUBLE
    LineBreakClass::ULB_AI, // 0x2559  # BOX DRAWINGS UP DOUBLE AND RIGHT SINGLE
    LineBreakClass::ULB_AI, // 0x255A  # BOX DRAWINGS DOUBLE UP AND RIGHT
    LineBreakClass::ULB_AI, // 0x255B  # BOX DRAWINGS UP SINGLE AND LEFT DOUBLE
    LineBreakClass::ULB_AI, // 0x255C  # BOX DRAWINGS UP DOUBLE AND LEFT SINGLE
    LineBreakClass::ULB_AI, // 0x255D  # BOX DRAWINGS DOUBLE UP AND LEFT
    LineBreakClass::ULB_AI, // 0x255E  # BOX DRAWINGS VERTICAL SINGLE AND RIGHT DOUBLE
    LineBreakClass::ULB_AI, // 0x255F  # BOX DRAWINGS VERTICAL DOUBLE AND RIGHT SINGLE
    LineBreakClass::ULB_AI, // 0x2560  # BOX DRAWINGS DOUBLE VERTICAL AND RIGHT
    LineBreakClass::ULB_AI, // 0x2561  # BOX DRAWINGS VERTICAL SINGLE AND LEFT DOUBLE
    LineBreakClass::ULB_AI, // 0x2562  # BOX DRAWINGS VERTICAL DOUBLE AND LEFT SINGLE
    LineBreakClass::ULB_AI, // 0x2563  # BOX DRAWINGS DOUBLE VERTICAL AND LEFT
    LineBreakClass::ULB_AI, // 0x2564  # BOX DRAWINGS DOWN SINGLE AND HORIZONTAL DOUBLE
    LineBreakClass::ULB_AI, // 0x2565  # BOX DRAWINGS DOWN DOUBLE AND HORIZONTAL SINGLE
    LineBreakClass::ULB_AI, // 0x2566  # BOX DRAWINGS DOUBLE DOWN AND HORIZONTAL
    LineBreakClass::ULB_AI, // 0x2567  # BOX DRAWINGS UP SINGLE AND HORIZONTAL DOUBLE
    LineBreakClass::ULB_AI, // 0x2568  # BOX DRAWINGS UP DOUBLE AND HORIZONTAL SINGLE
    LineBreakClass::ULB_AI, // 0x2569  # BOX DRAWINGS DOUBLE UP AND HORIZONTAL
    LineBreakClass::ULB_AI, // 0x256A  # BOX DRAWINGS VERTICAL SINGLE AND HORIZONTAL DOUBLE
    LineBreakClass::ULB_AI, // 0x256B  # BOX DRAWINGS VERTICAL DOUBLE AND HORIZONTAL SINGLE
    LineBreakClass::ULB_AI, // 0x256C  # BOX DRAWINGS DOUBLE VERTICAL AND HORIZONTAL
    LineBreakClass::ULB_AI, // 0x256D  # BOX DRAWINGS LIGHT ARC DOWN AND RIGHT
    LineBreakClass::ULB_AI, // 0x256E  # BOX DRAWINGS LIGHT ARC DOWN AND LEFT
    LineBreakClass::ULB_AI, // 0x256F  # BOX DRAWINGS LIGHT ARC UP AND LEFT
    LineBreakClass::ULB_AI, // 0x2570  # BOX DRAWINGS LIGHT ARC UP AND RIGHT
    LineBreakClass::ULB_AI, // 0x2571  # BOX DRAWINGS LIGHT DIAGONAL UPPER RIGHT TO LOWER LEFT
    LineBreakClass::ULB_AI, // 0x2572  # BOX DRAWINGS LIGHT DIAGONAL UPPER LEFT TO LOWER RIGHT
    LineBreakClass::ULB_AI, // 0x2573  # BOX DRAWINGS LIGHT DIAGONAL CROSS
    LineBreakClass::ULB_AI, // 0x2574  # BOX DRAWINGS LIGHT LEFT
    LineBreakClass::ULB_AL, // 0x2575  # BOX DRAWINGS LIGHT UP
    LineBreakClass::ULB_AL, // 0x2576  # BOX DRAWINGS LIGHT RIGHT
    LineBreakClass::ULB_AL, // 0x2577  # BOX DRAWINGS LIGHT DOWN
    LineBreakClass::ULB_AL, // 0x2578  # BOX DRAWINGS HEAVY LEFT
    LineBreakClass::ULB_AL, // 0x2579  # BOX DRAWINGS HEAVY UP
    LineBreakClass::ULB_AL, // 0x257A  # BOX DRAWINGS HEAVY RIGHT
    LineBreakClass::ULB_AL, // 0x257B  # BOX DRAWINGS HEAVY DOWN
    LineBreakClass::ULB_AL, // 0x257C  # BOX DRAWINGS LIGHT LEFT AND HEAVY RIGHT
    LineBreakClass::ULB_AL, // 0x257D  # BOX DRAWINGS LIGHT UP AND HEAVY DOWN
    LineBreakClass::ULB_AL, // 0x257E  # BOX DRAWINGS HEAVY LEFT AND LIGHT RIGHT
    LineBreakClass::ULB_AL, // 0x257F  # BOX DRAWINGS HEAVY UP AND LIGHT DOWN
    LineBreakClass::ULB_AI, // 0x2580  # UPPER HALF BLOCK
    LineBreakClass::ULB_AI, // 0x2581  # LOWER ONE EIGHTH BLOCK
    LineBreakClass::ULB_AI, // 0x2582  # LOWER ONE QUARTER BLOCK
    LineBreakClass::ULB_AI, // 0x2583  # LOWER THREE EIGHTHS BLOCK
    LineBreakClass::ULB_AI, // 0x2584  # LOWER HALF BLOCK
    LineBreakClass::ULB_AI, // 0x2585  # LOWER FIVE EIGHTHS BLOCK
    LineBreakClass::ULB_AI, // 0x2586  # LOWER THREE QUARTERS BLOCK
    LineBreakClass::ULB_AI, // 0x2587  # LOWER SEVEN EIGHTHS BLOCK
    LineBreakClass::ULB_AI, // 0x2588  # FULL BLOCK
    LineBreakClass::ULB_AI, // 0x2589  # LEFT SEVEN EIGHTHS BLOCK
    LineBreakClass::ULB_AI, // 0x258A  # LEFT THREE QUARTERS BLOCK
    LineBreakClass::ULB_AI, // 0x258B  # LEFT FIVE EIGHTHS BLOCK
    LineBreakClass::ULB_AI, // 0x258C  # LEFT HALF BLOCK
    LineBreakClass::ULB_AI, // 0x258D  # LEFT THREE EIGHTHS BLOCK
    LineBreakClass::ULB_AI, // 0x258E  # LEFT ONE QUARTER BLOCK
    LineBreakClass::ULB_AI, // 0x258F  # LEFT ONE EIGHTH BLOCK
    LineBreakClass::ULB_AL, // 0x2590  # RIGHT HALF BLOCK
    LineBreakClass::ULB_AL, // 0x2591  # LIGHT SHADE
    LineBreakClass::ULB_AI, // 0x2592  # MEDIUM SHADE
    LineBreakClass::ULB_AI, // 0x2593  # DARK SHADE
    LineBreakClass::ULB_AI, // 0x2594  # UPPER ONE EIGHTH BLOCK
    LineBreakClass::ULB_AI, // 0x2595  # RIGHT ONE EIGHTH BLOCK
    LineBreakClass::ULB_AL, // 0x2596  # QUADRANT LOWER LEFT
    LineBreakClass::ULB_AL, // 0x2597  # QUADRANT LOWER RIGHT
    LineBreakClass::ULB_AL, // 0x2598  # QUADRANT UPPER LEFT
    LineBreakClass::ULB_AL, // 0x2599  # QUADRANT UPPER LEFT AND LOWER LEFT AND LOWER RIGHT
    LineBreakClass::ULB_AL, // 0x259A  # QUADRANT UPPER LEFT AND LOWER RIGHT
    LineBreakClass::ULB_AL, // 0x259B  # QUADRANT UPPER LEFT AND UPPER RIGHT AND LOWER LEFT
    LineBreakClass::ULB_AL, // 0x259C  # QUADRANT UPPER LEFT AND UPPER RIGHT AND LOWER RIGHT
    LineBreakClass::ULB_AL, // 0x259D  # QUADRANT UPPER RIGHT
    LineBreakClass::ULB_AL, // 0x259E  # QUADRANT UPPER RIGHT AND LOWER LEFT
    LineBreakClass::ULB_AL, // 0x259F  # QUADRANT UPPER RIGHT AND LOWER LEFT AND LOWER RIGHT
    LineBreakClass::ULB_AI, // 0x25A0  # BLACK SQUARE
    LineBreakClass::ULB_AI, // 0x25A1  # WHITE SQUARE
    LineBreakClass::ULB_AL, // 0x25A2  # WHITE SQUARE WITH ROUNDED CORNERS
    LineBreakClass::ULB_AI, // 0x25A3  # WHITE SQUARE CONTAINING BLACK SMALL SQUARE
    LineBreakClass::ULB_AI, // 0x25A4  # SQUARE WITH HORIZONTAL FILL
    LineBreakClass::ULB_AI, // 0x25A5  # SQUARE WITH VERTICAL FILL
    LineBreakClass::ULB_AI, // 0x25A6  # SQUARE WITH ORTHOGONAL CROSSHATCH FILL
    LineBreakClass::ULB_AI, // 0x25A7  # SQUARE WITH UPPER LEFT TO LOWER RIGHT FILL
    LineBreakClass::ULB_AI, // 0x25A8  # SQUARE WITH UPPER RIGHT TO LOWER LEFT FILL
    LineBreakClass::ULB_AI, // 0x25A9  # SQUARE WITH DIAGONAL CROSSHATCH FILL
    LineBreakClass::ULB_AL, // 0x25AA  # BLACK SMALL SQUARE
    LineBreakClass::ULB_AL, // 0x25AB  # WHITE SMALL SQUARE
    LineBreakClass::ULB_AL, // 0x25AC  # BLACK RECTANGLE
    LineBreakClass::ULB_AL, // 0x25AD  # WHITE RECTANGLE
    LineBreakClass::ULB_AL, // 0x25AE  # BLACK VERTICAL RECTANGLE
    LineBreakClass::ULB_AL, // 0x25AF  # WHITE VERTICAL RECTANGLE
    LineBreakClass::ULB_AL, // 0x25B0  # BLACK PARALLELOGRAM
    LineBreakClass::ULB_AL, // 0x25B1  # WHITE PARALLELOGRAM
    LineBreakClass::ULB_AI, // 0x25B2  # BLACK UP-POINTING TRIANGLE
    LineBreakClass::ULB_AI, // 0x25B3  # WHITE UP-POINTING TRIANGLE
    LineBreakClass::ULB_AL, // 0x25B4  # BLACK UP-POINTING SMALL TRIANGLE
    LineBreakClass::ULB_AL, // 0x25B5  # WHITE UP-POINTING SMALL TRIANGLE
    LineBreakClass::ULB_AI, // 0x25B6  # BLACK RIGHT-POINTING TRIANGLE
    LineBreakClass::ULB_AI, // 0x25B7  # WHITE RIGHT-POINTING TRIANGLE
    LineBreakClass::ULB_AL, // 0x25B8  # BLACK RIGHT-POINTING SMALL TRIANGLE
    LineBreakClass::ULB_AL, // 0x25B9  # WHITE RIGHT-POINTING SMALL TRIANGLE
    LineBreakClass::ULB_AL, // 0x25BA  # BLACK RIGHT-POINTING POINTER
    LineBreakClass::ULB_AL, // 0x25BB  # WHITE RIGHT-POINTING POINTER
    LineBreakClass::ULB_AI, // 0x25BC  # BLACK DOWN-POINTING TRIANGLE
    LineBreakClass::ULB_AI, // 0x25BD  # WHITE DOWN-POINTING TRIANGLE
    LineBreakClass::ULB_AL, // 0x25BE  # BLACK DOWN-POINTING SMALL TRIANGLE
    LineBreakClass::ULB_AL, // 0x25BF  # WHITE DOWN-POINTING SMALL TRIANGLE
    LineBreakClass::ULB_AI, // 0x25C0  # BLACK LEFT-POINTING TRIANGLE
    LineBreakClass::ULB_AI, // 0x25C1  # WHITE LEFT-POINTING TRIANGLE
    LineBreakClass::ULB_AL, // 0x25C2  # BLACK LEFT-POINTING SMALL TRIANGLE
    LineBreakClass::ULB_AL, // 0x25C3  # WHITE LEFT-POINTING SMALL TRIANGLE
    LineBreakClass::ULB_AL, // 0x25C4  # BLACK LEFT-POINTING POINTER
    LineBreakClass::ULB_AL, // 0x25C5  # WHITE LEFT-POINTING POINTER
    LineBreakClass::ULB_AI, // 0x25C6  # BLACK DIAMOND
    LineBreakClass::ULB_AI, // 0x25C7  # WHITE DIAMOND
    LineBreakClass::ULB_AI, // 0x25C8  # WHITE DIAMOND CONTAINING BLACK SMALL DIAMOND
    LineBreakClass::ULB_AL, // 0x25C9  # FISHEYE
    LineBreakClass::ULB_AL, // 0x25CA  # LOZENGE
    LineBreakClass::ULB_AI, // 0x25CB  # WHITE CIRCLE
    LineBreakClass::ULB_AL, // 0x25CC  # DOTTED CIRCLE
    LineBreakClass::ULB_AL, // 0x25CD  # CIRCLE WITH VERTICAL FILL
    LineBreakClass::ULB_AI, // 0x25CE  # BULLSEYE
    LineBreakClass::ULB_AI, // 0x25CF  # BLACK CIRCLE
    LineBreakClass::ULB_AI, // 0x25D0  # CIRCLE WITH LEFT HALF BLACK
    LineBreakClass::ULB_AI, // 0x25D1  # CIRCLE WITH RIGHT HALF BLACK
    LineBreakClass::ULB_AL, // 0x25D2  # CIRCLE WITH LOWER HALF BLACK
    LineBreakClass::ULB_AL, // 0x25D3  # CIRCLE WITH UPPER HALF BLACK
    LineBreakClass::ULB_AL, // 0x25D4  # CIRCLE WITH UPPER RIGHT QUADRANT BLACK
    LineBreakClass::ULB_AL, // 0x25D5  # CIRCLE WITH ALL BUT UPPER LEFT QUADRANT BLACK
    LineBreakClass::ULB_AL, // 0x25D6  # LEFT HALF BLACK CIRCLE
    LineBreakClass::ULB_AL, // 0x25D7  # RIGHT HALF BLACK CIRCLE
    LineBreakClass::ULB_AL, // 0x25D8  # INVERSE BULLET
    LineBreakClass::ULB_AL, // 0x25D9  # INVERSE WHITE CIRCLE
    LineBreakClass::ULB_AL, // 0x25DA  # UPPER HALF INVERSE WHITE CIRCLE
    LineBreakClass::ULB_AL, // 0x25DB  # LOWER HALF INVERSE WHITE CIRCLE
    LineBreakClass::ULB_AL, // 0x25DC  # UPPER LEFT QUADRANT CIRCULAR ARC
    LineBreakClass::ULB_AL, // 0x25DD  # UPPER RIGHT QUADRANT CIRCULAR ARC
    LineBreakClass::ULB_AL, // 0x25DE  # LOWER RIGHT QUADRANT CIRCULAR ARC
    LineBreakClass::ULB_AL, // 0x25DF  # LOWER LEFT QUADRANT CIRCULAR ARC
    LineBreakClass::ULB_AL, // 0x25E0  # UPPER HALF CIRCLE
    LineBreakClass::ULB_AL, // 0x25E1  # LOWER HALF CIRCLE
    LineBreakClass::ULB_AI, // 0x25E2  # BLACK LOWER RIGHT TRIANGLE
    LineBreakClass::ULB_AI, // 0x25E3  # BLACK LOWER LEFT TRIANGLE
    LineBreakClass::ULB_AI, // 0x25E4  # BLACK UPPER LEFT TRIANGLE
    LineBreakClass::ULB_AI, // 0x25E5  # BLACK UPPER RIGHT TRIANGLE
    LineBreakClass::ULB_AL, // 0x25E6  # WHITE BULLET
    LineBreakClass::ULB_AL, // 0x25E7  # SQUARE WITH LEFT HALF BLACK
    LineBreakClass::ULB_AL, // 0x25E8  # SQUARE WITH RIGHT HALF BLACK
    LineBreakClass::ULB_AL, // 0x25E9  # SQUARE WITH UPPER LEFT DIAGONAL HALF BLACK
    LineBreakClass::ULB_AL, // 0x25EA  # SQUARE WITH LOWER RIGHT DIAGONAL HALF BLACK
    LineBreakClass::ULB_AL, // 0x25EB  # WHITE SQUARE WITH VERTICAL BISECTING LINE
    LineBreakClass::ULB_AL, // 0x25EC  # WHITE UP-POINTING TRIANGLE WITH DOT
    LineBreakClass::ULB_AL, // 0x25ED  # UP-POINTING TRIANGLE WITH LEFT HALF BLACK
    LineBreakClass::ULB_AL, // 0x25EE  # UP-POINTING TRIANGLE WITH RIGHT HALF BLACK
    LineBreakClass::ULB_AI, // 0x25EF  # LARGE CIRCLE
    LineBreakClass::ULB_AL, // 0x25F0  # WHITE SQUARE WITH UPPER LEFT QUADRANT
    LineBreakClass::ULB_AL, // 0x25F1  # WHITE SQUARE WITH LOWER LEFT QUADRANT
    LineBreakClass::ULB_AL, // 0x25F2  # WHITE SQUARE WITH LOWER RIGHT QUADRANT
    LineBreakClass::ULB_AL, // 0x25F3  # WHITE SQUARE WITH UPPER RIGHT QUADRANT
    LineBreakClass::ULB_AL, // 0x25F4  # WHITE CIRCLE WITH UPPER LEFT QUADRANT
    LineBreakClass::ULB_AL, // 0x25F5  # WHITE CIRCLE WITH LOWER LEFT QUADRANT
    LineBreakClass::ULB_AL, // 0x25F6  # WHITE CIRCLE WITH LOWER RIGHT QUADRANT
    LineBreakClass::ULB_AL, // 0x25F7  # WHITE CIRCLE WITH UPPER RIGHT QUADRANT
    LineBreakClass::ULB_AL, // 0x25F8  # UPPER LEFT TRIANGLE
    LineBreakClass::ULB_AL, // 0x25F9  # UPPER RIGHT TRIANGLE
    LineBreakClass::ULB_AL, // 0x25FA  # LOWER LEFT TRIANGLE
    LineBreakClass::ULB_AL, // 0x25FB  # WHITE MEDIUM SQUARE
    LineBreakClass::ULB_AL, // 0x25FC  # BLACK MEDIUM SQUARE
    LineBreakClass::ULB_AL, // 0x25FD  # WHITE MEDIUM SMALL SQUARE
    LineBreakClass::ULB_AL, // 0x25FE  # BLACK MEDIUM SMALL SQUARE
    LineBreakClass::ULB_AL, // 0x25FF  # LOWER RIGHT TRIANGLE
    LineBreakClass::ULB_AL, // 0x2600  # BLACK SUN WITH RAYS
    LineBreakClass::ULB_AL, // 0x2601  # CLOUD
    LineBreakClass::ULB_AL, // 0x2602  # UMBRELLA
    LineBreakClass::ULB_AL, // 0x2603  # SNOWMAN
    LineBreakClass::ULB_AL, // 0x2604  # COMET
    LineBreakClass::ULB_AI, // 0x2605  # BLACK STAR
    LineBreakClass::ULB_AI, // 0x2606  # WHITE STAR
    LineBreakClass::ULB_AL, // 0x2607  # LIGHTNING
    LineBreakClass::ULB_AL, // 0x2608  # THUNDERSTORM
    LineBreakClass::ULB_AI, // 0x2609  # SUN
    LineBreakClass::ULB_AL, // 0x260A  # ASCENDING NODE
    LineBreakClass::ULB_AL, // 0x260B  # DESCENDING NODE
    LineBreakClass::ULB_AL, // 0x260C  # CONJUNCTION
    LineBreakClass::ULB_AL, // 0x260D  # OPPOSITION
    LineBreakClass::ULB_AI, // 0x260E  # BLACK TELEPHONE
    LineBreakClass::ULB_AI, // 0x260F  # WHITE TELEPHONE
    LineBreakClass::ULB_AL, // 0x2610  # BALLOT BOX
    LineBreakClass::ULB_AL, // 0x2611  # BALLOT BOX WITH CHECK
    LineBreakClass::ULB_AL, // 0x2612  # BALLOT BOX WITH X
    LineBreakClass::ULB_AL, // 0x2613  # SALTIRE
    LineBreakClass::ULB_AI, // 0x2614  # UMBRELLA WITH RAIN DROPS
    LineBreakClass::ULB_AI, // 0x2615  # HOT BEVERAGE
    LineBreakClass::ULB_AI, // 0x2616  # WHITE SHOGI PIECE
    LineBreakClass::ULB_AI, // 0x2617  # BLACK SHOGI PIECE
    LineBreakClass::ULB_AL, // 0x2618  # SHAMROCK
    LineBreakClass::ULB_AL, // 0x2619  # REVERSED ROTATED FLORAL HEART BULLET
    LineBreakClass::ULB_AL, // 0x261A  # BLACK LEFT POINTING INDEX
    LineBreakClass::ULB_AL, // 0x261B  # BLACK RIGHT POINTING INDEX
    LineBreakClass::ULB_AI, // 0x261C  # WHITE LEFT POINTING INDEX
    LineBreakClass::ULB_AL, // 0x261D  # WHITE UP POINTING INDEX
    LineBreakClass::ULB_AI, // 0x261E  # WHITE RIGHT POINTING INDEX
    LineBreakClass::ULB_AL, // 0x261F  # WHITE DOWN POINTING INDEX
    LineBreakClass::ULB_AL, // 0x2620  # SKULL AND CROSSBONES
    LineBreakClass::ULB_AL, // 0x2621  # CAUTION SIGN
    LineBreakClass::ULB_AL, // 0x2622  # RADIOACTIVE SIGN
    LineBreakClass::ULB_AL, // 0x2623  # BIOHAZARD SIGN
    LineBreakClass::ULB_AL, // 0x2624  # CADUCEUS
    LineBreakClass::ULB_AL, // 0x2625  # ANKH
    LineBreakClass::ULB_AL, // 0x2626  # ORTHODOX CROSS
    LineBreakClass::ULB_AL, // 0x2627  # CHI RHO
    LineBreakClass::ULB_AL, // 0x2628  # CROSS OF LORRAINE
    LineBreakClass::ULB_AL, // 0x2629  # CROSS OF JERUSALEM
    LineBreakClass::ULB_AL, // 0x262A  # STAR AND CRESCENT
    LineBreakClass::ULB_AL, // 0x262B  # FARSI SYMBOL
    LineBreakClass::ULB_AL, // 0x262C  # ADI SHAKTI
    LineBreakClass::ULB_AL, // 0x262D  # HAMMER AND SICKLE
    LineBreakClass::ULB_AL, // 0x262E  # PEACE SYMBOL
    LineBreakClass::ULB_AL, // 0x262F  # YIN YANG
    LineBreakClass::ULB_AL, // 0x2630  # TRIGRAM FOR HEAVEN
    LineBreakClass::ULB_AL, // 0x2631  # TRIGRAM FOR LAKE
    LineBreakClass::ULB_AL, // 0x2632  # TRIGRAM FOR FIRE
    LineBreakClass::ULB_AL, // 0x2633  # TRIGRAM FOR THUNDER
    LineBreakClass::ULB_AL, // 0x2634  # TRIGRAM FOR WIND
    LineBreakClass::ULB_AL, // 0x2635  # TRIGRAM FOR WATER
    LineBreakClass::ULB_AL, // 0x2636  # TRIGRAM FOR MOUNTAIN
    LineBreakClass::ULB_AL, // 0x2637  # TRIGRAM FOR EARTH
    LineBreakClass::ULB_AL, // 0x2638  # WHEEL OF DHARMA
    LineBreakClass::ULB_AL, // 0x2639  # WHITE FROWNING FACE
    LineBreakClass::ULB_AL, // 0x263A  # WHITE SMILING FACE
    LineBreakClass::ULB_AL, // 0x263B  # BLACK SMILING FACE
    LineBreakClass::ULB_AL, // 0x263C  # WHITE SUN WITH RAYS
    LineBreakClass::ULB_AL, // 0x263D  # FIRST QUARTER MOON
    LineBreakClass::ULB_AL, // 0x263E  # LAST QUARTER MOON
    LineBreakClass::ULB_AL, // 0x263F  # MERCURY
    LineBreakClass::ULB_AI, // 0x2640  # FEMALE SIGN
    LineBreakClass::ULB_AL, // 0x2641  # EARTH
    LineBreakClass::ULB_AI, // 0x2642  # MALE SIGN
    LineBreakClass::ULB_AL, // 0x2643  # JUPITER
    LineBreakClass::ULB_AL, // 0x2644  # SATURN
    LineBreakClass::ULB_AL, // 0x2645  # URANUS
    LineBreakClass::ULB_AL, // 0x2646  # NEPTUNE
    LineBreakClass::ULB_AL, // 0x2647  # PLUTO
    LineBreakClass::ULB_AL, // 0x2648  # ARIES
    LineBreakClass::ULB_AL, // 0x2649  # TAURUS
    LineBreakClass::ULB_AL, // 0x264A  # GEMINI
    LineBreakClass::ULB_AL, // 0x264B  # CANCER
    LineBreakClass::ULB_AL, // 0x264C  # LEO
    LineBreakClass::ULB_AL, // 0x264D  # VIRGO
    LineBreakClass::ULB_AL, // 0x264E  # LIBRA
    LineBreakClass::ULB_AL, // 0x264F  # SCORPIUS
    LineBreakClass::ULB_AL, // 0x2650  # SAGITTARIUS
    LineBreakClass::ULB_AL, // 0x2651  # CAPRICORN
    LineBreakClass::ULB_AL, // 0x2652  # AQUARIUS
    LineBreakClass::ULB_AL, // 0x2653  # PISCES
    LineBreakClass::ULB_AL, // 0x2654  # WHITE CHESS KING
    LineBreakClass::ULB_AL, // 0x2655  # WHITE CHESS QUEEN
    LineBreakClass::ULB_AL, // 0x2656  # WHITE CHESS ROOK
    LineBreakClass::ULB_AL, // 0x2657  # WHITE CHESS BISHOP
    LineBreakClass::ULB_AL, // 0x2658  # WHITE CHESS KNIGHT
    LineBreakClass::ULB_AL, // 0x2659  # WHITE CHESS PAWN
    LineBreakClass::ULB_AL, // 0x265A  # BLACK CHESS KING
    LineBreakClass::ULB_AL, // 0x265B  # BLACK CHESS QUEEN
    LineBreakClass::ULB_AL, // 0x265C  # BLACK CHESS ROOK
    LineBreakClass::ULB_AL, // 0x265D  # BLACK CHESS BISHOP
    LineBreakClass::ULB_AL, // 0x265E  # BLACK CHESS KNIGHT
    LineBreakClass::ULB_AL, // 0x265F  # BLACK CHESS PAWN
    LineBreakClass::ULB_AI, // 0x2660  # BLACK SPADE SUIT
    LineBreakClass::ULB_AI, // 0x2661  # WHITE HEART SUIT
    LineBreakClass::ULB_AL, // 0x2662  # WHITE DIAMOND SUIT
    LineBreakClass::ULB_AI, // 0x2663  # BLACK CLUB SUIT
    LineBreakClass::ULB_AI, // 0x2664  # WHITE SPADE SUIT
    LineBreakClass::ULB_AI, // 0x2665  # BLACK HEART SUIT
    LineBreakClass::ULB_AL, // 0x2666  # BLACK DIAMOND SUIT
    LineBreakClass::ULB_AI, // 0x2667  # WHITE CLUB SUIT
    LineBreakClass::ULB_AI, // 0x2668  # HOT SPRINGS
    LineBreakClass::ULB_AI, // 0x2669  # QUARTER NOTE
    LineBreakClass::ULB_AI, // 0x266A  # EIGHTH NOTE
    LineBreakClass::ULB_AL, // 0x266B  # BEAMED EIGHTH NOTES
    LineBreakClass::ULB_AI, // 0x266C  # BEAMED SIXTEENTH NOTES
    LineBreakClass::ULB_AI, // 0x266D  # MUSIC FLAT SIGN
    LineBreakClass::ULB_AL, // 0x266E  # MUSIC NATURAL SIGN
    LineBreakClass::ULB_AI, // 0x266F  # MUSIC SHARP SIGN
    LineBreakClass::ULB_AL, // 0x2670  # WEST SYRIAC CROSS
    LineBreakClass::ULB_AL, // 0x2671  # EAST SYRIAC CROSS
    LineBreakClass::ULB_AL, // 0x2672  # UNIVERSAL RECYCLING SYMBOL
    LineBreakClass::ULB_AL, // 0x2673  # RECYCLING SYMBOL FOR TYPE-1 PLASTICS
    LineBreakClass::ULB_AL, // 0x2674  # RECYCLING SYMBOL FOR TYPE-2 PLASTICS
    LineBreakClass::ULB_AL, // 0x2675  # RECYCLING SYMBOL FOR TYPE-3 PLASTICS
    LineBreakClass::ULB_AL, // 0x2676  # RECYCLING SYMBOL FOR TYPE-4 PLASTICS
    LineBreakClass::ULB_AL, // 0x2677  # RECYCLING SYMBOL FOR TYPE-5 PLASTICS
    LineBreakClass::ULB_AL, // 0x2678  # RECYCLING SYMBOL FOR TYPE-6 PLASTICS
    LineBreakClass::ULB_AL, // 0x2679  # RECYCLING SYMBOL FOR TYPE-7 PLASTICS
    LineBreakClass::ULB_AL, // 0x267A  # RECYCLING SYMBOL FOR GENERIC MATERIALS
    LineBreakClass::ULB_AL, // 0x267B  # BLACK UNIVERSAL RECYCLING SYMBOL
    LineBreakClass::ULB_AL, // 0x267C  # RECYCLED PAPER SYMBOL
    LineBreakClass::ULB_AL, // 0x267D  # PARTIALLY-RECYCLED PAPER SYMBOL
    LineBreakClass::ULB_AL, // 0x267E  # PERMANENT PAPER SIGN
    LineBreakClass::ULB_AL, // 0x267F  # WHEELCHAIR SYMBOL
    LineBreakClass::ULB_AL, // 0x2680  # DIE FACE-1
    LineBreakClass::ULB_AL, // 0x2681  # DIE FACE-2
    LineBreakClass::ULB_AL, // 0x2682  # DIE FACE-3
    LineBreakClass::ULB_AL, // 0x2683  # DIE FACE-4
    LineBreakClass::ULB_AL, // 0x2684  # DIE FACE-5
    LineBreakClass::ULB_AL, // 0x2685  # DIE FACE-6
    LineBreakClass::ULB_AL, // 0x2686  # WHITE CIRCLE WITH DOT RIGHT
    LineBreakClass::ULB_AL, // 0x2687  # WHITE CIRCLE WITH TWO DOTS
    LineBreakClass::ULB_AL, // 0x2688  # BLACK CIRCLE WITH WHITE DOT RIGHT
    LineBreakClass::ULB_AL, // 0x2689  # BLACK CIRCLE WITH TWO WHITE DOTS
    LineBreakClass::ULB_AL, // 0x268A  # MONOGRAM FOR YANG
    LineBreakClass::ULB_AL, // 0x268B  # MONOGRAM FOR YIN
    LineBreakClass::ULB_AL, // 0x268C  # DIGRAM FOR GREATER YANG
    LineBreakClass::ULB_AL, // 0x268D  # DIGRAM FOR LESSER YIN
    LineBreakClass::ULB_AL, // 0x268E  # DIGRAM FOR LESSER YANG
    LineBreakClass::ULB_AL, // 0x268F  # DIGRAM FOR GREATER YIN
    LineBreakClass::ULB_AL, // 0x2690  # WHITE FLAG
    LineBreakClass::ULB_AL, // 0x2691  # BLACK FLAG
    LineBreakClass::ULB_AL, // 0x2692  # HAMMER AND PICK
    LineBreakClass::ULB_AL, // 0x2693  # ANCHOR
    LineBreakClass::ULB_AL, // 0x2694  # CROSSED SWORDS
    LineBreakClass::ULB_AL, // 0x2695  # STAFF OF AESCULAPIUS
    LineBreakClass::ULB_AL, // 0x2696  # SCALES
    LineBreakClass::ULB_AL, // 0x2697  # ALEMBIC
    LineBreakClass::ULB_AL, // 0x2698  # FLOWER
    LineBreakClass::ULB_AL, // 0x2699  # GEAR
    LineBreakClass::ULB_AL, // 0x269A  # STAFF OF HERMES
    LineBreakClass::ULB_AL, // 0x269B  # ATOM SYMBOL
    LineBreakClass::ULB_AL, // 0x269C  # FLEUR-DE-LIS
    LineBreakClass::ULB_ID, // 0x269D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x269E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x269F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x26A0  # WARNING SIGN
    LineBreakClass::ULB_AL, // 0x26A1  # HIGH VOLTAGE SIGN
    LineBreakClass::ULB_AL, // 0x26A2  # DOUBLED FEMALE SIGN
    LineBreakClass::ULB_AL, // 0x26A3  # DOUBLED MALE SIGN
    LineBreakClass::ULB_AL, // 0x26A4  # INTERLOCKED FEMALE AND MALE SIGN
    LineBreakClass::ULB_AL, // 0x26A5  # MALE AND FEMALE SIGN
    LineBreakClass::ULB_AL, // 0x26A6  # MALE WITH STROKE SIGN
    LineBreakClass::ULB_AL, // 0x26A7  # MALE WITH STROKE AND MALE AND FEMALE SIGN
    LineBreakClass::ULB_AL, // 0x26A8  # VERTICAL MALE WITH STROKE SIGN
    LineBreakClass::ULB_AL, // 0x26A9  # HORIZONTAL MALE WITH STROKE SIGN
    LineBreakClass::ULB_AL, // 0x26AA  # MEDIUM WHITE CIRCLE
    LineBreakClass::ULB_AL, // 0x26AB  # MEDIUM BLACK CIRCLE
    LineBreakClass::ULB_AL, // 0x26AC  # MEDIUM SMALL WHITE CIRCLE
    LineBreakClass::ULB_AL, // 0x26AD  # MARRIAGE SYMBOL
    LineBreakClass::ULB_AL, // 0x26AE  # DIVORCE SYMBOL
    LineBreakClass::ULB_AL, // 0x26AF  # UNMARRIED PARTNERSHIP SYMBOL
    LineBreakClass::ULB_AL, // 0x26B0  # COFFIN
    LineBreakClass::ULB_AL, // 0x26B1  # FUNERAL URN
    LineBreakClass::ULB_AL, // 0x26B2  # NEUTER
    LineBreakClass::ULB_ID, // 0x26B3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26B4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26B5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26B6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26B7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26B8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26B9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26BA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26BB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26BC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26BD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26BE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26BF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26C0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26C1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26C2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26C3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26C4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26C5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26C6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26C7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26C8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26C9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26CA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26CB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26CC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26CD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26CE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26CF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26D0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26D1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26D2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26D3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26D4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26D5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26D6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26D7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26D8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26D9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26DA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26DB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26DC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26DD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26DE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26DF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26E0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26E1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26E2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26E3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26E4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26E5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26E6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26E7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26E8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26E9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26EA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26EB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26EC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26ED # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26EE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26EF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26F0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26F1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26F2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26F3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26F4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26F5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26F6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26F7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26F8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26F9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26FA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26FB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26FC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26FD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26FE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x26FF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2700 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2701  # UPPER BLADE SCISSORS
    LineBreakClass::ULB_AL, // 0x2702  # BLACK SCISSORS
    LineBreakClass::ULB_AL, // 0x2703  # LOWER BLADE SCISSORS
    LineBreakClass::ULB_AL, // 0x2704  # WHITE SCISSORS
    LineBreakClass::ULB_ID, // 0x2705 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2706  # TELEPHONE LOCATION SIGN
    LineBreakClass::ULB_AL, // 0x2707  # TAPE DRIVE
    LineBreakClass::ULB_AL, // 0x2708  # AIRPLANE
    LineBreakClass::ULB_AL, // 0x2709  # ENVELOPE
    LineBreakClass::ULB_ID, // 0x270A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x270B # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x270C  # VICTORY HAND
    LineBreakClass::ULB_AL, // 0x270D  # WRITING HAND
    LineBreakClass::ULB_AL, // 0x270E  # LOWER RIGHT PENCIL
    LineBreakClass::ULB_AL, // 0x270F  # PENCIL
    LineBreakClass::ULB_AL, // 0x2710  # UPPER RIGHT PENCIL
    LineBreakClass::ULB_AL, // 0x2711  # WHITE NIB
    LineBreakClass::ULB_AL, // 0x2712  # BLACK NIB
    LineBreakClass::ULB_AL, // 0x2713  # CHECK MARK
    LineBreakClass::ULB_AL, // 0x2714  # HEAVY CHECK MARK
    LineBreakClass::ULB_AL, // 0x2715  # MULTIPLICATION X
    LineBreakClass::ULB_AL, // 0x2716  # HEAVY MULTIPLICATION X
    LineBreakClass::ULB_AL, // 0x2717  # BALLOT X
    LineBreakClass::ULB_AL, // 0x2718  # HEAVY BALLOT X
    LineBreakClass::ULB_AL, // 0x2719  # OUTLINED GREEK CROSS
    LineBreakClass::ULB_AL, // 0x271A  # HEAVY GREEK CROSS
    LineBreakClass::ULB_AL, // 0x271B  # OPEN CENTRE CROSS
    LineBreakClass::ULB_AL, // 0x271C  # HEAVY OPEN CENTRE CROSS
    LineBreakClass::ULB_AL, // 0x271D  # LATIN CROSS
    LineBreakClass::ULB_AL, // 0x271E  # SHADOWED WHITE LATIN CROSS
    LineBreakClass::ULB_AL, // 0x271F  # OUTLINED LATIN CROSS
    LineBreakClass::ULB_AL, // 0x2720  # MALTESE CROSS
    LineBreakClass::ULB_AL, // 0x2721  # STAR OF DAVID
    LineBreakClass::ULB_AL, // 0x2722  # FOUR TEARDROP-SPOKED ASTERISK
    LineBreakClass::ULB_AL, // 0x2723  # FOUR BALLOON-SPOKED ASTERISK
    LineBreakClass::ULB_AL, // 0x2724  # HEAVY FOUR BALLOON-SPOKED ASTERISK
    LineBreakClass::ULB_AL, // 0x2725  # FOUR CLUB-SPOKED ASTERISK
    LineBreakClass::ULB_AL, // 0x2726  # BLACK FOUR POINTED STAR
    LineBreakClass::ULB_AL, // 0x2727  # WHITE FOUR POINTED STAR
    LineBreakClass::ULB_ID, // 0x2728 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2729  # STRESS OUTLINED WHITE STAR
    LineBreakClass::ULB_AL, // 0x272A  # CIRCLED WHITE STAR
    LineBreakClass::ULB_AL, // 0x272B  # OPEN CENTRE BLACK STAR
    LineBreakClass::ULB_AL, // 0x272C  # BLACK CENTRE WHITE STAR
    LineBreakClass::ULB_AL, // 0x272D  # OUTLINED BLACK STAR
    LineBreakClass::ULB_AL, // 0x272E  # HEAVY OUTLINED BLACK STAR
    LineBreakClass::ULB_AL, // 0x272F  # PINWHEEL STAR
    LineBreakClass::ULB_AL, // 0x2730  # SHADOWED WHITE STAR
    LineBreakClass::ULB_AL, // 0x2731  # HEAVY ASTERISK
    LineBreakClass::ULB_AL, // 0x2732  # OPEN CENTRE ASTERISK
    LineBreakClass::ULB_AL, // 0x2733  # EIGHT SPOKED ASTERISK
    LineBreakClass::ULB_AL, // 0x2734  # EIGHT POINTED BLACK STAR
    LineBreakClass::ULB_AL, // 0x2735  # EIGHT POINTED PINWHEEL STAR
    LineBreakClass::ULB_AL, // 0x2736  # SIX POINTED BLACK STAR
    LineBreakClass::ULB_AL, // 0x2737  # EIGHT POINTED RECTILINEAR BLACK STAR
    LineBreakClass::ULB_AL, // 0x2738  # HEAVY EIGHT POINTED RECTILINEAR BLACK STAR
    LineBreakClass::ULB_AL, // 0x2739  # TWELVE POINTED BLACK STAR
    LineBreakClass::ULB_AL, // 0x273A  # SIXTEEN POINTED ASTERISK
    LineBreakClass::ULB_AL, // 0x273B  # TEARDROP-SPOKED ASTERISK
    LineBreakClass::ULB_AL, // 0x273C  # OPEN CENTRE TEARDROP-SPOKED ASTERISK
    LineBreakClass::ULB_AL, // 0x273D  # HEAVY TEARDROP-SPOKED ASTERISK
    LineBreakClass::ULB_AL, // 0x273E  # SIX PETALLED BLACK AND WHITE FLORETTE
    LineBreakClass::ULB_AL, // 0x273F  # BLACK FLORETTE
    LineBreakClass::ULB_AL, // 0x2740  # WHITE FLORETTE
    LineBreakClass::ULB_AL, // 0x2741  # EIGHT PETALLED OUTLINED BLACK FLORETTE
    LineBreakClass::ULB_AL, // 0x2742  # CIRCLED OPEN CENTRE EIGHT POINTED STAR
    LineBreakClass::ULB_AL, // 0x2743  # HEAVY TEARDROP-SPOKED PINWHEEL ASTERISK
    LineBreakClass::ULB_AL, // 0x2744  # SNOWFLAKE
    LineBreakClass::ULB_AL, // 0x2745  # TIGHT TRIFOLIATE SNOWFLAKE
    LineBreakClass::ULB_AL, // 0x2746  # HEAVY CHEVRON SNOWFLAKE
    LineBreakClass::ULB_AL, // 0x2747  # SPARKLE
    LineBreakClass::ULB_AL, // 0x2748  # HEAVY SPARKLE
    LineBreakClass::ULB_AL, // 0x2749  # BALLOON-SPOKED ASTERISK
    LineBreakClass::ULB_AL, // 0x274A  # EIGHT TEARDROP-SPOKED PROPELLER ASTERISK
    LineBreakClass::ULB_AL, // 0x274B  # HEAVY EIGHT TEARDROP-SPOKED PROPELLER ASTERISK
    LineBreakClass::ULB_ID, // 0x274C # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x274D  # SHADOWED WHITE CIRCLE
    LineBreakClass::ULB_ID, // 0x274E # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x274F  # LOWER RIGHT DROP-SHADOWED WHITE SQUARE
    LineBreakClass::ULB_AL, // 0x2750  # UPPER RIGHT DROP-SHADOWED WHITE SQUARE
    LineBreakClass::ULB_AL, // 0x2751  # LOWER RIGHT SHADOWED WHITE SQUARE
    LineBreakClass::ULB_AL, // 0x2752  # UPPER RIGHT SHADOWED WHITE SQUARE
    LineBreakClass::ULB_ID, // 0x2753 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2754 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2755 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2756  # BLACK DIAMOND MINUS WHITE X
    LineBreakClass::ULB_ID, // 0x2757 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2758  # LIGHT VERTICAL BAR
    LineBreakClass::ULB_AL, // 0x2759  # MEDIUM VERTICAL BAR
    LineBreakClass::ULB_AL, // 0x275A  # HEAVY VERTICAL BAR
    LineBreakClass::ULB_QU, // 0x275B  # HEAVY SINGLE TURNED COMMA QUOTATION MARK ORNAMENT
    LineBreakClass::ULB_QU, // 0x275C  # HEAVY SINGLE COMMA QUOTATION MARK ORNAMENT
    LineBreakClass::ULB_QU, // 0x275D  # HEAVY DOUBLE TURNED COMMA QUOTATION MARK ORNAMENT
    LineBreakClass::ULB_QU, // 0x275E  # HEAVY DOUBLE COMMA QUOTATION MARK ORNAMENT
    LineBreakClass::ULB_ID, // 0x275F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2760 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2761  # CURVED STEM PARAGRAPH SIGN ORNAMENT
    LineBreakClass::ULB_EX, // 0x2762  # HEAVY EXCLAMATION MARK ORNAMENT
    LineBreakClass::ULB_EX, // 0x2763  # HEAVY HEART EXCLAMATION MARK ORNAMENT
    LineBreakClass::ULB_AL, // 0x2764  # HEAVY BLACK HEART
    LineBreakClass::ULB_AL, // 0x2765  # ROTATED HEAVY BLACK HEART BULLET
    LineBreakClass::ULB_AL, // 0x2766  # FLORAL HEART
    LineBreakClass::ULB_AL, // 0x2767  # ROTATED FLORAL HEART BULLET
    LineBreakClass::ULB_OP, // 0x2768  # MEDIUM LEFT PARENTHESIS ORNAMENT
    LineBreakClass::ULB_CL, // 0x2769  # MEDIUM RIGHT PARENTHESIS ORNAMENT
    LineBreakClass::ULB_OP, // 0x276A  # MEDIUM FLATTENED LEFT PARENTHESIS ORNAMENT
    LineBreakClass::ULB_CL, // 0x276B  # MEDIUM FLATTENED RIGHT PARENTHESIS ORNAMENT
    LineBreakClass::ULB_OP, // 0x276C  # MEDIUM LEFT-POINTING ANGLE BRACKET ORNAMENT
    LineBreakClass::ULB_CL, // 0x276D  # MEDIUM RIGHT-POINTING ANGLE BRACKET ORNAMENT
    LineBreakClass::ULB_OP, // 0x276E  # HEAVY LEFT-POINTING ANGLE QUOTATION MARK ORNAMENT
    LineBreakClass::ULB_CL, // 0x276F  # HEAVY RIGHT-POINTING ANGLE QUOTATION MARK ORNAMENT
    LineBreakClass::ULB_OP, // 0x2770  # HEAVY LEFT-POINTING ANGLE BRACKET ORNAMENT
    LineBreakClass::ULB_CL, // 0x2771  # HEAVY RIGHT-POINTING ANGLE BRACKET ORNAMENT
    LineBreakClass::ULB_OP, // 0x2772  # LIGHT LEFT TORTOISE SHELL BRACKET ORNAMENT
    LineBreakClass::ULB_CL, // 0x2773  # LIGHT RIGHT TORTOISE SHELL BRACKET ORNAMENT
    LineBreakClass::ULB_OP, // 0x2774  # MEDIUM LEFT CURLY BRACKET ORNAMENT
    LineBreakClass::ULB_CL, // 0x2775  # MEDIUM RIGHT CURLY BRACKET ORNAMENT
    LineBreakClass::ULB_AI, // 0x2776  # DINGBAT NEGATIVE CIRCLED DIGIT ONE
    LineBreakClass::ULB_AI, // 0x2777  # DINGBAT NEGATIVE CIRCLED DIGIT TWO
    LineBreakClass::ULB_AI, // 0x2778  # DINGBAT NEGATIVE CIRCLED DIGIT THREE
    LineBreakClass::ULB_AI, // 0x2779  # DINGBAT NEGATIVE CIRCLED DIGIT FOUR
    LineBreakClass::ULB_AI, // 0x277A  # DINGBAT NEGATIVE CIRCLED DIGIT FIVE
    LineBreakClass::ULB_AI, // 0x277B  # DINGBAT NEGATIVE CIRCLED DIGIT SIX
    LineBreakClass::ULB_AI, // 0x277C  # DINGBAT NEGATIVE CIRCLED DIGIT SEVEN
    LineBreakClass::ULB_AI, // 0x277D  # DINGBAT NEGATIVE CIRCLED DIGIT EIGHT
    LineBreakClass::ULB_AI, // 0x277E  # DINGBAT NEGATIVE CIRCLED DIGIT NINE
    LineBreakClass::ULB_AI, // 0x277F  # DINGBAT NEGATIVE CIRCLED NUMBER TEN
    LineBreakClass::ULB_AI, // 0x2780  # DINGBAT CIRCLED SANS-SERIF DIGIT ONE
    LineBreakClass::ULB_AI, // 0x2781  # DINGBAT CIRCLED SANS-SERIF DIGIT TWO
    LineBreakClass::ULB_AI, // 0x2782  # DINGBAT CIRCLED SANS-SERIF DIGIT THREE
    LineBreakClass::ULB_AI, // 0x2783  # DINGBAT CIRCLED SANS-SERIF DIGIT FOUR
    LineBreakClass::ULB_AI, // 0x2784  # DINGBAT CIRCLED SANS-SERIF DIGIT FIVE
    LineBreakClass::ULB_AI, // 0x2785  # DINGBAT CIRCLED SANS-SERIF DIGIT SIX
    LineBreakClass::ULB_AI, // 0x2786  # DINGBAT CIRCLED SANS-SERIF DIGIT SEVEN
    LineBreakClass::ULB_AI, // 0x2787  # DINGBAT CIRCLED SANS-SERIF DIGIT EIGHT
    LineBreakClass::ULB_AI, // 0x2788  # DINGBAT CIRCLED SANS-SERIF DIGIT NINE
    LineBreakClass::ULB_AI, // 0x2789  # DINGBAT CIRCLED SANS-SERIF NUMBER TEN
    LineBreakClass::ULB_AI, // 0x278A  # DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT ONE
    LineBreakClass::ULB_AI, // 0x278B  # DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT TWO
    LineBreakClass::ULB_AI, // 0x278C  # DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT THREE
    LineBreakClass::ULB_AI, // 0x278D  # DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT FOUR
    LineBreakClass::ULB_AI, // 0x278E  # DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT FIVE
    LineBreakClass::ULB_AI, // 0x278F  # DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT SIX
    LineBreakClass::ULB_AI, // 0x2790  # DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT SEVEN
    LineBreakClass::ULB_AI, // 0x2791  # DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT EIGHT
    LineBreakClass::ULB_AI, // 0x2792  # DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT NINE
    LineBreakClass::ULB_AI, // 0x2793  # DINGBAT NEGATIVE CIRCLED SANS-SERIF NUMBER TEN
    LineBreakClass::ULB_AL, // 0x2794  # HEAVY WIDE-HEADED RIGHTWARDS ARROW
    LineBreakClass::ULB_ID, // 0x2795 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2796 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2797 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2798  # HEAVY SOUTH EAST ARROW
    LineBreakClass::ULB_AL, // 0x2799  # HEAVY RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x279A  # HEAVY NORTH EAST ARROW
    LineBreakClass::ULB_AL, // 0x279B  # DRAFTING POINT RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x279C  # HEAVY ROUND-TIPPED RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x279D  # TRIANGLE-HEADED RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x279E  # HEAVY TRIANGLE-HEADED RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x279F  # DASHED TRIANGLE-HEADED RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27A0  # HEAVY DASHED TRIANGLE-HEADED RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27A1  # BLACK RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27A2  # THREE-D TOP-LIGHTED RIGHTWARDS ARROWHEAD
    LineBreakClass::ULB_AL, // 0x27A3  # THREE-D BOTTOM-LIGHTED RIGHTWARDS ARROWHEAD
    LineBreakClass::ULB_AL, // 0x27A4  # BLACK RIGHTWARDS ARROWHEAD
    LineBreakClass::ULB_AL, // 0x27A5  # HEAVY BLACK CURVED DOWNWARDS AND RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27A6  # HEAVY BLACK CURVED UPWARDS AND RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27A7  # SQUAT BLACK RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27A8  # HEAVY CONCAVE-POINTED BLACK RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27A9  # RIGHT-SHADED WHITE RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27AA  # LEFT-SHADED WHITE RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27AB  # BACK-TILTED SHADOWED WHITE RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27AC  # FRONT-TILTED SHADOWED WHITE RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27AD  # HEAVY LOWER RIGHT-SHADOWED WHITE RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27AE  # HEAVY UPPER RIGHT-SHADOWED WHITE RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27AF  # NOTCHED LOWER RIGHT-SHADOWED WHITE RIGHTWARDS ARROW
    LineBreakClass::ULB_ID, // 0x27B0 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x27B1  # NOTCHED UPPER RIGHT-SHADOWED WHITE RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27B2  # CIRCLED HEAVY WHITE RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27B3  # WHITE-FEATHERED RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27B4  # BLACK-FEATHERED SOUTH EAST ARROW
    LineBreakClass::ULB_AL, // 0x27B5  # BLACK-FEATHERED RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27B6  # BLACK-FEATHERED NORTH EAST ARROW
    LineBreakClass::ULB_AL, // 0x27B7  # HEAVY BLACK-FEATHERED SOUTH EAST ARROW
    LineBreakClass::ULB_AL, // 0x27B8  # HEAVY BLACK-FEATHERED RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27B9  # HEAVY BLACK-FEATHERED NORTH EAST ARROW
    LineBreakClass::ULB_AL, // 0x27BA  # TEARDROP-BARBED RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27BB  # HEAVY TEARDROP-SHANKED RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27BC  # WEDGE-TAILED RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27BD  # HEAVY WEDGE-TAILED RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27BE  # OPEN-OUTLINED RIGHTWARDS ARROW
    LineBreakClass::ULB_ID, // 0x27BF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x27C0  # THREE DIMENSIONAL ANGLE
    LineBreakClass::ULB_AL, // 0x27C1  # WHITE TRIANGLE CONTAINING SMALL WHITE TRIANGLE
    LineBreakClass::ULB_AL, // 0x27C2  # PERPENDICULAR
    LineBreakClass::ULB_AL, // 0x27C3  # OPEN SUBSET
    LineBreakClass::ULB_AL, // 0x27C4  # OPEN SUPERSET
    LineBreakClass::ULB_OP, // 0x27C5  # LEFT S-SHAPED BAG DELIMITER
    LineBreakClass::ULB_CL, // 0x27C6  # RIGHT S-SHAPED BAG DELIMITER
    LineBreakClass::ULB_AL, // 0x27C7  # OR WITH DOT INSIDE
    LineBreakClass::ULB_AL, // 0x27C8  # REVERSE SOLIDUS PRECEDING SUBSET
    LineBreakClass::ULB_AL, // 0x27C9  # SUPERSET PRECEDING SOLIDUS
    LineBreakClass::ULB_AL, // 0x27CA  # VERTICAL BAR WITH HORIZONTAL STROKE
    LineBreakClass::ULB_ID, // 0x27CB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x27CC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x27CD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x27CE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x27CF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x27D0  # WHITE DIAMOND WITH CENTRED DOT
    LineBreakClass::ULB_AL, // 0x27D1  # AND WITH DOT
    LineBreakClass::ULB_AL, // 0x27D2  # ELEMENT OF OPENING UPWARDS
    LineBreakClass::ULB_AL, // 0x27D3  # LOWER RIGHT CORNER WITH DOT
    LineBreakClass::ULB_AL, // 0x27D4  # UPPER LEFT CORNER WITH DOT
    LineBreakClass::ULB_AL, // 0x27D5  # LEFT OUTER JOIN
    LineBreakClass::ULB_AL, // 0x27D6  # RIGHT OUTER JOIN
    LineBreakClass::ULB_AL, // 0x27D7  # FULL OUTER JOIN
    LineBreakClass::ULB_AL, // 0x27D8  # LARGE UP TACK
    LineBreakClass::ULB_AL, // 0x27D9  # LARGE DOWN TACK
    LineBreakClass::ULB_AL, // 0x27DA  # LEFT AND RIGHT DOUBLE TURNSTILE
    LineBreakClass::ULB_AL, // 0x27DB  # LEFT AND RIGHT TACK
    LineBreakClass::ULB_AL, // 0x27DC  # LEFT MULTIMAP
    LineBreakClass::ULB_AL, // 0x27DD  # LONG RIGHT TACK
    LineBreakClass::ULB_AL, // 0x27DE  # LONG LEFT TACK
    LineBreakClass::ULB_AL, // 0x27DF  # UP TACK WITH CIRCLE ABOVE
    LineBreakClass::ULB_AL, // 0x27E0  # LOZENGE DIVIDED BY HORIZONTAL RULE
    LineBreakClass::ULB_AL, // 0x27E1  # WHITE CONCAVE-SIDED DIAMOND
    LineBreakClass::ULB_AL, // 0x27E2  # WHITE CONCAVE-SIDED DIAMOND WITH LEFTWARDS TICK
    LineBreakClass::ULB_AL, // 0x27E3  # WHITE CONCAVE-SIDED DIAMOND WITH RIGHTWARDS TICK
    LineBreakClass::ULB_AL, // 0x27E4  # WHITE SQUARE WITH LEFTWARDS TICK
    LineBreakClass::ULB_AL, // 0x27E5  # WHITE SQUARE WITH RIGHTWARDS TICK
    LineBreakClass::ULB_OP, // 0x27E6  # MATHEMATICAL LEFT WHITE SQUARE BRACKET
    LineBreakClass::ULB_CL, // 0x27E7  # MATHEMATICAL RIGHT WHITE SQUARE BRACKET
    LineBreakClass::ULB_OP, // 0x27E8  # MATHEMATICAL LEFT ANGLE BRACKET
    LineBreakClass::ULB_CL, // 0x27E9  # MATHEMATICAL RIGHT ANGLE BRACKET
    LineBreakClass::ULB_OP, // 0x27EA  # MATHEMATICAL LEFT DOUBLE ANGLE BRACKET
    LineBreakClass::ULB_CL, // 0x27EB  # MATHEMATICAL RIGHT DOUBLE ANGLE BRACKET
    LineBreakClass::ULB_ID, // 0x27EC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x27ED # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x27EE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x27EF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x27F0  # UPWARDS QUADRUPLE ARROW
    LineBreakClass::ULB_AL, // 0x27F1  # DOWNWARDS QUADRUPLE ARROW
    LineBreakClass::ULB_AL, // 0x27F2  # ANTICLOCKWISE GAPPED CIRCLE ARROW
    LineBreakClass::ULB_AL, // 0x27F3  # CLOCKWISE GAPPED CIRCLE ARROW
    LineBreakClass::ULB_AL, // 0x27F4  # RIGHT ARROW WITH CIRCLED PLUS
    LineBreakClass::ULB_AL, // 0x27F5  # LONG LEFTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27F6  # LONG RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x27F7  # LONG LEFT RIGHT ARROW
    LineBreakClass::ULB_AL, // 0x27F8  # LONG LEFTWARDS DOUBLE ARROW
    LineBreakClass::ULB_AL, // 0x27F9  # LONG RIGHTWARDS DOUBLE ARROW
    LineBreakClass::ULB_AL, // 0x27FA  # LONG LEFT RIGHT DOUBLE ARROW
    LineBreakClass::ULB_AL, // 0x27FB  # LONG LEFTWARDS ARROW FROM BAR
    LineBreakClass::ULB_AL, // 0x27FC  # LONG RIGHTWARDS ARROW FROM BAR
    LineBreakClass::ULB_AL, // 0x27FD  # LONG LEFTWARDS DOUBLE ARROW FROM BAR
    LineBreakClass::ULB_AL, // 0x27FE  # LONG RIGHTWARDS DOUBLE ARROW FROM BAR
    LineBreakClass::ULB_AL, // 0x27FF  # LONG RIGHTWARDS SQUIGGLE ARROW
    LineBreakClass::ULB_AL, // 0x2800  # BRAILLE PATTERN BLANK
    LineBreakClass::ULB_AL, // 0x2801  # BRAILLE PATTERN DOTS-1
    LineBreakClass::ULB_AL, // 0x2802  # BRAILLE PATTERN DOTS-2
    LineBreakClass::ULB_AL, // 0x2803  # BRAILLE PATTERN DOTS-12
    LineBreakClass::ULB_AL, // 0x2804  # BRAILLE PATTERN DOTS-3
    LineBreakClass::ULB_AL, // 0x2805  # BRAILLE PATTERN DOTS-13
    LineBreakClass::ULB_AL, // 0x2806  # BRAILLE PATTERN DOTS-23
    LineBreakClass::ULB_AL, // 0x2807  # BRAILLE PATTERN DOTS-123
    LineBreakClass::ULB_AL, // 0x2808  # BRAILLE PATTERN DOTS-4
    LineBreakClass::ULB_AL, // 0x2809  # BRAILLE PATTERN DOTS-14
    LineBreakClass::ULB_AL, // 0x280A  # BRAILLE PATTERN DOTS-24
    LineBreakClass::ULB_AL, // 0x280B  # BRAILLE PATTERN DOTS-124
    LineBreakClass::ULB_AL, // 0x280C  # BRAILLE PATTERN DOTS-34
    LineBreakClass::ULB_AL, // 0x280D  # BRAILLE PATTERN DOTS-134
    LineBreakClass::ULB_AL, // 0x280E  # BRAILLE PATTERN DOTS-234
    LineBreakClass::ULB_AL, // 0x280F  # BRAILLE PATTERN DOTS-1234
    LineBreakClass::ULB_AL, // 0x2810  # BRAILLE PATTERN DOTS-5
    LineBreakClass::ULB_AL, // 0x2811  # BRAILLE PATTERN DOTS-15
    LineBreakClass::ULB_AL, // 0x2812  # BRAILLE PATTERN DOTS-25
    LineBreakClass::ULB_AL, // 0x2813  # BRAILLE PATTERN DOTS-125
    LineBreakClass::ULB_AL, // 0x2814  # BRAILLE PATTERN DOTS-35
    LineBreakClass::ULB_AL, // 0x2815  # BRAILLE PATTERN DOTS-135
    LineBreakClass::ULB_AL, // 0x2816  # BRAILLE PATTERN DOTS-235
    LineBreakClass::ULB_AL, // 0x2817  # BRAILLE PATTERN DOTS-1235
    LineBreakClass::ULB_AL, // 0x2818  # BRAILLE PATTERN DOTS-45
    LineBreakClass::ULB_AL, // 0x2819  # BRAILLE PATTERN DOTS-145
    LineBreakClass::ULB_AL, // 0x281A  # BRAILLE PATTERN DOTS-245
    LineBreakClass::ULB_AL, // 0x281B  # BRAILLE PATTERN DOTS-1245
    LineBreakClass::ULB_AL, // 0x281C  # BRAILLE PATTERN DOTS-345
    LineBreakClass::ULB_AL, // 0x281D  # BRAILLE PATTERN DOTS-1345
    LineBreakClass::ULB_AL, // 0x281E  # BRAILLE PATTERN DOTS-2345
    LineBreakClass::ULB_AL, // 0x281F  # BRAILLE PATTERN DOTS-12345
    LineBreakClass::ULB_AL, // 0x2820  # BRAILLE PATTERN DOTS-6
    LineBreakClass::ULB_AL, // 0x2821  # BRAILLE PATTERN DOTS-16
    LineBreakClass::ULB_AL, // 0x2822  # BRAILLE PATTERN DOTS-26
    LineBreakClass::ULB_AL, // 0x2823  # BRAILLE PATTERN DOTS-126
    LineBreakClass::ULB_AL, // 0x2824  # BRAILLE PATTERN DOTS-36
    LineBreakClass::ULB_AL, // 0x2825  # BRAILLE PATTERN DOTS-136
    LineBreakClass::ULB_AL, // 0x2826  # BRAILLE PATTERN DOTS-236
    LineBreakClass::ULB_AL, // 0x2827  # BRAILLE PATTERN DOTS-1236
    LineBreakClass::ULB_AL, // 0x2828  # BRAILLE PATTERN DOTS-46
    LineBreakClass::ULB_AL, // 0x2829  # BRAILLE PATTERN DOTS-146
    LineBreakClass::ULB_AL, // 0x282A  # BRAILLE PATTERN DOTS-246
    LineBreakClass::ULB_AL, // 0x282B  # BRAILLE PATTERN DOTS-1246
    LineBreakClass::ULB_AL, // 0x282C  # BRAILLE PATTERN DOTS-346
    LineBreakClass::ULB_AL, // 0x282D  # BRAILLE PATTERN DOTS-1346
    LineBreakClass::ULB_AL, // 0x282E  # BRAILLE PATTERN DOTS-2346
    LineBreakClass::ULB_AL, // 0x282F  # BRAILLE PATTERN DOTS-12346
    LineBreakClass::ULB_AL, // 0x2830  # BRAILLE PATTERN DOTS-56
    LineBreakClass::ULB_AL, // 0x2831  # BRAILLE PATTERN DOTS-156
    LineBreakClass::ULB_AL, // 0x2832  # BRAILLE PATTERN DOTS-256
    LineBreakClass::ULB_AL, // 0x2833  # BRAILLE PATTERN DOTS-1256
    LineBreakClass::ULB_AL, // 0x2834  # BRAILLE PATTERN DOTS-356
    LineBreakClass::ULB_AL, // 0x2835  # BRAILLE PATTERN DOTS-1356
    LineBreakClass::ULB_AL, // 0x2836  # BRAILLE PATTERN DOTS-2356
    LineBreakClass::ULB_AL, // 0x2837  # BRAILLE PATTERN DOTS-12356
    LineBreakClass::ULB_AL, // 0x2838  # BRAILLE PATTERN DOTS-456
    LineBreakClass::ULB_AL, // 0x2839  # BRAILLE PATTERN DOTS-1456
    LineBreakClass::ULB_AL, // 0x283A  # BRAILLE PATTERN DOTS-2456
    LineBreakClass::ULB_AL, // 0x283B  # BRAILLE PATTERN DOTS-12456
    LineBreakClass::ULB_AL, // 0x283C  # BRAILLE PATTERN DOTS-3456
    LineBreakClass::ULB_AL, // 0x283D  # BRAILLE PATTERN DOTS-13456
    LineBreakClass::ULB_AL, // 0x283E  # BRAILLE PATTERN DOTS-23456
    LineBreakClass::ULB_AL, // 0x283F  # BRAILLE PATTERN DOTS-123456
    LineBreakClass::ULB_AL, // 0x2840  # BRAILLE PATTERN DOTS-7
    LineBreakClass::ULB_AL, // 0x2841  # BRAILLE PATTERN DOTS-17
    LineBreakClass::ULB_AL, // 0x2842  # BRAILLE PATTERN DOTS-27
    LineBreakClass::ULB_AL, // 0x2843  # BRAILLE PATTERN DOTS-127
    LineBreakClass::ULB_AL, // 0x2844  # BRAILLE PATTERN DOTS-37
    LineBreakClass::ULB_AL, // 0x2845  # BRAILLE PATTERN DOTS-137
    LineBreakClass::ULB_AL, // 0x2846  # BRAILLE PATTERN DOTS-237
    LineBreakClass::ULB_AL, // 0x2847  # BRAILLE PATTERN DOTS-1237
    LineBreakClass::ULB_AL, // 0x2848  # BRAILLE PATTERN DOTS-47
    LineBreakClass::ULB_AL, // 0x2849  # BRAILLE PATTERN DOTS-147
    LineBreakClass::ULB_AL, // 0x284A  # BRAILLE PATTERN DOTS-247
    LineBreakClass::ULB_AL, // 0x284B  # BRAILLE PATTERN DOTS-1247
    LineBreakClass::ULB_AL, // 0x284C  # BRAILLE PATTERN DOTS-347
    LineBreakClass::ULB_AL, // 0x284D  # BRAILLE PATTERN DOTS-1347
    LineBreakClass::ULB_AL, // 0x284E  # BRAILLE PATTERN DOTS-2347
    LineBreakClass::ULB_AL, // 0x284F  # BRAILLE PATTERN DOTS-12347
    LineBreakClass::ULB_AL, // 0x2850  # BRAILLE PATTERN DOTS-57
    LineBreakClass::ULB_AL, // 0x2851  # BRAILLE PATTERN DOTS-157
    LineBreakClass::ULB_AL, // 0x2852  # BRAILLE PATTERN DOTS-257
    LineBreakClass::ULB_AL, // 0x2853  # BRAILLE PATTERN DOTS-1257
    LineBreakClass::ULB_AL, // 0x2854  # BRAILLE PATTERN DOTS-357
    LineBreakClass::ULB_AL, // 0x2855  # BRAILLE PATTERN DOTS-1357
    LineBreakClass::ULB_AL, // 0x2856  # BRAILLE PATTERN DOTS-2357
    LineBreakClass::ULB_AL, // 0x2857  # BRAILLE PATTERN DOTS-12357
    LineBreakClass::ULB_AL, // 0x2858  # BRAILLE PATTERN DOTS-457
    LineBreakClass::ULB_AL, // 0x2859  # BRAILLE PATTERN DOTS-1457
    LineBreakClass::ULB_AL, // 0x285A  # BRAILLE PATTERN DOTS-2457
    LineBreakClass::ULB_AL, // 0x285B  # BRAILLE PATTERN DOTS-12457
    LineBreakClass::ULB_AL, // 0x285C  # BRAILLE PATTERN DOTS-3457
    LineBreakClass::ULB_AL, // 0x285D  # BRAILLE PATTERN DOTS-13457
    LineBreakClass::ULB_AL, // 0x285E  # BRAILLE PATTERN DOTS-23457
    LineBreakClass::ULB_AL, // 0x285F  # BRAILLE PATTERN DOTS-123457
    LineBreakClass::ULB_AL, // 0x2860  # BRAILLE PATTERN DOTS-67
    LineBreakClass::ULB_AL, // 0x2861  # BRAILLE PATTERN DOTS-167
    LineBreakClass::ULB_AL, // 0x2862  # BRAILLE PATTERN DOTS-267
    LineBreakClass::ULB_AL, // 0x2863  # BRAILLE PATTERN DOTS-1267
    LineBreakClass::ULB_AL, // 0x2864  # BRAILLE PATTERN DOTS-367
    LineBreakClass::ULB_AL, // 0x2865  # BRAILLE PATTERN DOTS-1367
    LineBreakClass::ULB_AL, // 0x2866  # BRAILLE PATTERN DOTS-2367
    LineBreakClass::ULB_AL, // 0x2867  # BRAILLE PATTERN DOTS-12367
    LineBreakClass::ULB_AL, // 0x2868  # BRAILLE PATTERN DOTS-467
    LineBreakClass::ULB_AL, // 0x2869  # BRAILLE PATTERN DOTS-1467
    LineBreakClass::ULB_AL, // 0x286A  # BRAILLE PATTERN DOTS-2467
    LineBreakClass::ULB_AL, // 0x286B  # BRAILLE PATTERN DOTS-12467
    LineBreakClass::ULB_AL, // 0x286C  # BRAILLE PATTERN DOTS-3467
    LineBreakClass::ULB_AL, // 0x286D  # BRAILLE PATTERN DOTS-13467
    LineBreakClass::ULB_AL, // 0x286E  # BRAILLE PATTERN DOTS-23467
    LineBreakClass::ULB_AL, // 0x286F  # BRAILLE PATTERN DOTS-123467
    LineBreakClass::ULB_AL, // 0x2870  # BRAILLE PATTERN DOTS-567
    LineBreakClass::ULB_AL, // 0x2871  # BRAILLE PATTERN DOTS-1567
    LineBreakClass::ULB_AL, // 0x2872  # BRAILLE PATTERN DOTS-2567
    LineBreakClass::ULB_AL, // 0x2873  # BRAILLE PATTERN DOTS-12567
    LineBreakClass::ULB_AL, // 0x2874  # BRAILLE PATTERN DOTS-3567
    LineBreakClass::ULB_AL, // 0x2875  # BRAILLE PATTERN DOTS-13567
    LineBreakClass::ULB_AL, // 0x2876  # BRAILLE PATTERN DOTS-23567
    LineBreakClass::ULB_AL, // 0x2877  # BRAILLE PATTERN DOTS-123567
    LineBreakClass::ULB_AL, // 0x2878  # BRAILLE PATTERN DOTS-4567
    LineBreakClass::ULB_AL, // 0x2879  # BRAILLE PATTERN DOTS-14567
    LineBreakClass::ULB_AL, // 0x287A  # BRAILLE PATTERN DOTS-24567
    LineBreakClass::ULB_AL, // 0x287B  # BRAILLE PATTERN DOTS-124567
    LineBreakClass::ULB_AL, // 0x287C  # BRAILLE PATTERN DOTS-34567
    LineBreakClass::ULB_AL, // 0x287D  # BRAILLE PATTERN DOTS-134567
    LineBreakClass::ULB_AL, // 0x287E  # BRAILLE PATTERN DOTS-234567
    LineBreakClass::ULB_AL, // 0x287F  # BRAILLE PATTERN DOTS-1234567
    LineBreakClass::ULB_AL, // 0x2880  # BRAILLE PATTERN DOTS-8
    LineBreakClass::ULB_AL, // 0x2881  # BRAILLE PATTERN DOTS-18
    LineBreakClass::ULB_AL, // 0x2882  # BRAILLE PATTERN DOTS-28
    LineBreakClass::ULB_AL, // 0x2883  # BRAILLE PATTERN DOTS-128
    LineBreakClass::ULB_AL, // 0x2884  # BRAILLE PATTERN DOTS-38
    LineBreakClass::ULB_AL, // 0x2885  # BRAILLE PATTERN DOTS-138
    LineBreakClass::ULB_AL, // 0x2886  # BRAILLE PATTERN DOTS-238
    LineBreakClass::ULB_AL, // 0x2887  # BRAILLE PATTERN DOTS-1238
    LineBreakClass::ULB_AL, // 0x2888  # BRAILLE PATTERN DOTS-48
    LineBreakClass::ULB_AL, // 0x2889  # BRAILLE PATTERN DOTS-148
    LineBreakClass::ULB_AL, // 0x288A  # BRAILLE PATTERN DOTS-248
    LineBreakClass::ULB_AL, // 0x288B  # BRAILLE PATTERN DOTS-1248
    LineBreakClass::ULB_AL, // 0x288C  # BRAILLE PATTERN DOTS-348
    LineBreakClass::ULB_AL, // 0x288D  # BRAILLE PATTERN DOTS-1348
    LineBreakClass::ULB_AL, // 0x288E  # BRAILLE PATTERN DOTS-2348
    LineBreakClass::ULB_AL, // 0x288F  # BRAILLE PATTERN DOTS-12348
    LineBreakClass::ULB_AL, // 0x2890  # BRAILLE PATTERN DOTS-58
    LineBreakClass::ULB_AL, // 0x2891  # BRAILLE PATTERN DOTS-158
    LineBreakClass::ULB_AL, // 0x2892  # BRAILLE PATTERN DOTS-258
    LineBreakClass::ULB_AL, // 0x2893  # BRAILLE PATTERN DOTS-1258
    LineBreakClass::ULB_AL, // 0x2894  # BRAILLE PATTERN DOTS-358
    LineBreakClass::ULB_AL, // 0x2895  # BRAILLE PATTERN DOTS-1358
    LineBreakClass::ULB_AL, // 0x2896  # BRAILLE PATTERN DOTS-2358
    LineBreakClass::ULB_AL, // 0x2897  # BRAILLE PATTERN DOTS-12358
    LineBreakClass::ULB_AL, // 0x2898  # BRAILLE PATTERN DOTS-458
    LineBreakClass::ULB_AL, // 0x2899  # BRAILLE PATTERN DOTS-1458
    LineBreakClass::ULB_AL, // 0x289A  # BRAILLE PATTERN DOTS-2458
    LineBreakClass::ULB_AL, // 0x289B  # BRAILLE PATTERN DOTS-12458
    LineBreakClass::ULB_AL, // 0x289C  # BRAILLE PATTERN DOTS-3458
    LineBreakClass::ULB_AL, // 0x289D  # BRAILLE PATTERN DOTS-13458
    LineBreakClass::ULB_AL, // 0x289E  # BRAILLE PATTERN DOTS-23458
    LineBreakClass::ULB_AL, // 0x289F  # BRAILLE PATTERN DOTS-123458
    LineBreakClass::ULB_AL, // 0x28A0  # BRAILLE PATTERN DOTS-68
    LineBreakClass::ULB_AL, // 0x28A1  # BRAILLE PATTERN DOTS-168
    LineBreakClass::ULB_AL, // 0x28A2  # BRAILLE PATTERN DOTS-268
    LineBreakClass::ULB_AL, // 0x28A3  # BRAILLE PATTERN DOTS-1268
    LineBreakClass::ULB_AL, // 0x28A4  # BRAILLE PATTERN DOTS-368
    LineBreakClass::ULB_AL, // 0x28A5  # BRAILLE PATTERN DOTS-1368
    LineBreakClass::ULB_AL, // 0x28A6  # BRAILLE PATTERN DOTS-2368
    LineBreakClass::ULB_AL, // 0x28A7  # BRAILLE PATTERN DOTS-12368
    LineBreakClass::ULB_AL, // 0x28A8  # BRAILLE PATTERN DOTS-468
    LineBreakClass::ULB_AL, // 0x28A9  # BRAILLE PATTERN DOTS-1468
    LineBreakClass::ULB_AL, // 0x28AA  # BRAILLE PATTERN DOTS-2468
    LineBreakClass::ULB_AL, // 0x28AB  # BRAILLE PATTERN DOTS-12468
    LineBreakClass::ULB_AL, // 0x28AC  # BRAILLE PATTERN DOTS-3468
    LineBreakClass::ULB_AL, // 0x28AD  # BRAILLE PATTERN DOTS-13468
    LineBreakClass::ULB_AL, // 0x28AE  # BRAILLE PATTERN DOTS-23468
    LineBreakClass::ULB_AL, // 0x28AF  # BRAILLE PATTERN DOTS-123468
    LineBreakClass::ULB_AL, // 0x28B0  # BRAILLE PATTERN DOTS-568
    LineBreakClass::ULB_AL, // 0x28B1  # BRAILLE PATTERN DOTS-1568
    LineBreakClass::ULB_AL, // 0x28B2  # BRAILLE PATTERN DOTS-2568
    LineBreakClass::ULB_AL, // 0x28B3  # BRAILLE PATTERN DOTS-12568
    LineBreakClass::ULB_AL, // 0x28B4  # BRAILLE PATTERN DOTS-3568
    LineBreakClass::ULB_AL, // 0x28B5  # BRAILLE PATTERN DOTS-13568
    LineBreakClass::ULB_AL, // 0x28B6  # BRAILLE PATTERN DOTS-23568
    LineBreakClass::ULB_AL, // 0x28B7  # BRAILLE PATTERN DOTS-123568
    LineBreakClass::ULB_AL, // 0x28B8  # BRAILLE PATTERN DOTS-4568
    LineBreakClass::ULB_AL, // 0x28B9  # BRAILLE PATTERN DOTS-14568
    LineBreakClass::ULB_AL, // 0x28BA  # BRAILLE PATTERN DOTS-24568
    LineBreakClass::ULB_AL, // 0x28BB  # BRAILLE PATTERN DOTS-124568
    LineBreakClass::ULB_AL, // 0x28BC  # BRAILLE PATTERN DOTS-34568
    LineBreakClass::ULB_AL, // 0x28BD  # BRAILLE PATTERN DOTS-134568
    LineBreakClass::ULB_AL, // 0x28BE  # BRAILLE PATTERN DOTS-234568
    LineBreakClass::ULB_AL, // 0x28BF  # BRAILLE PATTERN DOTS-1234568
    LineBreakClass::ULB_AL, // 0x28C0  # BRAILLE PATTERN DOTS-78
    LineBreakClass::ULB_AL, // 0x28C1  # BRAILLE PATTERN DOTS-178
    LineBreakClass::ULB_AL, // 0x28C2  # BRAILLE PATTERN DOTS-278
    LineBreakClass::ULB_AL, // 0x28C3  # BRAILLE PATTERN DOTS-1278
    LineBreakClass::ULB_AL, // 0x28C4  # BRAILLE PATTERN DOTS-378
    LineBreakClass::ULB_AL, // 0x28C5  # BRAILLE PATTERN DOTS-1378
    LineBreakClass::ULB_AL, // 0x28C6  # BRAILLE PATTERN DOTS-2378
    LineBreakClass::ULB_AL, // 0x28C7  # BRAILLE PATTERN DOTS-12378
    LineBreakClass::ULB_AL, // 0x28C8  # BRAILLE PATTERN DOTS-478
    LineBreakClass::ULB_AL, // 0x28C9  # BRAILLE PATTERN DOTS-1478
    LineBreakClass::ULB_AL, // 0x28CA  # BRAILLE PATTERN DOTS-2478
    LineBreakClass::ULB_AL, // 0x28CB  # BRAILLE PATTERN DOTS-12478
    LineBreakClass::ULB_AL, // 0x28CC  # BRAILLE PATTERN DOTS-3478
    LineBreakClass::ULB_AL, // 0x28CD  # BRAILLE PATTERN DOTS-13478
    LineBreakClass::ULB_AL, // 0x28CE  # BRAILLE PATTERN DOTS-23478
    LineBreakClass::ULB_AL, // 0x28CF  # BRAILLE PATTERN DOTS-123478
    LineBreakClass::ULB_AL, // 0x28D0  # BRAILLE PATTERN DOTS-578
    LineBreakClass::ULB_AL, // 0x28D1  # BRAILLE PATTERN DOTS-1578
    LineBreakClass::ULB_AL, // 0x28D2  # BRAILLE PATTERN DOTS-2578
    LineBreakClass::ULB_AL, // 0x28D3  # BRAILLE PATTERN DOTS-12578
    LineBreakClass::ULB_AL, // 0x28D4  # BRAILLE PATTERN DOTS-3578
    LineBreakClass::ULB_AL, // 0x28D5  # BRAILLE PATTERN DOTS-13578
    LineBreakClass::ULB_AL, // 0x28D6  # BRAILLE PATTERN DOTS-23578
    LineBreakClass::ULB_AL, // 0x28D7  # BRAILLE PATTERN DOTS-123578
    LineBreakClass::ULB_AL, // 0x28D8  # BRAILLE PATTERN DOTS-4578
    LineBreakClass::ULB_AL, // 0x28D9  # BRAILLE PATTERN DOTS-14578
    LineBreakClass::ULB_AL, // 0x28DA  # BRAILLE PATTERN DOTS-24578
    LineBreakClass::ULB_AL, // 0x28DB  # BRAILLE PATTERN DOTS-124578
    LineBreakClass::ULB_AL, // 0x28DC  # BRAILLE PATTERN DOTS-34578
    LineBreakClass::ULB_AL, // 0x28DD  # BRAILLE PATTERN DOTS-134578
    LineBreakClass::ULB_AL, // 0x28DE  # BRAILLE PATTERN DOTS-234578
    LineBreakClass::ULB_AL, // 0x28DF  # BRAILLE PATTERN DOTS-1234578
    LineBreakClass::ULB_AL, // 0x28E0  # BRAILLE PATTERN DOTS-678
    LineBreakClass::ULB_AL, // 0x28E1  # BRAILLE PATTERN DOTS-1678
    LineBreakClass::ULB_AL, // 0x28E2  # BRAILLE PATTERN DOTS-2678
    LineBreakClass::ULB_AL, // 0x28E3  # BRAILLE PATTERN DOTS-12678
    LineBreakClass::ULB_AL, // 0x28E4  # BRAILLE PATTERN DOTS-3678
    LineBreakClass::ULB_AL, // 0x28E5  # BRAILLE PATTERN DOTS-13678
    LineBreakClass::ULB_AL, // 0x28E6  # BRAILLE PATTERN DOTS-23678
    LineBreakClass::ULB_AL, // 0x28E7  # BRAILLE PATTERN DOTS-123678
    LineBreakClass::ULB_AL, // 0x28E8  # BRAILLE PATTERN DOTS-4678
    LineBreakClass::ULB_AL, // 0x28E9  # BRAILLE PATTERN DOTS-14678
    LineBreakClass::ULB_AL, // 0x28EA  # BRAILLE PATTERN DOTS-24678
    LineBreakClass::ULB_AL, // 0x28EB  # BRAILLE PATTERN DOTS-124678
    LineBreakClass::ULB_AL, // 0x28EC  # BRAILLE PATTERN DOTS-34678
    LineBreakClass::ULB_AL, // 0x28ED  # BRAILLE PATTERN DOTS-134678
    LineBreakClass::ULB_AL, // 0x28EE  # BRAILLE PATTERN DOTS-234678
    LineBreakClass::ULB_AL, // 0x28EF  # BRAILLE PATTERN DOTS-1234678
    LineBreakClass::ULB_AL, // 0x28F0  # BRAILLE PATTERN DOTS-5678
    LineBreakClass::ULB_AL, // 0x28F1  # BRAILLE PATTERN DOTS-15678
    LineBreakClass::ULB_AL, // 0x28F2  # BRAILLE PATTERN DOTS-25678
    LineBreakClass::ULB_AL, // 0x28F3  # BRAILLE PATTERN DOTS-125678
    LineBreakClass::ULB_AL, // 0x28F4  # BRAILLE PATTERN DOTS-35678
    LineBreakClass::ULB_AL, // 0x28F5  # BRAILLE PATTERN DOTS-135678
    LineBreakClass::ULB_AL, // 0x28F6  # BRAILLE PATTERN DOTS-235678
    LineBreakClass::ULB_AL, // 0x28F7  # BRAILLE PATTERN DOTS-1235678
    LineBreakClass::ULB_AL, // 0x28F8  # BRAILLE PATTERN DOTS-45678
    LineBreakClass::ULB_AL, // 0x28F9  # BRAILLE PATTERN DOTS-145678
    LineBreakClass::ULB_AL, // 0x28FA  # BRAILLE PATTERN DOTS-245678
    LineBreakClass::ULB_AL, // 0x28FB  # BRAILLE PATTERN DOTS-1245678
    LineBreakClass::ULB_AL, // 0x28FC  # BRAILLE PATTERN DOTS-345678
    LineBreakClass::ULB_AL, // 0x28FD  # BRAILLE PATTERN DOTS-1345678
    LineBreakClass::ULB_AL, // 0x28FE  # BRAILLE PATTERN DOTS-2345678
    LineBreakClass::ULB_AL, // 0x28FF  # BRAILLE PATTERN DOTS-12345678
    LineBreakClass::ULB_AL, // 0x2900  # RIGHTWARDS TWO-HEADED ARROW WITH VERTICAL STROKE
    LineBreakClass::ULB_AL, // 0x2901  # RIGHTWARDS TWO-HEADED ARROW WITH DOUBLE VERTICAL STROKE
    LineBreakClass::ULB_AL, // 0x2902  # LEFTWARDS DOUBLE ARROW WITH VERTICAL STROKE
    LineBreakClass::ULB_AL, // 0x2903  # RIGHTWARDS DOUBLE ARROW WITH VERTICAL STROKE
    LineBreakClass::ULB_AL, // 0x2904  # LEFT RIGHT DOUBLE ARROW WITH VERTICAL STROKE
    LineBreakClass::ULB_AL, // 0x2905  # RIGHTWARDS TWO-HEADED ARROW FROM BAR
    LineBreakClass::ULB_AL, // 0x2906  # LEFTWARDS DOUBLE ARROW FROM BAR
    LineBreakClass::ULB_AL, // 0x2907  # RIGHTWARDS DOUBLE ARROW FROM BAR
    LineBreakClass::ULB_AL, // 0x2908  # DOWNWARDS ARROW WITH HORIZONTAL STROKE
    LineBreakClass::ULB_AL, // 0x2909  # UPWARDS ARROW WITH HORIZONTAL STROKE
    LineBreakClass::ULB_AL, // 0x290A  # UPWARDS TRIPLE ARROW
    LineBreakClass::ULB_AL, // 0x290B  # DOWNWARDS TRIPLE ARROW
    LineBreakClass::ULB_AL, // 0x290C  # LEFTWARDS DOUBLE DASH ARROW
    LineBreakClass::ULB_AL, // 0x290D  # RIGHTWARDS DOUBLE DASH ARROW
    LineBreakClass::ULB_AL, // 0x290E  # LEFTWARDS TRIPLE DASH ARROW
    LineBreakClass::ULB_AL, // 0x290F  # RIGHTWARDS TRIPLE DASH ARROW
    LineBreakClass::ULB_AL, // 0x2910  # RIGHTWARDS TWO-HEADED TRIPLE DASH ARROW
    LineBreakClass::ULB_AL, // 0x2911  # RIGHTWARDS ARROW WITH DOTTED STEM
    LineBreakClass::ULB_AL, // 0x2912  # UPWARDS ARROW TO BAR
    LineBreakClass::ULB_AL, // 0x2913  # DOWNWARDS ARROW TO BAR
    LineBreakClass::ULB_AL, // 0x2914  # RIGHTWARDS ARROW WITH TAIL WITH VERTICAL STROKE
    LineBreakClass::ULB_AL, // 0x2915  # RIGHTWARDS ARROW WITH TAIL WITH DOUBLE VERTICAL STROKE
    LineBreakClass::ULB_AL, // 0x2916  # RIGHTWARDS TWO-HEADED ARROW WITH TAIL
    LineBreakClass::ULB_AL, // 0x2917  # RIGHTWARDS TWO-HEADED ARROW WITH TAIL WITH VERTICAL STROKE
    LineBreakClass::ULB_AL, // 0x2918  # RIGHTWARDS TWO-HEADED ARROW WITH TAIL WITH DOUBLE VERTICAL
                            // STROKE
    LineBreakClass::ULB_AL, // 0x2919  # LEFTWARDS ARROW-TAIL
    LineBreakClass::ULB_AL, // 0x291A  # RIGHTWARDS ARROW-TAIL
    LineBreakClass::ULB_AL, // 0x291B  # LEFTWARDS DOUBLE ARROW-TAIL
    LineBreakClass::ULB_AL, // 0x291C  # RIGHTWARDS DOUBLE ARROW-TAIL
    LineBreakClass::ULB_AL, // 0x291D  # LEFTWARDS ARROW TO BLACK DIAMOND
    LineBreakClass::ULB_AL, // 0x291E  # RIGHTWARDS ARROW TO BLACK DIAMOND
    LineBreakClass::ULB_AL, // 0x291F  # LEFTWARDS ARROW FROM BAR TO BLACK DIAMOND
    LineBreakClass::ULB_AL, // 0x2920  # RIGHTWARDS ARROW FROM BAR TO BLACK DIAMOND
    LineBreakClass::ULB_AL, // 0x2921  # NORTH WEST AND SOUTH EAST ARROW
    LineBreakClass::ULB_AL, // 0x2922  # NORTH EAST AND SOUTH WEST ARROW
    LineBreakClass::ULB_AL, // 0x2923  # NORTH WEST ARROW WITH HOOK
    LineBreakClass::ULB_AL, // 0x2924  # NORTH EAST ARROW WITH HOOK
    LineBreakClass::ULB_AL, // 0x2925  # SOUTH EAST ARROW WITH HOOK
    LineBreakClass::ULB_AL, // 0x2926  # SOUTH WEST ARROW WITH HOOK
    LineBreakClass::ULB_AL, // 0x2927  # NORTH WEST ARROW AND NORTH EAST ARROW
    LineBreakClass::ULB_AL, // 0x2928  # NORTH EAST ARROW AND SOUTH EAST ARROW
    LineBreakClass::ULB_AL, // 0x2929  # SOUTH EAST ARROW AND SOUTH WEST ARROW
    LineBreakClass::ULB_AL, // 0x292A  # SOUTH WEST ARROW AND NORTH WEST ARROW
    LineBreakClass::ULB_AL, // 0x292B  # RISING DIAGONAL CROSSING FALLING DIAGONAL
    LineBreakClass::ULB_AL, // 0x292C  # FALLING DIAGONAL CROSSING RISING DIAGONAL
    LineBreakClass::ULB_AL, // 0x292D  # SOUTH EAST ARROW CROSSING NORTH EAST ARROW
    LineBreakClass::ULB_AL, // 0x292E  # NORTH EAST ARROW CROSSING SOUTH EAST ARROW
    LineBreakClass::ULB_AL, // 0x292F  # FALLING DIAGONAL CROSSING NORTH EAST ARROW
    LineBreakClass::ULB_AL, // 0x2930  # RISING DIAGONAL CROSSING SOUTH EAST ARROW
    LineBreakClass::ULB_AL, // 0x2931  # NORTH EAST ARROW CROSSING NORTH WEST ARROW
    LineBreakClass::ULB_AL, // 0x2932  # NORTH WEST ARROW CROSSING NORTH EAST ARROW
    LineBreakClass::ULB_AL, // 0x2933  # WAVE ARROW POINTING DIRECTLY RIGHT
    LineBreakClass::ULB_AL, // 0x2934  # ARROW POINTING RIGHTWARDS THEN CURVING UPWARDS
    LineBreakClass::ULB_AL, // 0x2935  # ARROW POINTING RIGHTWARDS THEN CURVING DOWNWARDS
    LineBreakClass::ULB_AL, // 0x2936  # ARROW POINTING DOWNWARDS THEN CURVING LEFTWARDS
    LineBreakClass::ULB_AL, // 0x2937  # ARROW POINTING DOWNWARDS THEN CURVING RIGHTWARDS
    LineBreakClass::ULB_AL, // 0x2938  # RIGHT-SIDE ARC CLOCKWISE ARROW
    LineBreakClass::ULB_AL, // 0x2939  # LEFT-SIDE ARC ANTICLOCKWISE ARROW
    LineBreakClass::ULB_AL, // 0x293A  # TOP ARC ANTICLOCKWISE ARROW
    LineBreakClass::ULB_AL, // 0x293B  # BOTTOM ARC ANTICLOCKWISE ARROW
    LineBreakClass::ULB_AL, // 0x293C  # TOP ARC CLOCKWISE ARROW WITH MINUS
    LineBreakClass::ULB_AL, // 0x293D  # TOP ARC ANTICLOCKWISE ARROW WITH PLUS
    LineBreakClass::ULB_AL, // 0x293E  # LOWER RIGHT SEMICIRCULAR CLOCKWISE ARROW
    LineBreakClass::ULB_AL, // 0x293F  # LOWER LEFT SEMICIRCULAR ANTICLOCKWISE ARROW
    LineBreakClass::ULB_AL, // 0x2940  # ANTICLOCKWISE CLOSED CIRCLE ARROW
    LineBreakClass::ULB_AL, // 0x2941  # CLOCKWISE CLOSED CIRCLE ARROW
    LineBreakClass::ULB_AL, // 0x2942  # RIGHTWARDS ARROW ABOVE SHORT LEFTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x2943  # LEFTWARDS ARROW ABOVE SHORT RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x2944  # SHORT RIGHTWARDS ARROW ABOVE LEFTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x2945  # RIGHTWARDS ARROW WITH PLUS BELOW
    LineBreakClass::ULB_AL, // 0x2946  # LEFTWARDS ARROW WITH PLUS BELOW
    LineBreakClass::ULB_AL, // 0x2947  # RIGHTWARDS ARROW THROUGH X
    LineBreakClass::ULB_AL, // 0x2948  # LEFT RIGHT ARROW THROUGH SMALL CIRCLE
    LineBreakClass::ULB_AL, // 0x2949  # UPWARDS TWO-HEADED ARROW FROM SMALL CIRCLE
    LineBreakClass::ULB_AL, // 0x294A  # LEFT BARB UP RIGHT BARB DOWN HARPOON
    LineBreakClass::ULB_AL, // 0x294B  # LEFT BARB DOWN RIGHT BARB UP HARPOON
    LineBreakClass::ULB_AL, // 0x294C  # UP BARB RIGHT DOWN BARB LEFT HARPOON
    LineBreakClass::ULB_AL, // 0x294D  # UP BARB LEFT DOWN BARB RIGHT HARPOON
    LineBreakClass::ULB_AL, // 0x294E  # LEFT BARB UP RIGHT BARB UP HARPOON
    LineBreakClass::ULB_AL, // 0x294F  # UP BARB RIGHT DOWN BARB RIGHT HARPOON
    LineBreakClass::ULB_AL, // 0x2950  # LEFT BARB DOWN RIGHT BARB DOWN HARPOON
    LineBreakClass::ULB_AL, // 0x2951  # UP BARB LEFT DOWN BARB LEFT HARPOON
    LineBreakClass::ULB_AL, // 0x2952  # LEFTWARDS HARPOON WITH BARB UP TO BAR
    LineBreakClass::ULB_AL, // 0x2953  # RIGHTWARDS HARPOON WITH BARB UP TO BAR
    LineBreakClass::ULB_AL, // 0x2954  # UPWARDS HARPOON WITH BARB RIGHT TO BAR
    LineBreakClass::ULB_AL, // 0x2955  # DOWNWARDS HARPOON WITH BARB RIGHT TO BAR
    LineBreakClass::ULB_AL, // 0x2956  # LEFTWARDS HARPOON WITH BARB DOWN TO BAR
    LineBreakClass::ULB_AL, // 0x2957  # RIGHTWARDS HARPOON WITH BARB DOWN TO BAR
    LineBreakClass::ULB_AL, // 0x2958  # UPWARDS HARPOON WITH BARB LEFT TO BAR
    LineBreakClass::ULB_AL, // 0x2959  # DOWNWARDS HARPOON WITH BARB LEFT TO BAR
    LineBreakClass::ULB_AL, // 0x295A  # LEFTWARDS HARPOON WITH BARB UP FROM BAR
    LineBreakClass::ULB_AL, // 0x295B  # RIGHTWARDS HARPOON WITH BARB UP FROM BAR
    LineBreakClass::ULB_AL, // 0x295C  # UPWARDS HARPOON WITH BARB RIGHT FROM BAR
    LineBreakClass::ULB_AL, // 0x295D  # DOWNWARDS HARPOON WITH BARB RIGHT FROM BAR
    LineBreakClass::ULB_AL, // 0x295E  # LEFTWARDS HARPOON WITH BARB DOWN FROM BAR
    LineBreakClass::ULB_AL, // 0x295F  # RIGHTWARDS HARPOON WITH BARB DOWN FROM BAR
    LineBreakClass::ULB_AL, // 0x2960  # UPWARDS HARPOON WITH BARB LEFT FROM BAR
    LineBreakClass::ULB_AL, // 0x2961  # DOWNWARDS HARPOON WITH BARB LEFT FROM BAR
    LineBreakClass::ULB_AL, // 0x2962  # LEFTWARDS HARPOON WITH BARB UP ABOVE LEFTWARDS HARPOON
                            // WITH BARB DOWN
    LineBreakClass::ULB_AL, // 0x2963  # UPWARDS HARPOON WITH BARB LEFT BESIDE UPWARDS HARPOON WITH
                            // BARB RIGHT
    LineBreakClass::ULB_AL, // 0x2964  # RIGHTWARDS HARPOON WITH BARB UP ABOVE RIGHTWARDS HARPOON
                            // WITH BARB DOWN
    LineBreakClass::ULB_AL, // 0x2965  # DOWNWARDS HARPOON WITH BARB LEFT BESIDE DOWNWARDS HARPOON
                            // WITH BARB RIGHT
    LineBreakClass::ULB_AL, // 0x2966  # LEFTWARDS HARPOON WITH BARB UP ABOVE RIGHTWARDS HARPOON
                            // WITH BARB UP
    LineBreakClass::ULB_AL, // 0x2967  # LEFTWARDS HARPOON WITH BARB DOWN ABOVE RIGHTWARDS HARPOON
                            // WITH BARB DOWN
    LineBreakClass::ULB_AL, // 0x2968  # RIGHTWARDS HARPOON WITH BARB UP ABOVE LEFTWARDS HARPOON
                            // WITH BARB UP
    LineBreakClass::ULB_AL, // 0x2969  # RIGHTWARDS HARPOON WITH BARB DOWN ABOVE LEFTWARDS HARPOON
                            // WITH BARB DOWN
    LineBreakClass::ULB_AL, // 0x296A  # LEFTWARDS HARPOON WITH BARB UP ABOVE LONG DASH
    LineBreakClass::ULB_AL, // 0x296B  # LEFTWARDS HARPOON WITH BARB DOWN BELOW LONG DASH
    LineBreakClass::ULB_AL, // 0x296C  # RIGHTWARDS HARPOON WITH BARB UP ABOVE LONG DASH
    LineBreakClass::ULB_AL, // 0x296D  # RIGHTWARDS HARPOON WITH BARB DOWN BELOW LONG DASH
    LineBreakClass::ULB_AL, // 0x296E  # UPWARDS HARPOON WITH BARB LEFT BESIDE DOWNWARDS HARPOON
                            // WITH BARB RIGHT
    LineBreakClass::ULB_AL, // 0x296F  # DOWNWARDS HARPOON WITH BARB LEFT BESIDE UPWARDS HARPOON
                            // WITH BARB RIGHT
    LineBreakClass::ULB_AL, // 0x2970  # RIGHT DOUBLE ARROW WITH ROUNDED HEAD
    LineBreakClass::ULB_AL, // 0x2971  # EQUALS SIGN ABOVE RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x2972  # TILDE OPERATOR ABOVE RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x2973  # LEFTWARDS ARROW ABOVE TILDE OPERATOR
    LineBreakClass::ULB_AL, // 0x2974  # RIGHTWARDS ARROW ABOVE TILDE OPERATOR
    LineBreakClass::ULB_AL, // 0x2975  # RIGHTWARDS ARROW ABOVE ALMOST EQUAL TO
    LineBreakClass::ULB_AL, // 0x2976  # LESS-THAN ABOVE LEFTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x2977  # LEFTWARDS ARROW THROUGH LESS-THAN
    LineBreakClass::ULB_AL, // 0x2978  # GREATER-THAN ABOVE RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x2979  # SUBSET ABOVE RIGHTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x297A  # LEFTWARDS ARROW THROUGH SUBSET
    LineBreakClass::ULB_AL, // 0x297B  # SUPERSET ABOVE LEFTWARDS ARROW
    LineBreakClass::ULB_AL, // 0x297C  # LEFT FISH TAIL
    LineBreakClass::ULB_AL, // 0x297D  # RIGHT FISH TAIL
    LineBreakClass::ULB_AL, // 0x297E  # UP FISH TAIL
    LineBreakClass::ULB_AL, // 0x297F  # DOWN FISH TAIL
    LineBreakClass::ULB_AL, // 0x2980  # TRIPLE VERTICAL BAR DELIMITER
    LineBreakClass::ULB_AL, // 0x2981  # Z NOTATION SPOT
    LineBreakClass::ULB_AL, // 0x2982  # Z NOTATION TYPE COLON
    LineBreakClass::ULB_OP, // 0x2983  # LEFT WHITE CURLY BRACKET
    LineBreakClass::ULB_CL, // 0x2984  # RIGHT WHITE CURLY BRACKET
    LineBreakClass::ULB_OP, // 0x2985  # LEFT WHITE PARENTHESIS
    LineBreakClass::ULB_CL, // 0x2986  # RIGHT WHITE PARENTHESIS
    LineBreakClass::ULB_OP, // 0x2987  # Z NOTATION LEFT IMAGE BRACKET
    LineBreakClass::ULB_CL, // 0x2988  # Z NOTATION RIGHT IMAGE BRACKET
    LineBreakClass::ULB_OP, // 0x2989  # Z NOTATION LEFT BINDING BRACKET
    LineBreakClass::ULB_CL, // 0x298A  # Z NOTATION RIGHT BINDING BRACKET
    LineBreakClass::ULB_OP, // 0x298B  # LEFT SQUARE BRACKET WITH UNDERBAR
    LineBreakClass::ULB_CL, // 0x298C  # RIGHT SQUARE BRACKET WITH UNDERBAR
    LineBreakClass::ULB_OP, // 0x298D  # LEFT SQUARE BRACKET WITH TICK IN TOP CORNER
    LineBreakClass::ULB_CL, // 0x298E  # RIGHT SQUARE BRACKET WITH TICK IN BOTTOM CORNER
    LineBreakClass::ULB_OP, // 0x298F  # LEFT SQUARE BRACKET WITH TICK IN BOTTOM CORNER
    LineBreakClass::ULB_CL, // 0x2990  # RIGHT SQUARE BRACKET WITH TICK IN TOP CORNER
    LineBreakClass::ULB_OP, // 0x2991  # LEFT ANGLE BRACKET WITH DOT
    LineBreakClass::ULB_CL, // 0x2992  # RIGHT ANGLE BRACKET WITH DOT
    LineBreakClass::ULB_OP, // 0x2993  # LEFT ARC LESS-THAN BRACKET
    LineBreakClass::ULB_CL, // 0x2994  # RIGHT ARC GREATER-THAN BRACKET
    LineBreakClass::ULB_OP, // 0x2995  # DOUBLE LEFT ARC GREATER-THAN BRACKET
    LineBreakClass::ULB_CL, // 0x2996  # DOUBLE RIGHT ARC LESS-THAN BRACKET
    LineBreakClass::ULB_OP, // 0x2997  # LEFT BLACK TORTOISE SHELL BRACKET
    LineBreakClass::ULB_CL, // 0x2998  # RIGHT BLACK TORTOISE SHELL BRACKET
    LineBreakClass::ULB_AL, // 0x2999  # DOTTED FENCE
    LineBreakClass::ULB_AL, // 0x299A  # VERTICAL ZIGZAG LINE
    LineBreakClass::ULB_AL, // 0x299B  # MEASURED ANGLE OPENING LEFT
    LineBreakClass::ULB_AL, // 0x299C  # RIGHT ANGLE VARIANT WITH SQUARE
    LineBreakClass::ULB_AL, // 0x299D  # MEASURED RIGHT ANGLE WITH DOT
    LineBreakClass::ULB_AL, // 0x299E  # ANGLE WITH S INSIDE
    LineBreakClass::ULB_AL, // 0x299F  # ACUTE ANGLE
    LineBreakClass::ULB_AL, // 0x29A0  # SPHERICAL ANGLE OPENING LEFT
    LineBreakClass::ULB_AL, // 0x29A1  # SPHERICAL ANGLE OPENING UP
    LineBreakClass::ULB_AL, // 0x29A2  # TURNED ANGLE
    LineBreakClass::ULB_AL, // 0x29A3  # REVERSED ANGLE
    LineBreakClass::ULB_AL, // 0x29A4  # ANGLE WITH UNDERBAR
    LineBreakClass::ULB_AL, // 0x29A5  # REVERSED ANGLE WITH UNDERBAR
    LineBreakClass::ULB_AL, // 0x29A6  # OBLIQUE ANGLE OPENING UP
    LineBreakClass::ULB_AL, // 0x29A7  # OBLIQUE ANGLE OPENING DOWN
    LineBreakClass::ULB_AL, // 0x29A8  # MEASURED ANGLE WITH OPEN ARM ENDING IN ARROW POINTING UP
                            // AND RIGHT
    LineBreakClass::ULB_AL, // 0x29A9  # MEASURED ANGLE WITH OPEN ARM ENDING IN ARROW POINTING UP
                            // AND LEFT
    LineBreakClass::ULB_AL, // 0x29AA  # MEASURED ANGLE WITH OPEN ARM ENDING IN ARROW POINTING DOWN
                            // AND RIGHT
    LineBreakClass::ULB_AL, // 0x29AB  # MEASURED ANGLE WITH OPEN ARM ENDING IN ARROW POINTING DOWN
                            // AND LEFT
    LineBreakClass::ULB_AL, // 0x29AC  # MEASURED ANGLE WITH OPEN ARM ENDING IN ARROW POINTING
                            // RIGHT AND UP
    LineBreakClass::ULB_AL, // 0x29AD  # MEASURED ANGLE WITH OPEN ARM ENDING IN ARROW POINTING LEFT
                            // AND UP
    LineBreakClass::ULB_AL, // 0x29AE  # MEASURED ANGLE WITH OPEN ARM ENDING IN ARROW POINTING
                            // RIGHT AND DOWN
    LineBreakClass::ULB_AL, // 0x29AF  # MEASURED ANGLE WITH OPEN ARM ENDING IN ARROW POINTING LEFT
                            // AND DOWN
    LineBreakClass::ULB_AL, // 0x29B0  # REVERSED EMPTY SET
    LineBreakClass::ULB_AL, // 0x29B1  # EMPTY SET WITH OVERBAR
    LineBreakClass::ULB_AL, // 0x29B2  # EMPTY SET WITH SMALL CIRCLE ABOVE
    LineBreakClass::ULB_AL, // 0x29B3  # EMPTY SET WITH RIGHT ARROW ABOVE
    LineBreakClass::ULB_AL, // 0x29B4  # EMPTY SET WITH LEFT ARROW ABOVE
    LineBreakClass::ULB_AL, // 0x29B5  # CIRCLE WITH HORIZONTAL BAR
    LineBreakClass::ULB_AL, // 0x29B6  # CIRCLED VERTICAL BAR
    LineBreakClass::ULB_AL, // 0x29B7  # CIRCLED PARALLEL
    LineBreakClass::ULB_AL, // 0x29B8  # CIRCLED REVERSE SOLIDUS
    LineBreakClass::ULB_AL, // 0x29B9  # CIRCLED PERPENDICULAR
    LineBreakClass::ULB_AL, // 0x29BA  # CIRCLE DIVIDED BY HORIZONTAL BAR AND TOP HALF DIVIDED BY
                            // VERTICAL BAR
    LineBreakClass::ULB_AL, // 0x29BB  # CIRCLE WITH SUPERIMPOSED X
    LineBreakClass::ULB_AL, // 0x29BC  # CIRCLED ANTICLOCKWISE-ROTATED DIVISION SIGN
    LineBreakClass::ULB_AL, // 0x29BD  # UP ARROW THROUGH CIRCLE
    LineBreakClass::ULB_AL, // 0x29BE  # CIRCLED WHITE BULLET
    LineBreakClass::ULB_AL, // 0x29BF  # CIRCLED BULLET
    LineBreakClass::ULB_AL, // 0x29C0  # CIRCLED LESS-THAN
    LineBreakClass::ULB_AL, // 0x29C1  # CIRCLED GREATER-THAN
    LineBreakClass::ULB_AL, // 0x29C2  # CIRCLE WITH SMALL CIRCLE TO THE RIGHT
    LineBreakClass::ULB_AL, // 0x29C3  # CIRCLE WITH TWO HORIZONTAL STROKES TO THE RIGHT
    LineBreakClass::ULB_AL, // 0x29C4  # SQUARED RISING DIAGONAL SLASH
    LineBreakClass::ULB_AL, // 0x29C5  # SQUARED FALLING DIAGONAL SLASH
    LineBreakClass::ULB_AL, // 0x29C6  # SQUARED ASTERISK
    LineBreakClass::ULB_AL, // 0x29C7  # SQUARED SMALL CIRCLE
    LineBreakClass::ULB_AL, // 0x29C8  # SQUARED SQUARE
    LineBreakClass::ULB_AL, // 0x29C9  # TWO JOINED SQUARES
    LineBreakClass::ULB_AL, // 0x29CA  # TRIANGLE WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x29CB  # TRIANGLE WITH UNDERBAR
    LineBreakClass::ULB_AL, // 0x29CC  # S IN TRIANGLE
    LineBreakClass::ULB_AL, // 0x29CD  # TRIANGLE WITH SERIFS AT BOTTOM
    LineBreakClass::ULB_AL, // 0x29CE  # RIGHT TRIANGLE ABOVE LEFT TRIANGLE
    LineBreakClass::ULB_AL, // 0x29CF  # LEFT TRIANGLE BESIDE VERTICAL BAR
    LineBreakClass::ULB_AL, // 0x29D0  # VERTICAL BAR BESIDE RIGHT TRIANGLE
    LineBreakClass::ULB_AL, // 0x29D1  # BOWTIE WITH LEFT HALF BLACK
    LineBreakClass::ULB_AL, // 0x29D2  # BOWTIE WITH RIGHT HALF BLACK
    LineBreakClass::ULB_AL, // 0x29D3  # BLACK BOWTIE
    LineBreakClass::ULB_AL, // 0x29D4  # TIMES WITH LEFT HALF BLACK
    LineBreakClass::ULB_AL, // 0x29D5  # TIMES WITH RIGHT HALF BLACK
    LineBreakClass::ULB_AL, // 0x29D6  # WHITE HOURGLASS
    LineBreakClass::ULB_AL, // 0x29D7  # BLACK HOURGLASS
    LineBreakClass::ULB_OP, // 0x29D8  # LEFT WIGGLY FENCE
    LineBreakClass::ULB_CL, // 0x29D9  # RIGHT WIGGLY FENCE
    LineBreakClass::ULB_OP, // 0x29DA  # LEFT DOUBLE WIGGLY FENCE
    LineBreakClass::ULB_CL, // 0x29DB  # RIGHT DOUBLE WIGGLY FENCE
    LineBreakClass::ULB_AL, // 0x29DC  # INCOMPLETE INFINITY
    LineBreakClass::ULB_AL, // 0x29DD  # TIE OVER INFINITY
    LineBreakClass::ULB_AL, // 0x29DE  # INFINITY NEGATED WITH VERTICAL BAR
    LineBreakClass::ULB_AL, // 0x29DF  # DOUBLE-ENDED MULTIMAP
    LineBreakClass::ULB_AL, // 0x29E0  # SQUARE WITH CONTOURED OUTLINE
    LineBreakClass::ULB_AL, // 0x29E1  # INCREASES AS
    LineBreakClass::ULB_AL, // 0x29E2  # SHUFFLE PRODUCT
    LineBreakClass::ULB_AL, // 0x29E3  # EQUALS SIGN AND SLANTED PARALLEL
    LineBreakClass::ULB_AL, // 0x29E4  # EQUALS SIGN AND SLANTED PARALLEL WITH TILDE ABOVE
    LineBreakClass::ULB_AL, // 0x29E5  # IDENTICAL TO AND SLANTED PARALLEL
    LineBreakClass::ULB_AL, // 0x29E6  # GLEICH STARK
    LineBreakClass::ULB_AL, // 0x29E7  # THERMODYNAMIC
    LineBreakClass::ULB_AL, // 0x29E8  # DOWN-POINTING TRIANGLE WITH LEFT HALF BLACK
    LineBreakClass::ULB_AL, // 0x29E9  # DOWN-POINTING TRIANGLE WITH RIGHT HALF BLACK
    LineBreakClass::ULB_AL, // 0x29EA  # BLACK DIAMOND WITH DOWN ARROW
    LineBreakClass::ULB_AL, // 0x29EB  # BLACK LOZENGE
    LineBreakClass::ULB_AL, // 0x29EC  # WHITE CIRCLE WITH DOWN ARROW
    LineBreakClass::ULB_AL, // 0x29ED  # BLACK CIRCLE WITH DOWN ARROW
    LineBreakClass::ULB_AL, // 0x29EE  # ERROR-BARRED WHITE SQUARE
    LineBreakClass::ULB_AL, // 0x29EF  # ERROR-BARRED BLACK SQUARE
    LineBreakClass::ULB_AL, // 0x29F0  # ERROR-BARRED WHITE DIAMOND
    LineBreakClass::ULB_AL, // 0x29F1  # ERROR-BARRED BLACK DIAMOND
    LineBreakClass::ULB_AL, // 0x29F2  # ERROR-BARRED WHITE CIRCLE
    LineBreakClass::ULB_AL, // 0x29F3  # ERROR-BARRED BLACK CIRCLE
    LineBreakClass::ULB_AL, // 0x29F4  # RULE-DELAYED
    LineBreakClass::ULB_AL, // 0x29F5  # REVERSE SOLIDUS OPERATOR
    LineBreakClass::ULB_AL, // 0x29F6  # SOLIDUS WITH OVERBAR
    LineBreakClass::ULB_AL, // 0x29F7  # REVERSE SOLIDUS WITH HORIZONTAL STROKE
    LineBreakClass::ULB_AL, // 0x29F8  # BIG SOLIDUS
    LineBreakClass::ULB_AL, // 0x29F9  # BIG REVERSE SOLIDUS
    LineBreakClass::ULB_AL, // 0x29FA  # DOUBLE PLUS
    LineBreakClass::ULB_AL, // 0x29FB  # TRIPLE PLUS
    LineBreakClass::ULB_OP, // 0x29FC  # LEFT-POINTING CURVED ANGLE BRACKET
    LineBreakClass::ULB_CL, // 0x29FD  # RIGHT-POINTING CURVED ANGLE BRACKET
    LineBreakClass::ULB_AL, // 0x29FE  # TINY
    LineBreakClass::ULB_AL, // 0x29FF  # MINY
    LineBreakClass::ULB_AL, // 0x2A00  # N-ARY CIRCLED DOT OPERATOR
    LineBreakClass::ULB_AL, // 0x2A01  # N-ARY CIRCLED PLUS OPERATOR
    LineBreakClass::ULB_AL, // 0x2A02  # N-ARY CIRCLED TIMES OPERATOR
    LineBreakClass::ULB_AL, // 0x2A03  # N-ARY UNION OPERATOR WITH DOT
    LineBreakClass::ULB_AL, // 0x2A04  # N-ARY UNION OPERATOR WITH PLUS
    LineBreakClass::ULB_AL, // 0x2A05  # N-ARY SQUARE INTERSECTION OPERATOR
    LineBreakClass::ULB_AL, // 0x2A06  # N-ARY SQUARE UNION OPERATOR
    LineBreakClass::ULB_AL, // 0x2A07  # TWO LOGICAL AND OPERATOR
    LineBreakClass::ULB_AL, // 0x2A08  # TWO LOGICAL OR OPERATOR
    LineBreakClass::ULB_AL, // 0x2A09  # N-ARY TIMES OPERATOR
    LineBreakClass::ULB_AL, // 0x2A0A  # MODULO TWO SUM
    LineBreakClass::ULB_AL, // 0x2A0B  # SUMMATION WITH INTEGRAL
    LineBreakClass::ULB_AL, // 0x2A0C  # QUADRUPLE INTEGRAL OPERATOR
    LineBreakClass::ULB_AL, // 0x2A0D  # FINITE PART INTEGRAL
    LineBreakClass::ULB_AL, // 0x2A0E  # INTEGRAL WITH DOUBLE STROKE
    LineBreakClass::ULB_AL, // 0x2A0F  # INTEGRAL AVERAGE WITH SLASH
    LineBreakClass::ULB_AL, // 0x2A10  # CIRCULATION FUNCTION
    LineBreakClass::ULB_AL, // 0x2A11  # ANTICLOCKWISE INTEGRATION
    LineBreakClass::ULB_AL, // 0x2A12  # LINE INTEGRATION WITH RECTANGULAR PATH AROUND POLE
    LineBreakClass::ULB_AL, // 0x2A13  # LINE INTEGRATION WITH SEMICIRCULAR PATH AROUND POLE
    LineBreakClass::ULB_AL, // 0x2A14  # LINE INTEGRATION NOT INCLUDING THE POLE
    LineBreakClass::ULB_AL, // 0x2A15  # INTEGRAL AROUND A POINT OPERATOR
    LineBreakClass::ULB_AL, // 0x2A16  # QUATERNION INTEGRAL OPERATOR
    LineBreakClass::ULB_AL, // 0x2A17  # INTEGRAL WITH LEFTWARDS ARROW WITH HOOK
    LineBreakClass::ULB_AL, // 0x2A18  # INTEGRAL WITH TIMES SIGN
    LineBreakClass::ULB_AL, // 0x2A19  # INTEGRAL WITH INTERSECTION
    LineBreakClass::ULB_AL, // 0x2A1A  # INTEGRAL WITH UNION
    LineBreakClass::ULB_AL, // 0x2A1B  # INTEGRAL WITH OVERBAR
    LineBreakClass::ULB_AL, // 0x2A1C  # INTEGRAL WITH UNDERBAR
    LineBreakClass::ULB_AL, // 0x2A1D  # JOIN
    LineBreakClass::ULB_AL, // 0x2A1E  # LARGE LEFT TRIANGLE OPERATOR
    LineBreakClass::ULB_AL, // 0x2A1F  # Z NOTATION SCHEMA COMPOSITION
    LineBreakClass::ULB_AL, // 0x2A20  # Z NOTATION SCHEMA PIPING
    LineBreakClass::ULB_AL, // 0x2A21  # Z NOTATION SCHEMA PROJECTION
    LineBreakClass::ULB_AL, // 0x2A22  # PLUS SIGN WITH SMALL CIRCLE ABOVE
    LineBreakClass::ULB_AL, // 0x2A23  # PLUS SIGN WITH CIRCUMFLEX ACCENT ABOVE
    LineBreakClass::ULB_AL, // 0x2A24  # PLUS SIGN WITH TILDE ABOVE
    LineBreakClass::ULB_AL, // 0x2A25  # PLUS SIGN WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x2A26  # PLUS SIGN WITH TILDE BELOW
    LineBreakClass::ULB_AL, // 0x2A27  # PLUS SIGN WITH SUBSCRIPT TWO
    LineBreakClass::ULB_AL, // 0x2A28  # PLUS SIGN WITH BLACK TRIANGLE
    LineBreakClass::ULB_AL, // 0x2A29  # MINUS SIGN WITH COMMA ABOVE
    LineBreakClass::ULB_AL, // 0x2A2A  # MINUS SIGN WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x2A2B  # MINUS SIGN WITH FALLING DOTS
    LineBreakClass::ULB_AL, // 0x2A2C  # MINUS SIGN WITH RISING DOTS
    LineBreakClass::ULB_AL, // 0x2A2D  # PLUS SIGN IN LEFT HALF CIRCLE
    LineBreakClass::ULB_AL, // 0x2A2E  # PLUS SIGN IN RIGHT HALF CIRCLE
    LineBreakClass::ULB_AL, // 0x2A2F  # VECTOR OR CROSS PRODUCT
    LineBreakClass::ULB_AL, // 0x2A30  # MULTIPLICATION SIGN WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x2A31  # MULTIPLICATION SIGN WITH UNDERBAR
    LineBreakClass::ULB_AL, // 0x2A32  # SEMIDIRECT PRODUCT WITH BOTTOM CLOSED
    LineBreakClass::ULB_AL, // 0x2A33  # SMASH PRODUCT
    LineBreakClass::ULB_AL, // 0x2A34  # MULTIPLICATION SIGN IN LEFT HALF CIRCLE
    LineBreakClass::ULB_AL, // 0x2A35  # MULTIPLICATION SIGN IN RIGHT HALF CIRCLE
    LineBreakClass::ULB_AL, // 0x2A36  # CIRCLED MULTIPLICATION SIGN WITH CIRCUMFLEX ACCENT
    LineBreakClass::ULB_AL, // 0x2A37  # MULTIPLICATION SIGN IN DOUBLE CIRCLE
    LineBreakClass::ULB_AL, // 0x2A38  # CIRCLED DIVISION SIGN
    LineBreakClass::ULB_AL, // 0x2A39  # PLUS SIGN IN TRIANGLE
    LineBreakClass::ULB_AL, // 0x2A3A  # MINUS SIGN IN TRIANGLE
    LineBreakClass::ULB_AL, // 0x2A3B  # MULTIPLICATION SIGN IN TRIANGLE
    LineBreakClass::ULB_AL, // 0x2A3C  # INTERIOR PRODUCT
    LineBreakClass::ULB_AL, // 0x2A3D  # RIGHTHAND INTERIOR PRODUCT
    LineBreakClass::ULB_AL, // 0x2A3E  # Z NOTATION RELATIONAL COMPOSITION
    LineBreakClass::ULB_AL, // 0x2A3F  # AMALGAMATION OR COPRODUCT
    LineBreakClass::ULB_AL, // 0x2A40  # INTERSECTION WITH DOT
    LineBreakClass::ULB_AL, // 0x2A41  # UNION WITH MINUS SIGN
    LineBreakClass::ULB_AL, // 0x2A42  # UNION WITH OVERBAR
    LineBreakClass::ULB_AL, // 0x2A43  # INTERSECTION WITH OVERBAR
    LineBreakClass::ULB_AL, // 0x2A44  # INTERSECTION WITH LOGICAL AND
    LineBreakClass::ULB_AL, // 0x2A45  # UNION WITH LOGICAL OR
    LineBreakClass::ULB_AL, // 0x2A46  # UNION ABOVE INTERSECTION
    LineBreakClass::ULB_AL, // 0x2A47  # INTERSECTION ABOVE UNION
    LineBreakClass::ULB_AL, // 0x2A48  # UNION ABOVE BAR ABOVE INTERSECTION
    LineBreakClass::ULB_AL, // 0x2A49  # INTERSECTION ABOVE BAR ABOVE UNION
    LineBreakClass::ULB_AL, // 0x2A4A  # UNION BESIDE AND JOINED WITH UNION
    LineBreakClass::ULB_AL, // 0x2A4B  # INTERSECTION BESIDE AND JOINED WITH INTERSECTION
    LineBreakClass::ULB_AL, // 0x2A4C  # CLOSED UNION WITH SERIFS
    LineBreakClass::ULB_AL, // 0x2A4D  # CLOSED INTERSECTION WITH SERIFS
    LineBreakClass::ULB_AL, // 0x2A4E  # DOUBLE SQUARE INTERSECTION
    LineBreakClass::ULB_AL, // 0x2A4F  # DOUBLE SQUARE UNION
    LineBreakClass::ULB_AL, // 0x2A50  # CLOSED UNION WITH SERIFS AND SMASH PRODUCT
    LineBreakClass::ULB_AL, // 0x2A51  # LOGICAL AND WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x2A52  # LOGICAL OR WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x2A53  # DOUBLE LOGICAL AND
    LineBreakClass::ULB_AL, // 0x2A54  # DOUBLE LOGICAL OR
    LineBreakClass::ULB_AL, // 0x2A55  # TWO INTERSECTING LOGICAL AND
    LineBreakClass::ULB_AL, // 0x2A56  # TWO INTERSECTING LOGICAL OR
    LineBreakClass::ULB_AL, // 0x2A57  # SLOPING LARGE OR
    LineBreakClass::ULB_AL, // 0x2A58  # SLOPING LARGE AND
    LineBreakClass::ULB_AL, // 0x2A59  # LOGICAL OR OVERLAPPING LOGICAL AND
    LineBreakClass::ULB_AL, // 0x2A5A  # LOGICAL AND WITH MIDDLE STEM
    LineBreakClass::ULB_AL, // 0x2A5B  # LOGICAL OR WITH MIDDLE STEM
    LineBreakClass::ULB_AL, // 0x2A5C  # LOGICAL AND WITH HORIZONTAL DASH
    LineBreakClass::ULB_AL, // 0x2A5D  # LOGICAL OR WITH HORIZONTAL DASH
    LineBreakClass::ULB_AL, // 0x2A5E  # LOGICAL AND WITH DOUBLE OVERBAR
    LineBreakClass::ULB_AL, // 0x2A5F  # LOGICAL AND WITH UNDERBAR
    LineBreakClass::ULB_AL, // 0x2A60  # LOGICAL AND WITH DOUBLE UNDERBAR
    LineBreakClass::ULB_AL, // 0x2A61  # SMALL VEE WITH UNDERBAR
    LineBreakClass::ULB_AL, // 0x2A62  # LOGICAL OR WITH DOUBLE OVERBAR
    LineBreakClass::ULB_AL, // 0x2A63  # LOGICAL OR WITH DOUBLE UNDERBAR
    LineBreakClass::ULB_AL, // 0x2A64  # Z NOTATION DOMAIN ANTIRESTRICTION
    LineBreakClass::ULB_AL, // 0x2A65  # Z NOTATION RANGE ANTIRESTRICTION
    LineBreakClass::ULB_AL, // 0x2A66  # EQUALS SIGN WITH DOT BELOW
    LineBreakClass::ULB_AL, // 0x2A67  # IDENTICAL WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x2A68  # TRIPLE HORIZONTAL BAR WITH DOUBLE VERTICAL STROKE
    LineBreakClass::ULB_AL, // 0x2A69  # TRIPLE HORIZONTAL BAR WITH TRIPLE VERTICAL STROKE
    LineBreakClass::ULB_AL, // 0x2A6A  # TILDE OPERATOR WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x2A6B  # TILDE OPERATOR WITH RISING DOTS
    LineBreakClass::ULB_AL, // 0x2A6C  # SIMILAR MINUS SIMILAR
    LineBreakClass::ULB_AL, // 0x2A6D  # CONGRUENT WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x2A6E  # EQUALS WITH ASTERISK
    LineBreakClass::ULB_AL, // 0x2A6F  # ALMOST EQUAL TO WITH CIRCUMFLEX ACCENT
    LineBreakClass::ULB_AL, // 0x2A70  # APPROXIMATELY EQUAL OR EQUAL TO
    LineBreakClass::ULB_AL, // 0x2A71  # EQUALS SIGN ABOVE PLUS SIGN
    LineBreakClass::ULB_AL, // 0x2A72  # PLUS SIGN ABOVE EQUALS SIGN
    LineBreakClass::ULB_AL, // 0x2A73  # EQUALS SIGN ABOVE TILDE OPERATOR
    LineBreakClass::ULB_AL, // 0x2A74  # DOUBLE COLON EQUAL
    LineBreakClass::ULB_AL, // 0x2A75  # TWO CONSECUTIVE EQUALS SIGNS
    LineBreakClass::ULB_AL, // 0x2A76  # THREE CONSECUTIVE EQUALS SIGNS
    LineBreakClass::ULB_AL, // 0x2A77  # EQUALS SIGN WITH TWO DOTS ABOVE AND TWO DOTS BELOW
    LineBreakClass::ULB_AL, // 0x2A78  # EQUIVALENT WITH FOUR DOTS ABOVE
    LineBreakClass::ULB_AL, // 0x2A79  # LESS-THAN WITH CIRCLE INSIDE
    LineBreakClass::ULB_AL, // 0x2A7A  # GREATER-THAN WITH CIRCLE INSIDE
    LineBreakClass::ULB_AL, // 0x2A7B  # LESS-THAN WITH QUESTION MARK ABOVE
    LineBreakClass::ULB_AL, // 0x2A7C  # GREATER-THAN WITH QUESTION MARK ABOVE
    LineBreakClass::ULB_AL, // 0x2A7D  # LESS-THAN OR SLANTED EQUAL TO
    LineBreakClass::ULB_AL, // 0x2A7E  # GREATER-THAN OR SLANTED EQUAL TO
    LineBreakClass::ULB_AL, // 0x2A7F  # LESS-THAN OR SLANTED EQUAL TO WITH DOT INSIDE
    LineBreakClass::ULB_AL, // 0x2A80  # GREATER-THAN OR SLANTED EQUAL TO WITH DOT INSIDE
    LineBreakClass::ULB_AL, // 0x2A81  # LESS-THAN OR SLANTED EQUAL TO WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x2A82  # GREATER-THAN OR SLANTED EQUAL TO WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x2A83  # LESS-THAN OR SLANTED EQUAL TO WITH DOT ABOVE RIGHT
    LineBreakClass::ULB_AL, // 0x2A84  # GREATER-THAN OR SLANTED EQUAL TO WITH DOT ABOVE LEFT
    LineBreakClass::ULB_AL, // 0x2A85  # LESS-THAN OR APPROXIMATE
    LineBreakClass::ULB_AL, // 0x2A86  # GREATER-THAN OR APPROXIMATE
    LineBreakClass::ULB_AL, // 0x2A87  # LESS-THAN AND SINGLE-LINE NOT EQUAL TO
    LineBreakClass::ULB_AL, // 0x2A88  # GREATER-THAN AND SINGLE-LINE NOT EQUAL TO
    LineBreakClass::ULB_AL, // 0x2A89  # LESS-THAN AND NOT APPROXIMATE
    LineBreakClass::ULB_AL, // 0x2A8A  # GREATER-THAN AND NOT APPROXIMATE
    LineBreakClass::ULB_AL, // 0x2A8B  # LESS-THAN ABOVE DOUBLE-LINE EQUAL ABOVE GREATER-THAN
    LineBreakClass::ULB_AL, // 0x2A8C  # GREATER-THAN ABOVE DOUBLE-LINE EQUAL ABOVE LESS-THAN
    LineBreakClass::ULB_AL, // 0x2A8D  # LESS-THAN ABOVE SIMILAR OR EQUAL
    LineBreakClass::ULB_AL, // 0x2A8E  # GREATER-THAN ABOVE SIMILAR OR EQUAL
    LineBreakClass::ULB_AL, // 0x2A8F  # LESS-THAN ABOVE SIMILAR ABOVE GREATER-THAN
    LineBreakClass::ULB_AL, // 0x2A90  # GREATER-THAN ABOVE SIMILAR ABOVE LESS-THAN
    LineBreakClass::ULB_AL, // 0x2A91  # LESS-THAN ABOVE GREATER-THAN ABOVE DOUBLE-LINE EQUAL
    LineBreakClass::ULB_AL, // 0x2A92  # GREATER-THAN ABOVE LESS-THAN ABOVE DOUBLE-LINE EQUAL
    LineBreakClass::ULB_AL, // 0x2A93  # LESS-THAN ABOVE SLANTED EQUAL ABOVE GREATER-THAN ABOVE
                            // SLANTED EQUAL
    LineBreakClass::ULB_AL, // 0x2A94  # GREATER-THAN ABOVE SLANTED EQUAL ABOVE LESS-THAN ABOVE
                            // SLANTED EQUAL
    LineBreakClass::ULB_AL, // 0x2A95  # SLANTED EQUAL TO OR LESS-THAN
    LineBreakClass::ULB_AL, // 0x2A96  # SLANTED EQUAL TO OR GREATER-THAN
    LineBreakClass::ULB_AL, // 0x2A97  # SLANTED EQUAL TO OR LESS-THAN WITH DOT INSIDE
    LineBreakClass::ULB_AL, // 0x2A98  # SLANTED EQUAL TO OR GREATER-THAN WITH DOT INSIDE
    LineBreakClass::ULB_AL, // 0x2A99  # DOUBLE-LINE EQUAL TO OR LESS-THAN
    LineBreakClass::ULB_AL, // 0x2A9A  # DOUBLE-LINE EQUAL TO OR GREATER-THAN
    LineBreakClass::ULB_AL, // 0x2A9B  # DOUBLE-LINE SLANTED EQUAL TO OR LESS-THAN
    LineBreakClass::ULB_AL, // 0x2A9C  # DOUBLE-LINE SLANTED EQUAL TO OR GREATER-THAN
    LineBreakClass::ULB_AL, // 0x2A9D  # SIMILAR OR LESS-THAN
    LineBreakClass::ULB_AL, // 0x2A9E  # SIMILAR OR GREATER-THAN
    LineBreakClass::ULB_AL, // 0x2A9F  # SIMILAR ABOVE LESS-THAN ABOVE EQUALS SIGN
    LineBreakClass::ULB_AL, // 0x2AA0  # SIMILAR ABOVE GREATER-THAN ABOVE EQUALS SIGN
    LineBreakClass::ULB_AL, // 0x2AA1  # DOUBLE NESTED LESS-THAN
    LineBreakClass::ULB_AL, // 0x2AA2  # DOUBLE NESTED GREATER-THAN
    LineBreakClass::ULB_AL, // 0x2AA3  # DOUBLE NESTED LESS-THAN WITH UNDERBAR
    LineBreakClass::ULB_AL, // 0x2AA4  # GREATER-THAN OVERLAPPING LESS-THAN
    LineBreakClass::ULB_AL, // 0x2AA5  # GREATER-THAN BESIDE LESS-THAN
    LineBreakClass::ULB_AL, // 0x2AA6  # LESS-THAN CLOSED BY CURVE
    LineBreakClass::ULB_AL, // 0x2AA7  # GREATER-THAN CLOSED BY CURVE
    LineBreakClass::ULB_AL, // 0x2AA8  # LESS-THAN CLOSED BY CURVE ABOVE SLANTED EQUAL
    LineBreakClass::ULB_AL, // 0x2AA9  # GREATER-THAN CLOSED BY CURVE ABOVE SLANTED EQUAL
    LineBreakClass::ULB_AL, // 0x2AAA  # SMALLER THAN
    LineBreakClass::ULB_AL, // 0x2AAB  # LARGER THAN
    LineBreakClass::ULB_AL, // 0x2AAC  # SMALLER THAN OR EQUAL TO
    LineBreakClass::ULB_AL, // 0x2AAD  # LARGER THAN OR EQUAL TO
    LineBreakClass::ULB_AL, // 0x2AAE  # EQUALS SIGN WITH BUMPY ABOVE
    LineBreakClass::ULB_AL, // 0x2AAF  # PRECEDES ABOVE SINGLE-LINE EQUALS SIGN
    LineBreakClass::ULB_AL, // 0x2AB0  # SUCCEEDS ABOVE SINGLE-LINE EQUALS SIGN
    LineBreakClass::ULB_AL, // 0x2AB1  # PRECEDES ABOVE SINGLE-LINE NOT EQUAL TO
    LineBreakClass::ULB_AL, // 0x2AB2  # SUCCEEDS ABOVE SINGLE-LINE NOT EQUAL TO
    LineBreakClass::ULB_AL, // 0x2AB3  # PRECEDES ABOVE EQUALS SIGN
    LineBreakClass::ULB_AL, // 0x2AB4  # SUCCEEDS ABOVE EQUALS SIGN
    LineBreakClass::ULB_AL, // 0x2AB5  # PRECEDES ABOVE NOT EQUAL TO
    LineBreakClass::ULB_AL, // 0x2AB6  # SUCCEEDS ABOVE NOT EQUAL TO
    LineBreakClass::ULB_AL, // 0x2AB7  # PRECEDES ABOVE ALMOST EQUAL TO
    LineBreakClass::ULB_AL, // 0x2AB8  # SUCCEEDS ABOVE ALMOST EQUAL TO
    LineBreakClass::ULB_AL, // 0x2AB9  # PRECEDES ABOVE NOT ALMOST EQUAL TO
    LineBreakClass::ULB_AL, // 0x2ABA  # SUCCEEDS ABOVE NOT ALMOST EQUAL TO
    LineBreakClass::ULB_AL, // 0x2ABB  # DOUBLE PRECEDES
    LineBreakClass::ULB_AL, // 0x2ABC  # DOUBLE SUCCEEDS
    LineBreakClass::ULB_AL, // 0x2ABD  # SUBSET WITH DOT
    LineBreakClass::ULB_AL, // 0x2ABE  # SUPERSET WITH DOT
    LineBreakClass::ULB_AL, // 0x2ABF  # SUBSET WITH PLUS SIGN BELOW
    LineBreakClass::ULB_AL, // 0x2AC0  # SUPERSET WITH PLUS SIGN BELOW
    LineBreakClass::ULB_AL, // 0x2AC1  # SUBSET WITH MULTIPLICATION SIGN BELOW
    LineBreakClass::ULB_AL, // 0x2AC2  # SUPERSET WITH MULTIPLICATION SIGN BELOW
    LineBreakClass::ULB_AL, // 0x2AC3  # SUBSET OF OR EQUAL TO WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x2AC4  # SUPERSET OF OR EQUAL TO WITH DOT ABOVE
    LineBreakClass::ULB_AL, // 0x2AC5  # SUBSET OF ABOVE EQUALS SIGN
    LineBreakClass::ULB_AL, // 0x2AC6  # SUPERSET OF ABOVE EQUALS SIGN
    LineBreakClass::ULB_AL, // 0x2AC7  # SUBSET OF ABOVE TILDE OPERATOR
    LineBreakClass::ULB_AL, // 0x2AC8  # SUPERSET OF ABOVE TILDE OPERATOR
    LineBreakClass::ULB_AL, // 0x2AC9  # SUBSET OF ABOVE ALMOST EQUAL TO
    LineBreakClass::ULB_AL, // 0x2ACA  # SUPERSET OF ABOVE ALMOST EQUAL TO
    LineBreakClass::ULB_AL, // 0x2ACB  # SUBSET OF ABOVE NOT EQUAL TO
    LineBreakClass::ULB_AL, // 0x2ACC  # SUPERSET OF ABOVE NOT EQUAL TO
    LineBreakClass::ULB_AL, // 0x2ACD  # SQUARE LEFT OPEN BOX OPERATOR
    LineBreakClass::ULB_AL, // 0x2ACE  # SQUARE RIGHT OPEN BOX OPERATOR
    LineBreakClass::ULB_AL, // 0x2ACF  # CLOSED SUBSET
    LineBreakClass::ULB_AL, // 0x2AD0  # CLOSED SUPERSET
    LineBreakClass::ULB_AL, // 0x2AD1  # CLOSED SUBSET OR EQUAL TO
    LineBreakClass::ULB_AL, // 0x2AD2  # CLOSED SUPERSET OR EQUAL TO
    LineBreakClass::ULB_AL, // 0x2AD3  # SUBSET ABOVE SUPERSET
    LineBreakClass::ULB_AL, // 0x2AD4  # SUPERSET ABOVE SUBSET
    LineBreakClass::ULB_AL, // 0x2AD5  # SUBSET ABOVE SUBSET
    LineBreakClass::ULB_AL, // 0x2AD6  # SUPERSET ABOVE SUPERSET
    LineBreakClass::ULB_AL, // 0x2AD7  # SUPERSET BESIDE SUBSET
    LineBreakClass::ULB_AL, // 0x2AD8  # SUPERSET BESIDE AND JOINED BY DASH WITH SUBSET
    LineBreakClass::ULB_AL, // 0x2AD9  # ELEMENT OF OPENING DOWNWARDS
    LineBreakClass::ULB_AL, // 0x2ADA  # PITCHFORK WITH TEE TOP
    LineBreakClass::ULB_AL, // 0x2ADB  # TRANSVERSAL INTERSECTION
    LineBreakClass::ULB_AL, // 0x2ADC  # FORKING
    LineBreakClass::ULB_AL, // 0x2ADD  # NONFORKING
    LineBreakClass::ULB_AL, // 0x2ADE  # SHORT LEFT TACK
    LineBreakClass::ULB_AL, // 0x2ADF  # SHORT DOWN TACK
    LineBreakClass::ULB_AL, // 0x2AE0  # SHORT UP TACK
    LineBreakClass::ULB_AL, // 0x2AE1  # PERPENDICULAR WITH S
    LineBreakClass::ULB_AL, // 0x2AE2  # VERTICAL BAR TRIPLE RIGHT TURNSTILE
    LineBreakClass::ULB_AL, // 0x2AE3  # DOUBLE VERTICAL BAR LEFT TURNSTILE
    LineBreakClass::ULB_AL, // 0x2AE4  # VERTICAL BAR DOUBLE LEFT TURNSTILE
    LineBreakClass::ULB_AL, // 0x2AE5  # DOUBLE VERTICAL BAR DOUBLE LEFT TURNSTILE
    LineBreakClass::ULB_AL, // 0x2AE6  # LONG DASH FROM LEFT MEMBER OF DOUBLE VERTICAL
    LineBreakClass::ULB_AL, // 0x2AE7  # SHORT DOWN TACK WITH OVERBAR
    LineBreakClass::ULB_AL, // 0x2AE8  # SHORT UP TACK WITH UNDERBAR
    LineBreakClass::ULB_AL, // 0x2AE9  # SHORT UP TACK ABOVE SHORT DOWN TACK
    LineBreakClass::ULB_AL, // 0x2AEA  # DOUBLE DOWN TACK
    LineBreakClass::ULB_AL, // 0x2AEB  # DOUBLE UP TACK
    LineBreakClass::ULB_AL, // 0x2AEC  # DOUBLE STROKE NOT SIGN
    LineBreakClass::ULB_AL, // 0x2AED  # REVERSED DOUBLE STROKE NOT SIGN
    LineBreakClass::ULB_AL, // 0x2AEE  # DOES NOT DIVIDE WITH REVERSED NEGATION SLASH
    LineBreakClass::ULB_AL, // 0x2AEF  # VERTICAL LINE WITH CIRCLE ABOVE
    LineBreakClass::ULB_AL, // 0x2AF0  # VERTICAL LINE WITH CIRCLE BELOW
    LineBreakClass::ULB_AL, // 0x2AF1  # DOWN TACK WITH CIRCLE BELOW
    LineBreakClass::ULB_AL, // 0x2AF2  # PARALLEL WITH HORIZONTAL STROKE
    LineBreakClass::ULB_AL, // 0x2AF3  # PARALLEL WITH TILDE OPERATOR
    LineBreakClass::ULB_AL, // 0x2AF4  # TRIPLE VERTICAL BAR BINARY RELATION
    LineBreakClass::ULB_AL, // 0x2AF5  # TRIPLE VERTICAL BAR WITH HORIZONTAL STROKE
    LineBreakClass::ULB_AL, // 0x2AF6  # TRIPLE COLON OPERATOR
    LineBreakClass::ULB_AL, // 0x2AF7  # TRIPLE NESTED LESS-THAN
    LineBreakClass::ULB_AL, // 0x2AF8  # TRIPLE NESTED GREATER-THAN
    LineBreakClass::ULB_AL, // 0x2AF9  # DOUBLE-LINE SLANTED LESS-THAN OR EQUAL TO
    LineBreakClass::ULB_AL, // 0x2AFA  # DOUBLE-LINE SLANTED GREATER-THAN OR EQUAL TO
    LineBreakClass::ULB_AL, // 0x2AFB  # TRIPLE SOLIDUS BINARY RELATION
    LineBreakClass::ULB_AL, // 0x2AFC  # LARGE TRIPLE VERTICAL BAR OPERATOR
    LineBreakClass::ULB_AL, // 0x2AFD  # DOUBLE SOLIDUS OPERATOR
    LineBreakClass::ULB_AL, // 0x2AFE  # WHITE VERTICAL BAR
    LineBreakClass::ULB_AL, // 0x2AFF  # N-ARY WHITE VERTICAL BAR
    LineBreakClass::ULB_AL, // 0x2B00  # NORTH EAST WHITE ARROW
    LineBreakClass::ULB_AL, // 0x2B01  # NORTH WEST WHITE ARROW
    LineBreakClass::ULB_AL, // 0x2B02  # SOUTH EAST WHITE ARROW
    LineBreakClass::ULB_AL, // 0x2B03  # SOUTH WEST WHITE ARROW
    LineBreakClass::ULB_AL, // 0x2B04  # LEFT RIGHT WHITE ARROW
    LineBreakClass::ULB_AL, // 0x2B05  # LEFTWARDS BLACK ARROW
    LineBreakClass::ULB_AL, // 0x2B06  # UPWARDS BLACK ARROW
    LineBreakClass::ULB_AL, // 0x2B07  # DOWNWARDS BLACK ARROW
    LineBreakClass::ULB_AL, // 0x2B08  # NORTH EAST BLACK ARROW
    LineBreakClass::ULB_AL, // 0x2B09  # NORTH WEST BLACK ARROW
    LineBreakClass::ULB_AL, // 0x2B0A  # SOUTH EAST BLACK ARROW
    LineBreakClass::ULB_AL, // 0x2B0B  # SOUTH WEST BLACK ARROW
    LineBreakClass::ULB_AL, // 0x2B0C  # LEFT RIGHT BLACK ARROW
    LineBreakClass::ULB_AL, // 0x2B0D  # UP DOWN BLACK ARROW
    LineBreakClass::ULB_AL, // 0x2B0E  # RIGHTWARDS ARROW WITH TIP DOWNWARDS
    LineBreakClass::ULB_AL, // 0x2B0F  # RIGHTWARDS ARROW WITH TIP UPWARDS
    LineBreakClass::ULB_AL, // 0x2B10  # LEFTWARDS ARROW WITH TIP DOWNWARDS
    LineBreakClass::ULB_AL, // 0x2B11  # LEFTWARDS ARROW WITH TIP UPWARDS
    LineBreakClass::ULB_AL, // 0x2B12  # SQUARE WITH TOP HALF BLACK
    LineBreakClass::ULB_AL, // 0x2B13  # SQUARE WITH BOTTOM HALF BLACK
    LineBreakClass::ULB_AL, // 0x2B14  # SQUARE WITH UPPER RIGHT DIAGONAL HALF BLACK
    LineBreakClass::ULB_AL, // 0x2B15  # SQUARE WITH LOWER LEFT DIAGONAL HALF BLACK
    LineBreakClass::ULB_AL, // 0x2B16  # DIAMOND WITH LEFT HALF BLACK
    LineBreakClass::ULB_AL, // 0x2B17  # DIAMOND WITH RIGHT HALF BLACK
    LineBreakClass::ULB_AL, // 0x2B18  # DIAMOND WITH TOP HALF BLACK
    LineBreakClass::ULB_AL, // 0x2B19  # DIAMOND WITH BOTTOM HALF BLACK
    LineBreakClass::ULB_AL, // 0x2B1A  # DOTTED SQUARE
    LineBreakClass::ULB_ID, // 0x2B1B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B1C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B1D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B1E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B1F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2B20  # WHITE PENTAGON
    LineBreakClass::ULB_AL, // 0x2B21  # WHITE HEXAGON
    LineBreakClass::ULB_AL, // 0x2B22  # BLACK HEXAGON
    LineBreakClass::ULB_AL, // 0x2B23  # HORIZONTAL BLACK HEXAGON
    LineBreakClass::ULB_ID, // 0x2B24 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B25 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B26 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B27 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B28 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B29 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B2A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B2B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B2C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B2D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B2E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B2F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B30 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B31 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B32 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B33 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B34 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B35 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B36 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B37 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B38 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B39 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B3A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B3B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B3C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B3D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B3E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B3F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B40 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B41 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B42 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B43 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B44 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B45 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B46 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B47 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B48 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B49 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B4A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B4B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B4C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B4D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B4E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B4F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B50 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B51 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B52 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B53 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B54 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B55 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B56 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B57 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B58 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B59 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B5A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B5B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B5C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B5D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B5E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B5F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B60 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B61 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B62 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B63 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B64 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B65 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B66 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B67 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B68 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B69 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B6A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B6B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B6C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B6D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B6E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B6F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B70 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B71 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B72 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B73 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B74 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B75 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B76 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B77 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B78 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B79 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B7A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B7B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B7C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B7D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B7E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B7F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B80 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B81 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B82 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B83 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B84 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B85 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B86 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B87 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B88 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B89 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B8A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B8B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B8C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B8D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B8E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B8F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B90 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B91 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B92 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B93 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B94 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B95 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B96 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B97 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B98 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B99 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B9A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B9B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B9C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B9D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B9E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2B9F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BA0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BA1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BA2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BA3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BA4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BA5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BA6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BA7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BA8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BA9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BAA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BAB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BAC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BAD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BAE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BAF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BB0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BB1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BB2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BB3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BB4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BB5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BB6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BB7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BB8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BB9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BBA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BBB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BBC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BBD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BBE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BBF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BC0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BC1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BC2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BC3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BC4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BC5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BC6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BC7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BC8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BC9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BCA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BCB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BCC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BCD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BCE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BCF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BD0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BD1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BD2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BD3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BD4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BD5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BD6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BD7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BD8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BD9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BDA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BDB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BDC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BDD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BDE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BDF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BE0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BE1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BE2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BE3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BE4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BE5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BE6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BE7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BE8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BE9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BEA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BEB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BEC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BED # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BEE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BEF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BF0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BF1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BF2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BF3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BF4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BF5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BF6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BF7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BF8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BF9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BFA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BFB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BFC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BFD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BFE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2BFF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2C00  # GLAGOLITIC CAPITAL LETTER AZU
    LineBreakClass::ULB_AL, // 0x2C01  # GLAGOLITIC CAPITAL LETTER BUKY
    LineBreakClass::ULB_AL, // 0x2C02  # GLAGOLITIC CAPITAL LETTER VEDE
    LineBreakClass::ULB_AL, // 0x2C03  # GLAGOLITIC CAPITAL LETTER GLAGOLI
    LineBreakClass::ULB_AL, // 0x2C04  # GLAGOLITIC CAPITAL LETTER DOBRO
    LineBreakClass::ULB_AL, // 0x2C05  # GLAGOLITIC CAPITAL LETTER YESTU
    LineBreakClass::ULB_AL, // 0x2C06  # GLAGOLITIC CAPITAL LETTER ZHIVETE
    LineBreakClass::ULB_AL, // 0x2C07  # GLAGOLITIC CAPITAL LETTER DZELO
    LineBreakClass::ULB_AL, // 0x2C08  # GLAGOLITIC CAPITAL LETTER ZEMLJA
    LineBreakClass::ULB_AL, // 0x2C09  # GLAGOLITIC CAPITAL LETTER IZHE
    LineBreakClass::ULB_AL, // 0x2C0A  # GLAGOLITIC CAPITAL LETTER INITIAL IZHE
    LineBreakClass::ULB_AL, // 0x2C0B  # GLAGOLITIC CAPITAL LETTER I
    LineBreakClass::ULB_AL, // 0x2C0C  # GLAGOLITIC CAPITAL LETTER DJERVI
    LineBreakClass::ULB_AL, // 0x2C0D  # GLAGOLITIC CAPITAL LETTER KAKO
    LineBreakClass::ULB_AL, // 0x2C0E  # GLAGOLITIC CAPITAL LETTER LJUDIJE
    LineBreakClass::ULB_AL, // 0x2C0F  # GLAGOLITIC CAPITAL LETTER MYSLITE
    LineBreakClass::ULB_AL, // 0x2C10  # GLAGOLITIC CAPITAL LETTER NASHI
    LineBreakClass::ULB_AL, // 0x2C11  # GLAGOLITIC CAPITAL LETTER ONU
    LineBreakClass::ULB_AL, // 0x2C12  # GLAGOLITIC CAPITAL LETTER POKOJI
    LineBreakClass::ULB_AL, // 0x2C13  # GLAGOLITIC CAPITAL LETTER RITSI
    LineBreakClass::ULB_AL, // 0x2C14  # GLAGOLITIC CAPITAL LETTER SLOVO
    LineBreakClass::ULB_AL, // 0x2C15  # GLAGOLITIC CAPITAL LETTER TVRIDO
    LineBreakClass::ULB_AL, // 0x2C16  # GLAGOLITIC CAPITAL LETTER UKU
    LineBreakClass::ULB_AL, // 0x2C17  # GLAGOLITIC CAPITAL LETTER FRITU
    LineBreakClass::ULB_AL, // 0x2C18  # GLAGOLITIC CAPITAL LETTER HERU
    LineBreakClass::ULB_AL, // 0x2C19  # GLAGOLITIC CAPITAL LETTER OTU
    LineBreakClass::ULB_AL, // 0x2C1A  # GLAGOLITIC CAPITAL LETTER PE
    LineBreakClass::ULB_AL, // 0x2C1B  # GLAGOLITIC CAPITAL LETTER SHTA
    LineBreakClass::ULB_AL, // 0x2C1C  # GLAGOLITIC CAPITAL LETTER TSI
    LineBreakClass::ULB_AL, // 0x2C1D  # GLAGOLITIC CAPITAL LETTER CHRIVI
    LineBreakClass::ULB_AL, // 0x2C1E  # GLAGOLITIC CAPITAL LETTER SHA
    LineBreakClass::ULB_AL, // 0x2C1F  # GLAGOLITIC CAPITAL LETTER YERU
    LineBreakClass::ULB_AL, // 0x2C20  # GLAGOLITIC CAPITAL LETTER YERI
    LineBreakClass::ULB_AL, // 0x2C21  # GLAGOLITIC CAPITAL LETTER YATI
    LineBreakClass::ULB_AL, // 0x2C22  # GLAGOLITIC CAPITAL LETTER SPIDERY HA
    LineBreakClass::ULB_AL, // 0x2C23  # GLAGOLITIC CAPITAL LETTER YU
    LineBreakClass::ULB_AL, // 0x2C24  # GLAGOLITIC CAPITAL LETTER SMALL YUS
    LineBreakClass::ULB_AL, // 0x2C25  # GLAGOLITIC CAPITAL LETTER SMALL YUS WITH TAIL
    LineBreakClass::ULB_AL, // 0x2C26  # GLAGOLITIC CAPITAL LETTER YO
    LineBreakClass::ULB_AL, // 0x2C27  # GLAGOLITIC CAPITAL LETTER IOTATED SMALL YUS
    LineBreakClass::ULB_AL, // 0x2C28  # GLAGOLITIC CAPITAL LETTER BIG YUS
    LineBreakClass::ULB_AL, // 0x2C29  # GLAGOLITIC CAPITAL LETTER IOTATED BIG YUS
    LineBreakClass::ULB_AL, // 0x2C2A  # GLAGOLITIC CAPITAL LETTER FITA
    LineBreakClass::ULB_AL, // 0x2C2B  # GLAGOLITIC CAPITAL LETTER IZHITSA
    LineBreakClass::ULB_AL, // 0x2C2C  # GLAGOLITIC CAPITAL LETTER SHTAPIC
    LineBreakClass::ULB_AL, // 0x2C2D  # GLAGOLITIC CAPITAL LETTER TROKUTASTI A
    LineBreakClass::ULB_AL, // 0x2C2E  # GLAGOLITIC CAPITAL LETTER LATINATE MYSLITE
    LineBreakClass::ULB_ID, // 0x2C2F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2C30  # GLAGOLITIC SMALL LETTER AZU
    LineBreakClass::ULB_AL, // 0x2C31  # GLAGOLITIC SMALL LETTER BUKY
    LineBreakClass::ULB_AL, // 0x2C32  # GLAGOLITIC SMALL LETTER VEDE
    LineBreakClass::ULB_AL, // 0x2C33  # GLAGOLITIC SMALL LETTER GLAGOLI
    LineBreakClass::ULB_AL, // 0x2C34  # GLAGOLITIC SMALL LETTER DOBRO
    LineBreakClass::ULB_AL, // 0x2C35  # GLAGOLITIC SMALL LETTER YESTU
    LineBreakClass::ULB_AL, // 0x2C36  # GLAGOLITIC SMALL LETTER ZHIVETE
    LineBreakClass::ULB_AL, // 0x2C37  # GLAGOLITIC SMALL LETTER DZELO
    LineBreakClass::ULB_AL, // 0x2C38  # GLAGOLITIC SMALL LETTER ZEMLJA
    LineBreakClass::ULB_AL, // 0x2C39  # GLAGOLITIC SMALL LETTER IZHE
    LineBreakClass::ULB_AL, // 0x2C3A  # GLAGOLITIC SMALL LETTER INITIAL IZHE
    LineBreakClass::ULB_AL, // 0x2C3B  # GLAGOLITIC SMALL LETTER I
    LineBreakClass::ULB_AL, // 0x2C3C  # GLAGOLITIC SMALL LETTER DJERVI
    LineBreakClass::ULB_AL, // 0x2C3D  # GLAGOLITIC SMALL LETTER KAKO
    LineBreakClass::ULB_AL, // 0x2C3E  # GLAGOLITIC SMALL LETTER LJUDIJE
    LineBreakClass::ULB_AL, // 0x2C3F  # GLAGOLITIC SMALL LETTER MYSLITE
    LineBreakClass::ULB_AL, // 0x2C40  # GLAGOLITIC SMALL LETTER NASHI
    LineBreakClass::ULB_AL, // 0x2C41  # GLAGOLITIC SMALL LETTER ONU
    LineBreakClass::ULB_AL, // 0x2C42  # GLAGOLITIC SMALL LETTER POKOJI
    LineBreakClass::ULB_AL, // 0x2C43  # GLAGOLITIC SMALL LETTER RITSI
    LineBreakClass::ULB_AL, // 0x2C44  # GLAGOLITIC SMALL LETTER SLOVO
    LineBreakClass::ULB_AL, // 0x2C45  # GLAGOLITIC SMALL LETTER TVRIDO
    LineBreakClass::ULB_AL, // 0x2C46  # GLAGOLITIC SMALL LETTER UKU
    LineBreakClass::ULB_AL, // 0x2C47  # GLAGOLITIC SMALL LETTER FRITU
    LineBreakClass::ULB_AL, // 0x2C48  # GLAGOLITIC SMALL LETTER HERU
    LineBreakClass::ULB_AL, // 0x2C49  # GLAGOLITIC SMALL LETTER OTU
    LineBreakClass::ULB_AL, // 0x2C4A  # GLAGOLITIC SMALL LETTER PE
    LineBreakClass::ULB_AL, // 0x2C4B  # GLAGOLITIC SMALL LETTER SHTA
    LineBreakClass::ULB_AL, // 0x2C4C  # GLAGOLITIC SMALL LETTER TSI
    LineBreakClass::ULB_AL, // 0x2C4D  # GLAGOLITIC SMALL LETTER CHRIVI
    LineBreakClass::ULB_AL, // 0x2C4E  # GLAGOLITIC SMALL LETTER SHA
    LineBreakClass::ULB_AL, // 0x2C4F  # GLAGOLITIC SMALL LETTER YERU
    LineBreakClass::ULB_AL, // 0x2C50  # GLAGOLITIC SMALL LETTER YERI
    LineBreakClass::ULB_AL, // 0x2C51  # GLAGOLITIC SMALL LETTER YATI
    LineBreakClass::ULB_AL, // 0x2C52  # GLAGOLITIC SMALL LETTER SPIDERY HA
    LineBreakClass::ULB_AL, // 0x2C53  # GLAGOLITIC SMALL LETTER YU
    LineBreakClass::ULB_AL, // 0x2C54  # GLAGOLITIC SMALL LETTER SMALL YUS
    LineBreakClass::ULB_AL, // 0x2C55  # GLAGOLITIC SMALL LETTER SMALL YUS WITH TAIL
    LineBreakClass::ULB_AL, // 0x2C56  # GLAGOLITIC SMALL LETTER YO
    LineBreakClass::ULB_AL, // 0x2C57  # GLAGOLITIC SMALL LETTER IOTATED SMALL YUS
    LineBreakClass::ULB_AL, // 0x2C58  # GLAGOLITIC SMALL LETTER BIG YUS
    LineBreakClass::ULB_AL, // 0x2C59  # GLAGOLITIC SMALL LETTER IOTATED BIG YUS
    LineBreakClass::ULB_AL, // 0x2C5A  # GLAGOLITIC SMALL LETTER FITA
    LineBreakClass::ULB_AL, // 0x2C5B  # GLAGOLITIC SMALL LETTER IZHITSA
    LineBreakClass::ULB_AL, // 0x2C5C  # GLAGOLITIC SMALL LETTER SHTAPIC
    LineBreakClass::ULB_AL, // 0x2C5D  # GLAGOLITIC SMALL LETTER TROKUTASTI A
    LineBreakClass::ULB_AL, // 0x2C5E  # GLAGOLITIC SMALL LETTER LATINATE MYSLITE
    LineBreakClass::ULB_ID, // 0x2C5F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2C60  # LATIN CAPITAL LETTER L WITH DOUBLE BAR
    LineBreakClass::ULB_AL, // 0x2C61  # LATIN SMALL LETTER L WITH DOUBLE BAR
    LineBreakClass::ULB_AL, // 0x2C62  # LATIN CAPITAL LETTER L WITH MIDDLE TILDE
    LineBreakClass::ULB_AL, // 0x2C63  # LATIN CAPITAL LETTER P WITH STROKE
    LineBreakClass::ULB_AL, // 0x2C64  # LATIN CAPITAL LETTER R WITH TAIL
    LineBreakClass::ULB_AL, // 0x2C65  # LATIN SMALL LETTER A WITH STROKE
    LineBreakClass::ULB_AL, // 0x2C66  # LATIN SMALL LETTER T WITH DIAGONAL STROKE
    LineBreakClass::ULB_AL, // 0x2C67  # LATIN CAPITAL LETTER H WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x2C68  # LATIN SMALL LETTER H WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x2C69  # LATIN CAPITAL LETTER K WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x2C6A  # LATIN SMALL LETTER K WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x2C6B  # LATIN CAPITAL LETTER Z WITH DESCENDER
    LineBreakClass::ULB_AL, // 0x2C6C  # LATIN SMALL LETTER Z WITH DESCENDER
    LineBreakClass::ULB_ID, // 0x2C6D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2C6E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2C6F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2C70 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2C71 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2C72 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2C73 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2C74  # LATIN SMALL LETTER V WITH CURL
    LineBreakClass::ULB_AL, // 0x2C75  # LATIN CAPITAL LETTER HALF H
    LineBreakClass::ULB_AL, // 0x2C76  # LATIN SMALL LETTER HALF H
    LineBreakClass::ULB_AL, // 0x2C77  # LATIN SMALL LETTER TAILLESS PHI
    LineBreakClass::ULB_ID, // 0x2C78 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2C79 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2C7A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2C7B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2C7C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2C7D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2C7E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2C7F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2C80  # COPTIC CAPITAL LETTER ALFA
    LineBreakClass::ULB_AL, // 0x2C81  # COPTIC SMALL LETTER ALFA
    LineBreakClass::ULB_AL, // 0x2C82  # COPTIC CAPITAL LETTER VIDA
    LineBreakClass::ULB_AL, // 0x2C83  # COPTIC SMALL LETTER VIDA
    LineBreakClass::ULB_AL, // 0x2C84  # COPTIC CAPITAL LETTER GAMMA
    LineBreakClass::ULB_AL, // 0x2C85  # COPTIC SMALL LETTER GAMMA
    LineBreakClass::ULB_AL, // 0x2C86  # COPTIC CAPITAL LETTER DALDA
    LineBreakClass::ULB_AL, // 0x2C87  # COPTIC SMALL LETTER DALDA
    LineBreakClass::ULB_AL, // 0x2C88  # COPTIC CAPITAL LETTER EIE
    LineBreakClass::ULB_AL, // 0x2C89  # COPTIC SMALL LETTER EIE
    LineBreakClass::ULB_AL, // 0x2C8A  # COPTIC CAPITAL LETTER SOU
    LineBreakClass::ULB_AL, // 0x2C8B  # COPTIC SMALL LETTER SOU
    LineBreakClass::ULB_AL, // 0x2C8C  # COPTIC CAPITAL LETTER ZATA
    LineBreakClass::ULB_AL, // 0x2C8D  # COPTIC SMALL LETTER ZATA
    LineBreakClass::ULB_AL, // 0x2C8E  # COPTIC CAPITAL LETTER HATE
    LineBreakClass::ULB_AL, // 0x2C8F  # COPTIC SMALL LETTER HATE
    LineBreakClass::ULB_AL, // 0x2C90  # COPTIC CAPITAL LETTER THETHE
    LineBreakClass::ULB_AL, // 0x2C91  # COPTIC SMALL LETTER THETHE
    LineBreakClass::ULB_AL, // 0x2C92  # COPTIC CAPITAL LETTER IAUDA
    LineBreakClass::ULB_AL, // 0x2C93  # COPTIC SMALL LETTER IAUDA
    LineBreakClass::ULB_AL, // 0x2C94  # COPTIC CAPITAL LETTER KAPA
    LineBreakClass::ULB_AL, // 0x2C95  # COPTIC SMALL LETTER KAPA
    LineBreakClass::ULB_AL, // 0x2C96  # COPTIC CAPITAL LETTER LAULA
    LineBreakClass::ULB_AL, // 0x2C97  # COPTIC SMALL LETTER LAULA
    LineBreakClass::ULB_AL, // 0x2C98  # COPTIC CAPITAL LETTER MI
    LineBreakClass::ULB_AL, // 0x2C99  # COPTIC SMALL LETTER MI
    LineBreakClass::ULB_AL, // 0x2C9A  # COPTIC CAPITAL LETTER NI
    LineBreakClass::ULB_AL, // 0x2C9B  # COPTIC SMALL LETTER NI
    LineBreakClass::ULB_AL, // 0x2C9C  # COPTIC CAPITAL LETTER KSI
    LineBreakClass::ULB_AL, // 0x2C9D  # COPTIC SMALL LETTER KSI
    LineBreakClass::ULB_AL, // 0x2C9E  # COPTIC CAPITAL LETTER O
    LineBreakClass::ULB_AL, // 0x2C9F  # COPTIC SMALL LETTER O
    LineBreakClass::ULB_AL, // 0x2CA0  # COPTIC CAPITAL LETTER PI
    LineBreakClass::ULB_AL, // 0x2CA1  # COPTIC SMALL LETTER PI
    LineBreakClass::ULB_AL, // 0x2CA2  # COPTIC CAPITAL LETTER RO
    LineBreakClass::ULB_AL, // 0x2CA3  # COPTIC SMALL LETTER RO
    LineBreakClass::ULB_AL, // 0x2CA4  # COPTIC CAPITAL LETTER SIMA
    LineBreakClass::ULB_AL, // 0x2CA5  # COPTIC SMALL LETTER SIMA
    LineBreakClass::ULB_AL, // 0x2CA6  # COPTIC CAPITAL LETTER TAU
    LineBreakClass::ULB_AL, // 0x2CA7  # COPTIC SMALL LETTER TAU
    LineBreakClass::ULB_AL, // 0x2CA8  # COPTIC CAPITAL LETTER UA
    LineBreakClass::ULB_AL, // 0x2CA9  # COPTIC SMALL LETTER UA
    LineBreakClass::ULB_AL, // 0x2CAA  # COPTIC CAPITAL LETTER FI
    LineBreakClass::ULB_AL, // 0x2CAB  # COPTIC SMALL LETTER FI
    LineBreakClass::ULB_AL, // 0x2CAC  # COPTIC CAPITAL LETTER KHI
    LineBreakClass::ULB_AL, // 0x2CAD  # COPTIC SMALL LETTER KHI
    LineBreakClass::ULB_AL, // 0x2CAE  # COPTIC CAPITAL LETTER PSI
    LineBreakClass::ULB_AL, // 0x2CAF  # COPTIC SMALL LETTER PSI
    LineBreakClass::ULB_AL, // 0x2CB0  # COPTIC CAPITAL LETTER OOU
    LineBreakClass::ULB_AL, // 0x2CB1  # COPTIC SMALL LETTER OOU
    LineBreakClass::ULB_AL, // 0x2CB2  # COPTIC CAPITAL LETTER DIALECT-P ALEF
    LineBreakClass::ULB_AL, // 0x2CB3  # COPTIC SMALL LETTER DIALECT-P ALEF
    LineBreakClass::ULB_AL, // 0x2CB4  # COPTIC CAPITAL LETTER OLD COPTIC AIN
    LineBreakClass::ULB_AL, // 0x2CB5  # COPTIC SMALL LETTER OLD COPTIC AIN
    LineBreakClass::ULB_AL, // 0x2CB6  # COPTIC CAPITAL LETTER CRYPTOGRAMMIC EIE
    LineBreakClass::ULB_AL, // 0x2CB7  # COPTIC SMALL LETTER CRYPTOGRAMMIC EIE
    LineBreakClass::ULB_AL, // 0x2CB8  # COPTIC CAPITAL LETTER DIALECT-P KAPA
    LineBreakClass::ULB_AL, // 0x2CB9  # COPTIC SMALL LETTER DIALECT-P KAPA
    LineBreakClass::ULB_AL, // 0x2CBA  # COPTIC CAPITAL LETTER DIALECT-P NI
    LineBreakClass::ULB_AL, // 0x2CBB  # COPTIC SMALL LETTER DIALECT-P NI
    LineBreakClass::ULB_AL, // 0x2CBC  # COPTIC CAPITAL LETTER CRYPTOGRAMMIC NI
    LineBreakClass::ULB_AL, // 0x2CBD  # COPTIC SMALL LETTER CRYPTOGRAMMIC NI
    LineBreakClass::ULB_AL, // 0x2CBE  # COPTIC CAPITAL LETTER OLD COPTIC OOU
    LineBreakClass::ULB_AL, // 0x2CBF  # COPTIC SMALL LETTER OLD COPTIC OOU
    LineBreakClass::ULB_AL, // 0x2CC0  # COPTIC CAPITAL LETTER SAMPI
    LineBreakClass::ULB_AL, // 0x2CC1  # COPTIC SMALL LETTER SAMPI
    LineBreakClass::ULB_AL, // 0x2CC2  # COPTIC CAPITAL LETTER CROSSED SHEI
    LineBreakClass::ULB_AL, // 0x2CC3  # COPTIC SMALL LETTER CROSSED SHEI
    LineBreakClass::ULB_AL, // 0x2CC4  # COPTIC CAPITAL LETTER OLD COPTIC SHEI
    LineBreakClass::ULB_AL, // 0x2CC5  # COPTIC SMALL LETTER OLD COPTIC SHEI
    LineBreakClass::ULB_AL, // 0x2CC6  # COPTIC CAPITAL LETTER OLD COPTIC ESH
    LineBreakClass::ULB_AL, // 0x2CC7  # COPTIC SMALL LETTER OLD COPTIC ESH
    LineBreakClass::ULB_AL, // 0x2CC8  # COPTIC CAPITAL LETTER AKHMIMIC KHEI
    LineBreakClass::ULB_AL, // 0x2CC9  # COPTIC SMALL LETTER AKHMIMIC KHEI
    LineBreakClass::ULB_AL, // 0x2CCA  # COPTIC CAPITAL LETTER DIALECT-P HORI
    LineBreakClass::ULB_AL, // 0x2CCB  # COPTIC SMALL LETTER DIALECT-P HORI
    LineBreakClass::ULB_AL, // 0x2CCC  # COPTIC CAPITAL LETTER OLD COPTIC HORI
    LineBreakClass::ULB_AL, // 0x2CCD  # COPTIC SMALL LETTER OLD COPTIC HORI
    LineBreakClass::ULB_AL, // 0x2CCE  # COPTIC CAPITAL LETTER OLD COPTIC HA
    LineBreakClass::ULB_AL, // 0x2CCF  # COPTIC SMALL LETTER OLD COPTIC HA
    LineBreakClass::ULB_AL, // 0x2CD0  # COPTIC CAPITAL LETTER L-SHAPED HA
    LineBreakClass::ULB_AL, // 0x2CD1  # COPTIC SMALL LETTER L-SHAPED HA
    LineBreakClass::ULB_AL, // 0x2CD2  # COPTIC CAPITAL LETTER OLD COPTIC HEI
    LineBreakClass::ULB_AL, // 0x2CD3  # COPTIC SMALL LETTER OLD COPTIC HEI
    LineBreakClass::ULB_AL, // 0x2CD4  # COPTIC CAPITAL LETTER OLD COPTIC HAT
    LineBreakClass::ULB_AL, // 0x2CD5  # COPTIC SMALL LETTER OLD COPTIC HAT
    LineBreakClass::ULB_AL, // 0x2CD6  # COPTIC CAPITAL LETTER OLD COPTIC GANGIA
    LineBreakClass::ULB_AL, // 0x2CD7  # COPTIC SMALL LETTER OLD COPTIC GANGIA
    LineBreakClass::ULB_AL, // 0x2CD8  # COPTIC CAPITAL LETTER OLD COPTIC DJA
    LineBreakClass::ULB_AL, // 0x2CD9  # COPTIC SMALL LETTER OLD COPTIC DJA
    LineBreakClass::ULB_AL, // 0x2CDA  # COPTIC CAPITAL LETTER OLD COPTIC SHIMA
    LineBreakClass::ULB_AL, // 0x2CDB  # COPTIC SMALL LETTER OLD COPTIC SHIMA
    LineBreakClass::ULB_AL, // 0x2CDC  # COPTIC CAPITAL LETTER OLD NUBIAN SHIMA
    LineBreakClass::ULB_AL, // 0x2CDD  # COPTIC SMALL LETTER OLD NUBIAN SHIMA
    LineBreakClass::ULB_AL, // 0x2CDE  # COPTIC CAPITAL LETTER OLD NUBIAN NGI
    LineBreakClass::ULB_AL, // 0x2CDF  # COPTIC SMALL LETTER OLD NUBIAN NGI
    LineBreakClass::ULB_AL, // 0x2CE0  # COPTIC CAPITAL LETTER OLD NUBIAN NYI
    LineBreakClass::ULB_AL, // 0x2CE1  # COPTIC SMALL LETTER OLD NUBIAN NYI
    LineBreakClass::ULB_AL, // 0x2CE2  # COPTIC CAPITAL LETTER OLD NUBIAN WAU
    LineBreakClass::ULB_AL, // 0x2CE3  # COPTIC SMALL LETTER OLD NUBIAN WAU
    LineBreakClass::ULB_AL, // 0x2CE4  # COPTIC SYMBOL KAI
    LineBreakClass::ULB_AL, // 0x2CE5  # COPTIC SYMBOL MI RO
    LineBreakClass::ULB_AL, // 0x2CE6  # COPTIC SYMBOL PI RO
    LineBreakClass::ULB_AL, // 0x2CE7  # COPTIC SYMBOL STAUROS
    LineBreakClass::ULB_AL, // 0x2CE8  # COPTIC SYMBOL TAU RO
    LineBreakClass::ULB_AL, // 0x2CE9  # COPTIC SYMBOL KHI RO
    LineBreakClass::ULB_AL, // 0x2CEA  # COPTIC SYMBOL SHIMA SIMA
    LineBreakClass::ULB_ID, // 0x2CEB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2CEC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2CED # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2CEE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2CEF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2CF0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2CF1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2CF2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2CF3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2CF4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2CF5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2CF6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2CF7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2CF8 # <UNDEFINED>
    LineBreakClass::ULB_BA, // 0x2CF9  # COPTIC OLD NUBIAN FULL STOP
    LineBreakClass::ULB_BA, // 0x2CFA  # COPTIC OLD NUBIAN DIRECT QUESTION MARK
    LineBreakClass::ULB_BA, // 0x2CFB  # COPTIC OLD NUBIAN INDIRECT QUESTION MARK
    LineBreakClass::ULB_BA, // 0x2CFC  # COPTIC OLD NUBIAN VERSE DIVIDER
    LineBreakClass::ULB_AL, // 0x2CFD  # COPTIC FRACTION ONE HALF
    LineBreakClass::ULB_BA, // 0x2CFE  # COPTIC FULL STOP
    LineBreakClass::ULB_BA, // 0x2CFF  # COPTIC MORPHOLOGICAL DIVIDER
    LineBreakClass::ULB_AL, // 0x2D00  # GEORGIAN SMALL LETTER AN
    LineBreakClass::ULB_AL, // 0x2D01  # GEORGIAN SMALL LETTER BAN
    LineBreakClass::ULB_AL, // 0x2D02  # GEORGIAN SMALL LETTER GAN
    LineBreakClass::ULB_AL, // 0x2D03  # GEORGIAN SMALL LETTER DON
    LineBreakClass::ULB_AL, // 0x2D04  # GEORGIAN SMALL LETTER EN
    LineBreakClass::ULB_AL, // 0x2D05  # GEORGIAN SMALL LETTER VIN
    LineBreakClass::ULB_AL, // 0x2D06  # GEORGIAN SMALL LETTER ZEN
    LineBreakClass::ULB_AL, // 0x2D07  # GEORGIAN SMALL LETTER TAN
    LineBreakClass::ULB_AL, // 0x2D08  # GEORGIAN SMALL LETTER IN
    LineBreakClass::ULB_AL, // 0x2D09  # GEORGIAN SMALL LETTER KAN
    LineBreakClass::ULB_AL, // 0x2D0A  # GEORGIAN SMALL LETTER LAS
    LineBreakClass::ULB_AL, // 0x2D0B  # GEORGIAN SMALL LETTER MAN
    LineBreakClass::ULB_AL, // 0x2D0C  # GEORGIAN SMALL LETTER NAR
    LineBreakClass::ULB_AL, // 0x2D0D  # GEORGIAN SMALL LETTER ON
    LineBreakClass::ULB_AL, // 0x2D0E  # GEORGIAN SMALL LETTER PAR
    LineBreakClass::ULB_AL, // 0x2D0F  # GEORGIAN SMALL LETTER ZHAR
    LineBreakClass::ULB_AL, // 0x2D10  # GEORGIAN SMALL LETTER RAE
    LineBreakClass::ULB_AL, // 0x2D11  # GEORGIAN SMALL LETTER SAN
    LineBreakClass::ULB_AL, // 0x2D12  # GEORGIAN SMALL LETTER TAR
    LineBreakClass::ULB_AL, // 0x2D13  # GEORGIAN SMALL LETTER UN
    LineBreakClass::ULB_AL, // 0x2D14  # GEORGIAN SMALL LETTER PHAR
    LineBreakClass::ULB_AL, // 0x2D15  # GEORGIAN SMALL LETTER KHAR
    LineBreakClass::ULB_AL, // 0x2D16  # GEORGIAN SMALL LETTER GHAN
    LineBreakClass::ULB_AL, // 0x2D17  # GEORGIAN SMALL LETTER QAR
    LineBreakClass::ULB_AL, // 0x2D18  # GEORGIAN SMALL LETTER SHIN
    LineBreakClass::ULB_AL, // 0x2D19  # GEORGIAN SMALL LETTER CHIN
    LineBreakClass::ULB_AL, // 0x2D1A  # GEORGIAN SMALL LETTER CAN
    LineBreakClass::ULB_AL, // 0x2D1B  # GEORGIAN SMALL LETTER JIL
    LineBreakClass::ULB_AL, // 0x2D1C  # GEORGIAN SMALL LETTER CIL
    LineBreakClass::ULB_AL, // 0x2D1D  # GEORGIAN SMALL LETTER CHAR
    LineBreakClass::ULB_AL, // 0x2D1E  # GEORGIAN SMALL LETTER XAN
    LineBreakClass::ULB_AL, // 0x2D1F  # GEORGIAN SMALL LETTER JHAN
    LineBreakClass::ULB_AL, // 0x2D20  # GEORGIAN SMALL LETTER HAE
    LineBreakClass::ULB_AL, // 0x2D21  # GEORGIAN SMALL LETTER HE
    LineBreakClass::ULB_AL, // 0x2D22  # GEORGIAN SMALL LETTER HIE
    LineBreakClass::ULB_AL, // 0x2D23  # GEORGIAN SMALL LETTER WE
    LineBreakClass::ULB_AL, // 0x2D24  # GEORGIAN SMALL LETTER HAR
    LineBreakClass::ULB_AL, // 0x2D25  # GEORGIAN SMALL LETTER HOE
    LineBreakClass::ULB_ID, // 0x2D26 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D27 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D28 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D29 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D2A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D2B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D2C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D2D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D2E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D2F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2D30  # TIFINAGH LETTER YA
    LineBreakClass::ULB_AL, // 0x2D31  # TIFINAGH LETTER YAB
    LineBreakClass::ULB_AL, // 0x2D32  # TIFINAGH LETTER YABH
    LineBreakClass::ULB_AL, // 0x2D33  # TIFINAGH LETTER YAG
    LineBreakClass::ULB_AL, // 0x2D34  # TIFINAGH LETTER YAGHH
    LineBreakClass::ULB_AL, // 0x2D35  # TIFINAGH LETTER BERBER ACADEMY YAJ
    LineBreakClass::ULB_AL, // 0x2D36  # TIFINAGH LETTER YAJ
    LineBreakClass::ULB_AL, // 0x2D37  # TIFINAGH LETTER YAD
    LineBreakClass::ULB_AL, // 0x2D38  # TIFINAGH LETTER YADH
    LineBreakClass::ULB_AL, // 0x2D39  # TIFINAGH LETTER YADD
    LineBreakClass::ULB_AL, // 0x2D3A  # TIFINAGH LETTER YADDH
    LineBreakClass::ULB_AL, // 0x2D3B  # TIFINAGH LETTER YEY
    LineBreakClass::ULB_AL, // 0x2D3C  # TIFINAGH LETTER YAF
    LineBreakClass::ULB_AL, // 0x2D3D  # TIFINAGH LETTER YAK
    LineBreakClass::ULB_AL, // 0x2D3E  # TIFINAGH LETTER TUAREG YAK
    LineBreakClass::ULB_AL, // 0x2D3F  # TIFINAGH LETTER YAKHH
    LineBreakClass::ULB_AL, // 0x2D40  # TIFINAGH LETTER YAH
    LineBreakClass::ULB_AL, // 0x2D41  # TIFINAGH LETTER BERBER ACADEMY YAH
    LineBreakClass::ULB_AL, // 0x2D42  # TIFINAGH LETTER TUAREG YAH
    LineBreakClass::ULB_AL, // 0x2D43  # TIFINAGH LETTER YAHH
    LineBreakClass::ULB_AL, // 0x2D44  # TIFINAGH LETTER YAA
    LineBreakClass::ULB_AL, // 0x2D45  # TIFINAGH LETTER YAKH
    LineBreakClass::ULB_AL, // 0x2D46  # TIFINAGH LETTER TUAREG YAKH
    LineBreakClass::ULB_AL, // 0x2D47  # TIFINAGH LETTER YAQ
    LineBreakClass::ULB_AL, // 0x2D48  # TIFINAGH LETTER TUAREG YAQ
    LineBreakClass::ULB_AL, // 0x2D49  # TIFINAGH LETTER YI
    LineBreakClass::ULB_AL, // 0x2D4A  # TIFINAGH LETTER YAZH
    LineBreakClass::ULB_AL, // 0x2D4B  # TIFINAGH LETTER AHAGGAR YAZH
    LineBreakClass::ULB_AL, // 0x2D4C  # TIFINAGH LETTER TUAREG YAZH
    LineBreakClass::ULB_AL, // 0x2D4D  # TIFINAGH LETTER YAL
    LineBreakClass::ULB_AL, // 0x2D4E  # TIFINAGH LETTER YAM
    LineBreakClass::ULB_AL, // 0x2D4F  # TIFINAGH LETTER YAN
    LineBreakClass::ULB_AL, // 0x2D50  # TIFINAGH LETTER TUAREG YAGN
    LineBreakClass::ULB_AL, // 0x2D51  # TIFINAGH LETTER TUAREG YANG
    LineBreakClass::ULB_AL, // 0x2D52  # TIFINAGH LETTER YAP
    LineBreakClass::ULB_AL, // 0x2D53  # TIFINAGH LETTER YU
    LineBreakClass::ULB_AL, // 0x2D54  # TIFINAGH LETTER YAR
    LineBreakClass::ULB_AL, // 0x2D55  # TIFINAGH LETTER YARR
    LineBreakClass::ULB_AL, // 0x2D56  # TIFINAGH LETTER YAGH
    LineBreakClass::ULB_AL, // 0x2D57  # TIFINAGH LETTER TUAREG YAGH
    LineBreakClass::ULB_AL, // 0x2D58  # TIFINAGH LETTER AYER YAGH
    LineBreakClass::ULB_AL, // 0x2D59  # TIFINAGH LETTER YAS
    LineBreakClass::ULB_AL, // 0x2D5A  # TIFINAGH LETTER YASS
    LineBreakClass::ULB_AL, // 0x2D5B  # TIFINAGH LETTER YASH
    LineBreakClass::ULB_AL, // 0x2D5C  # TIFINAGH LETTER YAT
    LineBreakClass::ULB_AL, // 0x2D5D  # TIFINAGH LETTER YATH
    LineBreakClass::ULB_AL, // 0x2D5E  # TIFINAGH LETTER YACH
    LineBreakClass::ULB_AL, // 0x2D5F  # TIFINAGH LETTER YATT
    LineBreakClass::ULB_AL, // 0x2D60  # TIFINAGH LETTER YAV
    LineBreakClass::ULB_AL, // 0x2D61  # TIFINAGH LETTER YAW
    LineBreakClass::ULB_AL, // 0x2D62  # TIFINAGH LETTER YAY
    LineBreakClass::ULB_AL, // 0x2D63  # TIFINAGH LETTER YAZ
    LineBreakClass::ULB_AL, // 0x2D64  # TIFINAGH LETTER TAWELLEMET YAZ
    LineBreakClass::ULB_AL, // 0x2D65  # TIFINAGH LETTER YAZZ
    LineBreakClass::ULB_ID, // 0x2D66 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D67 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D68 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D69 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D6A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D6B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D6C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D6D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D6E # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2D6F  # TIFINAGH MODIFIER LETTER LABIALIZATION MARK
    LineBreakClass::ULB_ID, // 0x2D70 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D71 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D72 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D73 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D74 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D75 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D76 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D77 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D78 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D79 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D7A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D7B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D7C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D7D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D7E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D7F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2D80  # ETHIOPIC SYLLABLE LOA
    LineBreakClass::ULB_AL, // 0x2D81  # ETHIOPIC SYLLABLE MOA
    LineBreakClass::ULB_AL, // 0x2D82  # ETHIOPIC SYLLABLE ROA
    LineBreakClass::ULB_AL, // 0x2D83  # ETHIOPIC SYLLABLE SOA
    LineBreakClass::ULB_AL, // 0x2D84  # ETHIOPIC SYLLABLE SHOA
    LineBreakClass::ULB_AL, // 0x2D85  # ETHIOPIC SYLLABLE BOA
    LineBreakClass::ULB_AL, // 0x2D86  # ETHIOPIC SYLLABLE TOA
    LineBreakClass::ULB_AL, // 0x2D87  # ETHIOPIC SYLLABLE COA
    LineBreakClass::ULB_AL, // 0x2D88  # ETHIOPIC SYLLABLE NOA
    LineBreakClass::ULB_AL, // 0x2D89  # ETHIOPIC SYLLABLE NYOA
    LineBreakClass::ULB_AL, // 0x2D8A  # ETHIOPIC SYLLABLE GLOTTAL OA
    LineBreakClass::ULB_AL, // 0x2D8B  # ETHIOPIC SYLLABLE ZOA
    LineBreakClass::ULB_AL, // 0x2D8C  # ETHIOPIC SYLLABLE DOA
    LineBreakClass::ULB_AL, // 0x2D8D  # ETHIOPIC SYLLABLE DDOA
    LineBreakClass::ULB_AL, // 0x2D8E  # ETHIOPIC SYLLABLE JOA
    LineBreakClass::ULB_AL, // 0x2D8F  # ETHIOPIC SYLLABLE THOA
    LineBreakClass::ULB_AL, // 0x2D90  # ETHIOPIC SYLLABLE CHOA
    LineBreakClass::ULB_AL, // 0x2D91  # ETHIOPIC SYLLABLE PHOA
    LineBreakClass::ULB_AL, // 0x2D92  # ETHIOPIC SYLLABLE POA
    LineBreakClass::ULB_AL, // 0x2D93  # ETHIOPIC SYLLABLE GGWA
    LineBreakClass::ULB_AL, // 0x2D94  # ETHIOPIC SYLLABLE GGWI
    LineBreakClass::ULB_AL, // 0x2D95  # ETHIOPIC SYLLABLE GGWEE
    LineBreakClass::ULB_AL, // 0x2D96  # ETHIOPIC SYLLABLE GGWE
    LineBreakClass::ULB_ID, // 0x2D97 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D98 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D99 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D9A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D9B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D9C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D9D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D9E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2D9F # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2DA0  # ETHIOPIC SYLLABLE SSA
    LineBreakClass::ULB_AL, // 0x2DA1  # ETHIOPIC SYLLABLE SSU
    LineBreakClass::ULB_AL, // 0x2DA2  # ETHIOPIC SYLLABLE SSI
    LineBreakClass::ULB_AL, // 0x2DA3  # ETHIOPIC SYLLABLE SSAA
    LineBreakClass::ULB_AL, // 0x2DA4  # ETHIOPIC SYLLABLE SSEE
    LineBreakClass::ULB_AL, // 0x2DA5  # ETHIOPIC SYLLABLE SSE
    LineBreakClass::ULB_AL, // 0x2DA6  # ETHIOPIC SYLLABLE SSO
    LineBreakClass::ULB_ID, // 0x2DA7 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2DA8  # ETHIOPIC SYLLABLE CCA
    LineBreakClass::ULB_AL, // 0x2DA9  # ETHIOPIC SYLLABLE CCU
    LineBreakClass::ULB_AL, // 0x2DAA  # ETHIOPIC SYLLABLE CCI
    LineBreakClass::ULB_AL, // 0x2DAB  # ETHIOPIC SYLLABLE CCAA
    LineBreakClass::ULB_AL, // 0x2DAC  # ETHIOPIC SYLLABLE CCEE
    LineBreakClass::ULB_AL, // 0x2DAD  # ETHIOPIC SYLLABLE CCE
    LineBreakClass::ULB_AL, // 0x2DAE  # ETHIOPIC SYLLABLE CCO
    LineBreakClass::ULB_ID, // 0x2DAF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2DB0  # ETHIOPIC SYLLABLE ZZA
    LineBreakClass::ULB_AL, // 0x2DB1  # ETHIOPIC SYLLABLE ZZU
    LineBreakClass::ULB_AL, // 0x2DB2  # ETHIOPIC SYLLABLE ZZI
    LineBreakClass::ULB_AL, // 0x2DB3  # ETHIOPIC SYLLABLE ZZAA
    LineBreakClass::ULB_AL, // 0x2DB4  # ETHIOPIC SYLLABLE ZZEE
    LineBreakClass::ULB_AL, // 0x2DB5  # ETHIOPIC SYLLABLE ZZE
    LineBreakClass::ULB_AL, // 0x2DB6  # ETHIOPIC SYLLABLE ZZO
    LineBreakClass::ULB_ID, // 0x2DB7 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2DB8  # ETHIOPIC SYLLABLE CCHA
    LineBreakClass::ULB_AL, // 0x2DB9  # ETHIOPIC SYLLABLE CCHU
    LineBreakClass::ULB_AL, // 0x2DBA  # ETHIOPIC SYLLABLE CCHI
    LineBreakClass::ULB_AL, // 0x2DBB  # ETHIOPIC SYLLABLE CCHAA
    LineBreakClass::ULB_AL, // 0x2DBC  # ETHIOPIC SYLLABLE CCHEE
    LineBreakClass::ULB_AL, // 0x2DBD  # ETHIOPIC SYLLABLE CCHE
    LineBreakClass::ULB_AL, // 0x2DBE  # ETHIOPIC SYLLABLE CCHO
    LineBreakClass::ULB_ID, // 0x2DBF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2DC0  # ETHIOPIC SYLLABLE QYA
    LineBreakClass::ULB_AL, // 0x2DC1  # ETHIOPIC SYLLABLE QYU
    LineBreakClass::ULB_AL, // 0x2DC2  # ETHIOPIC SYLLABLE QYI
    LineBreakClass::ULB_AL, // 0x2DC3  # ETHIOPIC SYLLABLE QYAA
    LineBreakClass::ULB_AL, // 0x2DC4  # ETHIOPIC SYLLABLE QYEE
    LineBreakClass::ULB_AL, // 0x2DC5  # ETHIOPIC SYLLABLE QYE
    LineBreakClass::ULB_AL, // 0x2DC6  # ETHIOPIC SYLLABLE QYO
    LineBreakClass::ULB_ID, // 0x2DC7 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2DC8  # ETHIOPIC SYLLABLE KYA
    LineBreakClass::ULB_AL, // 0x2DC9  # ETHIOPIC SYLLABLE KYU
    LineBreakClass::ULB_AL, // 0x2DCA  # ETHIOPIC SYLLABLE KYI
    LineBreakClass::ULB_AL, // 0x2DCB  # ETHIOPIC SYLLABLE KYAA
    LineBreakClass::ULB_AL, // 0x2DCC  # ETHIOPIC SYLLABLE KYEE
    LineBreakClass::ULB_AL, // 0x2DCD  # ETHIOPIC SYLLABLE KYE
    LineBreakClass::ULB_AL, // 0x2DCE  # ETHIOPIC SYLLABLE KYO
    LineBreakClass::ULB_ID, // 0x2DCF # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2DD0  # ETHIOPIC SYLLABLE XYA
    LineBreakClass::ULB_AL, // 0x2DD1  # ETHIOPIC SYLLABLE XYU
    LineBreakClass::ULB_AL, // 0x2DD2  # ETHIOPIC SYLLABLE XYI
    LineBreakClass::ULB_AL, // 0x2DD3  # ETHIOPIC SYLLABLE XYAA
    LineBreakClass::ULB_AL, // 0x2DD4  # ETHIOPIC SYLLABLE XYEE
    LineBreakClass::ULB_AL, // 0x2DD5  # ETHIOPIC SYLLABLE XYE
    LineBreakClass::ULB_AL, // 0x2DD6  # ETHIOPIC SYLLABLE XYO
    LineBreakClass::ULB_ID, // 0x2DD7 # <UNDEFINED>
    LineBreakClass::ULB_AL, // 0x2DD8  # ETHIOPIC SYLLABLE GYA
    LineBreakClass::ULB_AL, // 0x2DD9  # ETHIOPIC SYLLABLE GYU
    LineBreakClass::ULB_AL, // 0x2DDA  # ETHIOPIC SYLLABLE GYI
    LineBreakClass::ULB_AL, // 0x2DDB  # ETHIOPIC SYLLABLE GYAA
    LineBreakClass::ULB_AL, // 0x2DDC  # ETHIOPIC SYLLABLE GYEE
    LineBreakClass::ULB_AL, // 0x2DDD  # ETHIOPIC SYLLABLE GYE
    LineBreakClass::ULB_AL, // 0x2DDE  # ETHIOPIC SYLLABLE GYO
    LineBreakClass::ULB_ID, // 0x2DDF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DE0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DE1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DE2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DE3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DE4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DE5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DE6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DE7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DE8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DE9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DEA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DEB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DEC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DED # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DEE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DEF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DF0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DF1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DF2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DF3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DF4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DF5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DF6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DF7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DF8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DF9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DFA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DFB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DFC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DFD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DFE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2DFF # <UNDEFINED>
    LineBreakClass::ULB_QU, // 0x2E00  # RIGHT ANGLE SUBSTITUTION MARKER
    LineBreakClass::ULB_QU, // 0x2E01  # RIGHT ANGLE DOTTED SUBSTITUTION MARKER
    LineBreakClass::ULB_QU, // 0x2E02  # LEFT SUBSTITUTION BRACKET
    LineBreakClass::ULB_QU, // 0x2E03  # RIGHT SUBSTITUTION BRACKET
    LineBreakClass::ULB_QU, // 0x2E04  # LEFT DOTTED SUBSTITUTION BRACKET
    LineBreakClass::ULB_QU, // 0x2E05  # RIGHT DOTTED SUBSTITUTION BRACKET
    LineBreakClass::ULB_QU, // 0x2E06  # RAISED INTERPOLATION MARKER
    LineBreakClass::ULB_QU, // 0x2E07  # RAISED DOTTED INTERPOLATION MARKER
    LineBreakClass::ULB_QU, // 0x2E08  # DOTTED TRANSPOSITION MARKER
    LineBreakClass::ULB_QU, // 0x2E09  # LEFT TRANSPOSITION BRACKET
    LineBreakClass::ULB_QU, // 0x2E0A  # RIGHT TRANSPOSITION BRACKET
    LineBreakClass::ULB_QU, // 0x2E0B  # RAISED SQUARE
    LineBreakClass::ULB_QU, // 0x2E0C  # LEFT RAISED OMISSION BRACKET
    LineBreakClass::ULB_QU, // 0x2E0D  # RIGHT RAISED OMISSION BRACKET
    LineBreakClass::ULB_BA, // 0x2E0E  # EDITORIAL CORONIS
    LineBreakClass::ULB_BA, // 0x2E0F  # PARAGRAPHOS
    LineBreakClass::ULB_BA, // 0x2E10  # FORKED PARAGRAPHOS
    LineBreakClass::ULB_BA, // 0x2E11  # REVERSED FORKED PARAGRAPHOS
    LineBreakClass::ULB_BA, // 0x2E12  # HYPODIASTOLE
    LineBreakClass::ULB_BA, // 0x2E13  # DOTTED OBELOS
    LineBreakClass::ULB_BA, // 0x2E14  # DOWNWARDS ANCORA
    LineBreakClass::ULB_BA, // 0x2E15  # UPWARDS ANCORA
    LineBreakClass::ULB_AL, // 0x2E16  # DOTTED RIGHT-POINTING ANGLE
    LineBreakClass::ULB_BA, // 0x2E17  # DOUBLE OBLIQUE HYPHEN
    LineBreakClass::ULB_ID, // 0x2E18 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E19 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E1A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E1B # <UNDEFINED>
    LineBreakClass::ULB_QU, // 0x2E1C  # LEFT LOW PARAPHRASE BRACKET
    LineBreakClass::ULB_QU, // 0x2E1D  # RIGHT LOW PARAPHRASE BRACKET
    LineBreakClass::ULB_ID, // 0x2E1E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E1F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E20 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E21 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E22 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E23 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E24 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E25 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E26 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E27 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E28 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E29 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E2A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E2B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E2C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E2D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E2E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E2F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E30 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E31 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E32 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E33 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E34 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E35 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E36 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E37 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E38 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E39 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E3A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E3B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E3C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E3D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E3E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E3F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E40 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E41 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E42 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E43 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E44 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E45 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E46 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E47 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E48 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E49 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E4A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E4B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E4C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E4D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E4E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E4F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E50 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E51 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E52 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E53 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E54 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E55 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E56 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E57 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E58 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E59 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E5A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E5B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E5C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E5D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E5E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E5F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E60 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E61 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E62 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E63 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E64 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E65 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E66 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E67 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E68 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E69 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E6A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E6B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E6C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E6D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E6E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E6F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E70 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E71 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E72 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E73 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E74 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E75 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E76 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E77 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E78 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E79 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E7A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E7B # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E7C # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E7D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E7E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E7F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E80  # CJK RADICAL REPEAT
    LineBreakClass::ULB_ID, // 0x2E81  # CJK RADICAL CLIFF
    LineBreakClass::ULB_ID, // 0x2E82  # CJK RADICAL SECOND ONE
    LineBreakClass::ULB_ID, // 0x2E83  # CJK RADICAL SECOND TWO
    LineBreakClass::ULB_ID, // 0x2E84  # CJK RADICAL SECOND THREE
    LineBreakClass::ULB_ID, // 0x2E85  # CJK RADICAL PERSON
    LineBreakClass::ULB_ID, // 0x2E86  # CJK RADICAL BOX
    LineBreakClass::ULB_ID, // 0x2E87  # CJK RADICAL TABLE
    LineBreakClass::ULB_ID, // 0x2E88  # CJK RADICAL KNIFE ONE
    LineBreakClass::ULB_ID, // 0x2E89  # CJK RADICAL KNIFE TWO
    LineBreakClass::ULB_ID, // 0x2E8A  # CJK RADICAL DIVINATION
    LineBreakClass::ULB_ID, // 0x2E8B  # CJK RADICAL SEAL
    LineBreakClass::ULB_ID, // 0x2E8C  # CJK RADICAL SMALL ONE
    LineBreakClass::ULB_ID, // 0x2E8D  # CJK RADICAL SMALL TWO
    LineBreakClass::ULB_ID, // 0x2E8E  # CJK RADICAL LAME ONE
    LineBreakClass::ULB_ID, // 0x2E8F  # CJK RADICAL LAME TWO
    LineBreakClass::ULB_ID, // 0x2E90  # CJK RADICAL LAME THREE
    LineBreakClass::ULB_ID, // 0x2E91  # CJK RADICAL LAME FOUR
    LineBreakClass::ULB_ID, // 0x2E92  # CJK RADICAL SNAKE
    LineBreakClass::ULB_ID, // 0x2E93  # CJK RADICAL THREAD
    LineBreakClass::ULB_ID, // 0x2E94  # CJK RADICAL SNOUT ONE
    LineBreakClass::ULB_ID, // 0x2E95  # CJK RADICAL SNOUT TWO
    LineBreakClass::ULB_ID, // 0x2E96  # CJK RADICAL HEART ONE
    LineBreakClass::ULB_ID, // 0x2E97  # CJK RADICAL HEART TWO
    LineBreakClass::ULB_ID, // 0x2E98  # CJK RADICAL HAND
    LineBreakClass::ULB_ID, // 0x2E99  # CJK RADICAL RAP
    LineBreakClass::ULB_ID, // 0x2E9A # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2E9B  # CJK RADICAL CHOKE
    LineBreakClass::ULB_ID, // 0x2E9C  # CJK RADICAL SUN
    LineBreakClass::ULB_ID, // 0x2E9D  # CJK RADICAL MOON
    LineBreakClass::ULB_ID, // 0x2E9E  # CJK RADICAL DEATH
    LineBreakClass::ULB_ID, // 0x2E9F  # CJK RADICAL MOTHER
    LineBreakClass::ULB_ID, // 0x2EA0  # CJK RADICAL CIVILIAN
    LineBreakClass::ULB_ID, // 0x2EA1  # CJK RADICAL WATER ONE
    LineBreakClass::ULB_ID, // 0x2EA2  # CJK RADICAL WATER TWO
    LineBreakClass::ULB_ID, // 0x2EA3  # CJK RADICAL FIRE
    LineBreakClass::ULB_ID, // 0x2EA4  # CJK RADICAL PAW ONE
    LineBreakClass::ULB_ID, // 0x2EA5  # CJK RADICAL PAW TWO
    LineBreakClass::ULB_ID, // 0x2EA6  # CJK RADICAL SIMPLIFIED HALF TREE TRUNK
    LineBreakClass::ULB_ID, // 0x2EA7  # CJK RADICAL COW
    LineBreakClass::ULB_ID, // 0x2EA8  # CJK RADICAL DOG
    LineBreakClass::ULB_ID, // 0x2EA9  # CJK RADICAL JADE
    LineBreakClass::ULB_ID, // 0x2EAA  # CJK RADICAL BOLT OF CLOTH
    LineBreakClass::ULB_ID, // 0x2EAB  # CJK RADICAL EYE
    LineBreakClass::ULB_ID, // 0x2EAC  # CJK RADICAL SPIRIT ONE
    LineBreakClass::ULB_ID, // 0x2EAD  # CJK RADICAL SPIRIT TWO
    LineBreakClass::ULB_ID, // 0x2EAE  # CJK RADICAL BAMBOO
    LineBreakClass::ULB_ID, // 0x2EAF  # CJK RADICAL SILK
    LineBreakClass::ULB_ID, // 0x2EB0  # CJK RADICAL C-SIMPLIFIED SILK
    LineBreakClass::ULB_ID, // 0x2EB1  # CJK RADICAL NET ONE
    LineBreakClass::ULB_ID, // 0x2EB2  # CJK RADICAL NET TWO
    LineBreakClass::ULB_ID, // 0x2EB3  # CJK RADICAL NET THREE
    LineBreakClass::ULB_ID, // 0x2EB4  # CJK RADICAL NET FOUR
    LineBreakClass::ULB_ID, // 0x2EB5  # CJK RADICAL MESH
    LineBreakClass::ULB_ID, // 0x2EB6  # CJK RADICAL SHEEP
    LineBreakClass::ULB_ID, // 0x2EB7  # CJK RADICAL RAM
    LineBreakClass::ULB_ID, // 0x2EB8  # CJK RADICAL EWE
    LineBreakClass::ULB_ID, // 0x2EB9  # CJK RADICAL OLD
    LineBreakClass::ULB_ID, // 0x2EBA  # CJK RADICAL BRUSH ONE
    LineBreakClass::ULB_ID, // 0x2EBB  # CJK RADICAL BRUSH TWO
    LineBreakClass::ULB_ID, // 0x2EBC  # CJK RADICAL MEAT
    LineBreakClass::ULB_ID, // 0x2EBD  # CJK RADICAL MORTAR
    LineBreakClass::ULB_ID, // 0x2EBE  # CJK RADICAL GRASS ONE
    LineBreakClass::ULB_ID, // 0x2EBF  # CJK RADICAL GRASS TWO
    LineBreakClass::ULB_ID, // 0x2EC0  # CJK RADICAL GRASS THREE
    LineBreakClass::ULB_ID, // 0x2EC1  # CJK RADICAL TIGER
    LineBreakClass::ULB_ID, // 0x2EC2  # CJK RADICAL CLOTHES
    LineBreakClass::ULB_ID, // 0x2EC3  # CJK RADICAL WEST ONE
    LineBreakClass::ULB_ID, // 0x2EC4  # CJK RADICAL WEST TWO
    LineBreakClass::ULB_ID, // 0x2EC5  # CJK RADICAL C-SIMPLIFIED SEE
    LineBreakClass::ULB_ID, // 0x2EC6  # CJK RADICAL SIMPLIFIED HORN
    LineBreakClass::ULB_ID, // 0x2EC7  # CJK RADICAL HORN
    LineBreakClass::ULB_ID, // 0x2EC8  # CJK RADICAL C-SIMPLIFIED SPEECH
    LineBreakClass::ULB_ID, // 0x2EC9  # CJK RADICAL C-SIMPLIFIED SHELL
    LineBreakClass::ULB_ID, // 0x2ECA  # CJK RADICAL FOOT
    LineBreakClass::ULB_ID, // 0x2ECB  # CJK RADICAL C-SIMPLIFIED CART
    LineBreakClass::ULB_ID, // 0x2ECC  # CJK RADICAL SIMPLIFIED WALK
    LineBreakClass::ULB_ID, // 0x2ECD  # CJK RADICAL WALK ONE
    LineBreakClass::ULB_ID, // 0x2ECE  # CJK RADICAL WALK TWO
    LineBreakClass::ULB_ID, // 0x2ECF  # CJK RADICAL CITY
    LineBreakClass::ULB_ID, // 0x2ED0  # CJK RADICAL C-SIMPLIFIED GOLD
    LineBreakClass::ULB_ID, // 0x2ED1  # CJK RADICAL LONG ONE
    LineBreakClass::ULB_ID, // 0x2ED2  # CJK RADICAL LONG TWO
    LineBreakClass::ULB_ID, // 0x2ED3  # CJK RADICAL C-SIMPLIFIED LONG
    LineBreakClass::ULB_ID, // 0x2ED4  # CJK RADICAL C-SIMPLIFIED GATE
    LineBreakClass::ULB_ID, // 0x2ED5  # CJK RADICAL MOUND ONE
    LineBreakClass::ULB_ID, // 0x2ED6  # CJK RADICAL MOUND TWO
    LineBreakClass::ULB_ID, // 0x2ED7  # CJK RADICAL RAIN
    LineBreakClass::ULB_ID, // 0x2ED8  # CJK RADICAL BLUE
    LineBreakClass::ULB_ID, // 0x2ED9  # CJK RADICAL C-SIMPLIFIED TANNED LEATHER
    LineBreakClass::ULB_ID, // 0x2EDA  # CJK RADICAL C-SIMPLIFIED LEAF
    LineBreakClass::ULB_ID, // 0x2EDB  # CJK RADICAL C-SIMPLIFIED WIND
    LineBreakClass::ULB_ID, // 0x2EDC  # CJK RADICAL C-SIMPLIFIED FLY
    LineBreakClass::ULB_ID, // 0x2EDD  # CJK RADICAL EAT ONE
    LineBreakClass::ULB_ID, // 0x2EDE  # CJK RADICAL EAT TWO
    LineBreakClass::ULB_ID, // 0x2EDF  # CJK RADICAL EAT THREE
    LineBreakClass::ULB_ID, // 0x2EE0  # CJK RADICAL C-SIMPLIFIED EAT
    LineBreakClass::ULB_ID, // 0x2EE1  # CJK RADICAL HEAD
    LineBreakClass::ULB_ID, // 0x2EE2  # CJK RADICAL C-SIMPLIFIED HORSE
    LineBreakClass::ULB_ID, // 0x2EE3  # CJK RADICAL BONE
    LineBreakClass::ULB_ID, // 0x2EE4  # CJK RADICAL GHOST
    LineBreakClass::ULB_ID, // 0x2EE5  # CJK RADICAL C-SIMPLIFIED FISH
    LineBreakClass::ULB_ID, // 0x2EE6  # CJK RADICAL C-SIMPLIFIED BIRD
    LineBreakClass::ULB_ID, // 0x2EE7  # CJK RADICAL C-SIMPLIFIED SALT
    LineBreakClass::ULB_ID, // 0x2EE8  # CJK RADICAL SIMPLIFIED WHEAT
    LineBreakClass::ULB_ID, // 0x2EE9  # CJK RADICAL SIMPLIFIED YELLOW
    LineBreakClass::ULB_ID, // 0x2EEA  # CJK RADICAL C-SIMPLIFIED FROG
    LineBreakClass::ULB_ID, // 0x2EEB  # CJK RADICAL J-SIMPLIFIED EVEN
    LineBreakClass::ULB_ID, // 0x2EEC  # CJK RADICAL C-SIMPLIFIED EVEN
    LineBreakClass::ULB_ID, // 0x2EED  # CJK RADICAL J-SIMPLIFIED TOOTH
    LineBreakClass::ULB_ID, // 0x2EEE  # CJK RADICAL C-SIMPLIFIED TOOTH
    LineBreakClass::ULB_ID, // 0x2EEF  # CJK RADICAL J-SIMPLIFIED DRAGON
    LineBreakClass::ULB_ID, // 0x2EF0  # CJK RADICAL C-SIMPLIFIED DRAGON
    LineBreakClass::ULB_ID, // 0x2EF1  # CJK RADICAL TURTLE
    LineBreakClass::ULB_ID, // 0x2EF2  # CJK RADICAL J-SIMPLIFIED TURTLE
    LineBreakClass::ULB_ID, // 0x2EF3  # CJK RADICAL C-SIMPLIFIED TURTLE
    LineBreakClass::ULB_ID, // 0x2EF4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2EF5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2EF6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2EF7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2EF8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2EF9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2EFA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2EFB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2EFC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2EFD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2EFE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2EFF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2F00  # KANGXI RADICAL ONE
    LineBreakClass::ULB_ID, // 0x2F01  # KANGXI RADICAL LINE
    LineBreakClass::ULB_ID, // 0x2F02  # KANGXI RADICAL DOT
    LineBreakClass::ULB_ID, // 0x2F03  # KANGXI RADICAL SLASH
    LineBreakClass::ULB_ID, // 0x2F04  # KANGXI RADICAL SECOND
    LineBreakClass::ULB_ID, // 0x2F05  # KANGXI RADICAL HOOK
    LineBreakClass::ULB_ID, // 0x2F06  # KANGXI RADICAL TWO
    LineBreakClass::ULB_ID, // 0x2F07  # KANGXI RADICAL LID
    LineBreakClass::ULB_ID, // 0x2F08  # KANGXI RADICAL MAN
    LineBreakClass::ULB_ID, // 0x2F09  # KANGXI RADICAL LEGS
    LineBreakClass::ULB_ID, // 0x2F0A  # KANGXI RADICAL ENTER
    LineBreakClass::ULB_ID, // 0x2F0B  # KANGXI RADICAL EIGHT
    LineBreakClass::ULB_ID, // 0x2F0C  # KANGXI RADICAL DOWN BOX
    LineBreakClass::ULB_ID, // 0x2F0D  # KANGXI RADICAL COVER
    LineBreakClass::ULB_ID, // 0x2F0E  # KANGXI RADICAL ICE
    LineBreakClass::ULB_ID, // 0x2F0F  # KANGXI RADICAL TABLE
    LineBreakClass::ULB_ID, // 0x2F10  # KANGXI RADICAL OPEN BOX
    LineBreakClass::ULB_ID, // 0x2F11  # KANGXI RADICAL KNIFE
    LineBreakClass::ULB_ID, // 0x2F12  # KANGXI RADICAL POWER
    LineBreakClass::ULB_ID, // 0x2F13  # KANGXI RADICAL WRAP
    LineBreakClass::ULB_ID, // 0x2F14  # KANGXI RADICAL SPOON
    LineBreakClass::ULB_ID, // 0x2F15  # KANGXI RADICAL RIGHT OPEN BOX
    LineBreakClass::ULB_ID, // 0x2F16  # KANGXI RADICAL HIDING ENCLOSURE
    LineBreakClass::ULB_ID, // 0x2F17  # KANGXI RADICAL TEN
    LineBreakClass::ULB_ID, // 0x2F18  # KANGXI RADICAL DIVINATION
    LineBreakClass::ULB_ID, // 0x2F19  # KANGXI RADICAL SEAL
    LineBreakClass::ULB_ID, // 0x2F1A  # KANGXI RADICAL CLIFF
    LineBreakClass::ULB_ID, // 0x2F1B  # KANGXI RADICAL PRIVATE
    LineBreakClass::ULB_ID, // 0x2F1C  # KANGXI RADICAL AGAIN
    LineBreakClass::ULB_ID, // 0x2F1D  # KANGXI RADICAL MOUTH
    LineBreakClass::ULB_ID, // 0x2F1E  # KANGXI RADICAL ENCLOSURE
    LineBreakClass::ULB_ID, // 0x2F1F  # KANGXI RADICAL EARTH
    LineBreakClass::ULB_ID, // 0x2F20  # KANGXI RADICAL SCHOLAR
    LineBreakClass::ULB_ID, // 0x2F21  # KANGXI RADICAL GO
    LineBreakClass::ULB_ID, // 0x2F22  # KANGXI RADICAL GO SLOWLY
    LineBreakClass::ULB_ID, // 0x2F23  # KANGXI RADICAL EVENING
    LineBreakClass::ULB_ID, // 0x2F24  # KANGXI RADICAL BIG
    LineBreakClass::ULB_ID, // 0x2F25  # KANGXI RADICAL WOMAN
    LineBreakClass::ULB_ID, // 0x2F26  # KANGXI RADICAL CHILD
    LineBreakClass::ULB_ID, // 0x2F27  # KANGXI RADICAL ROOF
    LineBreakClass::ULB_ID, // 0x2F28  # KANGXI RADICAL INCH
    LineBreakClass::ULB_ID, // 0x2F29  # KANGXI RADICAL SMALL
    LineBreakClass::ULB_ID, // 0x2F2A  # KANGXI RADICAL LAME
    LineBreakClass::ULB_ID, // 0x2F2B  # KANGXI RADICAL CORPSE
    LineBreakClass::ULB_ID, // 0x2F2C  # KANGXI RADICAL SPROUT
    LineBreakClass::ULB_ID, // 0x2F2D  # KANGXI RADICAL MOUNTAIN
    LineBreakClass::ULB_ID, // 0x2F2E  # KANGXI RADICAL RIVER
    LineBreakClass::ULB_ID, // 0x2F2F  # KANGXI RADICAL WORK
    LineBreakClass::ULB_ID, // 0x2F30  # KANGXI RADICAL ONESELF
    LineBreakClass::ULB_ID, // 0x2F31  # KANGXI RADICAL TURBAN
    LineBreakClass::ULB_ID, // 0x2F32  # KANGXI RADICAL DRY
    LineBreakClass::ULB_ID, // 0x2F33  # KANGXI RADICAL SHORT THREAD
    LineBreakClass::ULB_ID, // 0x2F34  # KANGXI RADICAL DOTTED CLIFF
    LineBreakClass::ULB_ID, // 0x2F35  # KANGXI RADICAL LONG STRIDE
    LineBreakClass::ULB_ID, // 0x2F36  # KANGXI RADICAL TWO HANDS
    LineBreakClass::ULB_ID, // 0x2F37  # KANGXI RADICAL SHOOT
    LineBreakClass::ULB_ID, // 0x2F38  # KANGXI RADICAL BOW
    LineBreakClass::ULB_ID, // 0x2F39  # KANGXI RADICAL SNOUT
    LineBreakClass::ULB_ID, // 0x2F3A  # KANGXI RADICAL BRISTLE
    LineBreakClass::ULB_ID, // 0x2F3B  # KANGXI RADICAL STEP
    LineBreakClass::ULB_ID, // 0x2F3C  # KANGXI RADICAL HEART
    LineBreakClass::ULB_ID, // 0x2F3D  # KANGXI RADICAL HALBERD
    LineBreakClass::ULB_ID, // 0x2F3E  # KANGXI RADICAL DOOR
    LineBreakClass::ULB_ID, // 0x2F3F  # KANGXI RADICAL HAND
    LineBreakClass::ULB_ID, // 0x2F40  # KANGXI RADICAL BRANCH
    LineBreakClass::ULB_ID, // 0x2F41  # KANGXI RADICAL RAP
    LineBreakClass::ULB_ID, // 0x2F42  # KANGXI RADICAL SCRIPT
    LineBreakClass::ULB_ID, // 0x2F43  # KANGXI RADICAL DIPPER
    LineBreakClass::ULB_ID, // 0x2F44  # KANGXI RADICAL AXE
    LineBreakClass::ULB_ID, // 0x2F45  # KANGXI RADICAL SQUARE
    LineBreakClass::ULB_ID, // 0x2F46  # KANGXI RADICAL NOT
    LineBreakClass::ULB_ID, // 0x2F47  # KANGXI RADICAL SUN
    LineBreakClass::ULB_ID, // 0x2F48  # KANGXI RADICAL SAY
    LineBreakClass::ULB_ID, // 0x2F49  # KANGXI RADICAL MOON
    LineBreakClass::ULB_ID, // 0x2F4A  # KANGXI RADICAL TREE
    LineBreakClass::ULB_ID, // 0x2F4B  # KANGXI RADICAL LACK
    LineBreakClass::ULB_ID, // 0x2F4C  # KANGXI RADICAL STOP
    LineBreakClass::ULB_ID, // 0x2F4D  # KANGXI RADICAL DEATH
    LineBreakClass::ULB_ID, // 0x2F4E  # KANGXI RADICAL WEAPON
    LineBreakClass::ULB_ID, // 0x2F4F  # KANGXI RADICAL DO NOT
    LineBreakClass::ULB_ID, // 0x2F50  # KANGXI RADICAL COMPARE
    LineBreakClass::ULB_ID, // 0x2F51  # KANGXI RADICAL FUR
    LineBreakClass::ULB_ID, // 0x2F52  # KANGXI RADICAL CLAN
    LineBreakClass::ULB_ID, // 0x2F53  # KANGXI RADICAL STEAM
    LineBreakClass::ULB_ID, // 0x2F54  # KANGXI RADICAL WATER
    LineBreakClass::ULB_ID, // 0x2F55  # KANGXI RADICAL FIRE
    LineBreakClass::ULB_ID, // 0x2F56  # KANGXI RADICAL CLAW
    LineBreakClass::ULB_ID, // 0x2F57  # KANGXI RADICAL FATHER
    LineBreakClass::ULB_ID, // 0x2F58  # KANGXI RADICAL DOUBLE X
    LineBreakClass::ULB_ID, // 0x2F59  # KANGXI RADICAL HALF TREE TRUNK
    LineBreakClass::ULB_ID, // 0x2F5A  # KANGXI RADICAL SLICE
    LineBreakClass::ULB_ID, // 0x2F5B  # KANGXI RADICAL FANG
    LineBreakClass::ULB_ID, // 0x2F5C  # KANGXI RADICAL COW
    LineBreakClass::ULB_ID, // 0x2F5D  # KANGXI RADICAL DOG
    LineBreakClass::ULB_ID, // 0x2F5E  # KANGXI RADICAL PROFOUND
    LineBreakClass::ULB_ID, // 0x2F5F  # KANGXI RADICAL JADE
    LineBreakClass::ULB_ID, // 0x2F60  # KANGXI RADICAL MELON
    LineBreakClass::ULB_ID, // 0x2F61  # KANGXI RADICAL TILE
    LineBreakClass::ULB_ID, // 0x2F62  # KANGXI RADICAL SWEET
    LineBreakClass::ULB_ID, // 0x2F63  # KANGXI RADICAL LIFE
    LineBreakClass::ULB_ID, // 0x2F64  # KANGXI RADICAL USE
    LineBreakClass::ULB_ID, // 0x2F65  # KANGXI RADICAL FIELD
    LineBreakClass::ULB_ID, // 0x2F66  # KANGXI RADICAL BOLT OF CLOTH
    LineBreakClass::ULB_ID, // 0x2F67  # KANGXI RADICAL SICKNESS
    LineBreakClass::ULB_ID, // 0x2F68  # KANGXI RADICAL DOTTED TENT
    LineBreakClass::ULB_ID, // 0x2F69  # KANGXI RADICAL WHITE
    LineBreakClass::ULB_ID, // 0x2F6A  # KANGXI RADICAL SKIN
    LineBreakClass::ULB_ID, // 0x2F6B  # KANGXI RADICAL DISH
    LineBreakClass::ULB_ID, // 0x2F6C  # KANGXI RADICAL EYE
    LineBreakClass::ULB_ID, // 0x2F6D  # KANGXI RADICAL SPEAR
    LineBreakClass::ULB_ID, // 0x2F6E  # KANGXI RADICAL ARROW
    LineBreakClass::ULB_ID, // 0x2F6F  # KANGXI RADICAL STONE
    LineBreakClass::ULB_ID, // 0x2F70  # KANGXI RADICAL SPIRIT
    LineBreakClass::ULB_ID, // 0x2F71  # KANGXI RADICAL TRACK
    LineBreakClass::ULB_ID, // 0x2F72  # KANGXI RADICAL GRAIN
    LineBreakClass::ULB_ID, // 0x2F73  # KANGXI RADICAL CAVE
    LineBreakClass::ULB_ID, // 0x2F74  # KANGXI RADICAL STAND
    LineBreakClass::ULB_ID, // 0x2F75  # KANGXI RADICAL BAMBOO
    LineBreakClass::ULB_ID, // 0x2F76  # KANGXI RADICAL RICE
    LineBreakClass::ULB_ID, // 0x2F77  # KANGXI RADICAL SILK
    LineBreakClass::ULB_ID, // 0x2F78  # KANGXI RADICAL JAR
    LineBreakClass::ULB_ID, // 0x2F79  # KANGXI RADICAL NET
    LineBreakClass::ULB_ID, // 0x2F7A  # KANGXI RADICAL SHEEP
    LineBreakClass::ULB_ID, // 0x2F7B  # KANGXI RADICAL FEATHER
    LineBreakClass::ULB_ID, // 0x2F7C  # KANGXI RADICAL OLD
    LineBreakClass::ULB_ID, // 0x2F7D  # KANGXI RADICAL AND
    LineBreakClass::ULB_ID, // 0x2F7E  # KANGXI RADICAL PLOW
    LineBreakClass::ULB_ID, // 0x2F7F  # KANGXI RADICAL EAR
    LineBreakClass::ULB_ID, // 0x2F80  # KANGXI RADICAL BRUSH
    LineBreakClass::ULB_ID, // 0x2F81  # KANGXI RADICAL MEAT
    LineBreakClass::ULB_ID, // 0x2F82  # KANGXI RADICAL MINISTER
    LineBreakClass::ULB_ID, // 0x2F83  # KANGXI RADICAL SELF
    LineBreakClass::ULB_ID, // 0x2F84  # KANGXI RADICAL ARRIVE
    LineBreakClass::ULB_ID, // 0x2F85  # KANGXI RADICAL MORTAR
    LineBreakClass::ULB_ID, // 0x2F86  # KANGXI RADICAL TONGUE
    LineBreakClass::ULB_ID, // 0x2F87  # KANGXI RADICAL OPPOSE
    LineBreakClass::ULB_ID, // 0x2F88  # KANGXI RADICAL BOAT
    LineBreakClass::ULB_ID, // 0x2F89  # KANGXI RADICAL STOPPING
    LineBreakClass::ULB_ID, // 0x2F8A  # KANGXI RADICAL COLOR
    LineBreakClass::ULB_ID, // 0x2F8B  # KANGXI RADICAL GRASS
    LineBreakClass::ULB_ID, // 0x2F8C  # KANGXI RADICAL TIGER
    LineBreakClass::ULB_ID, // 0x2F8D  # KANGXI RADICAL INSECT
    LineBreakClass::ULB_ID, // 0x2F8E  # KANGXI RADICAL BLOOD
    LineBreakClass::ULB_ID, // 0x2F8F  # KANGXI RADICAL WALK ENCLOSURE
    LineBreakClass::ULB_ID, // 0x2F90  # KANGXI RADICAL CLOTHES
    LineBreakClass::ULB_ID, // 0x2F91  # KANGXI RADICAL WEST
    LineBreakClass::ULB_ID, // 0x2F92  # KANGXI RADICAL SEE
    LineBreakClass::ULB_ID, // 0x2F93  # KANGXI RADICAL HORN
    LineBreakClass::ULB_ID, // 0x2F94  # KANGXI RADICAL SPEECH
    LineBreakClass::ULB_ID, // 0x2F95  # KANGXI RADICAL VALLEY
    LineBreakClass::ULB_ID, // 0x2F96  # KANGXI RADICAL BEAN
    LineBreakClass::ULB_ID, // 0x2F97  # KANGXI RADICAL PIG
    LineBreakClass::ULB_ID, // 0x2F98  # KANGXI RADICAL BADGER
    LineBreakClass::ULB_ID, // 0x2F99  # KANGXI RADICAL SHELL
    LineBreakClass::ULB_ID, // 0x2F9A  # KANGXI RADICAL RED
    LineBreakClass::ULB_ID, // 0x2F9B  # KANGXI RADICAL RUN
    LineBreakClass::ULB_ID, // 0x2F9C  # KANGXI RADICAL FOOT
    LineBreakClass::ULB_ID, // 0x2F9D  # KANGXI RADICAL BODY
    LineBreakClass::ULB_ID, // 0x2F9E  # KANGXI RADICAL CART
    LineBreakClass::ULB_ID, // 0x2F9F  # KANGXI RADICAL BITTER
    LineBreakClass::ULB_ID, // 0x2FA0  # KANGXI RADICAL MORNING
    LineBreakClass::ULB_ID, // 0x2FA1  # KANGXI RADICAL WALK
    LineBreakClass::ULB_ID, // 0x2FA2  # KANGXI RADICAL CITY
    LineBreakClass::ULB_ID, // 0x2FA3  # KANGXI RADICAL WINE
    LineBreakClass::ULB_ID, // 0x2FA4  # KANGXI RADICAL DISTINGUISH
    LineBreakClass::ULB_ID, // 0x2FA5  # KANGXI RADICAL VILLAGE
    LineBreakClass::ULB_ID, // 0x2FA6  # KANGXI RADICAL GOLD
    LineBreakClass::ULB_ID, // 0x2FA7  # KANGXI RADICAL LONG
    LineBreakClass::ULB_ID, // 0x2FA8  # KANGXI RADICAL GATE
    LineBreakClass::ULB_ID, // 0x2FA9  # KANGXI RADICAL MOUND
    LineBreakClass::ULB_ID, // 0x2FAA  # KANGXI RADICAL SLAVE
    LineBreakClass::ULB_ID, // 0x2FAB  # KANGXI RADICAL SHORT TAILED BIRD
    LineBreakClass::ULB_ID, // 0x2FAC  # KANGXI RADICAL RAIN
    LineBreakClass::ULB_ID, // 0x2FAD  # KANGXI RADICAL BLUE
    LineBreakClass::ULB_ID, // 0x2FAE  # KANGXI RADICAL WRONG
    LineBreakClass::ULB_ID, // 0x2FAF  # KANGXI RADICAL FACE
    LineBreakClass::ULB_ID, // 0x2FB0  # KANGXI RADICAL LEATHER
    LineBreakClass::ULB_ID, // 0x2FB1  # KANGXI RADICAL TANNED LEATHER
    LineBreakClass::ULB_ID, // 0x2FB2  # KANGXI RADICAL LEEK
    LineBreakClass::ULB_ID, // 0x2FB3  # KANGXI RADICAL SOUND
    LineBreakClass::ULB_ID, // 0x2FB4  # KANGXI RADICAL LEAF
    LineBreakClass::ULB_ID, // 0x2FB5  # KANGXI RADICAL WIND
    LineBreakClass::ULB_ID, // 0x2FB6  # KANGXI RADICAL FLY
    LineBreakClass::ULB_ID, // 0x2FB7  # KANGXI RADICAL EAT
    LineBreakClass::ULB_ID, // 0x2FB8  # KANGXI RADICAL HEAD
    LineBreakClass::ULB_ID, // 0x2FB9  # KANGXI RADICAL FRAGRANT
    LineBreakClass::ULB_ID, // 0x2FBA  # KANGXI RADICAL HORSE
    LineBreakClass::ULB_ID, // 0x2FBB  # KANGXI RADICAL BONE
    LineBreakClass::ULB_ID, // 0x2FBC  # KANGXI RADICAL TALL
    LineBreakClass::ULB_ID, // 0x2FBD  # KANGXI RADICAL HAIR
    LineBreakClass::ULB_ID, // 0x2FBE  # KANGXI RADICAL FIGHT
    LineBreakClass::ULB_ID, // 0x2FBF  # KANGXI RADICAL SACRIFICIAL WINE
    LineBreakClass::ULB_ID, // 0x2FC0  # KANGXI RADICAL CAULDRON
    LineBreakClass::ULB_ID, // 0x2FC1  # KANGXI RADICAL GHOST
    LineBreakClass::ULB_ID, // 0x2FC2  # KANGXI RADICAL FISH
    LineBreakClass::ULB_ID, // 0x2FC3  # KANGXI RADICAL BIRD
    LineBreakClass::ULB_ID, // 0x2FC4  # KANGXI RADICAL SALT
    LineBreakClass::ULB_ID, // 0x2FC5  # KANGXI RADICAL DEER
    LineBreakClass::ULB_ID, // 0x2FC6  # KANGXI RADICAL WHEAT
    LineBreakClass::ULB_ID, // 0x2FC7  # KANGXI RADICAL HEMP
    LineBreakClass::ULB_ID, // 0x2FC8  # KANGXI RADICAL YELLOW
    LineBreakClass::ULB_ID, // 0x2FC9  # KANGXI RADICAL MILLET
    LineBreakClass::ULB_ID, // 0x2FCA  # KANGXI RADICAL BLACK
    LineBreakClass::ULB_ID, // 0x2FCB  # KANGXI RADICAL EMBROIDERY
    LineBreakClass::ULB_ID, // 0x2FCC  # KANGXI RADICAL FROG
    LineBreakClass::ULB_ID, // 0x2FCD  # KANGXI RADICAL TRIPOD
    LineBreakClass::ULB_ID, // 0x2FCE  # KANGXI RADICAL DRUM
    LineBreakClass::ULB_ID, // 0x2FCF  # KANGXI RADICAL RAT
    LineBreakClass::ULB_ID, // 0x2FD0  # KANGXI RADICAL NOSE
    LineBreakClass::ULB_ID, // 0x2FD1  # KANGXI RADICAL EVEN
    LineBreakClass::ULB_ID, // 0x2FD2  # KANGXI RADICAL TOOTH
    LineBreakClass::ULB_ID, // 0x2FD3  # KANGXI RADICAL DRAGON
    LineBreakClass::ULB_ID, // 0x2FD4  # KANGXI RADICAL TURTLE
    LineBreakClass::ULB_ID, // 0x2FD5  # KANGXI RADICAL FLUTE
    LineBreakClass::ULB_ID, // 0x2FD6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FD7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FD8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FD9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FDA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FDB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FDC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FDD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FDE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FDF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FE0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FE1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FE2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FE3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FE4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FE5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FE6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FE7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FE8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FE9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FEA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FEB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FEC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FED # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FEE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FEF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FF0  # IDEOGRAPHIC DESCRIPTION CHARACTER LEFT TO RIGHT
    LineBreakClass::ULB_ID, // 0x2FF1  # IDEOGRAPHIC DESCRIPTION CHARACTER ABOVE TO BELOW
    LineBreakClass::ULB_ID, // 0x2FF2  # IDEOGRAPHIC DESCRIPTION CHARACTER LEFT TO MIDDLE AND RIGHT
    LineBreakClass::ULB_ID, // 0x2FF3  # IDEOGRAPHIC DESCRIPTION CHARACTER ABOVE TO MIDDLE AND
                            // BELOW
    LineBreakClass::ULB_ID, // 0x2FF4  # IDEOGRAPHIC DESCRIPTION CHARACTER FULL SURROUND
    LineBreakClass::ULB_ID, // 0x2FF5  # IDEOGRAPHIC DESCRIPTION CHARACTER SURROUND FROM ABOVE
    LineBreakClass::ULB_ID, // 0x2FF6  # IDEOGRAPHIC DESCRIPTION CHARACTER SURROUND FROM BELOW
    LineBreakClass::ULB_ID, // 0x2FF7  # IDEOGRAPHIC DESCRIPTION CHARACTER SURROUND FROM LEFT
    LineBreakClass::ULB_ID, // 0x2FF8  # IDEOGRAPHIC DESCRIPTION CHARACTER SURROUND FROM UPPER LEFT
    LineBreakClass::ULB_ID, // 0x2FF9  # IDEOGRAPHIC DESCRIPTION CHARACTER SURROUND FROM UPPER
                            // RIGHT
    LineBreakClass::ULB_ID, // 0x2FFA  # IDEOGRAPHIC DESCRIPTION CHARACTER SURROUND FROM LOWER LEFT
    LineBreakClass::ULB_ID, // 0x2FFB  # IDEOGRAPHIC DESCRIPTION CHARACTER OVERLAID
    LineBreakClass::ULB_ID, // 0x2FFC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FFD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FFE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x2FFF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x3000  # IDEOGRAPHIC SPACE
    LineBreakClass::ULB_CL, // 0x3001  # IDEOGRAPHIC COMMA
    LineBreakClass::ULB_CL, // 0x3002  # IDEOGRAPHIC FULL STOP
    LineBreakClass::ULB_ID, // 0x3003  # DITTO MARK
    LineBreakClass::ULB_ID, // 0x3004  # JAPANESE INDUSTRIAL STANDARD SYMBOL
    LineBreakClass::ULB_NS, // 0x3005  # IDEOGRAPHIC ITERATION MARK
    LineBreakClass::ULB_ID, // 0x3006  # IDEOGRAPHIC CLOSING MARK
    LineBreakClass::ULB_ID, // 0x3007  # IDEOGRAPHIC NUMBER ZERO
    LineBreakClass::ULB_OP, // 0x3008  # LEFT ANGLE BRACKET
    LineBreakClass::ULB_CL, // 0x3009  # RIGHT ANGLE BRACKET
    LineBreakClass::ULB_OP, // 0x300A  # LEFT DOUBLE ANGLE BRACKET
    LineBreakClass::ULB_CL, // 0x300B  # RIGHT DOUBLE ANGLE BRACKET
    LineBreakClass::ULB_OP, // 0x300C  # LEFT CORNER BRACKET
    LineBreakClass::ULB_CL, // 0x300D  # RIGHT CORNER BRACKET
    LineBreakClass::ULB_OP, // 0x300E  # LEFT WHITE CORNER BRACKET
    LineBreakClass::ULB_CL, // 0x300F  # RIGHT WHITE CORNER BRACKET
    LineBreakClass::ULB_OP, // 0x3010  # LEFT BLACK LENTICULAR BRACKET
    LineBreakClass::ULB_CL, // 0x3011  # RIGHT BLACK LENTICULAR BRACKET
    LineBreakClass::ULB_ID, // 0x3012  # POSTAL MARK
    LineBreakClass::ULB_ID, // 0x3013  # GETA MARK
    LineBreakClass::ULB_OP, // 0x3014  # LEFT TORTOISE SHELL BRACKET
    LineBreakClass::ULB_CL, // 0x3015  # RIGHT TORTOISE SHELL BRACKET
    LineBreakClass::ULB_OP, // 0x3016  # LEFT WHITE LENTICULAR BRACKET
    LineBreakClass::ULB_CL, // 0x3017  # RIGHT WHITE LENTICULAR BRACKET
    LineBreakClass::ULB_OP, // 0x3018  # LEFT WHITE TORTOISE SHELL BRACKET
    LineBreakClass::ULB_CL, // 0x3019  # RIGHT WHITE TORTOISE SHELL BRACKET
    LineBreakClass::ULB_OP, // 0x301A  # LEFT WHITE SQUARE BRACKET
    LineBreakClass::ULB_CL, // 0x301B  # RIGHT WHITE SQUARE BRACKET
    LineBreakClass::ULB_NS, // 0x301C  # WAVE DASH
    LineBreakClass::ULB_OP, // 0x301D  # REVERSED DOUBLE PRIME QUOTATION MARK
    LineBreakClass::ULB_CL, // 0x301E  # DOUBLE PRIME QUOTATION MARK
    LineBreakClass::ULB_CL, // 0x301F  # LOW DOUBLE PRIME QUOTATION MARK
    LineBreakClass::ULB_ID, // 0x3020  # POSTAL MARK FACE
    LineBreakClass::ULB_ID, // 0x3021  # HANGZHOU NUMERAL ONE
    LineBreakClass::ULB_ID, // 0x3022  # HANGZHOU NUMERAL TWO
    LineBreakClass::ULB_ID, // 0x3023  # HANGZHOU NUMERAL THREE
    LineBreakClass::ULB_ID, // 0x3024  # HANGZHOU NUMERAL FOUR
    LineBreakClass::ULB_ID, // 0x3025  # HANGZHOU NUMERAL FIVE
    LineBreakClass::ULB_ID, // 0x3026  # HANGZHOU NUMERAL SIX
    LineBreakClass::ULB_ID, // 0x3027  # HANGZHOU NUMERAL SEVEN
    LineBreakClass::ULB_ID, // 0x3028  # HANGZHOU NUMERAL EIGHT
    LineBreakClass::ULB_ID, // 0x3029  # HANGZHOU NUMERAL NINE
    LineBreakClass::ULB_CM, // 0x302A  # IDEOGRAPHIC LEVEL TONE MARK
    LineBreakClass::ULB_CM, // 0x302B  # IDEOGRAPHIC RISING TONE MARK
    LineBreakClass::ULB_CM, // 0x302C  # IDEOGRAPHIC DEPARTING TONE MARK
    LineBreakClass::ULB_CM, // 0x302D  # IDEOGRAPHIC ENTERING TONE MARK
    LineBreakClass::ULB_CM, // 0x302E  # HANGUL SINGLE DOT TONE MARK
    LineBreakClass::ULB_CM, // 0x302F  # HANGUL DOUBLE DOT TONE MARK
    LineBreakClass::ULB_ID, // 0x3030  # WAVY DASH
    LineBreakClass::ULB_ID, // 0x3031  # VERTICAL KANA REPEAT MARK
    LineBreakClass::ULB_ID, // 0x3032  # VERTICAL KANA REPEAT WITH VOICED SOUND MARK
    LineBreakClass::ULB_ID, // 0x3033  # VERTICAL KANA REPEAT MARK UPPER HALF
    LineBreakClass::ULB_ID, // 0x3034  # VERTICAL KANA REPEAT WITH VOICED SOUND MARK UPPER HALF
    LineBreakClass::ULB_ID, // 0x3035  # VERTICAL KANA REPEAT MARK LOWER HALF
    LineBreakClass::ULB_ID, // 0x3036  # CIRCLED POSTAL MARK
    LineBreakClass::ULB_ID, // 0x3037  # IDEOGRAPHIC TELEGRAPH LINE FEED SEPARATOR SYMBOL
    LineBreakClass::ULB_ID, // 0x3038  # HANGZHOU NUMERAL TEN
    LineBreakClass::ULB_ID, // 0x3039  # HANGZHOU NUMERAL TWENTY
    LineBreakClass::ULB_ID, // 0x303A  # HANGZHOU NUMERAL THIRTY
    LineBreakClass::ULB_NS, // 0x303B  # VERTICAL IDEOGRAPHIC ITERATION MARK
    LineBreakClass::ULB_NS, // 0x303C  # MASU MARK
    LineBreakClass::ULB_ID, // 0x303D  # PART ALTERNATION MARK
    LineBreakClass::ULB_ID, // 0x303E  # IDEOGRAPHIC VARIATION INDICATOR
    LineBreakClass::ULB_ID, // 0x303F  # IDEOGRAPHIC HALF FILL SPACE
    LineBreakClass::ULB_ID, // 0x3040 # <UNDEFINED>
    LineBreakClass::ULB_NS, // 0x3041  # HIRAGANA LETTER SMALL A
    LineBreakClass::ULB_ID, // 0x3042  # HIRAGANA LETTER A
    LineBreakClass::ULB_NS, // 0x3043  # HIRAGANA LETTER SMALL I
    LineBreakClass::ULB_ID, // 0x3044  # HIRAGANA LETTER I
    LineBreakClass::ULB_NS, // 0x3045  # HIRAGANA LETTER SMALL U
    LineBreakClass::ULB_ID, // 0x3046  # HIRAGANA LETTER U
    LineBreakClass::ULB_NS, // 0x3047  # HIRAGANA LETTER SMALL E
    LineBreakClass::ULB_ID, // 0x3048  # HIRAGANA LETTER E
    LineBreakClass::ULB_NS, // 0x3049  # HIRAGANA LETTER SMALL O
    LineBreakClass::ULB_ID, // 0x304A  # HIRAGANA LETTER O
    LineBreakClass::ULB_ID, // 0x304B  # HIRAGANA LETTER KA
    LineBreakClass::ULB_ID, // 0x304C  # HIRAGANA LETTER GA
    LineBreakClass::ULB_ID, // 0x304D  # HIRAGANA LETTER KI
    LineBreakClass::ULB_ID, // 0x304E  # HIRAGANA LETTER GI
    LineBreakClass::ULB_ID, // 0x304F  # HIRAGANA LETTER KU
    LineBreakClass::ULB_ID, // 0x3050  # HIRAGANA LETTER GU
    LineBreakClass::ULB_ID, // 0x3051  # HIRAGANA LETTER KE
    LineBreakClass::ULB_ID, // 0x3052  # HIRAGANA LETTER GE
    LineBreakClass::ULB_ID, // 0x3053  # HIRAGANA LETTER KO
    LineBreakClass::ULB_ID, // 0x3054  # HIRAGANA LETTER GO
    LineBreakClass::ULB_ID, // 0x3055  # HIRAGANA LETTER SA
    LineBreakClass::ULB_ID, // 0x3056  # HIRAGANA LETTER ZA
    LineBreakClass::ULB_ID, // 0x3057  # HIRAGANA LETTER SI
    LineBreakClass::ULB_ID, // 0x3058  # HIRAGANA LETTER ZI
    LineBreakClass::ULB_ID, // 0x3059  # HIRAGANA LETTER SU
    LineBreakClass::ULB_ID, // 0x305A  # HIRAGANA LETTER ZU
    LineBreakClass::ULB_ID, // 0x305B  # HIRAGANA LETTER SE
    LineBreakClass::ULB_ID, // 0x305C  # HIRAGANA LETTER ZE
    LineBreakClass::ULB_ID, // 0x305D  # HIRAGANA LETTER SO
    LineBreakClass::ULB_ID, // 0x305E  # HIRAGANA LETTER ZO
    LineBreakClass::ULB_ID, // 0x305F  # HIRAGANA LETTER TA
    LineBreakClass::ULB_ID, // 0x3060  # HIRAGANA LETTER DA
    LineBreakClass::ULB_ID, // 0x3061  # HIRAGANA LETTER TI
    LineBreakClass::ULB_ID, // 0x3062  # HIRAGANA LETTER DI
    LineBreakClass::ULB_NS, // 0x3063  # HIRAGANA LETTER SMALL TU
    LineBreakClass::ULB_ID, // 0x3064  # HIRAGANA LETTER TU
    LineBreakClass::ULB_ID, // 0x3065  # HIRAGANA LETTER DU
    LineBreakClass::ULB_ID, // 0x3066  # HIRAGANA LETTER TE
    LineBreakClass::ULB_ID, // 0x3067  # HIRAGANA LETTER DE
    LineBreakClass::ULB_ID, // 0x3068  # HIRAGANA LETTER TO
    LineBreakClass::ULB_ID, // 0x3069  # HIRAGANA LETTER DO
    LineBreakClass::ULB_ID, // 0x306A  # HIRAGANA LETTER NA
    LineBreakClass::ULB_ID, // 0x306B  # HIRAGANA LETTER NI
    LineBreakClass::ULB_ID, // 0x306C  # HIRAGANA LETTER NU
    LineBreakClass::ULB_ID, // 0x306D  # HIRAGANA LETTER NE
    LineBreakClass::ULB_ID, // 0x306E  # HIRAGANA LETTER NO
    LineBreakClass::ULB_ID, // 0x306F  # HIRAGANA LETTER HA
    LineBreakClass::ULB_ID, // 0x3070  # HIRAGANA LETTER BA
    LineBreakClass::ULB_ID, // 0x3071  # HIRAGANA LETTER PA
    LineBreakClass::ULB_ID, // 0x3072  # HIRAGANA LETTER HI
    LineBreakClass::ULB_ID, // 0x3073  # HIRAGANA LETTER BI
    LineBreakClass::ULB_ID, // 0x3074  # HIRAGANA LETTER PI
    LineBreakClass::ULB_ID, // 0x3075  # HIRAGANA LETTER HU
    LineBreakClass::ULB_ID, // 0x3076  # HIRAGANA LETTER BU
    LineBreakClass::ULB_ID, // 0x3077  # HIRAGANA LETTER PU
    LineBreakClass::ULB_ID, // 0x3078  # HIRAGANA LETTER HE
    LineBreakClass::ULB_ID, // 0x3079  # HIRAGANA LETTER BE
    LineBreakClass::ULB_ID, // 0x307A  # HIRAGANA LETTER PE
    LineBreakClass::ULB_ID, // 0x307B  # HIRAGANA LETTER HO
    LineBreakClass::ULB_ID, // 0x307C  # HIRAGANA LETTER BO
    LineBreakClass::ULB_ID, // 0x307D  # HIRAGANA LETTER PO
    LineBreakClass::ULB_ID, // 0x307E  # HIRAGANA LETTER MA
    LineBreakClass::ULB_ID, // 0x307F  # HIRAGANA LETTER MI
    LineBreakClass::ULB_ID, // 0x3080  # HIRAGANA LETTER MU
    LineBreakClass::ULB_ID, // 0x3081  # HIRAGANA LETTER ME
    LineBreakClass::ULB_ID, // 0x3082  # HIRAGANA LETTER MO
    LineBreakClass::ULB_NS, // 0x3083  # HIRAGANA LETTER SMALL YA
    LineBreakClass::ULB_ID, // 0x3084  # HIRAGANA LETTER YA
    LineBreakClass::ULB_NS, // 0x3085  # HIRAGANA LETTER SMALL YU
    LineBreakClass::ULB_ID, // 0x3086  # HIRAGANA LETTER YU
    LineBreakClass::ULB_NS, // 0x3087  # HIRAGANA LETTER SMALL YO
    LineBreakClass::ULB_ID, // 0x3088  # HIRAGANA LETTER YO
    LineBreakClass::ULB_ID, // 0x3089  # HIRAGANA LETTER RA
    LineBreakClass::ULB_ID, // 0x308A  # HIRAGANA LETTER RI
    LineBreakClass::ULB_ID, // 0x308B  # HIRAGANA LETTER RU
    LineBreakClass::ULB_ID, // 0x308C  # HIRAGANA LETTER RE
    LineBreakClass::ULB_ID, // 0x308D  # HIRAGANA LETTER RO
    LineBreakClass::ULB_NS, // 0x308E  # HIRAGANA LETTER SMALL WA
    LineBreakClass::ULB_ID, // 0x308F  # HIRAGANA LETTER WA
    LineBreakClass::ULB_ID, // 0x3090  # HIRAGANA LETTER WI
    LineBreakClass::ULB_ID, // 0x3091  # HIRAGANA LETTER WE
    LineBreakClass::ULB_ID, // 0x3092  # HIRAGANA LETTER WO
    LineBreakClass::ULB_ID, // 0x3093  # HIRAGANA LETTER N
    LineBreakClass::ULB_ID, // 0x3094  # HIRAGANA LETTER VU
    LineBreakClass::ULB_NS, // 0x3095  # HIRAGANA LETTER SMALL KA
    LineBreakClass::ULB_NS, // 0x3096  # HIRAGANA LETTER SMALL KE
    LineBreakClass::ULB_ID, // 0x3097 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x3098 # <UNDEFINED>
    LineBreakClass::ULB_CM, // 0x3099  # COMBINING KATAKANA-HIRAGANA VOICED SOUND MARK
    LineBreakClass::ULB_CM, // 0x309A  # COMBINING KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK
    LineBreakClass::ULB_NS, // 0x309B  # KATAKANA-HIRAGANA VOICED SOUND MARK
    LineBreakClass::ULB_NS, // 0x309C  # KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK
    LineBreakClass::ULB_NS, // 0x309D  # HIRAGANA ITERATION MARK
    LineBreakClass::ULB_NS, // 0x309E  # HIRAGANA VOICED ITERATION MARK
    LineBreakClass::ULB_ID, // 0x309F  # HIRAGANA DIGRAPH YORI
    LineBreakClass::ULB_NS, // 0x30A0  # KATAKANA-HIRAGANA DOUBLE HYPHEN
    LineBreakClass::ULB_NS, // 0x30A1  # KATAKANA LETTER SMALL A
    LineBreakClass::ULB_ID, // 0x30A2  # KATAKANA LETTER A
    LineBreakClass::ULB_NS, // 0x30A3  # KATAKANA LETTER SMALL I
    LineBreakClass::ULB_ID, // 0x30A4  # KATAKANA LETTER I
    LineBreakClass::ULB_NS, // 0x30A5  # KATAKANA LETTER SMALL U
    LineBreakClass::ULB_ID, // 0x30A6  # KATAKANA LETTER U
    LineBreakClass::ULB_NS, // 0x30A7  # KATAKANA LETTER SMALL E
    LineBreakClass::ULB_ID, // 0x30A8  # KATAKANA LETTER E
    LineBreakClass::ULB_NS, // 0x30A9  # KATAKANA LETTER SMALL O
    LineBreakClass::ULB_ID, // 0x30AA  # KATAKANA LETTER O
    LineBreakClass::ULB_ID, // 0x30AB  # KATAKANA LETTER KA
    LineBreakClass::ULB_ID, // 0x30AC  # KATAKANA LETTER GA
    LineBreakClass::ULB_ID, // 0x30AD  # KATAKANA LETTER KI
    LineBreakClass::ULB_ID, // 0x30AE  # KATAKANA LETTER GI
    LineBreakClass::ULB_ID, // 0x30AF  # KATAKANA LETTER KU
    LineBreakClass::ULB_ID, // 0x30B0  # KATAKANA LETTER GU
    LineBreakClass::ULB_ID, // 0x30B1  # KATAKANA LETTER KE
    LineBreakClass::ULB_ID, // 0x30B2  # KATAKANA LETTER GE
    LineBreakClass::ULB_ID, // 0x30B3  # KATAKANA LETTER KO
    LineBreakClass::ULB_ID, // 0x30B4  # KATAKANA LETTER GO
    LineBreakClass::ULB_ID, // 0x30B5  # KATAKANA LETTER SA
    LineBreakClass::ULB_ID, // 0x30B6  # KATAKANA LETTER ZA
    LineBreakClass::ULB_ID, // 0x30B7  # KATAKANA LETTER SI
    LineBreakClass::ULB_ID, // 0x30B8  # KATAKANA LETTER ZI
    LineBreakClass::ULB_ID, // 0x30B9  # KATAKANA LETTER SU
    LineBreakClass::ULB_ID, // 0x30BA  # KATAKANA LETTER ZU
    LineBreakClass::ULB_ID, // 0x30BB  # KATAKANA LETTER SE
    LineBreakClass::ULB_ID, // 0x30BC  # KATAKANA LETTER ZE
    LineBreakClass::ULB_ID, // 0x30BD  # KATAKANA LETTER SO
    LineBreakClass::ULB_ID, // 0x30BE  # KATAKANA LETTER ZO
    LineBreakClass::ULB_ID, // 0x30BF  # KATAKANA LETTER TA
    LineBreakClass::ULB_ID, // 0x30C0  # KATAKANA LETTER DA
    LineBreakClass::ULB_ID, // 0x30C1  # KATAKANA LETTER TI
    LineBreakClass::ULB_ID, // 0x30C2  # KATAKANA LETTER DI
    LineBreakClass::ULB_NS, // 0x30C3  # KATAKANA LETTER SMALL TU
    LineBreakClass::ULB_ID, // 0x30C4  # KATAKANA LETTER TU
    LineBreakClass::ULB_ID, // 0x30C5  # KATAKANA LETTER DU
    LineBreakClass::ULB_ID, // 0x30C6  # KATAKANA LETTER TE
    LineBreakClass::ULB_ID, // 0x30C7  # KATAKANA LETTER DE
    LineBreakClass::ULB_ID, // 0x30C8  # KATAKANA LETTER TO
    LineBreakClass::ULB_ID, // 0x30C9  # KATAKANA LETTER DO
    LineBreakClass::ULB_ID, // 0x30CA  # KATAKANA LETTER NA
    LineBreakClass::ULB_ID, // 0x30CB  # KATAKANA LETTER NI
    LineBreakClass::ULB_ID, // 0x30CC  # KATAKANA LETTER NU
    LineBreakClass::ULB_ID, // 0x30CD  # KATAKANA LETTER NE
    LineBreakClass::ULB_ID, // 0x30CE  # KATAKANA LETTER NO
    LineBreakClass::ULB_ID, // 0x30CF  # KATAKANA LETTER HA
    LineBreakClass::ULB_ID, // 0x30D0  # KATAKANA LETTER BA
    LineBreakClass::ULB_ID, // 0x30D1  # KATAKANA LETTER PA
    LineBreakClass::ULB_ID, // 0x30D2  # KATAKANA LETTER HI
    LineBreakClass::ULB_ID, // 0x30D3  # KATAKANA LETTER BI
    LineBreakClass::ULB_ID, // 0x30D4  # KATAKANA LETTER PI
    LineBreakClass::ULB_ID, // 0x30D5  # KATAKANA LETTER HU
    LineBreakClass::ULB_ID, // 0x30D6  # KATAKANA LETTER BU
    LineBreakClass::ULB_ID, // 0x30D7  # KATAKANA LETTER PU
    LineBreakClass::ULB_ID, // 0x30D8  # KATAKANA LETTER HE
    LineBreakClass::ULB_ID, // 0x30D9  # KATAKANA LETTER BE
    LineBreakClass::ULB_ID, // 0x30DA  # KATAKANA LETTER PE
    LineBreakClass::ULB_ID, // 0x30DB  # KATAKANA LETTER HO
    LineBreakClass::ULB_ID, // 0x30DC  # KATAKANA LETTER BO
    LineBreakClass::ULB_ID, // 0x30DD  # KATAKANA LETTER PO
    LineBreakClass::ULB_ID, // 0x30DE  # KATAKANA LETTER MA
    LineBreakClass::ULB_ID, // 0x30DF  # KATAKANA LETTER MI
    LineBreakClass::ULB_ID, // 0x30E0  # KATAKANA LETTER MU
    LineBreakClass::ULB_ID, // 0x30E1  # KATAKANA LETTER ME
    LineBreakClass::ULB_ID, // 0x30E2  # KATAKANA LETTER MO
    LineBreakClass::ULB_NS, // 0x30E3  # KATAKANA LETTER SMALL YA
    LineBreakClass::ULB_ID, // 0x30E4  # KATAKANA LETTER YA
    LineBreakClass::ULB_NS, // 0x30E5  # KATAKANA LETTER SMALL YU
    LineBreakClass::ULB_ID, // 0x30E6  # KATAKANA LETTER YU
    LineBreakClass::ULB_NS, // 0x30E7  # KATAKANA LETTER SMALL YO
    LineBreakClass::ULB_ID, // 0x30E8  # KATAKANA LETTER YO
    LineBreakClass::ULB_ID, // 0x30E9  # KATAKANA LETTER RA
    LineBreakClass::ULB_ID, // 0x30EA  # KATAKANA LETTER RI
    LineBreakClass::ULB_ID, // 0x30EB  # KATAKANA LETTER RU
    LineBreakClass::ULB_ID, // 0x30EC  # KATAKANA LETTER RE
    LineBreakClass::ULB_ID, // 0x30ED  # KATAKANA LETTER RO
    LineBreakClass::ULB_NS, // 0x30EE  # KATAKANA LETTER SMALL WA
    LineBreakClass::ULB_ID, // 0x30EF  # KATAKANA LETTER WA
    LineBreakClass::ULB_ID, // 0x30F0  # KATAKANA LETTER WI
    LineBreakClass::ULB_ID, // 0x30F1  # KATAKANA LETTER WE
    LineBreakClass::ULB_ID, // 0x30F2  # KATAKANA LETTER WO
    LineBreakClass::ULB_ID, // 0x30F3  # KATAKANA LETTER N
    LineBreakClass::ULB_ID, // 0x30F4  # KATAKANA LETTER VU
    LineBreakClass::ULB_NS, // 0x30F5  # KATAKANA LETTER SMALL KA
    LineBreakClass::ULB_NS, // 0x30F6  # KATAKANA LETTER SMALL KE
    LineBreakClass::ULB_ID, // 0x30F7  # KATAKANA LETTER VA
    LineBreakClass::ULB_ID, // 0x30F8  # KATAKANA LETTER VI
    LineBreakClass::ULB_ID, // 0x30F9  # KATAKANA LETTER VE
    LineBreakClass::ULB_ID, // 0x30FA  # KATAKANA LETTER VO
    LineBreakClass::ULB_NS, // 0x30FB  # KATAKANA MIDDLE DOT
    LineBreakClass::ULB_NS, // 0x30FC  # KATAKANA-HIRAGANA PROLONGED SOUND MARK
    LineBreakClass::ULB_NS, // 0x30FD  # KATAKANA ITERATION MARK
    LineBreakClass::ULB_NS, // 0x30FE  # KATAKANA VOICED ITERATION MARK
    LineBreakClass::ULB_ID, // 0x30FF  # KATAKANA DIGRAPH KOTO
    LineBreakClass::ULB_ID, // 0x3100 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x3101 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x3102 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x3103 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x3104 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x3105  # BOPOMOFO LETTER B
    LineBreakClass::ULB_ID, // 0x3106  # BOPOMOFO LETTER P
    LineBreakClass::ULB_ID, // 0x3107  # BOPOMOFO LETTER M
    LineBreakClass::ULB_ID, // 0x3108  # BOPOMOFO LETTER F
    LineBreakClass::ULB_ID, // 0x3109  # BOPOMOFO LETTER D
    LineBreakClass::ULB_ID, // 0x310A  # BOPOMOFO LETTER T
    LineBreakClass::ULB_ID, // 0x310B  # BOPOMOFO LETTER N
    LineBreakClass::ULB_ID, // 0x310C  # BOPOMOFO LETTER L
    LineBreakClass::ULB_ID, // 0x310D  # BOPOMOFO LETTER G
    LineBreakClass::ULB_ID, // 0x310E  # BOPOMOFO LETTER K
    LineBreakClass::ULB_ID, // 0x310F  # BOPOMOFO LETTER H
    LineBreakClass::ULB_ID, // 0x3110  # BOPOMOFO LETTER J
    LineBreakClass::ULB_ID, // 0x3111  # BOPOMOFO LETTER Q
    LineBreakClass::ULB_ID, // 0x3112  # BOPOMOFO LETTER X
    LineBreakClass::ULB_ID, // 0x3113  # BOPOMOFO LETTER ZH
    LineBreakClass::ULB_ID, // 0x3114  # BOPOMOFO LETTER CH
    LineBreakClass::ULB_ID, // 0x3115  # BOPOMOFO LETTER SH
    LineBreakClass::ULB_ID, // 0x3116  # BOPOMOFO LETTER R
    LineBreakClass::ULB_ID, // 0x3117  # BOPOMOFO LETTER Z
    LineBreakClass::ULB_ID, // 0x3118  # BOPOMOFO LETTER C
    LineBreakClass::ULB_ID, // 0x3119  # BOPOMOFO LETTER S
    LineBreakClass::ULB_ID, // 0x311A  # BOPOMOFO LETTER A
    LineBreakClass::ULB_ID, // 0x311B  # BOPOMOFO LETTER O
    LineBreakClass::ULB_ID, // 0x311C  # BOPOMOFO LETTER E
    LineBreakClass::ULB_ID, // 0x311D  # BOPOMOFO LETTER EH
    LineBreakClass::ULB_ID, // 0x311E  # BOPOMOFO LETTER AI
    LineBreakClass::ULB_ID, // 0x311F  # BOPOMOFO LETTER EI
    LineBreakClass::ULB_ID, // 0x3120  # BOPOMOFO LETTER AU
    LineBreakClass::ULB_ID, // 0x3121  # BOPOMOFO LETTER OU
    LineBreakClass::ULB_ID, // 0x3122  # BOPOMOFO LETTER AN
    LineBreakClass::ULB_ID, // 0x3123  # BOPOMOFO LETTER EN
    LineBreakClass::ULB_ID, // 0x3124  # BOPOMOFO LETTER ANG
    LineBreakClass::ULB_ID, // 0x3125  # BOPOMOFO LETTER ENG
    LineBreakClass::ULB_ID, // 0x3126  # BOPOMOFO LETTER ER
    LineBreakClass::ULB_ID, // 0x3127  # BOPOMOFO LETTER I
    LineBreakClass::ULB_ID, // 0x3128  # BOPOMOFO LETTER U
    LineBreakClass::ULB_ID, // 0x3129  # BOPOMOFO LETTER IU
    LineBreakClass::ULB_ID, // 0x312A  # BOPOMOFO LETTER V
    LineBreakClass::ULB_ID, // 0x312B  # BOPOMOFO LETTER NG
    LineBreakClass::ULB_ID, // 0x312C  # BOPOMOFO LETTER GN
    LineBreakClass::ULB_ID, // 0x312D # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x312E # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x312F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x3130 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x3131  # HANGUL LETTER KIYEOK
    LineBreakClass::ULB_ID, // 0x3132  # HANGUL LETTER SSANGKIYEOK
    LineBreakClass::ULB_ID, // 0x3133  # HANGUL LETTER KIYEOK-SIOS
    LineBreakClass::ULB_ID, // 0x3134  # HANGUL LETTER NIEUN
    LineBreakClass::ULB_ID, // 0x3135  # HANGUL LETTER NIEUN-CIEUC
    LineBreakClass::ULB_ID, // 0x3136  # HANGUL LETTER NIEUN-HIEUH
    LineBreakClass::ULB_ID, // 0x3137  # HANGUL LETTER TIKEUT
    LineBreakClass::ULB_ID, // 0x3138  # HANGUL LETTER SSANGTIKEUT
    LineBreakClass::ULB_ID, // 0x3139  # HANGUL LETTER RIEUL
    LineBreakClass::ULB_ID, // 0x313A  # HANGUL LETTER RIEUL-KIYEOK
    LineBreakClass::ULB_ID, // 0x313B  # HANGUL LETTER RIEUL-MIEUM
    LineBreakClass::ULB_ID, // 0x313C  # HANGUL LETTER RIEUL-PIEUP
    LineBreakClass::ULB_ID, // 0x313D  # HANGUL LETTER RIEUL-SIOS
    LineBreakClass::ULB_ID, // 0x313E  # HANGUL LETTER RIEUL-THIEUTH
    LineBreakClass::ULB_ID, // 0x313F  # HANGUL LETTER RIEUL-PHIEUPH
    LineBreakClass::ULB_ID, // 0x3140  # HANGUL LETTER RIEUL-HIEUH
    LineBreakClass::ULB_ID, // 0x3141  # HANGUL LETTER MIEUM
    LineBreakClass::ULB_ID, // 0x3142  # HANGUL LETTER PIEUP
    LineBreakClass::ULB_ID, // 0x3143  # HANGUL LETTER SSANGPIEUP
    LineBreakClass::ULB_ID, // 0x3144  # HANGUL LETTER PIEUP-SIOS
    LineBreakClass::ULB_ID, // 0x3145  # HANGUL LETTER SIOS
    LineBreakClass::ULB_ID, // 0x3146  # HANGUL LETTER SSANGSIOS
    LineBreakClass::ULB_ID, // 0x3147  # HANGUL LETTER IEUNG
    LineBreakClass::ULB_ID, // 0x3148  # HANGUL LETTER CIEUC
    LineBreakClass::ULB_ID, // 0x3149  # HANGUL LETTER SSANGCIEUC
    LineBreakClass::ULB_ID, // 0x314A  # HANGUL LETTER CHIEUCH
    LineBreakClass::ULB_ID, // 0x314B  # HANGUL LETTER KHIEUKH
    LineBreakClass::ULB_ID, // 0x314C  # HANGUL LETTER THIEUTH
    LineBreakClass::ULB_ID, // 0x314D  # HANGUL LETTER PHIEUPH
    LineBreakClass::ULB_ID, // 0x314E  # HANGUL LETTER HIEUH
    LineBreakClass::ULB_ID, // 0x314F  # HANGUL LETTER A
    LineBreakClass::ULB_ID, // 0x3150  # HANGUL LETTER AE
    LineBreakClass::ULB_ID, // 0x3151  # HANGUL LETTER YA
    LineBreakClass::ULB_ID, // 0x3152  # HANGUL LETTER YAE
    LineBreakClass::ULB_ID, // 0x3153  # HANGUL LETTER EO
    LineBreakClass::ULB_ID, // 0x3154  # HANGUL LETTER E
    LineBreakClass::ULB_ID, // 0x3155  # HANGUL LETTER YEO
    LineBreakClass::ULB_ID, // 0x3156  # HANGUL LETTER YE
    LineBreakClass::ULB_ID, // 0x3157  # HANGUL LETTER O
    LineBreakClass::ULB_ID, // 0x3158  # HANGUL LETTER WA
    LineBreakClass::ULB_ID, // 0x3159  # HANGUL LETTER WAE
    LineBreakClass::ULB_ID, // 0x315A  # HANGUL LETTER OE
    LineBreakClass::ULB_ID, // 0x315B  # HANGUL LETTER YO
    LineBreakClass::ULB_ID, // 0x315C  # HANGUL LETTER U
    LineBreakClass::ULB_ID, // 0x315D  # HANGUL LETTER WEO
    LineBreakClass::ULB_ID, // 0x315E  # HANGUL LETTER WE
    LineBreakClass::ULB_ID, // 0x315F  # HANGUL LETTER WI
    LineBreakClass::ULB_ID, // 0x3160  # HANGUL LETTER YU
    LineBreakClass::ULB_ID, // 0x3161  # HANGUL LETTER EU
    LineBreakClass::ULB_ID, // 0x3162  # HANGUL LETTER YI
    LineBreakClass::ULB_ID, // 0x3163  # HANGUL LETTER I
    LineBreakClass::ULB_ID, // 0x3164  # HANGUL FILLER
    LineBreakClass::ULB_ID, // 0x3165  # HANGUL LETTER SSANGNIEUN
    LineBreakClass::ULB_ID, // 0x3166  # HANGUL LETTER NIEUN-TIKEUT
    LineBreakClass::ULB_ID, // 0x3167  # HANGUL LETTER NIEUN-SIOS
    LineBreakClass::ULB_ID, // 0x3168  # HANGUL LETTER NIEUN-PANSIOS
    LineBreakClass::ULB_ID, // 0x3169  # HANGUL LETTER RIEUL-KIYEOK-SIOS
    LineBreakClass::ULB_ID, // 0x316A  # HANGUL LETTER RIEUL-TIKEUT
    LineBreakClass::ULB_ID, // 0x316B  # HANGUL LETTER RIEUL-PIEUP-SIOS
    LineBreakClass::ULB_ID, // 0x316C  # HANGUL LETTER RIEUL-PANSIOS
    LineBreakClass::ULB_ID, // 0x316D  # HANGUL LETTER RIEUL-YEORINHIEUH
    LineBreakClass::ULB_ID, // 0x316E  # HANGUL LETTER MIEUM-PIEUP
    LineBreakClass::ULB_ID, // 0x316F  # HANGUL LETTER MIEUM-SIOS
    LineBreakClass::ULB_ID, // 0x3170  # HANGUL LETTER MIEUM-PANSIOS
    LineBreakClass::ULB_ID, // 0x3171  # HANGUL LETTER KAPYEOUNMIEUM
    LineBreakClass::ULB_ID, // 0x3172  # HANGUL LETTER PIEUP-KIYEOK
    LineBreakClass::ULB_ID, // 0x3173  # HANGUL LETTER PIEUP-TIKEUT
    LineBreakClass::ULB_ID, // 0x3174  # HANGUL LETTER PIEUP-SIOS-KIYEOK
    LineBreakClass::ULB_ID, // 0x3175  # HANGUL LETTER PIEUP-SIOS-TIKEUT
    LineBreakClass::ULB_ID, // 0x3176  # HANGUL LETTER PIEUP-CIEUC
    LineBreakClass::ULB_ID, // 0x3177  # HANGUL LETTER PIEUP-THIEUTH
    LineBreakClass::ULB_ID, // 0x3178  # HANGUL LETTER KAPYEOUNPIEUP
    LineBreakClass::ULB_ID, // 0x3179  # HANGUL LETTER KAPYEOUNSSANGPIEUP
    LineBreakClass::ULB_ID, // 0x317A  # HANGUL LETTER SIOS-KIYEOK
    LineBreakClass::ULB_ID, // 0x317B  # HANGUL LETTER SIOS-NIEUN
    LineBreakClass::ULB_ID, // 0x317C  # HANGUL LETTER SIOS-TIKEUT
    LineBreakClass::ULB_ID, // 0x317D  # HANGUL LETTER SIOS-PIEUP
    LineBreakClass::ULB_ID, // 0x317E  # HANGUL LETTER SIOS-CIEUC
    LineBreakClass::ULB_ID, // 0x317F  # HANGUL LETTER PANSIOS
    LineBreakClass::ULB_ID, // 0x3180  # HANGUL LETTER SSANGIEUNG
    LineBreakClass::ULB_ID, // 0x3181  # HANGUL LETTER YESIEUNG
    LineBreakClass::ULB_ID, // 0x3182  # HANGUL LETTER YESIEUNG-SIOS
    LineBreakClass::ULB_ID, // 0x3183  # HANGUL LETTER YESIEUNG-PANSIOS
    LineBreakClass::ULB_ID, // 0x3184  # HANGUL LETTER KAPYEOUNPHIEUPH
    LineBreakClass::ULB_ID, // 0x3185  # HANGUL LETTER SSANGHIEUH
    LineBreakClass::ULB_ID, // 0x3186  # HANGUL LETTER YEORINHIEUH
    LineBreakClass::ULB_ID, // 0x3187  # HANGUL LETTER YO-YA
    LineBreakClass::ULB_ID, // 0x3188  # HANGUL LETTER YO-YAE
    LineBreakClass::ULB_ID, // 0x3189  # HANGUL LETTER YO-I
    LineBreakClass::ULB_ID, // 0x318A  # HANGUL LETTER YU-YEO
    LineBreakClass::ULB_ID, // 0x318B  # HANGUL LETTER YU-YE
    LineBreakClass::ULB_ID, // 0x318C  # HANGUL LETTER YU-I
    LineBreakClass::ULB_ID, // 0x318D  # HANGUL LETTER ARAEA
    LineBreakClass::ULB_ID, // 0x318E  # HANGUL LETTER ARAEAE
    LineBreakClass::ULB_ID, // 0x318F # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x3190  # IDEOGRAPHIC ANNOTATION LINKING MARK
    LineBreakClass::ULB_ID, // 0x3191  # IDEOGRAPHIC ANNOTATION REVERSE MARK
    LineBreakClass::ULB_ID, // 0x3192  # IDEOGRAPHIC ANNOTATION ONE MARK
    LineBreakClass::ULB_ID, // 0x3193  # IDEOGRAPHIC ANNOTATION TWO MARK
    LineBreakClass::ULB_ID, // 0x3194  # IDEOGRAPHIC ANNOTATION THREE MARK
    LineBreakClass::ULB_ID, // 0x3195  # IDEOGRAPHIC ANNOTATION FOUR MARK
    LineBreakClass::ULB_ID, // 0x3196  # IDEOGRAPHIC ANNOTATION TOP MARK
    LineBreakClass::ULB_ID, // 0x3197  # IDEOGRAPHIC ANNOTATION MIDDLE MARK
    LineBreakClass::ULB_ID, // 0x3198  # IDEOGRAPHIC ANNOTATION BOTTOM MARK
    LineBreakClass::ULB_ID, // 0x3199  # IDEOGRAPHIC ANNOTATION FIRST MARK
    LineBreakClass::ULB_ID, // 0x319A  # IDEOGRAPHIC ANNOTATION SECOND MARK
    LineBreakClass::ULB_ID, // 0x319B  # IDEOGRAPHIC ANNOTATION THIRD MARK
    LineBreakClass::ULB_ID, // 0x319C  # IDEOGRAPHIC ANNOTATION FOURTH MARK
    LineBreakClass::ULB_ID, // 0x319D  # IDEOGRAPHIC ANNOTATION HEAVEN MARK
    LineBreakClass::ULB_ID, // 0x319E  # IDEOGRAPHIC ANNOTATION EARTH MARK
    LineBreakClass::ULB_ID, // 0x319F  # IDEOGRAPHIC ANNOTATION MAN MARK
    LineBreakClass::ULB_ID, // 0x31A0  # BOPOMOFO LETTER BU
    LineBreakClass::ULB_ID, // 0x31A1  # BOPOMOFO LETTER ZI
    LineBreakClass::ULB_ID, // 0x31A2  # BOPOMOFO LETTER JI
    LineBreakClass::ULB_ID, // 0x31A3  # BOPOMOFO LETTER GU
    LineBreakClass::ULB_ID, // 0x31A4  # BOPOMOFO LETTER EE
    LineBreakClass::ULB_ID, // 0x31A5  # BOPOMOFO LETTER ENN
    LineBreakClass::ULB_ID, // 0x31A6  # BOPOMOFO LETTER OO
    LineBreakClass::ULB_ID, // 0x31A7  # BOPOMOFO LETTER ONN
    LineBreakClass::ULB_ID, // 0x31A8  # BOPOMOFO LETTER IR
    LineBreakClass::ULB_ID, // 0x31A9  # BOPOMOFO LETTER ANN
    LineBreakClass::ULB_ID, // 0x31AA  # BOPOMOFO LETTER INN
    LineBreakClass::ULB_ID, // 0x31AB  # BOPOMOFO LETTER UNN
    LineBreakClass::ULB_ID, // 0x31AC  # BOPOMOFO LETTER IM
    LineBreakClass::ULB_ID, // 0x31AD  # BOPOMOFO LETTER NGG
    LineBreakClass::ULB_ID, // 0x31AE  # BOPOMOFO LETTER AINN
    LineBreakClass::ULB_ID, // 0x31AF  # BOPOMOFO LETTER AUNN
    LineBreakClass::ULB_ID, // 0x31B0  # BOPOMOFO LETTER AM
    LineBreakClass::ULB_ID, // 0x31B1  # BOPOMOFO LETTER OM
    LineBreakClass::ULB_ID, // 0x31B2  # BOPOMOFO LETTER ONG
    LineBreakClass::ULB_ID, // 0x31B3  # BOPOMOFO LETTER INNN
    LineBreakClass::ULB_ID, // 0x31B4  # BOPOMOFO FINAL LETTER P
    LineBreakClass::ULB_ID, // 0x31B5  # BOPOMOFO FINAL LETTER T
    LineBreakClass::ULB_ID, // 0x31B6  # BOPOMOFO FINAL LETTER K
    LineBreakClass::ULB_ID, // 0x31B7  # BOPOMOFO FINAL LETTER H
    LineBreakClass::ULB_ID, // 0x31B8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31B9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31BA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31BB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31BC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31BD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31BE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31BF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31C0  # CJK STROKE T
    LineBreakClass::ULB_ID, // 0x31C1  # CJK STROKE WG
    LineBreakClass::ULB_ID, // 0x31C2  # CJK STROKE XG
    LineBreakClass::ULB_ID, // 0x31C3  # CJK STROKE BXG
    LineBreakClass::ULB_ID, // 0x31C4  # CJK STROKE SW
    LineBreakClass::ULB_ID, // 0x31C5  # CJK STROKE HZZ
    LineBreakClass::ULB_ID, // 0x31C6  # CJK STROKE HZG
    LineBreakClass::ULB_ID, // 0x31C7  # CJK STROKE HP
    LineBreakClass::ULB_ID, // 0x31C8  # CJK STROKE HZWG
    LineBreakClass::ULB_ID, // 0x31C9  # CJK STROKE SZWG
    LineBreakClass::ULB_ID, // 0x31CA  # CJK STROKE HZT
    LineBreakClass::ULB_ID, // 0x31CB  # CJK STROKE HZZP
    LineBreakClass::ULB_ID, // 0x31CC  # CJK STROKE HPWG
    LineBreakClass::ULB_ID, // 0x31CD  # CJK STROKE HZW
    LineBreakClass::ULB_ID, // 0x31CE  # CJK STROKE HZZZ
    LineBreakClass::ULB_ID, // 0x31CF  # CJK STROKE N
    LineBreakClass::ULB_ID, // 0x31D0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31D1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31D2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31D3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31D4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31D5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31D6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31D7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31D8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31D9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31DA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31DB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31DC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31DD # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31DE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31DF # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31E0 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31E1 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31E2 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31E3 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31E4 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31E5 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31E6 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31E7 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31E8 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31E9 # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31EA # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31EB # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31EC # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31ED # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31EE # <UNDEFINED>
    LineBreakClass::ULB_ID, // 0x31EF # <UNDEFINED>
    LineBreakClass::ULB_NS, // 0x31F0  # KATAKANA LETTER SMALL KU
    LineBreakClass::ULB_NS, // 0x31F1  # KATAKANA LETTER SMALL SI
    LineBreakClass::ULB_NS, // 0x31F2  # KATAKANA LETTER SMALL SU
    LineBreakClass::ULB_NS, // 0x31F3  # KATAKANA LETTER SMALL TO
    LineBreakClass::ULB_NS, // 0x31F4  # KATAKANA LETTER SMALL NU
    LineBreakClass::ULB_NS, // 0x31F5  # KATAKANA LETTER SMALL HA
    LineBreakClass::ULB_NS, // 0x31F6  # KATAKANA LETTER SMALL HI
    LineBreakClass::ULB_NS, // 0x31F7  # KATAKANA LETTER SMALL HU
    LineBreakClass::ULB_NS, // 0x31F8  # KATAKANA LETTER SMALL HE
    LineBreakClass::ULB_NS, // 0x31F9  # KATAKANA LETTER SMALL HO
    LineBreakClass::ULB_NS, // 0x31FA  # KATAKANA LETTER SMALL MU
    LineBreakClass::ULB_NS, // 0x31FB  # KATAKANA LETTER SMALL RA
    LineBreakClass::ULB_NS, // 0x31FC  # KATAKANA LETTER SMALL RI
    LineBreakClass::ULB_NS, // 0x31FD  # KATAKANA LETTER SMALL RU
    LineBreakClass::ULB_NS, // 0x31FE  # KATAKANA LETTER SMALL RE
    LineBreakClass::ULB_NS, // 0x31FF  # KATAKANA LETTER SMALL RO
};

CommonTextCodePage::CommonTextIndirectLineBreakClass
    CommonTextCodePage::_indirectLineBreakClass[] = {
        {
            0x3200,
            0x4DBF,
            LineBreakClass::ULB_ID,
        },
        {
            0x4DC0,
            0x4DFF,
            LineBreakClass::ULB_AL,
        },
        {
            0x4E00,
            0xA014,
            LineBreakClass::ULB_ID,
        },
        {
            0xA015,
            0xA015,
            LineBreakClass::ULB_NS,
        },
        {
            0xA016,
            0xA6FF,
            LineBreakClass::ULB_ID,
        },
        {
            0xA700,
            0xA801,
            LineBreakClass::ULB_AL,
        },
        {
            0xA802,
            0xA802,
            LineBreakClass::ULB_CM,
        },
        {
            0xA803,
            0xA805,
            LineBreakClass::ULB_AL,
        },
        {
            0xA806,
            0xA806,
            LineBreakClass::ULB_CM,
        },
        {
            0xA807,
            0xA80A,
            LineBreakClass::ULB_AL,
        },
        {
            0xA80B,
            0xA80B,
            LineBreakClass::ULB_CM,
        },
        {
            0xA80C,
            0xA822,
            LineBreakClass::ULB_AL,
        },
        {
            0xA823,
            0xA827,
            LineBreakClass::ULB_CM,
        },
        {
            0xA828,
            0xA873,
            LineBreakClass::ULB_AL,
        },
        {
            0xA874,
            0xA875,
            LineBreakClass::ULB_BB,
        },
        {
            0xA876,
            0xABFF,
            LineBreakClass::ULB_EX,
        },
        {
            0xAC00,
            0xDFFF,
            LineBreakClass::ULB_ID,
        },
        {
            0xE000,
            0xF8FF,
            LineBreakClass::ULB_XX,
        },
        {
            0xF900,
            0xFAFF,
            LineBreakClass::ULB_ID,
        },
        {
            0xFB00,
            0xFB1D,
            LineBreakClass::ULB_AL,
        },
        {
            0xFB1E,
            0xFB1E,
            LineBreakClass::ULB_CM,
        },
        {
            0xFB1F,
            0xFD3D,
            LineBreakClass::ULB_AL,
        },
        {
            0xFD3E,
            0xFD3E,
            LineBreakClass::ULB_OP,
        },
        {
            0xFD3F,
            0xFD4F,
            LineBreakClass::ULB_CL,
        },
        {
            0xFD50,
            0xFDFB,
            LineBreakClass::ULB_AL,
        },
        {
            0xFDFC,
            0xFDFC,
            LineBreakClass::ULB_PO,
        },
        {
            0xFDFD,
            0xFDFF,
            LineBreakClass::ULB_AL,
        },
        {
            0xFE00,
            0xFE0F,
            LineBreakClass::ULB_CM,
        },
        {
            0xFE10,
            0xFE10,
            LineBreakClass::ULB_IS,
        },
        {
            0xFE11,
            0xFE12,
            LineBreakClass::ULB_CL,
        },
        {
            0xFE13,
            0xFE14,
            LineBreakClass::ULB_IS,
        },
        {
            0xFE15,
            0xFE16,
            LineBreakClass::ULB_EX,
        },
        {
            0xFE17,
            0xFE17,
            LineBreakClass::ULB_OP,
        },
        {
            0xFE18,
            0xFE18,
            LineBreakClass::ULB_CL,
        },
        {
            0xFE19,
            0xFE1F,
            LineBreakClass::ULB_IN,
        },
        {
            0xFE20,
            0xFE2F,
            LineBreakClass::ULB_CM,
        },
        {
            0xFE30,
            0xFE34,
            LineBreakClass::ULB_ID,
        },
        {
            0xFE35,
            0xFE35,
            LineBreakClass::ULB_OP,
        },
        {
            0xFE36,
            0xFE36,
            LineBreakClass::ULB_CL,
        },
        {
            0xFE37,
            0xFE37,
            LineBreakClass::ULB_OP,
        },
        {
            0xFE38,
            0xFE38,
            LineBreakClass::ULB_CL,
        },
        {
            0xFE39,
            0xFE39,
            LineBreakClass::ULB_OP,
        },
        {
            0xFE3A,
            0xFE3A,
            LineBreakClass::ULB_CL,
        },
        {
            0xFE3B,
            0xFE3B,
            LineBreakClass::ULB_OP,
        },
        {
            0xFE3C,
            0xFE3C,
            LineBreakClass::ULB_CL,
        },
        {
            0xFE3D,
            0xFE3D,
            LineBreakClass::ULB_OP,
        },
        {
            0xFE3E,
            0xFE3E,
            LineBreakClass::ULB_CL,
        },
        {
            0xFE3F,
            0xFE3F,
            LineBreakClass::ULB_OP,
        },
        {
            0xFE40,
            0xFE40,
            LineBreakClass::ULB_CL,
        },
        {
            0xFE41,
            0xFE41,
            LineBreakClass::ULB_OP,
        },
        {
            0xFE42,
            0xFE42,
            LineBreakClass::ULB_CL,
        },
        {
            0xFE43,
            0xFE43,
            LineBreakClass::ULB_OP,
        },
        {
            0xFE44,
            0xFE44,
            LineBreakClass::ULB_CL,
        },
        {
            0xFE45,
            0xFE46,
            LineBreakClass::ULB_ID,
        },
        {
            0xFE47,
            0xFE47,
            LineBreakClass::ULB_OP,
        },
        {
            0xFE48,
            0xFE48,
            LineBreakClass::ULB_CL,
        },
        {
            0xFE49,
            0xFE4F,
            LineBreakClass::ULB_ID,
        },
        {
            0xFE50,
            0xFE50,
            LineBreakClass::ULB_CL,
        },
        {
            0xFE51,
            0xFE51,
            LineBreakClass::ULB_ID,
        },
        {
            0xFE52,
            0xFE53,
            LineBreakClass::ULB_CL,
        },
        {
            0xFE54,
            0xFE55,
            LineBreakClass::ULB_NS,
        },
        {
            0xFE56,
            0xFE57,
            LineBreakClass::ULB_EX,
        },
        {
            0xFE58,
            0xFE58,
            LineBreakClass::ULB_ID,
        },
        {
            0xFE59,
            0xFE59,
            LineBreakClass::ULB_OP,
        },
        {
            0xFE5A,
            0xFE5A,
            LineBreakClass::ULB_CL,
        },
        {
            0xFE5B,
            0xFE5B,
            LineBreakClass::ULB_OP,
        },
        {
            0xFE5C,
            0xFE5C,
            LineBreakClass::ULB_CL,
        },
        {
            0xFE5D,
            0xFE5D,
            LineBreakClass::ULB_OP,
        },
        {
            0xFE5E,
            0xFE5E,
            LineBreakClass::ULB_CL,
        },
        {
            0xFE5F,
            0xFE68,
            LineBreakClass::ULB_ID,
        },
        {
            0xFE69,
            0xFE69,
            LineBreakClass::ULB_PR,
        },
        {
            0xFE6A,
            0xFE6A,
            LineBreakClass::ULB_PO,
        },
        {
            0xFE6B,
            0xFE6F,
            LineBreakClass::ULB_ID,
        },
        {
            0xFE70,
            0xFEFE,
            LineBreakClass::ULB_AL,
        },
        {
            0xFEFF,
            0xFF00,
            LineBreakClass::ULB_WJ,
        },
        {
            0xFF01,
            0xFF01,
            LineBreakClass::ULB_EX,
        },
        {
            0xFF02,
            0xFF03,
            LineBreakClass::ULB_ID,
        },
        {
            0xFF04,
            0xFF04,
            LineBreakClass::ULB_PR,
        },
        {
            0xFF05,
            0xFF05,
            LineBreakClass::ULB_PO,
        },
        {
            0xFF06,
            0xFF07,
            LineBreakClass::ULB_ID,
        },
        {
            0xFF08,
            0xFF08,
            LineBreakClass::ULB_OP,
        },
        {
            0xFF09,
            0xFF09,
            LineBreakClass::ULB_CL,
        },
        {
            0xFF0A,
            0xFF0B,
            LineBreakClass::ULB_ID,
        },
        {
            0xFF0C,
            0xFF0C,
            LineBreakClass::ULB_CL,
        },
        {
            0xFF0D,
            0xFF0D,
            LineBreakClass::ULB_ID,
        },
        {
            0xFF0E,
            0xFF0E,
            LineBreakClass::ULB_CL,
        },
        {
            0xFF0F,
            0xFF19,
            LineBreakClass::ULB_ID,
        },
        {
            0xFF1A,
            0xFF1B,
            LineBreakClass::ULB_NS,
        },
        {
            0xFF1C,
            0xFF1E,
            LineBreakClass::ULB_ID,
        },
        {
            0xFF1F,
            0xFF1F,
            LineBreakClass::ULB_EX,
        },
        {
            0xFF20,
            0xFF3A,
            LineBreakClass::ULB_ID,
        },
        {
            0xFF3B,
            0xFF3B,
            LineBreakClass::ULB_OP,
        },
        {
            0xFF3C,
            0xFF3C,
            LineBreakClass::ULB_ID,
        },
        {
            0xFF3D,
            0xFF3D,
            LineBreakClass::ULB_CL,
        },
        {
            0xFF3E,
            0xFF5A,
            LineBreakClass::ULB_ID,
        },
        {
            0xFF5B,
            0xFF5B,
            LineBreakClass::ULB_OP,
        },
        {
            0xFF5C,
            0xFF5C,
            LineBreakClass::ULB_ID,
        },
        {
            0xFF5D,
            0xFF5D,
            LineBreakClass::ULB_CL,
        },
        {
            0xFF5E,
            0xFF5E,
            LineBreakClass::ULB_ID,
        },
        {
            0xFF5F,
            0xFF5F,
            LineBreakClass::ULB_OP,
        },
        {
            0xFF60,
            0xFF61,
            LineBreakClass::ULB_CL,
        },
        {
            0xFF62,
            0xFF62,
            LineBreakClass::ULB_OP,
        },
        {
            0xFF63,
            0xFF64,
            LineBreakClass::ULB_CL,
        },
        {
            0xFF65,
            0xFF65,
            LineBreakClass::ULB_NS,
        },
        {
            0xFF66,
            0xFF66,
            LineBreakClass::ULB_AL,
        },
        {
            0xFF67,
            0xFF70,
            LineBreakClass::ULB_NS,
        },
        {
            0xFF71,
            0xFF9D,
            LineBreakClass::ULB_AL,
        },
        {
            0xFF9E,
            0xFF9F,
            LineBreakClass::ULB_NS,
        },
        {
            0xFFA0,
            0xFFDF,
            LineBreakClass::ULB_AL,
        },
        {
            0xFFE0,
            0xFFE0,
            LineBreakClass::ULB_PO,
        },
        {
            0xFFE1,
            0xFFE1,
            LineBreakClass::ULB_PR,
        },
        {
            0xFFE2,
            0xFFE4,
            LineBreakClass::ULB_ID,
        },
        {
            0xFFE5,
            0xFFE7,
            LineBreakClass::ULB_PR,
        },
        {
            0xFFE8,
            0xFFF8,
            LineBreakClass::ULB_AL,
        },
        {
            0xFFF9,
            0xFFFB,
            LineBreakClass::ULB_CM,
        },
        {
            0xFFFC,
            0xFFFC,
            LineBreakClass::ULB_CB,
        },
        {
            0xFFFD,
            0xFFFF,
            LineBreakClass::ULB_AI,
        },
        {
            0x10000,
            0x100FF,
            LineBreakClass::ULB_AL,
        },
        {
            0x10100,
            0x10106,
            LineBreakClass::ULB_BA,
        },
        {
            0x10107,
            0x1039E,
            LineBreakClass::ULB_AL,
        },
        {
            0x1039F,
            0x1039F,
            LineBreakClass::ULB_BA,
        },
        {
            0x103A0,
            0x103CF,
            LineBreakClass::ULB_AL,
        },
        {
            0x103D0,
            0x103D0,
            LineBreakClass::ULB_BA,
        },
        {
            0x103D1,
            0x1049F,
            LineBreakClass::ULB_AL,
        },
        {
            0x104A0,
            0x107FF,
            LineBreakClass::ULB_NU,
        },
        {
            0x10800,
            0x1091E,
            LineBreakClass::ULB_AL,
        },
        {
            0x1091F,
            0x109FF,
            LineBreakClass::ULB_BA,
        },
        {
            0x10A00,
            0x10A00,
            LineBreakClass::ULB_AL,
        },
        {
            0x10A01,
            0x10A0F,
            LineBreakClass::ULB_CM,
        },
        {
            0x10A10,
            0x10A37,
            LineBreakClass::ULB_AL,
        },
        {
            0x10A38,
            0x10A3F,
            LineBreakClass::ULB_CM,
        },
        {
            0x10A40,
            0x10A4F,
            LineBreakClass::ULB_AL,
        },
        {
            0x10A50,
            0x10A57,
            LineBreakClass::ULB_BA,
        },
        {
            0x10A58,
            0x1246F,
            LineBreakClass::ULB_AL,
        },
        {
            0x12470,
            0x1CFFF,
            LineBreakClass::ULB_BA,
        },
        {
            0x1D000,
            0x1D164,
            LineBreakClass::ULB_AL,
        },
        {
            0x1D165,
            0x1D169,
            LineBreakClass::ULB_CM,
        },
        {
            0x1D16A,
            0x1D16C,
            LineBreakClass::ULB_AL,
        },
        {
            0x1D16D,
            0x1D182,
            LineBreakClass::ULB_CM,
        },
        {
            0x1D183,
            0x1D184,
            LineBreakClass::ULB_AL,
        },
        {
            0x1D185,
            0x1D18B,
            LineBreakClass::ULB_CM,
        },
        {
            0x1D18C,
            0x1D1A9,
            LineBreakClass::ULB_AL,
        },
        {
            0x1D1AA,
            0x1D1AD,
            LineBreakClass::ULB_CM,
        },
        {
            0x1D1AE,
            0x1D241,
            LineBreakClass::ULB_AL,
        },
        {
            0x1D242,
            0x1D244,
            LineBreakClass::ULB_CM,
        },
        {
            0x1D245,
            0x1D7CD,
            LineBreakClass::ULB_AL,
        },
        {
            0x1D7CE,
            0x1FFFF,
            LineBreakClass::ULB_NU,
        },
        {
            0x20000,
            0xE0000,
            LineBreakClass::ULB_ID,
        },
        {
            0xE0001,
            0xEFFFF,
            LineBreakClass::ULB_CM,
        },
        {
            0xF0000,
            0x10FFFD,
            LineBreakClass::ULB_XX,
        },
    };
PXR_NAMESPACE_CLOSE_SCOPE

#endif // _ACCODEPAGE_CPP

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_LINE_BREAK_H