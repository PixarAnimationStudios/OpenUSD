/*
    nanobind/nb_cast.h: Type caster interface and essential type casters

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#define NB_TYPE_CASTER(Value_, descr)                                          \
    using Value = Value_;                                                      \
    static constexpr auto Name = descr;                                        \
    template <typename T_> using Cast = movable_cast_t<T_>;                    \
    template <typename T_> static constexpr bool can_cast() { return true; }   \
    template <typename T_,                                                     \
              enable_if_t<std::is_same_v<std::remove_cv_t<T_>, Value>> = 0>    \
    static handle from_cpp(T_ *p, rv_policy policy,                            \
                           cleanup_list *list) noexcept {                      \
        if (!p)                                                                \
            return none().release();                                           \
        return from_cpp(*p, policy, list);                                     \
    }                                                                          \
    explicit operator Value*() { return &value; }                              \
    explicit operator Value&() { return (Value &) value; }                     \
    explicit operator Value&&() { return (Value &&) value; }                   \
    Value value;

#define NB_MAKE_OPAQUE(...)                                                    \
    namespace nanobind::detail {                                               \
    template <> class type_caster<__VA_ARGS__>                                 \
        : public type_caster_base<__VA_ARGS__> { }; }

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

/**
 * Type casters expose a member 'Cast<T>' which users of a type caster must
 * query to determine what the caster actually can (and prefers) to produce.
 * The convenience alias ``cast_t<T>`` defined below performs this query for a
 * given type ``T``.
 *
 * Often ``cast_t<T>`` is simply equal to ``T`` or ``T&``. More significant
 * deviations are also possible, which could be due to one of the following
 * two reasons:
 *
 * 1. Efficiency: most STL type casters create a local copy (``value`` member)
 *    of the value being cast. The caller should move this value to its
 *    intended destination instead of making further copies along the way.
 *    Consequently, ``cast_t<std::vector<T>>`` yields ``cast_t<std::vector<T>>
 *    &&`` to enable such behavior.
 *
 * 2. STL pairs may contain references, and such pairs aren't
 *    default-constructible. The STL pair caster therefore cannot create a local
 *    copy and must construct the pair on the fly, which in turns means that it
 *    cannot return references. Therefore, ``cast_t<const std::pair<T1, T2>&>``
 *    yields ``std::pair<T1, T2>``.
 */

/// Ask a type caster what flavors of a type it can actually produce -- may be different from 'T'
template <typename T> using cast_t = typename make_caster<T>::template Cast<T>;

/// This is a default choice for the 'Cast' type alias described above. It
/// prefers to return rvalue references to allow the caller to move the object.
template <typename T>
using movable_cast_t =
    std::conditional_t<is_pointer_v<T>, intrinsic_t<T> *,
                       std::conditional_t<std::is_lvalue_reference_v<T>,
                                          intrinsic_t<T> &, intrinsic_t<T> &&>>;

/// This version is more careful about what the caller actually requested and
/// only moves when this was explicitly requested. It is the default for the
/// base type caster (i.e., types bound via ``nanobind::class_<..>``)
template <typename T>
using precise_cast_t =
    std::conditional_t<is_pointer_v<T>, intrinsic_t<T> *,
                       std::conditional_t<std::is_rvalue_reference_v<T>,
                                          intrinsic_t<T> &&, intrinsic_t<T> &>>;

/// Type trait to detect arguments where a value/reference cast excludes ``None``
template <typename T>
inline constexpr uint32_t none_disallowed_flag =
    (is_base_caster_v<make_caster<T>> &&
     !std::is_pointer_v<std::remove_reference_t<T>>)
        ? (uint32_t) cast_flags::none_disallowed : 0u;

/// Many type casters delegate to another caster using the pattern:
/// ~~~ .cc
/// bool from_python(handle src, uint32_t flags, cleanup_list *cl) noexcept {
///     SomeCaster c;
///     if (!c.from_python(src, flags, cl)) return false;
///     /* do something with */ c.operator T();
///     return true;
/// }
/// ~~~
/// This function adjusts the flags to avoid issues where the resulting T object
/// refers into storage that will dangle after SomeCaster is destroyed, and
/// causes a static assertion failure if that's not sufficient. Use it like:
/// ~~~ .cc
///     if (!c.from_python(src, flags_for_local_caster<T>(flags), cl))
///         return false;
/// ~~~
/// where the template argument T is the type you plan to extract.
template <typename T>
NB_INLINE uint32_t flags_for_local_caster(uint32_t flags) noexcept {
    using Caster = make_caster<T>;
    constexpr bool is_ref = std::is_pointer_v<T> || std::is_reference_v<T>;
    if constexpr (is_base_caster_v<Caster>) {
        if constexpr (is_ref) {
            // References/pointers to a type produced by implicit conversions
            // refer to storage owned by the cleanup_list. In a nb::cast() call,
            // that storage will be released before the reference can be used;
            // to prevent dangling, don't allow implicit conversions there.
            if (flags & cast_flags::manual)
                flags &= ~cast_flags::convert;
        }
        flags |= none_disallowed_flag<T>;
    } else {
        // Any pointer produced by a non-base caster will generally point
        // into storage owned by the caster, which won't live long enough.
        // Exception: the 'char' caster produces a result that points to
        // storage owned by the incoming Python 'str' object, so it's OK.
        static_assert(!is_ref || std::is_same_v<T, const char*> ||
                      (std::is_pointer_v<T> && std::is_constructible_v<T*, Caster>),
                      "nanobind generally cannot produce objects that "
                      "contain interior pointers T* (or references T&) if "
                      "the pointee T is not handled by nanobind's regular "
                      "class binding mechanism. For example, you can write "
                      "a function that accepts int*, or std::vector<int>, "
                      "but not std::vector<int*>.");
    }
    return flags;
}

template <typename T>
struct type_caster<T, enable_if_t<std::is_arithmetic_v<T> && !is_std_char_v<T>>> {
    NB_INLINE bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept {
        if constexpr (std::is_floating_point_v<T>) {
            if constexpr (std::is_same_v<T, double>) {
                return NB_CALL(load_f64)(NB_CTX_C(cleanup), src.ptr(), flags, &value);
            } else if constexpr (std::is_same_v<T, float>) {
                return NB_CALL(load_f32)(NB_CTX_C(cleanup), src.ptr(), flags, &value);
            } else {
                double d;
                if (!NB_CALL(load_f64)(NB_CTX_C(cleanup), src.ptr(), flags, &d))
                    return false;
                T result = (T) d;
                if ((flags & cast_flags::convert)
                        || (double) result == d
                        || (result != result && d != d)) {
                    value = result;
                    return true;
                }
                return false;
            }
        } else {
            if constexpr (std::is_signed_v<T>) {
                if constexpr (sizeof(T) == 8)
                    return NB_CALL(load_i64)(NB_CTX_C(cleanup), src.ptr(), flags, (int64_t *) &value);
                else if constexpr (sizeof(T) == 4)
                    return NB_CALL(load_i32)(NB_CTX_C(cleanup), src.ptr(), flags, (int32_t *) &value);
                else if constexpr (sizeof(T) == 2)
                    return NB_CALL(load_i16)(NB_CTX_C(cleanup), src.ptr(), flags, (int16_t *) &value);
                else
                    return NB_CALL(load_i8)(NB_CTX_C(cleanup), src.ptr(), flags, (int8_t *) &value);
            } else {
                if constexpr (sizeof(T) == 8)
                    return NB_CALL(load_u64)(NB_CTX_C(cleanup), src.ptr(), flags, (uint64_t *) &value);
                else if constexpr (sizeof(T) == 4)
                    return NB_CALL(load_u32)(NB_CTX_C(cleanup), src.ptr(), flags, (uint32_t *) &value);
                else if constexpr (sizeof(T) == 2)
                    return NB_CALL(load_u16)(NB_CTX_C(cleanup), src.ptr(), flags, (uint16_t *) &value);
                else
                    return NB_CALL(load_u8)(NB_CTX_C(cleanup), src.ptr(), flags, (uint8_t *) &value);
            }
        }
    }

    NB_INLINE static handle from_cpp(T src, rv_policy, cleanup_list *) noexcept {
        if constexpr (std::is_floating_point_v<T>) {
            return PyFloat_FromDouble((double) src);
        } else {
            if constexpr (std::is_signed_v<T>) {
                if constexpr (sizeof(T) <= sizeof(long))
                    return PyLong_FromLong((long) src);
                else
                    return PyLong_FromLongLong((long long) src);
            } else {
                if constexpr (sizeof(T) <= sizeof(unsigned long))
                    return PyLong_FromUnsignedLong((unsigned long) src);
                else
                    return PyLong_FromUnsignedLongLong((unsigned long long) src);
            }
        }
    }

    NB_TYPE_CASTER(T, const_name<std::is_integral_v<T>>("int", "float"))
};

template <typename T>
struct type_caster<T, enable_if_t<std::is_enum_v<T>>> {
    NB_INLINE bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept {
        int64_t result;
        bool rv = NB_CALL(enum_from_python)(NB_CTX_C(cleanup), &typeid(T), src.ptr(), &result, flags);
        if (rv)
            value = (T) result;
        return rv;
    }

    NB_INLINE static handle from_cpp(T src, rv_policy, cleanup_list *cleanup) noexcept {
        return NB_CALL(enum_from_cpp)(NB_CTX_C(cleanup), &typeid(T), (int64_t) src);
    }

    NB_TYPE_CASTER(T, const_name<T>())
};

template <> struct type_caster<void_type> {
    static constexpr auto Name = const_name("None");
};

template <> struct type_caster<void> {
    template <typename T_> using Cast = void *;
    template <typename T_> static constexpr bool can_cast() { return true; }
    using Value = void*;
    static constexpr auto Name = const_name("typing_extensions.CapsuleType");
    explicit operator void *() { return value; }
    Value value;

    bool from_python(handle src, uint32_t, cleanup_list *) noexcept {
        if (src.is_none()) {
            value = nullptr;
            return true;
        } else {
            if (!PyCapsule_CheckExact(src.ptr()))
                return false;
            value = PyCapsule_GetPointer(src.ptr(), "nb_handle");
            if (!value) {
                PyErr_Clear();
                return false;
            }
            return true;
        }
    }

    static handle from_cpp(void *ptr, rv_policy, cleanup_list *) noexcept {
        if (ptr)
            return PyCapsule_New(ptr, "nb_handle", nullptr);
        else
            return none().release();
    }
};

template <typename T> struct none_caster {
    bool from_python(handle src, uint32_t, cleanup_list *) noexcept {
        if (src.is_none())
            return true;
        return false;
    }

    static handle from_cpp(T, rv_policy, cleanup_list *) noexcept {
        return none().release();
    }

    NB_TYPE_CASTER(T, const_name("None"))
};

template <> struct type_caster<std::nullptr_t> : none_caster<std::nullptr_t> { };
template <> struct has_arg_defaults<std::nullptr_t> : std::true_type {};

template <> struct type_caster<bool> {
    bool from_python(handle src, uint32_t, cleanup_list *) noexcept {
        if (src.ptr() == true_ptr()) {
            value = true;
            return true;
        } else if (src.ptr() == false_ptr()) {
            value = false;
            return true;
        } else {
            return false;
        }
    }

    static handle from_cpp(bool src, rv_policy, cleanup_list *) noexcept {
        return src ? true_ref() : false_ref();
    }

    NB_TYPE_CASTER(bool, const_name("bool"))
};

template <> struct type_caster<char> {
    using Value = const char *;
    Value value;
    Py_ssize_t size;
    static constexpr auto Name = const_name("str");
    template <typename T_>
    using Cast = std::conditional_t<is_pointer_v<T_>, const char *, char>;

    bool from_python(handle src, uint32_t, cleanup_list *) noexcept {
        if (!str_check(src.ptr()))
            return false;
        value = PyUnicode_AsUTF8AndSize(src.ptr(), &size);
        if (!value) {
            PyErr_Clear();
            return false;
        }
        return true;
    }

    static handle from_cpp(const char *value, rv_policy,
                           cleanup_list *) noexcept {
        if (value == nullptr)
            return none_ref();
        return PyUnicode_FromString(value);
    }

    static handle from_cpp(char value, rv_policy, cleanup_list *) noexcept {
        return PyUnicode_FromStringAndSize(&value, 1);
    }

    template <typename T_>
    NB_INLINE bool can_cast() const noexcept {
        return std::is_pointer_v<T_> || (value && size == 1);
    }

    explicit operator const char *() { return value; }

    explicit operator char() {
        if (can_cast<char>())
            return value[0];
        else
            throw next_overload();
    }
};

template <typename T> struct type_caster<pointer_and_handle<T>> {
    using Caster = make_caster<T>;
    using T2 = pointer_and_handle<T>;
    NB_TYPE_CASTER(T2, Caster::Name)

    bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept {
        // Fast path for implicit ``self`` argument from ``nb_type_vectorcall()``
        if (flags & cast_flags::trusted) {
            value.h = src;
            value.p = (T *) NB_CALL(nb_inst_ptr)(src.ptr());
            return true;
        }
        Caster c;
        if (!c.from_python(src, flags_for_local_caster<T*>(flags), cleanup) ||
            !c.template can_cast<T*>())
            return false;
        value.h = src;
        value.p = c.operator T*();
        return true;
    }
};

template <> struct type_caster<fallback> {
    NB_TYPE_CASTER(fallback, const_name("object"))
    bool from_python(handle src, uint32_t flags, cleanup_list *) noexcept {
        if (!(flags & cast_flags::convert))
            return false;
        value = src;
        return true;
    }
};

template <typename T> struct typed_base_name {
      static constexpr auto Name = type_caster<T>::Name;
};

// Base case: typed<T, Ts...> renders as T[Ts...]
template <typename T, typename... Ts> struct typed_name {
    static constexpr auto Name =
            typed_base_name<intrinsic_t<T>>::Name + const_name("[") +
            concat(const_name<std::is_same_v<Ts, ellipsis>>(const_name("..."),
                    make_caster<Ts>::Name)...) + const_name("]");
};

// typed<object, T> or typed<handle, T> renders as T, rather than as
// the nonsensical object[T]
template <typename T> struct typed_name<object, T> {
    static constexpr auto Name = make_caster<T>::Name;
};
template <typename T> struct typed_name<handle, T> {
    static constexpr auto Name = make_caster<T>::Name;
};

// typed<callable, R(Args...)> renders as Callable[[Args...], R]
template <typename R, typename... Args>
struct typed_name<callable, R(Args...)> {
    using Ret = std::conditional_t<std::is_void_v<R>, void_type, R>;
    static constexpr auto Name =
            const_name("collections.abc.Callable[[") +
            concat(make_caster<Args>::Name...) + const_name("], ") +
            make_caster<Ret>::Name + const_name("]");
};
// typed<callable, R(...)> renders as Callable[..., R]
template <typename R>
struct typed_name<callable, R(...)> {
    using Ret = std::conditional_t<std::is_void_v<R>, void_type, R>;
    static constexpr auto Name =
            const_name("collections.abc.Callable[..., ") +
            make_caster<Ret>::Name + const_name("]");
};

template <typename T, typename... Ts> struct type_caster<typed<T, Ts...>> {
    using Caster = make_caster<T>;
    using Typed = typed<T, Ts...>;

    NB_TYPE_CASTER(Typed, (typed_name<T, Ts...>::Name))

    bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept {
        Caster caster;
        if (!caster.from_python(src, flags_for_local_caster<T>(flags), cleanup) ||
            !caster.template can_cast<T>())
            return false;
        value = caster.operator cast_t<T>();
        return true;
    }

    static handle from_cpp(const Value &src, rv_policy policy, cleanup_list *cleanup) noexcept {
        return Caster::from_cpp(src, policy, cleanup);
    }
};

template <typename T>
struct type_caster<T, enable_if_t<std::is_base_of_v<detail::api_tag, T> && !T::nb_typed>> {
public:
    NB_TYPE_CASTER(T, T::Name)

    type_caster() : value(nullptr, ::nanobind::detail::steal_t()) { }

    bool from_python(handle src, uint32_t, cleanup_list *) noexcept {
        if (!isinstance<T>(src))
            return false;

        if constexpr (std::is_base_of_v<object, T>)
            value = borrow<T>(src);
        else
            value = src;

        return true;
    }

    static handle from_cpp(T&& src, rv_policy, cleanup_list *) noexcept {
        if constexpr (std::is_base_of_v<object, T>)
            return src.release();
        else if constexpr (is_accessor_v<T>)
            return from_accessor(src);
        else
            return src.inc_ref();
    }

    static handle from_cpp(const T &src, rv_policy, cleanup_list *) noexcept {
        if constexpr (is_accessor_v<T>)
            return from_accessor(src);
        else
            return src.inc_ref();
    }

private:
    /// Accessors may fail by raising an exception
    static handle from_accessor(const T &src) noexcept {
        try {
            return src.inc_ref();
        } catch (python_error &e) {
            e.restore();
            return handle();
        }
    }
};

template <typename T> NB_INLINE rv_policy infer_policy(rv_policy policy) {
    if constexpr (is_pointer_v<T>) {
        if (policy == rv_policy::automatic)
            policy = rv_policy::take_ownership;
        else if (policy == rv_policy::automatic_reference)
            policy = rv_policy::reference;
    } else if constexpr (std::is_lvalue_reference_v<T>) {
        if (policy == rv_policy::automatic ||
            policy == rv_policy::automatic_reference)
            policy = rv_policy::copy;
    } else {
        if (policy == rv_policy::automatic ||
            policy == rv_policy::automatic_reference ||
            policy == rv_policy::reference ||
            policy == rv_policy::reference_internal)
            policy = rv_policy::move;
    }
    return policy;
}

template <typename T, typename SFINAE = int> struct type_hook : std::false_type { };

template <typename Type_> struct type_caster_base : type_caster_base_tag {
    using Type = Type_;
    static constexpr auto Name = const_name<Type>();
    template <typename T> using Cast = precise_cast_t<T>;

    NB_INLINE bool from_python(handle src, uint32_t flags,
                               cleanup_list *cleanup) noexcept {
        // The 'trusted' fast path lives in the pointer_and_handle caster (the
        // only one that is ever trusted) and, as a fallback, in nb_type_get.
        // The generic base caster therefore need not test for it here, which
        // would only add a never-taken branch to every bound-type argument.
        return NB_CALL(nb_type_get)(NB_CTX_C(cleanup), &typeid(Type), src.ptr(),
                                    flags, cleanup, (void **) &value);
    }

    template <typename T>
    NB_INLINE static handle from_cpp(T &&value, rv_policy policy,
                                     cleanup_list *cleanup) noexcept {
        Type *ptr;
        if constexpr (is_pointer_v<T>)
            ptr = (Type *) value;
        else
            ptr = (Type *) &value;

        policy = infer_policy<T>(policy);
        const std::type_info *type = &typeid(Type);

        constexpr bool has_type_hook =
            !std::is_base_of_v<std::false_type, type_hook<Type>>;
        if constexpr (has_type_hook)
            type = type_hook<Type>::get(ptr);

        const std::type_info *type_p = nullptr;
        if constexpr (std::is_polymorphic_v<Type>)
            type_p = (!has_type_hook && ptr) ? &typeid(*ptr) : nullptr;

        return NB_CALL(nb_type_put)(NB_CTX_C(cleanup), type, type_p, ptr, policy,
                                    cleanup, nullptr);
    }

    template <typename T_>
    bool can_cast() const noexcept {
        return std::is_pointer_v<T_> || (value != nullptr);
    }

    operator Type*() { return value; }
    // Code using this cast operator must ensure it is safe (see none_disallowed)
    operator Type&() { return *value; }
    operator Type&&() { return (Type &&) *value; }

private:
    Type *value;
};

template <typename Type, typename SFINAE>
struct type_caster : type_caster_base<Type> { };

template <bool Convert, typename T>
T cast_impl(handle h) {
    using Caster = detail::make_caster<T>;

    // A returned reference/pointer would usually refer into the type_caster
    // object, which will be destroyed before the returned value can be used,
    // so we prohibit it by default, with two exceptions that we know are safe:
    //
    // - If we're casting to a bound object type, the returned pointer points
    //   into storage owned by that object, not the type caster. Note this is
    //   only safe if we don't allow implicit conversions, because the pointer
    //   produced after an implicit conversion points into storage owned by
    //   a temporary object in the cleanup list, and we have to release those
    //   temporaries before we return.
    //
    // - If we're casting to const char*, the caster was provided by nanobind,
    //   and we know it will only accept Python 'str' objects, producing
    //   a pointer to storage owned by that object.

    constexpr bool is_ref = std::is_reference_v<T> || std::is_pointer_v<T>;
    static_assert(
        !is_ref ||
            is_base_caster_v<Caster> ||
            std::is_same_v<const char *, T>,
        "nanobind::cast(): cannot return a reference to a temporary.");

    Caster caster;
    bool rv;
    if constexpr (Convert && !is_ref) {
        // Release the values in the cleanup list only after we
        // initialize the return object, since the initialization
        // might access those temporaries.
        struct raii_cleanup {
            cleanup_list list{nullptr, NB_CTX};
            ~raii_cleanup() { list.release(); }
        } cleanup;
        rv = caster.from_python(h.ptr(),
                                cast_flags::convert | cast_flags::manual,
                                &cleanup.list) &&
             caster.template can_cast<T>();
        if (!rv)
            detail::raise_python_or_cast_error();
        return caster.operator cast_t<T>();
    } else {
        rv = caster.from_python(h.ptr(), cast_flags::manual, nullptr) &&
             caster.template can_cast<T>();
        if (!rv)
            detail::raise_python_or_cast_error();
        return caster.operator cast_t<T>();
    }
}

template <bool Convert, typename T>
bool try_cast_impl(handle h, T &out) noexcept {
    using Caster = detail::make_caster<T>;

    // See comments in cast_impl above
    constexpr bool is_ref = std::is_reference_v<T> || std::is_pointer_v<T>;
    static_assert(
        !is_ref ||
            is_base_caster_v<Caster> ||
            std::is_same_v<const char *, T>,
        "nanobind::try_cast(): cannot return a reference to a temporary.");

    Caster caster;
    bool rv;
    if constexpr (Convert && !is_ref) {
        cleanup_list cleanup(nullptr, NB_CTX);
        rv = caster.from_python(h.ptr(),
                                cast_flags::convert | cast_flags::manual,
                                &cleanup) &&
             caster.template can_cast<T>();
        if (rv) {
            out = caster.operator cast_t<T>();
        }
        cleanup.release(); // 'from_python' is 'noexcept', so this always runs
    } else {
        rv = caster.from_python(h.ptr(), cast_flags::manual, nullptr) &&
             caster.template can_cast<T>();
        if (rv) {
            out = caster.operator cast_t<T>();
        }
    }

    return rv;
}

NAMESPACE_END(detail)

template <typename T, typename Derived>
NB_INLINE T cast(const detail::api<Derived> &value, bool convert = true) {
    if constexpr (std::is_same_v<T, void>) {
        (void) value; (void) convert;
        return;
    } else {
        if (convert)
            return detail::cast_impl<true, T>(value);
        else
            return detail::cast_impl<false, T>(value);
    }
}

template <typename T, typename Derived>
NB_INLINE bool try_cast(const detail::api<Derived> &value, T &out, bool convert = true) noexcept {
    if (convert)
        return detail::try_cast_impl<true, T>(value, out);
    else
        return detail::try_cast_impl<false, T>(value, out);
}

template <typename T>
object cast(T &&value, rv_policy policy = rv_policy::automatic_reference) {
    handle h = detail::make_caster<T>::from_cpp((detail::forward_t<T>) value,
                                                policy, nullptr);
    if (!h.is_valid())
        detail::raise_python_or_cast_error();

    return steal(h);
}

template <typename T>
object cast(T &&value, rv_policy policy, handle parent) {
    detail::cleanup_list cleanup(parent.ptr(), NB_CTX);
    handle h = detail::make_caster<T>::from_cpp((detail::forward_t<T>) value,
                                                policy, &cleanup);

    cleanup.release();

    if (!h.is_valid())
        detail::raise_python_or_cast_error();

    return steal(h);
}

template <typename T> object find(const T &value) noexcept {
    return steal(detail::make_caster<T>::from_cpp(value, rv_policy::none, nullptr));
}

template <rv_policy::value policy = rv_policy::automatic_v, typename... Args>
tuple make_tuple(Args &&...args) {
    constexpr size_t Size = sizeof...(Args);

    PyObject *items[Size > 0 ? Size : 1] { };
    size_t i = 0;

    (void) (... && ((items[i++] = detail::make_caster<Args>::from_cpp(
                         (detail::forward_t<Args>) args, policy, nullptr)
                         .ptr()) != nullptr));

    PyObject *result = NB_CALL(tuple_new)(items, Size);
    if (NB_UNLIKELY(!result))
        detail::raise_python_or_cast_error();

    return steal<tuple>(result);
}

template <uint32_t Flags, bool Locked> template <typename T>
arg_v_t<Flags, Locked> arg_t<Flags, Locked>::operator=(T &&value) const {
    return arg_v_t<Flags, Locked>(*this, cast((detail::forward_t<T>) value));
}

template <typename Impl> template <typename T>
detail::accessor<Impl>& detail::accessor<Impl>::operator=(T &&value) {
    object result = cast((detail::forward_t<T>) value);
    Impl::set(m_base, m_key, result.ptr());
    return *this;
}

template <typename T> void list::append(T &&value) {
    object o = nanobind::cast((detail::forward_t<T>) value);
    if (PyList_Append(m_ptr, o.ptr()))
        detail::raise_python_error();
}

template <typename T> void list::insert(Py_ssize_t index, T &&value) {
    object o = nanobind::cast((detail::forward_t<T>) value);
    if (PyList_Insert(m_ptr, index, o.ptr()))
        detail::raise_python_error();
}

template <typename Seq> template <typename T>
NB_INLINE bool builder<Seq>::put(T &&value) noexcept {
    handle h = detail::make_caster<T>::from_cpp((detail::forward_t<T>) value,
                                                rv_policy::automatic, nullptr);
    if (NB_UNLIKELY(!h.is_valid()))
        return false;

    m_core.put(h);
    return true;
}

template <typename T, detail::enable_if_t<!detail::is_c_string_v<T>>>
bool dict::contains(T&& key) const {
    object o = nanobind::cast((detail::forward_t<T>) key);
    int rv = PyDict_Contains(m_ptr, o.ptr());
    if (rv == -1)
        detail::raise_python_error();
    return rv == 1;
}

inline bool dict::contains(detail::str_key key) const {
    return NB_CALL(contains_str)(NB_CTX, m_ptr, key.str, key.bound);
}

template <typename T, detail::enable_if_t<!detail::is_c_string_v<T>>>
bool set::contains(T&& key) const {
    object o = nanobind::cast((detail::forward_t<T>) key);
    int rv = PySet_Contains(m_ptr, o.ptr());
    if (rv == -1)
        detail::raise_python_error();
    return rv == 1;
}

inline bool set::contains(detail::str_key key) const {
    return NB_CALL(contains_str)(NB_CTX, m_ptr, key.str, key.bound);
}

template <typename T> void set::add(T&& key) {
    object o = nanobind::cast((detail::forward_t<T>) key);
    int rv = PySet_Add(m_ptr, o.ptr());
    if (rv == -1)
        detail::raise_python_error();
}

template <typename T> bool set::discard(T &&value) {
    object o = nanobind::cast((detail::forward_t<T>) value);
    int rv = PySet_Discard(m_ptr, o.ptr());
    if (rv < 0)
        detail::raise_python_error();
    return rv == 1;
}

template <typename T, detail::enable_if_t<!detail::is_c_string_v<T>>>
bool frozenset::contains(T&& key) const {
    object o = nanobind::cast((detail::forward_t<T>) key);
    int rv = PySet_Contains(m_ptr, o.ptr());
    if (rv == -1)
        detail::raise_python_error();
    return rv == 1;
}

inline bool frozenset::contains(detail::str_key key) const {
    return NB_CALL(contains_str)(NB_CTX, m_ptr, key.str, key.bound);
}

template <typename T, detail::enable_if_t<!detail::is_c_string_v<T>>>
bool mapping::contains(T&& key) const {
    object o = nanobind::cast((detail::forward_t<T>) key);
#if NB_PYTHON_VERSION >= 0x030D0000
    int rv = PyMapping_HasKeyWithError(m_ptr, o.ptr());
#else
    int rv = PyMapping_HasKey(m_ptr, o.ptr());
#endif
    if (rv == -1)
        detail::raise_python_error();
    return rv == 1;
}

inline bool mapping::contains(detail::str_key key) const {
    return NB_CALL(contains_str)(NB_CTX, m_ptr, key.str, key.bound);
}

NAMESPACE_END(NB_NAMESPACE)
