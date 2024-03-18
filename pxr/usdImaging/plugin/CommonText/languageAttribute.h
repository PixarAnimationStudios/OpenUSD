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

#ifndef PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_LANGUAGE_ATTRIBUTE_H
#define PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_LANGUAGE_ATTRIBUTE_H

#include "definitions.h"

PXR_NAMESPACE_OPEN_SCOPE

/// \struct CommonTextLanguageAttribute
///
/// The attribute of a language.
///
struct CommonTextLanguageAttribute
{
    int _startIndex              = 0;     // the start of the language in unicode
    int _endIndex                = 0;     // the end of the language in unicode
    bool _haveWordBreakCharacter = false; // whether the language have word break character
    wchar_t _wordBreakCharacter  = 0;     // the word break character

    /// The default constructor
    CommonTextLanguageAttribute() = default;

    /// The constructor
    CommonTextLanguageAttribute(
        int start, 
        int end, 
        bool haveBreak, 
        wchar_t demiliter)
        : _startIndex(start)
        , _endIndex(end)
        , _haveWordBreakCharacter(haveBreak)
        , _wordBreakCharacter(demiliter)
    {
    }
};

typedef std::vector<CommonTextLanguageAttribute> CommonTextLanguageAttributeSet;

// Initialize the language attributes.
void InitializeLanguageAttributeSet();

// Get the language attributes.
const CommonTextLanguageAttributeSet& GetLanguageAttributeSet();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_IMAGING_PLUGIN_COMMON_TEXT_LANGUAGE_ATTRIBUTE_H
