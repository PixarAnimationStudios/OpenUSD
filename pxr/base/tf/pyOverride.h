//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PY_OVERRIDE_H
#define PXR_BASE_TF_PY_OVERRIDE_H

#include "pxr/pxr.h"

#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyObjWrapper.h"

#include "pxr/external/boost/python/override.hpp"
#include "pxr/external/boost/python/type.hpp"

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfPyMethodResult
///
/// A reimplementation of pxr_boost::python::detail::method_result.
///
/// This class is reimplemented from the boost class simply because the
/// provided class only allows construction from it's friended class
/// pxr_boost::python::override, which we also reimplement below.
///
/// \see TfPyOverride
class TfPyMethodResult
{
private:
    /// Clients must hold GIL to construct.
    friend class TfPyOverride;
    explicit TfPyMethodResult(PyObject* x)
    : m_obj(x)
    {}

public:
    /// Implement copy to do python refcounting while holding the GIL.
    TfPyMethodResult(TfPyMethodResult const &other);

    /// Implement dtor to do python refcounting while holding the GIL.
    ~TfPyMethodResult();

    /// Implement assign to do python refcounting while holding the GIL.
    TfPyMethodResult &operator=(TfPyMethodResult const &other);

    template <class T>
    operator T()
    {
        TfPyLock lock;
        pxr_boost::python::converter::return_from_python<T> converter;
        return converter(m_obj.release());
    }

    template <class T>
    operator T&() const
    {
        TfPyLock lock;
        pxr_boost::python::converter::return_from_python<T&> converter;
        return converter(
            const_cast<pxr_boost::python::handle<>&>(m_obj).release());
    }

    template <class T>
    T as(pxr_boost::python::type<T>* = 0)
    {
        TfPyLock lock;
        pxr_boost::python::converter::return_from_python<T> converter;
        return converter(m_obj.release());
    }

    template <class T>
    T unchecked(pxr_boost::python::type<T>* = 0)
    {
        TfPyLock lock;
        return pxr_boost::python::extract<T>(m_obj)();
    }

private:
    mutable pxr_boost::python::handle<> m_obj;
};

/// \class TfPyOverride
///
/// A reimplementation of pxr_boost::python::override.
///
/// This class is reimplemented from the boost class simply because the
/// provided class only allows construction from, ultimately,
/// pxr_boost::python::wrapper::get_override(). Unfortunately, that method 
/// returns the wrong result when the overridden function we are asking about
/// lives not on the directly wrapped C++ class, but a wrapped ancestor.
/// So we provide our own version, TfPyOverride, with a public constructor.
///
/// Note that clients must have the python GIL when constructing a
/// TfPyOverride object.
///
class TfPyOverride : public TfPyObjWrapper
{
    // Helper to generate Py_BuildValue-style format strings at compile time.
    template <typename Arg>
    constexpr static char _PyObjArg()
    {
        return 'O';
    }

public:
    /// Clients must hold the GIL to construct.
    TfPyOverride(pxr_boost::python::handle<> callable)
        : TfPyObjWrapper(pxr_boost::python::object(callable))
    {}

    /// Call the override.
    /// Clients need not hold the GIL to invoke the call operator.
    template <typename... Args>
    TfPyMethodResult
    operator()(Args const&... args) const
    {
        TfPyLock lock;
        // Use the Args parameter pack together with the _PyObjArg helper to
        // unpack the right number of 'O' elements into the format string.
        static const char pyCallFormat[] =
            { '(', _PyObjArg<Args>()..., ')', '\0' };

        TfPyMethodResult x(
            PyEval_CallFunction(
                this->ptr(),
                const_cast<char*>(pyCallFormat),
                pxr_boost::python::converter::arg_to_python<Args>(args).get()...));
        return x;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_OVERRIDE_H
