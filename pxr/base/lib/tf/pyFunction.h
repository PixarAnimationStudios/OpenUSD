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
#ifndef BOOST_PP_IS_ITERATING

#ifndef TF_PYFUNCTION_H
#define TF_PYFUNCTION_H

#ifndef TF_MAX_ARITY
#  define TF_MAX_ARITY 7
#endif // TF_MAX_ARITY

#include "pxr/base/tf/pyCall.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyObjWrapper.h"
#include "pxr/base/tf/pyUtils.h"

#include <boost/python/converter/from_python.hpp>
#include <boost/python/converter/registered.hpp>
#include <boost/python/converter/rvalue_from_python_data.hpp>
#include <boost/python/extract.hpp>
#include <boost/python/handle.hpp>
#include <boost/python/object.hpp>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/preprocessor.hpp>

template <typename T>
struct TfPyFunctionFromPython;

#define BOOST_PP_ITERATION_LIMITS (0, TF_MAX_ARITY)
#define BOOST_PP_FILENAME_1 "pxr/base/tf/pyFunction.h"
#include BOOST_PP_ITERATE()
/* comment needed for scons dependency scanner
#include "pxr/base/tf/pyFunction.h"
*/

#endif // TF_PYFUNCTION_H

#else // BOOST_PP_IS_ITERATING

#define N BOOST_PP_ITERATION()

// PLACEHOLDER is used to produce the _1 _2 _3 placeholders for
// boost::bind.
#define PLACEHOLDER(unused, n, unused2) BOOST_PP_CAT(_,BOOST_PP_INC(n))

template <typename Ret BOOST_PP_ENUM_TRAILING_PARAMS(N, typename A)>
struct TfPyFunctionFromPython<Ret (BOOST_PP_ENUM_PARAMS(N, A))>
{
    typedef boost::function<Ret (BOOST_PP_ENUM_PARAMS(N, A))> FuncType;

    static Ret Call(TfPyObjWrapper const &callable
                    BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(N, A, a)) {
        TfPyLock lock;
        return TfPyCall<Ret>(callable)(BOOST_PP_ENUM_PARAMS(N, a));
    }

    static Ret CallWeak(TfPyObjWrapper const &weak
                        BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(N, A, a)) {
        using namespace boost::python;
        // Attempt to get the referenced callable object.
        TfPyLock lock;
        object callable(handle<>(borrowed(PyWeakref_GetObject(weak.ptr()))));
        if (TfPyIsNone(callable)) {
            TF_WARN("Tried to call an expired python callback");
            return Ret();
        }            
        return TfPyCall<Ret>(callable)(BOOST_PP_ENUM_PARAMS(N, a));
    }

    static Ret CallMethod(TfPyObjWrapper const &func,
                          TfPyObjWrapper const &weakSelf,
                          TfPyObjWrapper const &cls
                          BOOST_PP_ENUM_TRAILING_BINARY_PARAMS(N, A, a)) {
        using namespace boost::python;
        // Attempt to get the referenced self parameter, then build a new
        // instance method and call it.
        TfPyLock lock;
        PyObject *self = PyWeakref_GetObject(weakSelf.ptr());
        if (self == Py_None) {
            TF_WARN("Tried to call a method on an expired python instance");
            return Ret();
        }
        object method(handle<>(PyMethod_New(func.ptr(), self, cls.ptr())));
        return TfPyCall<Ret>(method)(BOOST_PP_ENUM_PARAMS(N, a));
    }

    TfPyFunctionFromPython() {
        using namespace boost::python;
        converter::registry::
            insert(&convertible, &construct, type_id<FuncType>());
    }

    static void *convertible(PyObject *obj) {
        return PyCallable_Check(obj) ? obj : 0;
    }

    static void construct(PyObject *src, boost::python::converter::
                          rvalue_from_python_stage1_data *data) {
        using std::string;
        using boost::bind;
        using namespace boost::python;
        
        void *storage = ((converter::rvalue_from_python_storage<FuncType> *)
                         data)->storage.bytes;

        // In the case of instance methods, holding a strong reference will
        // keep the bound 'self' argument alive indefinitely, which is
        // undesirable. Unfortunately, we can't just keep a weak reference to
        // the instance method, because python synthesizes these on-the-fly.
        // Instead we do something like what PyQt's SIP does, and break the
        // method into three parts: the class, the function, and the self
        // parameter.  We keep strong references to the class and the
        // function, but a weak reference to 'self'.  Then at call-time, if
        // self has not expired, we build a new instancemethod and call it.
        //
        // Otherwise if the callable is a lambda (checked in a hacky way, but
        // mirroring SIP), we take a strong reference.
        // 
        // For all other callables, we attempt to take weak references to
        // them. If that fails, we take a strong reference.
        //
        // This is all sort of contrived, but seems to have the right behavior
        // for most usage patterns.

        object callable(handle<>(borrowed(src)));
        PyObject *pyCallable = callable.ptr();
        PyObject *self =
            PyMethod_Check(pyCallable) ? PyMethod_GET_SELF(pyCallable) : NULL;

        if (self) {
            // Deconstruct the method and attempt to get a weak reference to
            // the self instance.
            object cls(handle<>(borrowed(PyMethod_GET_CLASS(pyCallable))));
            object func(handle<>(borrowed(PyMethod_GET_FUNCTION(pyCallable))));
            object weakSelf(handle<>(PyWeakref_NewRef(self, NULL)));
            new (storage)
                FuncType(bind(CallMethod,
                              TfPyObjWrapper(func),
                              TfPyObjWrapper(weakSelf),
                              TfPyObjWrapper(cls)
                              BOOST_PP_ENUM_TRAILING(N, PLACEHOLDER, ~)));
        } else if (PyObject_HasAttrString(pyCallable, "__name__") and
                   extract<string>(callable.attr("__name__"))() == "<lambda>") {
            // Explicitly hold on to strong references to lambdas.
            new (storage)
                FuncType(bind(Call, TfPyObjWrapper(callable)
                              BOOST_PP_ENUM_TRAILING(N, PLACEHOLDER, ~)));
        } else {
            // Attempt to get a weak reference to the callable.
            if (PyObject *weakCallable =
                PyWeakref_NewRef(pyCallable, NULL)) {
                new (storage)
                    FuncType(bind(CallWeak,
                                  TfPyObjWrapper(object(handle<>(weakCallable)))
                                  BOOST_PP_ENUM_TRAILING(N, PLACEHOLDER, ~)));
            } else {
                // Fall back to taking a strong reference.
                PyErr_Clear();
                new (storage)
                    FuncType(bind(Call, TfPyObjWrapper(callable)
                                  BOOST_PP_ENUM_TRAILING(N, PLACEHOLDER, ~)));
            }
        }

        data->convertible = storage;
    }
};

#undef N
#undef PLACEHOLDER

#endif // BOOST_PP_IS_ITERATING
