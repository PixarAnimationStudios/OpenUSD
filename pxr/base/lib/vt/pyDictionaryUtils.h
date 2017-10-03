//
// Copyright 2017 Pixar
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
#ifndef VT_PY_DICTIONARY_UTILS_H
#define VT_PY_DICTIONARY_UTILS_H

#include "pxr/pxr.h"
#include "pxr/base/vt/dictionary.h"

#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// Evaluates the specified \p content string as a python dictionary and
/// returns it as a VtDictionary.
VT_API VtDictionary VtDictionaryFromPythonString(
    const std::string& content);

/// Same as VtDictionaryFromPythonString but with more flexible failure
/// policy.  Returns \c true if successful with the result in \p dict,
/// otherwise returns \c false.
/// 
VT_API bool VtDictionaryFromPythonString(
    const std::string& content, 
    VtDictionary* dict);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // VT_PY_DICTIONARY_UTILS_H
