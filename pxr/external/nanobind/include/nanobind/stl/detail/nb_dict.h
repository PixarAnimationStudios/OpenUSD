/*
    nanobind/stl/detail/nb_dict.h: base class of dict casters

    Copyright (c) 2022 Matej Ferencevic and Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/nanobind.h>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <typename Dict, typename Key, typename Val> struct dict_caster {
    NB_TYPE_CASTER(Dict, io_name("collections.abc.Mapping", "dict") +
                             const_name("[") + make_caster<Key>::Name +
                             const_name(", ") + make_caster<Val>::Name +
                             const_name("]"))

    using KeyCaster = make_caster<Key>;
    using ValCaster = make_caster<Val>;

    bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept {
        size_t size;
        PyObject *temp;

        // The snapshot is unique to this thread. All return values and return
        // parameters are zero/NULL in the case of a failure.
        PyObject **o = NB_CALL(mapping_get)(src.ptr(), &size, &temp);

        value.clear();

        uint32_t flags_key = flags_for_local_caster<Key>(flags),
                 flags_val = flags_for_local_caster<Val>(flags);

        KeyCaster key_caster;
        ValCaster val_caster;
        bool success = o != nullptr;

        for (size_t i = 0; i < size; ++i) {
            if (!key_caster.from_python(o[2 * i], flags_key, cleanup) ||
                !key_caster.template can_cast<Key>() ||
                !val_caster.from_python(o[2 * i + 1], flags_val, cleanup) ||
                !val_caster.template can_cast<Val>()) {
                success = false;
                break;
            }

            value.emplace(key_caster.operator cast_t<Key>(),
                          val_caster.operator cast_t<Val>());
        }

        Py_XDECREF(temp);

        return success;
    }

    template <typename T>
    static handle from_cpp(T &&src, rv_policy policy,
                           cleanup_list *cleanup) noexcept {
        dict ret;

        if (ret.is_valid()) {
            for (auto &&item : src) {
                object k = steal(KeyCaster::from_cpp(
                    forward_like_<T>(item.first), policy, cleanup));
                object e = steal(ValCaster::from_cpp(
                    forward_like_<T>(item.second), policy, cleanup));

                if (!k.is_valid() || !e.is_valid() ||
                    PyDict_SetItem(ret.ptr(), k.ptr(), e.ptr()) != 0) {
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
