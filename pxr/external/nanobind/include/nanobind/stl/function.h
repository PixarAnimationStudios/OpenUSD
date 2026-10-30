/*
    nanobind/stl/function.h: type caster for std::function<...>

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/nanobind.h>
#include <functional>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

struct pyfunc_wrapper {
    PyObject *f;

    explicit pyfunc_wrapper(PyObject *f) : f(f) {
        Py_INCREF(f);
    }

    pyfunc_wrapper(pyfunc_wrapper &&w) noexcept : f(w.f) {
        w.f = nullptr;
    }

    pyfunc_wrapper(const pyfunc_wrapper &w) : f(w.f) {
        if (f) {
            // Don't touch the reference count if the interpreter is shut down
            if (cleanup_guard guard{})
                Py_INCREF(f);
            else
                f = nullptr;
        }
    }

    ~pyfunc_wrapper() {
        if (f) {
            // Don't run the deleter if the interpreter has been shut down
            if (cleanup_guard guard{})
                Py_DECREF(f);
        }
    }

    pyfunc_wrapper &operator=(const pyfunc_wrapper) = delete;
    pyfunc_wrapper &operator=(pyfunc_wrapper &&) = delete;
};

template <typename Return, typename... Args>
struct type_caster<std::function<Return(Args...)>> {
    using ReturnCaster = make_caster<
        std::conditional_t<std::is_void_v<Return>, void_type, Return>>;

    NB_TYPE_CASTER(std::function <Return(Args...)>,
                   const_name("collections.abc.Callable[[") +
                       concat(make_caster<Args>::Name...) + const_name("], ") +
                       ReturnCaster::Name + const_name("]"))

    struct pyfunc_wrapper_t : pyfunc_wrapper {
        using pyfunc_wrapper::pyfunc_wrapper;

        Return operator()(Args... args) const {
            gil_scoped_acquire acq;
            if (!acq.is_valid())
                raise("nanobind: cannot invoke a Python callable, the "
                      "interpreter is shutting down!");
            return cast<Return>(handle(f)((forward_t<Args>) args...));
        }
    };

    bool from_python(handle src, uint32_t flags, cleanup_list *) noexcept {
        if (src.is_none())
            return flags & cast_flags::convert;

        if (!PyCallable_Check(src.ptr()))
            return false;

        value = pyfunc_wrapper_t(src.ptr());

        return true;
    }

    static handle from_cpp(const Value &value, rv_policy rvp,
                           cleanup_list *) noexcept {
        const pyfunc_wrapper_t *wrapper = value.template target<pyfunc_wrapper_t>();
        if (wrapper)
            return handle(wrapper->f).inc_ref();

        if (rvp == rv_policy::none)
            return handle();

        if (!value)
            return none().release();

        return cpp_function(value).release();
    }
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
