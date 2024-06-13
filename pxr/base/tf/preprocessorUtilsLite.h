//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_TF_PREPROCESSOR_UTILS_LITE_H
#define PXR_BASE_TF_PREPROCESSOR_UTILS_LITE_H

// This "lite" version of preprocessorUtils exists to avoid dependencies on
// boost.  Do not add any includes of boost headers, such as
// <boost/preprocessor/...> or <boost/vmd/...> to this file.

#include "pxr/base/arch/defines.h"

// Helper for TF_PP_CAT. This extra indirection is required so that macros get
// expanded before the pasting occurs.
#define TF_PP_CAT_IMPL(x, y) x ## y

/// Paste concatenate preprocessor expressions x and y after expansion.  This
/// is similar to BOOST_PP_CAT but doesn't require including the boost config
/// header which is somewhat heavy.
#define TF_PP_CAT(x, y) TF_PP_CAT_IMPL(x, y)

// Helper for TF_PP_STRINGIZE supplying macro expansion before pasting
#define TF_PP_STRINGIZE_IMPL(x) #x

/// Expand and convert the argument to a string, using a most minimal macro.
#define TF_PP_STRINGIZE(x) TF_PP_STRINGIZE_IMPL(x)

#ifdef ARCH_PREPROCESSOR_MSVC_TRADITIONAL

/// Expand to the number of arguments passed.  For example,
/// TF_PP_VARIADIC_SIZE(foo, bar, baz) expands to 3.  Supports up to 64
/// arguments.
#define TF_PP_VARIADIC_SIZE(...) TF_PP_CAT(TF_PP_VARIADIC_SIZE_IMPL(__VA_ARGS__, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1,),)

/// Expand to the \p n'th argument of the arguments following \p n,
/// zero-indexed.  For example, TF_PP_VARIADIC_ELEM(0, a, b, c) expands to a,
/// and TF_PP_VARIADIC_ELEM(1, a, b, c) expands to b.
#define TF_PP_VARIADIC_ELEM(n, ...) TF_PP_VARIADIC_ELEM_IMPL(n,__VA_ARGS__)
#define TF_PP_VARIADIC_ELEM_IMPL(n, ...) TF_PP_CAT(TF_PP_CAT(TF_PP_VAE_, n)(__VA_ARGS__,),)

#else // NOT MSVC

/// Expand to the number of arguments passed.  For example,
/// TF_PP_VARIADIC_SIZE(foo, bar, baz) expands to 3.  Supports up to 64
/// arguments.  Does not work for zero arguments, which is trickier.
#define TF_PP_VARIADIC_SIZE(...) TF_PP_VARIADIC_SIZE_IMPL(__VA_ARGS__, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1,)

/// Expand to the \p n'th argument of the arguments following \p n,
/// zero-indexed.  For example, TF_PP_VARIADIC_ELEM(0, a, b, c) expands to a,
/// and TF_PP_VARIADIC_ELEM(1, a, b, c) expands to b.
#define TF_PP_VARIADIC_ELEM(n, ...) TF_PP_CAT(TF_PP_VAE_, n)(__VA_ARGS__,)

#endif // ARCH_PREPROCESSOR_MSVC_TRADITIONAL

#define TF_PP_VARIADIC_SIZE_IMPL(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60, a61, a62, a63, size, ...) size

#define TF_PP_VAE_0(a0, ...) a0
#define TF_PP_VAE_1(a0, a1, ...) a1
#define TF_PP_VAE_2(a0, a1, a2, ...) a2
#define TF_PP_VAE_3(a0, a1, a2, a3, ...) a3
#define TF_PP_VAE_4(a0, a1, a2, a3, a4, ...) a4
#define TF_PP_VAE_5(a0, a1, a2, a3, a4, a5, ...) a5
#define TF_PP_VAE_6(a0, a1, a2, a3, a4, a5, a6, ...) a6
#define TF_PP_VAE_7(a0, a1, a2, a3, a4, a5, a6, a7, ...) a7
#define TF_PP_VAE_8(a0, a1, a2, a3, a4, a5, a6, a7, a8, ...) a8
#define TF_PP_VAE_9(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, ...) a9
#define TF_PP_VAE_10(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, ...) a10
#define TF_PP_VAE_11(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, ...) a11
#define TF_PP_VAE_12(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, ...) a12
#define TF_PP_VAE_13(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, ...) a13
#define TF_PP_VAE_14(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, ...) a14
#define TF_PP_VAE_15(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, ...) a15
#define TF_PP_VAE_16(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, ...) a16
#define TF_PP_VAE_17(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, ...) a17
#define TF_PP_VAE_18(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, ...) a18
#define TF_PP_VAE_19(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, ...) a19
#define TF_PP_VAE_20(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, ...) a20
#define TF_PP_VAE_21(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, ...) a21
#define TF_PP_VAE_22(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, ...) a22
#define TF_PP_VAE_23(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, ...) a23
#define TF_PP_VAE_24(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, ...) a24
#define TF_PP_VAE_25(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, ...) a25
#define TF_PP_VAE_26(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, ...) a26
#define TF_PP_VAE_27(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, ...) a27
#define TF_PP_VAE_28(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, ...) a28
#define TF_PP_VAE_29(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, ...) a29
#define TF_PP_VAE_30(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, ...) a30
#define TF_PP_VAE_31(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, ...) a31
#define TF_PP_VAE_32(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, ...) a32
#define TF_PP_VAE_33(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, ...) a33
#define TF_PP_VAE_34(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, ...) a34
#define TF_PP_VAE_35(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, ...) a35
#define TF_PP_VAE_36(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, ...) a36
#define TF_PP_VAE_37(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, ...) a37
#define TF_PP_VAE_38(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, ...) a38
#define TF_PP_VAE_39(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, ...) a39
#define TF_PP_VAE_40(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, ...) a40
#define TF_PP_VAE_41(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, ...) a41
#define TF_PP_VAE_42(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, ...) a42
#define TF_PP_VAE_43(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, ...) a43
#define TF_PP_VAE_44(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, ...) a44
#define TF_PP_VAE_45(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, ...) a45
#define TF_PP_VAE_46(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, ...) a46
#define TF_PP_VAE_47(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, ...) a47
#define TF_PP_VAE_48(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, ...) a48
#define TF_PP_VAE_49(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, ...) a49
#define TF_PP_VAE_50(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, ...) a50
#define TF_PP_VAE_51(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, ...) a51
#define TF_PP_VAE_52(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, ...) a52
#define TF_PP_VAE_53(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, ...) a53
#define TF_PP_VAE_54(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, ...) a54
#define TF_PP_VAE_55(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, ...) a55
#define TF_PP_VAE_56(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, ...) a56
#define TF_PP_VAE_57(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, ...) a57
#define TF_PP_VAE_58(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, ...) a58
#define TF_PP_VAE_59(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, ...) a59
#define TF_PP_VAE_60(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60, ...) a60
#define TF_PP_VAE_61(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60, a61, ...) a61
#define TF_PP_VAE_62(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60, a61, a62, ...) a62
#define TF_PP_VAE_63(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27, a28, a29, a30, a31, a32, a33, a34, a35, a36, a37, a38, a39, a40, a41, a42, a43, a44, a45, a46, a47, a48, a49, a50, a51, a52, a53, a54, a55, a56, a57, a58, a59, a60, a61, a62, a63, ...) a63

#ifdef ARCH_PREPROCESSOR_MSVC_TRADITIONAL

#define TF_PP_FE_0(_macro, ...)
#define TF_PP_FE_1(_macro, a) _macro(a)
#define TF_PP_FE_2(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_1,(_macro, __VA_ARGS__))
#define TF_PP_FE_3(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_2,(_macro, __VA_ARGS__))
#define TF_PP_FE_4(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_3,(_macro, __VA_ARGS__))
#define TF_PP_FE_5(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_4,(_macro, __VA_ARGS__))
#define TF_PP_FE_6(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_5,(_macro, __VA_ARGS__))
#define TF_PP_FE_7(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_6,(_macro, __VA_ARGS__))
#define TF_PP_FE_8(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_7,(_macro, __VA_ARGS__))
#define TF_PP_FE_9(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_8,(_macro, __VA_ARGS__))
#define TF_PP_FE_10(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_9,(_macro, __VA_ARGS__))
#define TF_PP_FE_11(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_10,(_macro, __VA_ARGS__))
#define TF_PP_FE_12(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_11,(_macro, __VA_ARGS__))
#define TF_PP_FE_13(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_12,(_macro, __VA_ARGS__))
#define TF_PP_FE_14(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_13,(_macro, __VA_ARGS__))
#define TF_PP_FE_15(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_14,(_macro, __VA_ARGS__))
#define TF_PP_FE_16(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_15,(_macro, __VA_ARGS__))
#define TF_PP_FE_17(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_16,(_macro, __VA_ARGS__))
#define TF_PP_FE_18(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_17,(_macro, __VA_ARGS__))
#define TF_PP_FE_19(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_18,(_macro, __VA_ARGS__))
#define TF_PP_FE_20(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_19,(_macro, __VA_ARGS__))
#define TF_PP_FE_21(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_20,(_macro, __VA_ARGS__))
#define TF_PP_FE_22(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_21,(_macro, __VA_ARGS__))
#define TF_PP_FE_23(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_22,(_macro, __VA_ARGS__))
#define TF_PP_FE_24(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_23,(_macro, __VA_ARGS__))
#define TF_PP_FE_25(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_24,(_macro, __VA_ARGS__))
#define TF_PP_FE_26(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_25,(_macro, __VA_ARGS__))
#define TF_PP_FE_27(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_26,(_macro, __VA_ARGS__))
#define TF_PP_FE_28(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_27,(_macro, __VA_ARGS__))
#define TF_PP_FE_29(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_28,(_macro, __VA_ARGS__))
#define TF_PP_FE_30(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_29,(_macro, __VA_ARGS__))
#define TF_PP_FE_31(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_30,(_macro, __VA_ARGS__))
#define TF_PP_FE_32(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_31,(_macro, __VA_ARGS__))
#define TF_PP_FE_33(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_32,(_macro, __VA_ARGS__))
#define TF_PP_FE_34(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_33,(_macro, __VA_ARGS__))
#define TF_PP_FE_35(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_34,(_macro, __VA_ARGS__))
#define TF_PP_FE_36(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_35,(_macro, __VA_ARGS__))
#define TF_PP_FE_37(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_36,(_macro, __VA_ARGS__))
#define TF_PP_FE_38(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_37,(_macro, __VA_ARGS__))
#define TF_PP_FE_39(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_38,(_macro, __VA_ARGS__))
#define TF_PP_FE_40(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_39,(_macro, __VA_ARGS__))
#define TF_PP_FE_41(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_40,(_macro, __VA_ARGS__))
#define TF_PP_FE_42(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_41,(_macro, __VA_ARGS__))
#define TF_PP_FE_43(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_42,(_macro, __VA_ARGS__))
#define TF_PP_FE_44(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_43,(_macro, __VA_ARGS__))
#define TF_PP_FE_45(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_44,(_macro, __VA_ARGS__))
#define TF_PP_FE_46(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_45,(_macro, __VA_ARGS__))
#define TF_PP_FE_47(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_46,(_macro, __VA_ARGS__))
#define TF_PP_FE_48(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_47,(_macro, __VA_ARGS__))
#define TF_PP_FE_49(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_48,(_macro, __VA_ARGS__))
#define TF_PP_FE_50(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_49,(_macro, __VA_ARGS__))
#define TF_PP_FE_51(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_50,(_macro, __VA_ARGS__))
#define TF_PP_FE_52(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_51,(_macro, __VA_ARGS__))
#define TF_PP_FE_53(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_52,(_macro, __VA_ARGS__))
#define TF_PP_FE_54(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_53,(_macro, __VA_ARGS__))
#define TF_PP_FE_55(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_54,(_macro, __VA_ARGS__))
#define TF_PP_FE_56(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_55,(_macro, __VA_ARGS__))
#define TF_PP_FE_57(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_56,(_macro, __VA_ARGS__))
#define TF_PP_FE_58(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_57,(_macro, __VA_ARGS__))
#define TF_PP_FE_59(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_58,(_macro, __VA_ARGS__))
#define TF_PP_FE_60(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_59,(_macro, __VA_ARGS__))
#define TF_PP_FE_61(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_60,(_macro, __VA_ARGS__))
#define TF_PP_FE_62(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_61,(_macro, __VA_ARGS__))
#define TF_PP_FE_63(_macro, a, ...) _macro(a) TF_PP_CAT(TF_PP_FE_62,(_macro, __VA_ARGS__))

#else // not MSVC

#define TF_PP_FE_0(_macro, ...)
#define TF_PP_FE_1(_macro, a) _macro(a)
#define TF_PP_FE_2(_macro, a, ...) _macro(a) TF_PP_FE_1(_macro, __VA_ARGS__)
#define TF_PP_FE_3(_macro, a, ...) _macro(a) TF_PP_FE_2(_macro, __VA_ARGS__)
#define TF_PP_FE_4(_macro, a, ...) _macro(a) TF_PP_FE_3(_macro, __VA_ARGS__)
#define TF_PP_FE_5(_macro, a, ...) _macro(a) TF_PP_FE_4(_macro, __VA_ARGS__)
#define TF_PP_FE_6(_macro, a, ...) _macro(a) TF_PP_FE_5(_macro, __VA_ARGS__)
#define TF_PP_FE_7(_macro, a, ...) _macro(a) TF_PP_FE_6(_macro, __VA_ARGS__)
#define TF_PP_FE_8(_macro, a, ...) _macro(a) TF_PP_FE_7(_macro, __VA_ARGS__)
#define TF_PP_FE_9(_macro, a, ...) _macro(a) TF_PP_FE_8(_macro, __VA_ARGS__)
#define TF_PP_FE_10(_macro, a, ...) _macro(a) TF_PP_FE_9(_macro, __VA_ARGS__)
#define TF_PP_FE_11(_macro, a, ...) _macro(a) TF_PP_FE_10(_macro, __VA_ARGS__)
#define TF_PP_FE_12(_macro, a, ...) _macro(a) TF_PP_FE_11(_macro, __VA_ARGS__)
#define TF_PP_FE_13(_macro, a, ...) _macro(a) TF_PP_FE_12(_macro, __VA_ARGS__)
#define TF_PP_FE_14(_macro, a, ...) _macro(a) TF_PP_FE_13(_macro, __VA_ARGS__)
#define TF_PP_FE_15(_macro, a, ...) _macro(a) TF_PP_FE_14(_macro, __VA_ARGS__)
#define TF_PP_FE_16(_macro, a, ...) _macro(a) TF_PP_FE_15(_macro, __VA_ARGS__)
#define TF_PP_FE_17(_macro, a, ...) _macro(a) TF_PP_FE_16(_macro, __VA_ARGS__)
#define TF_PP_FE_18(_macro, a, ...) _macro(a) TF_PP_FE_17(_macro, __VA_ARGS__)
#define TF_PP_FE_19(_macro, a, ...) _macro(a) TF_PP_FE_18(_macro, __VA_ARGS__)
#define TF_PP_FE_20(_macro, a, ...) _macro(a) TF_PP_FE_19(_macro, __VA_ARGS__)
#define TF_PP_FE_21(_macro, a, ...) _macro(a) TF_PP_FE_20(_macro, __VA_ARGS__)
#define TF_PP_FE_22(_macro, a, ...) _macro(a) TF_PP_FE_21(_macro, __VA_ARGS__)
#define TF_PP_FE_23(_macro, a, ...) _macro(a) TF_PP_FE_22(_macro, __VA_ARGS__)
#define TF_PP_FE_24(_macro, a, ...) _macro(a) TF_PP_FE_23(_macro, __VA_ARGS__)
#define TF_PP_FE_25(_macro, a, ...) _macro(a) TF_PP_FE_24(_macro, __VA_ARGS__)
#define TF_PP_FE_26(_macro, a, ...) _macro(a) TF_PP_FE_25(_macro, __VA_ARGS__)
#define TF_PP_FE_27(_macro, a, ...) _macro(a) TF_PP_FE_26(_macro, __VA_ARGS__)
#define TF_PP_FE_28(_macro, a, ...) _macro(a) TF_PP_FE_27(_macro, __VA_ARGS__)
#define TF_PP_FE_29(_macro, a, ...) _macro(a) TF_PP_FE_28(_macro, __VA_ARGS__)
#define TF_PP_FE_30(_macro, a, ...) _macro(a) TF_PP_FE_29(_macro, __VA_ARGS__)
#define TF_PP_FE_31(_macro, a, ...) _macro(a) TF_PP_FE_30(_macro, __VA_ARGS__)
#define TF_PP_FE_32(_macro, a, ...) _macro(a) TF_PP_FE_31(_macro, __VA_ARGS__)
#define TF_PP_FE_33(_macro, a, ...) _macro(a) TF_PP_FE_32(_macro, __VA_ARGS__)
#define TF_PP_FE_34(_macro, a, ...) _macro(a) TF_PP_FE_33(_macro, __VA_ARGS__)
#define TF_PP_FE_35(_macro, a, ...) _macro(a) TF_PP_FE_34(_macro, __VA_ARGS__)
#define TF_PP_FE_36(_macro, a, ...) _macro(a) TF_PP_FE_35(_macro, __VA_ARGS__)
#define TF_PP_FE_37(_macro, a, ...) _macro(a) TF_PP_FE_36(_macro, __VA_ARGS__)
#define TF_PP_FE_38(_macro, a, ...) _macro(a) TF_PP_FE_37(_macro, __VA_ARGS__)
#define TF_PP_FE_39(_macro, a, ...) _macro(a) TF_PP_FE_38(_macro, __VA_ARGS__)
#define TF_PP_FE_40(_macro, a, ...) _macro(a) TF_PP_FE_39(_macro, __VA_ARGS__)
#define TF_PP_FE_41(_macro, a, ...) _macro(a) TF_PP_FE_40(_macro, __VA_ARGS__)
#define TF_PP_FE_42(_macro, a, ...) _macro(a) TF_PP_FE_41(_macro, __VA_ARGS__)
#define TF_PP_FE_43(_macro, a, ...) _macro(a) TF_PP_FE_42(_macro, __VA_ARGS__)
#define TF_PP_FE_44(_macro, a, ...) _macro(a) TF_PP_FE_43(_macro, __VA_ARGS__)
#define TF_PP_FE_45(_macro, a, ...) _macro(a) TF_PP_FE_44(_macro, __VA_ARGS__)
#define TF_PP_FE_46(_macro, a, ...) _macro(a) TF_PP_FE_45(_macro, __VA_ARGS__)
#define TF_PP_FE_47(_macro, a, ...) _macro(a) TF_PP_FE_46(_macro, __VA_ARGS__)
#define TF_PP_FE_48(_macro, a, ...) _macro(a) TF_PP_FE_47(_macro, __VA_ARGS__)
#define TF_PP_FE_49(_macro, a, ...) _macro(a) TF_PP_FE_48(_macro, __VA_ARGS__)
#define TF_PP_FE_50(_macro, a, ...) _macro(a) TF_PP_FE_49(_macro, __VA_ARGS__)
#define TF_PP_FE_51(_macro, a, ...) _macro(a) TF_PP_FE_50(_macro, __VA_ARGS__)
#define TF_PP_FE_52(_macro, a, ...) _macro(a) TF_PP_FE_51(_macro, __VA_ARGS__)
#define TF_PP_FE_53(_macro, a, ...) _macro(a) TF_PP_FE_52(_macro, __VA_ARGS__)
#define TF_PP_FE_54(_macro, a, ...) _macro(a) TF_PP_FE_53(_macro, __VA_ARGS__)
#define TF_PP_FE_55(_macro, a, ...) _macro(a) TF_PP_FE_54(_macro, __VA_ARGS__)
#define TF_PP_FE_56(_macro, a, ...) _macro(a) TF_PP_FE_55(_macro, __VA_ARGS__)
#define TF_PP_FE_57(_macro, a, ...) _macro(a) TF_PP_FE_56(_macro, __VA_ARGS__)
#define TF_PP_FE_58(_macro, a, ...) _macro(a) TF_PP_FE_57(_macro, __VA_ARGS__)
#define TF_PP_FE_59(_macro, a, ...) _macro(a) TF_PP_FE_58(_macro, __VA_ARGS__)
#define TF_PP_FE_60(_macro, a, ...) _macro(a) TF_PP_FE_59(_macro, __VA_ARGS__)
#define TF_PP_FE_61(_macro, a, ...) _macro(a) TF_PP_FE_60(_macro, __VA_ARGS__)
#define TF_PP_FE_62(_macro, a, ...) _macro(a) TF_PP_FE_61(_macro, __VA_ARGS__)
#define TF_PP_FE_63(_macro, a, ...) _macro(a) TF_PP_FE_62(_macro, __VA_ARGS__)

#endif

#ifdef ARCH_PREPROCESSOR_MSVC_TRADITIONAL

/// Expand the macro \p x on every variadic argument.  For example
/// TF_PP_FOR_EACH(MACRO, foo, bar, baz) expands to MACRO(foo) MACRO(bar)
/// MACRO(baz).  Supports up to 64 variadic arguments.
#define TF_PP_FOR_EACH(x, ...) \
    TF_PP_CAT(TF_PP_VARIADIC_ELEM(TF_PP_VARIADIC_SIZE(__VA_ARGS__),  \
    TF_PP_FE_0, TF_PP_FE_1, TF_PP_FE_2, TF_PP_FE_3, TF_PP_FE_4,      \
    TF_PP_FE_5, TF_PP_FE_6, TF_PP_FE_7, TF_PP_FE_8, TF_PP_FE_9,      \
    TF_PP_FE_10, TF_PP_FE_11, TF_PP_FE_12, TF_PP_FE_13, TF_PP_FE_14, \
    TF_PP_FE_15, TF_PP_FE_16, TF_PP_FE_17, TF_PP_FE_18, TF_PP_FE_19, \
    TF_PP_FE_20, TF_PP_FE_21, TF_PP_FE_22, TF_PP_FE_23, TF_PP_FE_24, \
    TF_PP_FE_25, TF_PP_FE_26, TF_PP_FE_27, TF_PP_FE_28, TF_PP_FE_29, \
    TF_PP_FE_30, TF_PP_FE_31, TF_PP_FE_32, TF_PP_FE_33, TF_PP_FE_34, \
    TF_PP_FE_35, TF_PP_FE_36, TF_PP_FE_37, TF_PP_FE_38, TF_PP_FE_39, \
    TF_PP_FE_40, TF_PP_FE_41, TF_PP_FE_42, TF_PP_FE_43, TF_PP_FE_44, \
    TF_PP_FE_45, TF_PP_FE_46, TF_PP_FE_47, TF_PP_FE_48, TF_PP_FE_49, \
    TF_PP_FE_50, TF_PP_FE_51, TF_PP_FE_52, TF_PP_FE_53, TF_PP_FE_54, \
    TF_PP_FE_55, TF_PP_FE_56, TF_PP_FE_57, TF_PP_FE_58, TF_PP_FE_59, \
    TF_PP_FE_60, TF_PP_FE_61, TF_PP_FE_62, TF_PP_FE_63)(x, ##__VA_ARGS__),)

#else // Not MSVC.

/// Expand the macro \p x on every variadic argument.  For example
/// TF_PP_FOR_EACH(MACRO, foo, bar, baz) expands to MACRO(foo) MACRO(bar)
/// MACRO(baz).  Supports up to 64 variadic arguments.
#define TF_PP_FOR_EACH(x, ...) \
    TF_PP_VARIADIC_ELEM(TF_PP_VARIADIC_SIZE(__VA_ARGS__),  \
    TF_PP_FE_0, TF_PP_FE_1, TF_PP_FE_2, TF_PP_FE_3, TF_PP_FE_4,      \
    TF_PP_FE_5, TF_PP_FE_6, TF_PP_FE_7, TF_PP_FE_8, TF_PP_FE_9,      \
    TF_PP_FE_10, TF_PP_FE_11, TF_PP_FE_12, TF_PP_FE_13, TF_PP_FE_14, \
    TF_PP_FE_15, TF_PP_FE_16, TF_PP_FE_17, TF_PP_FE_18, TF_PP_FE_19, \
    TF_PP_FE_20, TF_PP_FE_21, TF_PP_FE_22, TF_PP_FE_23, TF_PP_FE_24, \
    TF_PP_FE_25, TF_PP_FE_26, TF_PP_FE_27, TF_PP_FE_28, TF_PP_FE_29, \
    TF_PP_FE_30, TF_PP_FE_31, TF_PP_FE_32, TF_PP_FE_33, TF_PP_FE_34, \
    TF_PP_FE_35, TF_PP_FE_36, TF_PP_FE_37, TF_PP_FE_38, TF_PP_FE_39, \
    TF_PP_FE_40, TF_PP_FE_41, TF_PP_FE_42, TF_PP_FE_43, TF_PP_FE_44, \
    TF_PP_FE_45, TF_PP_FE_46, TF_PP_FE_47, TF_PP_FE_48, TF_PP_FE_49, \
    TF_PP_FE_50, TF_PP_FE_51, TF_PP_FE_52, TF_PP_FE_53, TF_PP_FE_54, \
    TF_PP_FE_55, TF_PP_FE_56, TF_PP_FE_57, TF_PP_FE_58, TF_PP_FE_59, \
    TF_PP_FE_60, TF_PP_FE_61, TF_PP_FE_62, TF_PP_FE_63)(x, ##__VA_ARGS__)

#endif

/// Return the arguments as is except if the first argument starts with a
/// matched parenthesis then remove those parentheses.
/// \ingroup group_tf_Preprocessor
/// \hideinitializer
//
// If the arguments satisfy _TF_PP_IS_PARENS() then we expand to
// _TF_PP_PARENS_EXPAND1, otherwise to _TF_PP_PARENS_EXPAND.  The
// former eats the parentheses while the latter passes the arguments
// unchanged.
//
// We add the ~ after the first __VA_ARGS__ in case there are zero
// arguments. MSVC will complain about insufficient arguments otherwise.
// The ~ will be discarded in any case.
#define TF_PP_EAT_PARENS(...) \
    _TF_PP_IFF(_TF_PP_IS_PARENS(__VA_ARGS__ ~),\
        _TF_PP_PARENS_EXPAND1,_TF_PP_PARENS_EXPAND)(__VA_ARGS__)

/// Expand the arguments and make the result a string.
// We can't use
// BOOST_PP_STRINGIZE because on MSVC passing no arguments will report "not
// enough actual parameters" and yield nothing.  We want no warnings and an
// empty string.  We do that by passing an unused first argument to the inner
// macro (we need an inner macro to cause expansion).  This causes MSVC to
// yield "" for an empty __VA_ARGS__ list.
#define TF_PP_EAT_PARENS_STR(...) _TF_PP_EAT_PARENS_STR2(~, __VA_ARGS__)
#define _TF_PP_EAT_PARENS_STR2(x, ...) #__VA_ARGS__

// Expands to the second argument if c is 1 and the third argument if c is
// 0.  No other values of c are allowed.
#define _TF_PP_IFF(c, t, f) \
    TF_PP_CAT(_TF_PP_IFF_, c)(t, f)
#define _TF_PP_IFF_0(t, f) f
#define _TF_PP_IFF_1(t, f) t

// Force expansion of the arguments.
#define _TF_PP_PARENS_EXPAND(...) __VA_ARGS__

// Similar to expand except it will eat the first matching pair of
// parentheses. For example, _TF_PP_PARENS_EXPAND1((x)(y)) yields x(y).
// The outer _TF_PP_PARENS_EXPAND() is needed for MSVC, which otherwise
// would stringizing to "_TF_PP_PARENS_EXPAND " plus the literal
// substitution of the arguments.
#define _TF_PP_PARENS_EXPAND1(...) \
    _TF_PP_PARENS_EXPAND(_TF_PP_PARENS_EXPAND __VA_ARGS__)

// This works around a MSVC bug.  When a macro expands to FOO(__VA_ARGS__,bar),
// MSVC will separate the arguments of __VA_ARGS__ even if they're inside
// matching parentheses. So, for example, if __VA_ARGS__ is (x,y) then we'll
// expand to FOO(x,y,bar) instead of FOO((x,y),bar).  This macro works around
// that.  Use: _TF_PP_PARENS_CALL(FOO,(__VA_ARGS__,bar)).
//
// We need the _TF_PP_PARENS_EXPAND() here otherwise stringizing will
// stringize the literal replacements, not the result of the expansion of x y.
// If FOO(x,y) expands to x+y then we'd get "FOO ((x,y),bar)" without
// _TF_PP_PARENS_EXPAND() instead of the correct "(x,y)+bar".
#define _TF_PP_PARENS_CALL(x, y) _TF_PP_PARENS_EXPAND(x y)

// Expands to 1 if x starts with a matched parenthesis, otherwise expands to
// 0. "_TF_PP_IS_PARENS2 x" eats the parentheses if they exist and
// expands to "x, 1,", otherwise it expands to _TF_PP_IS_PARENS2
// and the literal expansion of x.  This result goes to
// _TF_PP_IS_PARENS_CHECK_N() which extracts the 1 expanded from
// _TF_PP_IS_PARENS2 or a 0 passed as a final argument.  In either
// case the desired result is the second argument to
// _TF_PP_IS_PARENS_CHECK_N.
#define _TF_PP_IS_PARENS(x) \
    _TF_PP_IS_PARENS_CHECK(_TF_PP_IS_PARENS2 x)
#define _TF_PP_IS_PARENS_CHECK(...) \
    _TF_PP_PARENS_CALL(_TF_PP_IS_PARENS_CHECK_N,(__VA_ARGS__,0,))
#define _TF_PP_IS_PARENS_CHECK_N(x, n, ...) n
#define _TF_PP_IS_PARENS_TRUE(x) x, 1,
#define _TF_PP_IS_PARENS2(...) _TF_PP_IS_PARENS_TRUE(~)

/// Exapnds to 1 if the argument is a tuple, and 0 otherwise.
/// \ingroup group_tf_Preprocessor
/// \hideinitializer
#define TF_PP_IS_TUPLE(arg) _TF_PP_IS_PARENS(arg)

/// Expands to the 'index' element of a non-empty 'tuple'.
#define TF_PP_TUPLE_ELEM(index, tuple) \
    TF_PP_VARIADIC_ELEM(index, TF_PP_EAT_PARENS(tuple))

// Sequence helpers designed to partion a sequence into a head and tail
#define _TF_PP_SEQ_PARTITION_WRAP(...) (__VA_ARGS__)
#define _TF_PP_SEQ_PARTITION_COMMA(elem) elem,
#define _TF_PP_SEQ_PARTITION_HEAD(seq) \
    _TF_PP_SEQ_PARTITION_WRAP(_TF_PP_SEQ_PARTITION_COMMA seq)
#define _TF_PP_SEQ_DISCARD_TAIL(head, ...) head
#define _TF_PP_SEQ_DISCARD_HEAD(head, ...) __VA_ARGS__
#define _TF_PP_SEQ_EXPAND(...) __VA_ARGS__
#define _TF_PP_SEQ_HEAD(seq) \
    _TF_PP_SEQ_EXPAND(_TF_PP_SEQ_DISCARD_TAIL _TF_PP_SEQ_PARTITION_HEAD(seq))
#define _TF_PP_SEQ_TAIL(seq) \
    _TF_PP_SEQ_EXPAND(_TF_PP_SEQ_DISCARD_HEAD _TF_PP_SEQ_PARTITION_HEAD(seq))

#define _TF_PP_SEQ_FE_0(_macro, ...)
#ifdef ARCH_PREPROCESSOR_MSVC_TRADITIONAL
#define _TF_PP_SEQ_FE_1(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),)
// # Generates _TF_PP_SEQ_FE_{2:229} (MSVC)
// python3 -c 'print("\n".join(f"#define _TF_PP_SEQ_FE_{i}(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_{i-1}(_macro, data, _TF_PP_SEQ_TAIL(seq)),)" for i in range(2, 230)))'
#define _TF_PP_SEQ_FE_2(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_1(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_3(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_2(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_4(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_3(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_5(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_4(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_6(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_5(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_7(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_6(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_8(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_7(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_9(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_8(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_10(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_9(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_11(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_10(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_12(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_11(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_13(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_12(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_14(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_13(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_15(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_14(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_16(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_15(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_17(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_16(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_18(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_17(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_19(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_18(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_20(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_19(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_21(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_20(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_22(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_21(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_23(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_22(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_24(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_23(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_25(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_24(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_26(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_25(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_27(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_26(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_28(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_27(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_29(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_28(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_30(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_29(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_31(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_30(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_32(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_31(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_33(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_32(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_34(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_33(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_35(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_34(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_36(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_35(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_37(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_36(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_38(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_37(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_39(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_38(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_40(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_39(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_41(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_40(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_42(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_41(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_43(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_42(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_44(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_43(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_45(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_44(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_46(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_45(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_47(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_46(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_48(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_47(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_49(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_48(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_50(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_49(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_51(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_50(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_52(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_51(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_53(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_52(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_54(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_53(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_55(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_54(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_56(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_55(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_57(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_56(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_58(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_57(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_59(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_58(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_60(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_59(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_61(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_60(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_62(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_61(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_63(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_62(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_64(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_63(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_65(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_64(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_66(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_65(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_67(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_66(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_68(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_67(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_69(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_68(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_70(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_69(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_71(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_70(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_72(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_71(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_73(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_72(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_74(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_73(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_75(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_74(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_76(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_75(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_77(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_76(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_78(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_77(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_79(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_78(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_80(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_79(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_81(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_80(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_82(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_81(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_83(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_82(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_84(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_83(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_85(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_84(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_86(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_85(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_87(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_86(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_88(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_87(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_89(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_88(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_90(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_89(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_91(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_90(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_92(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_91(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_93(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_92(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_94(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_93(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_95(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_94(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_96(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_95(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_97(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_96(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_98(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_97(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_99(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_98(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_100(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_99(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_101(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_100(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_102(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_101(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_103(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_102(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_104(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_103(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_105(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_104(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_106(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_105(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_107(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_106(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_108(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_107(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_109(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_108(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_110(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_109(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_111(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_110(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_112(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_111(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_113(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_112(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_114(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_113(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_115(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_114(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_116(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_115(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_117(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_116(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_118(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_117(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_119(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_118(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_120(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_119(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_121(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_120(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_122(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_121(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_123(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_122(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_124(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_123(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_125(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_124(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_126(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_125(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_127(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_126(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_128(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_127(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_129(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_128(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_130(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_129(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_131(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_130(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_132(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_131(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_133(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_132(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_134(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_133(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_135(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_134(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_136(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_135(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_137(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_136(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_138(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_137(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_139(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_138(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_140(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_139(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_141(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_140(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_142(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_141(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_143(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_142(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_144(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_143(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_145(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_144(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_146(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_145(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_147(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_146(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_148(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_147(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_149(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_148(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_150(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_149(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_151(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_150(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_152(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_151(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_153(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_152(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_154(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_153(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_155(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_154(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_156(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_155(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_157(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_156(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_158(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_157(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_159(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_158(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_160(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_159(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_161(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_160(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_162(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_161(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_163(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_162(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_164(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_163(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_165(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_164(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_166(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_165(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_167(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_166(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_168(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_167(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_169(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_168(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_170(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_169(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_171(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_170(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_172(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_171(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_173(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_172(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_174(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_173(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_175(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_174(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_176(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_175(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_177(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_176(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_178(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_177(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_179(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_178(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_180(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_179(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_181(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_180(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_182(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_181(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_183(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_182(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_184(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_183(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_185(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_184(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_186(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_185(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_187(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_186(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_188(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_187(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_189(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_188(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_190(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_189(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_191(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_190(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_192(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_191(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_193(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_192(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_194(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_193(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_195(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_194(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_196(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_195(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_197(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_196(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_198(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_197(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_199(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_198(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_200(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_199(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_201(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_200(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_202(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_201(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_203(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_202(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_204(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_203(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_205(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_204(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_206(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_205(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_207(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_206(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_208(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_207(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_209(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_208(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_210(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_209(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_211(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_210(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_212(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_211(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_213(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_212(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_214(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_213(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_215(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_214(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_216(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_215(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_217(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_216(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_218(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_217(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_219(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_218(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_220(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_219(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_221(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_220(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_222(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_221(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_223(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_222(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_224(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_223(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_225(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_224(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_226(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_225(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_227(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_226(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_228(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_227(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#define _TF_PP_SEQ_FE_229(_macro, data, seq) TF_PP_CAT(_macro(data, _TF_PP_SEQ_HEAD(seq)),) TF_PP_CAT(_TF_PP_SEQ_FE_228(_macro, data, _TF_PP_SEQ_TAIL(seq)),)
#else // not MSVC
#define _TF_PP_SEQ_FE_1(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq))
// # Generates _TF_PP_SEQ_FE{2:229} (GCC/CLANG)
// python3 -c 'print("\n".join(f"#define _TF_PP_SEQ_FE_{i}(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_{i-1}(_macro, data, _TF_PP_SEQ_TAIL(seq))" for i in range(2, 230)))'
#define _TF_PP_SEQ_FE_2(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_1(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_3(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_2(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_4(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_3(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_5(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_4(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_6(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_5(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_7(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_6(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_8(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_7(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_9(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_8(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_10(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_9(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_11(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_10(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_12(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_11(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_13(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_12(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_14(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_13(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_15(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_14(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_16(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_15(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_17(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_16(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_18(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_17(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_19(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_18(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_20(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_19(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_21(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_20(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_22(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_21(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_23(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_22(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_24(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_23(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_25(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_24(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_26(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_25(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_27(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_26(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_28(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_27(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_29(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_28(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_30(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_29(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_31(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_30(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_32(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_31(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_33(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_32(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_34(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_33(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_35(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_34(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_36(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_35(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_37(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_36(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_38(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_37(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_39(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_38(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_40(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_39(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_41(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_40(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_42(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_41(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_43(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_42(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_44(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_43(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_45(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_44(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_46(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_45(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_47(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_46(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_48(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_47(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_49(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_48(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_50(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_49(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_51(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_50(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_52(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_51(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_53(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_52(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_54(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_53(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_55(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_54(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_56(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_55(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_57(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_56(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_58(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_57(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_59(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_58(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_60(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_59(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_61(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_60(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_62(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_61(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_63(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_62(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_64(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_63(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_65(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_64(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_66(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_65(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_67(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_66(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_68(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_67(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_69(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_68(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_70(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_69(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_71(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_70(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_72(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_71(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_73(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_72(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_74(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_73(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_75(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_74(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_76(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_75(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_77(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_76(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_78(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_77(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_79(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_78(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_80(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_79(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_81(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_80(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_82(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_81(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_83(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_82(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_84(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_83(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_85(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_84(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_86(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_85(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_87(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_86(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_88(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_87(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_89(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_88(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_90(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_89(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_91(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_90(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_92(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_91(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_93(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_92(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_94(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_93(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_95(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_94(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_96(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_95(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_97(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_96(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_98(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_97(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_99(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_98(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_100(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_99(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_101(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_100(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_102(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_101(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_103(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_102(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_104(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_103(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_105(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_104(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_106(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_105(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_107(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_106(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_108(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_107(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_109(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_108(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_110(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_109(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_111(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_110(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_112(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_111(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_113(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_112(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_114(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_113(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_115(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_114(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_116(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_115(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_117(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_116(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_118(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_117(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_119(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_118(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_120(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_119(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_121(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_120(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_122(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_121(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_123(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_122(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_124(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_123(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_125(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_124(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_126(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_125(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_127(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_126(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_128(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_127(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_129(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_128(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_130(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_129(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_131(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_130(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_132(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_131(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_133(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_132(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_134(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_133(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_135(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_134(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_136(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_135(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_137(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_136(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_138(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_137(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_139(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_138(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_140(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_139(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_141(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_140(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_142(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_141(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_143(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_142(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_144(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_143(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_145(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_144(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_146(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_145(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_147(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_146(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_148(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_147(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_149(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_148(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_150(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_149(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_151(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_150(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_152(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_151(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_153(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_152(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_154(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_153(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_155(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_154(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_156(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_155(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_157(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_156(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_158(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_157(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_159(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_158(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_160(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_159(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_161(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_160(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_162(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_161(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_163(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_162(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_164(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_163(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_165(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_164(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_166(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_165(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_167(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_166(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_168(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_167(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_169(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_168(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_170(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_169(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_171(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_170(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_172(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_171(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_173(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_172(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_174(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_173(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_175(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_174(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_176(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_175(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_177(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_176(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_178(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_177(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_179(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_178(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_180(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_179(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_181(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_180(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_182(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_181(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_183(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_182(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_184(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_183(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_185(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_184(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_186(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_185(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_187(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_186(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_188(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_187(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_189(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_188(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_190(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_189(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_191(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_190(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_192(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_191(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_193(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_192(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_194(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_193(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_195(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_194(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_196(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_195(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_197(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_196(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_198(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_197(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_199(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_198(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_200(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_199(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_201(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_200(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_202(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_201(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_203(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_202(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_204(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_203(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_205(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_204(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_206(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_205(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_207(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_206(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_208(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_207(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_209(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_208(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_210(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_209(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_211(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_210(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_212(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_211(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_213(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_212(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_214(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_213(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_215(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_214(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_216(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_215(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_217(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_216(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_218(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_217(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_219(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_218(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_220(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_219(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_221(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_220(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_222(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_221(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_223(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_222(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_224(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_223(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_225(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_224(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_226(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_225(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_227(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_226(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_228(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_227(_macro, data, _TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_FE_229(_macro, data, seq) _macro(data, _TF_PP_SEQ_HEAD(seq)) _TF_PP_SEQ_FE_228(_macro, data, _TF_PP_SEQ_TAIL(seq))
#endif

/// Apply a macro to each element in the sequence of the form (x)(y)(z)(w)
/// \code{cpp}
/// // This should print out 'x y z w'.
/// #define _PRINT(elem) std::cout << elem
/// TF_PP_SEQ_FOR_EACH(_PRINT, ~, ("x")("y")("z")("w"))
/// #undef _PRINT
/// \endcode
/// Limited to sequences of up to 229 elements
#ifdef ARCH_PREPROCESSOR_MSVC_TRADITIONAL
#define _TF_PP_SEQ_FOR_EACH_IMPL(_macro, size, data, seq)                   \
    TF_PP_CAT(TF_PP_CAT(_TF_PP_SEQ_FE_, size),(_macro, data, seq))
#define TF_PP_SEQ_FOR_EACH(_macro, data, seq)                               \
    _TF_PP_SEQ_FOR_EACH_IMPL(_macro, TF_PP_SEQ_SIZE(seq), data, seq)
#else
#define TF_PP_SEQ_FOR_EACH(_macro, data, seq)                               \
    TF_PP_CAT(_TF_PP_SEQ_FE_, TF_PP_SEQ_SIZE(seq))(_macro, data, seq)
#endif

#define _TF_PP_SEQ_SIZE_0(seq) _TF_PP_SEQ_HEAD(seq)
// # Generates the _TF_PP_SEQ_SIZE_{1:229}
// python3 -c 'print("\n".join(f"#define _TF_PP_SEQ_SIZE_{i}(seq) _TF_PP_SEQ_SIZE_{i-1}(_TF_PP_SEQ_TAIL(seq))" for i in range(1, 230)))'
#define _TF_PP_SEQ_SIZE_1(seq) _TF_PP_SEQ_SIZE_0(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_2(seq) _TF_PP_SEQ_SIZE_1(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_3(seq) _TF_PP_SEQ_SIZE_2(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_4(seq) _TF_PP_SEQ_SIZE_3(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_5(seq) _TF_PP_SEQ_SIZE_4(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_6(seq) _TF_PP_SEQ_SIZE_5(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_7(seq) _TF_PP_SEQ_SIZE_6(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_8(seq) _TF_PP_SEQ_SIZE_7(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_9(seq) _TF_PP_SEQ_SIZE_8(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_10(seq) _TF_PP_SEQ_SIZE_9(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_11(seq) _TF_PP_SEQ_SIZE_10(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_12(seq) _TF_PP_SEQ_SIZE_11(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_13(seq) _TF_PP_SEQ_SIZE_12(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_14(seq) _TF_PP_SEQ_SIZE_13(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_15(seq) _TF_PP_SEQ_SIZE_14(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_16(seq) _TF_PP_SEQ_SIZE_15(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_17(seq) _TF_PP_SEQ_SIZE_16(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_18(seq) _TF_PP_SEQ_SIZE_17(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_19(seq) _TF_PP_SEQ_SIZE_18(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_20(seq) _TF_PP_SEQ_SIZE_19(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_21(seq) _TF_PP_SEQ_SIZE_20(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_22(seq) _TF_PP_SEQ_SIZE_21(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_23(seq) _TF_PP_SEQ_SIZE_22(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_24(seq) _TF_PP_SEQ_SIZE_23(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_25(seq) _TF_PP_SEQ_SIZE_24(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_26(seq) _TF_PP_SEQ_SIZE_25(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_27(seq) _TF_PP_SEQ_SIZE_26(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_28(seq) _TF_PP_SEQ_SIZE_27(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_29(seq) _TF_PP_SEQ_SIZE_28(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_30(seq) _TF_PP_SEQ_SIZE_29(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_31(seq) _TF_PP_SEQ_SIZE_30(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_32(seq) _TF_PP_SEQ_SIZE_31(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_33(seq) _TF_PP_SEQ_SIZE_32(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_34(seq) _TF_PP_SEQ_SIZE_33(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_35(seq) _TF_PP_SEQ_SIZE_34(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_36(seq) _TF_PP_SEQ_SIZE_35(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_37(seq) _TF_PP_SEQ_SIZE_36(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_38(seq) _TF_PP_SEQ_SIZE_37(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_39(seq) _TF_PP_SEQ_SIZE_38(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_40(seq) _TF_PP_SEQ_SIZE_39(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_41(seq) _TF_PP_SEQ_SIZE_40(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_42(seq) _TF_PP_SEQ_SIZE_41(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_43(seq) _TF_PP_SEQ_SIZE_42(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_44(seq) _TF_PP_SEQ_SIZE_43(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_45(seq) _TF_PP_SEQ_SIZE_44(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_46(seq) _TF_PP_SEQ_SIZE_45(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_47(seq) _TF_PP_SEQ_SIZE_46(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_48(seq) _TF_PP_SEQ_SIZE_47(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_49(seq) _TF_PP_SEQ_SIZE_48(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_50(seq) _TF_PP_SEQ_SIZE_49(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_51(seq) _TF_PP_SEQ_SIZE_50(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_52(seq) _TF_PP_SEQ_SIZE_51(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_53(seq) _TF_PP_SEQ_SIZE_52(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_54(seq) _TF_PP_SEQ_SIZE_53(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_55(seq) _TF_PP_SEQ_SIZE_54(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_56(seq) _TF_PP_SEQ_SIZE_55(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_57(seq) _TF_PP_SEQ_SIZE_56(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_58(seq) _TF_PP_SEQ_SIZE_57(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_59(seq) _TF_PP_SEQ_SIZE_58(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_60(seq) _TF_PP_SEQ_SIZE_59(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_61(seq) _TF_PP_SEQ_SIZE_60(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_62(seq) _TF_PP_SEQ_SIZE_61(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_63(seq) _TF_PP_SEQ_SIZE_62(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_64(seq) _TF_PP_SEQ_SIZE_63(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_65(seq) _TF_PP_SEQ_SIZE_64(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_66(seq) _TF_PP_SEQ_SIZE_65(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_67(seq) _TF_PP_SEQ_SIZE_66(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_68(seq) _TF_PP_SEQ_SIZE_67(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_69(seq) _TF_PP_SEQ_SIZE_68(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_70(seq) _TF_PP_SEQ_SIZE_69(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_71(seq) _TF_PP_SEQ_SIZE_70(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_72(seq) _TF_PP_SEQ_SIZE_71(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_73(seq) _TF_PP_SEQ_SIZE_72(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_74(seq) _TF_PP_SEQ_SIZE_73(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_75(seq) _TF_PP_SEQ_SIZE_74(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_76(seq) _TF_PP_SEQ_SIZE_75(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_77(seq) _TF_PP_SEQ_SIZE_76(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_78(seq) _TF_PP_SEQ_SIZE_77(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_79(seq) _TF_PP_SEQ_SIZE_78(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_80(seq) _TF_PP_SEQ_SIZE_79(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_81(seq) _TF_PP_SEQ_SIZE_80(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_82(seq) _TF_PP_SEQ_SIZE_81(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_83(seq) _TF_PP_SEQ_SIZE_82(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_84(seq) _TF_PP_SEQ_SIZE_83(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_85(seq) _TF_PP_SEQ_SIZE_84(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_86(seq) _TF_PP_SEQ_SIZE_85(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_87(seq) _TF_PP_SEQ_SIZE_86(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_88(seq) _TF_PP_SEQ_SIZE_87(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_89(seq) _TF_PP_SEQ_SIZE_88(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_90(seq) _TF_PP_SEQ_SIZE_89(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_91(seq) _TF_PP_SEQ_SIZE_90(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_92(seq) _TF_PP_SEQ_SIZE_91(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_93(seq) _TF_PP_SEQ_SIZE_92(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_94(seq) _TF_PP_SEQ_SIZE_93(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_95(seq) _TF_PP_SEQ_SIZE_94(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_96(seq) _TF_PP_SEQ_SIZE_95(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_97(seq) _TF_PP_SEQ_SIZE_96(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_98(seq) _TF_PP_SEQ_SIZE_97(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_99(seq) _TF_PP_SEQ_SIZE_98(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_100(seq) _TF_PP_SEQ_SIZE_99(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_101(seq) _TF_PP_SEQ_SIZE_100(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_102(seq) _TF_PP_SEQ_SIZE_101(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_103(seq) _TF_PP_SEQ_SIZE_102(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_104(seq) _TF_PP_SEQ_SIZE_103(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_105(seq) _TF_PP_SEQ_SIZE_104(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_106(seq) _TF_PP_SEQ_SIZE_105(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_107(seq) _TF_PP_SEQ_SIZE_106(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_108(seq) _TF_PP_SEQ_SIZE_107(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_109(seq) _TF_PP_SEQ_SIZE_108(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_110(seq) _TF_PP_SEQ_SIZE_109(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_111(seq) _TF_PP_SEQ_SIZE_110(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_112(seq) _TF_PP_SEQ_SIZE_111(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_113(seq) _TF_PP_SEQ_SIZE_112(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_114(seq) _TF_PP_SEQ_SIZE_113(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_115(seq) _TF_PP_SEQ_SIZE_114(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_116(seq) _TF_PP_SEQ_SIZE_115(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_117(seq) _TF_PP_SEQ_SIZE_116(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_118(seq) _TF_PP_SEQ_SIZE_117(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_119(seq) _TF_PP_SEQ_SIZE_118(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_120(seq) _TF_PP_SEQ_SIZE_119(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_121(seq) _TF_PP_SEQ_SIZE_120(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_122(seq) _TF_PP_SEQ_SIZE_121(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_123(seq) _TF_PP_SEQ_SIZE_122(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_124(seq) _TF_PP_SEQ_SIZE_123(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_125(seq) _TF_PP_SEQ_SIZE_124(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_126(seq) _TF_PP_SEQ_SIZE_125(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_127(seq) _TF_PP_SEQ_SIZE_126(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_128(seq) _TF_PP_SEQ_SIZE_127(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_129(seq) _TF_PP_SEQ_SIZE_128(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_130(seq) _TF_PP_SEQ_SIZE_129(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_131(seq) _TF_PP_SEQ_SIZE_130(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_132(seq) _TF_PP_SEQ_SIZE_131(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_133(seq) _TF_PP_SEQ_SIZE_132(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_134(seq) _TF_PP_SEQ_SIZE_133(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_135(seq) _TF_PP_SEQ_SIZE_134(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_136(seq) _TF_PP_SEQ_SIZE_135(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_137(seq) _TF_PP_SEQ_SIZE_136(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_138(seq) _TF_PP_SEQ_SIZE_137(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_139(seq) _TF_PP_SEQ_SIZE_138(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_140(seq) _TF_PP_SEQ_SIZE_139(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_141(seq) _TF_PP_SEQ_SIZE_140(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_142(seq) _TF_PP_SEQ_SIZE_141(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_143(seq) _TF_PP_SEQ_SIZE_142(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_144(seq) _TF_PP_SEQ_SIZE_143(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_145(seq) _TF_PP_SEQ_SIZE_144(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_146(seq) _TF_PP_SEQ_SIZE_145(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_147(seq) _TF_PP_SEQ_SIZE_146(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_148(seq) _TF_PP_SEQ_SIZE_147(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_149(seq) _TF_PP_SEQ_SIZE_148(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_150(seq) _TF_PP_SEQ_SIZE_149(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_151(seq) _TF_PP_SEQ_SIZE_150(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_152(seq) _TF_PP_SEQ_SIZE_151(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_153(seq) _TF_PP_SEQ_SIZE_152(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_154(seq) _TF_PP_SEQ_SIZE_153(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_155(seq) _TF_PP_SEQ_SIZE_154(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_156(seq) _TF_PP_SEQ_SIZE_155(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_157(seq) _TF_PP_SEQ_SIZE_156(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_158(seq) _TF_PP_SEQ_SIZE_157(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_159(seq) _TF_PP_SEQ_SIZE_158(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_160(seq) _TF_PP_SEQ_SIZE_159(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_161(seq) _TF_PP_SEQ_SIZE_160(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_162(seq) _TF_PP_SEQ_SIZE_161(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_163(seq) _TF_PP_SEQ_SIZE_162(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_164(seq) _TF_PP_SEQ_SIZE_163(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_165(seq) _TF_PP_SEQ_SIZE_164(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_166(seq) _TF_PP_SEQ_SIZE_165(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_167(seq) _TF_PP_SEQ_SIZE_166(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_168(seq) _TF_PP_SEQ_SIZE_167(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_169(seq) _TF_PP_SEQ_SIZE_168(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_170(seq) _TF_PP_SEQ_SIZE_169(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_171(seq) _TF_PP_SEQ_SIZE_170(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_172(seq) _TF_PP_SEQ_SIZE_171(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_173(seq) _TF_PP_SEQ_SIZE_172(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_174(seq) _TF_PP_SEQ_SIZE_173(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_175(seq) _TF_PP_SEQ_SIZE_174(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_176(seq) _TF_PP_SEQ_SIZE_175(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_177(seq) _TF_PP_SEQ_SIZE_176(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_178(seq) _TF_PP_SEQ_SIZE_177(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_179(seq) _TF_PP_SEQ_SIZE_178(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_180(seq) _TF_PP_SEQ_SIZE_179(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_181(seq) _TF_PP_SEQ_SIZE_180(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_182(seq) _TF_PP_SEQ_SIZE_181(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_183(seq) _TF_PP_SEQ_SIZE_182(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_184(seq) _TF_PP_SEQ_SIZE_183(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_185(seq) _TF_PP_SEQ_SIZE_184(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_186(seq) _TF_PP_SEQ_SIZE_185(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_187(seq) _TF_PP_SEQ_SIZE_186(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_188(seq) _TF_PP_SEQ_SIZE_187(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_189(seq) _TF_PP_SEQ_SIZE_188(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_190(seq) _TF_PP_SEQ_SIZE_189(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_191(seq) _TF_PP_SEQ_SIZE_190(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_192(seq) _TF_PP_SEQ_SIZE_191(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_193(seq) _TF_PP_SEQ_SIZE_192(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_194(seq) _TF_PP_SEQ_SIZE_193(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_195(seq) _TF_PP_SEQ_SIZE_194(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_196(seq) _TF_PP_SEQ_SIZE_195(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_197(seq) _TF_PP_SEQ_SIZE_196(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_198(seq) _TF_PP_SEQ_SIZE_197(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_199(seq) _TF_PP_SEQ_SIZE_198(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_200(seq) _TF_PP_SEQ_SIZE_199(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_201(seq) _TF_PP_SEQ_SIZE_200(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_202(seq) _TF_PP_SEQ_SIZE_201(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_203(seq) _TF_PP_SEQ_SIZE_202(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_204(seq) _TF_PP_SEQ_SIZE_203(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_205(seq) _TF_PP_SEQ_SIZE_204(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_206(seq) _TF_PP_SEQ_SIZE_205(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_207(seq) _TF_PP_SEQ_SIZE_206(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_208(seq) _TF_PP_SEQ_SIZE_207(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_209(seq) _TF_PP_SEQ_SIZE_208(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_210(seq) _TF_PP_SEQ_SIZE_209(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_211(seq) _TF_PP_SEQ_SIZE_210(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_212(seq) _TF_PP_SEQ_SIZE_211(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_213(seq) _TF_PP_SEQ_SIZE_212(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_214(seq) _TF_PP_SEQ_SIZE_213(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_215(seq) _TF_PP_SEQ_SIZE_214(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_216(seq) _TF_PP_SEQ_SIZE_215(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_217(seq) _TF_PP_SEQ_SIZE_216(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_218(seq) _TF_PP_SEQ_SIZE_217(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_219(seq) _TF_PP_SEQ_SIZE_218(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_220(seq) _TF_PP_SEQ_SIZE_219(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_221(seq) _TF_PP_SEQ_SIZE_220(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_222(seq) _TF_PP_SEQ_SIZE_221(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_223(seq) _TF_PP_SEQ_SIZE_222(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_224(seq) _TF_PP_SEQ_SIZE_223(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_225(seq) _TF_PP_SEQ_SIZE_224(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_226(seq) _TF_PP_SEQ_SIZE_225(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_227(seq) _TF_PP_SEQ_SIZE_226(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_228(seq) _TF_PP_SEQ_SIZE_227(_TF_PP_SEQ_TAIL(seq))
#define _TF_PP_SEQ_SIZE_229(seq) _TF_PP_SEQ_SIZE_228(_TF_PP_SEQ_TAIL(seq))
// # Generates _TF_PP_SEQ_SIZE_IMPL for up to 229 elements
// python3 -c 'print("#define _TF_PP_SEQ_SIZE_IMPL(seq) _TF_PP_SEQ_SIZE_229(seq{})".format("".join(f"({i})" for i in reversed(range(0, 230)))))'
#define _TF_PP_SEQ_SIZE_IMPL(seq) _TF_PP_SEQ_SIZE_229(seq(229)(228)(227)(226)(225)(224)(223)(222)(221)(220)(219)(218)(217)(216)(215)(214)(213)(212)(211)(210)(209)(208)(207)(206)(205)(204)(203)(202)(201)(200)(199)(198)(197)(196)(195)(194)(193)(192)(191)(190)(189)(188)(187)(186)(185)(184)(183)(182)(181)(180)(179)(178)(177)(176)(175)(174)(173)(172)(171)(170)(169)(168)(167)(166)(165)(164)(163)(162)(161)(160)(159)(158)(157)(156)(155)(154)(153)(152)(151)(150)(149)(148)(147)(146)(145)(144)(143)(142)(141)(140)(139)(138)(137)(136)(135)(134)(133)(132)(131)(130)(129)(128)(127)(126)(125)(124)(123)(122)(121)(120)(119)(118)(117)(116)(115)(114)(113)(112)(111)(110)(109)(108)(107)(106)(105)(104)(103)(102)(101)(100)(99)(98)(97)(96)(95)(94)(93)(92)(91)(90)(89)(88)(87)(86)(85)(84)(83)(82)(81)(80)(79)(78)(77)(76)(75)(74)(73)(72)(71)(70)(69)(68)(67)(66)(65)(64)(63)(62)(61)(60)(59)(58)(57)(56)(55)(54)(53)(52)(51)(50)(49)(48)(47)(46)(45)(44)(43)(42)(41)(40)(39)(38)(37)(36)(35)(34)(33)(32)(31)(30)(29)(28)(27)(26)(25)(24)(23)(22)(21)(20)(19)(18)(17)(16)(15)(14)(13)(12)(11)(10)(9)(8)(7)(6)(5)(4)(3)(2)(1)(0))

/// Compute the size of a sequence of the form (x)(y)(z)(w)
/// \code{cpp}
/// // The macro should evaluate to 4
/// TF_PP_SEQ_SIZE((x)(y)(z)(w))
/// \endcode
/// Limited to sequences of up to 229 elements
#define TF_PP_SEQ_SIZE(seq) _TF_PP_SEQ_SIZE_IMPL(seq)

#endif // PXR_BASE_TF_PREPROCESSOR_UTILS_LITE_H
