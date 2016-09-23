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
/// \file js/value.cpp

#include "pxr/base/js/value.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"
#include <boost/noncopyable.hpp>
#include <boost/variant.hpp>

/// \struct Js_Null
/// A sentinel type held by default constructed JsValue objects, which
/// corresponds to JSON 'null'.
struct Js_Null
{
    bool operator==(const Js_Null& v) const {
        return true;
    }
    bool operator!=(const Js_Null& v) const {
        return false;
    }
};

/// \struct JsValue::_Holder
/// Private holder class used to abstract away how a value is stored
/// internally in JsValue objects.
struct JsValue::_Holder : private boost::noncopyable
{
    // The order these types are defined in the variant is important. See the
    // comments in the implementation of IsUInt64 for details.
    typedef boost::variant<
        boost::recursive_wrapper<JsObject>,
        boost::recursive_wrapper<JsArray>,
        std::string, bool, int64_t, double, Js_Null, uint64_t>
        Variant;

    _Holder()
        : value(Js_Null()), type(JsValue::NullType) { }
    _Holder(const JsObject& value)
        : value(value), type(JsValue::ObjectType) { }
    _Holder(const JsArray& value)
        : value(value), type(JsValue::ArrayType) { }
    _Holder(const char* value)
        : value(std::string(value)), type(JsValue::StringType) { }
    _Holder(const std::string& value)
        : value(value), type(JsValue::StringType) { }
    _Holder(bool value)
        : value(value), type(JsValue::BoolType) { }
    _Holder(int value)
        : value(static_cast<int64_t>(value)), type(JsValue::IntType) { }
    _Holder(int64_t value)
        : value(value), type(JsValue::IntType) { }
    _Holder(uint64_t value)
        : value(value), type(JsValue::IntType) { }
    _Holder(double value)
        : value(value), type(JsValue::RealType) { }

    Variant value;
    JsValue::Type type;
};

static std::string
_GetTypeName(const JsValue::Type& t)
{
    switch (t) {
    case JsValue::ObjectType: return "object";
    case JsValue::ArrayType:  return "array";
    case JsValue::StringType: return "string";
    case JsValue::BoolType:   return "bool";
    case JsValue::IntType:    return "int";
    case JsValue::RealType:   return "real";
    case JsValue::NullType:   return "null";
    default:                    return "unknown";
    };
}

JsValue::JsValue()
    : _holder(new _Holder)
{
    // Do Nothing.
}

JsValue::JsValue(const JsObject& value)
    : _holder(new _Holder(value))
{
    // Do Nothing.
}

JsValue::JsValue(const JsArray& value)
    : _holder(new _Holder(value))
{
    // Do Nothing.
}

JsValue::JsValue(const char* value)
    : _holder(new _Holder(value))
{
    // Do Nothing.
}

JsValue::JsValue(const std::string& value)
    : _holder(new _Holder(value))
{
    // Do Nothing.
}

JsValue::JsValue(bool value)
    : _holder(new _Holder(value))
{
    // Do Nothing.
}

JsValue::JsValue(int value)
    : _holder(new _Holder(value))
{
    // Do Nothing.
}

JsValue::JsValue(int64_t value)
    : _holder(new _Holder(value))
{
    // Do Nothing.
}

JsValue::JsValue(uint64_t value)
    : _holder(new _Holder(value))
{
    // Do Nothing.
}

JsValue::JsValue(double value)
    : _holder(new _Holder(value))
{
    // Do Nothing.
}

static bool
_CheckType(
    const JsValue::Type& heldType,
    const JsValue::Type& requestedType,
    std::string* whyNot)
{
    if (heldType != requestedType) {
        if (whyNot) {
            *whyNot = TfStringPrintf(
                "Attempt to get %s from value holding %s",
                _GetTypeName(requestedType).c_str(),
                _GetTypeName(heldType).c_str());
        }
        return false;
    }
    return true;
}

const JsObject&
JsValue::GetJsObject() const
{
    static TfStaticData<JsObject> _emptyObject;

    std::string whyNot;
    if (not _CheckType(_holder->type, ObjectType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        return *_emptyObject;
    }

    return *boost::get<JsObject>(&_holder->value);
}

const JsArray&
JsValue::GetJsArray() const
{
    static TfStaticData<JsArray> _emptyArray;

    std::string whyNot;
    if (not _CheckType(_holder->type, ArrayType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        return *_emptyArray;
    }

    return *boost::get<JsArray>(&_holder->value);
}

const std::string&
JsValue::GetString() const
{
    static TfStaticData<std::string> _emptyString;

    std::string whyNot;
    if (not _CheckType(_holder->type, StringType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        return *_emptyString;
    }

    return *boost::get<std::string>(&_holder->value);
}

bool
JsValue::GetBool() const
{
    std::string whyNot;
    if (not _CheckType(_holder->type, BoolType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        return false;
    }

    return boost::get<bool>(_holder->value);
}

int
JsValue::GetInt() const
{
    std::string whyNot;
    if (not _CheckType(_holder->type, IntType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        return 0;
    }

    return static_cast<int>(GetInt64());
}

int64_t
JsValue::GetInt64() const
{
    std::string whyNot;
    if (not _CheckType(_holder->type, IntType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        return 0;
    }

    if (IsUInt64())
        return static_cast<int64_t>(GetUInt64());

    return boost::get<int64_t>(_holder->value);
}

uint64_t
JsValue::GetUInt64() const
{
    std::string whyNot;
    if (not _CheckType(_holder->type, IntType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        return 0;
    }

    if (not IsUInt64())
        return static_cast<uint64_t>(GetInt64());

    return boost::get<uint64_t>(_holder->value);
}

double
JsValue::GetReal() const
{
    if (_holder->type == IntType) {
        return IsUInt64() ?
            static_cast<double>(GetUInt64()) :
            static_cast<double>(GetInt64());
    }

    std::string whyNot;
    if (not _CheckType(_holder->type, RealType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        return 0;
    }

    return boost::get<double>(_holder->value);
}

JsValue::Type
JsValue::GetType() const
{
    return _holder->type;
}

std::string
JsValue::GetTypeName() const
{
    return _GetTypeName(_holder->type);
}

bool
JsValue::IsObject() const
{
    return _holder->type == ObjectType;
}

bool
JsValue::IsArray() const
{
    return _holder->type == ArrayType;
}

bool
JsValue::IsString() const
{
    return _holder->type == StringType;
}

bool
JsValue::IsBool() const
{
    return _holder->type == BoolType;
}

bool
JsValue::IsInt() const
{
    return _holder->type == IntType;
}

bool
JsValue::IsReal() const
{
    return _holder->type == RealType;
}

bool
JsValue::IsUInt64() const
{
    // This relies on the position of Js_Null and uint64_t in the variant
    // type declaration, as well as the value of NullType in the Type enum.
    // This is how json_spirit itself determines whether the type is uint64_t.
    return _holder->value.which() == (NullType + 1);
}

bool
JsValue::IsNull() const
{
    return _holder->type == NullType;
}

JsValue::operator bool() const
{
    return not IsNull();
}

bool
JsValue::operator==(const JsValue& other) const
{
    return _holder->type == other._holder->type and
        _holder->value == other._holder->value;
}

bool
JsValue::operator!=(const JsValue& other) const
{
    return not (*this == other);
}
