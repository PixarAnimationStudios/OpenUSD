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
//todo: cleanup: TF_DEFINE_PRIVATE_TOKENS, should use the public versions
//todo: cleanup: document each macro extensivly
//todo: cleanup: order macros, so that it is easier to see the structure
//todo: simply syntax, we should get rid of braces for each array element and
//      each element

#ifndef PXR_BASE_TF_STATIC_TOKENS_H
#define PXR_BASE_TF_STATIC_TOKENS_H

/// \file tf/staticTokens.h
///
/// This file defines some macros that are useful for declaring and using
/// static TfTokens.  Typically, using static TfTokens is either cumbersome,
/// unsafe, or slow.  These macros aim to solve many of the common problems.
///
/// The following is an example of how they can be used.
///
/// In header file:
///
/// \code
///    #define MF_TOKENS \.     <--- please ignore '.'
///        (transform)   \.
///        (moves)       \.
///
///        // Syntax when string name differs from symbol.
///        ((foo, "bar"))
///
///        // Syntax when defining an array of tokens. Note that the tokens can 
///        // be used either with amountTs[i] or directly as tx, ty, tz.
///        ((amountTs, ( (tx) (ty) (tz) )))   
///
///    TF_DECLARE_PUBLIC_TOKENS(MfTokens, MF_TOKENS);
/// \endcode
///
/// In cpp file:
///
/// \code
///     TF_DEFINE_PUBLIC_TOKENS(MfTokens, MF_TOKENS);
/// \endcode
///
/// Access the token by using the key as though it were a pointer, like this:
///
/// \code
///    MfTokens->transform;
/// \endcode
///
/// An additional member, allTokens, is a std::vector<TfToken> populated
/// with all of the generated token members.
///
/// There are PUBLIC and PRIVATE versions of the macros.  The PRIVATE ones are
/// intended to be used when the tokens will only be used in a single .cpp
/// file, in which case they can be made file static.  In the case of the
/// PRIVATE, you only need to use the DEFINE macro.

#include "pxr/pxr.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"

#include <vector>

#include <boost/preprocessor/seq/enum.hpp>
#include <boost/preprocessor/seq/filter.hpp>
#include <boost/preprocessor/seq/for_each.hpp>

PXR_NAMESPACE_OPEN_SCOPE

// TF_DECLARE_PUBLIC_TOKENS use these macros to handle two or three arguments.
// The three argument version takes an export/import macro (e.g. TF_API)
// while the two argument version does not export the tokens.

#define _TF_DECLARE_PUBLIC_TOKENS3(key, eiapi, seq)                         \
    _TF_DECLARE_TOKENS3(key, seq, eiapi)                                    \
    extern eiapi TfStaticData<_TF_TOKENS_STRUCT_NAME(key)> key
#define _TF_DECLARE_PUBLIC_TOKENS2(key, seq)                                \
    _TF_DECLARE_TOKENS2(key, seq)                                           \
    extern TfStaticData<_TF_TOKENS_STRUCT_NAME(key)> key
#define _TF_DECLARE_PUBLIC_TOKENS(N) _TF_DECLARE_PUBLIC_TOKENS##N
#define _TF_DECLARE_PUBLIC_TOKENS_EVAL(N) _TF_DECLARE_PUBLIC_TOKENS(N)
#define _TF_DECLARE_PUBLIC_TOKENS_EXPAND(x) x

/// Macro to define public tokens. This declares a list of tokens that can be
/// used globally.  Use in conjunction with TF_DEFINE_PUBLIC_TOKENS.
/// \hideinitializer
#define TF_DECLARE_PUBLIC_TOKENS(...) _TF_DECLARE_PUBLIC_TOKENS_EXPAND( _TF_DECLARE_PUBLIC_TOKENS_EVAL(_TF_DECLARE_PUBLIC_TOKENS_EXPAND( TF_PP_VARIADIC_SIZE(__VA_ARGS__) ))(__VA_ARGS__) )

/// Macro to define public tokens.  Use in conjunction with
/// TF_DECLARE_PUBLIC_TOKENS.
/// \hideinitializer
#define TF_DEFINE_PUBLIC_TOKENS(key, seq)                                   \
    _TF_DEFINE_TOKENS(key)                                                  \
    TfStaticData<_TF_TOKENS_STRUCT_NAME(key)> key

/// Macro to define private tokens.
/// \hideinitializer
#define TF_DEFINE_PRIVATE_TOKENS(key, seq)                                  \
    namespace {                                                             \
    struct _TF_TOKENS_STRUCT_NAME_PRIVATE(key) {                            \
        _TF_TOKENS_STRUCT_NAME_PRIVATE(key)() = default;                    \
        _TF_TOKENS_DECLARE_MEMBERS(seq)                                     \
    };                                                                      \
    }                                                                       \
    static TfStaticData<_TF_TOKENS_STRUCT_NAME_PRIVATE(key)> key

///////////////////////////////////////////////////////////////////////////////
// Private Macros

// Private macro to generate struct name from key.
//
// Note that this needs to be a unique struct name for each translation unit. 
//
#define _TF_TOKENS_STRUCT_NAME_PRIVATE(key) \
    TF_PP_CAT(key, _PrivateStaticTokenType)

// Private macro to generate struct name from key.  This version is used
// by the public token declarations, and so key must be unique for the entire
// namespace.
//
#define _TF_TOKENS_STRUCT_NAME(key) \
    TF_PP_CAT(key, _StaticTokenType)

///////////////////////////////////////////////////////////////////////////////
// Declaration Macros

// Private macro used to generate TfToken member variables.  elem can either
// be a tuple on the form (name, value) or just a name.
//
#define _TF_TOKENS_DECLARE_MEMBER(r, unused, elem)                          \
    TfToken _TF_PP_IFF(TF_PP_IS_TUPLE(elem),                                \
        TF_PP_TUPLE_ELEM(0, elem), elem){                                   \
            _TF_PP_IFF(TF_PP_IS_TUPLE(elem),                                \
                TF_PP_TUPLE_ELEM(1, elem), TF_PP_STRINGIZE(elem)),          \
            TfToken::Immortal};
#define _TF_TOKENS_DECLARE_TOKEN_MEMBERS(seq)                               \
    BOOST_PP_SEQ_FOR_EACH(_TF_TOKENS_DECLARE_MEMBER, ~, seq)

#define _TF_TOKENS_FORWARD_ARRAY(elem) TF_PP_TUPLE_ELEM(0, elem),
#define _TF_TOKENS_DECLARE_ARRAY_MEMBER_IMPL(identifier, ...)               \
    TfToken identifier[TF_PP_VARIADIC_SIZE(__VA_ARGS__)] = {                \
        TF_PP_FOR_EACH(_TF_TOKENS_FORWARD_ARRAY, __VA_ARGS__)};
#define _TF_TOKENS_DECLARE_ARRAY_MEMBER(r, data, elem)                      \
    _TF_TOKENS_DECLARE_ARRAY_MEMBER_IMPL(                                   \
        TF_PP_TUPLE_ELEM(0, elem),                                          \
        BOOST_PP_SEQ_ENUM(TF_PP_EAT_PARENS(TF_PP_TUPLE_ELEM(1, elem))))
#define _TF_TOKENS_DECLARE_ARRAY_MEMBERS(seq)                               \
    BOOST_PP_SEQ_FOR_EACH(_TF_TOKENS_DECLARE_ARRAY_MEMBER, ~, seq)

#define _TF_TOKENS_FORWARD_TOKEN(r, unused, elem) TF_PP_TUPLE_ELEM(0, elem),
#define _TF_TOKENS_DECLARE_ALL_TOKENS(seq)                                  \
    std::vector<TfToken> allTokens =                                        \
        {BOOST_PP_SEQ_FOR_EACH(_TF_TOKENS_FORWARD_TOKEN, ~, seq)};

// Private macro used to declare the list of members as TfTokens
//
#define _TF_TOKENS_DECLARE_MEMBERS(seq)                                     \
    _TF_TOKENS_DECLARE_TOKEN_MEMBERS(                                       \
        BOOST_PP_SEQ_FILTER(_TF_TOKENS_IS_NOT_ARRAY, ~, seq)                \
        _TF_TOKENS_EXPAND_ARRAY_ELEMENTS(seq)                               \
    )                                                                       \
    _TF_TOKENS_DECLARE_ARRAY_MEMBERS(                                       \
        BOOST_PP_SEQ_FILTER(_TF_TOKENS_IS_ARRAY, ~, seq))                   \
    _TF_TOKENS_DECLARE_ALL_TOKENS(                                          \
        BOOST_PP_SEQ_FILTER(_TF_TOKENS_IS_NOT_ARRAY, ~, seq)                \
        _TF_TOKENS_EXPAND_ARRAY_ELEMENTS(seq)                               \
    )

// Private macro that expands all array elements to make them members
// of the sequence.
//
#define _TF_TOKENS_EXPAND_ARRAY_ELEMENTS(seq)                               \
    BOOST_PP_SEQ_FOR_EACH(_TF_TOKENS_APPEND_ARRAY_ELEMENTS,                 \
        ~,                                                                  \
        BOOST_PP_SEQ_FILTER(_TF_TOKENS_IS_ARRAY, ~, seq))                   \

// Private macro used to generate a struct of TfTokens.
//
#define _TF_DECLARE_TOKENS3(key, seq, eiapi)                                \
    struct _TF_TOKENS_STRUCT_NAME(key) {                                    \
        eiapi _TF_TOKENS_STRUCT_NAME(key)();                                \
        eiapi ~_TF_TOKENS_STRUCT_NAME(key)();                               \
        _TF_TOKENS_DECLARE_MEMBERS(seq)                                     \
    };

#define _TF_DECLARE_TOKENS2(key, seq)                                       \
    struct _TF_TOKENS_STRUCT_NAME(key) {                                    \
        _TF_TOKENS_STRUCT_NAME(key)();                                      \
        ~_TF_TOKENS_STRUCT_NAME(key)();                                     \
        _TF_TOKENS_DECLARE_MEMBERS(seq)                                     \
    };

///////////////////////////////////////////////////////////////////////////////
// Filtering Macros

// Private predicate macros to be used by SEQ_FILTER that determine if an
// element of a sequence is an array of tokens or not.
//
#define _TF_TOKENS_IS_ARRAY(s, data, elem)                                  \
    _TF_PP_IFF(TF_PP_IS_TUPLE(elem),                                        \
               TF_PP_IS_TUPLE(TF_PP_TUPLE_ELEM(1, elem)), 0)

#define _TF_TOKENS_IS_NOT_ARRAY(s, data, elem)                              \
    _TF_PP_IFF(_TF_TOKENS_IS_ARRAY(s, data, elem), 0, 1)

// Private macro to append all array elements to a sequence.
//
#define _TF_TOKENS_APPEND_ARRAY_ELEMENTS(r, data, elem)                     \
    TF_PP_TUPLE_ELEM(0, TF_PP_TUPLE_ELEM(1, elem))

///////////////////////////////////////////////////////////////////////////////
// Definition Macros

// Private macro to define the struct of tokens.
//
#define _TF_DEFINE_TOKENS(key)                                              \
    _TF_TOKENS_STRUCT_NAME(key)::~_TF_TOKENS_STRUCT_NAME(key)() = default;  \
    _TF_TOKENS_STRUCT_NAME(key)::_TF_TOKENS_STRUCT_NAME(key)() = default;   \

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_STATIC_TOKENS_H
