/*
    nanobind/intrusive/ref.h: This file defines the ``ref<T>`` RAII scoped
    reference counting helper class.

    When included following ``nanobind/nanobind.h``, the code below also
    exposes a custom type caster to bind functions taking or returning
    ``ref<T>``-typed values.

    Copyright (c) 2023 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include "counter.h"

NAMESPACE_BEGIN(nanobind)

/**
 * \brief RAII scoped reference counting helper class
 *
 * ``ref`` is a simple RAII wrapper class that encapsulates a pointer to an
 * instance with intrusive reference counting.
 *
 * It takes care of increasing and decreasing the reference count as needed and
 * deleting the instance when the count reaches zero.
 *
 * For this to work, compatible functions ``inc_ref()`` and ``dec_ref()`` must
 * be defined before including this file. Default implementations for
 * subclasses of the type ``intrusive_base`` are already provided as part of the
 * file ``counter.h``.
 */
template <typename T> class ref {
public:
    /// Create a null reference
    ref() = default;

    /// Construct a reference from a pointer
    ref(T *ptr) : m_ptr(ptr) { inc_ref((intrusive_base *) m_ptr); }

    /// Copy a reference, increases the reference count
    ref(const ref &r) : m_ptr(r.m_ptr) { inc_ref((intrusive_base *) m_ptr); }

    /// Move a reference without changing the reference count
    ref(ref &&r) noexcept : m_ptr(r.m_ptr) { r.m_ptr = nullptr; }

    /// Destroy this reference
    ~ref() { dec_ref((intrusive_base *) m_ptr); }

    /// Move-assign another reference into this one
    ref &operator=(ref &&r) noexcept {
        dec_ref((intrusive_base *) m_ptr);
        m_ptr = r.m_ptr;
        r.m_ptr = nullptr;
        return *this;
    }

    /// Copy-assign another reference into this one
    ref &operator=(const ref &r) {
        inc_ref((intrusive_base *) r.m_ptr);
        dec_ref((intrusive_base *) m_ptr);
        m_ptr = r.m_ptr;
        return *this;
    }

    /// Overwrite this reference with a pointer to another object
    ref &operator=(T *ptr) {
        inc_ref((intrusive_base *) ptr);
        dec_ref((intrusive_base *) m_ptr);
        m_ptr = ptr;
        return *this;
    }

    /// Clear the currently stored reference
    void reset() {
        dec_ref((intrusive_base *) m_ptr);
        m_ptr = nullptr;
    }

    /// Compare this reference with another reference
    bool operator==(const ref &r) const { return m_ptr == r.m_ptr; }

    /// Compare this reference with another reference
    bool operator!=(const ref &r) const { return m_ptr != r.m_ptr; }

    /// Compare this reference with a pointer
    bool operator==(const T *ptr) const { return m_ptr == ptr; }

    /// Compare this reference with a pointer
    bool operator!=(const T *ptr) const { return m_ptr != ptr; }

    /// Access the object referenced by this reference
    T *operator->() { return m_ptr; }

    /// Access the object referenced by this reference
    const T *operator->() const { return m_ptr; }

    /// Return a C++ reference to the referenced object
    T &operator*() { return *m_ptr; }

    /// Return a const C++ reference to the referenced object
    const T &operator*() const { return *m_ptr; }

    /// Return a pointer to the referenced object
    operator T *() { return m_ptr; }

    /// Return a const pointer to the referenced object
    operator const T *() const { return m_ptr; }

    /// Return a pointer to the referenced object
    T *get() { return m_ptr; }

    /// Return a const pointer to the referenced object
    const T *get() const { return m_ptr; }

private:
    T *m_ptr = nullptr;
};

// Register a type caster for ``ref<T>`` if nanobind was previously #included
#if defined(NB_VERSION_MAJOR)
NAMESPACE_BEGIN(detail)
template <typename T> struct type_caster<nanobind::ref<T>> {
    using Caster = make_caster<T>;
    static constexpr bool IsClass = true;
    NB_TYPE_CASTER(ref<T>, Caster::Name)

    bool from_python(handle src, uint32_t flags,
                     cleanup_list *cleanup) noexcept {
        Caster caster;
        if (!caster.from_python(src, flags, cleanup))
            return false;

        value = Value(caster.operator T *());
        return true;
    }

    static handle from_cpp(const ref<T> &value, rv_policy policy,
                           cleanup_list *cleanup) noexcept {
        if constexpr (std::is_base_of_v<intrusive_base, T>)
            if (policy != rv_policy::copy && policy != rv_policy::move && value.get())
                if (PyObject* obj = value->self_py())
                    return handle(obj).inc_ref();

        return Caster::from_cpp(value.get(), policy, cleanup);
    }
};
NAMESPACE_END(detail)
#endif

NAMESPACE_END(nanobind)
