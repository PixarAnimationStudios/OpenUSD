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
#ifndef SDF_PARSER_HELPERS_H
#define SDF_PARSER_HELPERS_H

#include "pxr/pxr.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/base/arch/inttypes.h"

#include <boost/function.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/type_traits/is_floating_point.hpp>
#include <boost/type_traits/is_integral.hpp>
#include <boost/type_traits/is_signed.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/variant.hpp>

#include <climits>
#include <limits>
#include <map>
#include <string>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace Sdf_ParserHelpers {

using boost::variant;

using std::string;
using std::vector;
using std::map;

// Internal variant type.
typedef boost::variant<uint64_t, int64_t, double,
                       std::string, TfToken, SdfAssetPath> _Variant;

////////////////////////////////////////////////////////////////////////
// Utilities that implement the Sdf_ParserHelpers::Value::Get<T>() method.  The
// _GetImpl<T> class template provides the right implementation for T.  There
// are several partial specializations that provide the right behavior for
// various types.

// General Get case, requires exact match.
template <class T, class Enable = void>
struct _GetImpl
{
    typedef const T &ResultType;
    static const T &Visit(_Variant const &variant) {
        return boost::get<T>(variant);
    }
};

////////////////////////////////////////////////////////////////////////
// _GetImpl<T> for integral type T.  Convert finite doubles by static_cast,
// throw bad_get for non-finite doubles.  Throw bad_get for out-of-range
// integral values.
template <class T>
struct _GetImpl<
    T, typename boost::enable_if<boost::is_integral<T> >::type>
    : public boost::static_visitor<T>
{
    typedef T ResultType;

    T Visit(_Variant const &variant) {
        return boost::apply_visitor(*this, variant);
    }

    // Fallback case: throw bad_get.
    template <class Held>
    T operator()(Held held) { throw boost::bad_get(); }

    // Attempt to cast unsigned and signed int64_t.
    T operator()(uint64_t in) { return _Cast(in); }
    T operator()(int64_t in) { return _Cast(in); }

    // Attempt to cast finite doubles, throw otherwise.
    T operator()(double in) {
        if (std::isfinite(in))
            return _Cast(in);
        throw boost::bad_get();
    }

private:
    template <class In>
    T _Cast(In in) {
        try {
            return boost::numeric_cast<T>(in);
        } catch (const boost::bad_numeric_cast &) {
            throw boost::bad_get();
        }
    }
};
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// _GetImpl<T> for floating point type T.  Attempts to cast numeric values.
// Also handles strings like "inf", "-inf", and "nan" to produce +/- infinity
// and a quiet NaN.
template <class T>
struct _GetImpl<
    T, typename boost::enable_if<boost::is_floating_point<T> >::type>
    : public boost::static_visitor<T>
{
    typedef T ResultType;

    T Visit(_Variant const &variant) {
        return boost::apply_visitor(*this, variant);
    }

    // Fallback case: throw bad_get.
    template <class Held>
    T operator()(Held held) { throw boost::bad_get(); }

    // For numeric types, attempt to cast.
    T operator()(uint64_t in) { return _Cast(in); }
    T operator()(int64_t in) { return _Cast(in); }
    T operator()(double in) { return static_cast<T>(in); }
    
    // Convert special strings if possible.
    T operator()(const std::string &str) { return _FromString(str); }
    T operator()(const TfToken &tok) { return _FromString(tok.GetString()); }

private:
    T _FromString(const std::string &str) const {
        // Special case the strings 'inf', '-inf' and 'nan'.
        if (str == "inf") 
            return std::numeric_limits<T>::infinity();
        if (str == "-inf")
            return -std::numeric_limits<T>::infinity();
        if (str == "nan")
            return std::numeric_limits<T>::quiet_NaN();
        throw boost::bad_get();
    }

    template <class In>
    T _Cast(In in) {
        try {
            return boost::numeric_cast<T>(in);
        } catch (const boost::bad_numeric_cast &) {
            throw boost::bad_get();
        }
    }
};


////////////////////////////////////////////////////////////////////////

// Get an asset path: converts string to asset path, otherwise throw bad_get.
template <>
struct _GetImpl<SdfAssetPath>
{
    typedef SdfAssetPath ResultType;

    SdfAssetPath Visit(_Variant const &variant) {
        if (std::string const *str = boost::get<std::string>(&variant))
            return SdfAssetPath(*str);
        return boost::get<SdfAssetPath>(variant);
    }
};

// Get a bool.  Numbers are considered true if nonzero, false otherwise.
// Strings and tokens get parsed via SdfBoolFromString.  Otherwise throw
// bad_get.
template <>
struct _GetImpl<bool> : public boost::static_visitor<bool>
{
    typedef bool ResultType;
    
    bool Visit(_Variant const &variant) {
        return boost::apply_visitor(*this, variant);
    }

    // Parse string via SdfBoolFromString.
    bool operator()(const std::string &str) {
        bool parseOK = false;
        bool result = SdfBoolFromString(str, &parseOK);
        if (!parseOK)
            throw boost::bad_get();
        return result;
    }

    // Treat tokens as strings.
    bool operator()(const TfToken &tok) { return (*this)(tok.GetString()); }

    // For numbers, return true if not zero.
    template <class Number>
    typename boost::enable_if<boost::is_arithmetic<Number>, bool>::type
    operator()(Number val) {
        return val != static_cast<Number>(0);
    }

    // For anything else, throw bad_get().
    template <class T>
    typename boost::disable_if<boost::is_arithmetic<T>, bool>::type
    operator()(T) { throw boost::bad_get(); }
};

// A parser value.  This is used as the fundamental value object in the text
// parser.  It can hold one of a few different types: (u)int64_t, double,
// string, TfToken, and SdfAssetPath.  The lexer only ever produces Value
// objects holding (u)int64_t, double, and string.  The presence of TfToken and
// SdfAssetPath here are for a relatively obscure case where we're parsing a
// value whose type is unknown to the parser.  See
// StartRecordingString/StopRecordingString/IsRecordingString in
// parserValueContext.{cpp.h}.  We'd like to change this.
//
// Value's primary function is to provide a Get<T>() convenience API that
// handles appropriate conversions from the held types.  For example, it is okay
// to call Get<float>() on a Value that's holding an integral type, a double, or
// a string if that string's value is one of 'inf', '-inf', or 'nan'.  Similarly
// Get<bool>() works on numbers and strings like 'yes', 'no', 'on', 'off',
// 'true', 'false'.  If a Get<T>() call fails, it throws boost::bad_get, which
// the parser responds to and raises a parse error.
//
// The lexer constructs Value objects from input tokens.  It creates them to
// retain all the input information possible.  For example, negative integers
// are stored as int64_t Values, positive numbers are stored as uint64_t values,
// and so on.  As a special case of this, '-0' is stored as a double, since it
// is the only way to preserve a signed zero (integral types have no signed
// zero).
struct Value
{
    // Default constructor leaves the value in an undefined state.
    Value() {}
    
    // Construct and implicitly convert from an integral type \p Int.  If \p Int
    // is signed, the resulting value holds an 'int64_t' internally.  If \p Int
    // is unsigned, the result value holds an 'uint64_t'.
    template <class Int>
    Value(Int in, typename boost::enable_if<
          boost::is_integral<Int> >::type * = 0) {
        if (boost::is_signed<Int>::value) {
            _variant = static_cast<int64_t>(in);
        } else {
            _variant = static_cast<uint64_t>(in);
        }
    }
    
    // Construct and implicitly convert from a floating point type \p Flt.  The
    // resulting value holds a double internally.
    template <class Flt>
    Value(Flt in, typename boost::enable_if<
          boost::is_floating_point<Flt> >::type * = 0) :
        _variant(static_cast<double>(in)) {}
    
    // Construct and implicitly convert from std::string.
    Value(const std::string &in) : _variant(in) {}

    // Construct and implicitly convert from TfToken.
    Value(const TfToken &in) : _variant(in) {}

    // Construct and implicitly convert from SdfAssetPath.
    Value(const SdfAssetPath &in) : _variant(in) {}
    
    // Attempt to get a value of type T from this Value, applying appropriate
    // conversions.  If this value cannot be converted to T, throw
    // boost::bad_get.
    template <class T>
    typename _GetImpl<T>::ResultType Get() const {
        return _GetImpl<T>().Visit(_variant);
    }

    // Hopefully short-lived API that applies an external visitor to the held
    // variant type.
    template <class Visitor>
    typename Visitor::result_type
    ApplyVisitor(const Visitor &visitor) {
        return boost::apply_visitor(visitor, _variant);
    }

    template <class Visitor>
    typename Visitor::result_type
    ApplyVisitor(Visitor &visitor) {
        return boost::apply_visitor(visitor, _variant);
    }

    template <class Visitor>
    typename Visitor::result_type
    ApplyVisitor(const Visitor &visitor) const {
        return _variant.apply_visitor(visitor);
    }

    template <class Visitor>
    typename Visitor::result_type
    ApplyVisitor(Visitor &visitor) const {
        return _variant.apply_visitor(visitor);
    }

private:
    _Variant _variant;
};

typedef boost::function<VtValue (vector<unsigned int> const &,
                                 vector<Value> const &,
                                 size_t &, string *)> ValueFactoryFunc;

struct ValueFactory {
    ValueFactory() {}

    ValueFactory(std::string typeName_, SdfTupleDimensions dimensions_,
                 bool isShaped_, ValueFactoryFunc func_)
        : typeName(typeName_),
          dimensions(dimensions_),
          isShaped(isShaped_),
          func(func_) {}

    std::string typeName;
    SdfTupleDimensions dimensions;
    bool isShaped;
    ValueFactoryFunc func;
};

ValueFactory const &GetValueFactoryForMenvaName(std::string const &name,
                                                bool *found);
};

// Read the quoted string at [x..x+n], trimming 'trimBothSides' number
// of chars from either side, and evaluating any embedded escaped characters.
// If numLines is given, it will be populated with the number of newline
// characters present in the original string.
std::string Sdf_EvalQuotedString(const char* x, size_t n, size_t trimBothSides,
                                 unsigned int* numLines = NULL);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // SDF_PARSER_HELPERS_H
