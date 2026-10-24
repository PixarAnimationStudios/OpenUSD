/*
    nanobind/stl/unordered_set.h: type caster for std::unordered_set<...>

    Copyright (c) 2022 Raymond Yun Fei and Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include "detail/nb_set.h"
#include <unordered_set>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <typename Key, typename Hash, typename Compare, typename Alloc>
struct type_caster<std::unordered_set<Key, Hash, Compare, Alloc>>
    : set_caster<std::unordered_set<Key, Hash, Compare, Alloc>, Key> {
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
