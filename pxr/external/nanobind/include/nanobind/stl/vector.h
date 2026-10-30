/*
    nanobind/stl/vector.h: type caster for std::vector<...>

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include "detail/nb_list.h"
#include <vector>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <typename Type, typename Alloc> struct type_caster<std::vector<Type, Alloc>>
 : list_caster<std::vector<Type, Alloc>, Type> { };

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
