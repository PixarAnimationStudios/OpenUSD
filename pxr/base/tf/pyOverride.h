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
#ifndef PXR_BASE_TF_PY_OVERRIDE_H
#define PXR_BASE_TF_PY_OVERRIDE_H

#include "pxr/pxr.h"

#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyObjWrapper.h"

#include <boost/python/override.hpp>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfPyMethodResult
///
/// A reimplementation of boost::python::detail::method_result.
///
/// This class is reimplemented from the boost class simply because the
/// provided class only allows construction from it's friended class
/// boost::python::override, which we also reimplement below.
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
        boost::python::converter::return_from_python<T> converter;
        return converter(m_obj.release());
    }

    template <class T>
    operator T&() const
    {
        TfPyLock lock;
        boost::python::converter::return_from_python<T&> converter;
        return converter(
            const_cast<boost::python::handle<>&>(m_obj).release());
    }

    template <class T>
    T as(boost::type<T>* = 0)
    {
        TfPyLock lock;
        boost::python::converter::return_from_python<T> converter;
        return converter(m_obj.release());
    }

    template <class T>
    T unchecked(boost::type<T>* = 0)
    {
        TfPyLock lock;
        return boost::python::extract<T>(m_obj)();
    }

private:
    mutable boost::python::handle<> m_obj;
};

/// \class TfPyOverride
///
/// A reimplementation of boost::python::override.
///
/// This class is reimplemented from the boost class simply because the
/// provided class only allows construction from, ultimately,
/// boost::python::wrapper::get_override(). Unfortunately, that method 
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
    TfPyOverride(boost::python::handle<> callable)
        : TfPyObjWrapper(boost::python::object(callable))
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
                boost::python::converter::arg_to_python<Args>(args).get()...));
        return x;
    }
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_PY_OVERRIDE_H
