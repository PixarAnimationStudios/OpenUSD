//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file js/json.cpp

#include "pxr/pxr.h"
#include "pxr/base/js/json.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/stringUtils.h"

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

// This class is needed to override writing out doubles. There is a bug in 
// rapidJSON when writing out some double values. These classes uses the Tf
// library to do the conversion instead.
// See: https://github.com/Tencent/rapidjson/issues/954

template <class TBase>
class _WriterFix : public TBase
{
public:
    using Base = TBase;
    using Base::Base;

    bool Double(double d) { 
        constexpr int bufferSize = 32;
        char buffer[bufferSize];
        TfDoubleToString(d, buffer, bufferSize, true);
        
        return Base::RawValue(buffer, strlen(buffer), rj::kNumberType);
     }
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
    // Need Full precision flag to round trip double values correctly.
    rj::ParseResult result =
        reader.Parse<rj::kParseFullPrecisionFlag|rj::kParseStopWhenDoneFlag>(
            ss, handler);

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
    _WriterFix<rj::PrettyWriter<rj::OStreamWrapper>> writer(os);
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
     _WriterFix<rj::PrettyWriter<rj::StringBuffer>> writer(buffer);
    writer.SetFormatOptions(rj::kFormatSingleLineArray);
    ivalue.Accept(writer);

    return buffer.GetString();
} 


void
JsWriteValue(JsWriter* writer, const JsValue& js)
{
    if (!writer) {
        return;
    }

    if (js.IsObject()) {
        const JsObject& obj = js.GetJsObject();
        writer->BeginObject();
        for (const JsObject::value_type& field : obj) {
            writer->WriteKey(field.first);
            JsWriteValue(writer, field.second);
        }
        writer->EndObject();
    } else if (js.IsArray()) {
        const JsArray& array = js.GetJsArray();
        writer->BeginArray();
        for (const JsValue& elem : array) {
            JsWriteValue(writer, elem);
        }
        writer->EndArray();
    } else if (js.IsUInt64()) {
        writer->WriteValue(js.GetUInt64());
    } else if (js.IsString()) {
        writer->WriteValue(js.GetString());
    } else if (js.IsBool()) {
        writer->WriteValue(js.GetBool());
    } else if (js.IsReal()) {
        writer->WriteValue(js.GetReal());
    } else if (js.IsInt()) {
        writer->WriteValue(js.GetInt64());
    } else if (js.IsNull()) {
        writer->WriteValue(nullptr);
    }
}

//
// JsWriter
//

namespace {

// This helper interface is to wrap rapidJSON Writer and PrettyWriter so we can 
// choose which writer to use at runtime.
class Js_PolymorphicWriterInterface
{
public:
    virtual ~Js_PolymorphicWriterInterface();
    virtual bool Null () = 0;
    virtual bool Bool (bool b) = 0; 
    virtual bool Int (int i) = 0; 
    virtual bool Uint (unsigned u) = 0; 
    virtual bool Int64 (int64_t i64) = 0; 
    virtual bool Uint64 (uint64_t u64) = 0; 
    virtual bool Double (double d) = 0; 
    virtual bool String (const char *str, size_t length) = 0; 
    virtual bool StartObject () = 0; 
    virtual bool Key (const char *str, size_t length) = 0; 
    virtual bool EndObject () = 0; 
    virtual bool StartArray () = 0; 
    virtual bool EndArray() = 0;
};

Js_PolymorphicWriterInterface::~Js_PolymorphicWriterInterface() = default;

// Wraps the rapidJSON class and exposes its interface via virtual functions.
template <class TWriter>
class Js_PolymorphicWriter : public Js_PolymorphicWriterInterface, public TWriter
{
public:
    using Writer = TWriter;
    using Writer::Writer;

    bool Null () override {
        return Writer::Null();
    }
    bool Bool (bool b) override {
        return Writer::Bool(b);
    }
    bool Int (int i) override {
        return Writer::Int(i);
    }
    bool Uint (unsigned u) override {
        return Writer::Uint(u);
    }
    bool Int64 (int64_t i64) override {
        return Writer::Int64(i64);
    }
    bool Uint64 (uint64_t u64) override {
        return Writer::Uint64(u64);
    }
    bool Double (double d) override {
        return Writer::Double(d);
    }
    bool String (const char *str, size_t length) override {
        return Writer::String(str, length);
    }
    bool StartObject () override {
        return Writer::StartObject();
    }
    bool Key (const char *str, size_t length) override {
        return Writer::Key(str, length);
    }
    bool EndObject () override {
        return Writer::EndObject();
    }
    bool StartArray () override {
        return Writer::StartArray();
    }
    bool EndArray() override {
        return Writer::EndArray();
    }
};

}

// JsWriter is a wrapper around a Js_PolymorphicWriterInterface instance.
class JsWriter::_Impl
{
    using PrettyWriter =
        Js_PolymorphicWriter<_WriterFix<rj::PrettyWriter<rj::OStreamWrapper>>>;
    using Writer =
        Js_PolymorphicWriter<_WriterFix<rj::Writer<rj::OStreamWrapper>>>;

public:
    _Impl(std::ostream& s, Style style) 
    : _strWrapper(s) {
        switch (style) {
            case Style::Compact:
                _writer = std::unique_ptr<Writer>(new Writer(_strWrapper));
                break;
            case Style::Pretty:
                _writer = std::unique_ptr<PrettyWriter>(
                    new PrettyWriter(_strWrapper));
                break;
        }
    }
    
    Js_PolymorphicWriterInterface* GetWriter() {
        return _writer.get();
    }

private:
    std::unique_ptr<Js_PolymorphicWriterInterface> _writer;
    rj::OStreamWrapper _strWrapper;
};

JsWriter::JsWriter(std::ostream& ostr, Style style)
    : _impl(new _Impl(ostr, style))
{
    
}

JsWriter::~JsWriter() = default;

bool JsWriter::WriteValue(std::nullptr_t)
{
    return _impl->GetWriter()->Null();
}

bool JsWriter::WriteValue(bool b )
{
    return _impl->GetWriter()->Bool(b);
}

bool JsWriter::WriteValue(int i )
{
    return _impl->GetWriter()->Int(i);
}

bool JsWriter::WriteValue(unsigned u )
{
    return _impl->GetWriter()->Uint(u);
}

bool JsWriter::WriteValue(int64_t i64 )
{
    return _impl->GetWriter()->Int64(i64);
}

bool JsWriter::WriteValue(uint64_t u64 )
{
    return _impl->GetWriter()->Uint64(u64);
}

bool JsWriter::WriteValue(double d )
{
    return _impl->GetWriter()->Double(d);
}

bool JsWriter::WriteValue(const std::string& s)
{
    return _impl->GetWriter()->String(s.c_str(), s.length());
}

bool JsWriter::WriteValue(const char* s)
{
    return _impl->GetWriter()->String(s, strlen(s));
}

bool JsWriter::BeginObject( )
{
    return _impl->GetWriter()->StartObject();
}

bool JsWriter::WriteKey(const std::string& k)
{
    return _impl->GetWriter()->Key(k.c_str(), k.length());
}

bool JsWriter::WriteKey(const char* k)
{
    return _impl->GetWriter()->Key(k, strlen(k));
}

bool JsWriter::_Key(const char* s, size_t len)
{
    return _impl->GetWriter()->Key(s, len);
}

bool JsWriter::_String(const char* s, size_t len)
{
    return _impl->GetWriter()->String(s, len);
}

bool JsWriter::EndObject( )
{
    return _impl->GetWriter()->EndObject();
}

bool JsWriter::BeginArray( )
{
    return _impl->GetWriter()->StartArray();
}

bool JsWriter::EndArray( )
{
    return _impl->GetWriter()->EndArray();
}

PXR_NAMESPACE_CLOSE_SCOPE
