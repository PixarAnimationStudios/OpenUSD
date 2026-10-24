/*
    nanobind/stl/bind_map.h: Automatic creation of bindings for map-style containers

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/nanobind.h>
#include <nanobind/make_iterator.h>
#include <nanobind/operators.h>
#include <nanobind/stl/detail/traits.h>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <typename Map, typename Key, typename Value>
inline void map_set(Map &m, const Key &k, const Value &v) {
    if constexpr (detail::is_copy_assignable_v<Value>) {
        m[k] = v;
    } else {
        auto r = m.emplace(k, v);
        if (!r.second) {
            // Value is not copy-assignable. Erase and retry
            m.erase(r.first);
            m.emplace(k, v);
        }
    }
}

/// Render the repr of a mapping in Python dict syntax
NB_NOINLINE inline PyObject *repr_map(PyObject *o) {
    object name = steal(NB_CALL(nb_inst_name)(o));
    list items;
    for (handle kv : handle(o).attr("items")()) {
        object k = kv[0], v = kv[1],
               item = steal(raise_if_null(
                   PyUnicode_FromFormat("%R: %R", k.ptr(), v.ptr())));
        items.append(item);
    }
    object body = steal(raise_if_null(
        PyUnicode_Join(str(", ").ptr(), items.ptr())));
    return raise_if_null(
        PyUnicode_FromFormat("%U({%U})", name.ptr(), body.ptr()));
}

NAMESPACE_END(detail)

template <typename Map,
          rv_policy::value Policy = rv_policy::automatic_reference_v,
          typename... Args>
class_<Map> bind_map(handle scope, const char *name, Args &&...args) {
    using Key = typename Map::key_type;
    using Value = typename Map::mapped_type;

    // Element access strategies of the key, value, and item iterators
    using KeyAccess   = detail::iterator_key_access<typename Map::iterator>;
    using ValueAccess = detail::iterator_value_access<typename Map::iterator>;
    using ItemAccess  = detail::iterator_access<typename Map::iterator>;

    using ValueRef = typename ValueAccess::result_type;

    static_assert(
        !detail::is_base_caster_v<detail::make_caster<Value>> ||
        detail::is_copy_constructible_v<Value> ||
        (Policy != rv_policy::automatic_reference &&
         Policy != rv_policy::copy),
        "bind_map(): the generated __getitem__ would copy elements, so the "
        "value type must be copy-constructible");

    static_assert(
        detail::is_copy_constructible_v<Key>,
        "bind_map(): the generated iterators store a copy of the last "
        "visited key, so the key type must be copy-constructible");

    handle cl_cur = type<Map>();
    if (cl_cur.is_valid()) {
        // Binding already exists, don't re-create
        return borrow<class_<Map>>(cl_cur);
    }

    // The nb::lock_self() and nb::arg().lock() annotations protect the C++
    // container from concurrent modification in free-threaded builds. They have
    // no effect in GIL-protected Python.
    auto cl = class_<Map>(scope, name, std::forward<Args>(args)...)
        .def(init<>(),
             "Default constructor")

        .def("__len__", [](const Map &m) { return m.size(); }, lock_self())

        .def("__bool__",
             [](const Map &m) { return !m.empty(); },
             lock_self(),
             "Check whether the map is nonempty")

        // __repr__ needs no annotation of its own. ``repr_map`` reaches the
        // map through the bound ``items()`` view.
        .def("__repr__",
             [](handle_t<Map> h) {
                return steal<str>(detail::repr_map(h.ptr()));
             })

        .def("__contains__",
             [](const Map &m, const Key &k) { return m.find(k) != m.end(); },
             lock_self())

        .def("__contains__", // fallback for incompatible types
             [](const Map &, handle) { return false; })

        // The key-based iterator raises ``RuntimeError`` when it detects a
        // modification of the map, see ``make_cursor_iterator()``
        .def("__iter__",
             [](handle_t<Map> h) {
                 return detail::make_cursor_iterator<KeyAccess, Policy, Map>(
                     type<Map>(), "KeyIterator", h);
             })

        .def("__getitem__",
             [](Map &m, const Key &k) -> ValueRef {
                 auto it = m.find(k);
                 if (it == m.end())
                     throw key_error();
                 return (*it).second;
             }, rv_policy::policy_tag<Policy>(), lock_self())

        .def("__delitem__",
            [](Map &m, const Key &k) {
                auto it = m.find(k);
                if (it == m.end())
                    throw key_error();
                m.erase(it);
            }, lock_self())

        .def("clear", [](Map &m) { m.clear(); },
             lock_self(),
             "Remove all items");

    if constexpr (detail::is_copy_constructible_v<Map>) {
        cl.def(init<const Map &>(), arg().lock(), "Copy constructor");

        cl.def("__init__", [](Map *m, typed<dict, Key, Value> d) {
            new (m) Map();
            try {
                for (auto [k, v] : borrow<dict>(std::move(d)))
                    m->emplace(cast<Key>(k), cast<Value>(v));
            } catch (...) {
                m->~Map();
                throw;
            }
        }, "Construct from a dictionary");

        implicitly_convertible<dict, Map>();
    }

    // Assignment operator for copy-assignable/copy-constructible types
    if constexpr (detail::is_copy_assignable_v<Value> ||
                  detail::is_copy_constructible_v<Value>) {
        cl.def("__setitem__", [](Map &m, const Key &k, const Value &v) {
            detail::map_set<Map, Key, Value>(m, k, v);
        }, lock_self());

        cl.def("update", [](Map &m, const Map &m2) {
            // Updating a map with itself would be a no-op, but the underlying
            // map_set() may erase and re-emplace nodes; doing so while
            // iterating m2 == m leaves kv referencing freed storage (a
            // dangling-reference for non-copy-assignable values). Skip it.
            if (&m2 == &m)
                return;
            for (auto &kv : m2)
                detail::map_set<Map, Key, Value>(m, kv.first, kv.second);
        },
        lock_self(), arg().lock(),
        "Update the map with element from ``arg``");
    }

    if constexpr (detail::is_equality_comparable_v<Map>) {
        cl.def(self == self, sig("def __eq__(self, arg: object, /) -> bool"),
               lock_self(), arg().lock())
          .def(self != self, sig("def __ne__(self, arg: object, /) -> bool"),
               lock_self(), arg().lock());
    }

    // Item, value, and key views. Each holds a strong reference to the map
    // object, which keeps the map alive and provides a lock target for
    // free-threaded interpreters.
    struct View {
        object owner;
        Map &map() const { return *inst_ptr<Map>(owner); }
    };
    struct KeyView   : View { };
    struct ValueView : View { };
    struct ItemView  : View { };

    class_<ItemView>(cl, "ItemView")
        .def("__len__",
             [](ItemView &v) {
                 ft_object_guard guard(v.owner);
                 return v.map().size();
             })
        .def("__iter__",
             [](ItemView &v) {
                 return detail::make_cursor_iterator<ItemAccess, Policy, Map>(
                     type<Map>(), "ItemIterator", v.owner);
             });

    class_<KeyView>(cl, "KeyView")
        .def("__contains__",
             [](KeyView &v, const Key &k) {
                 ft_object_guard guard(v.owner);
                 Map &m = v.map();
                 return m.find(k) != m.end();
             })
        .def("__contains__", [](KeyView &, handle) { return false; })
        .def("__len__",
             [](KeyView &v) {
                 ft_object_guard guard(v.owner);
                 return v.map().size();
             })
        .def("__iter__",
             [](KeyView &v) {
                 return detail::make_cursor_iterator<KeyAccess, Policy, Map>(
                     type<Map>(), "KeyIterator", v.owner);
             });

    class_<ValueView>(cl, "ValueView")
        .def("__len__",
             [](ValueView &v) {
                 ft_object_guard guard(v.owner);
                 return v.map().size();
             })
        .def("__iter__",
             [](ValueView &v) {
                 return detail::make_cursor_iterator<ValueAccess, Policy, Map>(
                     type<Map>(), "ValueIterator", v.owner);
             });

    cl.def("keys",   [](handle_t<Map> h) { return new KeyView{{ borrow(h) }};   },
           "Returns an iterable view of the map's keys.");
    cl.def("values", [](handle_t<Map> h) { return new ValueView{{ borrow(h) }}; },
           "Returns an iterable view of the map's values.");
    cl.def("items",  [](handle_t<Map> h) { return new ItemView{{ borrow(h) }};  },
           "Returns an iterable view of the map's items.");

    return cl;
}

NAMESPACE_END(NB_NAMESPACE)
