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

#include "pxr/pxr.h"
#include "pxr/base/vt/api.h"
#include "pxr/base/vt/arrayPyBuffer.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/types.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/vt/value.h"
#include "pxr/base/vt/wrapArray.h"

#include "pxr/base/gf/traits.h"

#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyUtils.h"

#include <boost/preprocessor.hpp>
#include <boost/python.hpp>

#include <numeric>
#include <string>
#include <type_traits>
#include <vector>

using std::string;
using std::vector;

PXR_NAMESPACE_OPEN_SCOPE

////////////////////////////////////////////////////////////////////////
// Producer side: Implement the buffer protocol on VtArrays.
namespace {

namespace bp = boost::python;

////////////////////////////////////////////////////////////////////////
// Element sub-type.  e.g. GfVec3f -> float.
template <class T, class Enable = void>
struct Vt_GetSubElementType { typedef T Type; };

template <class T>
struct Vt_GetSubElementType<
    T, typename std::enable_if<GfIsGfVec<T>::value ||
                               GfIsGfMatrix<T>::value ||
                               GfIsGfQuat<T>::value ||
                               GfIsGfRange<T>::value>::type> {
    typedef typename T::ScalarType Type;
};

template <>
struct Vt_GetSubElementType<GfRect2i> { typedef int Type; };

////////////////////////////////////////////////////////////////////////
// Format strings.
template <class T> constexpr char Vt_FmtFor();
template <> constexpr char Vt_FmtFor<bool>() { return '?'; }
template <> constexpr char Vt_FmtFor<char>() { return 'b'; }
template <> constexpr char Vt_FmtFor<unsigned char>() { return 'B'; }
template <> constexpr char Vt_FmtFor<short>() { return 'h'; }
template <> constexpr char Vt_FmtFor<unsigned short>() { return 'H'; }
template <> constexpr char Vt_FmtFor<int>() { return 'i'; }
template <> constexpr char Vt_FmtFor<unsigned int>() { return 'I'; }
template <> constexpr char Vt_FmtFor<long>() { return 'l'; }
template <> constexpr char Vt_FmtFor<unsigned long>() { return 'L'; }
template <> constexpr char Vt_FmtFor<long long>() { return 'q'; }
template <> constexpr char Vt_FmtFor<unsigned long long>() { return 'Q'; }
template <> constexpr char Vt_FmtFor<GfHalf>() { return 'e'; }
template <> constexpr char Vt_FmtFor<float>() { return 'f'; }
template <> constexpr char Vt_FmtFor<double>() { return 'd'; }

template <class Src, class Dst>
inline Dst
Vt_ConvertSingle(void const *src) {
    return static_cast<Dst>(*static_cast<Src const *>(src));
}

template <class Dst>
using Vt_ConvertFn = Dst (*)(void const *);

template <class Dst>
Vt_ConvertFn<Dst>
Vt_GetConvertFn(char srcFmt)
{
    switch (srcFmt) {
    case '?': return Vt_ConvertSingle<bool, Dst>;
    case 'b': return Vt_ConvertSingle<char, Dst>;
    case 'B': return Vt_ConvertSingle<unsigned char, Dst>;
    case 'h': return Vt_ConvertSingle<short, Dst>;
    case 'H': return Vt_ConvertSingle<unsigned short, Dst>;
    case 'i': return Vt_ConvertSingle<int, Dst>;
    case 'I': return Vt_ConvertSingle<unsigned int, Dst>;
    case 'l': return Vt_ConvertSingle<long, Dst>;
    case 'L': return Vt_ConvertSingle<unsigned long, Dst>;
    case 'q': return Vt_ConvertSingle<long long, Dst>;
    case 'Q': return Vt_ConvertSingle<unsigned long long, Dst>;
    case 'e': return Vt_ConvertSingle<GfHalf, Dst>;
    case 'f': return Vt_ConvertSingle<float, Dst>;
    case 'd': return Vt_ConvertSingle<double, Dst>;
    }
    return nullptr;
}

template <class T>
struct Vt_FormatStr {
    static char *Get() { return str; }
    static char str[2];
};
template <class T>
char Vt_FormatStr<T>::str[2] = {
    Vt_FmtFor<typename Vt_GetSubElementType<T>::Type>(), '\0' };


////////////////////////////////////////////////////////////////////////
// Element intrinsic shape.  e.g. GfVec3f -> 1x3.
template <size_t N>
using Vt_PyShape = std::array<Py_ssize_t, N>;

constexpr Vt_PyShape<0> Vt_GetElementShapeImpl(...) { return {}; }

template <class T>
constexpr typename std::enable_if<GfIsGfVec<T>::value, Vt_PyShape<1> >::type
Vt_GetElementShapeImpl(T *) { return {{ T::dimension }}; }

template <class T>
constexpr typename std::enable_if<GfIsGfMatrix<T>::value, Vt_PyShape<2> >::type
Vt_GetElementShapeImpl(T *) { return {{ T::numRows, T::numColumns }}; }

template <class T>
constexpr typename std::enable_if<GfIsGfQuat<T>::value, Vt_PyShape<1> >::type
Vt_GetElementShapeImpl(T *) { return {{ 4 }}; }

template <class T>
constexpr typename std::enable_if<
    GfIsGfRange<T>::value && T::dimension == 1, Vt_PyShape<1> >::type
Vt_GetElementShapeImpl(T *) { return {{ 2 }}; }

template <class T>
constexpr typename std::enable_if<
    GfIsGfRange<T>::value && T::dimension != 1, Vt_PyShape<2> >::type
Vt_GetElementShapeImpl(T *) { return {{ 2, T::dimension }}; }

constexpr Vt_PyShape<2>
Vt_GetElementShapeImpl(GfRect2i *) { return {{ 2, 2 }}; }

template <class T>
constexpr auto Vt_GetElementShape() ->
    decltype(Vt_GetElementShapeImpl(static_cast<T *>(nullptr)))
{
    return Vt_GetElementShapeImpl(static_cast<T *>(nullptr));
}

////////////////////////////////////////////////////////////////////////
// Array wrapper object -- we allocate one of these when we get a buffer
// request.  It holds a copy of the VtArray, plus shape and stride arrays.  We
// store a pointer to the allocated wrapper in the Py_buffer object and delete
// it on 'releasebuffer'.
template <class Array>
struct Vt_ArrayBufferWrapper
{
    using value_type = typename Array::value_type;
    using SubElementType = typename Vt_GetSubElementType<value_type>::Type;

    explicit Vt_ArrayBufferWrapper(Array const &array) : array(array) {
        // First element of shape is overall length.  Other elements are filled
        // from the array's value_type's intrinsic shape (e.g. an array of
        // GfMatrix3f's will add two additional dimensions).
        shape[0] = array.size();
        auto elemShape = Vt_GetElementShape<value_type>();
        std::copy(elemShape.begin(), elemShape.end(), shape.begin()+1);

        // The last element of the strides array is always the size of the
        // sub-element type.  E.g. for GfVec3d, it's sizeof(double).  The other
        // elements, in reverse order multiply by the shape in that dimension.
        // For example, the shape and strides for a VtArray<GfMatrix3f> size=11
        // would look like:
        //   shape   = [11,          3,        3]
        //   strides = [36 (=12*3), 12 (=4*3), 4 (=sizeof(float))]
        strides[strides.size()-1] = sizeof(SubElementType);
        for (size_t i = strides.size()-1; i; --i) {
            strides[i-1] = strides[i] * shape[i];
        }
    }

    void MakeWritable() {
        // Invoke the .data() method on array.  As a side effect this will
        // detach it from any shared storage, ensuring that any modifications
        // will not affect other VtArray instances.
        (void)array.data();
    }

    Array array;

    Vt_PyShape<Vt_GetElementShape<value_type>().size() + 1> shape;
    Vt_PyShape<Vt_GetElementShape<value_type>().size() + 1> strides;
};

////////////////////////////////////////////////////////////////////////
// Python buffer protocol entry points.

// Python's getreadbuf interface function.
template <class T>
Py_ssize_t
Vt_getreadbuf(PyObject *self, Py_ssize_t segment, void **ptrptr) {
    if (segment != 0) {
        // Always one-segment for arrays.
        PyErr_SetString(PyExc_ValueError, "accessed non-existent segment");
        return -1;
    }
    T &selfT = bp::extract<T &>(self);
    *ptrptr = static_cast<void *>(selfT.data());
    // Return size in bytes.
    return selfT.size() * sizeof(typename T::value_type);
}

// Python's getwritebuf interface function.
template <class T>
Py_ssize_t
Vt_getwritebuf(PyObject *self, Py_ssize_t segment, void **ptrptr) {
    PyErr_SetString(PyExc_ValueError, "writable buffers supported only with "
                    "new-style buffer protocol.");
    return -1;
}

// Python's getsegcount interface function.
template <class T>
Py_ssize_t
Vt_getsegcount(PyObject *self, Py_ssize_t *lenp) {
    T &selfT = bp::extract<T &>(self);
    if (lenp)
        *lenp = selfT.size() * sizeof(typename T::value_type);
    return 1; // Always one contiguous segment.
}

// Python's getcharbuf interface function.
template <class T>
Py_ssize_t
Vt_getcharbuf(PyObject *self, Py_ssize_t segment, const char **ptrptr) {
    return Vt_getreadbuf<T>(self, segment, (void **) ptrptr);
}

// Python's releasebuffer interface function.
template <class T>
void
Vt_releasebuffer(PyObject *self, Py_buffer *view) {
    delete static_cast<Vt_ArrayBufferWrapper<T> *>(view->internal);
}

// Python's getbuffer interface function.
template <class T>
int
Vt_getbuffer(PyObject *self, Py_buffer *view, int flags)
{
    using value_type = typename T::value_type;

    if (view == NULL) {
        PyErr_SetString(PyExc_ValueError, "NULL view in getbuffer");
        return -1;
    }

    // We don't support fortran order.
    if ((flags & PyBUF_F_CONTIGUOUS) == PyBUF_F_CONTIGUOUS) {
        PyErr_SetString(PyExc_ValueError, "Fortran contiguity unsupported");
        return -1;
    }

    T &array = bp::extract<T &>(self);
    auto wrapper = std::unique_ptr<Vt_ArrayBufferWrapper<T>>(
        new Vt_ArrayBufferWrapper<T>(array));

    bool writable = ((flags & PyBUF_WRITABLE) == PyBUF_WRITABLE);

    if (writable)
        wrapper->MakeWritable();

    view->obj = self;
    view->buf = static_cast<void *>(wrapper->array.data());
    view->len = wrapper->array.size() * sizeof(value_type);
    view->readonly = static_cast<int>(!writable);
    view->itemsize = sizeof(typename Vt_GetSubElementType<value_type>::Type);
    if ((flags & PyBUF_FORMAT) == PyBUF_FORMAT) {
        view->format = Vt_FormatStr<value_type>::Get();
    } else {
        view->format = NULL;
    }
    if ((flags & PyBUF_ND) == PyBUF_ND) {
        view->ndim = wrapper->shape.size();
        view->shape = wrapper->shape.data();
    } else {
        view->ndim = 0;
        view->shape = NULL;
    }
    if ((flags & PyBUF_STRIDES) == PyBUF_STRIDES) {
        view->strides = wrapper->strides.data();
    } else {
        view->strides = NULL;
    }
    view->suboffsets = NULL;
    view->internal = static_cast<void *>(wrapper.release());

    Py_INCREF(self); // need to retain a reference to self.
    return 0;
}

// This structure serves to instantiate a PyBufferProcs instance with pointers
// to the right buffer protocol functions.
template <class T>
struct Vt_ArrayBufferProcs
{
    static PyBufferProcs procs;
};
template <class T>
PyBufferProcs Vt_ArrayBufferProcs<T>::procs = {
    (readbufferproc) Vt_getreadbuf<T>,   /*bf_getreadbuffer*/
    (writebufferproc) Vt_getwritebuf<T>, /*bf_getwritebuffer*/
    (segcountproc) Vt_getsegcount<T>,    /*bf_getsegcount*/
    (charbufferproc) Vt_getcharbuf<T>,   /*bf_getcharbuffer*/
    (getbufferproc) Vt_getbuffer<T>,
    (releasebufferproc) Vt_releasebuffer<T>,
};

////////////////////////////////////////////////////////////////////////
// Top-level protocol install functions.
template <class ArrayType>
void
Vt_AddBufferProtocol()
{
    TfPyLock lock;

    // Look up the python class object, then set its fields to point at our
    // buffer protocol implementation.
    bp::object cls = TfPyGetClassObject<ArrayType>();
    if (TfPyIsNone(cls)) {
        TF_CODING_ERROR("Failed to find python class object for '%s'",
                        ArchGetDemangled<ArrayType>().c_str());
        return;
    }

    // Set the tp_as_buffer slot to point to a structure of function pointers
    // that implement the buffer protocol for this type, and set the type flags
    // to indicate that this type supports the buffer protocol.
    auto *typeObj = reinterpret_cast<PyTypeObject *>(cls.ptr());
    typeObj->tp_as_buffer = &Vt_ArrayBufferProcs<ArrayType>::procs;
    typeObj->tp_flags |= (Py_TPFLAGS_HAVE_NEWBUFFER |
                          Py_TPFLAGS_HAVE_GETCHARBUFFER);
}


////////////////////////////////////////////////////////////////////////
// Consumer-side: make VtArrays from python objects that provide the buffer
// protocol.
template <class T>
bool
Vt_ArrayFromBuffer(TfPyObjWrapper const &obj,
                   VtArray<T> *out, string *errPtr = nullptr)
{
    string localErr;
    auto &err = errPtr ? *errPtr : localErr;

    TfPyLock lock;

    if (!PyObject_CheckBuffer(obj.ptr())) {
        err = "Python object does not support the buffer protocol";
        return false;
    }

    // Request a strided buffer with type & dimensions.
    Py_buffer view;
    memset(&view, 0, sizeof(view));
    if (PyObject_GetBuffer(
            obj.ptr(), &view, PyBUF_FORMAT | PyBUF_STRIDES) != 0) {
        err = "Failed to get dimensioned, typed buffer";
        return false;
    }

    // We have a buffer.  Check that the type matches.
    if (!view.format ||
        view.format[0] == '>' ||  
        view.format[0] == '!' || 
        view.format[0] == '=' || 
        view.format[0] == '^') {
        err = TfStringPrintf("Unsupported format '%s'",
                             view.format ? view.format : "<null>");
        PyBuffer_Release(&view);
        return false;
    }

    // Check that the number of elements matches.
    auto multiply = [](Py_ssize_t x, Py_ssize_t y) { return x * y; };
    auto numItems = std::accumulate(
        view.shape, view.shape + view.ndim, 1, multiply);

    // Compute the total # of items in one element.
    auto elemShape = Vt_GetElementShape<T>();
    auto elemSize = std::accumulate(
        elemShape.begin(), elemShape.end(), 1, multiply);

    // Sanity check data sizes.
    auto arraySize = numItems / elemSize;

    // Check that the element shape evenly divides the items in the buffer and
    // that the byte sizes match.
    if (numItems % elemSize) {
        err = TfStringPrintf("Buffer size (%s items) must be a multiple of %s",
                             TfStringify(numItems).c_str(),
                             TfStringify(elemSize).c_str());
        PyBuffer_Release(&view);
        return false;
    }

    // Try to convert.
    char const *desiredFmt = Vt_FormatStr<T>::Get();

    char typeChar = '\0';
    char const *p = view.format;
    if (*p == '<' || *p == '@')
        ++p;
    typeChar = *p;

    bool isConvertible = false;

    using SubType = typename Vt_GetSubElementType<T>::Type;
    auto convertFn = Vt_GetConvertFn<SubType>(typeChar);

    if (convertFn) {
        isConvertible = true;

        if (out) {
            out->resize(arraySize);

            // Copy the contents to out.  An element at 'index' is located at:
            // buf + index[0] * strides[0] + ... + index[n-1] * strides[n-1].
            // The bulk of the code here is to manage that index.  The lambda
            // 'increment' increments an index, and 'getPtrAt' returns a pointer
            // to the start of the data item at the given index.

            // Use a stack-based index storage if possible, otherwise overflow
            // to the heap.
            Py_ssize_t localIdx[8];
            std::unique_ptr<Py_ssize_t []> overflowIdx;
            Py_ssize_t *index = localIdx;
            if (view.ndim > std::extent<decltype(localIdx)>::value) {
                overflowIdx.reset(new Py_ssize_t[view.ndim]);
                index = overflowIdx.get();
            }
            // Start at the first index.
            memset(index, 0, sizeof(*index) * view.ndim);

            // Helper function to increment an index.
            auto increment = [&view](Py_ssize_t *index) {
                auto ndim = view.ndim;
                for (int i = 0; i != view.ndim; ++i) {
                    if (++index[ndim-i-1] < view.shape[ndim-i-1])
                        return;
                    index[ndim-i-1] = 0;
                }
            };

            // Helper function to calculate an element pointer given an index.
            auto getPtrAt = [&view](Py_ssize_t *index) {
                auto i = view.ndim;
                char *result = static_cast<char *>(view.buf);
                while (i--)
                    result += index[i] * view.strides[i];
                return result;
            };

            // Roll through, converting elements.
            auto outPtr = reinterpret_cast<SubType *>(out->data());
            while (numItems--) {
                *outPtr++ = convertFn(getPtrAt(index));
                increment(index);
            }
        }

    } else {
        err = TfStringPrintf("No known conversion from format %c to %c",
                             typeChar, desiredFmt[0]);
    }

    PyBuffer_Release(&view);
    return isConvertible;
}



template <class T>
static VtValue Vt_CastPyObjToArray(VtValue const &v)
{
    VtValue ret;
    TfPyObjWrapper obj;
    if (v.IsHolding<TfPyObjWrapper>())
        obj = v.UncheckedGet<TfPyObjWrapper>();

    // Attempt to produce the requested VtArray.
    VtArray<T> array;
    if (Vt_ArrayFromBuffer(obj, &array)) {
        ret.Swap(array);
    }
    else {
        ret = Vt_ConvertFromPySequence<VtArray<T>>(obj);
    }

    return ret;
}

template <class T>
static VtValue Vt_CastVectorToArray(VtValue const &v) {
    VtValue ret;
    if (v.IsHolding<vector<VtValue>>()) {
        // This is a bit unfortunate.  We convert back to python, attempt to get
        // a python list, and then attempt to convert each element in it.
        VtArray<T> result;
        TfPyLock lock;
        try {
            auto obj = TfPyObject(v);
            bp::list pylist = bp::extract<bp::list>(obj);
            size_t len = bp::len(pylist);
            result.reserve(len);
            for (size_t i = 0; i != len; ++i) {
                bp::object item = pylist[i];
                bp::extract<T> e(item);
                if (e.check()) {
                    result.push_back(e());
                } else {
                    VtValue val = bp::extract<VtValue>(item);
                    if (val.Cast<T>().template IsHolding<T>()) {
                        result.push_back(val.UncheckedGet<T>());
                    } else {
                        TfPyThrowValueError(TfStringPrintf(
                            "Failed to produce an element of type '%s'",
                            ArchGetDemangled<T>().c_str()));
                    }
                }
            }
            ret.Swap(result);
        } catch (bp::error_already_set const &) {
            // swallow the exception and just fail the cast.
            PyErr_Clear();
        }
    }
    return ret;
}

template <class T>
TfPyObjWrapper
Vt_WrapArrayFromBuffer(TfPyObjWrapper const &obj)
{
    VtArray<T> result;
    string err;
    if (Vt_ArrayFromBuffer(obj, &result, &err)) {
        return bp::object(result);
    }
    TfPyThrowValueError(
        TfStringPrintf("Failed to produce VtArray<%s> via python buffer "
                       "protocol: %s",
                       ArchGetDemangled<T>().c_str(), err.c_str()));
    return TfPyObjWrapper();
}

} // anon

template <class T>
boost::optional<VtArray<T> >
VtArrayFromPyBuffer(TfPyObjWrapper const &obj, std::string *err)
{
    VtArray<T> array;
    boost::optional<VtArray<T> > result;
    if (Vt_ArrayFromBuffer(obj, &array, err))
        result = array;
    return result;
}

#define INSTANTIATE(r, unused, elem)                                       \
template boost::optional<VtArray<VT_TYPE(elem)> >                          \
VtArrayFromPyBuffer<VT_TYPE(elem)>(TfPyObjWrapper const &obj, string *err);
BOOST_PP_SEQ_FOR_EACH(INSTANTIATE, ~, VT_ARRAY_PYBUFFER_TYPES)
#undef INSTANTIATE

VT_API void Vt_AddBufferProtocolSupportToVtArrays()
{

// Add the buffer protocol support to every array type that we support it for.
#define VT_ADD_BUFFER_PROTOCOL(r, unused, elem)                         \
    Vt_AddBufferProtocol<VtArray<VT_TYPE(elem)> >();                    \
    VtValue::RegisterCast<TfPyObjWrapper, VtArray<VT_TYPE(elem)> >(     \
        Vt_CastPyObjToArray<VT_TYPE(elem)>);                            \
    VtValue::RegisterCast<vector<VtValue>, VtArray<VT_TYPE(elem)> >(    \
        Vt_CastVectorToArray<VT_TYPE(elem)>);                           \
    boost::python::def(BOOST_PP_STRINGIZE(VT_TYPE_NAME(elem))           \
                        "ArrayFromBuffer",                              \
                        Vt_WrapArrayFromBuffer<VT_TYPE(elem)>);

BOOST_PP_SEQ_FOR_EACH(VT_ADD_BUFFER_PROTOCOL, ~, VT_ARRAY_PYBUFFER_TYPES)

#undef VT_ADD_BUFFER_PROTOCOL
}

PXR_NAMESPACE_CLOSE_SCOPE
