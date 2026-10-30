/*
    nanobind/nb_class.h: Functionality for binding C++ classes/structs

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

NB_INLINE void type_extra_apply(type_data_init &t, const handle &h) {
    t.flags |= (uint32_t) type_init_flags::has_base_py;
    t.base_py = (PyTypeObject *) h.ptr();
}

NB_INLINE void type_extra_apply(type_data_init &t, const char *doc) {
    t.flags |= (uint32_t) type_init_flags::has_doc;
    t.doc = doc;
}

NB_INLINE void type_extra_apply(type_data_init &t, type_slots c) {
    t.flags |= (uint32_t) type_init_flags::has_type_slots;
    t.type_slots = c.value;
}

template <typename T>
NB_INLINE void type_extra_apply(type_data_init &t, intrusive_ptr<T> ip) {
    t.flags |= (uint32_t) type_flags::intrusive_ptr;
    t.set_self_py = (void (*)(void *, PyObject *) noexcept) ip.set_self_py;
}

NB_INLINE void type_extra_apply(type_data_init &t, is_final) {
    t.flags |= (uint32_t) type_flags::is_final;
}

NB_INLINE void type_extra_apply(type_data_init &t, dynamic_attr) {
    t.flags |= (uint32_t) type_flags::has_dynamic_attr;
}

NB_INLINE void type_extra_apply(type_data_init & t, is_weak_referenceable) {
    t.flags |= (uint32_t) type_flags::is_weak_referenceable;
}

NB_INLINE void type_extra_apply(type_data_init & t, is_generic) {
    t.flags |= (uint32_t) type_flags::is_generic;
}

NB_INLINE void type_extra_apply(type_data_init & t, const sig &s) {
    t.flags |= (uint32_t) type_flags::has_signature;
    t.name = s.value;
}

NB_INLINE void type_extra_apply(type_data_init &, never_destruct) {
    // intentionally empty
}

NB_INLINE void type_extra_apply(type_data_init &t, pooled p) {
    t.flags |= (uint32_t) type_flags::pooled;
    t.pool_capacity = p.capacity;
}

template <typename T>
NB_INLINE void type_extra_apply(type_data_init &t, supplement<T>) {
    static_assert(std::is_trivially_default_constructible_v<T>,
                  "The supplement must be a POD (plain old data) type");
    static_assert(alignof(T) <= alignof(void *),
                  "The alignment requirement of the supplement is too high.");
    t.flags |= (uint32_t) type_init_flags::has_supplement | (uint32_t) type_flags::is_final;
    t.supplement_size = (uint32_t) sizeof(T);
}

NB_INLINE void enum_extra_apply(enum_data_init &e, is_arithmetic) {
    e.flags |= (uint32_t) enum_flags::is_arithmetic;
}

NB_INLINE void enum_extra_apply(enum_data_init &e, is_flag) {
    e.flags |= (uint32_t) enum_flags::is_flag;
}

NB_INLINE void enum_extra_apply(enum_data_init &e, is_str) {
    e.flags |= (uint32_t) enum_flags::is_str;
}

NB_INLINE void enum_extra_apply(enum_data_init &e, const char *doc) {
    e.docstr = doc;
}

template <typename T>
NB_INLINE void enum_extra_apply(enum_data_init &, T) {
    static_assert(
        std::is_void_v<T>,
        "Invalid enum binding annotation. The implementation of "
        "enums changed nanobind 2.0.0: only nb::is_arithmetic and "
        "docstrings can be passed since this change.");
}

template <typename T> void wrap_copy(void *dst, const void *src) {
    new ((T *) dst) T(*(const T *) src);
}

template <typename T> void wrap_move(void *dst, void *src) noexcept {
    new ((T *) dst) T(std::move(*(T *) src));
}

template <typename T> void wrap_destruct(void *value) noexcept {
    ((T *) value)->~T();
}

template <typename, template <typename, typename> typename, typename...>
struct extract;

template <typename T, template <typename, typename> typename Pred>
struct extract<T, Pred> {
    using type = T;
};

template <typename T, template <typename, typename> typename Pred,
          typename Tv, typename... Ts>
struct extract<T, Pred, Tv, Ts...> {
    using type = std::conditional_t<
        Pred<T, Tv>::value,
        Tv,
        typename extract<T, Pred, Ts...>::type
    >;
};

template <typename T, typename Arg> using is_alias = std::is_base_of<T, Arg>;
template <typename T, typename Arg> using is_base = std::is_base_of<Arg, T>;

enum op_id : int;
enum op_type : int;
struct undefined_t;
template <op_id id, op_type ot, typename L = undefined_t, typename R = undefined_t> struct op_;

// The header file include/nanobind/stl/detail/traits.h extends this type trait
template <typename T, typename SFINAE = int>
struct is_copy_constructible : std::is_copy_constructible<T> { };

template <typename T>
constexpr bool is_copy_constructible_v = is_copy_constructible<T>::value;

NAMESPACE_END(detail)

// Low level access to nanobind type objects
inline bool type_check(handle h) { return NB_CALL(nb_type_check)(NB_CTX, h.ptr()); }
inline size_t type_size(handle h) { return NB_CALL(nb_type_size)(h.ptr()); }
inline size_t type_align(handle h) { return NB_CALL(nb_type_align)(h.ptr()); }
inline const std::type_info& type_info(handle h) { return *NB_CALL(nb_type_info)(h.ptr()); }
template <typename T>
inline T &type_supplement(handle h) { return *(T *) NB_CALL(nb_type_supplement)(h.ptr()); }
inline str type_name(handle h) { return steal<str>(NB_CALL(nb_type_name)(h.ptr())); }

// Low level access to arbitrary Python type objects and instances
inline dict type_dict(handle h) noexcept {
    return steal<dict>(NB_CALL(type_dict)(h.ptr()));
}

inline dict inst_dict(handle h) noexcept {
    return steal<dict>(NB_CALL(inst_dict)(h.ptr()));
}

inline object type_lookup(handle h, detail::str_key key) noexcept {
    return steal(NB_CALL(type_lookup_str)(NB_CTX, h.ptr(), key.str, key.bound));
}

inline object type_lookup(handle h, handle key) noexcept {
    return steal(NB_CALL(type_lookup)(NB_CTX, h.ptr(), key.ptr()));
}

inline bool type_freeze(handle h) { return NB_CALL(type_freeze)(h.ptr()); }

// Low level access to nanobind instance objects
inline bool inst_check(handle h) { return type_check(h.type()); }
inline str inst_name(handle h) {
    return steal<str>(NB_CALL(nb_inst_name)(h.ptr()));
}
inline bool inst_python_derived(handle h) {
    return NB_CALL(nb_inst_python_derived)(h.ptr());
}
inline object inst_alloc(handle h) {
    return steal(NB_CALL(nb_inst_alloc)((PyTypeObject *) h.ptr()));
}
inline object inst_alloc_zero(handle h) {
    return steal(NB_CALL(nb_inst_alloc_zero)((PyTypeObject *) h.ptr()));
}
inline object inst_take_ownership(handle h, void *p) {
    return steal(NB_CALL(nb_inst_take_ownership)((PyTypeObject *) h.ptr(), p));
}
inline object inst_reference(handle h, void *p, handle parent = handle()) {
    return steal(NB_CALL(nb_inst_reference)((PyTypeObject *) h.ptr(), p, parent.ptr()));
}
inline void inst_zero(handle h) { NB_CALL(nb_inst_zero)(h.ptr()); }
inline void inst_set_state(handle h, bool ready, bool destruct) {
    NB_CALL(nb_inst_state_write)(h.ptr(), (uint32_t) ready | ((uint32_t) destruct << 1));
}
inline std::pair<bool, bool> inst_state(handle h) {
    uint32_t state = NB_CALL(nb_inst_state_read)(h.ptr());
    return { (state & 1) != 0, (state & 2) != 0 };
}
inline void inst_mark_ready(handle h) { inst_set_state(h, true, true); }
inline bool inst_ready(handle h) { return inst_state(h).first; }
inline void inst_destruct(handle h) { NB_CALL(nb_inst_destruct)(h.ptr()); }
inline void inst_copy(handle dst, handle src) { NB_CALL(nb_inst_copy)(dst.ptr(), src.ptr()); }
inline void inst_move(handle dst, handle src) { NB_CALL(nb_inst_move)(dst.ptr(), src.ptr()); }
inline void inst_replace_copy(handle dst, handle src) { NB_CALL(nb_inst_copy)(dst.ptr(), src.ptr()); }
inline void inst_replace_move(handle dst, handle src) { NB_CALL(nb_inst_move)(dst.ptr(), src.ptr()); }
template <typename T> T *inst_ptr(handle h) { return (T *) NB_CALL(nb_inst_ptr)(h.ptr()); }

// Lifetime dependencies between objects (the mechanism behind nb::keep_alive)
inline void keep_alive_obj(handle nurse, handle patient) {
    NB_CALL(keep_alive_py)(NB_CTX, nurse.ptr(), patient.ptr());
}
inline void keep_alive_cb(handle nurse, void *payload,
                          void (*deleter)(void *) noexcept) noexcept {
    NB_CALL(keep_alive_ptr)(NB_CTX, nurse.ptr(), payload, deleter);
}

#if NB_TYPE_GET_SLOT_IMPL
NAMESPACE_BEGIN(detail)
// PyPy-only replacement for PyType_GetSlot(); not part of the dispatch
// table, since split mode requires CPython
NB_CORE void *type_get_slot(PyTypeObject *t, int slot_id);
NAMESPACE_END(detail)
#endif

inline void *type_get_slot(handle h, int slot_id) {
#if NB_TYPE_GET_SLOT_IMPL
    return detail::type_get_slot((PyTypeObject *) h.ptr(), slot_id);
#else
    return PyType_GetSlot((PyTypeObject *) h.ptr(), slot_id);
#endif
}

template <typename Visitor> struct def_visitor {
    // Ensure def_visitor<T> can only be derived from, not constructed
    // directly
    ~def_visitor() {
        static_assert(std::is_base_of_v<def_visitor, Visitor>,
                      "def_visitor uses CRTP: def_visitor<T> should be "
                      "a base of T");
    }
};

template <typename... Args> struct init : def_visitor<init<Args...>> {
    template <typename T, typename... Ts> friend class class_;
    NB_INLINE init() {}

private:
    template <typename Class, typename... Extra>
    NB_INLINE static void execute(Class &cl, const Extra&... extra) {
        using Type = typename Class::Type;
        using Alias = typename Class::Alias;
        cl.def(
            "__init__",
            [](pointer_and_handle<Type> v, Args... args) {
                if constexpr (!std::is_same_v<Type, Alias> &&
                              std::is_constructible_v<Type, Args...>) {
                    if (!inst_python_derived(v.h)) {
                        new (v.p) Type((detail::forward_t<Args>) args...);
                        return;
                    }
                }
                // Prefer direct- over list-initialization
                if constexpr (std::is_constructible_v<Alias, Args...>)
                    new ((void *) v.p) Alias((detail::forward_t<Args>) args...);
                else
                    new ((void *) v.p) Alias{(detail::forward_t<Args>) args...};
            },
            extra...);
    }
};

template <typename Arg> struct init_implicit : def_visitor<init_implicit<Arg>> {
    template <typename T, typename... Ts> friend class class_;
    NB_INLINE init_implicit() { }

private:
    template <typename Class, typename... Extra>
    NB_INLINE static void execute(Class &cl, const Extra&... extra) {
        using Type = typename Class::Type;
        using Alias = typename Class::Alias;

        cl.def(
            "__init__",
            [](pointer_and_handle<Type> v, Arg arg) {
                if constexpr (!std::is_same_v<Type, Alias> &&
                              std::is_constructible_v<Type, Arg>) {
                    if (!inst_python_derived(v.h)) {
                        new ((Type *) v.p) Type((detail::forward_t<Arg>) arg);
                        return;
                    }
                }
                // Prefer direct- over list-initialization
                if constexpr (std::is_constructible_v<Alias, Arg>)
                    new ((Alias *) v.p) Alias((detail::forward_t<Arg>) arg);
                else
                    new ((Alias *) v.p) Alias{ (detail::forward_t<Arg>) arg };
            }, is_implicit(), extra...);

        using Caster = detail::make_caster<Arg>;

        if constexpr (!detail::is_class_caster_v<Caster>) {
            bool (*pred)(PyTypeObject *, PyObject *,
                         detail::cleanup_list *) noexcept =
                [](PyTypeObject *, PyObject *src,
                   detail::cleanup_list *cleanup) noexcept -> bool {
                    return Caster().from_python(
                        src, detail::cast_flags::convert, cleanup);
                };
            NB_CALL(implicitly_convertible)(NB_CTX, &typeid(Type), (void *) pred, true);
        }
    }
};

namespace detail {
    // This is 'inline' so we can define it in a header and not pay
    // for it if unused, and also 'noinline' so we don't generate
    // multiple copies and produce code bloat.
    NB_NOINLINE inline void wrap_base_new(handle cls, bool do_wrap) {
        if (PyCFunction_Check(cls.attr("__new__").ptr())) {
            if (do_wrap) {
                cpp_function_def(
                    [](handle type) {
                        if (!type_check(type))
                            throw cast_error();
                        return inst_alloc(type);
                    },
                    scope(cls), name("__new__"));
            }
        } else {
            if (!do_wrap) {
                // We already defined the wrapper, so this zero-arg overload
                // would be unreachable. Raise an error rather than hiding it.
                raise("nanobind: %s must define its zero-argument __new__ "
                      "before any other overloads", type_name(cls).c_str());
            }
        }
    }

    // Call policy that ensures __new__ returns an instance of the correct
    // Python type, even when deriving from the C++ class in Python
    struct new_returntype_fixup_policy {
        static inline void precall(PyObject **, size_t,
                                   detail::cleanup_list *) {}
        NB_NOINLINE static inline void postcall(PyObject **args, size_t,
                                                PyObject *&ret) {
            handle type_requested = args[0];
            if (ret == nullptr || !type_requested.is_type())
                return; // somethign strange about this call; don't meddle
            handle type_created = Py_TYPE(ret);
            if (type_created.is(type_requested))
                return; // already created the requested type so no fixup needed

            if (type_check(type_created) &&
                PyType_IsSubtype((PyTypeObject *) type_requested.ptr(),
                                 (PyTypeObject *) type_created.ptr()) &&
                type_info(type_created) == type_info(type_requested)) {
                // The new_ constructor returned an instance of a bound type T.
                // The user wanted an instance of some python subclass S of T.
                // Since both wrap the same C++ type, we can satisfy the request
                // by returning a pyobject of type S that wraps a C++ T*, and
                // handling the lifetimes by having that pyobject keep the
                // already-created T pyobject alive.
                object wrapper = inst_reference(type_requested,
                                                inst_ptr<void>(ret),
                                                /* parent = */ ret);
                handle(ret).dec_ref();
                ret = wrapper.release().ptr();
            }
        }
    };
}

template <typename Func, typename Sig = detail::function_signature_t<Func>>
struct new_;

template <typename Func, typename Return, typename... Args>
struct new_<Func, Return(Args...)> : def_visitor<new_<Func, Return(Args...)>> {
    std::remove_reference_t<Func> func;

    new_(Func &&f) : func((detail::forward_t<Func>) f) {}

    template <typename Class, typename... Extra>
    NB_INLINE void execute(Class &cl, const Extra&... extra) {
        // If this is the first __new__ overload we're defining, then wrap
        // nanobind's built-in __new__ so we overload with it instead of
        // replacing it; this is important for pickle support.
        // We can't do this if the user-provided __new__ takes no
        // arguments, because it would make an ambiguous overload set.
        constexpr size_t num_defaults =
            (detail::arg_traits<Extra>::has_default + ... + 0);
        constexpr size_t num_varargs =
            ((std::is_same_v<detail::intrinsic_t<Args>, args> ||
              std::is_same_v<detail::intrinsic_t<Args>, kwargs>) + ... + 0);
        detail::wrap_base_new(cl, sizeof...(Args) > num_defaults + num_varargs);

        auto wrapper = [func_ = (detail::forward_t<Func>) func](handle, Args... args) {
            return func_((detail::forward_t<Args>) args...);
        };

        auto policy = call_policy<detail::new_returntype_fixup_policy>();
        if constexpr ((detail::arg_traits<Extra>::is_arg || ...)) {
            // If any argument annotations are specified, add another for the
            // extra class argument that we don't forward to Func, so visible
            // arg() annotations stay aligned with visible function arguments.
            cl.def_static("__new__", std::move(wrapper), arg("cls"), extra...,
                          policy);
        } else {
            cl.def_static("__new__", std::move(wrapper), extra..., policy);
        }
        cl.def("__init__", [](handle, Args...) {}, extra...);
    }
};
template <typename Func> new_(Func&& f) -> new_<Func>;

template <typename T> struct for_setter {
    T value;
    for_setter(const T &value) : value(value) { }
};

template <typename T> struct for_getter {
    T value;
    for_getter(const T &value) : value(value) { }
};

template <typename T> for_getter(T) -> for_getter<std::decay_t<T>>;
template <typename T> for_setter(T) -> for_setter<std::decay_t<T>>;

namespace detail {
    template <typename T> auto filter_getter(const T &v) { return v; }
    template <typename T> auto filter_getter(const for_getter<T> &v) { return v.value; }
    template <typename T> std::nullptr_t filter_getter(const for_setter<T> &) { return nullptr; }

    template <typename T> auto filter_setter(const T &v) { return v; }
    template <typename T> auto filter_setter(const for_setter<T> &v) { return v.value; }
    template <typename T> std::nullptr_t filter_setter(const for_getter<T> &) { return nullptr; }
}

template <typename T, typename... Ts>
class class_ : public object {
public:
    NB_OBJECT_DEFAULT(class_, object, "type", detail::py_type_check)
    using Type = T;
    using Base  = typename detail::extract<T, detail::is_base,  Ts...>::type;
    using Alias = typename detail::extract<T, detail::is_alias, Ts...>::type;

    static_assert(sizeof(Alias) < (((uint64_t) 1) << 32), "Instance size is too big!");
    static_assert(
        sizeof...(Ts) == !std::is_same_v<Base, T> + !std::is_same_v<Alias, T>,
        "nanobind::class_<> was invoked with extra arguments that could not be handled");

    static_assert(
        detail::is_base_caster_v<detail::make_caster<Type>>,
        "You attempted to bind a type that is already intercepted by a type "
        "caster. Having both at the same time is not allowed. Are you perhaps "
        "binding an STL type, while at the same time including a matching "
        "type caster from <nanobind/stl/*>? Or did you perhaps forget to "
        "declare NB_MAKE_OPAQUE(..) to specifically disable the type caster "
        "catch-all for a specific type? Please review the documentation "
        "to learn about the difference between bindings and type casters.");

    template <typename... Extra>
    NB_INLINE class_(handle scope, const char *name, const Extra &... extra) {
        detail::type_data_init d;

        static_assert(sizeof(Alias) / alignof(Alias) < ((size_t) 1 << 27),
                      "nanobind: instance size is too large");
        d.size_align = detail::type_size_align(sizeof(Alias), alignof(Alias));
        d.flags = NB_ABI_MINOR_TAG;
        d.name = name;
        d.scope = scope.ptr();
        d.type = &typeid(T);

        if constexpr (!std::is_same_v<Base, T>) {
            d.base = &typeid(Base);
            d.flags |= (uint32_t) detail::type_init_flags::has_base;
        }

        if constexpr (detail::is_copy_constructible_v<T>) {
            d.flags |= (uint32_t) detail::type_flags::is_copy_constructible;

            if constexpr (!std::is_trivially_copy_constructible_v<T>) {
                d.flags |= (uint32_t) detail::type_flags::has_copy;
                d.copy = detail::wrap_copy<T>;
            }
        }

        if constexpr (std::is_move_constructible<T>::value) {
            d.flags |= (uint32_t) detail::type_flags::is_move_constructible;

            if constexpr (!std::is_trivially_move_constructible_v<T>) {
                d.flags |= (uint32_t) detail::type_flags::has_move;
                d.move = detail::wrap_move<T>;
            }
        }

        constexpr bool has_never_destruct = (std::is_same_v<Extra, never_destruct> || ...);

        if constexpr (std::is_destructible_v<T> && !has_never_destruct) {
            d.flags |= (uint32_t) detail::type_flags::is_destructible;

            if constexpr (!std::is_trivially_destructible_v<T>) {
                d.flags |= (uint32_t) detail::type_flags::has_destruct;
                d.destruct = detail::wrap_destruct<T>;
            }
        }

        if constexpr (detail::has_shared_from_this_v<T>) {
            d.flags |= (uint32_t) detail::type_flags::has_shared_from_this;
            d.keep_shared_from_this_alive = [](PyObject *self) noexcept {
                // weak_from_this().lock() is equivalent to shared_from_this(),
                // except that it returns an empty shared_ptr instead of
                // throwing an exception if there is no active shared_ptr
                // for this object. (Added in C++17.)
                if (auto sp = inst_ptr<T>(self)->weak_from_this().lock()) {
                    NB_CALL(keep_alive_ptr)(NB_CTX, self, new auto(std::move(sp)),
                                            [](void *p) noexcept {
                                                delete (decltype(sp) *) p;
                                            });
                    return true;
                }
                return false;
            };
        }

        (detail::type_extra_apply(d, extra), ...);

        m_ptr = NB_CALL(nb_type_new)(NB_CTX, &d);
    }

    template <typename Func, typename... Extra>
    NB_INLINE class_ &def(const char *name_, Func &&f, const Extra &... extra) {
        cpp_function_def<T>((detail::forward_t<Func>) f, scope(*this),
                            name(name_), is_method(), extra...);
        return *this;
    }

    template <typename Visitor, typename... Extra>
    NB_INLINE class_ &def(def_visitor<Visitor> &&arg, const Extra &... extra) {
        static_cast<Visitor&&>(arg).execute(*this, extra...);
        return *this;
    }

    template <typename Func, typename... Extra>
    NB_INLINE class_ &def_static(const char *name_, Func &&f,
                                 const Extra &... extra) {
        static_assert(
            !std::is_member_function_pointer_v<Func>,
            "def_static(...) called with a non-static member function pointer");
        cpp_function_def((detail::forward_t<Func>) f, scope(*this), name(name_),
                         extra...);
        return *this;
    }

    template <typename Getter, typename Setter, typename... Extra>
    NB_INLINE class_ &def_prop_rw(const char *name_, Getter &&getter,
                                  Setter &&setter, const Extra &...extra) {
        object get_p, set_p;

        if constexpr (!std::is_same_v<Getter, std::nullptr_t>)
            get_p = cpp_function<T>((detail::forward_t<Getter>) getter,
                                    is_method(), is_getter(),
                                    rv_policy::reference_internal,
                                    detail::filter_getter(extra)...);

        if constexpr (!std::is_same_v<Setter, std::nullptr_t>)
            set_p = cpp_function<T>((detail::forward_t<Setter>) setter,
                                    is_method(), detail::filter_setter(extra)...);

        NB_CALL(property_install)(NB_CTX, m_ptr, name_, get_p.ptr(), set_p.ptr(), false);
        return *this;
    }

    template <typename Getter, typename Setter, typename... Extra>
    NB_INLINE class_ &def_prop_rw_static(const char *name_, Getter &&getter,
                                         Setter &&setter,
                                         const Extra &...extra) {
        object get_p, set_p;

        if constexpr (!std::is_same_v<Getter, std::nullptr_t>)
            get_p = cpp_function((detail::forward_t<Getter>) getter, is_getter(),
                                 rv_policy::reference,
                                 detail::filter_getter(extra)...);

        if constexpr (!std::is_same_v<Setter, std::nullptr_t>)
            set_p = cpp_function((detail::forward_t<Setter>) setter,
                                 detail::filter_setter(extra)...);

        NB_CALL(property_install)(NB_CTX, m_ptr, name_, get_p.ptr(), set_p.ptr(), true);
        return *this;
    }

    template <typename Getter, typename... Extra>
    NB_INLINE class_ &def_prop_ro(const char *name_, Getter &&getter,
                                  const Extra &...extra) {
        return def_prop_rw(name_, getter, nullptr, extra...);
    }

    template <typename Getter, typename... Extra>
    NB_INLINE class_ &def_prop_ro_static(const char *name_,
                                         Getter &&getter,
                                         const Extra &...extra) {
        return def_prop_rw_static(name_, getter, nullptr, extra...);
    }

    template <typename C, typename D, typename... Extra>
    NB_INLINE class_ &def_rw(const char *name, D C::*p,
                             const Extra &...extra) {
        // Unions never satisfy is_base_of, thus the is_same alternative
        static_assert(std::is_base_of_v<C, T> || std::is_same_v<C, T>,
                      "def_rw() requires a (base) class member!");

        using Q =
            std::conditional_t<detail::is_base_caster_v<detail::make_caster<D>>,
                               const D &, D &&>;

        def_prop_rw(name,
            [p](const T &c) -> const D & { return c.*p; },
            [p](T &c, Q value) { c.*p = (Q) value; },
            extra...);

        return *this;
    }

    template <typename D, typename... Extra>
    NB_INLINE class_ &def_rw_static(const char *name, D *p,
                                    const Extra &...extra) {
        using Q =
            std::conditional_t<detail::is_base_caster_v<detail::make_caster<D>>,
                               const D &, D &&>;

        def_prop_rw_static(name,
            [p](handle) -> const D & { return *p; },
            [p](handle, Q value) { *p = (Q) value; }, extra...);

        return *this;
    }

    template <typename C, typename D, typename... Extra>
    NB_INLINE class_ &def_ro(const char *name, D C::*p,
                             const Extra &...extra) {
        // Unions never satisfy is_base_of, thus the is_same alternative
        static_assert(std::is_base_of_v<C, T> || std::is_same_v<C, T>,
                      "def_ro() requires a (base) class member!");

        def_prop_ro(name,
            [p](const T &c) -> const D & { return c.*p; }, extra...);

        return *this;
    }

    template <typename D, typename... Extra>
    NB_INLINE class_ &def_ro_static(const char *name, D *p,
                                    const Extra &...extra) {
        def_prop_ro_static(name,
            [p](handle) -> const D & { return *p; }, extra...);

        return *this;
    }

    template <detail::op_id id, detail::op_type ot, typename L, typename R, typename... Extra>
    class_ &def(const detail::op_<id, ot, L, R> &op, const Extra&... extra) {
        op.execute(*this, extra...);
        return *this;
    }

    template <detail::op_id id, detail::op_type ot, typename L, typename R, typename... Extra>
    class_ & def_cast(const detail::op_<id, ot, L, R> &op, const Extra&... extra) {
        op.execute_cast(*this, extra...);
        return *this;
    }

    /// Make the type immutable. Must be the last step of a binding chain
    NB_INLINE class_ &freeze() {
        NB_CALL(type_freeze)(m_ptr);
        return *this;
    }
};

template <typename T> class enum_ : public object {
public:
    static_assert(std::is_enum_v<T>, "nanobind::enum_<> requires an enumeration type!");

    using Base = class_<T>;
    using Underlying = std::underlying_type_t<T>;

    template <typename... Extra>
    NB_INLINE enum_(handle scope, const char *name, const Extra &... extra) {
        detail::enum_data_init ed;
        ed.type = &typeid(T);
        ed.scope = scope.ptr();
        ed.name = name;
        ed.docstr = nullptr;
        ed.flags = NB_ABI_MINOR_TAG | (std::is_signed_v<Underlying>
                       ? (uint32_t) detail::enum_flags::is_signed
                       : 0);
        (detail::enum_extra_apply(ed, extra), ...);
        m_ptr = NB_CALL(enum_create)(NB_CTX, &ed);
    }

    NB_INLINE enum_ &value(const char *name, T value, const char *doc = nullptr) {
        NB_CALL(enum_append)(m_ptr, name, (int64_t) value, nullptr, doc);
        return *this;
    }

    NB_INLINE enum_ &str_value(const char *name, T value, const char *str_val,
                               const char *doc = nullptr) {
        NB_CALL(enum_append)(m_ptr, name, (int64_t) value, str_val, doc);
        return *this;
    }

    NB_INLINE enum_ &export_values() { NB_CALL(enum_export)(m_ptr); return *this; }

    template <typename Func, typename... Extra>
    NB_INLINE enum_ &def(const char *name_, Func &&f, const Extra &... extra) {
        cpp_function_def<T>((detail::forward_t<Func>) f, scope(*this),
                            name(name_), is_method(), extra...);
        return *this;
    }

    template <typename Func, typename... Extra>
    NB_INLINE enum_ &def_static(const char *name_, Func &&f,
                                 const Extra &... extra) {
        static_assert(
            !std::is_member_function_pointer_v<Func>,
            "def_static(...) called with a non-static member function pointer");
        cpp_function_def((detail::forward_t<Func>) f, scope(*this), name(name_),
                         extra...);
        return *this;
    }

    template <typename Getter, typename Setter, typename... Extra>
    NB_INLINE enum_ &def_prop_rw(const char *name_, Getter &&getter,
                                 Setter &&setter, const Extra &...extra) {
        object get_p, set_p;

        if constexpr (!std::is_same_v<Getter, std::nullptr_t>)
            get_p = cpp_function<T>((detail::forward_t<Getter>) getter,
                                    is_method(), is_getter(),
                                    rv_policy::reference_internal,
                                    detail::filter_getter(extra)...);

        if constexpr (!std::is_same_v<Setter, std::nullptr_t>)
            set_p = cpp_function<T>((detail::forward_t<Setter>) setter,
                                    is_method(), detail::filter_setter(extra)...);

        NB_CALL(property_install)(NB_CTX, m_ptr, name_, get_p.ptr(), set_p.ptr(), false);
        return *this;
    }


    template <typename Getter, typename... Extra>
    NB_INLINE enum_ &def_prop_ro(const char *name_, Getter &&getter,
                                 const Extra &...extra) {
        return def_prop_rw(name_, getter, nullptr, extra...);
    }
};

template <typename Source, typename Target> void implicitly_convertible() {
    if constexpr (!std::is_same_v<Source, Target>) {
        using Caster = detail::make_caster<Source>;
        static_assert(
            !std::is_enum_v<Target> ||
                detail::is_base_caster_v<detail::make_caster<Target>>,
            "implicitly_convertible(): 'Target' cannot be an enumeration "
            "unless it is opaque.");

        if constexpr (detail::is_base_caster_v<Caster>) {
            NB_CALL(implicitly_convertible)(NB_CTX, &typeid(Target),
                                            (void *) &typeid(Source), false);
        } else {
            bool (*pred)(PyTypeObject *, PyObject *,
                         detail::cleanup_list *) noexcept =
                [](PyTypeObject *, PyObject *src,
                   detail::cleanup_list *cleanup) noexcept -> bool {
                    return Caster().from_python(src, detail::cast_flags::convert,
                                                cleanup);
                };
            NB_CALL(implicitly_convertible)(NB_CTX, &typeid(Target), (void *) pred,
                                            true);
        }
    }
}

NAMESPACE_END(NB_NAMESPACE)
