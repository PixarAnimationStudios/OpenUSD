//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_JS_CONVERTER_H
#define PXR_BASE_JS_CONVERTER_H

/// \file js/converter.h

#include "pxr/pxr.h"
#include "pxr/base/js/value.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

// Converts a \c JsValue \p value holding an \c int value to a \c ValueType
// holding an \c int64_t.
template <class ValueType, class MapType, bool UseInt64 = true>
struct Js_ValueToInt {
    static ValueType Apply(const JsValue& value) {
        return value.IsUInt64() ?
            ValueType(value.GetUInt64()) : ValueType(value.GetInt64());
    }
};

// Converts a \c JsValue \p value holding an \c int value to a \c ValueType
// holding an \c int.
template <class ValueType, class MapType>
struct Js_ValueToInt<ValueType, MapType, false>
{
    static ValueType Apply(const JsValue& value) {
        return ValueType(value.GetInt());
    }
};

/// \class JsValueTypeConverter
///
/// A helper class that can convert recursive JsValue structures to
/// identical structures using a different container type. The destination
/// container type is determined by the \c ValueType template parameter, while
/// the type to map objects to is determined by the \c MapType template
/// parameter.
///
/// It is expected that the class \c ValueType is default constructable. A
/// default constructed \c ValueType is used to represent JSON null. The value
/// type must also support construction from the fundamental bool, string,
/// real and integer types supported by JsValue.
///
/// JsArray values are converted to std::vector<ValueType>, and JsObject
/// values are converted to the MapType. MapType must have a value type of \c
/// ValueType, and support operator[] assignment.
///
/// If the \c UseInt64 template parameter is \c true (default), value types
/// converted from JsValue::IntType hold uint64_t or int64_t. If the parameter
/// is \c false, all IntType values are converted to int. Note that this may
/// cause truncation if the JsValue holds values too large to be stored in an
/// int on this platform.
///
template <class ValueType, class MapType, bool UseInt64 = true>
class JsValueTypeConverter
{
    typedef std::vector<ValueType> VectorType;
public:
    /// Converts the given \p value recursively to a structure using the value
    /// and map types specified by the \c ValueType and \c MapType class
    /// template parameters.
    static ValueType Convert(const JsValue& value) {
        return _ToValueType(value);
    }

private:
    /// Converts \p value to \c ValueType.
    static ValueType _ToValueType(const JsValue& value) {
        switch (value.GetType()) {
        case JsValue::ObjectType:
            return ValueType(_ObjectToMap(value.GetJsObject()));
        case JsValue::ArrayType:
            return ValueType(_ArrayToVector(value.GetJsArray()));
        case JsValue::BoolType:
            return ValueType(value.GetBool());
        case JsValue::StringType:
            return ValueType(value.GetString());
        case JsValue::RealType:
            return ValueType(value.GetReal());
        case JsValue::IntType:
            return Js_ValueToInt<ValueType, MapType, UseInt64>::Apply(value);
        case JsValue::NullType:
            return ValueType();
        default: {
            TF_CODING_ERROR("unknown value type");
            return ValueType();
            }
        }
    }

    /// Converts \p object to \c MapType.
    static MapType _ObjectToMap(const JsObject& object) {
        MapType result;
        for (const auto& p : object) {
            result[p.first] = _ToValueType(p.second);
        }
        return result;
    }

    /// Converts \p array to \c VectorType.
    static VectorType _ArrayToVector(const JsArray& array) {
        VectorType result;
        result.reserve(array.size());
        for (const auto& value : array) {
            result.push_back(_ToValueType(value));
        }
        return result;
    }
};

/// Returns \p value converted recursively to the template and map types given
/// by the \c ValueType and \p MapType parameters.
/// \see JsValueTypeConverter
template <class ValueType, class MapType>
ValueType JsConvertToContainerType(const JsValue& value) {
    return JsValueTypeConverter<ValueType, MapType>::Convert(value);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_JS_CONVERTER_H
