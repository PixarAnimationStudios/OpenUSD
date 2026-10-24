/*
    src/nb_backend.cpp: entry point of the backend module

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#include "nb_internals.h"
#include <algorithm>

#if !defined(NB_BACKEND_NAME)
#  error "nb_backend.cpp requires the NB_BACKEND_NAME definition " \
         "(use the nanobind_add_backend() CMake command)"
#endif

#define NB_BACKEND_NAME_STR NB_TOSTRING(NB_BACKEND_NAME)

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

/// Backend ABI for export
static const nb_backend_table nb_backend_export = {
    nb_backend_slot_count, NB_BACKEND_ABI_MINOR, { 0, 0, 0, 0 },
#define NB_SLOT(ret, name, args) name,
#include <nanobind/nb_backend_slots.h>
};

static PyObject *nb_backend_fill(PyObject *, PyObject *args) noexcept {
    int abi_major;
    const char *tag;
    PyObject *capsule;

    if (!PyArg_ParseTuple(args, "isO:fill", &abi_major, &tag, &capsule))
        return nullptr;

    // Complain if the caller's platform ABI tag doesn't match ours
    if (strcmp(tag, NB_PLATFORM_ABI_TAG) != 0) {
        PyErr_Format(PyExc_ImportError,
                     "nanobind: this extension was built for the platform "
                     "ABI \"%s\", but the backend module '%s' serves "
                     "\"%s\". You may need a backend module built with a "
                     "matching compiler/standard library, or this may be a "
                     "development-version mismatch.",
                     tag, NB_BACKEND_NAME_STR, NB_PLATFORM_ABI_TAG);
        return nullptr;
    }

    // Gate on ABI major version
    if (abi_major != NB_BACKEND_ABI_MAJOR) {
        PyErr_Format(PyExc_ImportError,
                     "nanobind: this extension requires backend ABI major "
                     "version %i, but the backend module '%s' serves major "
                     "version %i.",
                     abi_major, NB_BACKEND_NAME_STR, (int) NB_BACKEND_ABI_MAJOR);
        return nullptr;
    }

    // The capsule name repeats the platform ABI tag; unpacking it doubles
    // as the authoritative check
    nb_backend_table *table =
        (nb_backend_table *) PyCapsule_GetPointer(capsule, NB_PLATFORM_ABI_TAG);
    if (!table)
        return nullptr;

    // Reject callers newer than this backend. A rejected extension never
    // loads, which lets the slot implementations trust the ABI tag.
    if (table->abi_minor > NB_BACKEND_ABI_MINOR) {
        PyErr_Format(PyExc_ImportError,
                     "nanobind: this extension requires backend ABI version "
                     "%i.%i, but the backend module '%s' only offers "
                     "%i.%i. Upgrade it via 'pip install -U nanobind-backend' "
                     "(or the equivalent for a custom backend module).",
                     abi_major, (int) table->abi_minor,
                     NB_BACKEND_NAME_STR,
                     (int) NB_BACKEND_ABI_MAJOR, (int) NB_BACKEND_ABI_MINOR);
        return nullptr;
    }

    // All good, fill the caller's backend ABI table up to table->slot_count
    constexpr size_t slot_offset = offsetof(nb_backend_table, raise_v);
    size_t n = std::min<size_t>(table->slot_count, nb_backend_slot_count);
    memcpy((char *) table + slot_offset,
           (const char *) &nb_backend_export + slot_offset, n * sizeof(void *));

    return none_ref();
}

static PyMethodDef nb_backend_methods[] = {
    { "fill", nb_backend_fill, METH_VARARGS,
      "Fill a split-mode extension's function table" },
    { nullptr, nullptr, 0, nullptr }
};

static PyModuleDef_Slot nb_backend_slots[] = {
#if defined(NB_FREE_THREADED)
    { Py_mod_gil, Py_MOD_GIL_NOT_USED },
#endif
#if PY_VERSION_HEX >= 0x030C0000
    { Py_mod_multiple_interpreters,
      Py_MOD_MULTIPLE_INTERPRETERS_NOT_SUPPORTED },
#endif
    { 0, nullptr }
};

static PyModuleDef nb_backend_def = {
    PyModuleDef_HEAD_INIT,
    NB_BACKEND_NAME_STR,
    "nanobind backend module. See "
    "https://nanobind.readthedocs.io/en/latest/split_mode.html for details.",
    0, nb_backend_methods, nb_backend_slots, nullptr, nullptr, nullptr
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)

extern "C" NB_EXPORT PyObject *NB_CONCAT(PyInit_, NB_BACKEND_NAME)(void);
extern "C" PyObject *NB_CONCAT(PyInit_, NB_BACKEND_NAME)(void) {
    return PyModuleDef_Init(&nanobind::detail::nb_backend_def);
}
