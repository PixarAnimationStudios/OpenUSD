//
// Copyright 2016 Pixar
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
///
/// \file js/utils.h

#ifndef JS_UTILS_H
#define JS_UTILS_H

#include "pxr/base/js/api.h"
#include "pxr/base/js/value.h"
#include <boost/none.hpp>
#include <boost/optional.hpp>
#include <string>

typedef boost::optional<JsValue> JsOptionalValue;

/// Returns the value associated with \p key in the given \p object. If no
/// such key exists, and the supplied default is not supplied, this method
/// returns an uninitialized optional JsValue. Otherwise, the \p 
/// defaultValue is returned.

JS_API JsOptionalValue JsFindValue(
    const JsObject& object,
    const std::string& key,
    const JsOptionalValue& defaultValue = boost::none);

#endif // JS_UTILS_H
