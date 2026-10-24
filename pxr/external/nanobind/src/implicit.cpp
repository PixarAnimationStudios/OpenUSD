/*
    src/implicit.cpp: functions for registering implicit conversions

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#include <nanobind/trampoline.h>
#include "nb_internals.h"

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

// Note: nb_type_get_implicit() reads the conversion arrays grown below without
// holding the internals lock. This is safe because conversions are registered
// while binding a type, which never overlaps with concurrent use of that type.

void implicitly_convertible(nb_internals *p, const std::type_info *dst,
                            void *src, bool is_predicate) noexcept {
    type_data *t = nb_type_c2p(p, dst);
    check(t, "nanobind::detail::implicitly_convertible(dst=%s): "
             "destination type unknown!", type_name(dst));

    lock_internals guard(p);
    size_t size = 0;

    if (!(t->flags & (uint32_t) type_flags_internal::has_implicit_conversions)) {
        t->implicit.cpp = nullptr;
        t->implicit.py = nullptr;
        t->flags |= (uint32_t) type_flags_internal::has_implicit_conversions;
    }

    // Grow the null-terminated array selected by 'is_predicate' by one entry
    void **entries = is_predicate ? (void **) t->implicit.py
                                  : (void **) t->implicit.cpp;
    while (entries && entries[size])
        size++;

    void **data = (void **) PyMem_Malloc(sizeof(void *) * (size + 2));
    check(data, "nanobind::detail::implicitly_convertible(): out of memory!");

    if (size)
        memcpy(data, entries, size * sizeof(void *));
    data[size] = src;
    data[size + 1] = nullptr;
    PyMem_Free(entries);

    if (is_predicate)
        t->implicit.py = (decltype(t->implicit.py)) data;
    else
        t->implicit.cpp = (decltype(t->implicit.cpp)) data;
}

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
