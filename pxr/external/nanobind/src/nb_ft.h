/*
    src/nb_ft.h: implementation details related to free-threaded Python

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

/* Nanobind immortalizes type objects, enums, and function objects on FT builds.
   Reference counting operations on these can be completely skipped when it is
   known that the target object is immortal. On non-FT builds, these forward
   to Py_{INC,DEC}REF/Py_CLEAR */
#if defined(Py_GIL_DISABLED)
#  define NB_INCREF_TYPE(o) ((void) (o))
#  define NB_DECREF_TYPE(o) ((void) (o))
#  define NB_INCREF_ENUM(o) ((void) (o))
#  define NB_DECREF_ENUM(o) ((void) (o))
#  define NB_INCREF_FUNC(o) ((void) (o))
#  define NB_DECREF_FUNC(o) ((void) (o))
#  define NB_CLEAR_FUNC(o) ((o) = nullptr)
#else
#  define NB_INCREF_TYPE(o) Py_INCREF(o)
#  define NB_DECREF_TYPE(o) Py_DECREF(o)
#  define NB_INCREF_ENUM(o) Py_INCREF(o)
#  define NB_DECREF_ENUM(o) Py_DECREF(o)
#  define NB_INCREF_FUNC(o) Py_INCREF(o)
#  define NB_DECREF_FUNC(o) Py_DECREF(o)
#  define NB_CLEAR_FUNC(o) Py_CLEAR(o)
#endif

#if !defined(Py_GIL_DISABLED)
/// Trivial implementations for non-free-threaded Python
inline void make_immortal(PyObject *) noexcept { }
inline void nb_enable_try_inc_ref(PyObject *) noexcept { }
inline bool nb_try_inc_ref(PyObject *obj) noexcept {
    if (Py_REFCNT(obj) > 0) {
        Py_INCREF(obj);
        return true;
    }
    return false;
}
inline void nb_resurrect(PyObject *obj) noexcept {
    Py_SET_REFCNT(obj, 1);
}
#else
extern void make_immortal(PyObject *op) noexcept;

#if PY_VERSION_HEX >= 0x030E00A5
/// Sufficiently recent CPython versions provide an API for the following operations
inline void nb_enable_try_inc_ref(PyObject *obj) noexcept {
    PyUnstable_EnableTryIncRef(obj);
}
inline bool nb_try_inc_ref(PyObject *obj) noexcept {
    return PyUnstable_TryIncRef(obj);
}
#else
/// Otherwise, nanabind ships with a low-level implementation
extern void nb_enable_try_inc_ref(PyObject *) noexcept;
extern bool nb_try_inc_ref(PyObject *obj) noexcept;
#endif
inline void nb_resurrect(PyObject *obj) noexcept {
    _Py_NewReference(obj);
}
#endif
