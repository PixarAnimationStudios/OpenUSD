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
#ifndef TF_SCOPED_H
#define TF_SCOPED_H

#include "pxr/pxr.h"

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/utility.hpp>

PXR_NAMESPACE_OPEN_SCOPE

/// \class TfScoped
/// \ingroup group_tf_Multithreading
///
/// Execute code on exiting scope.
///
/// A \c TfScoped executes code when destroyed.  It's useful when cleanup code
/// should be executed when exiting the scope because it gets executed no
/// matter how the scope is exited (e.g. normal execution, return, exceptions,
/// etc).
///
/// \code
///     int func(bool x) {
///          TfScoped scope(cleanup);
///          return func2(x);          // call cleanup after calling func2
///     }
/// \endcode
///
template <typename T = boost::function<void ()> >
class TfScoped : boost::noncopyable {
public:
    /// The type of the function executed on destruction.
    typedef T Procedure;

    /// Execute \p leave when this object goes out of scope.
    explicit TfScoped(const Procedure& leave) : _onExit(leave) { }

    ~TfScoped() { _onExit(); }

private:
    // Can't put these on the heap.  No implemention needed.
    static void *operator new(::std::size_t size);

private:
    Procedure _onExit;
};

// Specialization of TfScoped for member functions.
template <typename T>
class TfScoped<void (T::*)()> : boost::noncopyable {
public:
    /// The type of the function executed on destruction.
    typedef void (T::*Procedure)();

    /// Execute \p leave on \p obj when this object goes out of scope.
    explicit TfScoped(T* obj, const Procedure& leave) :
        _obj(obj), _onExit(leave) { }

    ~TfScoped() { (_obj->*_onExit)(); }

private:
    // Can't put these on the heap.  No implemention needed.
    static void *operator new(::std::size_t size);

private:
    T* _obj;
    Procedure _onExit;
};

// Specialization of TfScoped for functions taking one pointer argument.
template <typename T>
class TfScoped<void (*)(T*)> : boost::noncopyable {
public:
    /// The type of the function executed on destruction.
    typedef void (*Procedure)(T*);

    /// Execute \p leave, passing \p obj, when this object goes out of scope.
    explicit TfScoped(const Procedure& leave, T* obj) :
        _obj(obj), _onExit(leave) { }

    ~TfScoped() { _onExit(_obj); }

private:
    // Can't put these on the heap.  No implemention needed.
    static void *operator new(::std::size_t size);

private:
    T* _obj;
    Procedure _onExit;
};

/// \class TfScopedVar
///
/// Reset variable on exiting scope.
///
/// A \c TfScopedVar sets a variable to a value when created then restores its
/// original value when destroyed.  For example:
///
/// \code
///     int func(bool x) {
///          TfScopedVar<bool> scope(x, true);  // set x to true
///          return func2(x);                   // restore x after calling func2
///     }
/// \endcode
template <typename T>
class TfScopedVar : boost::noncopyable {
public:
    /// Set/reset variable
    ///
    /// Sets \p x to \p val immediately and restores its old value when this
    /// goes out of scope.
    explicit TfScopedVar(T& x, const T& val) :
        _x(&x),
        _old(x)
    {
        x = val;
    }

    ~TfScopedVar() { *_x = _old; }

private:
    // Can't put these on the heap.  No implemention needed.
    static void *operator new(::std::size_t size);

private:
    T* _x;
    T _old;
};

/// \class TfScopedAutoVar
///
/// Reset variable on exiting scope.
///
/// A \c TfScopedAutoVar sets a variable to a value when created then restores
/// its original value when destroyed.
///
/// For example:
/// \code
///     int func(bool x) {
///          TfScopedAutoVar scope(x, true);    // set x to true
///          return func2(x);                   // restore x after calling func2
///     }
/// \endcode
///
/// This differs from \c TfScopedVar in that it's not a template class, the
/// value type is deduced automatically and it allocates memory on the heap.
/// If performance is critical or memory must not be allocated then use \c
/// TfScopedVar instead.
///
/// \see TfScopedVar
class TfScopedAutoVar : boost::noncopyable {
public:
    /// Set/reset variable
    ///
    /// Sets \p x to \p val immediately and restores its old value when this
    /// goes out of scope.
    template <typename T>
    explicit TfScopedAutoVar(T& x, const T& val) :
        _scope(boost::bind(&TfScopedAutoVar::_Set<T>, &x, x))
    {
        x = val;
    }

private:
    // Restore value function
    template <typename T>
    static void _Set(T* x, const T& val)
    {
        *x = val;
    }

    // Can't put these on the heap.  No implemention needed.
    static void *operator new(::std::size_t size);

private:
    TfScoped<> _scope;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // TF_SCOPED_H
