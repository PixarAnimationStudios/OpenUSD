//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
//todo: cleanup: TF_DEFINE_PRIVATE_TOKENS, should use the public versions
//todo: cleanup: document each macro extensively
//todo: cleanup: order macros, so that it is easier to see the structure

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
#include "pxr/base/arch/defines.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/tf/token.h"

#include <vector>

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
// This macro expansion is disabled for Intellisense to avoid severe
// slowdowns when using MSVC's non-conforming preprocessor.
#if defined(ARCH_PREPROCESSOR_MSVC_TRADITIONAL) && defined(__INTELLISENSE__)
#define TF_DECLARE_PUBLIC_TOKENS(...)
#else
#define TF_DECLARE_PUBLIC_TOKENS(...) _TF_DECLARE_PUBLIC_TOKENS_EXPAND( _TF_DECLARE_PUBLIC_TOKENS_EVAL(_TF_DECLARE_PUBLIC_TOKENS_EXPAND( TF_PP_VARIADIC_SIZE(__VA_ARGS__) ))(__VA_ARGS__) )
#endif

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
#define _TF_TOKENS_DECLARE_MEMBER(unused, elem)                             \
    TfToken _TF_PP_IFF(TF_PP_IS_TUPLE(elem),                                \
        TF_PP_TUPLE_ELEM(0, elem), elem){                                   \
            _TF_PP_IFF(TF_PP_IS_TUPLE(elem),                                \
                TF_PP_TUPLE_ELEM(1, elem), TF_PP_STRINGIZE(elem)),          \
            TfToken::Immortal};
#define _TF_TOKENS_DECLARE_TOKEN_MEMBERS(seq)                               \
    TF_PP_SEQ_FOR_EACH(_TF_TOKENS_DECLARE_MEMBER, ~, seq)

#define _TF_TOKENS_FORWARD_TOKEN(unused, elem) TF_PP_TUPLE_ELEM(0, elem),
#define _TF_TOKENS_DECLARE_ALL_TOKENS(seq)                                  \
    std::vector<TfToken> allTokens =                                        \
        {TF_PP_SEQ_FOR_EACH(_TF_TOKENS_FORWARD_TOKEN, ~, seq)};

// Private macro used to declare the list of members as TfTokens
//
#define _TF_TOKENS_DECLARE_MEMBERS(seq)                                     \
    _TF_TOKENS_DECLARE_TOKEN_MEMBERS(seq)                               \
    _TF_TOKENS_DECLARE_ALL_TOKENS(seq)                               \

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
// Definition Macros

// Private macro to define the struct of tokens.
//
#define _TF_DEFINE_TOKENS(key)                                              \
    _TF_TOKENS_STRUCT_NAME(key)::~_TF_TOKENS_STRUCT_NAME(key)() = default;  \
    _TF_TOKENS_STRUCT_NAME(key)::_TF_TOKENS_STRUCT_NAME(key)() = default;   \

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_STATIC_TOKENS_H
