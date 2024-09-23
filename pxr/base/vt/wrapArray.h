//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_VT_WRAP_ARRAY_H
#define PXR_BASE_VT_WRAP_ARRAY_H

#include "pxr/pxr.h"
#include "pxr/base/vt/api.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/pyOperators.h"

#include "pxr/base/arch/math.h"
#include "pxr/base/arch/inttypes.h"
#include "pxr/base/arch/pragmas.h"
#include "pxr/base/gf/half.h"
#include "pxr/base/gf/traits.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyObjWrapper.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/meta.h"
#include "pxr/base/tf/span.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/tf.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include "pxr/external/boost/python/class.hpp"
#include "pxr/external/boost/python/copy_const_reference.hpp"
#include "pxr/external/boost/python/def.hpp"
#include "pxr/external/boost/python/detail/api_placeholder.hpp"
#include "pxr/external/boost/python/extract.hpp"
#include "pxr/external/boost/python/implicit.hpp"
#include "pxr/external/boost/python/iterator.hpp"
#include "pxr/external/boost/python/make_constructor.hpp"
#include "pxr/external/boost/python/object.hpp"
#include "pxr/external/boost/python/operators.hpp"
#include "pxr/external/boost/python/return_arg.hpp"
#include "pxr/external/boost/python/slice.hpp"
#include "pxr/external/boost/python/type_id.hpp"
#include "pxr/external/boost/python/overloads.hpp"

#include <algorithm>
#include <numeric>
#include <ostream>
#include <string>
#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace Vt_WrapArray {

using namespace pxr_boost::python;

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
getitem_index(VtArray<T> const &self, int64_t idx)
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
    catch (std::invalid_argument const &) {
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
    catch (std::invalid_argument const &) {
        // Do nothing
        return;
    }

    // Get the number of items to be set.
    const size_t setSize = 1 + (range.stop - range.start) / range.step;

    // Copy from VtArray.  We only want to take this path if the passed value is
    // *exactly* a VtArray.  That is, we don't want to take this path if it can
    // merely *convert* to a VtArray, so we check that we can extract a mutable
    // lvalue reference from the python object, which requires that there be a
    // real VtArray there.
    if (extract< VtArray<T> &>(value).check()) {
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
setitem_index(VtArray<T> &self, int64_t idx, object value)
{
    idx = TfPyNormalizeIndex(idx, self.size(), /*throwError=*/true);
    setArraySlice(self, slice(idx, idx+1), value, /*tile=*/true);
}

template <typename T>
void
setitem_slice(VtArray<T> &self, slice idx, object value)
{
    setArraySlice(self, idx, value);
}


template <class T>
VT_API string GetVtArrayName();

template <class T, class... Ts>
constexpr bool Vt_IsAnySameImpl(TfMetaList<Ts...>) {
     return (std::is_same_v<T, Ts> || ...);
}

template <class T, class TypeList>
constexpr bool Vt_IsAnySame() {
    return Vt_IsAnySameImpl<T>(TypeList{});
}

// This is the same types as in VT_INTEGRAL_BUILTIN_VALUE_TYPES with char
// and bool types removed.
using Vt_OptimizedStreamIntegralTypes =
    TfMetaList<short, unsigned short,
               int, unsigned int,
               long, unsigned long,
               long long, unsigned long long>;

// Explicitly convert half to float here instead of relying on implicit
// conversion to float to work around the fact that libc++ only provides
// implementations of std::isfinite for types where std::is_arithmetic 
// is true.
template <typename T>
inline bool _IsFinite(T const &value) {
    return std::isfinite(value);
}
inline bool _IsFinite(GfHalf const &value) {
    return std::isfinite(static_cast<float>(value));
}

template <typename T>
static void streamValue(std::ostringstream &stream, T const &value) {
    // To avoid overhead we stream out certain builtin types directly
    // without calling TfPyRepr().
    if constexpr(Vt_IsAnySame<T, Vt_OptimizedStreamIntegralTypes>()) {
        stream << value;
    }
    // For float types we need to be make sure to represent infs and nans correctly.
    else if constexpr(GfIsFloatingPoint<T>::value) {
        if (_IsFinite(value)) {
            stream << value;
        }
        else {
            stream << TfPyRepr(value);
        }
    }
    else {
        stream << TfPyRepr(value);
    }
}

static unsigned int
Vt_ComputeEffectiveRankAndLastDimSize(
    Vt_ShapeData const *sd, size_t *lastDimSize)
{
    unsigned int rank = sd->GetRank();
    if (rank == 1)
        return rank;

    size_t divisor = std::accumulate(
        sd->otherDims, sd->otherDims + rank-1,
        1, [](size_t x, size_t y) { return x * y; });

    size_t remainder = divisor ? sd->totalSize % divisor : 0;
    *lastDimSize = divisor ? sd->totalSize / divisor : 0;
    
    if (remainder)
        rank = 1;

    return rank;
}

template <typename T>
string __repr__(VtArray<T> const &self)
{
    if (self.empty())
        return TF_PY_REPR_PREFIX +
            TfStringPrintf("%s()", GetVtArrayName<VtArray<T> >().c_str());

    std::ostringstream stream;
    stream.precision(17);
    stream << "(";
    for (size_t i = 0; i < self.size(); ++i) {
        stream << (i ? ", " : "");
        streamValue(stream, self[i]);
    }
    stream << (self.size() == 1 ? ",)" : ")");

    const std::string repr = TF_PY_REPR_PREFIX +
        TfStringPrintf("%s(%zd, %s)",
                       GetVtArrayName<VtArray<T> >().c_str(),
                       self.size(), stream.str().c_str());

    // XXX: This is to deal with legacy shaped arrays and should be removed
    // once all shaped arrays have been eliminated.
    // There is no nice way to make an eval()able __repr__ for shaped arrays
    // that preserves the shape information, so put it in <> to make it
    // clearly not eval()able. That has the advantage that, if somebody passes
    // the repr into eval(), it'll raise a SyntaxError that clearly points to
    // the beginning of the __repr__.
    Vt_ShapeData const *shapeData = self._GetShapeData();
    size_t lastDimSize = 0;
    unsigned int rank =
        Vt_ComputeEffectiveRankAndLastDimSize(shapeData, &lastDimSize);
    if (rank > 1) {
        std::string shapeStr = "(";
        for (size_t i = 0; i != rank-1; ++i) {
            shapeStr += TfStringPrintf(
                i ? ", %d" : "%d", shapeData->otherDims[i]);
        }
        shapeStr += TfStringPrintf(", %zu)", lastDimSize);
        return TfStringPrintf("<%s with shape %s>",
                              repr.c_str(), shapeStr.c_str());
    }

    return repr;
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
VtArray<T> *VtArray__init__2(size_t size, object const &values)
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

VTOPERATOR_WRAP(__add__,__radd__)
VTOPERATOR_WRAP_NONCOMM(__sub__,__rsub__)
VTOPERATOR_WRAP(__mul__,__rmul__)
VTOPERATOR_WRAP_NONCOMM(__div__,__rdiv__)
VTOPERATOR_WRAP_NONCOMM(__mod__,__rmod__)

ARCH_PRAGMA_POP
}

template <typename T>
static std::string _VtStr(T const &self)
{
    return TfStringify(self);
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
    
    auto selfCls = class_<This>(name.c_str(), docStr.c_str(), no_init)
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

        .def("__repr__", __repr__<Type>)

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

    // Wrap conversions from python sequences.
    TfPyContainerConversions::from_python_sequence<
        This,
        TfPyContainerConversions::
        variable_capacity_all_items_convertible_policy>();

    // Wrap implicit conversions from VtArray to TfSpan.
    implicitly_convertible<This, TfSpan<Type> >();
    implicitly_convertible<This, TfSpan<const Type> >();
}

template <class Array>
VtValue
Vt_ConvertFromPySequenceOrIter(TfPyObjWrapper const &obj)
{
    typedef typename Array::ElementType ElemType;
    TfPyLock lock;
    if (PySequence_Check(obj.ptr())) {
        Py_ssize_t len = PySequence_Length(obj.ptr());
        Array result(len);
        ElemType *elem = result.data();
        for (Py_ssize_t i = 0; i != len; ++i) {
            pxr_boost::python::handle<> h(PySequence_ITEM(obj.ptr(), i));
            if (!h) {
                if (PyErr_Occurred())
                    PyErr_Clear();
                return VtValue();
            }
            pxr_boost::python::extract<ElemType> e(h.get());
            if (!e.check())
                return VtValue();
            *elem++ = e();
        }
        return VtValue(result);
    } else if (PyIter_Check(obj.ptr())) {
        Array result;
        while (PyObject *item = PyIter_Next(obj.ptr())) {
            pxr_boost::python::handle<> h(item);
            if (!h) {
                if (PyErr_Occurred())
                    PyErr_Clear();
                return VtValue();
            }
            pxr_boost::python::extract<ElemType> e(h.get());
            if (!e.check())
                return VtValue();
            result.push_back(e());
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
        ret = Vt_ConvertFromPySequenceOrIter<T>(v.UncheckedGet<TfPyObjWrapper>());
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

#define VT_WRAP_ARRAY(unused, elem)          \
    VtWrapArray< VtArray< VT_TYPE(elem) > >();

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_VT_WRAP_ARRAY_H
