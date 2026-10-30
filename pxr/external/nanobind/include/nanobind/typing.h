/*
    nanobind/typing.h: Optional typing-related functionality

    Copyright (c) 2024 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/nanobind.h>

NAMESPACE_BEGIN(NB_NAMESPACE)

inline module_ typing() { return module_::import_("typing"); }

template <typename... Args>
object any_type() { return typing().attr("Any"); }

template <typename... Args>
object type_var(Args&&... args) {
    return typing().attr("TypeVar")((detail::forward_t<Args>) args...);
}

template <typename... Args>
object type_var_tuple(Args&&... args) {
    return typing().attr("TypeVarTuple")((detail::forward_t<Args>) args...);
}

template <typename... Args>
object param_spec(Args&&... args) {
    return typing().attr("ParamSpec")((detail::forward_t<Args>) args...);
}

NAMESPACE_END(NB_NAMESPACE)
