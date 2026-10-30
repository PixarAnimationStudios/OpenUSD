/*
    nanobind/nb_call.h: Functionality for calling Python functions from C++

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

class kwargs_proxy : public handle {
public:
    explicit kwargs_proxy(handle h) : handle(h) { }
};

class args_proxy : public handle {
public:
    explicit args_proxy(handle h) : handle(h) { }
    kwargs_proxy operator*() const { return kwargs_proxy(*this); }
};

template <typename Derived>
args_proxy api<Derived>::operator*() const {
    return args_proxy(derived().ptr());
}

/* Vectorcall borrows its arguments (PEP 590). The type caster of the handle
   family would merely take a new reference for a 'handle' or an lvalue
   'object', which the caller keeps alive across the call anyway. The call
   path skips that step and passes a borrowed pointer instead. */
template <typename T> constexpr bool call_borrows() {
    using D = std::decay_t<T>;
    return std::is_base_of_v<handle, D> &&
           (!std::is_base_of_v<object, D> || std::is_lvalue_reference_v<T> ||
            std::is_const_v<std::remove_reference_t<T>>);
}

template <rv_policy::value policy, typename T>
NB_INLINE PyObject *call_arg_value(T &&value) {
    if constexpr (call_borrows<T>())
        return value.ptr();
    else
        return make_caster<T>::from_cpp((forward_t<T>) value, policy, nullptr).ptr();
}

/// Fill a 'call_arg' entry of a call with keywords or '*'/'**' expansions
template <rv_policy::value policy, typename T>
NB_INLINE void call_arg_init(call_arg &a, T &&value) {
    using D = std::decay_t<T>;

    static_assert(!arg_traits<D>::locked,
                  "nb::arg().lock() may be used only when defining functions, "
                  "not when calling them");

    a.name = nullptr;

    if constexpr (arg_traits<D>::has_default) {
        a.kind = call_arg_kind::keyword;
        a.value = make_caster<object>::from_cpp(((forward_t<T>) value).value,
                                                policy, nullptr).ptr();
        if (value.name_) {
            cached_name name(value.name_);
            a.name = name.release(); // the record owns its fields
        }
    } else {
        a.kind = std::is_same_v<D, args_proxy>   ? call_arg_kind::args
               : std::is_same_v<D, kwargs_proxy> ? call_arg_kind::kwargs
                                                 : call_arg_kind::positional;
        a.value = make_caster<T>::from_cpp((forward_t<T>) value, policy, nullptr).ptr();
    }
}

template <typename Derived>
template <rv_policy::value policy, typename... Args>
object api<Derived>::operator()(Args &&...args_) const {
    static constexpr bool method_call =
        std::is_same_v<Derived, accessor<obj_attr>> ||
        std::is_same_v<Derived, accessor<obj_attr_own>> ||
        std::is_same_v<Derived, accessor<str_attr>>;

    static_assert(sizeof...(Args) + method_call <= 64,
                  "nanobind: too many arguments in a call, use '*' expansion");

#if !defined(NDEBUG)
    if (NB_UNLIKELY(!NB_CALL(gil_check)()))
        fail_gil();
#endif

    // Callable, or method name (borrowed unless created or fetched by value)
    PyObject *base;
    uint32_t flags = 0;

    if constexpr (method_call) {
        bool owned;
        base = derived().key(owned);
        flags = (uint32_t) call_flags::method |
                (owned ? (uint32_t) call_flags::base_owned : 0);
    } else {
        base = call_arg_value<policy>(derived());
        if constexpr (!call_borrows<const Derived &>())
            flags = (uint32_t) call_flags::base_owned;
    }

    if constexpr (((arg_traits<std::decay_t<Args>>::has_default ||
                    std::is_same_v<std::decay_t<Args>, args_proxy> ||
                    std::is_same_v<std::decay_t<Args>, kwargs_proxy>) || ...)) {
        // Call with keyword arguments and/or '*'/'**' expansions
        call_arg args[sizeof...(Args) + method_call];
        size_t i = 0;

        if constexpr (method_call)
            call_arg_init<policy>(args[i++], derived().base());
        (call_arg_init<policy>(args[i++], (forward_t<Args>) args_), ...);

        return steal(NB_CALL(obj_vectorcall_ex)(NB_CTX, base, args, i,
                                                NB_ABI_MINOR_TAG | flags));
    } else {
        // Call with only positional arguments. 'args[0]' is the writable slot
        // required by PEP 590, followed by 'self' (for method calls) and the
        // arguments. Bit 'i' of 'owned' marks a new reference in 'args[i + 1]'.
        PyObject *args[sizeof...(Args) + method_call + 1];
        uint64_t owned = 0;
        size_t i = 1;

        args[0] = nullptr;
        if constexpr (method_call)
            args[i++] = derived().base().ptr();

        ((args[i] = call_arg_value<policy>((forward_t<Args>) args_),
          owned |= (uint64_t) !call_borrows<Args>() << (i - 1), ++i), ...);

        return steal(NB_CALL(obj_vectorcall)(NB_CTX,
            base, args + 1, (i - 1) | NB_VECTORCALL_ARGUMENTS_OFFSET, owned,
            flags));
    }
}

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
