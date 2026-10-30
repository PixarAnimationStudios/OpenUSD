/*
    nanobind/nb_misc.h: Miscellaneous bits (GIL, etc.)

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

NAMESPACE_BEGIN(NB_NAMESPACE)

/* Attach a Python thread state to the calling thread, which acquires the GIL
   in non-free-threaded builds. Nested use is fine.

   On Python 3.15 and newer, the interpreter refuses further attachments once
   it starts shutting down (PEP 788), and the constructor then fails. Code
   that can run on threads not created by Python must query is_valid() and
   skip the Python part of its work when the guard is invalid. Older Python
   versions block forever in this situation, so is_valid() always returns true
   there. */
struct gil_scoped_acquire {
public:
    NB_NONCOPYABLE(gil_scoped_acquire)
    gil_scoped_acquire() noexcept : state(NB_CALL(tstate_ensure)()) { }
    ~gil_scoped_acquire() {
        if (state)
            NB_CALL(tstate_release)(state);
    }

    /// Was a thread state attached successfully?
    bool is_valid() const { return state != nullptr; }
    explicit operator bool() const { return state != nullptr; }

private:
    void * const state;
};

struct gil_scoped_release {
public:
    NB_NONCOPYABLE(gil_scoped_release)
    gil_scoped_release() noexcept : state(PyEval_SaveThread()) { }
    ~gil_scoped_release() { PyEval_RestoreThread(state); }

private:
    PyThreadState *state;
};

NAMESPACE_BEGIN(detail)

/* Guard for destructors and callbacks that release Python resources and may
   run on any thread, including while the process is tearing down. It is
   invalid when either the interpreter or the nanobind runtime can no longer
   be entered, and the caller must then skip its work:

       if (nb::detail::cleanup_guard guard{})
           Py_DECREF(o); */
struct cleanup_guard {
public:
    NB_NONCOPYABLE(cleanup_guard)
    cleanup_guard() noexcept
        : state(NB_CALL(is_alive)() ? NB_CALL(tstate_ensure)() : nullptr) { }
    ~cleanup_guard() {
        if (state)
            NB_CALL(tstate_release)(state);
    }

    explicit operator bool() const { return state != nullptr; }

private:
    void * const state;
};

NAMESPACE_END(detail)

struct ft_mutex {
public:
    NB_NONCOPYABLE(ft_mutex)
    ft_mutex() = default;

#if !defined(NB_FREE_THREADED)
    void lock() { }
    void unlock() { }
#elif defined(Py_LIMITED_API)
    // PyMutex is not part of the 'abi3t' stable ABI (PEP 803), the only
    // limited-API form of free-threaded extensions. Such extensions always
    // run in split mode, and the backend operates on the mutex byte
    void lock() { NB_CALL(ft_mutex_lock)(&mutex); }
    void unlock() { NB_CALL(ft_mutex_unlock)(&mutex); }
private:
    uint8_t mutex = 0;
#else
    void lock() { PyMutex_Lock(&mutex); }
    void unlock() { PyMutex_Unlock(&mutex); }
private:
    PyMutex mutex { 0 };
#endif
};

struct ft_lock_guard {
public:
    NB_NONCOPYABLE(ft_lock_guard)
    ft_lock_guard(ft_mutex &m) : m(m) { m.lock(); }
    ~ft_lock_guard() { m.unlock(); }
private:
    ft_mutex &m;
};


struct ft_object_guard {
public:
    NB_NONCOPYABLE(ft_object_guard)
#if !defined(NB_FREE_THREADED)
    ft_object_guard(handle) { }
#else
    ft_object_guard(handle h) { PyCriticalSection_Begin(&cs, h.ptr()); }
    ~ft_object_guard() { PyCriticalSection_End(&cs); }
private:
    PyCriticalSection cs;
#endif
};

struct ft_object2_guard {
public:
    NB_NONCOPYABLE(ft_object2_guard)
#if !defined(NB_FREE_THREADED)
    ft_object2_guard(handle, handle) { }
#else
    ft_object2_guard(handle h1, handle h2) { PyCriticalSection2_Begin(&cs, h1.ptr(), h2.ptr()); }
    ~ft_object2_guard() { PyCriticalSection2_End(&cs); }
private:
    PyCriticalSection2 cs;
#endif
};

inline bool leak_warnings() noexcept {
    return NB_CALL(read_flag)(NB_CTX, detail::nb_flag::leak_warnings) != 0;
}

inline bool implicit_cast_warnings() noexcept {
    return NB_CALL(read_flag)(NB_CTX, detail::nb_flag::implicit_cast_warnings) != 0;
}

inline void set_leak_warnings(bool value) noexcept {
    NB_CALL(write_flag)(NB_CTX, detail::nb_flag::leak_warnings, value);
}

inline void set_implicit_cast_warnings(bool value) noexcept {
    NB_CALL(write_flag)(NB_CTX, detail::nb_flag::implicit_cast_warnings, value);
}

inline dict globals() {
#if NB_PYTHON_VERSION >= 0x030D0000
    dict d = steal<dict>(PyEval_GetFrameGlobals());
#else
    dict d = borrow<dict>(PyEval_GetGlobals());
#endif
    if (!d.is_valid())
        raise("nanobind::globals(): no frame is currently executing!");
    return d;
}

inline Py_hash_t hash(handle h) {
    Py_hash_t rv = PyObject_Hash(h.ptr());
    if (rv == -1 && PyErr_Occurred())
        nanobind::raise_python_error();
    return rv;
}

inline bool isinstance(handle inst, handle cls) {
    int ret = PyObject_IsInstance(inst.ptr(), cls.ptr());
    if (ret == -1)
      nanobind::raise_python_error();
    return ret;
}

inline bool is_alive() noexcept {
    return NB_CALL(is_alive)();
}

#if !defined(NB_BUILD)
// Do the work of NB_MODULE() on a module created by other means. Returns
// false with a Python error set, since nanobind's exception machinery may
// not be available yet.
inline bool register_module(handle m) noexcept {
    detail::init_singletons();

    const char *name = PyModule_GetName(m.ptr());
    if (!name || !detail::nb_backend_init(name))
        return false;

    detail::internals = NB_CALL(nb_module_init)(NB_DOMAIN_STR, m.ptr());
    return detail::internals != nullptr;
}
#endif

NAMESPACE_END(NB_NAMESPACE)
