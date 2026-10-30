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
#include "pxr/base/vt/valueFromPython.h"

#include "pxr/base/arch/math.h"
#include "pxr/base/arch/inttypes.h"
#include "pxr/base/arch/pragmas.h"
#include "pxr/base/gf/half.h"
#include "pxr/base/gf/traits.h"
#include "pxr/base/tf/pyContainerConversions.h"
#include "pxr/base/tf/pyFunction.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include "pxr/base/tf/pyObjWrapper.h"
#include "pxr/base/tf/pyResultConversions.h"
#include "pxr/base/tf/pyUtils.h"
#include "pxr/base/tf/iterator.h"
#include "pxr/base/tf/meta.h"
#include "pxr/base/tf/span.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/tf.h"
#include "pxr/base/tf/wrapTypeHelpers.h"

#include <nanobind/make_iterator.h>
#include <nanobind/nanobind.h>
#include <nanobind/operators.h>

#include <algorithm>
#include <numeric>
#include <ostream>
#include <string>
#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace Vt_WrapArray {

using std::unique_ptr;
using std::vector;
using std::string;

template <typename Iter>
struct Vt_SliceRange {
    Iter start;
    Iter stop;
    Py_ssize_t step;
};

template <typename Iter>
bool
Vt_GetSliceIndices(nanobind::slice const &idx, Iter begin, Iter end,
                   Vt_SliceRange<Iter> *rangeOut, size_t *setSizeOut)
{
    Py_ssize_t start = 0, stop = 0, step = 0;
    if (PySlice_Unpack(idx.ptr(), &start, &stop, &step) < 0) {
        if (PyErr_ExceptionMatches(PyExc_ValueError)) {
            PyErr_SetString(PyExc_IndexError, "step size cannot be zero.");
        }
        throw nanobind::python_error();
    }
    const Py_ssize_t len = end - begin;
    const size_t setSize = PySlice_AdjustIndices(len, &start, &stop, step);
    if (setSize == 0) {
        return false;
    }
    rangeOut->start = begin + start;
    rangeOut->stop =
        begin + start + (static_cast<Py_ssize_t>(setSize) - 1) * step;
    rangeOut->step = step;
    *setSizeOut = setSize;
    return true;
}

template <typename T>
bool
Vt_TryExtractElement(nanobind::handle h, T *out, bool throwOnOverflow = false)
{
    if constexpr (std::is_same_v<T, bool>) {
        if (h.is_none() || PyBool_Check(h.ptr()) || PyLong_Check(h.ptr())) {
            int isTrue = PyObject_IsTrue(h.ptr());
            if (isTrue < 0) {
                PyErr_Clear();
                return false;
            }
            if (out) {
                *out = static_cast<bool>(isTrue);
            }
            return true;
        }
        return false;
    } else {
        nanobind::detail::make_caster<T> caster;
        nanobind::detail::cleanup_list cleanup(h.ptr());
        bool ok = caster.from_python(
            h, static_cast<uint8_t>(nanobind::detail::cast_flags::convert),
            &cleanup);
        if (ok) {
            if (out) {
                *out = static_cast<T>(
                    caster.operator nanobind::detail::cast_t<T>());
            }
            cleanup.release();
            return true;
        }
        cleanup.release();
        if (PyErr_Occurred()) {
            PyErr_Clear();
        }
        if constexpr (std::is_integral_v<T> && !std::is_same_v<T, char>) {
            if (throwOnOverflow && PyLong_Check(h.ptr())) {
                PyErr_SetString(PyExc_OverflowError,
                                "bad integer conversion: overflow");
                throw nanobind::python_error();
            }
        }
        return false;
    }
}

template <typename T>
T
Vt_ExtractElementOrThrow(nanobind::handle h)
{
    T val{};
    if (!Vt_TryExtractElement<T>(h, &val, /*throwOnOverflow=*/true)) {
        PyErr_SetString(PyExc_TypeError, "Element is of incorrect type.");
        throw nanobind::python_error();
    }
    return val;
}

} // namespace Vt_WrapArray

PXR_NAMESPACE_CLOSE_SCOPE

#include "pxr/base/vt/pyOperators.h"

PXR_NAMESPACE_OPEN_SCOPE

namespace Vt_WrapArray {

template <typename T>
nanobind::object
getitem_ellipsis(VtArray<T> const &self, nanobind::object idx)
{
    if (idx.ptr() != Py_Ellipsis) {
        PyErr_SetString(PyExc_TypeError, "unsupported index type");
        throw nanobind::python_error();
    }
    return nanobind::cast(self);
}

template <typename T>
nanobind::object
getitem_index(VtArray<T> const &self, int64_t idx)
{
    static const bool throwError = true;
    idx = TfPyNormalizeIndex(idx, self.size(), throwError);
    return nanobind::cast(self[idx]);
}

template <typename T>
nanobind::object
getitem_slice(VtArray<T> const &self, nanobind::slice idx)
{
    Vt_SliceRange<typename VtArray<T>::const_iterator> range;
    size_t setSize = 0;
    if (!Vt_GetSliceIndices(idx, self.begin(), self.end(), &range, &setSize)) {
        return nanobind::none();
    }
    VtArray<T> result(setSize);
    size_t i = 0;
    for (; range.start != range.stop; range.start += range.step, ++i) {
        result[i] = *range.start;
    }
    result[i] = *range.start;
    return nanobind::cast(result);
}

template <typename T, typename S>
void
setArraySlice(VtArray<T> &self, S value,
              Vt_SliceRange<T*>& range, size_t setSize, bool tile = false)
{
    // Check size.
    const size_t length = nanobind::len(value);
    if (length == 0)
        TfPyThrowValueError("No values with which to set array slice.");
    if (!tile && length < setSize) {
        string msg = TfStringPrintf
            ("Not enough values to set slice.  Expected %zu, got %zu.",
             setSize, length);
        TfPyThrowValueError(msg);
    }

    // Extract the values before setting any.
    std::vector<T> extracted;
    extracted.reserve(length);
    for (size_t i = 0; i != length; ++i) {
        extracted.push_back(Vt_ExtractElementOrThrow<T>(value[i]));
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
setArraySlice(VtArray<T> &self, nanobind::slice idx, nanobind::handle value,
              bool tile = false)
{
    // Get the range.
    Vt_SliceRange<T*> range;
    size_t setSize = 0;
    T* data = self.data();
    if (!Vt_GetSliceIndices(idx, data, data + self.size(), &range, &setSize)) {
        return;
    }

    // Copy from VtArray.  We only want to take this path if the passed value is
    // *exactly* a VtArray.  That is, we don't want to take this path if it can
    // merely *convert* to a VtArray, so we check that we can extract a mutable
    // lvalue reference from the python object, which requires that there be a
    // real VtArray there.
    VtArray<T> *arrPtr = nullptr;
    if (NB_CALL(nb_type_get)(NB_CTX, &typeid(VtArray<T>), value.ptr(), 0,
                             nullptr, reinterpret_cast<void **>(&arrPtr)) &&
        arrPtr) {
        const VtArray<T> val = *arrPtr;
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
        return;
    }

    // Copy from scalar.
    T scalarVal{};
    if (Vt_TryExtractElement<T>(value, &scalarVal, /*throwOnOverflow=*/true)) {
        if (!tile) {
            // XXX -- We're allowing implicit tiling;  do we want to?
            //TfPyThrowValueError("can only assign an iterable.");
        }

        // Use scalar to fill entire slice.
        for (size_t i = 0; i != setSize; range.start += range.step, ++i) {
            *range.start = scalarVal;
        }
    }

    // Copy from list.
    else if (PyList_Check(value.ptr())) {
        setArraySlice(self, nanobind::borrow<nanobind::list>(value),
                      range, setSize, tile);
    }

    // Copy from tuple.
    else if (PyTuple_Check(value.ptr())) {
        setArraySlice(self, nanobind::borrow<nanobind::tuple>(value),
                      range, setSize, tile);
    }

    // Copy from iterable.
    else {
        PyObject *pyList = PySequence_List(value.ptr());
        if (!pyList) {
            throw nanobind::python_error();
        }
        setArraySlice(self, nanobind::steal<nanobind::list>(pyList),
                      range, setSize, tile);
    }
}


template <typename T>
void
setitem_ellipsis(VtArray<T> &self, nanobind::object idx, nanobind::object value)
{
    if (idx.ptr() != Py_Ellipsis) {
        PyErr_SetString(PyExc_TypeError, "unsupported index type");
        throw nanobind::python_error();
    }
    setArraySlice(self, nanobind::slice(size_t(0), self.size()), value);
}

template <typename T>
void
setitem_index(VtArray<T> &self, int64_t idx, nanobind::object value)
{
    idx = TfPyNormalizeIndex(idx, self.size(), /*throwError=*/true);
    setArraySlice(self, nanobind::slice(idx, idx+1), value, /*tile=*/true);
}

template <typename T>
void
setitem_slice(VtArray<T> &self, nanobind::slice idx, nanobind::object value)
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

VT_API
unsigned int
Vt_ComputeEffectiveRankAndLastDimSize(
    Vt_ShapeData const *sd, size_t *lastDimSize);

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
void VtArray__init__(VtArray<T> *self, nanobind::object const &values)
{
    new (self) VtArray<T>(nanobind::len(values));
    static const bool tile = true;
    setArraySlice(*self, nanobind::slice(size_t(0), self->size()), values, tile);
}

template <typename T>
void VtArray__init__2(VtArray<T> *self, size_t size, nanobind::object const &values)
{
    new (self) VtArray<T>(size);
    static const bool tile = true;
    setArraySlice(*self, nanobind::slice(size_t(0), self->size()), values, tile);
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
VTOPERATOR_WRAP_NONCOMM(__truediv__,__rtruediv__)
VTOPERATOR_WRAP_NONCOMM(__mod__,__rmod__)

ARCH_PRAGMA_POP
}

template <typename T>
static std::string _VtStr(T const &self)
{
    return TfStringify(self);
}

template <typename T>
nanobind::class_<T> VtWrapArray()
{
    using namespace Vt_WrapArray;

    typedef T This;
    typedef typename This::ElementType Type;

    string name = GetVtArrayName<This>();
    string typeStr = ArchGetDemangled(typeid(Type));
    string docStr = TfStringPrintf("An array of type %s.", typeStr.c_str());

    nanobind::module_ m = TfPyGetCurrentNanobindModule();
    nanobind::class_<This> selfCls(m, name.c_str(), docStr.c_str());
    selfCls.attr("_isVtArray") = true;
    selfCls
        .def(TfTypePythonClass())
        .def(nanobind::init<>())
        .def(nanobind::init<unsigned int>())
        .def("__init__", VtArray__init__<Type>,
            "__init__(values)\n\n"
            "values: a sequence (tuple, list, or another VtArray with "
            "element type convertible to the new array's element type)\n\n"
            )
        .def("__init__", VtArray__init__2<Type>)

        .def("__getitem__", getitem_index<Type>)
        .def("__getitem__", getitem_slice<Type>)
        .def("__getitem__", getitem_ellipsis<Type>)
        .def("__setitem__", setitem_index<Type>)
        .def("__setitem__", setitem_slice<Type>)
        .def("__setitem__", setitem_ellipsis<Type>)

        .def("__len__", &This::size)
        .def("__iter__", [](This const &self) {
            return nanobind::make_iterator<nanobind::rv_policy::copy>(
                nanobind::type<This>(), "Iterator", self.cbegin(), self.cend());
        }, nanobind::keep_alive<0, 1>())

        .def("__repr__", __repr__<Type>)

        .def("__str__", _VtStr<T>)
        .def(nanobind::self == nanobind::self)
        .def(nanobind::self != nanobind::self)

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
        VTOPERATOR_WRAPDECLARE(/,__truediv__,__rtruediv__)
        .def("__div__", __div__tuple<Type>, nanobind::is_operator())
        .def("__div__", __div__list<Type>, nanobind::is_operator())
        .def("__rdiv__", __rdiv__tuple<Type>, nanobind::is_operator())
        .def("__rdiv__", __rdiv__list<Type>, nanobind::is_operator())
#endif
#ifdef MOD_OPERATOR
        VTOPERATOR_WRAPDECLARE(%,__mod__,__rmod__)
#endif
#ifdef DOUBLE_MULT_OPERATOR
        .def(nanobind::self * double())
        .def(double() * nanobind::self)
#endif
#ifdef DOUBLE_DIV_OPERATOR
        .def(nanobind::self / double())
#endif
#ifdef UNARY_NEG_OPERATOR
        .def(- nanobind::self)
#endif

        ;

    return selfCls;
}

template <class Array>
VtValue
Vt_ConvertFromPySequenceOrIter(TfPyObjWrapper const &obj)
{
    typedef typename Array::ElementType ElemType;
    TfPyLock lock;
    if (PySequence_Check(obj.ptr())) {
        Py_ssize_t len = PySequence_Length(obj.ptr());
        if (len < 0) {
            PyErr_Clear();
            return VtValue();
        }
        Array result(len);
        ElemType *elem = result.data();
        for (Py_ssize_t i = 0; i != len; ++i) {
            nanobind::object item = nanobind::steal(PySequence_ITEM(obj.ptr(), i));
            if (!item.ptr()) {
                if (PyErr_Occurred())
                    PyErr_Clear();
                return VtValue();
            }
            if (!Vt_WrapArray::Vt_TryExtractElement<ElemType>(item, elem))
                return VtValue();
            ++elem;
        }
        return VtValue(result);
    } else if (PyIter_Check(obj.ptr())) {
        Array result;
        while (PyObject *rawItem = PyIter_Next(obj.ptr())) {
            nanobind::object item = nanobind::steal(rawItem);
            ElemType elem{};
            if (!Vt_WrapArray::Vt_TryExtractElement<ElemType>(item, &elem))
                return VtValue();
            result.push_back(elem);
        }
        if (PyErr_Occurred()) {
            PyErr_Clear();
            return VtValue();
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
