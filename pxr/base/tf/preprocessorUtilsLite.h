//
// Copyright 2016 Pixar
//
// Licensed under the Apache License, Version 2.0 (the "Apache License")
// with the following modification; you may not use this file except in
// compliance with the Apache License and the following modification to it:
// Section 6. Trademarks. is deleted and replaced with:
//
// 6. Trademarks. This License does not grant permission to use the trade
//    names, trademarks, service marks, or product names of the Licensor
//    and its affiliates, except as required to comply with Section 4(c) of
//    the License and to reproduce the content of the NOTICE file.
//
// You may obtain a copy of the Apache License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the Apache License with the above modification is
// distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied. See the Apache License for the specific
// language governing permissions and limitations under the Apache License.
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

#ifdef ARCH_COMPILER_MSVC

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

#endif // ARCH_COMPILER_MSVC

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

#ifdef ARCH_COMPILER_MSVC

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

#ifdef ARCH_COMPILER_MSVC

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

#endif // PXR_BASE_TF_PREPROCESSOR_UTILS_LITE_H
