/*
    nanobind/stl/string.h: type caster for std::string

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/nanobind.h>
#include <string>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <> struct type_caster<std::string> {
    NB_TYPE_CASTER(std::string, const_name("str"))

    bool from_python(handle src, uint32_t, cleanup_list *) noexcept {
        if (!str_check(src.ptr()))
            return false;
        Py_ssize_t size;
        const char *str = PyUnicode_AsUTF8AndSize(src.ptr(), &size);
        if (!str) {
            PyErr_Clear();
            return false;
        }
        value = std::string(str, (size_t) size);
        return true;
    }

    static handle from_cpp(const std::string &value, rv_policy,
                           cleanup_list *) noexcept {
        return PyUnicode_FromStringAndSize(value.c_str(), (Py_ssize_t) value.size());
    }
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
