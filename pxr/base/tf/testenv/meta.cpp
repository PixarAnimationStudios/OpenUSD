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

#include "pxr/pxr.h"

#include "pxr/base/tf/functionTraits.h"
#include "pxr/base/tf/meta.h"
#include "pxr/base/tf/preprocessorUtils.h"

#include <string>
#include <tuple>
#include <type_traits>

PXR_NAMESPACE_USING_DIRECTIVE

// This a compile-only test -- it only does static_asserts, and produces no
// runtime code.

#define ASSERT_SAME(x, y)                               \
    static_assert(std::is_same<TF_PP_EAT_PARENS(x),     \
                  TF_PP_EAT_PARENS(y)>::value, "")
#define ASSERT_EQUAL(x, y) static_assert(x == y, "")

class TestMemberFns
{
public:
#define MAKE_DECLS(Ret, Name, ...)                            \
    Ret Name (__VA_ARGS__);                                   \
    Ret Name##Ref (__VA_ARGS__) &;                            \
    Ret Name##RvRef (__VA_ARGS__) &&;                         \
    Ret Name##Const (__VA_ARGS__) const;                      \
    Ret Name##ConstRef (__VA_ARGS__) const &;                 \
    Ret Name##ConstRvRef (__VA_ARGS__) const &&

    MAKE_DECLS(void, VoidNoArgs);
    MAKE_DECLS(int, IntNoArgs);
    MAKE_DECLS(void, VoidOneArg, int);
    MAKE_DECLS(int, IntTwoArgs, int, float);
};

int TestFreeFn(int, float &);

void testTfFunctionTraits()
{
    using FreeFnTraits = TfFunctionTraits<decltype(TestFreeFn) *>;
    ASSERT_SAME(FreeFnTraits::ReturnType, int);
    ASSERT_SAME(FreeFnTraits::ArgTypes, (TfMetaList<int, float &>));
    ASSERT_SAME(FreeFnTraits::ArgsTuple, (std::tuple<int, float &>));
    static_assert(FreeFnTraits::Arity == 2, "");

    auto testLambda = [](float &, int, int) { return true; };
    using LambdaTraits = TfFunctionTraits<decltype(testLambda)>;
    ASSERT_SAME(LambdaTraits::ReturnType, bool);
    ASSERT_SAME(LambdaTraits::ArgTypes, (TfMetaList<float &, int, int>));
    ASSERT_SAME(LambdaTraits::ArgsTuple, (std::tuple<float &, int, int>));
    static_assert(LambdaTraits::Arity == 3, "");

#define TEST_MEM_FN(ret, name, ...)                                            \
    TEST_MEM_FN_IMPL(ret, name, TestMemberFns &, __VA_ARGS__);                 \
    TEST_MEM_FN_IMPL(ret, name##Ref, TestMemberFns &, __VA_ARGS__);            \
    TEST_MEM_FN_IMPL(ret, name##RvRef, TestMemberFns &&, __VA_ARGS__);         \
    TEST_MEM_FN_IMPL(ret, name##Const, TestMemberFns const &, __VA_ARGS__);    \
    TEST_MEM_FN_IMPL(ret, name##ConstRef, TestMemberFns const &, __VA_ARGS__); \
    TEST_MEM_FN_IMPL(ret, name##ConstRvRef, TestMemberFns const &&, __VA_ARGS__)

#define TEST_MEM_FN_IMPL(ret, name, thisArg, ...)                              \
    using name##Traits = TfFunctionTraits<decltype(&TestMemberFns::name)>;     \
    ASSERT_SAME(name##Traits::ReturnType, ret);                                \
    ASSERT_SAME((TfMetaApply<TfMetaHead, name##Traits::ArgTypes>), thisArg);   \
    ASSERT_SAME((TfMetaApply<TfMetaTail, name##Traits::ArgTypes>),             \
                (TfMetaList<__VA_ARGS__>))

    TEST_MEM_FN(void, VoidNoArgs);
    TEST_MEM_FN(int, IntNoArgs);
    TEST_MEM_FN(void, VoidOneArg, int);
    TEST_MEM_FN(int, IntTwoArgs, int, float);
    
}

void testTfMeta()
{
    using TestList = TfMetaList<int, float, std::string>;

    ASSERT_SAME((TfMetaApply<TfMetaHead, TestList>), int);
    ASSERT_SAME((TfMetaApply<TfMetaTail, TestList>),
                (TfMetaList<float, std::string>));

    using TestRefList = TfMetaList<int &, float const &, std::string>;
    ASSERT_SAME((TfMetaApply<TfMetaDecay, TestRefList>), TestList);

    ASSERT_SAME((TfMetaApply<TfMetaLength, TestList>),
                (std::integral_constant<size_t, 3>));

    ASSERT_SAME((TfMetaApply<std::tuple, TestList>),
                (std::tuple<int, float, std::string>));
}

