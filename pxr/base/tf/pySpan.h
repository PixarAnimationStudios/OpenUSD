//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_SPAN_H
#define PXR_BASE_TF_PY_SPAN_H

/// \file
///
/// Python argument conversion for TfSpan<T>.
///
/// TfSpan is a view type: it does not own its elements.  Boost.python's
/// registry-based rvalue conversions construct their result in storage that is
/// destroyed before the wrapped function is called if that result depends on a
/// temporary, so TfSpan opts out of them (see pxr_boost_python_view_type in
/// tf/span.h) and instead provides arg_rvalue_from_python specializations here.
/// Any translation unit that wraps a function taking a TfSpan must include this
/// header.
///
/// A Python argument is converted to TfSpan<T> by trying, in order:
///
/// 1. Registered span sources for T (see TfPyRegisterSpanSource()).  These
///    produce a view of an existing wrapped C++ container with no copy.
/// 2. The buffer protocol, if a buffer layout is registered for T (see
///    TfPyRegisterSpanBufferLayout()) and the object exports a C-contiguous
///    buffer with matching element format and shape.  No copy.  Layouts for
///    bool and the builtin integer and floating-point types are registered
///    by Tf; libraries register layouts for their own types.
/// 3. For const T only, a Python sequence whose items each convert to T.  The
///    items are copied into storage owned by the converter for the duration
///    of the call.
///
/// Mutable spans are never satisfied by copying, since writes through the span
/// would be silently discarded.  Use them for data that must be edited in
/// place, such as images or mesh points.  To accept arbitrary sequences,
/// wrap a function that returns a new array instead.

#include "pxr/pxr.h"
#include "pxr/base/tf/api.h"
#include "pxr/base/tf/pySafePython.h"
#include "pxr/base/tf/span.h"

#include "pxr/external/boost/python/converter/arg_from_python.hpp"
#include "pxr/external/boost/python/converter/registered.hpp"
#include "pxr/external/boost/python/extract.hpp"

#include <cstddef>
#include <initializer_list>
#include <memory>
#include <type_traits>
#include <typeinfo>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

/// Function type for span sources.  If \p obj holds a source container whose
/// elements may be viewed, set \p *data and \p *size and return true.  If \p
/// wantMutable is true, \p *data must permit writes.
using Tf_PySpanSourceFn =
    bool (*)(PyObject *obj, bool wantMutable, void **data, size_t *size);

TF_API void
Tf_PyRegisterSpanSource(std::type_info const &elemType, Tf_PySpanSourceFn fn);

TF_API void
Tf_PyRegisterSpanBufferLayout(std::type_info const &elemType, size_t elemSize,
                              char scalarFormat,
                              Py_ssize_t const *elemShape, int elemNdim);

TF_API bool
Tf_PyGetSpanFromSources(PyObject *obj, std::type_info const &elemType,
                        bool wantMutable, void **data, size_t *size);

// On success, *view holds a buffer that the caller must release.
TF_API bool
Tf_PyGetSpanFromBuffer(PyObject *obj, std::type_info const &elemType,
                       bool wantMutable, Py_buffer *view,
                       void **data, size_t *size);

template <class Source, class T>
bool
Tf_PySpanSourceFromLValue(PyObject *obj, bool wantMutable,
                          void **data, size_t *size)
{
    using namespace PXR_BOOST_NAMESPACE::python::converter;
    using Value = std::remove_cv_t<T>;
    if (std::is_const_v<T> && wantMutable) {
        return false;
    }
    void *p = get_lvalue_from_python(obj, registered<Source>::converters);
    if (!p) {
        return false;
    }
    Source &src = *static_cast<Source *>(p);
    if constexpr (!std::is_const_v<T>) {
        if (wantMutable) {
            TfSpan<Value> span(src);
            *data = span.data();
            *size = span.size();
            return true;
        }
    }
    // Convert from const Source so that containers like VtArray do not detach.
    TfSpan<const Value> span(std::as_const(src));
    *data = const_cast<Value *>(span.data());
    *size = span.size();
    return true;
}

/// Register the wrapped C++ type \p Source as a span source for TfSpan<T>.
/// Python objects that hold a \p Source will convert to TfSpan<const T> as a
/// view of the held elements, by converting a const \p Source to TfSpan<const
/// T>.  If \p T is not const, they will also convert to TfSpan<T>, by
/// converting a non-const \p Source to TfSpan<T>.  This function acquires the
/// GIL, so once Python is initialized it may be called from any thread.
template <class Source, class T>
void
TfPyRegisterSpanSource()
{
    using Value = std::remove_cv_t<T>;
    static_assert(std::is_constructible_v<TfSpan<const Value>, Source const &>,
                  "Source must convert to TfSpan<const T>");
    static_assert(std::is_const_v<T> ||
                  std::is_constructible_v<TfSpan<Value>, Source &>,
                  "Source must convert to TfSpan<T>; for read-only sources, "
                  "register with const T");
    Tf_PyRegisterSpanSource(typeid(Value),
                            Tf_PySpanSourceFromLValue<Source, T>);
}

/// Register the buffer layout of \p T: a C-contiguous array of \p
/// elemShape-shaped blocks of scalars described by the struct-module format
/// character \p scalarFormat.  Buffers with one leading dimension followed by
/// \p elemShape, and a scalar format of matching kind and size, will convert to
/// TfSpan<T> with no copy.  For example, GfVec3f is registered with format 'f'
/// and shape {3}, and accepts buffers of shape (N, 3).  This function acquires
/// the GIL, so once Python is initialized it may be called from any thread.
template <class T>
void
TfPyRegisterSpanBufferLayout(char scalarFormat,
                             std::initializer_list<Py_ssize_t> elemShape = {})
{
    Tf_PyRegisterSpanBufferLayout(
        typeid(T), sizeof(T), scalarFormat,
        elemShape.begin(), static_cast<int>(elemShape.size()));
}

// Holds a TfSpan<T> and whatever keeps its elements alive for the duration of a
// wrapped function call.
template <class T>
class Tf_PySpanArgConverter
{
    using Value = std::remove_cv_t<T>;
    static constexpr bool _isMutable = !std::is_const_v<T>;

public:
    explicit Tf_PySpanArgConverter(PyObject *obj) {
        void *data;
        size_t size;
        if (Tf_PyGetSpanFromSources(
                obj, typeid(Value), _isMutable, &data, &size) ||
            (_hasView = Tf_PyGetSpanFromBuffer(
                obj, typeid(Value), _isMutable, &_view, &data, &size))) {
            _span = TfSpan<T>(static_cast<T *>(data), size);
            _convertible = true;
        }
        else if constexpr (!_isMutable) {
            _convertible = _CopyFromSequence(obj);
        }
    }

    ~Tf_PySpanArgConverter() {
        if (_hasView) {
            PyBuffer_Release(&_view);
        }
    }

    Tf_PySpanArgConverter(Tf_PySpanArgConverter const &) = delete;
    Tf_PySpanArgConverter &operator=(Tf_PySpanArgConverter const &) = delete;

    bool convertible() const { return _convertible; }

    TfSpan<T> &operator()() { return _span; }

private:
    bool _CopyFromSequence(PyObject *obj) {
        if (!PySequence_Check(obj) ||
            PyUnicode_Check(obj) || PyBytes_Check(obj)) {
            return false;
        }
        Py_ssize_t len = PySequence_Size(obj);
        if (len < 0) {
            PyErr_Clear();
            return false;
        }
        std::unique_ptr<Value []> storage(new Value[len]);
        for (Py_ssize_t i = 0; i != len; ++i) {
            PyObject *item = PySequence_GetItem(obj, i);
            if (!item) {
                PyErr_Clear();
                return false;
            }
            PXR_BOOST_NAMESPACE::python::extract<Value> get(item);
            bool ok = get.check();
            if (ok) {
                storage[i] = get();
            }
            Py_DECREF(item);
            if (!ok) {
                return false;
            }
        }
        _span = TfSpan<T>(storage.get(), len);
        _storage = std::move(storage);
        return true;
    }

    TfSpan<T> _span;
    std::unique_ptr<Value []> _storage;
    Py_buffer _view;
    bool _hasView = false;
    bool _convertible = false;
};

PXR_NAMESPACE_CLOSE_SCOPE

namespace PXR_BOOST_NAMESPACE::python::converter {

template <class T>
struct arg_rvalue_from_python<PXR_NS::TfSpan<T>>
    : PXR_NS::Tf_PySpanArgConverter<T>
{
    using result_type = PXR_NS::TfSpan<T> &;
    using PXR_NS::Tf_PySpanArgConverter<T>::Tf_PySpanArgConverter;
};

template <class T>
struct arg_rvalue_from_python<PXR_NS::TfSpan<T> const &>
    : PXR_NS::Tf_PySpanArgConverter<T>
{
    using result_type = PXR_NS::TfSpan<T> &;
    using PXR_NS::Tf_PySpanArgConverter<T>::Tf_PySpanArgConverter;
};

} // namespace PXR_BOOST_NAMESPACE::python::converter

#endif // PXR_BASE_TF_PY_SPAN_H
