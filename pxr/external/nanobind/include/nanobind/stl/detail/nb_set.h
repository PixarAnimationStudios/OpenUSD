/*
    nanobind/stl/detail/nb_set.h: base class of set casters

    Copyright (c) 2022 Raymond Yun Fei and Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/nanobind.h>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <typename Set, typename Key> struct set_caster {
    NB_TYPE_CASTER(Set, io_name("collections.abc.Set", "set") +
                            const_name("[") + make_caster<Key>::Name +
                            const_name("]"))

    using Caster = make_caster<Key>;

    bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept {
        value.clear();

        // Cheap pre-check so that a failed PyObject_GetIter on a
        // non-iterable overload candidate never raises and clears
        if (!iterable_check(src.ptr()))
            return false;

        PyObject *iter = PyObject_GetIter(src.ptr());
        if (NB_UNLIKELY(!iter)) {
            PyErr_Clear();
            return false;
        }

        bool success = true;
        Caster key_caster;
        PyObject *key;

        flags = flags_for_local_caster<Key>(flags);

        while ((key = PyIter_Next(iter)) != nullptr) {
            success &= (key_caster.from_python(key, flags, cleanup) &&
                        key_caster.template can_cast<Key>());

            if (!success) {
                Py_DECREF(key);
                break;
            }

            value.emplace(key_caster.operator cast_t<Key>());
            Py_DECREF(key);
        }

        if (PyErr_Occurred()) {
            PyErr_Clear();
            success = false;
        }

        Py_DECREF(iter);

        return success;
    }

    template <typename T>
    static handle from_cpp(T &&src, rv_policy policy,
                           cleanup_list *cleanup) noexcept {
        object ret = steal(PySet_New(nullptr));

        if (ret.is_valid()) {
            for (auto&& key : src) {
                object k = steal(
                    Caster::from_cpp(forward_like_<T>(key), policy, cleanup));

                if (!k.is_valid() || PySet_Add(ret.ptr(), k.ptr()) != 0) {
                    ret.reset();
                    break;
                }
            }
        }

        return ret.release();
    }
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
