//
// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_BASE_TF_FUNCTION_TRAITS_H
#define PXR_BASE_TF_FUNCTION_TRAITS_H

#include "pxr/pxr.h"

#include "pxr/base/tf/meta.h"

#include <cstddef>
#include <tuple>
#include <type_traits>

PXR_NAMESPACE_OPEN_SCOPE

// Function signature representation.
template <class Ret, class ArgTypeList>
struct Tf_FuncSig
{
    using ReturnType = Ret;
    using ArgTypes = ArgTypeList;
    using ArgsTuple = TfMetaApply<std::tuple, ArgTypes>;
    static const size_t Arity = TfMetaApply<TfMetaLength, ArgTypes>::value;

    template <size_t N>
    using NthArg = std::tuple_element_t<N, ArgsTuple>;
};

// Produce a new Tf_FuncSig from FuncSig by removing the initial "this"
// argument.
template <class FuncSig>
using Tf_RemoveThisArg = Tf_FuncSig<
    typename FuncSig::ReturnType,
    TfMetaApply<TfMetaTail, typename FuncSig::ArgTypes>>;

// For callable function objects, get signature from operator(), but remove the
// 'this' arg from that.
template <class Fn>
struct Tf_GetFuncSig
{
    using Type = Tf_RemoveThisArg<
        typename Tf_GetFuncSig<
            decltype(&std::remove_reference<Fn>::type::operator())
            >::Type
        >;
};

// Member function pointers, with optional 'const' and ref-qualifiers.
template <class Ret, class Cls, class... Args>
struct Tf_GetFuncSig<Ret (Cls::*)(Args...)>
{
    using Type = Tf_FuncSig<Ret, TfMetaList<Cls &, Args...>>;
};
template <class Ret, class Cls, class... Args>
struct Tf_GetFuncSig<Ret (Cls::*)(Args...) &>
{
    using Type = Tf_FuncSig<Ret, TfMetaList<Cls &, Args...>>;
};
template <class Ret, class Cls, class... Args>
struct Tf_GetFuncSig<Ret (Cls::*)(Args...) &&>
{
    using Type = Tf_FuncSig<Ret, TfMetaList<Cls &&, Args...>>;
};

template <class Ret, class Cls, class... Args>
struct Tf_GetFuncSig<Ret (Cls::*)(Args...) const>
{
    using Type = Tf_FuncSig<Ret, TfMetaList<Cls const &, Args...>>;
};
template <class Ret, class Cls, class... Args>
struct Tf_GetFuncSig<Ret (Cls::*)(Args...) const &>
{
    using Type = Tf_FuncSig<Ret, TfMetaList<Cls const &, Args...>>;
};
template <class Ret, class Cls, class... Args>
struct Tf_GetFuncSig<Ret (Cls::*)(Args...) const &&>
{
    using Type = Tf_FuncSig<Ret, TfMetaList<Cls const &&, Args...>>;
};

// Regular function pointers.
template <class Ret, class... Args>
struct Tf_GetFuncSig<Ret (*)(Args...)>
{
    using Type = Tf_FuncSig<Ret, TfMetaList<Args...>>;
};

// Function traits.
template <class Fn>
using TfFunctionTraits = typename Tf_GetFuncSig<Fn>::Type;

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_TF_FUNCTION_TRAITS_H

