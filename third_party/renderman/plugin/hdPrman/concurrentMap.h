//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CONCURRENT_MAP_H
#define EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CONCURRENT_MAP_H

#include "pxr/pxr.h"

#include <tbb/concurrent_unordered_map.h>
#include <tbb/spin_rw_mutex.h>

#include <atomic>
#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

// A simple concurrent hashmap built from tbb::concurrent_unordered_map but
// with a simpler interface. Thread-safe operations (insertion, retrieval,
// const iteration) happen under a shared_lock, while unsafe operations
// (erase, clear, non-const iteration) use an exclusive lock. This way, the
// thread-safe operations can all run concurrently with one another, relying
// on tbb::concurrent_unordered_map's thread safety, but will never run
// while an unsafe operation is in progress, nor will the unsafe operations
// start while a safe one is running.
//
// Currently supports two value types (T):
//  - Any T (other than std::atomic<U>) that is move- or default-
//        constructible: provides get() and set().
//  - std::atomic<U> were U is default-constructible: provides get() & load().
//        When U is an integer type, additionally provides inc() & dec().
//        Neither movable nor copyable.
template<
    typename Key,
    typename T,
    typename Hash = std::hash<Key>,
    typename KeyEqual = std::equal_to<Key>>
class HdPrmanConcurrentMap
{
private:
    template<typename U>
    struct _is_atomic : std::false_type { };

    template<typename U>
    struct _is_atomic<std::atomic<U>> : std::true_type { };

    // XXX: std::atomic<U>::value_type is not available until C++20; these
    // fill the gap until then.
    template<typename U>
    struct _atomic_value_type; // undefined for non-atomic U

    template<typename U>
    struct _atomic_value_type<std::atomic<U>> { using type = U; };

    template<typename U>
    using _atomic_value_t = typename _atomic_value_type<U>::type;

    static_assert(
        _is_atomic<T>::value ||
        std::is_move_constructible_v<T> ||
        std::is_default_constructible_v<T>,
        "HdPrmanConcurrentMap<Key, T>: T must be move- or default- "
        "constructible or std::atomic<U> for integral U.");

public:
    T& get(const Key& key)
    {
        if constexpr (_is_atomic<T>::value) {
            tbb::spin_rw_mutex::scoped_lock lock(_mutex, false);
            return _get_atomic(key, lock);
        }
        if constexpr (std::is_move_constructible_v<T>) {
            tbb::spin_rw_mutex::scoped_lock lock(_mutex, false);
            return _map[key];
        }
        tbb::spin_rw_mutex::scoped_lock lock(_mutex, false);
        auto it = _map.find(key);
        if (it == _map.end()) {
            if (!lock.upgrade_to_writer()) {
                it = _map.find(key);
            }
            if (it == _map.end()) {
                it = _map.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(key),
                    std::tuple<>{}).first;
            }
        }
        return it->second;

    }

    // Executes the given function under shared lock if the map has a
    // value for key. WARNING: The function must not call any methods on the
    // map itself; doing so will cause a deadlock!
    bool withIfPresent(const Key& key, std::function<void(T&)> fn)
    {
        tbb::spin_rw_mutex::scoped_lock lock(_mutex, false);
        auto it = _map.find(key);
        if (it == _map.end()) { return false; }
        fn(it->second);
        return true;
    }

    // Iterate the map with a non-const value reference under exclusive lock.
    // WARNING: The function must not call any methods on the map itself;
    // doing so will cause a deadlock!
    void iterate(std::function<void(const Key&, T&)> fn)
    {
        // exclusive lock
        tbb::spin_rw_mutex::scoped_lock lock(_mutex, true);
        for (auto& p : _map) {
            fn(p.first, p.second);
        }
    }

    // Iterate the map with a const value reference under shared lock.
    // WARNING: The function must not call any methods on the map itself;
    // doing so will cause a deadlock!
    void citerate(std::function<void(const Key&, const T&)> fn) const
    {
        tbb::spin_rw_mutex::scoped_lock lock(_mutex, false);
        for (const auto& p : _map) {
            fn(p.first, p.second);
        }
    }

    // Gives the count of keys currently in the map
    size_t size() const
    {
        tbb::spin_rw_mutex::scoped_lock lock(_mutex, false);
        return _map.size();
    }

    // Erase the given key from the map under exclusive lock
    void erase(const Key& key)
    {
        // tbb::concurrent_hash_map::erase() is not thread-safe
        tbb::spin_rw_mutex::scoped_lock lock(_mutex, true);
        _map.unsafe_erase(key);
    }

    // Clear all map entries under exclusive lock
    void clear()
    {
        // tbb::concurrent_hash_map::clear() is not thread-safe
        tbb::spin_rw_mutex::scoped_lock lock(_mutex, true);
        _map.clear();
    }

    // Set value for key. Available only for copy-assignable non-atomic T
    template<
        typename U = T,
        typename = std::enable_if_t<
            !_is_atomic<U>::value &&
            std::is_copy_assignable_v<U>>>
    void set(const Key& key, const T& val)
    {
        tbb::spin_rw_mutex::scoped_lock lock(_mutex, false);
        if constexpr (std::is_move_constructible_v<T>) {
            _map[key] = val;
            return;
        }
        auto it = _map.find(key);
        if (it == _map.end()) {
            if (!lock.upgrade_to_writer()) {
                it = _map.find(key);
            }
            if (it == _map.end()) {
                if constexpr (std::is_copy_constructible_v<T>) {
                    _map.insert({ key, val });
                    return;
                }
                it = _map.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(key),
                    std::tuple<>{}).first;
            }
        }
        it->second = val;
    }

    // Load the atomic value for key. Only available for atomic T.
    template<
        typename U = T,
        typename = std::enable_if_t<_is_atomic<U>::value>>
    _atomic_value_t<U> load(
        const Key& key,
        std::memory_order order = std::memory_order_seq_cst)
    {
        tbb::spin_rw_mutex::scoped_lock lock(_mutex, false);
        return _get_atomic(key, lock).load(order);
    }

    // Increment the atomic value for key. Returns value after increment.
    // Only available for atomic integral T.
    template<
        typename U = T,
        typename = std::enable_if_t<
            _is_atomic<U>::value &&
            std::is_integral_v<_atomic_value_t<U>>>>
    _atomic_value_t<U> inc(const Key& key)
    {
        tbb::spin_rw_mutex::scoped_lock lock(_mutex, false);
        return ++_get_atomic(key, lock);
    }

    // Decrement the atomic value for key. Returns value after decrement.
    // Only available for atomic integral T.
    template<
        typename U = T,
        typename = std::enable_if_t<
            _is_atomic<U>::value &&
            std::is_integral_v<_atomic_value_t<U>>>>
    _atomic_value_t<U> dec(const Key& key)
    {
        tbb::spin_rw_mutex::scoped_lock lock(_mutex, false);
        return --_get_atomic(key, lock);
    }

private:
    template<typename U = T, typename = std::enable_if_t<_is_atomic<U>::value>>
    T& _get_atomic(const Key& key, tbb::spin_rw_mutex::scoped_lock& lock)
    {
        auto it = _map.find(key);
        if (it == _map.end()) {
            if (!lock.upgrade_to_writer()) {
                it = _map.find(key);
            }
            if (it == _map.end()) {
                // Prior to C++20, std::atomic<U>'s default constructor does not
                // initialize the value held by the atomic. We must default
                // construct the value first and pass it as an argument to
                // emplace(), which will then invoke std::atomic<U>(U desired)
                // instead, which does initialize the value. Under C++20, the
                // third argument to emplace() may be simplified to
                // std::tuple<>{}.
                it = _map.emplace(
                    std::piecewise_construct,
                    std::forward_as_tuple(key),
                    std::forward_as_tuple(_atomic_value_t<T>{})).first;
            }
        }
        return it->second;
    }

    using _MapType = tbb::concurrent_unordered_map<Key, T, Hash, KeyEqual>;
    _MapType _map;

    mutable tbb::spin_rw_mutex _mutex;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // EXT_RMANPKG_PLUGIN_RENDERMAN_PLUGIN_HD_PRMAN_CONCURRENT_MAP_H
