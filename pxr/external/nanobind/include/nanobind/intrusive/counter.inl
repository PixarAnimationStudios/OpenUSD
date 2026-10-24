/*
    nanobind/intrusive/counter.inl: Intrusive reference counting sample
    implementation; see 'counter.h' for an explanation of the interface.

    Copyright (c) 2023 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#include "counter.h"
#include <cstdio>
#include <cstdlib>

NAMESPACE_BEGIN(nanobind)

// The code below uses intrinsics for atomic operations. This is not as nice
// and portable as ``std::atomic<T>`` but avoids pulling in large amounts of
// STL header code

#if !defined(_MSC_VER)
#define NB_ATOMIC_LOAD(ptr) __atomic_load_n(ptr, 0)
#define NB_ATOMIC_STORE(ptr, v) __atomic_store_n(ptr, v, 0)
#define NB_ATOMIC_CMPXCHG(ptr, cmp, xchg)                                      \
    __atomic_compare_exchange_n(ptr, cmp, xchg, true, 0, 0)
#else
extern "C" void *_InterlockedCompareExchangePointer(
    void *volatile *Destination,
    void *Exchange, void *Comparand);
#pragma intrinsic(_InterlockedCompareExchangePointer)

#define NB_ATOMIC_LOAD(ptr) *((volatile const uintptr_t *) ptr)
#define NB_ATOMIC_STORE(ptr, v) *((volatile uintptr_t *) ptr) = v;
#define NB_ATOMIC_CMPXCHG(ptr, cmp, xchg) nb_cmpxchg(ptr, cmp, xchg)

static bool nb_cmpxchg(uintptr_t *ptr, uintptr_t *cmp, uintptr_t xchg) {
    uintptr_t cmpv = *cmp;
    uintptr_t prev = (uintptr_t) _InterlockedCompareExchangePointer(
        (void * volatile *) ptr, (void *) xchg, (void *) cmpv);
    if (prev == cmpv) {
        return true;
    } else {
        *cmp = prev;
        return false;
    }
}
#endif

static void (*intrusive_inc_ref_py)(PyObject *) noexcept = nullptr,
            (*intrusive_dec_ref_py)(PyObject *) noexcept = nullptr;

void intrusive_init(void (*intrusive_inc_ref_py_)(PyObject *) noexcept,
                    void (*intrusive_dec_ref_py_)(PyObject *) noexcept) {
    intrusive_inc_ref_py = intrusive_inc_ref_py_;
    intrusive_dec_ref_py = intrusive_dec_ref_py_;
}

/** A few implementation details:
 *
 * The ``intrusive_counter`` constructor sets the ``m_state`` field to ``1``,
 * which indicates that the instance is owned by C++. Bits 2..63 of this
 * field are used to store the actual reference count value. The
 * ``inc_ref()`` and ``dec_ref()`` functions increment or decrement this
 * number. When ``dec_ref()`` removes the last reference, the instance
 * returns ``true`` to indicate that it should be deallocated using a
 * *delete expression* that would typically be handled using a polymorphic
 * destructor.
 *
 * When an class with intrusive reference counting is returned from C++ to
 * Python, nanobind will invoke ``set_self_py()``, which hands ownership
 * over to Python/nanobind. Any remaining references will be moved from the
 * ``m_state`` field to the Python reference count. In this mode,
 * ``inc_ref()`` and ``dec_ref()`` wrap Python reference counting
 * primitives (``Py_INCREF()`` / ``Py_DECREF()``) which must be made
 * available by calling the function ``intrusive_init`` once during module
 * initialization. Note that the `m_state` field is also used to store a
 * pointer to the `PyObject *`. Python instance pointers are always aligned
 * (i.e. bit 1 is zero), which disambiguates between the two possible
 * configurations.
 */

void intrusive_counter::inc_ref() const noexcept {
    uintptr_t v = NB_ATOMIC_LOAD(&m_state);

    while (true) {
        if (v & 1) {
            if (!NB_ATOMIC_CMPXCHG(&m_state, &v, v + 2))
                continue;
        } else {
            intrusive_inc_ref_py((PyObject *) v);
        }

        break;
    }
}

bool intrusive_counter::dec_ref() const noexcept {
    uintptr_t v = NB_ATOMIC_LOAD(&m_state);

    while (true) {
        if (v & 1) {
            if (v == 1) {
                fprintf(stderr,
                        "intrusive_counter::dec_ref(%p): reference count "
                        "underflow!", (void *) this);
                abort();
            }

            if (!NB_ATOMIC_CMPXCHG(&m_state, &v, v - 2))
                continue;

            if (v == 3)
                return true;
        } else {
            intrusive_dec_ref_py((PyObject *) v);
        }

        return false;
    }
}

size_t intrusive_counter::ref_count() const noexcept {
    uintptr_t v = NB_ATOMIC_LOAD(&m_state);
    return (v & 1) ? (size_t) (v >> 1) : 0;
}

void intrusive_counter::set_self_py(PyObject *o) noexcept {
    uintptr_t v = NB_ATOMIC_LOAD(&m_state);

    if (v & 1) {
        // Transfer the C++ reference count to the Python object. The CAS loop
        // accounts for concurrent inc_ref()/dec_ref() calls modifying the count.
        uintptr_t transferred = 0;
        do {
            for (uintptr_t count = v >> 1; transferred < count; ++transferred)
                intrusive_inc_ref_py(o);
        } while (!NB_ATOMIC_CMPXCHG(&m_state, &v, (uintptr_t) o));

        for (uintptr_t count = v >> 1; transferred > count; --transferred)
            intrusive_dec_ref_py(o);
    } else {
        fprintf(stderr,
                "intrusive_counter::set_self_py(%p): a Python object was "
                "already present!", (void *) this);
        abort();
    }
}

PyObject *intrusive_counter::self_py() const noexcept {
    uintptr_t v = NB_ATOMIC_LOAD(&m_state);

    if (v & 1)
        return nullptr;
    else
        return (PyObject *) v;
}

NAMESPACE_END(nanobind)
