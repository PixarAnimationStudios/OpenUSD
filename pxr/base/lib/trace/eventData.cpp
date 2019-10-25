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

#include "pxr/base/trace/eventData.h"

#include "pxr/pxr.h"
#include "pxr/base/js/json.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace {

// Boost variant visitor to convert TraceEventData to JsValue
class JsValue_visitor : public boost::static_visitor<void>
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

// Boost variant visitor to convert TraceEventData to TraceEvent::DataType
class Type_visitor : public boost::static_visitor<TraceEvent::DataType>
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
    return boost::apply_visitor(Type_visitor(), _data);
}

const int64_t* TraceEventData::GetInt() const
{
    return GetType() == TraceEvent::DataType::Int ?
        &boost::get<int64_t>(_data) : nullptr;
}

const uint64_t* TraceEventData::GetUInt() const
{
    return GetType() == TraceEvent::DataType::UInt ?
        &boost::get<uint64_t>(_data) : nullptr;
}

const double* TraceEventData::GetFloat() const
{
    return GetType() == TraceEvent::DataType::Float ?
        &boost::get<double>(_data) : nullptr;
}

const bool* TraceEventData::GetBool() const
{
    return GetType() == TraceEvent::DataType::Boolean ?
        &boost::get<bool>(_data) : nullptr;
}

const std::string* TraceEventData::GetString() const
{
    return GetType() == TraceEvent::DataType::String ?
        &boost::get<std::string>(_data) : nullptr;
}

void TraceEventData::WriteJson(JsWriter& writer) const
{
    boost::apply_visitor(JsValue_visitor(writer), _data);
}

PXR_NAMESPACE_CLOSE_SCOPE