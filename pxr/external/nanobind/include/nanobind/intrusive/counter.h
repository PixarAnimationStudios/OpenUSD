/*
    nanobind/intrusive/counter.h: Intrusive reference counting sample
    implementation.

    Intrusive reference counting is a simple solution for various lifetime and
    ownership-related issues that can arise in Python bindings of C++ code. The
    implementation here represents one of many ways in which intrusive
    reference counting can be realized and is included for convenience.

    The code in this file is designed to be truly minimal: it depends neither
    on Python, nanobind, nor the STL. This enables its use in small projects
    with a 100% optional Python interface.

    Two section of nanobind's documentation discuss intrusive reference
    counting in general:

     - https://nanobind.readthedocs.io/en/latest/ownership.html
     - https://nanobind.readthedocs.io/en/latest/ownership_adv.html

    Comments below are specific to this sample implementation.

    Copyright (c) 2023 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <cstdint>
#include <cstdlib>

// Override this definition to specify DLL export/import declarations
#if !defined(NB_INTRUSIVE_EXPORT)
#  define NB_INTRUSIVE_EXPORT
#endif

#if !defined(Py_PYTHON_H)
/* While the implementation below does not directly depend on Python, the
   PyObject type occurs in a few function interfaces (in a fully opaque
   manner). The lines below forward-declare it. */
extern "C" {
    struct _object;
    typedef _object PyObject;
};
#endif

#if !defined(NAMESPACE_BEGIN)
#  define NAMESPACE_BEGIN(name) namespace name {
#endif

#if !defined(NAMESPACE_END)
#  define NAMESPACE_END(name) }
#endif

NAMESPACE_BEGIN(nanobind)

/** \brief Simple intrusive reference counter.
 *
 * Intrusive reference counting is a simple solution for various lifetime and
 * ownership-related issues that can arise in Python bindings of C++ code. The
 * implementation here represents one of many ways in which intrusive reference
 * counting can be realized and is included for convenience.
 *
 * The ``intrusive_counter`` class represents an atomic counter that can be
 * increased (via ``inc_ref()``) or decreased (via ``dec_ref()``). When the
 * counter reaches zero, the object should be deleted, which ``dec_ref()``
 * indicates by returning ``true``.
 *
 * In addition to this simple counting mechanism, ownership of the object can
 * also be transferred to Python (via ``set_self_py()``). In this case,
 * subsequent calls to ``inc_ref()`` and ``dec_ref()`` modify the reference
 * count of the underlying Python object. The ``intrusive_counter`` class
 * supports both cases using only ``sizeof(void*)`` bytes of storage.
 *
 * To incorporate intrusive reference counting into your own project, you would
 * usually add an ``intrusive_counter``-typed member to the base class of an
 * object hierarchy and expose it as follows:
 *
 * ```cpp
 * #include <nanobind/intrusive/counter.h>
 *
 * class Object {
 * public:
 *     void inc_ref() noexcept { m_ref_count.inc_ref(); }
 *     bool dec_ref() noexcept { return m_ref_count.dec_ref(); }
 *
 *     // Important: must declare virtual destructor
 *     virtual ~Object() = default;
 *
 *     void set_self_py(PyObject *self) noexcept {
 *         m_ref_count.set_self_py(self);
 *     }
 *
 * private:
 *     nb::intrusive_counter m_ref_count;
 * };
 *
 * // Convenience function for increasing the reference count of an instance
 * inline void inc_ref(Object *o) noexcept {
 *     if (o)
 *        o->inc_ref();
 * }
 *
 * // Convenience function for decreasing the reference count of an instance
 * // and potentially deleting it when the count reaches zero
 * inline void dec_ref(Object *o) noexcept {
 *     if (o && o->dec_ref())
 *         delete o;
 * }
 * ```
 *
 * Alternatively, you could also inherit from ``intrusive_base``, which obviates
 * the need for all of the above declarations:
 *
 * ```cpp
 * class Object : public intrusive_base {
 * public:
 *     // ...
 * };
 * ```
 *
 * When binding the base class in Python, you must indicate to nanobind that
 * this type uses intrusive reference counting and expose the ``set_self_py``
 * member. This must only be done once, as the attribute is automatically
 * inherited by subclasses.
 *
 * ```cpp
 * nb::class_<Object>(
 *   m, "Object",
 *   nb::intrusive_ptr<Object>(
 *       [](Object *o, PyObject *po) noexcept { o->set_self_py(po); }));
 * ```
 *
 * Also, somewhere in your binding initialization code, you must call
 *
 * ```cpp
 * nb::intrusive_init(
 *     [](PyObject *o) noexcept {
 *         nb::gil_scoped_acquire guard;
 *         if (guard.is_valid())
 *             Py_INCREF(o);
 *     },
 *     [](PyObject *o) noexcept {
 *         nb::gil_scoped_acquire guard;
 *         if (guard.is_valid())
 *             Py_DECREF(o);
 *     });
 * ```
 *
 * For this all to compile, a single one of your .cpp files must include this
 * header file from somewhere as follows:
 *
 * ```cpp
 * #include <nanobind/intrusive/counter.inl>
 * ```
 *
 * Calling the ``inc_ref()`` and ``dec_ref()`` members many times throughout
 * the code can quickly become tedious. Nanobind also ships with a ``ref<T>``
 * RAII helper class to help with this.
 *
 * ```cpp
 * #include <nanobind/intrusive/ref.h>
 *
 * {
 *     ref<MyObject> x = new MyObject(); // <-- assigment to ref<..> automatically calls inc_ref()
 *     x->func(); // ref<..> can be used like a normal pointer
 * } // <-- Destruction of ref<..> calls dec_ref(), deleting the instance in this example.
 * ```
 *
 * When the file ``nanobind/intrusive/ref.h`` is included following
 * ``nanobind/nanobind.h``, it also exposes a custom type caster to bind
 * functions taking or returning ``ref<T>``-typed values.
 */
struct NB_INTRUSIVE_EXPORT intrusive_counter {
public:
    intrusive_counter() noexcept = default;

    // The counter value is not affected by copy/move assignment/construction
    intrusive_counter(const intrusive_counter &) noexcept { }
    intrusive_counter(intrusive_counter &&) noexcept { }
    intrusive_counter &operator=(const intrusive_counter &) noexcept { return *this; }
    intrusive_counter &operator=(intrusive_counter &&) noexcept { return *this; }

    /// Increase the object's reference count
    void inc_ref() const noexcept;

    /// Decrease the object's reference count, return ``true`` if it should be deallocated
    bool dec_ref() const noexcept;

    /// Return the Python object associated with this instance (or NULL)
    PyObject *self_py() const noexcept;

    /// Set the Python object associated with this instance
    void set_self_py(PyObject *self) noexcept;

    /**
     * \brief Return the reference count of the object while owned by C++.
     * When ownership has transferred to Python, this function returns 0.
     */
    size_t ref_count() const noexcept;

protected:
    /**
     * \brief Mutable counter. Note that the value ``1`` actually encodes
     * a zero reference count (see the file ``counter.inl`` for details).
     */
    mutable uintptr_t m_state = 1;
};

static_assert(
    sizeof(intrusive_counter) == sizeof(void *),
    "The intrusive_counter class should always have the same size as a pointer.");

/// Reference-counted base type of an object hierarchy
class NB_INTRUSIVE_EXPORT intrusive_base {
public:
    /// Increase the object's reference count
    void inc_ref() const noexcept { m_ref_count.inc_ref(); }

    /// Decrease the object's reference count, return ``true`` if it should be deallocated
    bool dec_ref() const noexcept { return m_ref_count.dec_ref(); }

    /// Set the Python object associated with this instance
    void set_self_py(PyObject *self) noexcept { m_ref_count.set_self_py(self); }

    /// Return the Python object associated with this instance (or NULL)
    PyObject *self_py() const noexcept { return m_ref_count.self_py(); }

    /// Return the reference count while owned by C++ (0 once ownership has transferred to Python)
    size_t ref_count() const noexcept { return m_ref_count.ref_count(); }

    /// Virtual destructor
    virtual ~intrusive_base() = default;

private:
    mutable intrusive_counter m_ref_count;
};

/**
 * \brief Increase the reference count of an intrusively reference-counted
 * object ``o`` if ``o`` is non-NULL.
 */
inline void inc_ref(const intrusive_base *o) noexcept {
    if (o)
        o->inc_ref();
}

/**
 * \brief Decrease the reference count and potentially delete an intrusively
 * reference-counted object ``o`` if ``o`` is non-NULL.
 */
inline void dec_ref(const intrusive_base *o) noexcept {
    if (o && o->dec_ref())
        delete o;
}

/**
 * \brief Install Python reference counting handlers
 *
 * The ``intrusive_counter`` class is designed so that the dependency on Python is
 * *optional*: the code compiles in ordinary C++ projects, in which case the
 * Python reference counting functionality will simply not be used.
 *
 * Python binding code must invoke ``intrusive_init`` once to supply two
 * functions that increase and decrease the reference count of a Python object,
 * while ensuring that the GIL is held.
 */
extern NB_INTRUSIVE_EXPORT
void intrusive_init(void (*intrusive_inc_ref_py)(PyObject *) noexcept,
                    void (*intrusive_dec_ref_py)(PyObject *) noexcept);

NAMESPACE_END(nanobind)
