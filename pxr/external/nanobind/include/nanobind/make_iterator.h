/*
    nanobind/make_iterator.h: nb::make_[key,value_]iterator()

    This implementation is a port from pybind11 with minimal adjustments.

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/nanobind.h>
#include <nanobind/stl/pair.h>
#include <iterator>
#include <new>
#include <optional>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

/* There are a large number of apparently unused template arguments because
   each combination requires a separate nb::class_ registration. */
template <typename Access, rv_policy::value Policy, typename Iterator,
          typename Sentinel, typename ValueType, typename... Extra>
struct iterator_state {
    Iterator it;
    Sentinel end;
    bool first_or_done;
};

template <typename T>
struct remove_rvalue_ref { using type = T; };
template <typename T>
struct remove_rvalue_ref<T&&> { using type = T; };

// Note: these helpers take the iterator by non-const reference because some
// iterators in the wild can't be dereferenced when const.
template <typename Iterator> struct iterator_access {
    using result_type = decltype(*std::declval<Iterator &>());
    result_type operator()(Iterator &it) const { return *it; }
};

template <typename Iterator> struct iterator_key_access {
    // Note double parens in decltype((...)) to capture the value category
    // as well. This will be lvalue if the iterator's operator* returned an
    // lvalue reference, and xvalue if the iterator's operator* returned an
    // object (or rvalue reference but that's unlikely). decltype of an xvalue
    // produces T&&, but we want to return a value T from operator() in that
    // case, in order to avoid creating a Python object that references a
    // C++ temporary. Thus, pass the result through remove_rvalue_ref.
    using result_type = typename remove_rvalue_ref<
        decltype(((*std::declval<Iterator &>()).first))>::type;
    result_type operator()(Iterator &it) const { return (*it).first; }
};

template <typename Iterator> struct iterator_value_access {
    using result_type = typename remove_rvalue_ref<
        decltype(((*std::declval<Iterator &>()).second))>::type;
    result_type operator()(Iterator &it) const { return (*it).second; }
};

/// Return type of the generated __next__() method. This exists to raise
/// ``StopIteration`` in Python without (slow) C++ exceptions.
template <typename T, typename /* SFINAE */ = int> struct iter_result;

/// Element access yielded a reference, so refer to the element in place
template <typename T> struct iter_result<T, enable_if_t<std::is_reference_v<T>>> {
    std::remove_reference_t<T> *value = nullptr;

    iter_result() = default;
    iter_result(T v) : value(std::addressof(v)) { }

    bool has_value() const { return value != nullptr; }
    T get() { return static_cast<T>(*value); }
};

/// Element access yielded a temporary, so hold on to it
template <typename T> struct iter_result<T, enable_if_t<!std::is_reference_v<T>>> {
    std::optional<T> value;

    iter_result() = default;
    iter_result(T &&v) : value(std::move(v)) { }

    bool has_value() const { return value.has_value(); }
    T &&get() { return std::move(*value); }
};

template <typename T> struct type_caster<iter_result<T>> {
    static constexpr auto Name = make_caster<T>::Name;

    static handle from_cpp(iter_result<T> r, rv_policy policy,
                           cleanup_list *cleanup) noexcept {
        if (!r.has_value()) {
            PyErr_SetNone(PyExc_StopIteration);
            return { };
        }

        return make_caster<T>::from_cpp(r.get(), policy, cleanup);
    }
};

template <typename Access, rv_policy::value Policy, typename Iterator,
          typename Sentinel, typename ValueType, typename... Extra>
typed<iterator, ValueType> make_iterator_impl(handle scope, const char *name,
                                              Iterator first, Sentinel last,
                                              Extra &&...extra) {
    using State = iterator_state<Access, Policy, Iterator, Sentinel, ValueType, Extra...>;

    static_assert(
        !detail::is_base_caster_v<detail::make_caster<ValueType>> ||
        detail::is_copy_constructible_v<ValueType> ||
        (Policy != rv_policy::automatic_reference &&
         Policy != rv_policy::copy),
        "make_iterator_impl(): the generated __next__ would copy elements, so the "
        "element type must be copy-constructible");

    {
#if defined(NB_FREE_THREADED)
        static ft_mutex mu;
        ft_lock_guard lock(mu);
#endif
        if (!type<State>().is_valid()) {
            class_<State>(scope, name)
                .def("__iter__", [](handle h) { return h; })
                .def("__next__",
                    [](State &s) -> iter_result<ValueType> {
                        if (!s.first_or_done)
                            ++s.it;
                        else
                            s.first_or_done = false;

                        if (s.it == s.end) {
                            s.first_or_done = true;
                            return { };
                        }

                        return Access()(s.it);
                    },
                    std::forward<Extra>(extra)...,
                    rv_policy::policy_tag<Policy>());
        }
    }
    return borrow<typed<iterator, ValueType>>(cast(State{
        std::forward<Iterator>(first), std::forward<Sentinel>(last), true }));
}

// Alternative iterator for random access sequences whose element storage may
// move during iteration. The iterator refers to its position by index and
// re-derives the element on every step. The state holds a reference to the
// sequence, which serves as the lock target in free-threaded builds.
template <rv_policy::value Policy, typename Seq> struct index_iterator_state {
    object owner;
    size_t index;
};

// Return type of the generated __next__() methods below
template <typename State> struct next_step { State *state; };

/// Register the Python iterator type ``State`` on first use
template <typename State, rv_policy::value Policy>
void register_step_iterator(handle scope, const char *name) {
#if defined(NB_FREE_THREADED)
    static ft_mutex mu;
    ft_lock_guard lock(mu);
#endif
    if (!type<State>().is_valid()) {
        class_<State>(scope, name)
            .def("__iter__", [](handle h) { return h; })
            .def("__next__",
                 [](State &s) -> next_step<State> { return { &s }; },
                 rv_policy::policy_tag<Policy>());
    }
}

template <rv_policy::value Policy, typename Seq>
struct type_caster<next_step<index_iterator_state<Policy, Seq>>> {
    using State = index_iterator_state<Policy, Seq>;
    using Access = iterator_access<decltype(std::declval<Seq &>().begin())>;
    static constexpr auto Name = make_caster<typename Access::result_type>::Name;

    static handle from_cpp(next_step<State> n, rv_policy policy,
                           cleanup_list *cleanup) noexcept {
        State &s = *n.state;
        ft_object_guard guard(s.owner);
        Seq &seq = *inst_ptr<Seq>(s.owner);

        if (s.index >= seq.size()) {
            // Exhausted iterators stay exhausted even if the sequence grows
            s.index = (size_t) -1;
            PyErr_SetNone(PyExc_StopIteration);
            return { };
        }

        auto it = seq.begin() + (ptrdiff_t) s.index++;
        return make_caster<typename Access::result_type>::from_cpp(
            Access()(it), policy, cleanup);
    }
};

/// Make an index-based Python iterator over the random access sequence ``seq``
template <rv_policy::value Policy = rv_policy::automatic_reference_v, typename Seq>
auto make_index_iterator(handle scope, const char *name, handle seq) {
    using State = index_iterator_state<Policy, Seq>;
    using ValueType = typename iterator_access<
        decltype(std::declval<Seq &>().begin())>::result_type;

    register_step_iterator<State, Policy>(scope, name);
    return borrow<typed<iterator, ValueType>>(cast(State{ borrow(seq), 0 }));
}

/// The analogous iterator for maps refers to its position by key and looks the
/// key up again on every step. A step that detects a modification raises
/// ``RuntimeError`` instead of continuing. The ``Access`` parameter selects what
/// the iterator yields (keys, values, or items).
template <typename Access, rv_policy::value Policy, typename Map>
struct cursor_iterator_state {
    object owner;
    std::optional<typename Map::key_type> cursor;
    size_t size;
    bool done;
};

/// The iteration step again runs during return value conversion, see ``next_step``
template <typename Access, rv_policy::value Policy, typename Map>
struct type_caster<next_step<cursor_iterator_state<Access, Policy, Map>>> {
    using State = cursor_iterator_state<Access, Policy, Map>;
    static constexpr auto Name = make_caster<typename Access::result_type>::Name;

    static handle from_cpp(next_step<State> n, rv_policy policy,
                           cleanup_list *cleanup) noexcept {
        State &s = *n.state;
        ft_object_guard guard(s.owner);
        Map &m = *inst_ptr<Map>(s.owner);

        if (s.done) {
            PyErr_SetNone(PyExc_StopIteration);
            return { };
        }

        if (m.size() != s.size) {
            // Stay failed like a dict iterator (no map ever has this size)
            s.size = (size_t) -1;
            PyErr_SetString(PyExc_RuntimeError,
                            "map changed size during iteration");
            return { };
        }

        // The key copy and the user's comparator or hash function may throw,
        // which must not escape this noexcept function
        try {
            typename Map::iterator it;
            if (s.cursor.has_value()) {
                it = m.find(*s.cursor);
                if (it == m.end()) {
                    s.size = (size_t) -1;
                    PyErr_SetString(PyExc_RuntimeError,
                                    "map keys changed during iteration");
                    return { };
                }
                ++it;
            } else {
                it = m.begin();
            }

            if (it == m.end()) {
                // Exhausted iterators stay exhausted, like their dict counterparts
                s.done = true;
                PyErr_SetNone(PyExc_StopIteration);
                return { };
            }

            s.cursor.emplace((*it).first);
            return make_caster<typename Access::result_type>::from_cpp(
                Access()(it), policy, cleanup);
        } catch (python_error &e) {
            e.restore();
        } catch (const std::bad_alloc &) {
            PyErr_NoMemory();
        } catch (...) {
            PyErr_SetString(PyExc_RuntimeError,
                            "an exception was raised while advancing the map "
                            "iterator");
        }

        // The interrupted step may have lost the cursor position, stay failed
        s.size = (size_t) -1;
        return { };
    }
};

/// Make a key-based Python iterator over the map ``map`` that yields the
/// elements selected by ``Access``
template <typename Access, rv_policy::value Policy, typename Map>
auto make_cursor_iterator(handle scope, const char *name, handle map) {
    using State = cursor_iterator_state<Access, Policy, Map>;
    using ValueType = typename Access::result_type;

    register_step_iterator<State, Policy>(scope, name);

    size_t size;
    {
        ft_object_guard guard(map);
        size = inst_ptr<Map>(map)->size();
    }

    return borrow<typed<iterator, ValueType>>(
        cast(State{ borrow(map), std::nullopt, size, false }));
}

NAMESPACE_END(detail)

/// Makes a python iterator from a first and past-the-end C++ InputIterator.
template <rv_policy::value Policy = rv_policy::automatic_reference_v,
          typename Iterator,
          typename Sentinel,
          typename ValueType = typename detail::iterator_access<Iterator>::result_type,
          typename... Extra,
          typename = decltype(std::declval<Iterator>() == std::declval<Sentinel>())>
auto make_iterator(handle scope, const char *name, Iterator first, Sentinel last, Extra &&...extra) {
    return detail::make_iterator_impl<detail::iterator_access<Iterator>, Policy,
                                      Iterator, Sentinel, ValueType, Extra...>(
        scope, name, std::forward<Iterator>(first),
        std::forward<Sentinel>(last), std::forward<Extra>(extra)...);
}

/// Makes an iterator over the keys (`.first`) of a iterator over pairs from a
/// first and past-the-end InputIterator.
template <rv_policy::value Policy = rv_policy::automatic_reference_v, typename Iterator,
          typename Sentinel,
          typename KeyType =
              typename detail::iterator_key_access<Iterator>::result_type,
          typename... Extra>
auto make_key_iterator(handle scope, const char *name, Iterator first,
                       Sentinel last, Extra &&...extra) {
    return detail::make_iterator_impl<detail::iterator_key_access<Iterator>,
                                      Policy, Iterator, Sentinel, KeyType,
                                      Extra...>(
        scope, name, std::forward<Iterator>(first),
        std::forward<Sentinel>(last), std::forward<Extra>(extra)...);
}

/// Makes an iterator over the values (`.second`) of a iterator over pairs from a
/// first and past-the-end InputIterator.
template <rv_policy::value Policy = rv_policy::automatic_reference_v,
          typename Iterator,
          typename Sentinel,
          typename ValueType = typename detail::iterator_value_access<Iterator>::result_type,
          typename... Extra>
auto make_value_iterator(handle scope, const char *name, Iterator first, Sentinel last, Extra &&...extra) {
    return detail::make_iterator_impl<detail::iterator_value_access<Iterator>,
                                      Policy, Iterator, Sentinel, ValueType,
                                      Extra...>(
        scope, name, std::forward<Iterator>(first),
        std::forward<Sentinel>(last), std::forward<Extra>(extra)...);
}

/// Makes an iterator over values of a container supporting `std::begin()`/`std::end()`
template <rv_policy::value Policy = rv_policy::automatic_reference_v,
          typename Type,
          typename... Extra,
          typename = decltype(std::begin(std::declval<Type&>()))>
auto make_iterator(handle scope, const char *name, Type &value, Extra &&...extra) {
    return make_iterator<Policy>(scope, name, std::begin(value),
                                 std::end(value),
                                 std::forward<Extra>(extra)...);
}

NAMESPACE_END(NB_NAMESPACE)
