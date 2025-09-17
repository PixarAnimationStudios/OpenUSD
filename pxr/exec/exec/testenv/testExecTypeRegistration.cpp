//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/exec/typeRegistry.h"
#include "pxr/exec/exec/valueExtractor.h"

#include "pxr/usd/sdf/timeCode.h"
#include "pxr/usd/sdf/types.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/stringUtils.h"
#include "pxr/base/tf/type.h"
#include "pxr/base/vt/types.h"

PXR_NAMESPACE_USING_DIRECTIVE

#define ASSERT_EQ(expr, expected)                                       \
    [&] {                                                               \
        auto&& expr_ = expr;                                            \
        if (expr_ != expected) {                                        \
            TF_FATAL_ERROR(                                             \
                "Expected " TF_PP_STRINGIZE(expr) " == '%s'; got '%s'", \
                TfStringify(expected).c_str(),                          \
                TfStringify(expr_).c_str());                            \
        }                                                               \
     }()

// A type that supports the minimum requirements to be an input value from
// external clients, a computation result type and a result value returned to
// external clients.
struct TestExecTypeRegistrationValue
{
    bool operator==(const TestExecTypeRegistrationValue &) const {
        return true;
    }
};

// A type that is not registered with the execution type registry.
struct TestExecTypeRegistrationUnregistered
{
    bool operator==(const TestExecTypeRegistrationUnregistered &) const {
        return true;
    }
};

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<TestExecTypeRegistrationUnregistered>();
}

static void
TestBasicRegistration()
{
    auto &reg = ExecTypeRegistry::GetInstance();

    reg.RegisterType(TestExecTypeRegistrationValue{});
    reg.CheckForRegistration<TestExecTypeRegistrationValue>();
}

// This tests conversion of VtValue to VdfVector with a variety of types.
// The are two special categories of types:
//
//   1. Vt known value types, for which VtValue has optimizations
//      related to type checking.
//   2. Sdf value types, which comprise the types of attribute and
//      metadata in Usd.
//
// The following test cases include types that cover all combinations of
// these categories, including a type that does not belong to either.
// Additionally, for VtArray<T> types, test that CreateVector yields a
// vectorized VdfVector of T rather than a VdfVector holding a single
// VtArray<T>.
//
static void
TestCreateVector()
{
    auto &reg = ExecTypeRegistry::GetInstance();

    // GfVec3d is both a Vt known value type and an Sdf value type.
    {
        static_assert(VtIsKnownValueType<GfVec3d>());
        static_assert(SdfValueTypeTraits<GfVec3d>::IsValueType);
        GfVec3d point(1, 2, 3);
        const VdfVector vec = reg.CreateVector(VtValue(point));
        TF_AXIOM(vec.Holds<GfVec3d>());
        const auto accessor = vec.GetReadAccessor<GfVec3d>();
        ASSERT_EQ(accessor.GetNumValues(), 1);
        ASSERT_EQ(accessor[0], point);
    }

    // VtArray<GfVec3d> is both a Vt known value type and an Sdf value type.
    {
        static_assert(VtIsKnownValueType<VtArray<GfVec3d>>());
        static_assert(SdfValueTypeTraits<VtArray<GfVec3d>>::IsValueType);
        const VtArray<GfVec3d> points = {
            { 0., 0., 0. },
            { 1., 0., 0. },
            { 2., 0., 0. },
            { 3., 0., 0. },
            { 4., 0., 0. },
            { 5., 0., 0. },
        };
        const VdfVector vec = reg.CreateVector(VtValue(points));
        TF_AXIOM(vec.Holds<GfVec3d>());
        const auto accessor = vec.GetReadAccessor<GfVec3d>();
        ASSERT_EQ(accessor.GetNumValues(), 6);
        ASSERT_EQ(accessor[0], points[0]);
        ASSERT_EQ(accessor[1], points[1]);
        ASSERT_EQ(accessor[2], points[2]);
        ASSERT_EQ(accessor[3], points[3]);
        ASSERT_EQ(accessor[4], points[4]);
        ASSERT_EQ(accessor[5], points[5]);
    }

    // short is known to Vt but is not an Sdf value type.
    {
        static_assert(VtIsKnownValueType<short>());
        static_assert(!SdfValueTypeTraits<short>::IsValueType);
        const short x = 3;
        const VdfVector vec = reg.CreateVector(VtValue(x));
        TF_AXIOM(vec.Holds<short>());
        const auto accessor = vec.GetReadAccessor<short>();
        ASSERT_EQ(accessor.GetNumValues(), 1);
        ASSERT_EQ(accessor[0], x);
    }

    // VtArray<short> is known to Vt but is not an Sdf value type.
    {
        static_assert(VtIsKnownValueType<VtArray<short>>());
        static_assert(!SdfValueTypeTraits<VtArray<short>>::IsValueType);
        const VtArray<short> arr = { 0, 1 };
        const VdfVector vec = reg.CreateVector(VtValue(arr));
        TF_AXIOM(vec.Holds<short>());
        const auto accessor = vec.GetReadAccessor<short>();
        ASSERT_EQ(accessor.GetNumValues(), 2);
        ASSERT_EQ(accessor[0], arr[0]);
        ASSERT_EQ(accessor[1], arr[1]);
    }

    // SdfTimeCode is not known to Vt but is an Sdf value type.
    {
        static_assert(!VtIsKnownValueType<SdfTimeCode>());
        static_assert(SdfValueTypeTraits<SdfTimeCode>::IsValueType);
        const SdfTimeCode t = 1.0;
        const VdfVector vec = reg.CreateVector(VtValue(t));
        TF_AXIOM(vec.Holds<SdfTimeCode>());
        const auto accessor = vec.GetReadAccessor<SdfTimeCode>();
        ASSERT_EQ(accessor.GetNumValues(), 1);
        ASSERT_EQ(accessor[0], t);
    }

    // VtArray<SdfTimeCode> is not known to Vt but is an Sdf value type.
    {
        static_assert(!VtIsKnownValueType<VtArray<SdfTimeCode>>());
        static_assert(SdfValueTypeTraits<VtArray<SdfTimeCode>>::IsValueType);
        const VtArray<SdfTimeCode> ts = { 0., 1. };
        const VdfVector vec = reg.CreateVector(VtValue(ts));
        TF_AXIOM(vec.Holds<SdfTimeCode>());
        const auto accessor = vec.GetReadAccessor<SdfTimeCode>();
        ASSERT_EQ(accessor.GetNumValues(), 2);
        ASSERT_EQ(accessor[0], ts[0]);
        ASSERT_EQ(accessor[1], ts[1]);
    }

    // TestExecTypeRegistrationValue is not known to Vt and is not an Sdf
    // value type.
    {
        static_assert(!VtIsKnownValueType<TestExecTypeRegistrationValue>());
        static_assert(!SdfValueTypeTraits<
                      TestExecTypeRegistrationValue>::IsValueType);
        TestExecTypeRegistrationValue val{};
        const VdfVector vec = reg.CreateVector(VtValue(val));
        TF_AXIOM(vec.Holds<TestExecTypeRegistrationValue>());
        const auto accessor = vec.GetReadAccessor<
            TestExecTypeRegistrationValue>();
        ASSERT_EQ(accessor.GetNumValues(), 1);
        // ASSERT_EQ is not used here because it requires that the type have a
        // TfStringify or ostream operator<< overload and this type should
        // define only what is necessary for VtValue & VdfVector.
        TF_AXIOM(accessor[0] == TestExecTypeRegistrationValue{});
    }
}

static void
TestExtractValue()
{
    const auto &reg = ExecTypeRegistry::GetInstance();

    // Get the extractors for GfVec3d and VtArray<GfVec3d>.
    const auto vec3dExtractor = reg.GetExtractor(TfType::Find<GfVec3d>());
    const auto vec3dArrayExtractor = reg.GetExtractor(
        TfType::Find<VtArray<GfVec3d>>());
    TF_AXIOM(vec3dExtractor);
    TF_AXIOM(vec3dArrayExtractor);

    // Construct a vector of one element and a vector of many (three)
    // elements.
    const VdfVector single = VdfTypedVector<GfVec3d>(GfVec3d(-1.));
    const VdfVector many = [] {
        const size_t len = 3;
        VdfVector vec = VdfTypedVector<GfVec3d>::CreateWithSize(len);
        auto accessor = vec.GetReadWriteAccessor<GfVec3d>();
        for (size_t i=0; i<len; ++i) {
            accessor[i] = GfVec3d(-1.*i);
        }
        return vec;
    }();

    const VdfMask singleMask = VdfMask::AllOnes(1);
    const VdfMask manyMask = VdfMask::AllOnes(many.GetSize());

    // Test extracting a value from a vector of length 1 into VtValue as a
    // scalar.
    {
        const VtValue val = vec3dExtractor(single, singleMask);
        TF_AXIOM(val.IsHolding<GfVec3d>());
        const GfVec3d &vec = val.UncheckedGet<GfVec3d>();
        ASSERT_EQ(vec, GfVec3d(-1.));
    }

    // Test extracting a value from a vector of length 1 into VtValue as an
    // array.
    {
        const VtValue val = vec3dArrayExtractor(single, singleMask);
        TF_AXIOM(val.IsHolding<VtArray<GfVec3d>>());
        const VtArray<GfVec3d> &arr = val.UncheckedGet<VtArray<GfVec3d>>();
        ASSERT_EQ(arr[0], GfVec3d(-1.));
    }

    // Test extracting a value from a vector of length 3 into VtValue as an
    // array.
    {
        const VtValue val = vec3dArrayExtractor(many, manyMask);
        TF_AXIOM(val.IsHolding<VtArray<GfVec3d>>());
        const VtArray<GfVec3d> &arr = val.UncheckedGet<VtArray<GfVec3d>>();
        ASSERT_EQ(arr[0], GfVec3d(-0.));
        ASSERT_EQ(arr[1], GfVec3d(-1.));
        ASSERT_EQ(arr[2], GfVec3d(-2.));
    }

    // Test extracting a single value from a vector of length 3 into a VtValue
    // as a scalar.
    {
        VdfMask mask(many.GetSize());
        mask.SetIndex(1);
        const VtValue val = vec3dExtractor(many, mask);
        TF_AXIOM(val.IsHolding<GfVec3d>());
        const GfVec3d &vec = val.UncheckedGet<GfVec3d>();
        ASSERT_EQ(vec, GfVec3d(-1.));
    }

    // Test looking up an extractor for the unknown type.
    {
        TfErrorMark m;
        const auto unknownExtractor = reg.GetExtractor(TfType());
        TF_AXIOM(!m.IsClean());
        m.Clear();
        TF_AXIOM(!unknownExtractor);
    }

    // Test looking up an extractor for a type that is known to TfType
    // but is not an execution type.
    {
        const TfType t = TfType::Find<TestExecTypeRegistrationUnregistered>();
        TF_AXIOM(t);

        TfErrorMark m;
        const auto invalidExtractor = reg.GetExtractor(t);
        TF_AXIOM(!m.IsClean());
        m.Clear();
        TF_AXIOM(!invalidExtractor);
    }
}

int
main()
{
    TestBasicRegistration();
    TestCreateVector();
    TestExtractValue();

    return 0;
}

