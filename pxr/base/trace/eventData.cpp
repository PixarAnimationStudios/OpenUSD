//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/base/trace/eventData.h"

#include "pxr/pxr.h"
#include "pxr/base/js/json.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Variant visitor to convert TraceEventData to JsValue
class JsValue_visitor
{
public:
    JsValue_visitor(JsWriter& writer)
        : _writer(writer) {}

    void operator()(int64_t i) const {
        _writer.WriteValue(i);
    }

    void operator()(uint64_t i) const {
        _writer.WriteValue(i);
    }

    void operator()(bool i) const {
        _writer.WriteValue(i);
    }

    void operator()(double v) const {
        _writer.WriteValue(v);
    }

    void operator()(const std::string& s) const {
        _writer.WriteValue(s);
    }
    
    template<class T>
    void operator()(T) const {
        _writer.WriteValue(nullptr);
    }

private:
    JsWriter& _writer;
};

// Variant visitor to convert TraceEventData to TraceEvent::DataType
class Type_visitor
{
public:
    TraceEvent::DataType operator()(int64_t i) const {
        return TraceEvent::DataType::Int;
    }

    TraceEvent::DataType operator()(uint64_t i) const {
        return TraceEvent::DataType::UInt;
    }

    TraceEvent::DataType operator()(bool i) const {
        return TraceEvent::DataType::Boolean;
    }

    TraceEvent::DataType operator()(double v) const {
        return TraceEvent::DataType::Float;
    }

    TraceEvent::DataType operator()(const std::string& s) const {
        return TraceEvent::DataType::String;
    }

    template<class T>
    TraceEvent::DataType operator()(T) const {
        return TraceEvent::DataType::Invalid;
    }
};

}

TraceEvent::DataType TraceEventData::GetType() const
{
    return std::visit(Type_visitor(), _data);
}

const int64_t* TraceEventData::GetInt() const
{
    return GetType() == TraceEvent::DataType::Int ?
        &std::get<int64_t>(_data) : nullptr;
}

const uint64_t* TraceEventData::GetUInt() const
{
    return GetType() == TraceEvent::DataType::UInt ?
        &std::get<uint64_t>(_data) : nullptr;
}

const double* TraceEventData::GetFloat() const
{
    return GetType() == TraceEvent::DataType::Float ?
        &std::get<double>(_data) : nullptr;
}

const bool* TraceEventData::GetBool() const
{
    return GetType() == TraceEvent::DataType::Boolean ?
        &std::get<bool>(_data) : nullptr;
}

const std::string* TraceEventData::GetString() const
{
    return GetType() == TraceEvent::DataType::String ?
        &std::get<std::string>(_data) : nullptr;
}

void TraceEventData::WriteJson(JsWriter& writer) const
{
    std::visit(JsValue_visitor(writer), _data);
}

PXR_NAMESPACE_CLOSE_SCOPE