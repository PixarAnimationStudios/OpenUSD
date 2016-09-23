//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
//
#ifndef USD_SHARED_H
#define USD_SHARED_H

#include <boost/functional/hash.hpp>
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <atomic>

// Implementation storage + refcount for Usd_Shared.
template <class T>
struct Usd_Counted {
    constexpr Usd_Counted() : count(0) {}
    explicit Usd_Counted(T const &data) : data(data), count(0) {}
    explicit Usd_Counted(T &&data) : data(std::move(data)), count(0) {}
    
    friend inline void
    intrusive_ptr_add_ref(Usd_Counted const *c) { ++c->count; }
    friend inline void
    intrusive_ptr_release(Usd_Counted const *c) {
        if (--c->count == 0)
            delete c;
    }

    T data;
    mutable std::atomic_int count;
};

struct Usd_EmptySharedTagType {};
constexpr Usd_EmptySharedTagType Usd_EmptySharedTag{};

// This class provides a simple way to share a data object between clients.  It
// can be used to do simple copy-on-write, etc.
template <class T>
struct Usd_Shared
{
    // Construct a Usd_Shared with a value-initialized T instance.
    Usd_Shared() : _held(new Usd_Counted<T>()) {}
    // Create a copy of \p obj.
    explicit Usd_Shared(T const &obj) : _held(new Usd_Counted<T>(obj)) {}
    // Move from \p obj.
    explicit Usd_Shared(T &&obj) : _held(new Usd_Counted<T>(std::move(obj))) {}

    // Create an empty shared, which may not be accessed via Get(),
    // GetMutable(), IsUnique(), Clone(), or MakeUnique().  This is useful when
    // using the insert() or emplace() methods on associative containers, to
    // avoid allocating a temporary in case the object is already present in the
    // container.
    Usd_Shared(Usd_EmptySharedTagType) {}

    // Return a const reference to the shared data.
    T const &Get() const { return _held->data; }
    // Return a mutable reference to the shared data.
    T &GetMutable() const { return _held->data; }
    // Return true if no other Usd_Shared instance shares this instance's data.
    bool IsUnique() const { return _held->count == 1; }
    // Make a new copy of the held data and refer to it.
    void Clone() { _held.reset(new Usd_Counted<T>(Get())); }
    // Ensure this Usd_Shared instance has unique data.  Equivalent to:
    // \code
    // if (not shared.IsUnique()) { shared.Clone(); }
    // \endcode
    void MakeUnique() { if (not IsUnique()) Clone(); }

    // Equality and inequality.
    bool operator==(Usd_Shared const &other) const {
        return _held == other._held or _held->data == other._held->data;
    }
    bool operator!=(Usd_Shared const &other) const { return *this != other; }

    // Swap.
    void swap(Usd_Shared &other) { _held.swap(other._held); }
    friend inline void swap(Usd_Shared &l, Usd_Shared &r) { l.swap(r); }

    // hash_value.
    friend inline size_t hash_value(Usd_Shared const &sh) {
        using boost::hash_value;
        return hash_value(sh._held->data);
    }
private:
    boost::intrusive_ptr<Usd_Counted<T>> _held;
};

#endif // USD_SHARED_H
