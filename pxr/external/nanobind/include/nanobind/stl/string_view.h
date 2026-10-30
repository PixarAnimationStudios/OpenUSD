/*
    nanobind/stl/string_view.h: type caster for std::string_view

    Copyright (c) 2022 Qingnan Zhou and Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/nanobind.h>
#include <string_view>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <> struct type_caster<std::string_view> {
    NB_TYPE_CASTER(std::string_view, const_name("str"))

    bool from_python(handle src, uint32_t, cleanup_list *) noexcept {
        if (!str_check(src.ptr()))
            return false;
        Py_ssize_t size;
        const char *str = PyUnicode_AsUTF8AndSize(src.ptr(), &size);
        if (!str) {
            PyErr_Clear();
            return false;
        }
        value = std::string_view(str, (size_t) size);
        return true;
    }

    static handle from_cpp(std::string_view value, rv_policy,
                           cleanup_list *) noexcept {
        return PyUnicode_FromStringAndSize(value.data(), (Py_ssize_t) value.size());
    }
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
