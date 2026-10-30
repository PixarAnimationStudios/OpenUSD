/*
    nanobind/stl/bind_vector.h: Automatic creation of bindings for vector-style containers

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/nanobind.h>
#include <nanobind/operators.h>
#include <nanobind/make_iterator.h>
#include <nanobind/stl/detail/traits.h>
#include <vector>
#include <algorithm>
#include <memory>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

inline size_t wrap(Py_ssize_t i, size_t n) {
    if (i < 0)
        i += (Py_ssize_t) n;

    if (i < 0 || (size_t) i >= n)
        throw index_error();

    return (size_t) i;
}

template <> struct iterator_access<typename std::vector<bool>::iterator> {
    using result_type = bool;
    result_type operator()(typename std::vector<bool>::iterator &it) const { return *it; }
};

/// Render the repr of a sequence in Python list syntax
NB_NOINLINE inline PyObject *repr_list(PyObject *o) {
    object name = steal(NB_CALL(nb_inst_name)(o));
    list items;
    for (size_t i = 0, l = len(o); i < l; ++i)
        items.append(repr(handle(o)[i]));
    object body = steal(raise_if_null(
        PyUnicode_Join(str(", ").ptr(), items.ptr())));
    return raise_if_null(
        PyUnicode_FromFormat("%U([%U])", name.ptr(), body.ptr()));
}

NAMESPACE_END(detail)


template <typename Vector,
          rv_policy::value Policy = rv_policy::automatic_reference_v,
          typename... Args>
class_<Vector> bind_vector(handle scope, const char *name, Args &&...args) {
    using ValueRef = typename detail::iterator_access<typename Vector::iterator>::result_type;
    using Value = std::decay_t<ValueRef>;

    static_assert(
        !detail::is_base_caster_v<detail::make_caster<Value>> ||
        detail::is_copy_constructible_v<Value> ||
        (Policy != rv_policy::automatic_reference &&
         Policy != rv_policy::copy),
        "bind_vector(): the generated __getitem__ would copy elements, so the "
        "element type must be copy-constructible");

    handle cl_cur = type<Vector>();
    if (cl_cur.is_valid()) {
        // Binding already exists, don't re-create
        return borrow<class_<Vector>>(cl_cur);
    }

    // The nb::lock_self() and nb::arg().lock() annotations protect the C++
    // container from concurrent modification in free-threaded builds. They have
    // no effect in GIL-protected Python.
    auto cl = class_<Vector>(scope, name, std::forward<Args>(args)...)
        .def(init<>(), "Default constructor")

        .def("__len__", [](const Vector &v) { return v.size(); }, lock_self())

        .def("__bool__",
             [](const Vector &v) { return !v.empty(); },
             lock_self(),
             "Check whether the vector is nonempty")

        // __repr__ needs no annotation of its own. ``repr_list`` reaches the
        // vector through the locked ``__len__`` and ``__getitem__`` methods.
        .def("__repr__",
             [](handle_t<Vector> h) {
                return steal<str>(detail::repr_list(h.ptr()));
             })

        // The index-based iterator remains valid when the vector is modified
        // during iteration, whether from the loop body or by another thread
        .def("__iter__",
             [](handle_t<Vector> h) {
                 return detail::make_index_iterator<Policy, Vector>(
                     type<Vector>(), "Iterator", h);
             })

        .def("__getitem__",
             [](Vector &v, Py_ssize_t i) -> ValueRef {
                 return v[detail::wrap(i, v.size())];
             }, rv_policy::policy_tag<Policy>(), lock_self())

        .def("clear", [](Vector &v) { v.clear(); },
             lock_self(),
             "Remove all items from list.");

    if constexpr (detail::is_copy_constructible_v<Value>) {
        cl.def(init<const Vector &>(), arg().lock(),
               "Copy constructor");

        cl.def("__init__", [](Vector *v, typed<iterable, Value> seq) {
            new (v) Vector();
            try {
                v->reserve(len_hint(seq));
                for (handle h : seq)
                    v->push_back(cast<Value>(h));
            } catch (...) {
                v->~Vector();
                throw;
            }
        }, "Construct from an iterable object");

        implicitly_convertible<iterable, Vector>();

        cl.def("append",
               [](Vector &v, const Value &value) { v.push_back(value); },
               lock_self(),
               "Append ``arg`` to the end of the list.")

          .def("insert",
               [](Vector &v, Py_ssize_t i, const Value &x) {
                   if (i < 0)
                       i += (Py_ssize_t) v.size();
                   if (i < 0 || (size_t) i > v.size())
                       throw index_error();
                   v.insert(v.begin() + i, x);
               },
               lock_self(),
               "Insert object ``arg1`` before index ``arg0``.")

           .def("pop",
                [](Vector &v, Py_ssize_t i) {
                    size_t index = detail::wrap(i, v.size());
                    Value result = std::move(v[index]);
                    v.erase(v.begin() + (ptrdiff_t) index);
                    return result;
                },
                arg("index") = -1, lock_self(),
                "Remove and return item at ``index`` (default last).")

          .def("extend",
               [](Vector &v, const Vector &src) {
                   if (&src == &v) {
                       // Self-extension: inserting [v.begin(), v.end()) into v
                       // itself violates the standard's precondition (the
                       // source range must not lie inside the container) and
                       // is undefined behavior. Reserve and append by index.
                       size_t n = v.size();
                       v.reserve(2 * n);
                       for (size_t i = 0; i < n; ++i)
                           v.push_back(v[i]);
                   } else {
                       v.insert(v.end(), src.begin(), src.end());
                   }
               },
               lock_self(), arg().lock(),
               "Extend ``self`` by appending elements from ``arg``.")

          .def("__setitem__",
               [](Vector &v, Py_ssize_t i, const Value &value) {
                   v[detail::wrap(i, v.size())] = value;
               }, lock_self())

          .def("__delitem__",
               [](Vector &v, Py_ssize_t i) {
                   v.erase(v.begin() + (ptrdiff_t) detail::wrap(i, (size_t) v.size()));
               }, lock_self())

          .def("__getitem__",
               [](const Vector &v, const slice &slice) -> Vector * {
                   auto [start, stop, step, length] = slice.compute(v.size());
                   auto seq = std::make_unique<Vector>();
                   seq->reserve(length);

                   for (size_t i = 0; i < length; ++i) {
                       seq->push_back(v[(size_t) start]);
                       start += step;
                   }

                   return seq.release();
               }, lock_self())

          .def("__setitem__",
               [](Vector &v, const slice &slice, const Vector &value) {
                   auto [start, stop, step, length] = slice.compute(v.size());

                   if (length != value.size())
                       throw index_error(
                           "The left and right hand side of the slice "
                           "assignment have mismatched sizes!");

                   // Copy the RHS first when assigning a slice from the
                   // container itself; otherwise the loop would read elements
                   // it has already overwritten (e.g. ``v[::-1] = v``).
                   if (&value == &v) {
                       Vector copy(value);
                       for (size_t i = 0; i < length; ++i) {
                           v[(size_t) start] = copy[i];
                           start += step;
                       }
                   } else {
                       for (size_t i = 0; i < length; ++i) {
                           v[(size_t) start] = value[i];
                           start += step;
                       }
                   }
               }, lock_self(), arg(), arg().lock())

          .def("__delitem__",
               [](Vector &v, const slice &slice) {
                   auto [start, stop, step, length] = slice.compute(v.size());
                   if (length == 0)
                       return;

                   stop = start + ((Py_ssize_t) length - 1) * step;
                   if (start > stop) {
                       std::swap(start, stop);
                       step = -step;
                   }

                   if (step == 1) {
                       v.erase(v.begin() + start, v.begin() + stop + 1);
                   } else {
                       for (size_t i = 0; i < length; ++i) {
                           v.erase(v.begin() + stop);
                           stop -= step;
                       }
                   }
               }, lock_self());
    }

    if constexpr (detail::is_equality_comparable_v<Value>) {
        cl.def(self == self, sig("def __eq__(self, arg: object, /) -> bool"),
               lock_self(), arg().lock())
          .def(self != self, sig("def __ne__(self, arg: object, /) -> bool"),
               lock_self(), arg().lock())

          .def("__contains__",
               [](const Vector &v, const Value &x) {
                   return std::find(v.begin(), v.end(), x) != v.end();
               }, lock_self())

          .def("__contains__", // fallback for incompatible types
               [](const Vector &, handle) { return false; })

          .def("count",
               [](const Vector &v, const Value &x) {
                   return std::count(v.begin(), v.end(), x);
               }, lock_self(),
               "Return number of occurrences of ``arg``.")

          .def("remove",
               [](Vector &v, const Value &x) {
                   auto p = std::find(v.begin(), v.end(), x);
                   if (p != v.end())
                       v.erase(p);
                   else
                       throw value_error();
               },
               lock_self(),
               "Remove first occurrence of ``arg``.");
    }

    return cl;
}

NAMESPACE_END(NB_NAMESPACE)
