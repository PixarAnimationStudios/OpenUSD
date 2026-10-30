/*
    nanobind/stl/optional.h: type caster for std::optional<...>

    Copyright (c) 2022 Yoshiki Matsuda and Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include "detail/nb_optional.h"
#include <optional>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <typename T> struct remove_opt_mono<std::optional<T>>
    : remove_opt_mono<T> { };

template <typename T>
struct type_caster<std::optional<T>> : optional_caster<std::optional<T>> {};

template <> struct type_caster<std::nullopt_t> : none_caster<std::nullopt_t> { };

template <typename T>
struct has_arg_defaults<std::optional<T>> : std::true_type {};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
