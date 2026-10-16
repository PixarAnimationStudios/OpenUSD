/*
    nanobind/stl/unordered_map.h: type caster for std::unordered_map<...>

    Copyright (c) 2022 Matej Ferencevic and Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include "detail/nb_dict.h"
#include <unordered_map>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <typename Key, typename T, typename Compare, typename Alloc>
struct type_caster<std::unordered_map<Key, T, Compare, Alloc>>
 : dict_caster<std::unordered_map<Key, T, Compare, Alloc>, Key, T> { };

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
