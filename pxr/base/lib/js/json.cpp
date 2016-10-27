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
/// \file js/json.cpp

#include "pxr/base/js/json.h"
#include "pxr/base/tf/diagnostic.h"
#include <iostream>
#include <vector>

// Place rapidjson into a namespace to prevent conflicts with d2.
#define RAPIDJSON_NAMESPACE PXRJS::rapidjson
#define RAPIDJSON_NAMESPACE_BEGIN namespace PXRJS { namespace rapidjson {
#define RAPIDJSON_NAMESPACE_END } }

#include "rapidjson/allocators.h"
#include "rapidjson/document.h"
#include "rapidjson/reader.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/error/error.h"
#include "rapidjson/error/en.h"

namespace rj = RAPIDJSON_NAMESPACE;

namespace {
struct _InputHandler : public rj::BaseReaderHandler<rj::UTF8<>, _InputHandler>
{
    bool Null() {
        values.push_back(JsValue());
        return true;
    }
    bool Bool(bool b) {
        values.push_back(JsValue(b));
        return true;
    }
    bool Int(int i) {
        values.push_back(JsValue(i));
        return true;
    }
    bool Uint(unsigned u) {
        values.push_back(JsValue(static_cast<uint64_t>(u)));
        return true;
    }
    bool Int64(int64_t i) {
        values.push_back(JsValue(i));
        return true;
    }
    bool Uint64(uint64_t u) {
        values.push_back(JsValue(u));
        return true;
    }
    bool Double(double d) {
        values.push_back(JsValue(d));
        return true;
    }
    bool String(const char* str, rj::SizeType length, bool /* copy */) {
        values.push_back(JsValue(std::string(str, length)));
        return true;
    }
    bool Key(const char* str, rj::SizeType length, bool /* copy */) {
        keys.push_back(std::string(str, length));
        return true;
    }
    bool StartObject() {
        return true;
    }
    bool EndObject(rj::SizeType memberCount) {
        const size_t kstart = keys.size() - memberCount;
        const size_t vstart = values.size() - memberCount;

        JsObject object;
        for (size_t i = 0; i < memberCount; ++i) {
            object.insert(std::make_pair(keys[kstart + i], values[vstart + i]));
        }

        keys.resize(kstart);
        values.resize(vstart);

        values.push_back(JsValue(object));
        return true;
    }
    bool StartArray() {
        return true;
    }
    bool EndArray(rj::SizeType elementCount) {
        const std::vector<JsValue> valueArray(
            values.end() - elementCount, values.end());
        values.resize(values.size() - elementCount);
        values.push_back(JsValue(valueArray));
        return true;
    }

public:
    std::vector<JsObject::key_type> keys;
    std::vector<JsObject::mapped_type> values;
};
}

template <typename Allocator>
static rj::Value
_ToImplObjectValue(
    const JsObject& object,
    Allocator& allocator)
{
    rj::Value value(rj::kObjectType);

    for (const auto& p : object) {
        value.AddMember(
            rj::Value(p.first.c_str(), allocator).Move(),
            _JsValueToImplValue(p.second, allocator),
            allocator);
    }

    return value;
}

template <typename Allocator>
static rj::Value
_ToImplArrayValue(
    const JsArray& array,
    Allocator& allocator)
{
    rj::Value value(rj::kArrayType);

    for (const auto& e : array) {
        value.PushBack(
            rj::Value(_JsValueToImplValue(e, allocator)).Move(),
            allocator);
    }

    return value;
}

template <typename Allocator>
static rj::Value
_JsValueToImplValue(
    const JsValue& value,
    Allocator& allocator)
{
    switch (value.GetType()) {
    case JsValue::ObjectType:
        return _ToImplObjectValue(value.GetJsObject(), allocator);
    case JsValue::ArrayType:
        return _ToImplArrayValue(value.GetJsArray(), allocator);
    case JsValue::BoolType:
        return rj::Value(value.GetBool());
    case JsValue::StringType:
        return rj::Value(value.GetString().c_str(), allocator);
    case JsValue::RealType:
        return rj::Value(value.GetReal());
    case JsValue::IntType:
        return value.IsUInt64() ?
            rj::Value(value.GetUInt64()) : rj::Value(value.GetInt64());
    case JsValue::NullType:
        return rj::Value();
    default: {
        TF_CODING_ERROR("Unknown JsValue type");
        return rj::Value();
        }
    }
}


JsValue
JsParseStream(
    std::istream& istr,
    JsParseError* error)
{
    if (not istr) {
        TF_CODING_ERROR("Stream error");
        return JsValue();
    }

    // Parse streams by reading into a string first. This makes it easier to
    // yield good error messages that include line and column numbers, rather
    // than the character offset that rapidjson currently provides.
    return JsParseString(std::string(
        (std::istreambuf_iterator<char>(istr)),
         std::istreambuf_iterator<char>()),
        error);
}

JsValue
JsParseString(
    const std::string& data,
    JsParseError* error)
{
    if (data.empty()) {
        TF_CODING_ERROR("JSON string is empty");
        return JsValue();
    }

    _InputHandler handler;
    rj::Reader reader;
    rj::StringStream ss(data.c_str());
    rj::ParseResult result =
        reader.Parse<rj::kParseStopWhenDoneFlag>(ss, handler);

    if (not result) {
        if (error) {
            // Rapidjson only provides a character offset for errors, not
            // line/column information like other parsers (like json_spirit,
            // upon which this library was previously implemented). Analyze
            // the input data to convert the offset to line/column.
            error->line = 1;
            const size_t eoff = result.Offset();
            size_t nlpos = 0;
            for (size_t i = 0; i < eoff; ++i) {
                if (data[i] == '\n') {
                    error->line++;
                    nlpos = i;
                }
            }
            error->column = static_cast<unsigned int>(eoff - nlpos);
            error->reason = rj::GetParseError_En(result.Code());
        }
        return JsValue();
    }

    TF_VERIFY(handler.values.size() == 1,
        "Unexpected value count: %zu", handler.values.size());

    return handler.values.empty() ? JsValue() : handler.values.front();
}

void
JsWriteToStream(
    const JsValue& value,
    std::ostream& ostr)
{
    if (not ostr) {
        TF_CODING_ERROR("Stream error");
        return;
    }

    ostr << JsWriteToString(value);
}

std::string
JsWriteToString(
    const JsValue& value)
{
    rj::Document d;
    const rj::Value ivalue = _JsValueToImplValue(value, d.GetAllocator());

    rj::StringBuffer buffer;
    rj::PrettyWriter<rj::StringBuffer> writer(buffer);
    writer.SetFormatOptions(rj::kFormatSingleLineArray);
    ivalue.Accept(writer);

    return buffer.GetString();
} 
