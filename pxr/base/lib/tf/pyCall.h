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
#if !BOOST_PP_IS_ITERATING


#  ifndef TF_PYCALL_H
#  define TF_PYCALL_H

/// \file tf/pyCall.h
/// Utilities for calling python callables.
/// 
/// These functions handle trapping python errors and converting them to \a
/// TfErrors.

#ifndef TF_MAX_ARITY
#  define TF_MAX_ARITY 7
#endif // TF_MAX_ARITY

#include "pxr/base/tf/pyError.h"
#include "pxr/base/tf/pyLock.h"
#include "pxr/base/tf/pyObjWrapper.h"

#include <boost/preprocessor.hpp>
#include <boost/python/call.hpp>
#include <boost/python/object.hpp>

/// \class TfPyCall
///
/// Provide a way to call a Python callable.
/// 
/// Usage is as follows:
/// \code
///     return TfPyCall<RetType>(callable)(arg1, arg2, ... argN);
/// \endcode
/// Generally speaking, TfPyCall instances may be copied, assigned, destroyed,
/// and invoked without the client holding the GIL.  However, if the \a Return
/// template parameter is a \a boost::python::object (or a derived class, such
/// as list or tuple) then the client must hold the GIL in order to invoke the
/// call operator.
template <typename Return>
struct TfPyCall {
    /// Construct with callable \a c.  Constructing with a \c
    /// boost::python::object works, since those implicitly convert to \c
    /// TfPyObjWrapper, however in that case the GIL must be held by the caller.
    explicit TfPyCall(TfPyObjWrapper const &c) : _callable(c) {}
  private:
    TfPyObjWrapper _callable;
  public:
#define BOOST_PP_ITERATION_LIMITS (0, TF_MAX_ARITY)
#define BOOST_PP_FILENAME_1 "pxr/base/tf/pyCall.h"
#include BOOST_PP_ITERATE()
};
/* comment needed for scons dependency scanner
#include "pxr/base/tf/pyCall.h"
*/

#  endif // TF_PYCALL_H

#else // BOOST_PP_IS_ITERATING

#  define N BOOST_PP_ITERATION()

// This generates multi-argument versions of TfPyCall.
// One nice thing about this style of PP repetition is that the debugger will
// actually step you over these lines for any instantiation of Ctor.  Also
// compile errors will point to the correct line here.

#if N
template <BOOST_PP_ENUM_PARAMS(N, class A)>
#endif
Return operator()(BOOST_PP_ENUM_BINARY_PARAMS(N, A, a))
{
    TfPyLock pyLock;
    // Do *not* call through if there's an active python exception.
    if (not PyErr_Occurred()) {
        try {
            return boost::python::call<Return>
                (_callable.ptr() BOOST_PP_ENUM_TRAILING_PARAMS(N, a));
        } catch (boost::python::error_already_set const &) {
            // Convert any exception to TF_ERRORs.
            TfPyConvertPythonExceptionToTfErrors();
            PyErr_Clear();
        }
    }
    return Return();
}
#  undef N

#endif // BOOST_PP_IS_ITERATING
