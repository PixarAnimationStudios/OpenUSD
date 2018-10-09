//
// Copyright 2018 Pixar
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
/// \file testenv/testJsWriter.cpp

#include "pxr/pxr.h"
#include "pxr/base/js/json.h"

#include <fstream>

PXR_NAMESPACE_USING_DIRECTIVE

int main(int argc, char const *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s outputFile\n", argv[0]);
        return 1;
    }

    std::ofstream ifs(argv[1]);
    if (!ifs) {
        fprintf(stderr, "Error: failed to open output file '%s'", argv[1]);
        return 2;
    }
    JsWriter js(ifs);
    js.BeginArray();

    // Explicit interface
    js.BeginObject();
    js.WriteKeyValue("bool", true);
    js.WriteKeyValue("null", nullptr);
    js.WriteKeyValue("int", -1);
    js.WriteKeyValue("uint", static_cast<unsigned int>(42));
    js.WriteKeyValue("int64", std::numeric_limits<int64_t>::min());
    js.WriteKeyValue("uint64", std::numeric_limits<uint64_t>::max());
    js.WriteKeyValue("double", std::numeric_limits<double>::epsilon());
    js.WriteKeyValue("string", "Some string");
    js.WriteKey("array");
    js.BeginArray();
        js.WriteValue(true);
        js.WriteValue(nullptr);
        js.WriteValue(-1);
        js.WriteValue(static_cast<unsigned int>(42));
        js.WriteValue(std::numeric_limits<int64_t>::min());
        js.WriteValue(std::numeric_limits<uint64_t>::max());
        js.WriteValue(std::numeric_limits<double>::epsilon());
        js.WriteValue("Some string");
    js.EndArray();
    js.EndObject();

    // Convenience interface
    js.WriteObject(
        "bool", true,
        "null", nullptr,
        "int", -1,
        "uint", static_cast<unsigned int>(42),
        "int64", std::numeric_limits<int64_t>::min(),
        "uint64", std::numeric_limits<uint64_t>::max(),
        "double", std::numeric_limits<double>::epsilon(),
        "string", "Some string",
        "array", [] (JsWriter& js) {
            js.BeginArray();
                js.WriteValue(true);
                js.WriteValue(nullptr);
                js.WriteValue(-1);
                js.WriteValue(static_cast<unsigned int>(42));
                js.WriteValue(std::numeric_limits<int64_t>::min());
                js.WriteValue(std::numeric_limits<uint64_t>::max());
                js.WriteValue(std::numeric_limits<double>::epsilon());
                js.WriteValue("Some string");
            js.EndArray();
        });
    
    using StringIntPair = std::pair<std::string, int>;
    std::vector<StringIntPair> v = {{"a",1}, {"b",2}, {"c",3}, {"d",4}};
    js.WriteArray(v, [] (JsWriter& js, const StringIntPair& p) {
        js.WriteObject(p.first, p.second);
    });
    js.EndArray();

    return 0;
}
