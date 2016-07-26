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
/// \file js/json.h

#ifndef JS_JSON_H
#define JS_JSON_H

#include "pxr/base/js/api.h"
#include "pxr/base/js/value.h"
#include <iosfwd>
#include <string>

/// \struct JsParseError
/// A struct containing information about a JSON parsing error.
struct JS_API JsParseError {
    JsParseError() : line(0), column(0) { }
    unsigned int line;
    unsigned int column;
    std::string reason;
};

/// Parse the contents of input stream \p istr and return a JsValue. On
/// failure, this returns a null JsValue.
JS_API
JsValue JsParseStream(std::istream& istr, JsParseError* error = 0);

/// Parse the contents of the JSON string \p data and return it as a JsValue.
/// On failure, this returns a null JsValue.
JS_API
JsValue JsParseString(const std::string& data, JsParseError* error = 0);

/// Convert the JsValue \p value to JSON and write the result to output stream
/// \p ostr.
JS_API
void JsWriteToStream(const JsValue& value, std::ostream& ostr);

/// Convert the JsValue \p value to JSON and return it as a string.
JS_API
std::string JsWriteToString(const JsValue& value);

#endif // JS_JSON_H
