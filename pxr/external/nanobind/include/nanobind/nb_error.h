/*
    nanobind/nb_error.h: Python exception handling, binding of exceptions

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

NAMESPACE_BEGIN(NB_NAMESPACE)

/// RAII wrapper that temporarily clears any Python error state
#if NB_PYTHON_VERSION >= 0x030C0000
struct error_scope {
    error_scope() { value = PyErr_GetRaisedException(); }
    ~error_scope() { PyErr_SetRaisedException(value); }
private:
    PyObject *value;
};
#else
struct error_scope {
    error_scope() { PyErr_Fetch(&type, &value, &trace); }
    ~error_scope() { PyErr_Restore(type, value, trace); }
private:
    PyObject *type, *value, *trace;
};
#endif

/* The layout of ``python_error`` and ``builtin_exception`` is part of
   nanobind's backend ABI contract and is frozen within major versions. */

inline namespace NB_BACKEND_ABI_NS {

// Wraps a Python error state as a C++ exception.
class NB_EXPORT python_error : public std::exception {
public:
    /// Storage of this class, see 'detail::error_payload'
    using payload = detail::error_payload;

    python_error() { NB_CALL(error_fetch)(&m_payload); }
    python_error(const python_error &e) : std::exception(e) {
        NB_CALL(error_copy)(&e.m_payload, &m_payload);
    }
    python_error(python_error &&e) noexcept
        : std::exception(e), m_payload(e.m_payload) {
        e.m_payload = payload { };
    }
    // The destructor is deliberately inline so that the vtable & typeinfo are
    // weak definitions in every binary that the dynamic linker can then unify.
    ~python_error() override { NB_CALL(error_release)(&m_payload); }

    bool matches(handle exc) const noexcept {
        return PyErr_GivenExceptionMatches(m_payload.value, exc.ptr()) != 0;
    }

    /// Move the error back into the Python domain. This may only be called
    /// once, and you should not reraise the exception in C++ afterward.
    void restore() noexcept {
        NB_CALL(error_restore)(m_payload.value);
        m_payload.value = nullptr;
    }

    /// Pass the error to Python's `sys.unraisablehook`, which prints
    /// a traceback to `sys.stderr` by default but may be overridden.
    /// The *context* should be some object whose repr() helps clarify where
    /// the error occurred. Like `.restore()`, this consumes the error and
    /// you should not reraise the exception in C++ afterward.
    void discard_as_unraisable(handle context) noexcept {
        restore();
        PyErr_WriteUnraisable(context.ptr());
    }

    void discard_as_unraisable(const char *context) noexcept {
        object context_s = steal(PyUnicode_FromString(context));
        discard_as_unraisable(context_s);
    }

    handle value() const { return m_payload.value; }

    handle type() const { return value().type(); }

    object traceback() const {
        return steal(PyException_GetTraceback(m_payload.value));
    }

    const char *what() const noexcept override {
        return NB_CALL(error_what)(&m_payload);
    }

private:
    mutable payload m_payload { };
};

/// Thrown by nanobind::cast when casting fails
using cast_error = std::bad_cast;

// Base interface used to expose common Python exceptions in C++. Fully
// inline for the same reason as python_error's destructor above.
class NB_EXPORT builtin_exception : public std::runtime_error {
public:
    builtin_exception(exception_type type, const char *what)
        : std::runtime_error(what ? what : ""), m_type(type) { }
    builtin_exception(builtin_exception &&) = default;
    builtin_exception(const builtin_exception &) = default;
    ~builtin_exception() override = default;
    exception_type type() const { return m_type; }
private:
    exception_type m_type;
};

} // namespace NB_BACKEND_ABI_NS

#define NB_EXCEPTION(name)                                                     \
    inline builtin_exception name(const char *what = nullptr) {                \
        return builtin_exception(exception_type::name, what);                  \
    }

NB_EXCEPTION(stop_iteration)
NB_EXCEPTION(index_error)
NB_EXCEPTION(key_error)
NB_EXCEPTION(value_error)
NB_EXCEPTION(type_error)
NB_EXCEPTION(buffer_error)
NB_EXCEPTION(import_error)
NB_EXCEPTION(attribute_error)
NB_EXCEPTION(next_overload)

#undef NB_EXCEPTION

NAMESPACE_BEGIN(detail)

[[noreturn]] NB_NOINLINE inline void raise_python_error() {
    throw python_error();
}

[[noreturn]] NB_NOINLINE inline void raise_python_or_cast_error() {
    if (PyErr_Occurred())
        throw python_error();
    throw cast_error();
}

NAMESPACE_END(detail)

inline void register_exception_translator(detail::exception_translator t,
                                          void *payload = nullptr) {
    NB_CALL(register_exception_translator)(NB_CTX, t, payload);
}

template <typename T>
class exception : public object {
    NB_OBJECT_DEFAULT(exception, object, "Exception", PyExceptionClass_Check)

    exception(handle scope, const char *name, handle base = PyExc_Exception)
        : object(NB_CALL(exception_new)(NB_CTX, scope.ptr(), name, base.ptr()),
                 detail::steal_t()) {
        NB_CALL(register_exception_translator)(NB_CTX,
            [](const std::exception_ptr &p, void *payload) {
                try {
                    std::rethrow_exception(p);
                } catch (T &e) {
                    PyErr_SetString((PyObject *) payload, e.what());
                }
            }, m_ptr);
    }
};

// The two functions below format through PyErr_FormatV(), whose format string
// language is not covered by the 'printf' __attribute__.

/// Chain a new error of type 'type' onto the currently pending one
inline void chain_error(handle type, const char *fmt, ...) noexcept {
    va_list args;
    va_start(args, fmt);
    NB_CALL(chain_v)(type.ptr(), fmt, args);
    va_end(args);
}

/// Restore 'e', chain a new error of type 'type' onto it, and re-raise
[[noreturn]] NB_NOINLINE
inline void raise_from(python_error &e, handle type, const char *fmt, ...) {
    e.restore();
    va_list args;
    va_start(args, fmt);
    NB_CALL(chain_v)(type.ptr(), fmt, args);
    va_end(args);
    detail::raise_python_error();
}

NAMESPACE_END(NB_NAMESPACE)
