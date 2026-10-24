/*
    nanobind/stl/filesystem.h: type caster for std::filesystem

    Copyright (c) 2023 Qingnan Zhou and Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/
#pragma once

#include <nanobind/nanobind.h>

#include <filesystem>
#include <string>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

inline import_cache pathlib_path { "pathlib", "Path" };

template <>
struct type_caster<std::filesystem::path> {

    static handle from_cpp(const std::filesystem::path &path, rv_policy,
                           cleanup_list *) noexcept {
        str py_str = to_py_str(path.native());
        if (py_str.is_valid()) {
            try {
                return pathlib_path.get()(py_str).release();
            } catch (python_error &e) {
                e.restore();
            }
        }
        return handle();
    }

    template <typename Char = typename std::filesystem::path::value_type>
    bool from_python(handle src, uint32_t, cleanup_list *) noexcept {
        bool success = false;

        // PyUnicode_FSConverter and PyUnicode_FSDecoder normally take care of
        // calling PyOS_FSPath themselves, but that's broken on PyPy (see PyPy
        // issue #3168) so we do it ourselves instead.
        PyObject *buf = PyOS_FSPath(src.ptr());
        if (buf) {
            PyObject *native = nullptr;
            if constexpr (std::is_same_v<Char, char>) {
                if (PyUnicode_FSConverter(buf, &native)) {
                    if (char* s = PyBytes_AsString(native)) {
                        value = s; // Points to internal buffer, no need to free
                        success = true;
                    }
                }
            } else {
                if (PyUnicode_FSDecoder(buf, &native)) {
                    if (wchar_t *s = PyUnicode_AsWideCharString(native, nullptr)) {
                        value = s;
                        PyMem_Free(s); // New string, must free
                        success = true;
                    }
                }
            }

            Py_DECREF(buf);
            Py_XDECREF(native);
        }

        if (!success)
            PyErr_Clear();

        return success;
    }

    NB_TYPE_CASTER(std::filesystem::path, io_name("str | os.PathLike", "pathlib.Path"))

private:
    static str to_py_str(const std::string &s) {
        return steal<str>(
            PyUnicode_DecodeFSDefaultAndSize(s.c_str(), (Py_ssize_t) s.size()));
    }

    static str to_py_str(const std::wstring &s) {
        return steal<str>(
            PyUnicode_FromWideChar(s.c_str(), (Py_ssize_t) s.size()));
    }
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
