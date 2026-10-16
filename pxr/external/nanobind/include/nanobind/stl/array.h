/*
    nanobind/stl/array.h: type caster for std::array<...>

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include "detail/nb_array.h"
#include <array>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <typename Type, size_t Size> struct type_caster<std::array<Type, Size>>
 : array_caster<std::array<Type, Size>, Type, Size> { };

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
