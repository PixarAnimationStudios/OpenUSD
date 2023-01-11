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

#include "languageAttribute.h"

PXR_NAMESPACE_OPEN_SCOPE
CommonTextLanguageAttributeSet gLanguageAttributeSet;

void 
InitializeLanguageAttributeSet()
{
    // All these are CJK characters. There is no word break for CJK characters.
    gLanguageAttributeSet.push_back(CommonTextLanguageAttribute(0x2E80, 0xA4CF, false, L' '));
    gLanguageAttributeSet.push_back(CommonTextLanguageAttribute(0xAC00, 0xFAFF, false, L' '));
    gLanguageAttributeSet.push_back(CommonTextLanguageAttribute(0xFE30, 0xFE6F, false, L' '));
    gLanguageAttributeSet.push_back(CommonTextLanguageAttribute(0xFF00, 0xFFEE, false, L' '));
    // All the other characters are considered as Western Europe scripts.
    gLanguageAttributeSet.push_back(CommonTextLanguageAttribute(0x00, 0xFFFF, true, L' '));
}

const 
CommonTextLanguageAttributeSet& GetLanguageAttributeSet()
{
    return gLanguageAttributeSet;
}
PXR_NAMESPACE_CLOSE_SCOPE
