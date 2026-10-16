/*
    nanobind/stl/optional.h: type caster for std::optional<...>

    Copyright (c) 2022 Yoshiki Matsuda and Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/nanobind.h>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <typename Optional, typename T = typename Optional::value_type>
struct optional_caster {
    using Caster = make_caster<T>;

    NB_TYPE_CASTER(Optional, optional_name(Caster::Name))

    bool from_python(handle src, uint32_t flags, cleanup_list* cleanup) noexcept {
        if (src.is_none()) {
            value.reset();
            return true;
        }

        Caster caster;
        if (!caster.from_python(src, flags_for_local_caster<T>(flags), cleanup) ||
            !caster.template can_cast<T>())
            return false;

        value.emplace(caster.operator cast_t<T>());

        return true;
    }

    template <typename T_>
    static handle from_cpp(T_ &&value, rv_policy policy, cleanup_list *cleanup) noexcept {
        if (!value)
            return none().release();

        return Caster::from_cpp(forward_like_<T_>(*value), policy, cleanup);
    }
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
