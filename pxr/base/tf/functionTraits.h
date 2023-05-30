//
// Copyright 2023 Pixar
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

