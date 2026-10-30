/*
    nanobind/trampoline.h: functionality for overriding C++ virtual functions
    from within Python. See src/trampoline.cpp for more details.

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/nanobind.h>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

// ``trampoline`` and ``ticket`` below are in the frozen backend ABI.

/// Member of every trampoline class (see NB_TRAMPOLINE). The constructor
/// asks the backend for the Python object that owns the C++ instance 'ptr'.
struct trampoline {
    PyObject *self;

#if defined(_MSC_VER) && !defined(__clang__)
    // MSVC requires this when the base class has a constexpr constructor. The
    // unused template parameter postpones the check that would otherwise
    // reject the constexpr specifier because of the backend call below.
    template <typename = void>
    NB_INLINE constexpr trampoline(void *ptr) : self(nullptr) {
#else
    NB_INLINE trampoline(void *ptr) : self(nullptr) {
#endif
        self = NB_CALL(trampoline_new)(NB_CTX, ptr);
    }
    NB_INLINE handle base() const { return self; }
};

/// Asks the backend whether the method 'name' is overridden in Python. If
/// so, 'key' holds the method name for the call, and the destructor notifies
/// the backend when the call is over. The other fields are backend-private.
struct ticket {
    handle self;
    handle key;
    ticket *prev{};
    void *state{};

    NB_INLINE ticket(const trampoline &t, const char *name, uint64_t hash, bool pure) {
        NB_CALL(trampoline_enter)(t.self, name, hash, pure, this);
    }

    NB_INLINE ~ticket() noexcept {
        if (key.is_valid())
            NB_CALL(trampoline_leave)(this);
    }
};

// Extracts the base type of NB_TRAMPOLINE. The specialization with a size
// argument is deprecated.
template <typename Base, int... Size> struct trampoline_base;
template <typename Base> struct trampoline_base<Base> { using type = Base; };
template <typename Base, int Size>
struct [[deprecated("the NB_TRAMPOLINE size argument is obsolete and can be removed")]]
trampoline_base<Base, Size> { using type = Base; };

#define NB_TRAMPOLINE(...)                                                     \
    using NBBase =                                                             \
        typename nanobind::detail::trampoline_base<__VA_ARGS__>::type;         \
    using NBBase::NBBase;                                                      \
    nanobind::detail::trampoline nb_trampoline{ this }

#define NB_OVERRIDE_NAME(name, func, ...)                                      \
    using nb_ret_type = decltype(NBBase::func(__VA_ARGS__));                   \
    constexpr uint64_t nb_hash = nanobind::detail::str_hash(name);             \
    nanobind::detail::ticket nb_ticket(nb_trampoline, name, nb_hash, false);   \
    if (nb_ticket.key.is_valid()) {                                            \
        return nanobind::cast<nb_ret_type>(                                    \
            nb_trampoline.base().attr(nb_ticket.key)(__VA_ARGS__));            \
    } else                                                                     \
        return NBBase::func(__VA_ARGS__)

#define NB_OVERRIDE_PURE_NAME(name, func, ...)                                 \
    using nb_ret_type = decltype(NBBase::func(__VA_ARGS__));                   \
    constexpr uint64_t nb_hash = nanobind::detail::str_hash(name);             \
    nanobind::detail::ticket nb_ticket(nb_trampoline, name, nb_hash, true);    \
    return nanobind::cast<nb_ret_type>(                                        \
        nb_trampoline.base().attr(nb_ticket.key)(__VA_ARGS__))

#define NB_OVERRIDE(func, ...)                                                 \
    NB_OVERRIDE_NAME(#func, func, __VA_ARGS__)

#define NB_OVERRIDE_PURE(func, ...)                                            \
    NB_OVERRIDE_PURE_NAME(#func, func, __VA_ARGS__)

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
