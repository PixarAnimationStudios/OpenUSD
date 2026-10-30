/*
    nanobind/nb_descr.h: Constexpr string class for function signatures

    Copyright (c) 2022 Wenzel Jakob

    All rights reserved. Use of this source code is governed by a
    BSD-style license that can be found in the LICENSE file.
*/

NAMESPACE_BEGIN(NB_NAMESPACE)
NAMESPACE_BEGIN(detail)

/// Helper type for concatenating type signatures at compile time
template <size_t N, typename... Ts>
struct descr {
    char text[N + 1]{'\0'};

    constexpr descr() = default;
    constexpr descr(char const (&s)[N+1]) : descr(s, std::make_index_sequence<N>()) { }

    template <size_t... Is>
    constexpr descr(char const (&s)[N+1], std::index_sequence<Is...>) : text{s[Is]..., '\0'} { }

    template <typename... Cs>
    constexpr descr(char c, Cs... cs) : text{c, static_cast<char>(cs)..., '\0'} { }

    constexpr size_t type_count() const { return sizeof...(Ts); }
    constexpr size_t size() const { return N; }

    NB_INLINE void put_types(const std::type_info **out) const {
        size_t ctr = 0;
        ((out[ctr++] = &typeid(Ts)), ...);
        out[ctr++] = nullptr;
    }
};

template <size_t N1, size_t N2, typename... Ts1, typename... Ts2, size_t... Is1, size_t... Is2>
constexpr descr<N1 + N2, Ts1..., Ts2...> plus_impl(const descr<N1, Ts1...> &a, const descr<N2, Ts2...> &b,
                                                   std::index_sequence<Is1...>, std::index_sequence<Is2...>) {
    return {a.text[Is1]..., b.text[Is2]...};
}

template <size_t N1, size_t N2, typename... Ts1, typename... Ts2>
constexpr descr<N1 + N2, Ts1..., Ts2...> operator+(const descr<N1, Ts1...> &a, const descr<N2, Ts2...> &b) {
    return plus_impl(a, b, std::make_index_sequence<N1>(), std::make_index_sequence<N2>());
}

template <size_t N>
constexpr descr<N - 1> const_name(char const(&text)[N]) { return descr<N - 1>(text); }
constexpr descr<0> const_name(char const(&)[1]) { return {}; }

template <size_t Rem, size_t... Digits>
struct int_to_str : int_to_str<Rem / 10, Rem % 10, Digits...> {};
template <size_t... Digits> struct int_to_str<0, Digits...> {
    static constexpr auto digits = descr<sizeof...(Digits)>(('0' + Digits)...);
};

constexpr auto const_name(char c) { return descr<1>(c); }

// Ternary description (like std::conditional)
template <bool B, size_t N1, size_t N2>
constexpr auto const_name(char const(&text1)[N1], char const(&text2)[N2]) {
    (void) text1; (void) text2;

    if constexpr(B)
        return const_name(text1);
    else
        return const_name(text2);
}

template <bool B, typename T1, typename T2>
constexpr auto const_name(const T1 &d1, const T2 &d2) {
    (void) d1; (void) d2;

    if constexpr (B)
        return d1;
    else
        return d2;
}

// Use a different name based on whether the parameter is used as input or output
template <size_t N1, size_t N2>
constexpr auto io_name(char const (&text1)[N1], char const (&text2)[N2]) {
    return const_name('@') + const_name(text1) + const_name('@') +
           const_name(text2) + const_name('@');
}

template <typename T> constexpr auto optional_name(const T &v) {
    return v + const_name(" | None");
}
template <typename T> constexpr auto union_name(const T &v) {
    return v;
}
template <typename T1, typename T2, typename... Ts>
constexpr auto union_name(const T1 &v1, const T2 &v2, const Ts &...vs) {
    return v1 + const_name(" | ") + union_name(v2, vs...);
}

template <size_t Size>
auto constexpr const_name() -> std::remove_cv_t<decltype(int_to_str<Size / 10, Size % 10>::digits)> {
    return int_to_str<Size / 10, Size % 10>::digits;
}

template <typename Type> constexpr descr<1, Type> const_name() { return {'%'}; }

constexpr descr<0> concat() { return {}; }
constexpr descr<0> concat_maybe() { return {}; }

template <size_t N, typename... Ts>
constexpr descr<N, Ts...> concat(const descr<N, Ts...> &descr) { return descr; }

template <size_t N, typename... Ts>
constexpr descr<N, Ts...> concat_maybe(const descr<N, Ts...> &descr) { return descr; }

template <size_t N, typename... Ts, typename... Args>
constexpr auto concat(const descr<N, Ts...> &d, const Args &...args)
    -> decltype(std::declval<descr<N + 2, Ts...>>() + concat(args...)) {
    return d + const_name(", ") + concat(args...);
}

template <typename... Args>
constexpr auto concat_maybe(const descr<0> &, const descr<0> &, const Args &...args)
    -> decltype(concat_maybe(args...)) { return concat_maybe(args...); }

template <size_t N, typename... Ts, typename... Args>
constexpr auto concat_maybe(const descr<0> &, const descr<N, Ts...> &arg, const Args &...args)
    -> decltype(concat_maybe(arg, args...)) { return concat_maybe(arg, args...); }

template <size_t N, typename... Ts, typename... Args>
constexpr auto concat_maybe(const descr<N, Ts...> &arg, const descr<0> &, const Args &...args)
    -> decltype(concat_maybe(arg, args...)) { return concat_maybe(arg, args...); }

template <size_t N, size_t N2, typename... Ts, typename... Ts2, typename... Args,
          enable_if_t<N != 0 && N2 != 0> = 0>
constexpr auto concat_maybe(const descr<N, Ts...> &arg0, const descr<N2, Ts2...> &arg1, const Args &...args)
    -> decltype(concat(arg0, concat_maybe(arg1, args...))) {
    return concat(arg0, concat_maybe(arg1, args...));
}

template <size_t N, typename... Ts>
constexpr descr<N + 2, Ts...> type_descr(const descr<N, Ts...> &descr) {
    return const_name("{") + descr + const_name("}");
}

NAMESPACE_END(detail)
NAMESPACE_END(NB_NAMESPACE)
