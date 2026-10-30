/*
    nanobind/stl/variant.h: type caster for std::variant<...>

    Copyright (c) 2022 Yoshiki Matsuda and Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

#pragma once

#include <nanobind/nanobind.h>
#include <variant>

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

template <typename T, typename...>
struct concat_variant { using type = T; };
template <typename... Ts1, typename... Ts2, typename... Ts3>
struct concat_variant<std::variant<Ts1...>, std::variant<Ts2...>, Ts3...>
    : concat_variant<std::variant<Ts1..., Ts2...>, Ts3...> {};

template <typename... Ts> struct remove_opt_mono<std::variant<Ts...>>
    : concat_variant<std::conditional_t<std::is_same_v<std::monostate, Ts>, std::variant<>, std::variant<remove_opt_mono_t<Ts>>>...> {};

template <> struct type_caster<std::monostate> : none_caster<std::monostate> { };
template <> struct has_arg_defaults<std::monostate> : std::true_type { };

template <bool Defaultable, typename... Ts>
struct variant_caster_storage;

template <typename... Ts>
struct variant_caster_storage<true, Ts...> {
    using Variant = std::variant<Ts...>;
    Variant value;
    Variant& get() { return value; }
    template <typename T>
    void store(T&& alternative) { value = (forward_t<T>) alternative; }
};

template <typename... Ts>
struct variant_caster_storage<false, Ts...> {
    using Variant = std::variant<Ts...>;
    std::variant<std::monostate, Variant> value;
    Variant& get() { return *std::get_if<1>(&value); }
    template <typename T>
    void store(T&& alternative) {
        value.template emplace<1>((forward_t<T>) alternative);
    }
};

template <typename T1, typename... Ts>
constexpr bool variant_is_defaultable = std::is_default_constructible_v<T1>;

template <typename... Ts>
struct type_caster<std::variant<Ts...>>
  : private variant_caster_storage<variant_is_defaultable<Ts...>, Ts...> {
    // We don't use NB_TYPE_CASTER so that we can customize the cast operators
    // to use `variant_caster_storage`, in order to support variants that are
    // not default-constructible.
    using Value = std::variant<Ts...>;
    static constexpr auto Name = union_name(make_caster<Ts>::Name...);

    template <typename T> using Cast = movable_cast_t<T>;
    template <typename T> static constexpr bool can_cast() { return true; }
    explicit operator Value*() { return &this->get(); }
    explicit operator Value&() { return (Value &) this->get(); }
    explicit operator Value&&() { return (Value &&) this->get(); }

    template <typename T>
    bool try_variant(const handle &src, uint32_t flags, cleanup_list *cleanup) {
        using CasterT = make_caster<T>;

        CasterT caster;

        if (!caster.from_python(src, flags_for_local_caster<T>(flags), cleanup) ||
            !caster.template can_cast<T>())
            return false;

        this->store(caster.operator cast_t<T>());

        return true;
    }

    bool from_python(handle src, uint32_t flags, cleanup_list *cleanup) noexcept {
        if (flags & cast_flags::convert) {
            if ((try_variant<Ts>(src, flags & ~cast_flags::convert, cleanup) || ...))
                return true;
        }
        return (try_variant<Ts>(src, flags, cleanup) || ...);
    }

    template <typename T>
    static handle from_cpp(T *value, rv_policy policy,
                           cleanup_list *cleanup) noexcept {
        if (!value)
            return none().release();
        return from_cpp(*value, policy, cleanup);
    }

    template <typename T>
    static handle from_cpp(T &&value, rv_policy policy, cleanup_list *cleanup) noexcept {
        return std::visit(
            [&](auto &&v) {
                return make_caster<decltype(v)>::from_cpp(
                    std::forward<decltype(v)>(v), policy, cleanup);
            },
            std::forward<T>(value));
    }
};

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
