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

#include "pxr/pxr.h"
#include "pxr/base/js/json.h"
#include "pxr/base/tf/diagnostic.h"

#include <iostream>
#include <vector>

#if PXR_USE_NAMESPACES
#define PXRJS PXR_NS
#else
#define PXRJS PXRJS
#endif

// Place rapidjson into a namespace to prevent conflicts with d2.
#define RAPIDJSON_NAMESPACE PXRJS::rapidjson
#define RAPIDJSON_NAMESPACE_BEGIN namespace PXRJS { namespace rapidjson {
#define RAPIDJSON_NAMESPACE_END } }

// USD requires a C++11 compliant compiler, so we can enable these.
#define RAPIDJSON_HAS_CXX11_RVALUE_REFS 1
#define RAPIDJSON_HAS_CXX11_NOEXCEPT 1
#define RAPIDJSON_HAS_CXX11_TYPETRAITS 1
#define RAPIDJSON_HAS_CXX11_RANGE_FOR 1

#include "rapidjson/allocators.h"
#include "rapidjson/document.h"
#include "rapidjson/reader.h"
#include "rapidjson/ostreamwrapper.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/error/error.h"
#include "rapidjson/error/en.h"

namespace rj = RAPIDJSON_NAMESPACE;

namespace {
PXR_NAMESPACE_USING_DIRECTIVE

struct _InputHandler : public rj::BaseReaderHandler<rj::UTF8<>, _InputHandler>
{
    bool Null() {
        values.emplace_back();
        return true;
    }
    bool Bool(bool b) {
        values.emplace_back(b);
        return true;
    }
    bool Int(int i) {
        values.emplace_back(i);
        return true;
    }
    bool Uint(unsigned u) {
        values.emplace_back(static_cast<uint64_t>(u));
        return true;
    }
    bool Int64(int64_t i) {
        values.emplace_back(i);
        return true;
    }
    bool Uint64(uint64_t u) {
        values.emplace_back(u);
        return true;
    }
    bool Double(double d) {
        values.emplace_back(d);
        return true;
    }
    bool String(const char* str, rj::SizeType length, bool /* copy */) {
        values.emplace_back(std::string(str, length));
        return true;
    }
    bool Key(const char* str, rj::SizeType length, bool /* copy */) {
        keys.emplace_back(str, length);
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
            object.insert(
                std::make_pair(
                    std::move(keys[kstart + i]),
                    std::move(values[vstart + i])));
        }

        keys.resize(kstart);
        values.resize(vstart);

        values.emplace_back(std::move(object));
        return true;
    }
    bool BeginArray() {
        return true;
    }
    bool EndArray(rj::SizeType elementCount) {
        std::vector<JsValue> valueArray(
            values.end() - elementCount, values.end());
        values.resize(values.size() - elementCount);
        values.emplace_back(std::move(valueArray));
        return true;
    }

public:
    std::vector<JsObject::key_type> keys;
    std::vector<JsObject::mapped_type> values;
};
}

PXR_NAMESPACE_OPEN_SCOPE

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
    if (!istr) {
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

    if (!result) {
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
            error->column = eoff - nlpos;
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
    if (!ostr) {
        TF_CODING_ERROR("Stream error");
        return;
    }

    rj::Document d;
    const rj::Value ivalue = _JsValueToImplValue(value, d.GetAllocator());

    rj::OStreamWrapper os(ostr);
    rj::PrettyWriter<rj::OStreamWrapper> writer(os);
    writer.SetFormatOptions(rj::kFormatSingleLineArray);
    ivalue.Accept(writer);
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

//
// JsWriter
//

// JsWriter is just a wrapper around a rapidJSON stream writer.

class JsWriter::_Impl : public rj::Writer<rj::OStreamWrapper>
{
    using Parent = rj::Writer<rj::OStreamWrapper>;
public:
    _Impl(std::ostream& s) 
    : Parent()
    , _strWrapper(s) {
        Reset(_strWrapper);
    }
private:
    rj::OStreamWrapper _strWrapper;
};

JsWriter::JsWriter(std::ostream& ostr)
    : _impl(new _Impl(ostr))
{
    
}

JsWriter::~JsWriter() = default;

bool JsWriter::WriteValue(std::nullptr_t)
{
    return _impl->Null();
}

bool JsWriter::WriteValue(bool b )
{
    return _impl->Bool(b);
}

bool JsWriter::WriteValue(int i )
{
    return _impl->Int(i);
}

bool JsWriter::WriteValue(unsigned u )
{
    return _impl->Uint(u);
}

bool JsWriter::WriteValue(int64_t i64 )
{
    return _impl->Int64(i64);
}

bool JsWriter::WriteValue(uint64_t u64 )
{
    return _impl->Uint64(u64);
}

bool JsWriter::WriteValue(double d )
{
    return _impl->Double(d);
}

bool JsWriter::WriteValue(const std::string& s)
{
    return _impl->String(s.c_str(), s.length());
}

bool JsWriter::WriteValue(const char* s)
{
    return _impl->String(s, strlen(s));
}

bool JsWriter::BeginObject( )
{
    return _impl->StartObject();
}

bool JsWriter::WriteKey(const std::string& k)
{
    return _impl->Key(k.c_str(), k.length());
}

bool JsWriter::WriteKey(const char* k)
{
    return _impl->Key(k, strlen(k));
}

bool JsWriter::_Key(const char* s, size_t len)
{
    return _impl->Key(s, len);
}

bool JsWriter::_String(const char* s, size_t len)
{
    return _impl->String(s, len);
}

bool JsWriter::EndObject( )
{
    return _impl->EndObject();
}

bool JsWriter::BeginArray( )
{
    return _impl->StartArray();
}

bool JsWriter::EndArray( )
{
    return _impl->EndArray();
}

PXR_NAMESPACE_CLOSE_SCOPE
