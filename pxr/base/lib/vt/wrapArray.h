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
#ifndef VT_WRAP_ARRAY_H
#define VT_WRAP_ARRAY_H

#include "pxr/pxr.h"
#include "pxr/base/vt/api.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/pyOperators.h"
#include "pxr/base/vt/functions.h"

#include "pxr/base/arch/math.h"
#include "pxr/base/arch/inttypes.h"
#include "pxr/base/arch/pragmas.h"
#include "pxr/base/gf/half.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyObjWrapper.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/tf.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <boost/preprocessor.hpp>

#include <boost/python/class.hpp>
#include <boost/python/copy_const_reference.hpp>
#include <boost/python/def.hpp>
#include <boost/python/detail/api_placeholder.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/iterator.hpp>
#include <boost/python/make_constructor.hpp>
#include <boost/python/object.hpp>
#include <boost/python/operators.hpp>
#include <boost/python/return_arg.hpp>
#include <boost/python/slice.hpp>
#include <boost/python/type_id.hpp>
#include <boost/python/overloads.hpp>

#include <algorithm>
#include <ostream>
#include <string>
#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace Vt_WrapArray {

using namespace boost::python;

using std::unique_ptr;
using std::vector;
using std::string;

template <typename T>
object
getitem_ellipsis(VtArray<T> const &self, object idx)
{
    object ellipsis = object(handle<>(borrowed(Py_Ellipsis)));
    if (idx != ellipsis) {
        PyErr_SetString(PyExc_TypeError, "unsupported index type");
        throw_error_already_set();
    }
    return object(self);
}

template <typename T>
object
getitem_index(VtArray<T> const &self, int idx)
{
    static const bool throwError = true;
    idx = TfPyNormalizeIndex(idx, self.size(), throwError);
    return object(self[idx]);
}

template <typename T>
object
getitem_slice(VtArray<T> const &self, slice idx)
{
    try {
        slice::range<typename VtArray<T>::const_iterator> range =
            idx.get_indices(self.begin(), self.end());
        const size_t setSize = 1 + (range.stop - range.start) / range.step;
        VtArray<T> result(setSize);
        size_t i = 0;
        for (; range.start != range.stop; range.start += range.step, ++i) {
            result[i] = *range.start;
        }
        result[i] = *range.start;
        return object(result);
    }
    catch (std::invalid_argument) {
        return object();
    }
}

template <typename T, typename S>
void
setArraySlice(VtArray<T> &self, S value,
              slice::range<T*>& range, size_t setSize, bool tile = false)
{
    // Check size.
    const size_t length = len(value);
    if (length == 0)
        TfPyThrowValueError("No values with which to set array slice.");
    if (!tile && length < setSize) {
        string msg = TfStringPrintf
            ("Not enough values to set slice.  Expected %zu, got %zu.",
             setSize, length);
        TfPyThrowValueError(msg);
    }

    // Extract the values before setting any.  If we can extract the
    // whole vector at once then do that since it should be faster.
    std::vector<T> extracted;
    extract<std::vector<T> > vectorExtraction(value);
    if (vectorExtraction.check()) {
        std::vector<T> tmp = vectorExtraction();
        extracted.swap(tmp);
    }
    else {
        extracted.reserve(length);
        for (size_t i = 0; i != length; ++i) {
            extracted.push_back(extract<T>(value[i]));
        }
    }

    // We're fine, go through and set them.  Handle common case as a fast
    // path.
    if (range.step == 1 && length >= setSize) {
        std::copy(extracted.begin(), extracted.begin() + setSize, range.start);
    }
    else {
        for (size_t i = 0; i != setSize; range.start += range.step, ++i) {
            *range.start = extracted[i % length];
        }
    }
}

template <typename T>
void
setArraySlice(VtArray<T> &self, slice idx, object value, bool tile = false)
{
    // Get the range.
    slice::range<T*> range;
    try {
        T* data = self.data();
        range = idx.get_indices(data, data + self.size());
    }
    catch (std::invalid_argument) {
        // Do nothing
        return;
    }

    // Get the number of items to be set.
    const size_t setSize = 1 + (range.stop - range.start) / range.step;

    // Copy from VtArray.
    if (extract< VtArray<T> >(value).check()) {
        const VtArray<T> val = extract< VtArray<T> >(value);
        const size_t length = val.size();
        if (length == 0)
            TfPyThrowValueError("No values with which to set array slice.");
        if (!tile && length < setSize) {
            string msg = TfStringPrintf
                ("Not enough values to set slice.  Expected %zu, got %zu.",
                 setSize, length);
            TfPyThrowValueError(msg);
        }

        // We're fine, go through and set them.
        for (size_t i = 0; i != setSize; range.start += range.step, ++i) {
            *range.start = val[i % length];
        }
    }

    // Copy from scalar.
    else if (extract<T>(value).check()) {
        if (!tile) {
            // XXX -- We're allowing implicit tiling;  do we want to?
            //TfPyThrowValueError("can only assign an iterable.");
        }

        // Use scalar to fill entire slice.
        const T val = extract<T>(value);
        for (size_t i = 0; i != setSize; range.start += range.step, ++i) {
            *range.start = val;
        }
    }

    // Copy from list.
    else if (extract<list>(value).check()) {
        setArraySlice(self, extract<list>(value)(), range, setSize, tile);
    }

    // Copy from tuple.
    else if (extract<tuple>(value).check()) {
        setArraySlice(self, extract<tuple>(value)(), range, setSize, tile);
    }

    // Copy from iterable.
    else {
        setArraySlice(self, list(value), range, setSize, tile);
    }
}


template <typename T>
void
setitem_ellipsis(VtArray<T> &self, object idx, object value)
{
    object ellipsis = object(handle<>(borrowed(Py_Ellipsis)));
    if (idx != ellipsis) {
        PyErr_SetString(PyExc_TypeError, "unsupported index type");
        throw_error_already_set();
    }
    setArraySlice(self, slice(0, self.size()), value);
}

template <typename T>
void
setitem_index(VtArray<T> &self, int idx, object value)
{
    static const bool tile = true;
    setArraySlice(self, slice(idx, idx + 1), value, tile);
}

template <typename T>
void
setitem_slice(VtArray<T> &self, slice idx, object value)
{
    setArraySlice(self, idx, value);
}


template <class T>
VT_API string GetVtArrayName();


// To avoid overhead we stream out certain builtin types directly
// without calling TfPyRepr().
template <typename T>
static void streamValue(std::ostringstream &stream, T const &value) {
    stream << TfPyRepr(value);
}

// This is the same types as in VT_INTEGRAL_BUILTIN_VALUE_TYPES with char
// and bool types removed.
#define _OPTIMIZED_STREAM_INTEGRAL_TYPES \
    (short)                              \
    (unsigned short)                     \
    (int)                                \
    (unsigned int)                       \
    (long)                               \
    (unsigned long)                      \
    (long long)                          \
    (unsigned long long)

#define MAKE_STREAM_FUNC(r, unused, type)                    \
static inline void                                           \
streamValue(std::ostringstream &stream, type const &value) { \
    stream << value;                                         \
}
BOOST_PP_SEQ_FOR_EACH(MAKE_STREAM_FUNC, ~, _OPTIMIZED_STREAM_INTEGRAL_TYPES)
#undef MAKE_STREAM_FUNC
#undef _OPTIMIZED_STREAM_INTEGRAL_TYPES

// Explicitly convert half to float here instead of relying on implicit
// convesion to float to work around the fact that libc++ only provides 
// implementations of std::isfinite for types where std::is_arithmetic 
// is true.
template <typename T>
static bool _IsFinite(T const &value) {
    return std::isfinite(value);
}
static bool _IsFinite(GfHalf const &value) {
    return std::isfinite(static_cast<float>(value));
}

// For float types we need to be make sure to represent infs and nans correctly.
#define MAKE_STREAM_FUNC(r, unused, elem)                             \
static inline void                                                    \
streamValue(std::ostringstream &stream, VT_TYPE(elem) const &value) { \
    if (_IsFinite(value)) {                                           \
        stream << value;                                              \
    } else {                                                          \
        stream << TfPyRepr(value);                                    \
    }                                                                 \
}
BOOST_PP_SEQ_FOR_EACH(
    MAKE_STREAM_FUNC, ~, VT_FLOATING_POINT_BUILTIN_VALUE_TYPES)
#undef MAKE_STREAM_FUNC

template <typename T>
string __repr__(VtArray<T> const &self, const std::vector<int>* shape)
{
    if (self.empty())
        return TF_PY_REPR_PREFIX +
            TfStringPrintf("%s()", GetVtArrayName<VtArray<T> >().c_str());

    string shapeStr;
    if (shape) {
        shapeStr = "(";
        for (size_t i = 0; i < shape->size(); ++i)
            shapeStr += TfStringPrintf(i ? ", %d" : "%d", (*shape)[i]);
        shapeStr += shape->size() == 1 ? ",), " : "), ";
    }
    else {
        shapeStr = TfStringPrintf("%zd, ", self.size());
    }

    std::ostringstream stream;
    stream.precision(17);
    stream << "(";
    for (size_t i = 0; i < self.size(); ++i) {
        stream << (i ? ", " : "");
        streamValue(stream, self[i]);
    }
    stream << (self.size() == 1 ? ",)" : ")");
    
    return TF_PY_REPR_PREFIX +
        TfStringPrintf("%s(%s%s)",
                       GetVtArrayName<VtArray<T> >().c_str(),
                       shapeStr.c_str(), stream.str().c_str());
}

template <typename T>
string __repr1__(VtArray<T> const &self)
{
    return __repr__(self, NULL);
}

template <typename T>
string __repr2__(VtArray<T> const &self, const std::vector<int>& shape)
{
    return __repr__(self, &shape);
}

template <typename T>
VtArray<T> *VtArray__init__(object const &values)
{
    // Make an array.
    unique_ptr<VtArray<T> > ret(new VtArray<T>(len(values)));

    // Set the values.  This is equivalent to saying 'ret[...] = values'
    // in python, except that we allow tiling here.
    static const bool tile = true;
    setArraySlice(*ret, slice(0, ret->size()), values, tile);
    return ret.release();
}
template <typename T>
VtArray<T> *VtArray__init__2(unsigned int size, object const &values)
{
    // Make the array.
    unique_ptr<VtArray<T> > ret(new VtArray<T>(size));

    // Set the values.  This is equivalent to saying 'ret[...] = values'
    // in python, except that we allow tiling here.
    static const bool tile = true;
    setArraySlice(*ret, slice(0, ret->size()), values, tile);

    return ret.release();
}

// overloading for operator special methods, to allow tuple / list & array
// combinations
ARCH_PRAGMA_PUSH
ARCH_PRAGMA_UNSAFE_USE_OF_BOOL
ARCH_PRAGMA_UNARY_MINUS_ON_UNSIGNED
VTOPERATOR_WRAP(+,__add__,__radd__)
VTOPERATOR_WRAP_NONCOMM(-,__sub__,__rsub__)
VTOPERATOR_WRAP(*,__mul__,__rmul__)
VTOPERATOR_WRAP_NONCOMM(/,__div__,__rdiv__)
VTOPERATOR_WRAP_NONCOMM(%,__mod__,__rmod__)

VTOPERATOR_WRAP_BOOL(Equal,==)
VTOPERATOR_WRAP_BOOL(NotEqual,!=)
VTOPERATOR_WRAP_BOOL(Greater,>)
VTOPERATOR_WRAP_BOOL(Less,<)
VTOPERATOR_WRAP_BOOL(GreaterOrEqual,>=)
VTOPERATOR_WRAP_BOOL(LessOrEqual,<=)
ARCH_PRAGMA_POP
}

template <typename T>
static std::string _VtStr(T const &self)
{
    return boost::lexical_cast<std::string>(self);
}

template <typename T>
void VtWrapArray()
{
    using namespace Vt_WrapArray;
    
    typedef T This;
    typedef typename This::ElementType Type;

    string name = GetVtArrayName<This>();
    string typeStr = ArchGetDemangled(typeid(Type));
    string docStr = TfStringPrintf("An array of type %s.", typeStr.c_str());
    
    class_<This>(name.c_str(), docStr.c_str(), no_init)
        .setattr("_isVtArray", true)
        .def(TfTypePythonClass())
        .def(init<>())
        .def("__init__", make_constructor(VtArray__init__<Type>),
            (const char *)
            "__init__(values)\n\n"
            "values: a sequence (tuple, list, or another VtArray with "
            "element type convertible to the new array's element type)\n\n"
            )
        .def("__init__", make_constructor(VtArray__init__2<Type>))
        .def(init<unsigned int>())

        .def("__getitem__", getitem_ellipsis<Type>)
        .def("__getitem__", getitem_slice<Type>)
        .def("__getitem__", getitem_index<Type>)
        .def("__setitem__", setitem_ellipsis<Type>)
        .def("__setitem__", setitem_slice<Type>)
        .def("__setitem__", setitem_index<Type>)

        .def("__len__", &This::size)
        .def("__iter__", iterator<This>())

        .def("__repr__", __repr1__<Type>)
        .def("__repr2__", __repr2__<Type>)

//        .def(str(self))
        .def("__str__", _VtStr<T>)
        .def(self == self)
        .def(self != self)

#ifdef NUMERIC_OPERATORS
#define ADDITION_OPERATOR
#define SUBTRACTION_OPERATOR
#define MULTIPLICATION_OPERATOR
#define DIVISION_OPERATOR
#define UNARY_NEG_OPERATOR
#endif

#ifdef ADDITION_OPERATOR
        VTOPERATOR_WRAPDECLARE(+,__add__,__radd__)
#endif
#ifdef SUBTRACTION_OPERATOR
        VTOPERATOR_WRAPDECLARE(-,__sub__,__rsub__)
#endif
#ifdef MULTIPLICATION_OPERATOR
        VTOPERATOR_WRAPDECLARE(*,__mul__,__rmul__)
#endif
#ifdef DIVISION_OPERATOR
        VTOPERATOR_WRAPDECLARE(/,__div__,__rdiv__)
#endif
#ifdef MOD_OPERATOR
        VTOPERATOR_WRAPDECLARE(%,__mod__,__rmod__)
#endif
#ifdef DOUBLE_MULT_OPERATOR
        .def(self * double())
        .def(double() * self)
#endif
#ifdef DOUBLE_DIV_OPERATOR
        .def(self / double())
#endif
#ifdef UNARY_NEG_OPERATOR
        .def(- self)
#endif

        ;

#define WRITE(z, n, data) BOOST_PP_COMMA_IF(n) data
#define VtCat_DEF(z, n, unused) \
    def("Cat",(VtArray<Type> (*)( BOOST_PP_REPEAT(n, WRITE, VtArray<Type> const &) ))VtCat<Type>);
    BOOST_PP_REPEAT_FROM_TO(1, VT_FUNCTIONS_MAX_ARGS, VtCat_DEF, ~)
#undef VtCat_DEF

    VTOPERATOR_WRAPDECLARE_BOOL(Equal)
    VTOPERATOR_WRAPDECLARE_BOOL(NotEqual)
}

// wrapping for functions that work for base types that support comparisons
template <typename T>
void VtWrapComparisonFunctions()
{
    using namespace Vt_WrapArray;
    
    typedef T This;
    typedef typename This::ElementType Type;

    def("AnyTrue", VtAnyTrue<Type>);
    def("AllTrue", VtAllTrue<Type>);

    VTOPERATOR_WRAPDECLARE_BOOL(Greater)
    VTOPERATOR_WRAPDECLARE_BOOL(Less)
    VTOPERATOR_WRAPDECLARE_BOOL(GreaterOrEqual)
    VTOPERATOR_WRAPDECLARE_BOOL(LessOrEqual)
}

template <class Array>
VtValue
Vt_ConvertFromPySequence(TfPyObjWrapper const &obj)
{
    typedef typename Array::ElementType ElemType;
    TfPyLock lock;
    if (PySequence_Check(obj.ptr())) {
        Py_ssize_t len = PySequence_Length(obj.ptr());
        Array result(len);
        ElemType *elem = result.data();
        for (size_t i = 0; i != len; ++i) {
            boost::python::handle<> h(PySequence_ITEM(obj.ptr(), i));
            if (!h) {
                if (PyErr_Occurred())
                    PyErr_Clear();
                return VtValue();
            }
            boost::python::extract<ElemType> e(h.get());
            if (!e.check())
                return VtValue();
            *elem++ = e();
        }
        return VtValue(result);
    }
    return VtValue();
}

template <class Array, class Iter>
VtValue
Vt_ConvertFromRange(Iter begin, Iter end)
{
    typedef typename Array::ElementType ElemType;
    Array result(distance(begin, end));
    for (ElemType *e = result.data(); begin != end; ++begin) {
        VtValue cast = VtValue::Cast<ElemType>(*begin);
        if (cast.IsEmpty())
            return cast;
        cast.Swap(*e++);
    }
    return VtValue(result);
}

template <class T>
VtValue
Vt_CastToArray(VtValue const &v) {
    VtValue ret;
    TfPyObjWrapper obj;
    // Attempt to convert from either python sequence or vector<VtValue>.
    if (v.IsHolding<TfPyObjWrapper>()) {
        ret = Vt_ConvertFromPySequence<T>(v.UncheckedGet<TfPyObjWrapper>());
    } else if (v.IsHolding<std::vector<VtValue> >()) {
        std::vector<VtValue> const &vec = v.UncheckedGet<std::vector<VtValue> >();
        ret = Vt_ConvertFromRange<T>(vec.begin(), vec.end());
    }
    return ret;
}

/// Register casts with VtValue from python sequences to VtArray types.
template <class Elem>
void VtRegisterValueCastsFromPythonSequencesToArray()
{
    typedef VtArray<Elem> Array;
    VtValue::RegisterCast<TfPyObjWrapper, Array>(Vt_CastToArray<Array>);
    VtValue::RegisterCast<std::vector<VtValue>, Array>(Vt_CastToArray<Array>);
}

#define VT_WRAP_ARRAY(r, unused, elem)          \
    VtWrapArray< VtArray< VT_TYPE(elem) > >();
#define VT_WRAP_COMPARISON(r, unused, elem)        \
    VtWrapComparisonFunctions< VtArray< VT_TYPE(elem) > >();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // VT_WRAP_ARRAY_H
