//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/schema.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/attributeQuery.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/tf/error.h"
#include "pxr/base/tf/errorMark.h"

#include "pxr/base/ts/spline.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

template <typename T>
static
void
_TestSplineAndAttr(
    const TsSpline& spline, const UsdAttribute& attr)
{
    T splineValue;
    bool splineSuccess = spline.Eval(1.0, &splineValue);
    std::cout << "spline value: " << splineValue << "\n";
    // different default value to make sure if Eval or Get fail, the test
    // doesn't result in a false positive.
    T attrValue = 1.0;
    bool attrSuccess = attr.Get<T>(&attrValue, 1.0);
    std::cout << "attr value: " << attrValue << "\n";
    if (!attrSuccess || !splineSuccess) {
        // If either fails, both should fail to eval / get the value.
        // This is the case when spline is empty.
        TF_AXIOM(attrSuccess == splineSuccess);
    } else {
        TF_AXIOM(splineValue == attrValue);
    }
    // Lets also try UsdAttributeQuery matching the spline value and the attr
    // value.
    UsdAttributeQuery attrQuery(attr);
    TF_AXIOM(attrQuery.GetSpline() == spline);
    VtValue queryValue;
    bool querySuccess = attrQuery.Get(&queryValue, 1.0);
    if (!querySuccess || !splineSuccess) {
        // If either fails, both should fail to eval / get the value.
        // This is the case when spline is empty.
        TF_AXIOM(querySuccess == splineSuccess);
    } else {
        TF_AXIOM(queryValue == splineValue);
    }

    // Do a pre value test.
    splineValue = 0.0;
    splineSuccess = spline.EvalPreValue(0.0, &splineValue);
    attrValue = 1.0;
    attrSuccess = attr.Get<T>(&attrValue, UsdTimeCode::PreTime(1.0));
    if (!attrSuccess || !splineSuccess) {
        // If either fails, both should fail to eval / get the value.
        // This is the case when spline is empty.
        TF_AXIOM(attrSuccess == splineSuccess);
    } else {
        TF_AXIOM(splineValue == attrValue);
    }
}

template <typename T>
static
TsSpline
_GetTestSpline() {
    TsSpline spline;
    TsKnot knot1(Ts_GetType<T>());
    knot1.SetTime(1);
    knot1.SetValue(T(8.0));
    knot1.SetPreValue(T(6.0));
    knot1.SetNextInterpolation(TsInterpCurve);
    knot1.SetPostTanWidth(1.3);
    knot1.SetPostTanSlope(T(0.125));
    spline.SetKnot(knot1);
    TsKnot knot2(Ts_GetType<T>());
    knot2.SetTime(6);
    knot2.SetValue(T(20.0));
    knot2.SetPreValue(T(10.0));
    knot2.SetNextInterpolation(TsInterpCurve);
    knot2.SetPreTanWidth(1.3);
    knot2.SetPreTanSlope(T(-0.2));
    knot2.SetPostTanWidth(2);
    knot2.SetPostTanSlope(T(0.3));
    spline.SetKnot(knot2);
    return spline;
}

static
void
_DoSerializationTest(
    const std::string& desc,
    const TsSpline& spline,
    const SdfValueTypeName& attrType = SdfValueTypeNames->Double,
    bool isEmpty = false)
{
    std::cout << "Doing serialization test for " << desc << " with type as "
        << attrType.GetAsToken().GetString() << "\n";
    for (const std::string format : {"usda", "usdc"}) {
        const std::string filename1 = TfStringPrintf(
            "test_Serialization_%s.%s", desc.c_str(), format.c_str());
        const std::string filename2 = TfStringPrintf(
            "%s.copy.%s", filename1.c_str(), format.c_str());
        UsdStageRefPtr stage = UsdStage::CreateNew(filename1);
        const UsdPrim prim = stage->DefinePrim(SdfPath("/MyPrim"));
        UsdAttribute attr = prim.CreateAttribute(TfToken("myAttr"), attrType);
        TF_AXIOM(!attr.HasSpline());
        TF_AXIOM(!attr.ValueMightBeTimeVarying());
        attr.SetSpline(spline);
        TF_AXIOM(attr.HasSpline());
        // Having a spline makes this attr might be time varying.
        TF_AXIOM(attr.ValueMightBeTimeVarying());

        stage->Save();
        stage->GetRootLayer()->Export(filename2);

        UsdStageRefPtr stage2 = UsdStage::Open(filename2);
        const UsdAttribute attr2 = stage2->GetAttributeAtPath(
            SdfPath("/MyPrim.myAttr"));
        TF_AXIOM(attr2.HasSpline());

        const TsSpline spline2 = attr2.GetSpline();
        TF_AXIOM(spline2.IsEmpty() == isEmpty);
        TF_AXIOM(spline == spline2);
    }
}

template <typename T>
static
void
_DoLayerOffsetTest(
    const std::string& desc,
    double scale)
{
    const TfType attrType = Ts_GetType<T>();
    const SdfValueTypeName sdfType =
        SdfSchema::GetInstance().FindType(attrType);
    std::cout << "Doing layer offset test for " << desc << "with type as "
        << attrType.GetTypeName() << "\n";
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    SdfLayerRefPtr rootLayer = stage->GetRootLayer();
    const SdfLayerRefPtr deepLayer = SdfLayer::CreateAnonymous();

    rootLayer->SetSubLayerPaths({deepLayer->GetIdentifier()});
    rootLayer->SetSubLayerOffset(SdfLayerOffset(5.0, scale), 0);

    stage->SetEditTarget(stage->GetEditTargetForLocalLayer(deepLayer));

    TsSpline spline = _GetTestSpline<T>();

    const UsdPrim prim = stage->DefinePrim(SdfPath("/MyPrim"));
    UsdAttribute attr = prim.CreateAttribute(TfToken("myAttr"), sdfType);
    TF_AXIOM(!attr.HasSpline());
    TF_AXIOM(!attr.ValueMightBeTimeVarying());
    attr.SetSpline(spline);
    TF_AXIOM(attr.HasSpline());
    TF_AXIOM(attr.ValueMightBeTimeVarying());

    _TestSplineAndAttr<T>(spline, attr);

    const SdfAttributeSpecHandle sdfAttr = deepLayer->GetAttributeAtPath(
        SdfPath("/MyPrim.myAttr"));
    VtValue value = sdfAttr->GetInfo(SdfFieldKeys->Spline);
    TF_AXIOM(value.IsHolding<TsSpline>());
    const TsSpline sdfSpline = value.UncheckedGet<TsSpline>();

    const UsdAttribute attr2 = 
        stage->GetAttributeAtPath(SdfPath("/MyPrim.myAttr"));
    const TsSpline spline2 = attr2.GetSpline();
    TF_AXIOM(spline2 == spline);
    _TestSplineAndAttr<T>(spline2, attr2);

    attr2.GetMetadata(SdfFieldKeys->Spline, &value);
    TF_AXIOM(value.IsHolding<TsSpline>());
    const TsSpline spline3 = value.UncheckedGet<TsSpline>();

    TF_AXIOM(spline3 == spline);
    TF_AXIOM(sdfSpline != spline);
}

static
void
TestSerializationEmpty()
{
    const SdfValueTypeName attrType = SdfValueTypeNames->Double;
    // Need to specify a Spline supported value type for the empty spline to be
    // valid.
    _DoSerializationTest("Empty", TsSpline(attrType.GetType()), attrType, true);
}

static
void
TestSerializationMuseum()
{
    // These test cases were adapted from the similarly-named examples in
    // tsTest_Museum.cpp.

    const TsSpline twoKnotBezier = []() {
        TsSpline spline;
        spline.SetCurveType(TsCurveTypeBezier);
        
        TsKnot knot1;
        knot1.SetTime(1.0);
        knot1.SetNextInterpolation(TsInterpCurve);
        knot1.SetValue(1.0);
        knot1.SetPostTanSlope(1.0);
        knot1.SetPostTanWidth(0.5);

        TsKnot knot2;
        knot2.SetTime(5.0);
        knot2.SetNextInterpolation(TsInterpCurve);
        knot2.SetValue(2.0);
        knot2.SetPreTanSlope(0.0);
        knot2.SetPreTanWidth(0.5);

        spline.SetKnot(knot1);
        spline.SetKnot(knot2);

        return spline;
    }();

    const TsSpline complexParams = []() {
        TsSpline spline;

        spline.SetPreExtrapolation(TsExtrapolation(TsExtrapLinear));
        TsExtrapolation postExtrap(TsExtrapSloped);
        postExtrap.slope = 0.57;
        spline.SetPostExtrapolation(postExtrap);

        TsKnot knot1;
        knot1.SetTime(7);
        knot1.SetNextInterpolation(TsInterpHeld);
        knot1.SetPreValue(5.5);
        knot1.SetValue(7.21);

        TsKnot knot2;
        knot2.SetTime(15);
        knot2.SetNextInterpolation(TsInterpCurve);
        knot2.SetValue(8.18);
        knot2.SetPostTanSlope(1.17);
        knot2.SetPostTanWidth(2.49);

        TsKnot knot3;
        knot3.SetTime(20);
        knot3.SetNextInterpolation(TsInterpCurve);
        knot3.SetValue(14.72);
        knot3.SetPreTanSlope(-1.4);
        knot3.SetPreTanWidth(3.77);
        knot3.SetPostTanSlope(-1.4);
        knot3.SetPostTanWidth(1.1);

        spline.SetKnot(knot1);
        spline.SetKnot(knot2);
        spline.SetKnot(knot3);

        return spline;
    }();

    for (const auto& [exhibit, spline] : 
             { std::make_pair("TwoKnotBezier", twoKnotBezier),
               std::make_pair("ComplexParams", complexParams) }) {
        _DoSerializationTest("Museum." + std::string(exhibit), spline);
    }
}

static
void
TestSerializationComplex()
{
    // Bezier, custom data dictionary.
    {
        TsSpline spline;
        TsKnot knot1;
        knot1.SetTime(-14.7);
        knot1.SetValue(11.30752);
        knot1.SetNextInterpolation(TsInterpCurve);
        knot1.SetPostTanWidth(1.8);
        knot1.SetPostTanSlope(1.4);

        VtDictionary customData;
        customData["a"] = VtValue("yes");
        customData["b"] = VtValue(4);
        VtDictionary innerDict;
        innerDict["d"] = VtValue("ugh");
        customData["c"] = VtValue(innerDict);
        knot1.SetCustomData(customData);
        spline.SetKnot(knot1);

        TsKnot knot2;
        knot2.SetTime(-1.2);
        knot2.SetValue(22.994037);
        knot2.SetNextInterpolation(TsInterpCurve);
        knot2.SetPreTanWidth(1.6);
        knot2.SetPreTanSlope(0.7);
        knot2.SetPostTanWidth(2.3);
        knot2.SetPostTanSlope(5.7);
        spline.SetKnot(knot2);
        _DoSerializationTest("Complex.1", spline);
    }

    // Bezier
    {
        TsSpline spline = _GetTestSpline<double>();
        TsKnot knot1;
        knot1.SetTime(40);
        knot1.SetValue(-44.0);
        knot1.SetNextInterpolation(TsInterpCurve);
        knot1.SetPreTanWidth(1);
        knot1.SetPreTanSlope(0.0);
        knot1.SetPostTanWidth(7.3);
        knot1.SetPostTanSlope(0.0);
        spline.SetKnot(knot1);
        _DoSerializationTest("Complex.2", spline);
    }

    // Hermite
    {
        TsSpline spline = TsSpline();
        spline.SetCurveType(TsCurveTypeHermite);
        TsKnot knot1;
        knot1.SetTime(1);
        knot1.SetValue(8.0);
        knot1.SetNextInterpolation(TsInterpCurve);
        knot1.SetPostTanSlope(4.0);
        spline.SetKnot(knot1);

        TsKnot knot2;
        knot2.SetTime(6);
        knot2.SetValue(20.0);
        knot2.SetNextInterpolation(TsInterpCurve);
        knot2.SetPreTanSlope(-0.7);
        knot2.SetPostTanSlope(1.0);
        spline.SetKnot(knot2);
        _DoSerializationTest("Complex.3", spline);
    }
}

static
void
TestSerializationValueTypes()
{
    // Test serialization of splines with different value types.
    //
    {
        const TsSpline spline = _GetTestSpline<float>();
        _DoSerializationTest("ValueTypes.Float", spline, 
                             SdfValueTypeNames->Float);
    }
    {
        const TsSpline spline = _GetTestSpline<GfHalf>();
        _DoSerializationTest("ValueTypes.Half", spline, 
                             SdfValueTypeNames->Half);
    }
    {
        TsSpline spline = _GetTestSpline<GfTimeCode>();
        _DoSerializationTest("ValueTypes.TimeCode", spline, 
                             SdfValueTypeNames->TimeCode);
    }
}

static
void
TestSerializationLoops()
{
    // Valid loop params.
    TsSpline spline;
    TsKnot knot1;
    knot1.SetTime(1);
    knot1.SetValue(5.0);
    knot1.SetNextInterpolation(TsInterpCurve);
    knot1.SetPreTanWidth(1);
    knot1.SetPreTanSlope(1.0);
    knot1.SetPostTanWidth(1);
    knot1.SetPostTanSlope(1.0);
    spline.SetKnot(knot1);

    // Do not generate compiler warnings for the deprecated inner looping
    // methods
    ARCH_PRAGMA_PUSH
    ARCH_PRAGMA_DEPRECATED

    TsLoopParams lp;
    lp.protoStart = 1;
    lp.protoEnd = 10;
    lp.numPostLoops = 1;
    spline.SetInnerLoopParams(lp);
    _DoSerializationTest("Loops.Valid", spline);

    // In this version, there is no knot at the prototype start time, so the
    // loop params are invalid.  They should be serialized and read back
    // anyway.
    lp.protoStart = 2;
    spline.SetInnerLoopParams(lp);
    ARCH_PRAGMA_POP

    _DoSerializationTest("Loops.Invalid", spline);
}

static
void
TestLayerOffsets()
{
    _DoLayerOffsetTest<double>(
        "test_LayerOffsets", 2.0);
}

static
void
TestLayerOffsetsTimeCode()
{
    _DoLayerOffsetTest<GfTimeCode>(
        "test_LayerOffsets_TimeCode", 2.0);
}

static
void
TestInvalidType()
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    const UsdPrim prim = stage->DefinePrim(SdfPath("/MyPrim"));
    UsdAttribute attr = prim.CreateAttribute(
        TfToken("myAttr"), SdfValueTypeNames->String);
    const TsSpline spline = _GetTestSpline<double>();

    TfErrorMark m;
    TF_AXIOM(!attr.HasSpline());
    attr.SetSpline(spline);

    // A coding error should have been posted as String value splines are not
    // allowed. Only double, float or GfHalf!
    TF_AXIOM(!m.IsClean());
    TF_AXIOM(!attr.HasSpline());
    TF_AXIOM(!attr.ValueMightBeTimeVarying());
}

static
void
TestClobbered()
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    const UsdPrim prim = stage->DefinePrim(SdfPath("/MyPrim"));
    UsdAttribute attr = prim.CreateAttribute(
        TfToken("myAttr"), SdfValueTypeNames->Double);
    const TsSpline spline = _GetTestSpline<double>();

    attr.SetSpline(spline);
    TF_AXIOM(attr.HasSpline());
    TF_AXIOM(attr.GetSpline() == spline);
    attr.Set(100.0, 1);

    TF_AXIOM(!attr.HasSpline());
    TF_AXIOM(attr.GetSpline().IsEmpty());
    attr.SetSpline(spline);
    TF_AXIOM(!attr.HasSpline());
    TF_AXIOM(attr.GetSpline().IsEmpty());

    double value;
    attr.Get(&value, 1);
    TF_AXIOM(value == 100.0);
}

static
void
TestWeakerSplineOpinion()
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    SdfLayerRefPtr rootLayer = stage->GetRootLayer();
    const SdfLayerRefPtr subLayer = SdfLayer::CreateAnonymous();
    rootLayer->SetSubLayerPaths({subLayer->GetIdentifier()});

    // Set spline in the subLayer
    stage->SetEditTarget(stage->GetEditTargetForLocalLayer(subLayer));
    const UsdPrim prim = stage->DefinePrim(SdfPath("/MyPrim"));
    UsdAttribute attr = prim.CreateAttribute(TfToken("myAttr"),
                                             SdfValueTypeNames->Double);
    TsSpline spline = _GetTestSpline<double>();
    attr.SetSpline(spline);
    TF_AXIOM(attr.HasSpline());
    TF_AXIOM(attr.GetSpline() == spline);

    // Set stronger time samples in the rootLayer
    stage->SetEditTarget(stage->GetEditTargetForLocalLayer(rootLayer));
    const UsdPrim rootPrim = stage->DefinePrim(SdfPath("/MyPrim"));
    UsdAttribute rootAttr = prim.CreateAttribute(TfToken("myAttr"),
                                                 SdfValueTypeNames->Double);
    TF_AXIOM(attr.HasSpline());
    TF_AXIOM(attr.GetSpline() == spline);
    rootAttr.Set(100.0, 1);
    TF_AXIOM(!attr.HasSpline());
    TF_AXIOM(attr.GetSpline().IsEmpty());

    double value;
    attr.Get(&value, 1);
    TF_AXIOM(value == 100.0);
}

int main()
{
    TestSerializationEmpty();
    TestSerializationMuseum();
    TestSerializationComplex();
    TestSerializationValueTypes();
    TestSerializationLoops();
    TestLayerOffsets();
    TestLayerOffsetsTimeCode();
    TestInvalidType();
    TestClobbered();
    TestWeakerSplineOpinion();
    return 0;
}
