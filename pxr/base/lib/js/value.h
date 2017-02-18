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
#ifndef JS_VALUE_H
#define JS_VALUE_H

/// \file js/value.h

#include "pxr/pxr.h"
#include "pxr/base/js/api.h"
#include "pxr/base/js/types.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

// Value API Version
// 1 (or undefined) - Initial version.
// 2 - Changed Get{Array,Object} to GetJs{Array,Object}.
#define JS_VALUE_API_VERSION 2

/// \class JsValue
///
/// A discriminated union type for JSON values. A JsValue may contain one of
/// the following types:
///
/// \li JsObject, a dictionary type
/// \li JsArray, a vector type
/// \li std::string
/// \li bool
/// \li int64_t
/// \li uint64_t
/// \li double
/// \li null
///
class JsValue
{
public:
    /// Type held by this JSON value.
    enum Type {
        ObjectType,
        ArrayType,
        StringType,
        BoolType,
        IntType,
        RealType,
        NullType
    };

    /// Constructs a null value.
    JS_API JsValue();

    /// Constructs a value holding the given object.
    JS_API JsValue(const JsObject& value);

    /// Constructs a value holding the given array.
    JS_API JsValue(const JsArray& value);

    /// Constructs a value holding the given char array as a std::string.
    JS_API explicit JsValue(const char* value);

    /// Constructs a value holding the given std::string.
    JS_API explicit JsValue(const std::string& value);

    /// Constructs a value holding a bool.
    JS_API explicit JsValue(bool value);

    /// Constructs a value holding a signed integer.
    JS_API explicit JsValue(int value);

    /// Constructs a value holding a 64-bit signed integer.
    JS_API explicit JsValue(int64_t value);

    /// Constructs a value holding a 64-bit unsigned integer.
    JS_API explicit JsValue(uint64_t value);

    /// Constructs a value holding a double.
    JS_API explicit JsValue(double value);

    /// Returns the object held by this value. If this value is not holding an
    /// object, this method raises a coding error and an empty object is
    /// returned.
    JS_API const JsObject& GetJsObject() const;

    /// Returns the array held by this value. If this value is not holding an
    /// array, this method raises a coding error and an empty array is
    /// returned.
    JS_API const JsArray& GetJsArray() const;

    /// Returns the string held by this value. If this value is not holding a
    /// string, this method raises a coding error and an empty string is
    /// returned.
    JS_API const std::string& GetString() const;

    /// Returns the bool held by this value. If this value is not holding a
    /// bool, this method raises a coding error and false is returned.
    JS_API bool GetBool() const;

    /// Returns the integer held by this value. If this value is not holding
    /// an int, this method raises a coding error and zero is returned. If the
    /// value is holding a 64-bit integer larger than the platform int may
    /// hold, the value is truncated.
    JS_API int GetInt() const;

    /// Returns the 64-bit integer held by this value. If this value is not
    /// holding a 64-bit integer, this method raises a coding error and zero
    /// is returned.
    JS_API int64_t GetInt64() const;

    /// Returns the 64-bit unsigned integer held by this value. If this value
    /// is not holding a 64-bit unsigned integer, this method raises a coding
    /// error and zero is returned.
    JS_API uint64_t GetUInt64() const;

    /// Returns the double held by this value. If this value is not holding a
    /// double, this method raises a coding error and zero is returned.
    JS_API double GetReal() const;

    /// Returns the value corresponding to the C++ type specified in the
    /// template parameter if it is holding such a value. Calling this
    /// function with C++ type T is equivalent to calling the specific Get
    /// function above that returns a value or reference to a type T.
    ///
    /// If a value corresponding to the C++ type is not being held, this
    /// method raises a coding error. See Get functions above for default
    /// value returned in this case.
    template <typename T,
              typename ReturnType = typename std::conditional<
                  std::is_same<T, JsObject>::value || 
                  std::is_same<T, JsArray>::value || 
                  std::is_same<T, std::string>::value,
                  const T&, T>::type>
    ReturnType Get() const {
        return _Get(static_cast<T*>(nullptr));
    }

    /// Returns a vector holding the elements of this value's array that
    /// correspond to the C++ type specified as the template parameter. 
    /// If this value is not holding an array, an empty vector is returned. 
    /// If any of the array's elements does not correspond to the C++ type, 
    /// it is replaced with the default value used by the Get functions above. 
    /// In both cases, a coding error will be raised.
    template <typename T>
    std::vector<T> GetArrayOf() const;

    /// Returns the type of this value.
    JS_API Type GetType() const;

    /// Returns a display name for the type of this value.
    JS_API std::string GetTypeName() const;

    /// Returns true if this value is holding an object type.
    JS_API bool IsObject() const;

    /// Returns true if this value is holding an array type.
    JS_API bool IsArray() const;

    /// Returns true if this value is holding a string type.
    JS_API bool IsString() const;

    /// Returns true if this value is holding a boolean type.
    JS_API bool IsBool() const;

    /// Returns true if this value is holding an integer type.
    JS_API bool IsInt() const;

    /// Returns true if this value is holding a real type.
    JS_API bool IsReal() const;

    /// Returns true if this value is holding a 64-bit unsigned integer.
    JS_API bool IsUInt64() const;

    /// Returns true if this value is holding a type that corresponds
    /// to the C++ type specified as the template parameter.
    template <typename T>
    bool Is() const {
        return _Is(static_cast<T*>(nullptr));
    }

    /// Returns true if this value is holding an array whose elements all
    /// correspond to the C++ type specified as the template parameter.
    template <typename T>
    bool IsArrayOf() const;

    /// Returns true if this value is null, false otherwise.
    JS_API bool IsNull() const;

    /// Evaluates to true if this value is not null.
    JS_API explicit operator bool() const;

    /// Returns true of both values hold the same type and the underlying held
    /// values are equal.
    JS_API bool operator==(const JsValue& other) const;

    /// Returns true if values are of different type, or the underlying held
    /// values are not equal.
    JS_API bool operator!=(const JsValue& other) const;

private:
    template <typename T> 
    struct _InvalidTypeHelper : public std::false_type { };

    template <class T>
    T _Get(T*) const {
        static_assert(_InvalidTypeHelper<T>::value, 
                      "Invalid type for JsValue");
        return T();
    }

    const JsObject& _Get(JsObject*) const { return GetJsObject(); }
    const JsArray& _Get(JsArray*) const { return GetJsArray(); }
    const std::string& _Get(std::string*) const { return GetString(); }
    bool _Get(bool*) const { return GetBool(); }
    int _Get(int*) const { return GetInt(); }
    int64_t _Get(int64_t*) const { return GetInt64(); }
    uint64_t _Get(uint64_t*) const { return GetUInt64(); }
    double _Get(double*) const { return GetReal(); }

    template <class T>
    bool _Is(T*) const {
        static_assert(_InvalidTypeHelper<T>::value, 
                      "Invalid type for JsValue");
        return false;
    }

    bool _Is(JsObject*) const { return IsObject(); }
    bool _Is(JsArray*) const { return IsArray(); }
    bool _Is(std::string*) const { return IsString(); }
    bool _Is(bool*) const { return IsBool(); }
    bool _Is(int*) const { return IsInt(); }
    bool _Is(int64_t*) const { return IsInt(); }
    bool _Is(uint64_t*) const { return IsUInt64(); }
    bool _Is(double*) const { return IsReal(); }

    struct _Holder;
    std::shared_ptr<_Holder> _holder;
};

template <typename T>
inline std::vector<T> JsValue::GetArrayOf() const
{
    const JsArray& array = GetJsArray();
    std::vector<T> result(array.size());
    std::transform(array.begin(), array.end(), result.begin(),
                   [](const JsValue& v) { return v.Get<T>(); });
    return result;
}

template <typename T>
inline bool JsValue::IsArrayOf() const 
{
    if (!IsArray()) {
        return false;
    }
    const JsArray& array = GetJsArray();
    return std::all_of(array.begin(), array.end(),
                       [](const JsValue& v) { return v.Is<T>(); });
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // JS_VALUE_H
