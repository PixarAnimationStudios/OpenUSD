/*
    nanobind/stl/unique_ptr.h: Type caster for std::unique_ptr<T>

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/nanobind.h>
#include <memory>

NAMESPACE_BEGIN(NB_NAMESPACE)

// Deleter for std::unique_ptr<T> (handles ownership by both C++ and Python)
template <typename T> struct deleter {
    /// Instance should be cleared using a delete expression
    deleter()  = default;

    /// Instance owned by Python, reduce reference count upon deletion
    deleter(handle h) : o(h.ptr()) { }

    /// Does Python own storage of the underlying object
    bool owned_by_python() const { return o != nullptr; }

    /// Does C++ own storage of the underlying object
    bool owned_by_cpp() const { return o == nullptr; }

    /// Perform the requested deletion operation
    void operator()(void *p) noexcept {
        if (o) {
            // Don't run the deleter if the interpreter has been shut down
            if (detail::cleanup_guard guard{})
                Py_DECREF(o);
        } else {
            delete (T *) p;
        }
    }

    PyObject *o{nullptr};
};

NAMESPACE_BEGIN(detail)

template <typename T, typename Deleter>
struct type_caster<std::unique_ptr<T, Deleter>> {
    static constexpr bool IsClass = true;
    using Value = std::unique_ptr<T, Deleter>;
    using Caster = make_caster<T>;
    using Td = std::decay_t<T>;

    static constexpr bool IsDefaultDeleter =
        std::is_same_v<Deleter, std::default_delete<T>>;
    static constexpr bool IsNanobindDeleter =
        std::is_same_v<Deleter, deleter<T>>;

    static_assert(is_base_caster_v<Caster>,
                  "Conversion of ``unique_ptr<T>`` requires that ``T`` is "
                  "handled by nanobind's regular class binding mechanism. "
                  "However, a type caster was registered to intercept this "
                  "particular type, which is not allowed.");

    static_assert(IsDefaultDeleter || IsNanobindDeleter,
                  "Binding std::unique_ptr<T, Deleter> requires that "
                  "'Deleter' is either 'std::default_delete<T>' or "
                  "'nanobind::deleter<T>'");

    static constexpr auto Name = Caster::Name;
    template <typename T_> using Cast = Value;

    Caster caster;
    handle src;

    // If true, the Python object has relinquished ownership but we have
    // not yet yielded a unique_ptr that holds ownership on the C++ side.
    //
    // `nb_type_relinquish_ownership()` can fail, so we must check it in
    // `can_cast()`. If we do so, but then wind up not executing the cast
    // operator, we must remember to undo our relinquishment and push the
    // ownership back onto the Python side. For example, this might be
    // necessary if the Python object `[(foo, foo)]` is converted to
    // `std::vector<std::pair<std::unique_ptr<T>, std::unique_ptr<T>>>`;
    // the pair caster won't know that it can't cast the second element
    // until after it's verified that it can cast the first one.
    mutable bool inflight = false;

    ~type_caster() {
        if (inflight)
            NB_CALL(nb_type_restore_ownership)(src.ptr(), IsDefaultDeleter);
    }

    bool from_python(handle src_, uint32_t, cleanup_list *) noexcept {
        // Stash source python object
        src = src_;

        // Try casting to a pointer of the underlying type. We pass flags=0 and
        // cleanup=nullptr to prevent implicit type conversions (they are
        // problematic since the instance then wouldn't be owned by 'src')
        return caster.from_python(src_, 0, nullptr);
    }

    template <typename T2>
    static handle from_cpp(T2 *value, rv_policy policy,
                           cleanup_list *cleanup) noexcept {
        if (!value)
            return handle();

        return from_cpp(*value, policy, cleanup);
    }

    template <typename T2>
    static handle from_cpp(T2 &&value,
                           rv_policy, cleanup_list *cleanup) noexcept {

        bool cpp_delete = true;
        if constexpr (IsNanobindDeleter)
            cpp_delete = value.get_deleter().owned_by_cpp();

        Td *ptr = (Td *) value.get();
        const std::type_info *type = &typeid(Td);
        if (!ptr)
            return none().release();

        constexpr bool has_type_hook =
            !std::is_base_of_v<std::false_type, type_hook<Td>>;
        if constexpr (has_type_hook)
            type = type_hook<Td>::get(ptr);

        const std::type_info *type_p = nullptr;
        if constexpr (std::is_polymorphic_v<Td>)
            type_p = (!has_type_hook && ptr) ? &typeid(*ptr) : nullptr;

        handle result = NB_CALL(nb_type_put_unique)(NB_CTX_C(cleanup), type,
                                                    type_p, ptr, cleanup,
                                                    cpp_delete);

        if (result.is_valid()) {
            if (cpp_delete)
                value.release();
            else
                value.reset();
        }

        return result;
    }

    template <typename T_>
    bool can_cast() const noexcept {
        if (src.is_none() || inflight)
            return true;
        else if (!NB_CALL(nb_type_relinquish_ownership)(src.ptr(), IsDefaultDeleter))
            return false;
        inflight = true;
        return true;
    }

    explicit operator Value() {
        if (!inflight && !src.is_none() &&
            !NB_CALL(nb_type_relinquish_ownership)(src.ptr(), IsDefaultDeleter))
            throw next_overload();

        Td *p = caster.operator Td *();

        Value value;
        if constexpr (IsNanobindDeleter)
            value = Value(p, deleter<T>(src.inc_ref()));
        else
            value = Value(p);
        inflight = false;
        return value;
    }
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
