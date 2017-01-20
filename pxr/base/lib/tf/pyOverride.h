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
#if !defined(BOOST_PP_IS_ITERATING)

#ifndef TF_PYOVERRIDE_H
#define TF_PYOVERRIDE_H

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
public:
    /// Clients must hold the GIL to construct.
    TfPyOverride(boost::python::handle<> callable)
        : TfPyObjWrapper(boost::python::object(callable))
    {}

    /// Call the override.
    /// Clients need not hold the GIL to invoke the call operator.
    TfPyMethodResult
    operator()() const
    {
        TfPyLock lock;

        TfPyMethodResult x(
            PyEval_CallFunction(
                this->ptr()
              , const_cast<char*>("()")
            ));
        return x;
    }

# define BOOST_PYTHON_fast_arg_to_python_get(z, n, _)   \
    , boost::python::converter::arg_to_python<A##n>(a##n).get()

# define BOOST_PP_ITERATION_PARAMS_1 (3, (1, BOOST_PYTHON_MAX_ARITY, "pxr/base/tf/pyOverride.h"))
# include BOOST_PP_ITERATE()

# undef BOOST_PYTHON_fast_arg_to_python_get

};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_PYOVERRIDE_H

#else

# define N BOOST_PP_ITERATION()

template <
    BOOST_PP_ENUM_PARAMS_Z(1, N, class A)
    >
TfPyMethodResult
operator()( BOOST_PP_ENUM_BINARY_PARAMS_Z(1, N, A, const& a) ) const
{
    TfPyLock lock;

    TfPyMethodResult x(
        PyEval_CallFunction(
            this->ptr()
          , const_cast<char*>("(" BOOST_PP_REPEAT_1ST(N, BOOST_PYTHON_FIXED, "O") ")")
            BOOST_PP_REPEAT_1ST(N, BOOST_PYTHON_fast_arg_to_python_get, nil)
        ));
    return x;
}

# undef N

#endif // TF_PYOVERRIDE_H
