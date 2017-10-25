//
// Copyright 2017 Pixar
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
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/interpolation.h"
#include "pxr/usd/usd/clipsAPI.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/kind/registry.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/layerOffset.h"

#include "pxr/base/gf/matrix2f.h"
#include "pxr/base/gf/matrix2d.h"
#include "pxr/base/tf/stringUtils.h"

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include <Python.h>
#endif // PXR_PYTHON_SUPPORT_ENABLED

#include <string>
#include <vector>

using std::string;
using std::vector;

PXR_NAMESPACE_USING_DIRECTIVE

// ------------------------------------------------------------
// Helper functions

template <typename T>
static void 
VerifyAttributeValue(const UsdAttribute& attr, UsdTimeCode time, 
                     T expectedValue)
{
    TF_VERIFY(attr.IsValid());

    // Use templated API to get result and compare.
    T result;
    TF_VERIFY(attr.Get(&result, time));
    TF_VERIFY(result == expectedValue, 
              "(attr: <%s> time: %s): "
              "Got value via templated API: %s, expected value: %s", 
              attr.GetPath().GetString().c_str(),
              TfStringify(time).c_str(),
              TfStringify(result).c_str(),
              TfStringify(expectedValue).c_str());

    // Use type-erased API to get result and compare.
    VtValue vtResult;
    TF_VERIFY(attr.Get(&vtResult, time));
    TF_VERIFY(vtResult.IsHolding<T>());
    TF_VERIFY(vtResult.Get<T>() == expectedValue, 
              "(attr: <%s> time: %s): "
              "Got value via VtValue API: %s, expected value: %s", 
              attr.GetPath().GetString().c_str(),
              TfStringify(time).c_str(),
              TfStringify(vtResult.Get<T>()).c_str(),
              TfStringify(expectedValue).c_str());
}

template <typename T>
static VtArray<T>
CreateVtArray(T fillValue, size_t numElems = 2)
{
    VtArray<T> result(numElems);
    std::fill(result.begin(), result.end(), fillValue);
    return result;
}

template <typename MatrixT, typename T>
static MatrixT
CreateGfMatrix(T fillValue)
{
    MatrixT matrix;
    for (size_t i = 0; i < MatrixT::numRows; ++i) {
        for (size_t j = 0; j < MatrixT::numColumns; ++j) {
            matrix[i][j] = fillValue;
        }
    }

    return matrix;
}

// ------------------------------------------------------------
// Test cases

template <typename T>
struct TestCase
{
};

template <> 
struct TestCase<bool>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr =
            prim.CreateAttribute(TfToken("testBool"), SdfValueTypeNames->Bool);
        TF_VERIFY(attr.Set(true, UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(false, UsdTimeCode(2.0)));
    }
    
    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        // bool does not linearly interpolate
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testBool"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), true);
        VerifyAttributeValue(attr, UsdTimeCode(1.0), true);
        VerifyAttributeValue(attr, UsdTimeCode(2.0), false);
    }
};

template <> 
struct TestCase<VtArray<bool> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr =
            prim.CreateAttribute(TfToken("testBoolArray"), SdfValueTypeNames->BoolArray);
        TF_VERIFY(attr.Set(CreateVtArray(true), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(false), UsdTimeCode(2.0)));
    }
    
    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        // bool does not linearly interpolate
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testBoolArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(true));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(true));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(false));
    }
};

template <> 
struct TestCase<string>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr =
            prim.CreateAttribute(TfToken("testString"), SdfValueTypeNames->String);
        TF_VERIFY(attr.Set(string("s1"), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(string("s2"), UsdTimeCode(2.0)));
    }
    
    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        // string does not linearly interpolate
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testString"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), string("s1"));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), string("s1"));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), string("s2"));
    }
};

template <> 
struct TestCase<VtArray<string> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr =
            prim.CreateAttribute(TfToken("testStringArray"), SdfValueTypeNames->StringArray);
        TF_VERIFY(attr.Set(CreateVtArray(string("s1")), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(string("s2")), UsdTimeCode(2.0)));
    }
    
    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        // string does not linearly interpolate
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testStringArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(string("s1")));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(string("s1")));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(string("s2")));
    }
};

template <> 
struct TestCase<TfToken>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr =
            prim.CreateAttribute(TfToken("testToken"), SdfValueTypeNames->Token);
        TF_VERIFY(attr.Set(TfToken("s1"), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(TfToken("s2"), UsdTimeCode(2.0)));
    }
    
    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        // token does not linearly interpolate
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testToken"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), TfToken("s1"));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), TfToken("s1"));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), TfToken("s2"));
    }
};

template <> 
struct TestCase<VtArray<TfToken> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr =
            prim.CreateAttribute(TfToken("testTokenArray"), SdfValueTypeNames->TokenArray);
        TF_VERIFY(attr.Set(CreateVtArray(TfToken("s1")), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(TfToken("s2")), UsdTimeCode(2.0)));
    }
    
    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        // token does not linearly interpolate
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testTokenArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(TfToken("s1")));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(TfToken("s1")));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(TfToken("s2")));
    }
};

template <> 
struct TestCase<SdfAssetPath>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr =
            prim.CreateAttribute(TfToken("testAsset"), SdfValueTypeNames->Asset);
        TF_VERIFY(attr.Set(SdfAssetPath("s1"), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(SdfAssetPath("s2"), UsdTimeCode(2.0)));
    }
    
    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        // token does not linearly interpolate
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testAsset"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), SdfAssetPath("s1"));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), SdfAssetPath("s1"));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), SdfAssetPath("s2"));
    }
};

template <> 
struct TestCase<VtArray<SdfAssetPath> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr =
            prim.CreateAttribute(TfToken("testAssetArray"), SdfValueTypeNames->AssetArray);
        TF_VERIFY(attr.Set(CreateVtArray(SdfAssetPath("s1")), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(SdfAssetPath("s2")), UsdTimeCode(2.0)));
    }
    
    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        // token does not linearly interpolate
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testAssetArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), 
                             CreateVtArray(SdfAssetPath("s1")));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), 
                             CreateVtArray(SdfAssetPath("s1")));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), 
                             CreateVtArray(SdfAssetPath("s2")));
    }
};

template <>
struct TestCase<GfHalf>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr =
            prim.CreateAttribute(TfToken("testHalf"), SdfValueTypeNames->Half);
        TF_VERIFY(attr.Set(GfHalf(0.0f), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(GfHalf(2.0f), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testHalf"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfHalf(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfHalf(1.0f));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfHalf(2.0f));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testHalf"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfHalf(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfHalf(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfHalf(2.0f));
    }
};

template <>
struct TestCase<VtArray<GfHalf> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testHalfArray"), SdfValueTypeNames->HalfArray);
        TF_VERIFY(attr.Set(CreateVtArray(GfHalf(0.0f)), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(GfHalf(2.0f)), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testHalfArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfHalf(0.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfHalf(1.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfHalf(2.0f)));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testHalfArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfHalf(0.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfHalf(0.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfHalf(2.0f)));
    }
};

template <>
struct TestCase<float>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testFloat"), SdfValueTypeNames->Float);
        TF_VERIFY(attr.Set(0.0f, UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(2.0f, UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testFloat"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), 0.0f);
        VerifyAttributeValue(attr, UsdTimeCode(1.0), 1.0f);
        VerifyAttributeValue(attr, UsdTimeCode(2.0), 2.0f);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testFloat"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), 0.0f);
        VerifyAttributeValue(attr, UsdTimeCode(1.0), 0.0f);
        VerifyAttributeValue(attr, UsdTimeCode(2.0), 2.0f);
    }
};

template <>
struct TestCase<VtArray<float> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testFloatArray"), SdfValueTypeNames->FloatArray);
        TF_VERIFY(attr.Set(CreateVtArray(0.0f), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(2.0f), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testFloatArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(1.0f));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(2.0f));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testFloatArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(2.0f));
    }
};

template <>
struct TestCase<double>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testDouble"), SdfValueTypeNames->Double);
        TF_VERIFY(attr.Set(0.0, UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(2.0, UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testDouble"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), 0.0);
        VerifyAttributeValue(attr, UsdTimeCode(1.0), 1.0);
        VerifyAttributeValue(attr, UsdTimeCode(2.0), 2.0);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testDouble"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), 0.0);
        VerifyAttributeValue(attr, UsdTimeCode(1.0), 0.0);
        VerifyAttributeValue(attr, UsdTimeCode(2.0), 2.0);
    }
};

template <>
struct TestCase<VtArray<double> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testDoubleArray"), SdfValueTypeNames->DoubleArray);
        TF_VERIFY(attr.Set(CreateVtArray(0.0), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(2.0), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testDoubleArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(0.0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(1.0));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(2.0));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testDoubleArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(0.0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(0.0));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(2.0));
    }
};

template <>
struct TestCase<unsigned char>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testUchar"), SdfValueTypeNames->UChar);
        TF_VERIFY(attr.Set<unsigned char>(0, UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set<unsigned char>(2, UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testUchar"));
        VerifyAttributeValue<unsigned char>(attr, UsdTimeCode(0.0), 0);
        VerifyAttributeValue<unsigned char>(attr, UsdTimeCode(1.0), 0);
        VerifyAttributeValue<unsigned char>(attr, UsdTimeCode(2.0), 2);
    }
};

template <>
struct TestCase<VtArray<unsigned char> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testUcharArray"), SdfValueTypeNames->UCharArray);
        TF_VERIFY(attr.Set(CreateVtArray<unsigned char>(0), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray<unsigned char>(2), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testUcharArray"));
        VerifyAttributeValue(
            attr, UsdTimeCode(0.0), CreateVtArray<unsigned char>(0));
        VerifyAttributeValue(
            attr, UsdTimeCode(1.0), CreateVtArray<unsigned char>(0));
        VerifyAttributeValue(
            attr, UsdTimeCode(2.0), CreateVtArray<unsigned char>(2));
    }
};

template <>
struct TestCase<int>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testInt"), SdfValueTypeNames->Int);
        TF_VERIFY(attr.Set(0, UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(2, UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testInt"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), 0);
        VerifyAttributeValue(attr, UsdTimeCode(1.0), 0);
        VerifyAttributeValue(attr, UsdTimeCode(2.0), 2);
    }
};

template <>
struct TestCase<VtArray<int> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testIntArray"), SdfValueTypeNames->IntArray);
        TF_VERIFY(attr.Set(CreateVtArray(0), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(2), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testIntArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(0));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(2));
    }
};

template <>
struct TestCase<unsigned int>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testUInt"), SdfValueTypeNames->UInt);
        TF_VERIFY(attr.Set<unsigned int>(0, UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set<unsigned int>(2, UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testUInt"));
        VerifyAttributeValue<unsigned int>(attr, UsdTimeCode(0.0), 0);
        VerifyAttributeValue<unsigned int>(attr, UsdTimeCode(1.0), 0);
        VerifyAttributeValue<unsigned int>(attr, UsdTimeCode(2.0), 2);
    }
};

template <>
struct TestCase<VtArray<unsigned int> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testUIntArray"), SdfValueTypeNames->UIntArray);
        TF_VERIFY(attr.Set(CreateVtArray<unsigned int>(0), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray<unsigned int>(2), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testUIntArray"));
        VerifyAttributeValue(
            attr, UsdTimeCode(0.0), CreateVtArray<unsigned int>(0));
        VerifyAttributeValue(
            attr, UsdTimeCode(1.0), CreateVtArray<unsigned int>(0));
        VerifyAttributeValue(
            attr, UsdTimeCode(2.0), CreateVtArray<unsigned int>(2));
    }
};

template <>
struct TestCase<int64_t>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testInt64"), SdfValueTypeNames->Int64);
        TF_VERIFY(attr.Set<int64_t>(0, UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set<int64_t>(2, UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testInt64"));
        VerifyAttributeValue<int64_t>(attr, UsdTimeCode(0.0), 0);
        VerifyAttributeValue<int64_t>(attr, UsdTimeCode(1.0), 0);
        VerifyAttributeValue<int64_t>(attr, UsdTimeCode(2.0), 2);
    }
};

template <>
struct TestCase<VtArray<int64_t> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testInt64Array"), SdfValueTypeNames->Int64Array);
        TF_VERIFY(attr.Set(CreateVtArray<int64_t>(0), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray<int64_t>(2), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testInt64Array"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray<int64_t>(0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray<int64_t>(0));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray<int64_t>(2));
    }
};

template <>
struct TestCase<uint64_t>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testUInt64"), SdfValueTypeNames->UInt64);
        TF_VERIFY(attr.Set<uint64_t>(0, UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set<uint64_t>(2, UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testUInt64"));
        VerifyAttributeValue<uint64_t>(attr, UsdTimeCode(0.0), 0);
        VerifyAttributeValue<uint64_t>(attr, UsdTimeCode(1.0), 0);
        VerifyAttributeValue<uint64_t>(attr, UsdTimeCode(2.0), 2);
    }
};

template <>
struct TestCase<VtArray<uint64_t> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testUInt64Array"), SdfValueTypeNames->UInt64Array);
        TF_VERIFY(attr.Set(CreateVtArray<uint64_t>(0), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray<uint64_t>(2), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testUInt64Array"));
        VerifyAttributeValue(
            attr, UsdTimeCode(0.0), CreateVtArray<uint64_t>(0));
        VerifyAttributeValue(
            attr, UsdTimeCode(1.0), CreateVtArray<uint64_t>(0));
        VerifyAttributeValue(
            attr, UsdTimeCode(2.0), CreateVtArray<uint64_t>(2));
    }
};

template <>
struct TestCase<GfVec2d>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec2d"), SdfValueTypeNames->Double2);
        TF_VERIFY(attr.Set(GfVec2d(0.0), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(GfVec2d(2.0), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec2d"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfVec2d(0.0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfVec2d(1.0));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfVec2d(2.0));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec2d"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfVec2d(0.0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfVec2d(0.0));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfVec2d(2.0));
    }
};

template <>
struct TestCase<VtArray<GfVec2d> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec2dArray"), SdfValueTypeNames->Double2Array);
        TF_VERIFY(attr.Set(CreateVtArray(GfVec2d(0.0)), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(GfVec2d(2.0)), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec2dArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfVec2d(0.0)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfVec2d(1.0)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfVec2d(2.0)));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec2dArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfVec2d(0.0)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfVec2d(0.0)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfVec2d(2.0)));
    }
};

template <>
struct TestCase<GfVec2f>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec2f"), SdfValueTypeNames->Float2);
        TF_VERIFY(attr.Set(GfVec2f(0.0f), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(GfVec2f(2.0f), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec2f"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfVec2f(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfVec2f(1.0f));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfVec2f(2.0f));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec2f"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfVec2f(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfVec2f(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfVec2f(2.0f));
    }
};

template <>
struct TestCase<VtArray<GfVec2f> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec2fArray"), SdfValueTypeNames->Float2Array);
        TF_VERIFY(attr.Set(CreateVtArray(GfVec2f(0.0f)), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(GfVec2f(2.0f)), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec2fArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfVec2f(0.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfVec2f(1.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfVec2f(2.0f)));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec2fArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfVec2f(0.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfVec2f(0.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfVec2f(2.0f)));
    }
};

template <>
struct TestCase<GfVec2h>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec2h"), SdfValueTypeNames->Half2);
        TF_VERIFY(attr.Set(GfVec2h(0.0f), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(GfVec2h(2.0f), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec2h"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfVec2h(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfVec2h(1.0f));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfVec2h(2.0f));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec2h"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfVec2h(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfVec2h(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfVec2h(2.0f));
    }
};

template <>
struct TestCase<VtArray<GfVec2h> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec2hArray"), SdfValueTypeNames->Half2Array);
        TF_VERIFY(attr.Set(CreateVtArray(GfVec2h(0.0f)), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(GfVec2h(2.0f)), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec2hArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfVec2h(0.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfVec2h(1.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfVec2h(2.0f)));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec2hArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfVec2h(0.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfVec2h(0.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfVec2h(2.0f)));
    }
};

template <>
struct TestCase<GfVec2i>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec2i"), SdfValueTypeNames->Int2);
        TF_VERIFY(attr.Set(GfVec2i(0), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(GfVec2i(2), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec2i"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfVec2i(0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfVec2i(0));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfVec2i(2));
    }
};

template <>
struct TestCase<VtArray<GfVec2i> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec2iArray"), SdfValueTypeNames->Int2Array);
        TF_VERIFY(attr.Set(CreateVtArray(GfVec2i(0)), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(GfVec2i(2)), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec2iArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfVec2i(0)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfVec2i(0)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfVec2i(2)));
    }
};

template <>
struct TestCase<GfVec3d>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec3d"), SdfValueTypeNames->Double3);
        TF_VERIFY(attr.Set(GfVec3d(0.0), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(GfVec3d(2.0), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec3d"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfVec3d(0.0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfVec3d(1.0));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfVec3d(2.0));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec3d"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfVec3d(0.0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfVec3d(0.0));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfVec3d(2.0));
    }
};

template <>
struct TestCase<VtArray<GfVec3d> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec3dArray"), SdfValueTypeNames->Double3Array);
        TF_VERIFY(attr.Set(CreateVtArray(GfVec3d(0.0)), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(GfVec3d(2.0)), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec3dArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfVec3d(0.0)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfVec3d(1.0)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfVec3d(2.0)));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec3dArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfVec3d(0.0)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfVec3d(0.0)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfVec3d(2.0)));
    }
};

template <>
struct TestCase<GfVec3f>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec3f"), SdfValueTypeNames->Float3);
        TF_VERIFY(attr.Set(GfVec3f(0.0f), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(GfVec3f(2.0f), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec3f"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfVec3f(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfVec3f(1.0f));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfVec3f(2.0f));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec3f"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfVec3f(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfVec3f(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfVec3f(2.0f));
    }
};

template <>
struct TestCase<VtArray<GfVec3f> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec3fArray"), SdfValueTypeNames->Float3Array);
        TF_VERIFY(attr.Set(CreateVtArray(GfVec3f(0.0f)), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(GfVec3f(2.0f)), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec3fArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfVec3f(0.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfVec3f(1.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfVec3f(2.0f)));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec3fArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfVec3f(0.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfVec3f(0.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfVec3f(2.0f)));
    }
};

template <>
struct TestCase<GfVec3h>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec3h"), SdfValueTypeNames->Half3);
        TF_VERIFY(attr.Set(GfVec3h(0.0f), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(GfVec3h(2.0f), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec3h"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfVec3h(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfVec3h(1.0f));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfVec3h(2.0f));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec3h"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfVec3h(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfVec3h(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfVec3h(2.0f));
    }
};

template <>
struct TestCase<VtArray<GfVec3h> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec3hArray"), SdfValueTypeNames->Half3Array);
        TF_VERIFY(attr.Set(CreateVtArray(GfVec3h(0.0f)), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(GfVec3h(2.0f)), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec3hArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfVec3h(0.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfVec3h(1.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfVec3h(2.0f)));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec3hArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfVec3h(0.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfVec3h(0.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfVec3h(2.0f)));
    }
};

template <>
struct TestCase<GfVec3i>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec3i"), SdfValueTypeNames->Int3);
        TF_VERIFY(attr.Set(GfVec3i(0), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(GfVec3i(2), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec3i"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfVec3i(0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfVec3i(0));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfVec3i(2));
    }
};

template <>
struct TestCase<VtArray<GfVec3i> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec3iArray"), SdfValueTypeNames->Int3Array);
        TF_VERIFY(attr.Set(CreateVtArray(GfVec3i(0)), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(GfVec3i(2)), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec3iArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfVec3i(0)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfVec3i(0)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfVec3i(2)));
    }
};

template <>
struct TestCase<GfVec4d>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec4d"), SdfValueTypeNames->Double4);
        TF_VERIFY(attr.Set(GfVec4d(0.0), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(GfVec4d(2.0), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec4d"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfVec4d(0.0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfVec4d(1.0));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfVec4d(2.0));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec4d"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfVec4d(0.0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfVec4d(0.0));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfVec4d(2.0));
    }
};

template <>
struct TestCase<VtArray<GfVec4d> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec4dArray"), SdfValueTypeNames->Double4Array);
        TF_VERIFY(attr.Set(CreateVtArray(GfVec4d(0.0)), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(GfVec4d(2.0)), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec4dArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfVec4d(0.0)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfVec4d(1.0)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfVec4d(2.0)));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec4dArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfVec4d(0.0)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfVec4d(0.0)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfVec4d(2.0)));
    }
};

template <>
struct TestCase<GfVec4f>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec4f"), SdfValueTypeNames->Float4);
        TF_VERIFY(attr.Set(GfVec4f(0.0f), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(GfVec4f(2.0f), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec4f"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfVec4f(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfVec4f(1.0f));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfVec4f(2.0f));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec4f"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfVec4f(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfVec4f(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfVec4f(2.0f));
    }
};

template <>
struct TestCase<VtArray<GfVec4f> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec4fArray"), SdfValueTypeNames->Float4Array);
        TF_VERIFY(attr.Set(CreateVtArray(GfVec4f(0.0f)), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(GfVec4f(2.0f)), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec4fArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfVec4f(0.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfVec4f(1.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfVec4f(2.0f)));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec4fArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfVec4f(0.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfVec4f(0.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfVec4f(2.0f)));
    }
};

template <>
struct TestCase<GfVec4h>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec4h"), SdfValueTypeNames->Half4);
        TF_VERIFY(attr.Set(GfVec4h(0.0f), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(GfVec4h(2.0f), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec4h"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfVec4h(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfVec4h(1.0f));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfVec4h(2.0f));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec4h"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfVec4h(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfVec4h(0.0f));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfVec4h(2.0f));
    }
};

template <>
struct TestCase<VtArray<GfVec4h> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec4hArray"), SdfValueTypeNames->Half4Array);
        TF_VERIFY(attr.Set(CreateVtArray(GfVec4h(0.0f)), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(GfVec4h(2.0f)), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec4hArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfVec4h(0.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfVec4h(1.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfVec4h(2.0f)));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec4hArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfVec4h(0.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfVec4h(0.0f)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfVec4h(2.0f)));
    }
};

template <>
struct TestCase<GfVec4i>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec4i"), SdfValueTypeNames->Int4);
        TF_VERIFY(attr.Set(GfVec4i(0), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(GfVec4i(2), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec4i"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfVec4i(0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfVec4i(0));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfVec4i(2));
    }
};

template <>
struct TestCase<VtArray<GfVec4i> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testVec4iArray"), SdfValueTypeNames->Int4Array);
        TF_VERIFY(attr.Set(CreateVtArray(GfVec4i(0)), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(GfVec4i(2)), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        TestHeldInterpolation(prim);
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testVec4iArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfVec4i(0)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfVec4i(0)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfVec4i(2)));
    }
};

template <>
struct TestCase<GfMatrix2d>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testMatrix2d"), SdfValueTypeNames->Matrix2d);
        TF_VERIFY(attr.Set(CreateGfMatrix<GfMatrix2d>(0.0), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateGfMatrix<GfMatrix2d>(2.0), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testMatrix2d"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), 
                             CreateGfMatrix<GfMatrix2d>(0.0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), 
                             CreateGfMatrix<GfMatrix2d>(1.0));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), 
                             CreateGfMatrix<GfMatrix2d>(2.0));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testMatrix2d"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), 
                             CreateGfMatrix<GfMatrix2d>(0.0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), 
                             CreateGfMatrix<GfMatrix2d>(0.0));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), 
                             CreateGfMatrix<GfMatrix2d>(2.0));
    }
};

template <>
struct TestCase<VtArray<GfMatrix2d> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testMatrix2dArray"), SdfValueTypeNames->Matrix2dArray);
        TF_VERIFY(attr.Set(CreateVtArray(CreateGfMatrix<GfMatrix2d>(0.0)),
                           UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(CreateGfMatrix<GfMatrix2d>(2.0)),
                           UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testMatrix2dArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), 
                             CreateVtArray(CreateGfMatrix<GfMatrix2d>(0.0)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), 
                             CreateVtArray(CreateGfMatrix<GfMatrix2d>(1.0)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), 
                             CreateVtArray(CreateGfMatrix<GfMatrix2d>(2.0)));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testMatrix2dArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), 
                             CreateVtArray(CreateGfMatrix<GfMatrix2d>(0.0)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), 
                             CreateVtArray(CreateGfMatrix<GfMatrix2d>(0.0)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), 
                             CreateVtArray(CreateGfMatrix<GfMatrix2d>(2.0)));
    }
};

template <>
struct TestCase<GfMatrix3d>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testMatrix3d"), SdfValueTypeNames->Matrix3d);
        TF_VERIFY(attr.Set(CreateGfMatrix<GfMatrix3d>(0.0), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateGfMatrix<GfMatrix3d>(2.0), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testMatrix3d"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), 
                             CreateGfMatrix<GfMatrix3d>(0.0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), 
                             CreateGfMatrix<GfMatrix3d>(1.0));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), 
                             CreateGfMatrix<GfMatrix3d>(2.0));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testMatrix3d"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), 
                             CreateGfMatrix<GfMatrix3d>(0.0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), 
                             CreateGfMatrix<GfMatrix3d>(0.0));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), 
                             CreateGfMatrix<GfMatrix3d>(2.0));
    }
};

template <>
struct TestCase<VtArray<GfMatrix3d> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testMatrix3dArray"), SdfValueTypeNames->Matrix3dArray);
        TF_VERIFY(attr.Set(CreateVtArray(CreateGfMatrix<GfMatrix3d>(0.0)),
                           UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(CreateGfMatrix<GfMatrix3d>(2.0)),
                           UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testMatrix3dArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), 
                             CreateVtArray(CreateGfMatrix<GfMatrix3d>(0.0)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), 
                             CreateVtArray(CreateGfMatrix<GfMatrix3d>(1.0)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), 
                             CreateVtArray(CreateGfMatrix<GfMatrix3d>(2.0)));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testMatrix3dArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), 
                             CreateVtArray(CreateGfMatrix<GfMatrix3d>(0.0)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), 
                             CreateVtArray(CreateGfMatrix<GfMatrix3d>(0.0)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), 
                             CreateVtArray(CreateGfMatrix<GfMatrix3d>(2.0)));
    }
};

template <>
struct TestCase<GfMatrix4d>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testMatrix4d"), SdfValueTypeNames->Matrix4d);
        TF_VERIFY(attr.Set(CreateGfMatrix<GfMatrix4d>(0.0), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateGfMatrix<GfMatrix4d>(2.0), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testMatrix4d"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), 
                             CreateGfMatrix<GfMatrix4d>(0.0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), 
                             CreateGfMatrix<GfMatrix4d>(1.0));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), 
                             CreateGfMatrix<GfMatrix4d>(2.0));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testMatrix4d"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), 
                             CreateGfMatrix<GfMatrix4d>(0.0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), 
                             CreateGfMatrix<GfMatrix4d>(0.0));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), 
                             CreateGfMatrix<GfMatrix4d>(2.0));
    }
};

template <>
struct TestCase<VtArray<GfMatrix4d> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testMatrix4dArray"), SdfValueTypeNames->Matrix4dArray);
        TF_VERIFY(attr.Set(CreateVtArray(CreateGfMatrix<GfMatrix4d>(0.0)),
                           UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(CreateGfMatrix<GfMatrix4d>(2.0)),
                           UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testMatrix4dArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), 
                             CreateVtArray(CreateGfMatrix<GfMatrix4d>(0.0)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), 
                             CreateVtArray(CreateGfMatrix<GfMatrix4d>(1.0)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), 
                             CreateVtArray(CreateGfMatrix<GfMatrix4d>(2.0)));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testMatrix4dArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), 
                             CreateVtArray(CreateGfMatrix<GfMatrix4d>(0.0)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), 
                             CreateVtArray(CreateGfMatrix<GfMatrix4d>(0.0)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), 
                             CreateVtArray(CreateGfMatrix<GfMatrix4d>(2.0)));
    }
};

template <>
struct TestCase<GfQuatd>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testQuatd"), SdfValueTypeNames->Quatd);
        TF_VERIFY(attr.Set(GfQuatd(0), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(GfQuatd(1), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testQuatd"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfQuatd(0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0),
                             GfSlerp(0.5, GfQuatd(0), GfQuatd(1)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfQuatd(1));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testQuatd"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfQuatd(0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfQuatd(0));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfQuatd(1));
    }
};

template <>
struct TestCase<VtArray<GfQuatd> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testQuatdArray"), SdfValueTypeNames->QuatdArray);
        TF_VERIFY(attr.Set(CreateVtArray(GfQuatd(0)), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(GfQuatd(1)), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testQuatdArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfQuatd(0)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0),
                             CreateVtArray(
                                 GfSlerp(0.5, GfQuatd(0), GfQuatd(1))));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfQuatd(1)));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testQuatdArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfQuatd(0)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfQuatd(0)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfQuatd(1)));
    }
};

template <>
struct TestCase<GfQuatf>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testQuatf"), SdfValueTypeNames->Quatf);
        TF_VERIFY(attr.Set(GfQuatf(0), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(GfQuatf(1), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testQuatf"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfQuatf(0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0),
                             GfSlerp(0.5, GfQuatf(0), GfQuatf(1)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfQuatf(1));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testQuatf"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfQuatf(0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfQuatf(0));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfQuatf(1));
    }
};

template <>
struct TestCase<VtArray<GfQuatf> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testQuatfArray"), SdfValueTypeNames->QuatfArray);
        TF_VERIFY(attr.Set(CreateVtArray(GfQuatf(0)), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(GfQuatf(1)), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testQuatfArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfQuatf(0)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0),
                             CreateVtArray(
                                 GfSlerp(0.5, GfQuatf(0), GfQuatf(1))));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfQuatf(1)));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testQuatfArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfQuatf(0)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfQuatf(0)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfQuatf(1)));
    }
};

template <>
struct TestCase<GfQuath>
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testQuath"), SdfValueTypeNames->Quath);
        TF_VERIFY(attr.Set(GfQuath(0), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(GfQuath(1), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testQuath"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfQuath(0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0),
                             GfSlerp(0.5, GfQuath(0), GfQuath(1)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfQuath(1));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testQuath"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), GfQuath(0));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), GfQuath(0));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), GfQuath(1));
    }
};

template <>
struct TestCase<VtArray<GfQuath> >
{
    static void AddTestCase(const UsdPrim& prim)
    {
        UsdAttribute attr = 
            prim.CreateAttribute(TfToken("testQuathArray"), SdfValueTypeNames->QuathArray);
        TF_VERIFY(attr.Set(CreateVtArray(GfQuath(0)), UsdTimeCode(0.0)));
        TF_VERIFY(attr.Set(CreateVtArray(GfQuath(1)), UsdTimeCode(2.0)));
    }

    static void TestLinearInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testQuathArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfQuath(0)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0),
                             CreateVtArray(
                                 GfSlerp(0.5, GfQuath(0), GfQuath(1))));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfQuath(1)));
    }

    static void TestHeldInterpolation(const UsdPrim& prim)
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("testQuathArray"));
        VerifyAttributeValue(attr, UsdTimeCode(0.0), CreateVtArray(GfQuath(0)));
        VerifyAttributeValue(attr, UsdTimeCode(1.0), CreateVtArray(GfQuath(0)));
        VerifyAttributeValue(attr, UsdTimeCode(2.0), CreateVtArray(GfQuath(1)));
    }
};


// ------------------------------------------------------------

static size_t
AddTestCasesToPrim(const UsdPrim& prim)
{
    size_t numTestCasesAdded = 0;
#define ADD_TEST_CASE(r, unused, tup)                                   \
    {                                                                   \
        typedef SDF_VALUE_TRAITS_TYPE(tup)::Type Type;                  \
        typedef SDF_VALUE_TRAITS_TYPE(tup)::ShapedType ShapedType;      \
        TestCase<Type>::AddTestCase(prim);                              \
        TestCase<ShapedType>::AddTestCase(prim);                        \
                                                                        \
        ++numTestCasesAdded;                                            \
    }
    BOOST_PP_SEQ_FOR_EACH(ADD_TEST_CASE, ~, SDF_VALUE_TYPES);
#undef ADD_TEST_CASE

    return numTestCasesAdded;
}

static void
ScaleAttributeSampledTimes(const UsdPrim& prim, double scale = 0.5)
{
    std::vector<UsdAttribute> attributes = prim.GetAuthoredAttributes();
    for (size_t i = 0; i < attributes.size(); ++i) {
        std::vector<double> times;
        attributes[i].GetTimeSamples(&times);
        // Read, clear, and then re-write all samples afterward
        // to avoid collisions from writing new samples over old.
        std::map<double, VtValue> scaledSamples;
        for (double currTime: times) {
            const double scaledTime = currTime * scale;
            VtValue attrVal;
            attributes[i].Get(&attrVal, UsdTimeCode(currTime));
            scaledSamples[scaledTime] = attrVal;
            attributes[i].ClearAtTime(currTime);
        }
        for (const auto entry: scaledSamples) {
            attributes[i].Set(entry.second, UsdTimeCode(entry.first));
        }
    }

}

static void
RunInterpolationTests(const UsdPrim& prim)
{
    const UsdStageWeakPtr stage = prim.GetStage();

    // Run linear interpolation tests for each value type.
    stage->SetInterpolationType(UsdInterpolationTypeLinear);
    TF_VERIFY(stage->GetInterpolationType() == UsdInterpolationTypeLinear);

#define RUN_LINEAR_INTERPOLATION_TEST(r, unused, tup)                   \
    {                                                                   \
        typedef SDF_VALUE_TRAITS_TYPE(tup)::Type Type;                  \
        typedef SDF_VALUE_TRAITS_TYPE(tup)::ShapedType ShapedType;      \
        TestCase<Type>::TestLinearInterpolation(prim);                  \
        TestCase<ShapedType>::TestLinearInterpolation(prim);            \
    }
    BOOST_PP_SEQ_FOR_EACH(
        RUN_LINEAR_INTERPOLATION_TEST, ~, SDF_VALUE_TYPES);
#undef RUN_LINEAR_INTERPOLATION_TEST

    // Run held interpolation tests for each value type.
    stage->SetInterpolationType(UsdInterpolationTypeHeld);
    TF_VERIFY(stage->GetInterpolationType() == UsdInterpolationTypeHeld);

#define RUN_HELD_INTERPOLATION_TEST(r, unused, tup)                     \
    {                                                                   \
        typedef SDF_VALUE_TRAITS_TYPE(tup)::Type Type;                  \
        typedef SDF_VALUE_TRAITS_TYPE(tup)::ShapedType ShapedType;      \
        TestCase<Type>::TestHeldInterpolation(prim);                    \
        TestCase<ShapedType>::TestHeldInterpolation(prim);              \
    }
    BOOST_PP_SEQ_FOR_EACH(
        RUN_HELD_INTERPOLATION_TEST, ~, SDF_VALUE_TYPES);
#undef RUN_HELD_INTERPOLATION_TEST    
}

// ------------------------------------------------------------

static void
TestInterpolation(const string &layerIdent)
{
    printf("TestInterpolation... %s\n", layerIdent.c_str());

    UsdStageRefPtr stage = UsdStage::CreateInMemory(layerIdent);
    UsdPrim testPrim(stage->OverridePrim(SdfPath("/TestPrim")));
    TF_VERIFY(testPrim);

    // Add and ensure we have the expected number of test cases. If a new
    // value type is added without a corresponding TestCase<T> added,
    // this test won't compile. If a value type is removed, this
    // check will fail at runtime.
    static const size_t numTestCasesExpected = 30;
    const size_t numTestCasesAdded = AddTestCasesToPrim(testPrim);
    TF_VERIFY(numTestCasesAdded == numTestCasesExpected,
              "Expected %zd cases, got %zu.",
              numTestCasesExpected, numTestCasesAdded);

    // Uncomment to dump authored layers for debugging
    // stage->GetRootLayer()->Export("testInterpolation.usd");

    // Run all the interpolation test cases.
    RunInterpolationTests(testPrim);
}

static void
TestInterpolationWithModelClips(const string &layerIdent)
{
    printf("TestInterpolationWithModelClips... %s\n", layerIdent.c_str());

    // Add test cases to the clip stage, then scale all of the time samples
    // by half. This should result in time samples being authored at times
    // 0.0 and 1.0.
    UsdStageRefPtr clipStage = UsdStage::CreateInMemory(layerIdent);
    UsdPrim testClipPrim(clipStage->OverridePrim(SdfPath("/TestPrim")));
    TF_VERIFY(testClipPrim);
    AddTestCasesToPrim(testClipPrim);
    ScaleAttributeSampledTimes(testClipPrim, 0.5);

    // Create the primary stage and set up model clips on the test prim
    // to refer to the clip stage's root layer and scaling its time samples
    // by a factor of 2.
    UsdStageRefPtr mainStage = UsdStage::CreateInMemory(layerIdent);
    UsdPrim mainTestPrim(mainStage->OverridePrim(SdfPath("/TestPrim")));

    // Because attribute specs in model clips aren't composed, we have
    // to declare the attributes in our main stage. We do this by
    // transferring the contents of the clip layer into the main
    // stage's root layer and clearing out the time samples.
    mainStage->GetRootLayer()->TransferContent(clipStage->GetRootLayer());
    for (const auto& attr : mainTestPrim.GetAttributes()) {
        attr.Clear();
    }

    UsdClipsAPI clipPrim(mainTestPrim);

    VtArray<SdfAssetPath> clipPaths(1);
    clipPaths[0] = SdfAssetPath(clipStage->GetRootLayer()->GetIdentifier());
    
    VtArray<GfVec2d> clipActive(1);
    clipActive[0] = GfVec2d(0.0, 0.0);

    VtArray<GfVec2d> clipTimes(2);
    clipTimes[0] = GfVec2d(0.0, 0.0);
    clipTimes[1] = GfVec2d(2.0, 1.0);

    clipPrim.SetClipAssetPaths(clipPaths);
    clipPrim.SetClipPrimPath(testClipPrim.GetPath().GetString());
    clipPrim.SetClipActive(clipActive);
    clipPrim.SetClipTimes(clipTimes);

    // Uncomment to dump authored layers for debugging. Note that
    // the root layer will need to be manually fixed up to reference
    // the clip usd file.
    //
    // mainStage->GetRootLayer()->Export(
    //     "testInterpolationWithModelClips.usd");
    // clipStage->GetRootLayer()->Export(
    //     "testInterpolationWithModelClips_clip.usd");

    // Run the interpolation tests. We expect the same results because
    // we 've carefully set up the model clip and clip times to cancel
    // out the scaling. This is to verify that interpolation works with
    // model clip timing.
    RunInterpolationTests(mainTestPrim);
}

static void
TestInterpolationWithLayerOffsets(const string &layerIdent)
{
    printf("TestInterpolationWithLayerOffsets... %s\n", layerIdent.c_str());
    
    // Add test cases to the sub stage, then scale all of the time samples
    // by half. This should result in time samples being authored at times
    // 0.0 and 1.0.
    UsdStageRefPtr subStage = UsdStage::CreateInMemory(layerIdent);
    UsdPrim testSubPrim(subStage->OverridePrim(SdfPath("/TestPrim")));
    TF_VERIFY(testSubPrim);
    AddTestCasesToPrim(testSubPrim);
    ScaleAttributeSampledTimes(testSubPrim, 0.5);

    // Create the primary stage and sublayer the sub stage's root layer,
    // specifying a layer offset that scales time by 2.
    UsdStageRefPtr mainStage = UsdStage::CreateInMemory(layerIdent);
    mainStage->GetRootLayer()->GetSubLayerPaths().push_back(
        subStage->GetRootLayer()->GetIdentifier());
    mainStage->GetRootLayer()->SetSubLayerOffset(SdfLayerOffset(0.0, 0.5), 0);

    // Uncomment to dump authored layers for debugging. Note that
    // the root layer will need to be manually fixed up to sublayer
    // the sub usd file.
    //
    // mainStage->GetRootLayer()->Export(
    //     "testInterpolationWithLayerOffsets.usd");
    // subStage->GetRootLayer()->Export(
    //     "testInterpolationWithLayerOffsets_sub.usd");

    UsdPrim mainTestPrim(mainStage->GetPrimAtPath(SdfPath("/TestPrim")));
    RunInterpolationTests(mainTestPrim);
}

static void
TestInterpolationWithMismatchedShapes(const string &layerIdent)
{
    printf("TestInterpolationWithMismatchedShapes... %s\n", layerIdent.c_str());

    UsdStageRefPtr stage = UsdStage::CreateInMemory(layerIdent);
    UsdPrim prim(stage->OverridePrim(SdfPath("/TestPrim")));
    UsdAttribute attr = 
        prim.CreateAttribute(TfToken("testAttr"), SdfValueTypeNames->DoubleArray);

    attr.Set(CreateVtArray<double>(1.0, /* numElems = */ 5), UsdTimeCode(0.0));
    attr.Set(CreateVtArray<double>(3.0, /* numElems = */ 3), UsdTimeCode(2.0));

    VerifyAttributeValue(
        attr, UsdTimeCode(1.0), 
        CreateVtArray<double>(1.0, /* numElems = */ 5));
}

int main(int argc, char** argv)
{
    vector<string> idents;
    idents.push_back("lerp.usda");
    idents.push_back("lerp.usdc");

    for (const auto& ident : idents) {
        TestInterpolation(ident);
        TestInterpolationWithLayerOffsets(ident);
        TestInterpolationWithMismatchedShapes(ident);
        TestInterpolationWithModelClips(ident);
    }

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    TF_AXIOM(!Py_IsInitialized());
#endif // PXR_PYTHON_SUPPORT_ENABLED

    printf("Passed!\n");
    return EXIT_SUCCESS;
}
