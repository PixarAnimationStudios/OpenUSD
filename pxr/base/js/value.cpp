//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
///
/// \file js/value.cpp

#include "pxr/pxr.h"
#include "pxr/base/js/value.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/stringUtils.h"

#include <variant>

PXR_NAMESPACE_OPEN_SCOPE

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

// Wrapper around std::unique_ptr that should always be
// dereferencable and whose equality is determined by
// its held object. This is needed to make JsObject
// and JsArray (which hold JsValues) holdable by JsValue
template <typename T>
class Js_Wrapper final {
public:
    // Js_Wrapper should never by null, so disallow
    // defualt construction
    Js_Wrapper() = delete;

    // Copy semantics are not necessary as the _Holder
    // type is not copyable
    Js_Wrapper(const Js_Wrapper&) = delete;
    Js_Wrapper& operator=(const Js_Wrapper&) = delete;

    Js_Wrapper(Js_Wrapper&&) = default;
    Js_Wrapper& operator=(Js_Wrapper&&) = default;
    ~Js_Wrapper() = default;

    explicit Js_Wrapper(const T& wrapped) :
        _ptr(std::make_unique<T>(wrapped)) {}
    explicit Js_Wrapper(T&& wrapped) :
        _ptr(std::make_unique<T>(std::move(wrapped))) {}

    const T& Get() const { return *_ptr; }

    bool operator==(const Js_Wrapper& other) const {
        return Get() == other.Get();
    }

    bool operator!=(const Js_Wrapper& other) const {
        return Get() != other.Get();
    }

private:
    std::unique_ptr<T> _ptr;
};

using Js_ObjectWrapper = Js_Wrapper<JsObject>;
using Js_ArrayWrapper = Js_Wrapper<JsArray>;

/// \struct JsValue::_Holder
/// Private holder class used to abstract away how a value is stored
/// internally in JsValue objects.
struct JsValue::_Holder
{
    // The order these types are defined in the variant is important. See the
    // comments in the implementation of IsUInt64 for details.
    using Variant = std::variant<
        Js_ObjectWrapper,
        Js_ArrayWrapper,
        std::string, bool, int64_t, double, Js_Null, uint64_t>;

    _Holder()
        : value(Js_Null()), type(JsValue::NullType) { }
    _Holder(const JsObject& value)
        : value(Js_ObjectWrapper(value)),
          type(JsValue::ObjectType) { }
    _Holder(JsObject&& value)
        : value(Js_ObjectWrapper(std::move(value))),
          type(JsValue::ObjectType) { }
    _Holder(const JsArray& value)
        : value(Js_ArrayWrapper(value)),
          type(JsValue::ArrayType) { }
    _Holder(JsArray&& value)
        : value(Js_ArrayWrapper(std::move(value))),
          type(JsValue::ArrayType) { }
    _Holder(const char* value)
        : value(std::string(value)), type(JsValue::StringType) { }
    _Holder(const std::string& value)
        : value(value), type(JsValue::StringType) { }
    _Holder(std::string&& value)
        : value(std::move(value)), type(JsValue::StringType) { }
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

    // _Holder is not copyable
    _Holder(const _Holder&) = delete;
    _Holder& operator=(const _Holder&) = delete;

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

JsValue::JsValue(JsObject&& value)
    : _holder(new _Holder(std::move(value)))
{
    // Do Nothing.
}

JsValue::JsValue(const JsArray& value)
    : _holder(new _Holder(value))
{
    // Do Nothing.
}

JsValue::JsValue(JsArray&& value)
    : _holder(new _Holder(std::move(value)))
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

JsValue::JsValue(std::string&& value)
    : _holder(new _Holder(std::move(value)))
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
    if (!_CheckType(_holder->type, ObjectType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        return *_emptyObject;
    }

    return std::get<Js_ObjectWrapper>(_holder->value).Get();
}

const JsArray&
JsValue::GetJsArray() const
{
    static TfStaticData<JsArray> _emptyArray;

    std::string whyNot;
    if (!_CheckType(_holder->type, ArrayType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        return *_emptyArray;
    }

    return std::get<Js_ArrayWrapper>(_holder->value).Get();
}

const std::string&
JsValue::GetString() const
{
    static TfStaticData<std::string> _emptyString;

    std::string whyNot;
    if (!_CheckType(_holder->type, StringType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        return *_emptyString;
    }

    return std::get<std::string>(_holder->value);
}

bool
JsValue::GetBool() const
{
    std::string whyNot;
    if (!_CheckType(_holder->type, BoolType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        return false;
    }

    return std::get<bool>(_holder->value);
}

int
JsValue::GetInt() const
{
    std::string whyNot;
    if (!_CheckType(_holder->type, IntType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        return 0;
    }

    return static_cast<int>(GetInt64());
}

int64_t
JsValue::GetInt64() const
{
    std::string whyNot;
    if (!_CheckType(_holder->type, IntType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        return 0;
    }

    if (IsUInt64())
        return static_cast<int64_t>(GetUInt64());

    return std::get<int64_t>(_holder->value);
}

uint64_t
JsValue::GetUInt64() const
{
    std::string whyNot;
    if (!_CheckType(_holder->type, IntType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        return 0;
    }

    if (!IsUInt64())
        return static_cast<uint64_t>(GetInt64());

    return std::get<uint64_t>(_holder->value);
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
    if (!_CheckType(_holder->type, RealType, &whyNot)) {
        TF_CODING_ERROR(whyNot);
        return 0;
    }

    return std::get<double>(_holder->value);
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
    return _holder->value.index() == (NullType + 1);
}

bool
JsValue::IsNull() const
{
    return _holder->type == NullType;
}

JsValue::operator bool() const
{
    return !IsNull();
}

bool
JsValue::operator==(const JsValue& other) const
{
    return _holder->type == other._holder->type && 
        _holder->value == other._holder->value;
}

bool
JsValue::operator!=(const JsValue& other) const
{
    return !(*this == other);
}

PXR_NAMESPACE_CLOSE_SCOPE
