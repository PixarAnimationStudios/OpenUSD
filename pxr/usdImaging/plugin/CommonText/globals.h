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
#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_GLOBALS_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_GLOBALS_H

#include "definitions.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \enum class CommonTextStatus
///
/// The status enum for CommonTextSystem return value.
///
enum class CommonTextStatus
{
    /// The function works successful.
    CommonTextStatusSuccess,
    /// The function meets with error.
    CommonTextStatusFail,
    /// The argument is invalid.
    CommonTextStatusInvalidArg,
    /// The instance is not initialized.
    CommonTextStatusNotInitialized,
    /// The function is not implemented yet.
    CommonTextStatusNotImplement,
    /// The feature is not supported.
    CommonTextStatusNotSupport,
    /// The font is not found.
    CommonTextStatusFontNotFound,
    /// The character is not found.
    CommonTextStatusCharacterNotFound,
    /// The text should do font substitution
    CommonTextStatusNeedSubstitution
};

/// \struct CommonTextScriptInfo
///
/// The information of the script in the string.
///
struct CommonTextScriptInfo
{
    // The index of the first character of the script. This index doesn't consider the tab
    // before the text run.
    int _indexOfFirstCharacter;

    // The number of the script. (number, not the count)
    // For a same script, this number maybe different for different computers.
    int _script;
};

typedef std::vector<std::string> CommonTextStringArray;

/// The CommonTextFontMapCache is a cache of the mapping from codepage to typefaces.
/// For each codepage, there will be one or more typefaces which supports this codepage.
/// So the CommonTextFontMapCache includes an array of typefaces for each codepage.
typedef std::unordered_map<int, std::shared_ptr<CommonTextStringArray>> CommonTextFontMapCache;

/// The maximum count of characters in one line
#define MAXIMUM_COUNT_OF_CHAR_IN_LINE 200

/// The glyph index to mark the character doesn't exist in the truetype font
/// Warning: the value of this macro is the same as the value which is set to the missing character
/// by the GDI function GetGlyphIndices.
#define TRUETYPE_MISSING_GLYPH_INDEX 0xffff

#if !defined(_WIN32)
#define DECLARE_HEAP_OPERATORS_POSIX() DECLARE_HEAP_OPERATORS()
#define DECLARE_PLACEMENT_NEW_POSIX()
#else
#define DECLARE_HEAP_OPERATORS_POSIX()
#define DECLARE_PLACEMENT_NEW_POSIX()
#endif

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_GLOBALS_H
