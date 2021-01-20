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
#ifndef PXR_BASE_TF_PREPROCESSOR_UTILS_H
#define PXR_BASE_TF_PREPROCESSOR_UTILS_H

/// \file tf/preprocessorUtils.h
/// \ingroup group_tf_Preprocessor

#ifndef TF_MAX_ARITY
#  define TF_MAX_ARITY 7
#endif // TF_MAX_ARITY

#include "pxr/base/arch/defines.h"
#include "pxr/base/arch/pragmas.h"
#include <boost/preprocessor/arithmetic/add.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/control/expr_iif.hpp>
#include <boost/preprocessor/comparison/equal.hpp>
#include <boost/preprocessor/facilities/expand.hpp>
#include <boost/preprocessor/punctuation/comma.hpp>
#include <boost/preprocessor/punctuation/paren.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/tuple/eat.hpp>
#include <boost/preprocessor/tuple/to_list.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#if defined(ARCH_COMPILER_MSVC)
#include <boost/preprocessor/variadic/size.hpp>
#include <boost/vmd/is_empty.hpp>
#include <boost/vmd/is_tuple.hpp>
ARCH_PRAGMA_MACRO_TOO_FEW_ARGUMENTS
#endif

// In boost version 1.51, they seem to have neglected to define this. 
// Without it, some functions will get confused about macros with no arguments
#ifndef BOOST_PP_TUPLE_TO_SEQ_0
#define BOOST_PP_TUPLE_TO_SEQ_0()
#endif

/// Count the number of arguments.
///
/// The underlying macro argument counting trick in the non-Windows
/// implementation originates from a posting on comp.std.c by Laurent Deniau.
///
/// \ingroup group_tf_Preprocessor
/// \hideinitializer
#if defined(ARCH_COMPILER_MSVC)
#define TF_NUM_ARGS(...) \
    BOOST_PP_IIF(BOOST_VMD_IS_EMPTY(__VA_ARGS__),\
        0, BOOST_PP_VARIADIC_SIZE(__VA_ARGS__))
#else
#define TF_NUM_ARGS(...)                                        \
    _TF_NUM_ARGS_CHECK(__VA_ARGS__)                             \
    BOOST_PP_IIF(BOOST_PP_EQUAL(1, _TF_NUM_ARGS1(__VA_ARGS__)), \
        BOOST_PP_EXPAND(TF_ARG_2 BOOST_PP_LPAREN()              \
        BOOST_PP_EXPAND(_TF_NUM_ARGS_0X TF_ARG_1(__VA_ARGS__)   \
        (BOOST_PP_REPEAT(TF_MAX_ARITY, _TF_NUM_ARGS_REP, _TF))) \
        BOOST_PP_COMMA() 1 BOOST_PP_RPAREN()),                  \
        _TF_NUM_ARGS1(__VA_ARGS__))

#define _TF_NUM_ARGS_CHECK(...)                                               \
    BOOST_PP_IIF(_TF_EXPAND(TF_ARG_2 BOOST_PP_LPAREN()                        \
        BOOST_PP_CAT(_TF_NUM_ARGS_00, _TF_EXPAND(                             \
        BOOST_PP_CAT(TF_ARG_, BOOST_PP_INC(TF_MAX_ARITY))                     \
        _TF_NUM_ARGS_TF(__VA_ARGS__))) BOOST_PP_COMMA() 1 BOOST_PP_RPAREN()), \
        _TF_MAX_ARITY_OVERFLOW_IN_TF_NUM_ARGS, BOOST_PP_TUPLE_EAT(1))(...)

#define _TF_NUM_ARGS_00_TF 0, 0
#define _TF_MAX_ARITY_OVERFLOW_IN_TF_NUM_ARGS(a, b, c)

#define _TF_NUM_ARGS_0X(a, ...)                                            \
    _TF_NUM_ARGS_CHECK(a, __VA_ARGS__) 0,                                  \
    BOOST_PP_IIF(BOOST_PP_EQUAL(TF_MAX_ARITY, _TF_NUM_ARGS1(__VA_ARGS__)), \
    0, 1 BOOST_PP_TUPLE_EAT(BOOST_PP_INC(TF_MAX_ARITY)))

#define _TF_EXPAND(x) x // We need this due to a bug in the preprocessor.
#define _TF_NUM_ARGS1(...)                                       \
    _TF_EXPAND(BOOST_PP_CAT(TF_ARG_, BOOST_PP_INC(TF_MAX_ARITY)) \
    _TF_NUM_ARGS_EXT(__VA_ARGS__))

#define _TF_NUM_ARGS_DEC(z, i, n) BOOST_PP_COMMA() BOOST_PP_SUB(n, i)
#define _TF_NUM_ARGS_REP(z, i, n) BOOST_PP_COMMA() n

#define _TF_NUM_ARGS_EXT(...)                                \
    (__VA_ARGS__ BOOST_PP_REPEAT(BOOST_PP_INC(TF_MAX_ARITY), \
    _TF_NUM_ARGS_DEC, TF_MAX_ARITY))
#define _TF_NUM_ARGS_TF(...)                                 \
    (__VA_ARGS__ BOOST_PP_REPEAT(BOOST_PP_INC(TF_MAX_ARITY), \
    _TF_NUM_ARGS_REP, _TF))
#endif


/// Return the arguments as is except if the first argument starts with a
/// matched parenthesis then remove those parentheses.
/// \ingroup group_tf_Preprocessor
/// \hideinitializer
//
// If the arguments satisfy _TF_PP_EAT_PARENS_IS_PARENS() then we expand to
// _TF_PP_EAT_PARENS_EXPAND1, otherwise to _TF_PP_EAT_PARENS_EXPAND.  The
// former eats the parentheses while the latter passes the arguments
// unchanged.
// 
// We add the ~ after the first __VA_ARGS__ in case there are zero
// arguments. MSVC will complain about insufficient arguments otherwise. 
// The ~ will be discarded in any case.
#define TF_PP_EAT_PARENS(...) \
    _TF_PP_EAT_PARENS_IFF(_TF_PP_EAT_PARENS_IS_PARENS(__VA_ARGS__ ~),\
        _TF_PP_EAT_PARENS_EXPAND1,_TF_PP_EAT_PARENS_EXPAND)(__VA_ARGS__)

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
// 0.  No other values of c are allowed.  We can't use BOOST_PP_IFF() because
// it won't expand during stringizing under MSVC.
#define _TF_PP_EAT_PARENS_IFF(c, t, f) \
    BOOST_PP_CAT(_TF_PP_EAT_PARENS_IFF_, c)(t, f)
#define _TF_PP_EAT_PARENS_IFF_0(t, f) f
#define _TF_PP_EAT_PARENS_IFF_1(t, f) t

// Force expansion of the arguments.
#define _TF_PP_EAT_PARENS_EXPAND(...) __VA_ARGS__

// Similar to expand except it will eat the first matching pair of
// parentheses. For example, _TF_PP_EAT_PARENS_EXPAND1((x)(y)) yields x(y). 
// The outer _TF_PP_EAT_PARENS_EXPAND() is needed for MSVC, which otherwise
// would stringizing to "_TF_PP_EAT_PARENS_EXPAND " plus the literal
// substitution of the arguments.
#define _TF_PP_EAT_PARENS_EXPAND1(...) \
    _TF_PP_EAT_PARENS_EXPAND(_TF_PP_EAT_PARENS_EXPAND __VA_ARGS__)

// This works around a MSVC bug.  When a macro expands to FOO(__VA_ARGS__,bar),
// MSVC will separate the arguments of __VA_ARGS__ even if they're inside
// matching parentheses. So, for example, if __VA_ARGS__ is (x,y) then we'll
// expand to FOO(x,y,bar) instead of FOO((x,y),bar).  This macro works around
// that.  Use: _TF_PP_EAT_PARENS_CALL(FOO,(__VA_ARGS__,bar)).
//
// We need the _TF_PP_EAT_PARENS_EXPAND() here otherwise stringizing will
// stringize the literal replacements, not the result of the expansion of x y. 
// If FOO(x,y) expands to x+y then we'd get "FOO ((x,y),bar)" without
// _TF_PP_EAT_PARENS_EXPAND() instead of the correct "(x,y)+bar".
#define _TF_PP_EAT_PARENS_CALL(x, y) _TF_PP_EAT_PARENS_EXPAND(x y)

// Expands to 1 if x starts with a matched parenthesis, otherwise expands to
// 0. "_TF_PP_EAT_PARENS_IS_PARENS2 x" eats the parentheses if they exist and
// expands to "x, 1,", otherwise it expands to _TF_PP_EAT_PARENS_IS_PARENS2
// and the literal expansion of x.  This result goes to
// _TF_PP_EAT_PARENS_IS_PARENS_CHECK_N() which extracts the 1 expanded from
// _TF_PP_EAT_PARENS_IS_PARENS2 or a 0 passed as a final argument.  In either
// case the desired result is the second argument to
// _TF_PP_EAT_PARENS_IS_PARENS_CHECK_N.
#define _TF_PP_EAT_PARENS_IS_PARENS(x) \
    _TF_PP_EAT_PARENS_IS_PARENS_CHECK(_TF_PP_EAT_PARENS_IS_PARENS2 x)
#define _TF_PP_EAT_PARENS_IS_PARENS_CHECK(...) \
    _TF_PP_EAT_PARENS_CALL(_TF_PP_EAT_PARENS_IS_PARENS_CHECK_N,(__VA_ARGS__,0,))
#define _TF_PP_EAT_PARENS_IS_PARENS_CHECK_N(x, n, ...) n
#define _TF_PP_EAT_PARENS_IS_PARENS_TRUE(x) x, 1,
#define _TF_PP_EAT_PARENS_IS_PARENS2(...) _TF_PP_EAT_PARENS_IS_PARENS_TRUE(~)

/// Exapnds to 1 if the argument is a tuple, and 0 otherwise.
/// \ingroup group_tf_Preprocessor
/// \hideinitializer
#if defined(ARCH_COMPILER_MSVC)
    #define TF_PP_IS_TUPLE(sequence) \
        BOOST_VMD_IS_TUPLE(sequence)
#else

#define TF_PP_IS_TUPLE(arg) \
    BOOST_PP_CAT(_TF_PP_IS_TUPLE, BOOST_PP_EXPAND(_TF_PP_IS_TUPLE arg)) )

#define _TF_PP_IS_TUPLE(...) _TF

#define _TF_PP_IS_TUPLE_TF _TF_PP_IS_TUPLE_TRUE(
#define _TF_PP_IS_TUPLE_TF_PP_IS_TUPLE _TF_PP_IS_TUPLE_FALSE(

#define _TF_PP_IS_TUPLE_TRUE() 1
#define _TF_PP_IS_TUPLE_FALSE(arg) 0

#endif 

/// Count the number of elements in a preprocessor tuple.
/// \ingroup group_tf_Preprocessor
/// \hideinitializer
#define TF_PP_TUPLE_SIZE(tuple) \
    BOOST_PP_EXPAND(TF_NUM_ARGS tuple)

/// Convert a preprocessor tuple to a preprocessor list.
/// \ingroup group_tf_Preprocessor
/// \hideinitializer
#define TF_PP_TUPLE_TO_LIST(tuple) \
    BOOST_PP_IIF(                                               \
        BOOST_PP_EQUAL(TF_PP_TUPLE_SIZE(tuple), 0),             \
        BOOST_PP_LIST_NIL,                                      \
        BOOST_PP_TUPLE_TO_LIST(TF_PP_TUPLE_SIZE(tuple), tuple))

/// Convert a preprocessor tuple to a preprocessor sequence.
/// \ingroup group_tf_Preprocessor
/// \hideinitializer
#define TF_PP_TUPLE_TO_SEQ(tuple) \
    BOOST_PP_IIF(                                               \
        BOOST_PP_EQUAL(TF_PP_TUPLE_SIZE(tuple), 0),             \
        BOOST_PP_EMPTY(),                                       \
        BOOST_PP_TUPLE_TO_SEQ(TF_PP_TUPLE_SIZE(tuple), tuple))

/// Create a preprocessor array.
/// \ingroup group_tf_Preprocessor
/// \hideinitializer
#define TF_MAKE_PP_ARRAY(...) \
    (TF_NUM_ARGS(__VA_ARGS__), (__VA_ARGS__))

/// Create a preprocessor list.
/// \ingroup group_tf_Preprocessor
/// \hideinitializer
#define TF_MAKE_PP_LIST(...) \
    TF_PP_TUPLE_TO_LIST((__VA_ARGS__))

/// Create a preprocessor sequence.
/// \ingroup group_tf_Preprocessor
/// \hideinitializer
#define TF_MAKE_PP_SEQ(...) \
    TF_PP_TUPLE_TO_SEQ((__VA_ARGS__))

/// Macros that expand to a specific argument.
/// \ingroup group_tf_Preprocessor
/// \hideinitializer
#define TF_ARG_1(_1,...) _1
#define TF_ARG_2(_1,_2,...) _2
#define TF_ARG_3(_1,_2,_3,...) _3
#define TF_ARG_4(_1,_2,_3,_4,...) _4
#define TF_ARG_5(_1,_2,_3,_4,_5,...) _5
#define TF_ARG_6(_1,_2,_3,_4,_5,_6,...) _6
#define TF_ARG_7(_1,_2,_3,_4,_5,_6,_7,...) _7
#define TF_ARG_8(_1,_2,_3,_4,_5,_6,_7,_8,...) _8
#define TF_ARG_9(_1,_2,_3,_4,_5,_6,_7,_8,_9,...) _9
#define TF_ARG_10(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,...) _10
#define TF_ARG_11(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,...) _11
#define TF_ARG_12(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,...) _12
#define TF_ARG_13(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,...) _13
#define TF_ARG_14(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,...) _14
#define TF_ARG_15(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,...) _15
#define TF_ARG_16(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,...) _16
#define TF_ARG_17(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,...) _17
#define TF_ARG_18(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,...) _18
#define TF_ARG_19(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,...) _19
#define TF_ARG_20(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,...) _20
#define TF_ARG_21(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,...) _21
#define TF_ARG_22(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,...) _22
#define TF_ARG_23(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,...) _23
#define TF_ARG_24(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,...) _24
#define TF_ARG_25(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,...) _25
#define TF_ARG_26(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,...) _26
#define TF_ARG_27(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,...) _27
#define TF_ARG_28(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,...) _28
#define TF_ARG_29(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,...) _29
#define TF_ARG_30(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,...) _30
#define TF_ARG_31(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,...) _31
#define TF_ARG_32(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,...) _32
#define TF_ARG_33(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,...) _33
#define TF_ARG_34(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,...) _34
#define TF_ARG_35(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,...) _35
#define TF_ARG_36(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,...) _36
#define TF_ARG_37(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,...) _37
#define TF_ARG_38(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,...) _38
#define TF_ARG_39(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,...) _39
#define TF_ARG_40(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,...) _40
#define TF_ARG_41(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,...) _41
#define TF_ARG_42(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,...) _42
#define TF_ARG_43(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,...) _43
#define TF_ARG_44(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,...) _44
#define TF_ARG_45(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,...) _45
#define TF_ARG_46(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,...) _46
#define TF_ARG_47(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,...) _47
#define TF_ARG_48(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,...) _48
#define TF_ARG_49(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,...) _49
#define TF_ARG_50(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,...) _50
#define TF_ARG_51(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,...) _51
#define TF_ARG_52(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,...) _52
#define TF_ARG_53(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,...) _53
#define TF_ARG_54(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,...) _54
#define TF_ARG_55(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,...) _55
#define TF_ARG_56(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,...) _56
#define TF_ARG_57(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,...) _57
#define TF_ARG_58(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,...) _58
#define TF_ARG_59(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,...) _59
#define TF_ARG_60(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,...) _60
#define TF_ARG_61(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61,...) _61
#define TF_ARG_62(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61,_62,...) _62
#define TF_ARG_63(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61,_62,_63,...) _63
#define TF_ARG_64(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,_17,_18,_19,_20,_21,_22,_23,_24,_25,_26,_27,_28,_29,_30,_31,_32,_33,_34,_35,_36,_37,_38,_39,_40,_41,_42,_43,_44,_45,_46,_47,_48,_49,_50,_51,_52,_53,_54,_55,_56,_57,_58,_59,_60,_61,_62,_63,_64,...) _64

#if TF_MAX_ARITY > 64
#error "TF_MAX_ARITY is larger than _MAX_ARGS"
#endif

#endif /* PXR_BASE_TF_PREPROCESSOR_UTILS_H */
