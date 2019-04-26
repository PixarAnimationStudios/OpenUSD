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

#ifndef JS_JSON_H
#define JS_JSON_H

/// \file js/json.h
/// Top-level entrypoints for reading and writing JSON.

#include "pxr/pxr.h"
#include "pxr/base/js/api.h"
#include "pxr/base/js/value.h"

#include <iosfwd>
#include <string>

PXR_NAMESPACE_OPEN_SCOPE

/// \struct JsParseError
///
/// A struct containing information about a JSON parsing error.
///
struct JsParseError {
    JsParseError() : line(0), column(0) { }
    unsigned int line;
    unsigned int column;
    std::string reason;
};

/// Parse the contents of input stream \p istr and return a JsValue. On
/// failure, this returns a null JsValue.
JS_API
JsValue JsParseStream(std::istream& istr, JsParseError* error = 0);

/// Parse the contents of the JSON string \p data and return it as a JsValue.
/// On failure, this returns a null JsValue.
JS_API
JsValue JsParseString(const std::string& data, JsParseError* error = 0);

/// Convert the JsValue \p value to JSON and write the result to output stream
/// \p ostr.
JS_API
void JsWriteToStream(const JsValue& value, std::ostream& ostr);

/// Convert the JsValue \p value to JSON and return it as a string.
JS_API
std::string JsWriteToString(const JsValue& value);

/// \class JsWriter
///
/// This class provides an interface to writing json values directly to a
/// stream. This can be much more efficient than constructing a JsValue instance
/// and using JsWriteToStream if the data size is significant.
///
class JsWriter {
public:
    enum class Style {
        Compact,
        Pretty
    };

    /// Constructor. The lifetime of the /p ostr parameter is assumed to be
    /// longer than the JsWriter instance.
    JS_API JsWriter(std::ostream& ostr, Style style = Style::Compact);

    /// Destructor.
    JS_API ~JsWriter();

    /// Disable copies.
    JsWriter(const JsWriter&) = delete;
    JsWriter& operator=(const JsWriter&) = delete;

    /// Write a null value.
    JS_API bool WriteValue(std::nullptr_t);

    /// Write a boolean value.
    JS_API bool WriteValue(bool b);

    /// Write an integer value.
    JS_API bool WriteValue(int i);

    /// Write an unsigned integer value.
    JS_API bool WriteValue(unsigned u);

    /// Write a 64-bit integer value.
    JS_API bool WriteValue(int64_t i);

    /// Write a 64-bit unsigned integer value.
    JS_API bool WriteValue(uint64_t u);

    /// Write a double value.
    JS_API bool WriteValue(double d);

    /// Write a string value.
    JS_API bool WriteValue(const std::string& s);

    /// Write a string value.
    JS_API bool WriteValue(const char* s);

    /// Write a string value.
    template< size_t N>
    bool WriteValue(const char(&s)[N]) { return _String(s, N-1); }

    /// Write the start of an object.
    JS_API bool BeginObject();

    /// Write an object key.
    JS_API bool WriteKey(const std::string&);

    /// Write an object key.
    JS_API bool WriteKey(const char*);

    /// Write a string literal object key.
    template< size_t N>
    bool WriteKey(const char(&s)[N]) { return _Key(s, N-1); }

    /// Convenience function to write an object key and value.
    template<class K, class V>
    void WriteKeyValue(K&& k, V&& v) {
        _WriteObjectFields(std::forward<K>(k), std::forward<V>(v));
    }
    
    /// Write the end of an object.
    JS_API bool EndObject();

    /// Write the start of an array.
    JS_API bool BeginArray();

    /// Write the end of an array.
    JS_API bool EndArray();

    /// Convenience function to write an array of values.
    template <class Container>
    void WriteArray(const Container& c) {
        BeginArray();
        for (const auto& i : c) {
            WriteValue(i);
        }
        EndArray();
    }

    /// Convenience function to write an array of values by calling the given 
    /// functor for each item in the container.
    template <class Container, class ItemWriteFn>
    void WriteArray(const Container& c, const ItemWriteFn& f) {
        BeginArray();
        for (const auto& i : c) {
            f(*this, i);
        }
        EndArray();
    }

    /// Convenience function to write an array of values given two iterators by 
    /// calling the given functor for each item in the container.
    template <class Iterator, class ItemWriteFn>
    void WriteArray(
        const Iterator& begin, const Iterator& end, const ItemWriteFn& f) {
        BeginArray();
        for (Iterator i = begin; i != end; ++i) {
            f(*this, i);
        }
        EndArray();
    }

    /// Convenience function to write an object given key value pair arguments.
    /// key arguments must be convertable to strings, value argruments must be 
    /// either a writable type, or a callablable type taking a JsWriter&.
    template< class ...T>
    void WriteObject(T&&... f) {
        static_assert(sizeof...(T) %2 == 0,
            "Arguments must come in key value pairs");
        BeginObject();
        _WriteObjectFields(std::forward<T>(f)...);
        EndObject();
    }

private:
    // Don't want implicit casts to write functions, its better to get an error.
    template <class T>
    bool WriteValue(T) = delete;

    JS_API bool _String(const char* s, size_t len);
    JS_API bool _Key(const char* s, size_t len);

    template <class KeyType, class T>
    auto _WriteObjectFields(KeyType&& key, T&& v)
        -> decltype(WriteValue(std::forward<T>(v)), void()) {
        WriteKey(std::forward<KeyType>(key));
        WriteValue(std::forward<T>(v));
    }

    template <class KeyType, class T>
    auto _WriteObjectFields(KeyType&& key, T&& v)
        -> decltype(v(std::declval<JsWriter&>()), void()) {
        WriteKey(std::forward<KeyType>(key));
        v(*this);
    }

    template< class Key0, class T0, class ...T>
    void _WriteObjectFields(Key0&& key0, T0&& f0, T&&...f){
        _WriteObjectFields(std::forward<Key0>(key0), std::forward<T0>(f0));
        _WriteObjectFields(std::forward<T>(f)...);
    }

    class _Impl;
    std::unique_ptr<_Impl> _impl;
};

/// Write a json value.
JS_API void JsWriteValue(JsWriter* writer, const JsValue& value);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // JS_JSON_H
