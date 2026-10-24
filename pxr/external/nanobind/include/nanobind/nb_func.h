/*
    nanobind/nb_func.h: Functionality for binding C++ functions/methods

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <typename Caster>
bool from_python_remember_conv(Caster &c, PyObject **args, uint32_t flags,
                               cleanup_list *cleanup, size_t index) {
    size_t size_before = cleanup->size();
    if (!c.from_python(args[index], flags, cleanup))
        return false;

    // If an implicit conversion took place, update the 'args' array so that
    // any keep_alive annotation or postcall hook can be aware of this change
    size_t size_after = cleanup->size();
    if (size_after != size_before)
        args[index] = (*cleanup)[size_after - 1];

    return true;
}

/// Compute the compile-time cast flags of every C++ function parameter.
/// Annotations map to consecutive parameters, skipping the 'self' argument
/// of methods (whose flags are zero).
template <bool IsMethod, bool HasArgAnnotations, typename... Extra,
          typename Return, typename... Args>
constexpr auto arg_flags_static(Return (*)(Args...)) {
    constexpr size_t N = sizeof...(Args), M = sizeof...(Extra);

    // Contribution of the C++ parameter types: std::optional-style arguments
    // implicitly accept 'None'; value/reference bindings of bound types do not
    constexpr uint32_t type_flags[] = {
        ((has_arg_defaults_v<Args> ? (uint32_t) cast_flags::accepts_none : 0u) |
         none_disallowed_flag<Args>)..., 0u };

    // Which 'Extra' entries are argument annotations, and their flags
    constexpr bool     is_ann[]    = { arg_traits<Extra>::is_arg..., false };
    constexpr uint32_t ann_flags[] = { arg_traits<Extra>::flags..., 0u };

    struct { uint32_t v[N == 0 ? 1 : N]; } r{};
    size_t j = 0;
    for (size_t i = IsMethod ? 1 : 0; i < N; ++i) {
        uint32_t f = arg_flags_default;
        if constexpr (HasArgAnnotations) {
            while (j < M && !is_ann[j]) // advance to the next annotation
                j++;
            if (j < M)
                f = ann_flags[j++];
        }
        r.v[i] = f | type_flags[i];
    }

    (void) is_ann; (void) ann_flags; (void) j;
    return r;
}

/// Combine an argument's compile-time cast flags with the call-wide dynamic
/// word assembled by the dispatcher
template <size_t Index, uint32_t ArgFlags>
NB_INLINE uint32_t combine_flags(uint32_t call_flags) {
    // 'construct' and 'trusted' only apply to the 'self' argument
    if constexpr (Index != 0)
        call_flags &= ~(cast_flags::construct |
                        cast_flags::trusted);

    // Implicit conversion requires agreement of both flag words
    if constexpr (!(ArgFlags & cast_flags::convert))
        call_flags &= ~cast_flags::convert;

    return call_flags | (ArgFlags & ~cast_flags::convert);
}

// Return the number of nb::arg and nb::arg_v types in the first I types Ts.
// Invoke with std::make_index_sequence<sizeof...(Ts)>() to provide
// an index pack 'Is' that parallels the types pack Ts.
template <size_t I, typename... Ts, size_t... Is>
constexpr size_t count_args_before_index(std::index_sequence<Is...>) {
    static_assert(sizeof...(Is) == sizeof...(Ts));
    return ((Is < I && arg_traits<Ts>::is_arg) + ... + 0);
}

#if defined(NB_FREE_THREADED)
struct ft_args_collector {
    PyObject **args;
    handle h1;
    handle h2;
    size_t index = 0;

    NB_INLINE explicit ft_args_collector(PyObject **a) : args(a) {}
    template <typename T> NB_INLINE void apply(T *) {
        if constexpr (arg_traits<T>::is_arg) {
            if constexpr (arg_traits<T>::locked) {
                if (h1.ptr() == nullptr)
                    h1 = args[index];
                h2 = args[index];
            }
            ++index;
        }
    }
};

struct ft_args_guard {
    NB_INLINE void lock(const ft_args_collector& info) {
        PyCriticalSection2_Begin(&cs, info.h1.ptr(), info.h2.ptr());
    }
    ~ft_args_guard() {
        PyCriticalSection2_End(&cs);
    }
    PyCriticalSection2 cs;
};
#endif

struct no_guard {};

template <bool ReturnRef, bool CheckGuard, typename Func, typename Return,
          typename... Args, size_t... Is, typename... Extra>
NB_INLINE PyObject *func_create(Func &&func, Return (*)(Args...),
                                std::index_sequence<Is...> is,
                                const Extra &...extra) {
    using Info = func_extra_info<Extra...>;

    if constexpr (CheckGuard && !std::is_same_v<typename Info::call_guard, void>) {
        return func_create<ReturnRef, false>(
            [func = (forward_t<Func>) func](Args... args) NB_INLINE_LAMBDA -> Return {
                typename Info::call_guard::type g;
                (void) g;
                return func((forward_t<Args>) args...);
            },
            (Return(*)(Args...)) nullptr, is, extra...);
    }

    (void) is;

    // Detect locations of nb::args / nb::kwargs (if they exist).
    // Find the first and last occurrence of each; we'll later make sure these
    // match, in order to guarantee there's only one instance.
    static constexpr size_t
        args_pos_1 = index_1_v<std::is_same_v<intrinsic_t<Args>, args>...>,
        args_pos_n = index_n_v<std::is_same_v<intrinsic_t<Args>, args>...>,
        kwargs_pos_1 = index_1_v<std::is_same_v<intrinsic_t<Args>, kwargs>...>,
        kwargs_pos_n = index_n_v<std::is_same_v<intrinsic_t<Args>, kwargs>...>,
        nargs = sizeof...(Args);

    // Detect location of nb::kw_only, if supplied. As with args/kwargs we find
    // the first and last location and later verify they match. Note this is an
    // index in Extra... while args/kwargs_pos_* are indices in Args...
    constexpr size_t
        kwonly_pos_1 = index_1_v<std::is_same_v<kw_only, Extra>...>,
        kwonly_pos_n = index_n_v<std::is_same_v<kw_only, Extra>...>;

    constexpr bool has_arg_defaults = (detail::has_arg_defaults_v<Args> || ... || false);

    // Determine the number of nb::arg/nb::arg_v annotations
    constexpr size_t nargs_provided =
        (arg_traits<Extra>::is_arg + ... + 0);
    static constexpr bool is_method_det =
        (std::is_same_v<is_method, Extra> + ... + 0) != 0;
    constexpr bool is_getter_det =
        (std::is_same_v<is_getter, Extra> + ... + 0) != 0;
    static constexpr bool has_arg_annotations = has_arg_defaults ||
        (nargs_provided > 0 && !is_getter_det);

    // Number of 'arg_data_init' records in the function record below. This can
    // be zero even when 'has_arg_annotations' is set, e.g. for a method whose
    // only parameter is a 'self' of type 'std::optional<T>'.
    constexpr size_t nrec =
        has_arg_defaults ? (nargs - is_method_det) : nargs_provided;

    // Determine the number of potentially-locked function arguments
    static constexpr bool lock_self_det =
        (std::is_same_v<lock_self, Extra> + ... + 0) != 0;
    static_assert(Info::nargs_locked <= 2,
        "At most two function arguments can be locked");
    static_assert(!(lock_self_det && !is_method_det),
        "The nb::lock_self() annotation only applies to methods");

    // Arguments after nb::args are implicitly keyword-only even if there is no
    // nb::kw_only annotation
    constexpr bool explicit_kw_only = kwonly_pos_1 != sizeof...(Extra);
    constexpr bool implicit_kw_only = args_pos_1 + 1 < kwargs_pos_1;

    // A few compile-time consistency checks
    static_assert(args_pos_1 == args_pos_n && kwargs_pos_1 == kwargs_pos_n,
        "Repeated use of nb::kwargs or nb::args in the function signature!");

    // If annotations are provided, they must exactly match the parameter count
    static_assert(!has_arg_annotations || nargs_provided == 0 ||
                  nargs_provided + is_method_det == nargs,
        "The number of nb::arg annotations must match the argument count!");
    static_assert(kwargs_pos_1 == nargs || kwargs_pos_1 + 1 == nargs,
        "nb::kwargs must be the last element of the function signature!");
    static_assert(args_pos_1 == nargs || args_pos_1 < kwargs_pos_1,
        "nb::args must precede nb::kwargs if both are present!");
    // Implicit annotations carry no name and cannot satisfy this requirement
    static_assert(nargs_provided > 0 || (!implicit_kw_only && !explicit_kw_only),
        "Keyword-only arguments must have names!");

    // Find the index in Args... of the first keyword-only parameter. Since
    // the 'self' parameter doesn't get a nb::arg annotation, we must adjust
    // by 1 for methods. Note that nargs_before_kw_only is only used if
    // a kw_only annotation exists (i.e., if explicit_kw_only is true);
    // the conditional is just to save the compiler some effort otherwise.
    constexpr size_t nargs_before_kw_only =
        explicit_kw_only
            ? is_method_det + count_args_before_index<kwonly_pos_1, Extra...>(
                  std::make_index_sequence<sizeof...(Extra)>())
            : nargs;

    (void) kwonly_pos_n;

    if constexpr (explicit_kw_only) {
        static_assert(kwonly_pos_1 == kwonly_pos_n,
            "Repeated use of nb::kw_only annotation!");

        // If both kw_only and *args are specified, kw_only must be
        // immediately after the nb::arg for *args.
        static_assert(args_pos_1 == nargs || nargs_before_kw_only == args_pos_1 + 1,
            "Arguments after nb::args are implicitly keyword-only; any "
            "nb::kw_only() annotation must be positioned to reflect that!");

        // If both kw_only and **kwargs are specified, kw_only must be
        // before the nb::arg for **kwargs.
        static_assert(nargs_before_kw_only < kwargs_pos_1,
            "Variadic nb::kwargs are implicitly keyword-only; any "
            "nb::kw_only() annotation must be positioned to reflect that!");
    }

    // Collect function signature information for the docstring
    using cast_out = make_caster<
        std::conditional_t<std::is_void_v<Return>, void_type, Return>>;

    // Compile-time function signature
    static constexpr auto descr =
        const_name("(") +
        concat(type_descr(
            make_caster<remove_opt_mono_t<intrinsic_t<Args>>>::Name)...) +
        const_name(") -> ") + cast_out::Name;

    // std::type_info for all function arguments
    const std::type_info* descr_types[descr.type_count() + 1];
    descr.put_types(descr_types);

    // Auxiliary data structure to capture the provided function/closure
    struct capture {
        std::remove_reference_t<Func> func;
    };

    // The following temporary record will describe the function in detail
    func_data_init<nrec> f;

    f.flags = NB_ABI_MINOR_TAG |
              (args_pos_1   < nargs ? (uint32_t) func_flags::has_var_args   : 0) |
              (kwargs_pos_1 < nargs ? (uint32_t) func_flags::has_var_kwargs : 0) |
              (ReturnRef            ? (uint32_t) func_flags::return_ref     : 0) |
              (has_arg_annotations  ? (uint32_t) func_flags::has_args       : 0);

    // Store captured function inside 'func_data_init' if there is space. Issues
    // with aliasing are resolved via separate compilation of libnanobind.
    if constexpr (sizeof(capture) <= sizeof(f.capture)) {
        capture *cap = (capture *) f.capture;
        new (cap) capture{ (forward_t<Func>) func };

        if constexpr (!std::is_trivially_destructible_v<capture>) {
            f.flags |= (uint32_t) func_flags::has_free;
            f.free_capture = [](void *p) {
                ((capture *) p)->~capture();
            };
        }
    } else {
        void **cap = (void **) f.capture;
        cap[0] = new capture{ (forward_t<Func>) func };

        f.flags |= (uint32_t) func_flags::has_free;
        f.free_capture = [](void *p) {
            delete (capture *) ((void **) p)[0];
        };
    }

    f.impl = [](void *p, PyObject **args, uint32_t call_flags,
                cleanup_list *cleanup)
                    NB_INLINE_LAMBDA -> PyObject * {
        (void) p; (void) args; (void) call_flags; (void) cleanup;

        const capture *cap;
        if constexpr (sizeof(capture) <= sizeof(f.capture))
            cap = (capture *) p;
        else
            cap = (capture *) ((void **) p)[0];

        tuple<make_caster<Args>...> in;
        (void) in;

#if defined(NB_FREE_THREADED)
        std::conditional_t<Info::nargs_locked != 0, ft_args_guard, no_guard> guard;
        if constexpr (Info::nargs_locked) {
            ft_args_collector collector{args};
            if constexpr (is_method_det) {
                if constexpr (lock_self_det)
                    collector.apply((arg_locked *) nullptr);
                else
                    collector.apply((arg *) nullptr);
            }
            (collector.apply((Extra *) nullptr), ...);
            guard.lock(collector);
        }
#endif

        // Per-argument cast flags, indexed by C++ parameter
        constexpr auto arg_flags =
            arg_flags_static<is_method_det, has_arg_annotations, Extra...>(
                (Return (*)(Args...)) nullptr);

        if constexpr (Info::pre_post_hooks) {
            std::integral_constant<size_t, nargs> nargs_c;
            (process_precall(args, nargs_c, cleanup, (Extra *) nullptr), ...);
            if ((!from_python_remember_conv(
                     in.template get<Is>(), args,
                     combine_flags<Is, arg_flags.v[Is]>(call_flags),
                     cleanup, Is) || ...))
                return NB_NEXT_OVERLOAD;
        } else {
            if ((!in.template get<Is>().from_python(
                     args[Is],
                     combine_flags<Is, arg_flags.v[Is]>(call_flags),
                     cleanup) || ...))
                return NB_NEXT_OVERLOAD;
        }

        PyObject *result;
        if constexpr (std::is_void_v<Return>) {
#if defined(_WIN32) && !defined(__CUDACC__) // temporary workaround for an internal compiler error in MSVC
            cap->func(static_cast<cast_t<Args>>(in.template get<Is>())...);
#else
            cap->func(in.template get<Is>().operator cast_t<Args>()...);
#endif
            result = none_ref();
        } else {
#if defined(_WIN32) && !defined(__CUDACC__) // temporary workaround for an internal compiler error in MSVC
            result = cast_out::from_cpp(
                       cap->func(static_cast<cast_t<Args>>(in.template get<Is>())...),
                       rv_policy(Info::policy), cleanup).ptr();
#else
            result = cast_out::from_cpp(
                       cap->func((in.template get<Is>())
                                     .operator cast_t<Args>()...),
                       rv_policy(Info::policy), cleanup).ptr();
#endif
        }

        if constexpr (Info::pre_post_hooks) {
            std::integral_constant<size_t, nargs> nargs_c;
            (process_postcall(args, nargs_c, result, (Extra *) nullptr), ...);
        }

        return result;
    };

    f.descr = descr.text;
    f.descr_types = descr_types;
    f.nargs = nargs;

    // Set nargs_pos to the number of C++ function parameters (Args...) that
    // can be filled from Python positional arguments in a one-to-one fashion.
    // This ends at:
    // - the location of the variadic *args parameter, if present; otherwise
    // - the location of the first keyword-only parameter, if any; otherwise
    // - the location of the variadic **kwargs parameter, if present; otherwise
    // - the end of the parameter list
    // It's correct to give *args priority over kw_only because we verified
    // above that kw_only comes afterward if both are present. It's correct
    // to give kw_only priority over **kwargs because we verified above that
    // kw_only comes before if both are present.
    f.nargs_pos =   args_pos_1 < nargs ? args_pos_1 :
                      explicit_kw_only ? nargs_before_kw_only :
                  kwargs_pos_1 < nargs ? kwargs_pos_1 : nargs;

    // Store information (flags, defaults) about annotated parameters. Skips
    // getters. When 'has_arg_defaults' is set (e.g. due to the presence of
    // ``std::optional<T>``), we provide an implicit annotation for every
    // parameter, which is initialized by the loop below.
    if constexpr (has_arg_annotations && nrec > 0) {
        constexpr auto arg_flags =
            arg_flags_static<is_method_det, has_arg_annotations, Extra...>(
                (Return (*)(Args...)) nullptr);
        for (size_t i = 0; i < nrec; ++i) {
            arg_data_init &a = f.args[i];
            a.name = nullptr;
            a.signature = nullptr;
            a.value = nullptr;
            a.flag = (uint32_t) (arg_flags.v[i + is_method_det] &
                                 cast_flags::accepts_none);
        }
    }

    // Fill remaining fields of 'f'
    size_t arg_index = 0;
    (func_extra_apply(f, extra, arg_index), ...);

    (void) arg_index;

    return NB_CALL(nb_func_new)(NB_CTX, &f);
}

NAMESPACE_END(detail)

// The initial template parameter to cpp_function/cpp_function_def is
// used by class_ to ensure that member pointers are treated as members
// of the class being defined; other users can safely leave it at its
// default of void.

template <typename = void, typename Return, typename... Args, typename... Extra>
NB_INLINE object cpp_function(Return (*f)(Args...), const Extra&... extra) {
    return steal(detail::func_create<true, true>(
        f, f, std::make_index_sequence<sizeof...(Args)>(), extra...));
}

template <typename = void, typename Return, typename... Args, typename... Extra>
NB_INLINE void cpp_function_def(Return (*f)(Args...), const Extra&... extra) {
    detail::func_create<false, true>(
        f, f, std::make_index_sequence<sizeof...(Args)>(), extra...);
}

/// Construct a cpp_function from a lambda function (pot. with internal state)
template <
    typename = void, typename Func, typename... Extra,
    detail::enable_if_t<detail::is_lambda_v<std::remove_reference_t<Func>>> = 0>
NB_INLINE object cpp_function(Func &&f, const Extra &...extra) {
    using am = detail::analyze_method<decltype(&std::remove_reference_t<Func>::operator())>;
    return steal(detail::func_create<true, true>(
        (detail::forward_t<Func>) f, (typename am::func *) nullptr,
        std::make_index_sequence<am::argc>(), extra...));
}

template <
    typename = void, typename Func, typename... Extra,
    detail::enable_if_t<detail::is_lambda_v<std::remove_reference_t<Func>>> = 0>
NB_INLINE void cpp_function_def(Func &&f, const Extra &...extra) {
    using am = detail::analyze_method<decltype(&std::remove_reference_t<Func>::operator())>;
    detail::func_create<false, true>(
        (detail::forward_t<Func>) f, (typename am::func *) nullptr,
        std::make_index_sequence<am::argc>(), extra...);
}

/// Construct a cpp_function from a class method (non-const)
template <typename Target = void,
          typename Return, typename Class, typename... Args, typename... Extra>
NB_INLINE object cpp_function(Return (Class::*f)(Args...), const Extra &...extra) {
    using T = std::conditional_t<std::is_void_v<Target>, Class, Target>;
    return steal(detail::func_create<true, true>(
        [f](T *c, Args... args) NB_INLINE_LAMBDA -> Return {
            return (c->*f)((detail::forward_t<Args>) args...);
        },
        (Return(*)(T *, Args...)) nullptr,
        std::make_index_sequence<sizeof...(Args) + 1>(), extra...));
}

template <typename Target = void,
          typename Return, typename Class, typename... Args, typename... Extra>
NB_INLINE void cpp_function_def(Return (Class::*f)(Args...), const Extra &...extra) {
    using T = std::conditional_t<std::is_void_v<Target>, Class, Target>;
    detail::func_create<false, true>(
        [f](T *c, Args... args) NB_INLINE_LAMBDA -> Return {
            return (c->*f)((detail::forward_t<Args>) args...);
        },
        (Return(*)(T *, Args...)) nullptr,
        std::make_index_sequence<sizeof...(Args) + 1>(), extra...);
}

/// Construct a cpp_function from a class method (const)
template <typename Target = void,
          typename Return, typename Class, typename... Args, typename... Extra>
NB_INLINE object cpp_function(Return (Class::*f)(Args...) const, const Extra &...extra) {
    using T = std::conditional_t<std::is_void_v<Target>, Class, Target>;
    return steal(detail::func_create<true, true>(
        [f](const T *c, Args... args) NB_INLINE_LAMBDA -> Return {
            return (c->*f)((detail::forward_t<Args>) args...);
        },
        (Return(*)(const T *, Args...)) nullptr,
        std::make_index_sequence<sizeof...(Args) + 1>(), extra...));
}

template <typename Target = void,
          typename Return, typename Class, typename... Args, typename... Extra>
NB_INLINE void cpp_function_def(Return (Class::*f)(Args...) const, const Extra &...extra) {
    using T = std::conditional_t<std::is_void_v<Target>, Class, Target>;
    detail::func_create<false, true>(
        [f](const T *c, Args... args) NB_INLINE_LAMBDA -> Return {
            return (c->*f)((detail::forward_t<Args>) args...);
        },
        (Return(*)(const T *, Args...)) nullptr,
        std::make_index_sequence<sizeof...(Args) + 1>(), extra...);
}

template <typename Func, typename... Extra>
module_ &module_::def(const char *name_, Func &&f, const Extra &...extra) {
    cpp_function_def((detail::forward_t<Func>) f, scope(*this),
                     name(name_), extra...);
    return *this;
}

NAMESPACE_END(NB_NAMESPACE)
