/*
    nanobind/eval.h: Support for evaluating Python expressions and
                     statements from strings

    Adapted by Nico Schlömer from pybind11's eval.h.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/nanobind.h>

NAMESPACE_BEGIN(NB_NAMESPACE)

enum eval_mode {
    // Evaluate a string containing an isolated expression
    eval_expr = Py_eval_input,

    // Evaluate a string containing a single statement. Returns \c none
    eval_single_statement = Py_single_input,

    // Evaluate a string containing a sequence of statement. Returns \c none
    eval_statements = Py_file_input
};

template <eval_mode start = eval_expr>
object eval(const str &expr, handle global = handle(), handle local = handle()) {
    dict global_default;
    if (!global.is_valid()) {
        global_default = globals();
        global = global_default;
    }

    if (!local.is_valid())
        local = global;

    // This used to be PyRun_String, but that function isn't in the stable ABI.
    object codeobj = steal(Py_CompileString(expr.c_str(), "<string>", start));
    if (!codeobj.is_valid())
        detail::raise_python_error();

    PyObject *result = PyEval_EvalCode(codeobj.ptr(), global.ptr(), local.ptr());
    if (!result)
        detail::raise_python_error();

    return steal(result);
}

namespace detail { inline import_cache textwrap_dedent { "textwrap", "dedent" }; }

template <eval_mode start = eval_expr, size_t N>
object eval(const char (&s)[N], handle global = handle(), handle local = handle()) {
    // Support raw string literals by removing common leading whitespace
    str expr = (s[0] == '\n') ? str(detail::textwrap_dedent.get()(s)) : str(s);
    return eval<start>(expr, global, local);
}

inline void exec(const str &expr, handle global = handle(), handle local = handle()) {
    eval<eval_statements>(expr, global, local);
}

template <size_t N>
void exec(const char (&s)[N], handle global = handle(), handle local = handle()) {
    eval<eval_statements>(s, global, local);
}

NAMESPACE_END(NB_NAMESPACE)
