/*
    nanobind/stl/detail/nb_list.h: base class of list casters

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/nanobind.h>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <typename List, typename Entry> struct list_caster {
    NB_TYPE_CASTER(List, io_name("collections.abc.Sequence", "list") +
                              const_name("[") + make_caster<Entry>::Name +
                              const_name("]"))

    using Caster = make_caster<Entry>;

    template <typename T> using has_reserve = decltype(std::declval<T>().reserve(0));

    bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept {
        size_t size;
        PyObject *temp;

        // Will initialize 'size' and 'temp'. All return values and
        // return parameters are zero/NULL in the case of a failure.
        PyObject **o = NB_CALL(seq_get)(src.ptr(), &size, &temp);

        value.clear();

        if constexpr (is_detected_v<has_reserve, List>)
            value.reserve(size);

        Caster caster;
        bool success = o != nullptr;

        flags = flags_for_local_caster<Entry>(flags);

        for (size_t i = 0; i < size; ++i) {
            if (!caster.from_python(o[i], flags, cleanup) ||
                !caster.template can_cast<Entry>()) {
                success = false;
                break;
            }

            value.push_back(caster.operator cast_t<Entry>());
        }

        Py_XDECREF(temp);

        return success;
    }

    template <typename T>
    static handle from_cpp(T &&src, rv_policy policy,
                           cleanup_list *cleanup) noexcept {
        seq_builder<false> b(src.size());

        if (NB_UNLIKELY(!b.valid()))
            return {};

        for (auto &&value : src) {
            if (NB_UNLIKELY(b.full())) // In case size() was inaccurate
                break;

            handle h = Caster::from_cpp(forward_like_<T>(value), policy, cleanup);
            if (NB_UNLIKELY(!h.is_valid()))
                break;

            b.put(h);
        }

        return b.commit();
    }
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
