/*
    nanobind/nb_attr.h: Annotations for function and class declarations

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

/// Per-argument cast flags of an argument without annotations
constexpr uint32_t arg_flags_default = cast_flags::convert;

NAMESPACE_END(detail)

struct scope {
    PyObject *value;
    NB_INLINE scope(handle value) : value(value.ptr()) {}
};

struct name {
    const char *value;
    NB_INLINE name(const char *value) : value(value) {}
};

template <uint32_t Flags, bool Locked> struct arg_v_t;

template <uint32_t Flags = detail::arg_flags_default, bool Locked = false>
struct arg_t {
    NB_INLINE constexpr explicit arg_t(const char *name = nullptr)
        : name_(name), signature_(nullptr) { }

    template <uint32_t F2, bool L2>
    NB_INLINE constexpr explicit arg_t(const arg_t<F2, L2> &base)
        : name_(base.name_), signature_(base.signature_) { }

    // operator= can be used to provide a default value. A 'None' default
    // (spelled nb::none() or nullptr) additionally marks the argument as
    // accepting 'None' (see 'cast_flags::accepts_none').
    template <typename T>
    NB_INLINE arg_v_t<Flags, Locked> operator=(T &&value) const;
    NB_INLINE auto operator=(::nanobind::none value) const;
    NB_INLINE auto operator=(std::nullptr_t) const;

    NB_INLINE constexpr auto noconvert() const {
        return arg_t<Flags & ~detail::cast_flags::convert, Locked>(*this);
    }
    NB_INLINE constexpr auto none() const {
        return arg_t<Flags | detail::cast_flags::accepts_none, Locked>(*this);
    }
    NB_INLINE constexpr auto lock() const { return arg_t<Flags, true>(*this); }
    NB_INLINE arg_t &sig(const char *value) {
        signature_ = value;
        return *this;
    }

    const char *name_, *signature_;
};

// Function argument descriptor with a default value
template <uint32_t Flags, bool Locked> struct arg_v_t : arg_t<Flags, Locked> {
    object value;

    template <uint32_t F2, bool L2>
    NB_INLINE arg_v_t(const arg_t<F2, L2> &base, object &&value)
        : arg_t<Flags, Locked>(base), value(std::move(value)) { }

  private:
    // Inherited mutators would slice off the default, and are not generally needed
    using arg_t<Flags, Locked>::noconvert;
    using arg_t<Flags, Locked>::none;
    using arg_t<Flags, Locked>::sig;
    using arg_t<Flags, Locked>::lock;
};

using arg = arg_t<>;
using arg_locked = arg_t<detail::arg_flags_default, true>;
using arg_v = arg_v_t<detail::arg_flags_default, false>;
using arg_locked_v = arg_v_t<detail::arg_flags_default, true>;

template <uint32_t Flags, bool Locked>
NB_INLINE auto arg_t<Flags, Locked>::operator=(::nanobind::none value) const {
    return arg_v_t<Flags | detail::cast_flags::accepts_none, Locked>(
        *this, std::move(value));
}
template <uint32_t Flags, bool Locked>
NB_INLINE auto arg_t<Flags, Locked>::operator=(std::nullptr_t) const {
    return operator=(::nanobind::none());
}

NAMESPACE_BEGIN(detail)

/// Compile-time properties of a function argument annotation
template <typename T> struct arg_traits {
    static constexpr bool is_arg = false, has_default = false, locked = false;
    static constexpr uint32_t flags = 0;
};
template <uint32_t F, bool L> struct arg_traits<arg_t<F, L>> {
    static constexpr bool is_arg = true, has_default = false, locked = L;
    static constexpr uint32_t flags = F;
};
template <uint32_t F, bool L> struct arg_traits<arg_v_t<F, L>> {
    static constexpr bool is_arg = true, has_default = true, locked = L;
    static constexpr uint32_t flags = F;
};

NAMESPACE_END(detail)

template <typename... Ts> struct call_guard {
    using type = detail::tuple<Ts...>;
};

struct dynamic_attr {};
struct is_weak_referenceable {};
struct is_method {};
struct is_implicit {};
struct is_operator {};
struct is_arithmetic {};
struct is_flag {};
struct is_str {};
struct is_final {};
struct is_generic {};
struct kw_only {};
struct lock_self {};
struct never_destruct {};

struct pooled {
    explicit pooled(uint32_t capacity = 128) : capacity(capacity) {}
    uint32_t capacity;
};

template <size_t /* Nurse */, size_t /* Patient */> struct keep_alive {};
template <typename T> struct supplement {};
template <typename T> struct intrusive_ptr {
    intrusive_ptr(void (*set_self_py)(T *, PyObject *) noexcept)
        : set_self_py(set_self_py) { }
    void (*set_self_py)(T *, PyObject *) noexcept;
};

struct type_slots {
    type_slots (const PyType_Slot *value) : value(value) { }
    const PyType_Slot *value;
};

struct type_slots_callback {
    using cb_t = void (*)(const detail::type_data_init *t,
                          PyType_Slot *&slots, size_t max_slots) noexcept;
    type_slots_callback(cb_t callback) : callback(callback) { }
    cb_t callback;
};

struct sig {
    const char *value;
    sig(const char *value) : value(value) { }
};

struct is_getter { };

template <typename Policy> struct call_policy final {};

NAMESPACE_BEGIN(literals)
constexpr arg operator""_a(const char *name, size_t) { return arg(name); }
NAMESPACE_END(literals)

NAMESPACE_BEGIN(detail)

template <typename F>
NB_INLINE void func_extra_apply(F &f, const name &name, size_t &) {
    f.name = name.value;
    f.flags |= (uint32_t) func_flags::has_name;
}

template <typename F>
NB_INLINE void func_extra_apply(F &f, const scope &scope, size_t &) {
    f.scope = scope.value;
    f.flags |= (uint32_t) func_flags::has_scope;
}

template <typename F>
NB_INLINE void func_extra_apply(F &f, const sig &s, size_t &) {
    f.flags |= (uint32_t) func_flags::has_signature;
    f.name = s.value;
}

template <typename F>
NB_INLINE void func_extra_apply(F &f, const char *doc, size_t &) {
    f.doc = doc;
    f.flags |= (uint32_t) func_flags::has_doc;
}

template <typename F>
NB_INLINE void func_extra_apply(F &f, is_method, size_t &) {
    f.flags |= (uint32_t) func_flags::is_method;
}

template <typename F>
NB_INLINE void func_extra_apply(F &, is_getter, size_t &) { }

template <typename F>
NB_INLINE void func_extra_apply(F &f, is_implicit, size_t &) {
    f.flags |= (uint32_t) func_flags::is_implicit;
}

template <typename F>
NB_INLINE void func_extra_apply(F &f, is_operator, size_t &) {
    f.flags |= (uint32_t) func_flags::is_operator;
}

template <typename F, rv_policy::value V>
NB_INLINE void func_extra_apply(F &, rv_policy::policy_tag<V>, size_t &) {
    // Consumed at compile time via func_extra_info<...>::policy
}

template <typename F>
NB_INLINE void func_extra_apply(F &, rv_policy, size_t &) {
    static_assert(false_v<F>,
                  "nb::def() and friends require a compile-time return value "
                  "policy tag such as nb::rv_policy::move; a runtime "
                  "rv_policy value cannot be used as a function annotation.");
}

template <typename F>
NB_INLINE void func_extra_apply(F &, std::nullptr_t, size_t &) { }

// The two overloads below fill the runtime argument record. Cast flags and
// locking are handled statically in nb_func.h, which also initializes the
// record's flag field (see 'arg_flags_static').

template <typename F, uint32_t Flags, bool Locked>
NB_INLINE void func_extra_apply(F &f, const arg_t<Flags, Locked> &a,
                                size_t &index) {
    arg_data_init &ad = f.args[index++];
    ad.name = a.name_;
    ad.signature = a.signature_;
    ad.value = nullptr;
}

template <typename F, uint32_t Flags, bool Locked>
NB_INLINE void func_extra_apply(F &f, const arg_v_t<Flags, Locked> &a,
                                size_t &index) {
    arg_data_init &ad = f.args[index];
    func_extra_apply(f, (const arg_t<Flags, Locked> &) a, index);
    ad.value = a.value.ptr();
}

template <typename F>
NB_INLINE void func_extra_apply(F &, kw_only, size_t &) {}

template <typename F>
NB_INLINE void func_extra_apply(F &, lock_self, size_t &) {}

template <typename F, typename... Ts>
NB_INLINE void func_extra_apply(F &, call_guard<Ts...>, size_t &) {}

template <typename F, size_t Nurse, size_t Patient>
NB_INLINE void func_extra_apply(F &f, nanobind::keep_alive<Nurse, Patient>, size_t &) {
    f.flags |= (uint32_t) func_flags::can_mutate_args;
}

template <typename F, typename Policy>
NB_INLINE void func_extra_apply(F &f, call_policy<Policy>, size_t &) {
    f.flags |= (uint32_t) func_flags::can_mutate_args;
}

template <typename... Ts> struct func_extra_info {
    using call_guard = void;
    static constexpr bool pre_post_hooks = false;
    static constexpr size_t nargs_locked = 0;
    static constexpr bool has_policy = false;
    static constexpr rv_policy::value policy = rv_policy::automatic_v;
};

template <typename T, typename... Ts> struct func_extra_info<T, Ts...>
    : func_extra_info<Ts...> { };

// When several policy annotations are given, the last one takes precedence,
// matching the runtime traversal order in func_extra_apply().
template <rv_policy::value V, typename... Ts>
struct func_extra_info<rv_policy::policy_tag<V>, Ts...> : func_extra_info<Ts...> {
    static constexpr bool has_policy = true;
    static constexpr rv_policy::value policy =
        func_extra_info<Ts...>::has_policy ? func_extra_info<Ts...>::policy : V;
};

template <typename... Cs, typename... Ts>
struct func_extra_info<call_guard<Cs...>, Ts...> : func_extra_info<Ts...> {
    static_assert(std::is_same_v<typename func_extra_info<Ts...>::call_guard, void>,
                  "call_guard<> can only be specified once!");
    using call_guard = nanobind::call_guard<Cs...>;
};

template <size_t Nurse, size_t Patient, typename... Ts>
struct func_extra_info<nanobind::keep_alive<Nurse, Patient>, Ts...> : func_extra_info<Ts...> {
    static constexpr bool pre_post_hooks = true;
};

template <typename Policy, typename... Ts>
struct func_extra_info<call_policy<Policy>, Ts...> : func_extra_info<Ts...> {
    static constexpr bool pre_post_hooks = true;
};

template <uint32_t F, typename... Ts>
struct func_extra_info<arg_t<F, true>, Ts...> : func_extra_info<Ts...> {
    static constexpr size_t nargs_locked = 1 + func_extra_info<Ts...>::nargs_locked;
};

template <uint32_t F, typename... Ts>
struct func_extra_info<arg_v_t<F, true>, Ts...> : func_extra_info<Ts...> {
    static constexpr size_t nargs_locked = 1 + func_extra_info<Ts...>::nargs_locked;
};

template <typename... Ts>
struct func_extra_info<lock_self, Ts...> : func_extra_info<Ts...> {
    static constexpr size_t nargs_locked = 1 + func_extra_info<Ts...>::nargs_locked;
};

NB_INLINE void process_precall(PyObject **, size_t, detail::cleanup_list *, void *) { }

template <size_t NArgs, typename Policy>
NB_INLINE void
process_precall(PyObject **args, std::integral_constant<size_t, NArgs> nargs,
                detail::cleanup_list *cleanup, call_policy<Policy> *) {
    Policy::precall(args, nargs, cleanup);
}

NB_INLINE void process_postcall(PyObject **, size_t, PyObject *, void *) { }

template <size_t NArgs, size_t Nurse, size_t Patient>
NB_INLINE void
process_postcall(PyObject **args, std::integral_constant<size_t, NArgs>,
                 PyObject *result, nanobind::keep_alive<Nurse, Patient> *) {
    static_assert(Nurse != Patient,
                  "keep_alive with the same argument as both nurse and patient "
                  "doesn't make sense");
    static_assert(Nurse <= NArgs && Patient <= NArgs,
                  "keep_alive template parameters must be in the range "
                  "[0, number of C++ function arguments]");
    NB_CALL(keep_alive_py)(NB_CTX, Nurse   == 0 ? result : args[Nurse - 1],
                           Patient == 0 ? result : args[Patient - 1]);
}

template <size_t NArgs, typename Policy>
NB_INLINE void
process_postcall(PyObject **args, std::integral_constant<size_t, NArgs> nargs,
                 PyObject *&result, call_policy<Policy> *) {
    // result_guard avoids leaking a reference to the return object
    // if postcall throws an exception
    object result_guard = steal(result);
    Policy::postcall(args, nargs, result);
    result_guard.release();
}

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
