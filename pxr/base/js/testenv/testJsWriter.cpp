//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file testenv/testJsWriter.cpp

#include "pxr/pxr.h"
#include "pxr/base/js/json.h"
#include "pxr/base/tf/diagnostic.h"

#include <fstream>
#include <limits>

PXR_NAMESPACE_USING_DIRECTIVE

static JsWriter::Style
StyleFromArg(const std::string& styleStr)
{
    if (styleStr == "compact") {
        return JsWriter::Style::Compact;
    } else if (styleStr == "pretty") {
        return JsWriter::Style::Pretty;
    } else {
        // Invalid style argument.
        TF_AXIOM(false);
        return JsWriter::Style::Compact;
    }
}

int main(int argc, char const *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s outputFile [compact|pretty]\n", argv[0]);
        return 1;
    }

    std::ofstream ofs(argv[1]);
    if (!ofs) {
        fprintf(stderr, "Error: failed to open output file '%s'", argv[1]);
        return 2;
    }

    JsWriter::Style style = StyleFromArg(argv[2]);

    JsWriter js(ofs, style);
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
