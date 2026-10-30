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

#include "pxr/base/tf/pySafePython.h"
#include "pxr/base/tf/pyNanobindBridge.h"
#include <nanobind/nanobind.h>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfPyMethodResult
///
/// \see TfPyOverride
class TfPyMethodResult
{
private:
    /// Clients must hold GIL to construct.
    friend class TfPyOverride;
    explicit TfPyMethodResult(PyObject* x)
    : m_obj(nanobind::steal<nanobind::object>(x))
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
        return nanobind::cast<T>(m_obj);
    }

    template <class T>
    operator T&() const
    {
        TfPyLock lock;
        return nanobind::cast<T&>(m_obj);
    }

    template <class T>
    T as()
    {
        TfPyLock lock;
        return nanobind::cast<T>(m_obj);
    }

    template <class T>
    T unchecked()
    {
        TfPyLock lock;
        return nanobind::cast<T>(m_obj);
    }

private:
    mutable nanobind::object m_obj;
};

/// \class TfPyOverride
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
    TfPyOverride(nanobind::handle callable)
        : TfPyObjWrapper(nanobind::borrow<nanobind::object>(callable))
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
                nanobind::cast(args).ptr()...));
        return x;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_OVERRIDE_H
