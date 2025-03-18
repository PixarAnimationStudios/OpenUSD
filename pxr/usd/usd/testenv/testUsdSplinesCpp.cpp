//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/base/tf/error.h"
#include "pxr/base/tf/errorMark.h"

#include "pxr/base/ts/spline.h"
#include "pxr/base/ts/tsTest_Museum.h"
#include "pxr/base/ts/tsTest_TsEvaluator.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static
void
_TestSplineAndAttr(
    const TsSpline& spline, const UsdAttribute& attr)
{
    // Our spline in this test is all double valued, hence the following will be
    // to retrieve double values for both the raw spline and the attribute.
    double splineDouble = 0.0;
    bool splineSuccess = spline.Eval(1.0, &splineDouble);
    std::cout << "splineDouble: " << splineDouble << "\n";
    // different default value to make sure if Eval or Get fail, the test
    // doesn't result in a false positive.
    double attrDouble = 1.0;;
    bool attrSuccess = attr.Get<double>(&attrDouble, 1.0);
    std::cout << "attrDouble: " << attrDouble << "\n";
    if (!attrSuccess || !splineSuccess) {
        // If either fails, both should fail to eval / get the value.
        // This is the case when spline is empty.
        TF_AXIOM(attrSuccess == splineSuccess);
    } else {
        TF_AXIOM(splineDouble == attrDouble);
    }

    // Now lets try to get a float value from this double values spline via
    // TsSpline and UsdAttr both. As it is now, UsdAttribute will fail to
    // retrieve the value because of type mismatch, but TsSpline will be able to
    // which might also be addressed with USD-10630.
    float splineFloat = 0.0;
    splineSuccess = spline.Eval(1.0, &splineFloat);
    float attrFloat = 1.0;
    attrSuccess = attr.Get<float>(&attrFloat, 1.0);
    TF_AXIOM(splineSuccess && !attrSuccess);

    VtValue splineValue;
    splineSuccess = spline.Eval(1.0, &splineValue);
    VtValue attrValue;
    attrSuccess = attr.Get(&attrValue, 1.0);
    if (!attrSuccess || !splineSuccess) {
        // If either fails, both should fail to eval / get the value.
        // This is the case when spline is empty.
        TF_AXIOM(attrSuccess == splineSuccess);
    } else {
        TF_AXIOM(splineValue == attrValue);
    }
}

static
TsSpline
_GetTestSpline(
    const SdfValueTypeName& attrType = SdfValueTypeNames->Double)
{
    TsSpline spline;
    TsKnot knot1(attrType.GetType());
    knot1.SetTime(1);
    knot1.SetValue(8.0);
    knot1.SetNextInterpolation(TsInterpCurve);
    knot1.SetPostTanWidth(1.3);
    knot1.SetPostTanSlope(0.125);
    spline.SetKnot(knot1);
    TsKnot knot2(attrType.GetType());
    knot2.SetTime(6);
    knot2.SetValue(20.0);
    knot2.SetNextInterpolation(TsInterpCurve);
    knot2.SetPreTanWidth(1.3);
    knot2.SetPreTanSlope(-0.2);
    knot2.SetPostTanWidth(2);
    knot2.SetPostTanSlope(0.3);
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
    std::cout << "Doing serialization test for " << desc << "with type as "
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

        if (isEmpty) {
            TF_AXIOM(!attr2.HasSpline());
        } else {
            TF_AXIOM(attr2.HasSpline());
            const TsSpline spline2 = attr2.GetSpline();
            TF_AXIOM(spline == spline2);
        }
    }
}

static
void
_DoLayerOffsetTest(
    const std::string& desc,
    const SdfValueTypeName& attrType,
    bool timeValued,
    double scale)
{
    std::cout << "Doing layer offset test for " << desc << "with type as "
        << attrType.GetAsToken().GetString() << "\n";
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    SdfLayerRefPtr rootLayer = stage->GetRootLayer();
    const SdfLayerRefPtr deepLayer = SdfLayer::CreateAnonymous();

    rootLayer->SetSubLayerPaths({deepLayer->GetIdentifier()});
    rootLayer->SetSubLayerOffset(SdfLayerOffset(5.0, scale), 0);

    stage->SetEditTarget(stage->GetEditTargetForLocalLayer(deepLayer));

    TsSpline spline = _GetTestSpline();
    spline.SetTimeValued(timeValued);

    const UsdPrim prim = stage->DefinePrim(SdfPath("/MyPrim"));
    UsdAttribute attr = prim.CreateAttribute(TfToken("myAttr"), attrType);
    TF_AXIOM(!attr.HasSpline());
    TF_AXIOM(!attr.ValueMightBeTimeVarying());
    attr.SetSpline(spline);
    TF_AXIOM(attr.HasSpline());
    TF_AXIOM(attr.ValueMightBeTimeVarying());

    _TestSplineAndAttr(spline, attr);

    const SdfAttributeSpecHandle sdfAttr = deepLayer->GetAttributeAtPath(
        SdfPath("/MyPrim.myAttr"));
    VtValue value = sdfAttr->GetInfo(SdfFieldKeys->Spline);
    TF_AXIOM(value.IsHolding<TsSpline>());
    const TsSpline sdfSpline = value.UncheckedGet<TsSpline>();

    const UsdAttribute attr2 = 
        stage->GetAttributeAtPath(SdfPath("/MyPrim.myAttr"));
    const TsSpline spline2 = attr2.GetSpline();
    TF_AXIOM(spline2 == spline);
    _TestSplineAndAttr(spline2, attr2);

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
    for (const std::string exhibit : {"TwoKnotBezier", "ComplexParams"}) {
        const TsSpline spline = 
            TsTest_TsEvaluator().SplineDataToSpline(
                TsTest_Museum::GetDataByName(exhibit));
        _DoSerializationTest("Museum." + exhibit, spline);
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
        TsSpline spline = _GetTestSpline();
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
        knot1.SetCurveType(TsCurveTypeHermite);
        knot1.SetTime(1);
        knot1.SetValue(8.0);
        knot1.SetNextInterpolation(TsInterpCurve);
        knot1.SetPostTanSlope(4.0);
        spline.SetKnot(knot1);

        TsKnot knot2;
        knot2.SetCurveType(TsCurveTypeHermite);
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
        const TsSpline spline = _GetTestSpline(SdfValueTypeNames->Float);
        _DoSerializationTest("ValueTypes.Float", spline, 
                             SdfValueTypeNames->Float);
    }
    {
        const TsSpline spline = _GetTestSpline(SdfValueTypeNames->Half);
        _DoSerializationTest("ValueTypes.Half", spline, 
                             SdfValueTypeNames->Half);
    }
    {
        // Note that TimeCode spline have Double attrType and is marked as
        // TimeValues
        TsSpline spline = _GetTestSpline(SdfValueTypeNames->Double);
        spline.SetTimeValued(true);
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
    _DoSerializationTest("Loops.Invalid", spline);
}

static
void
TestLayerOffsets()
{
    _DoLayerOffsetTest(
        "test_LayerOffsets", SdfValueTypeNames->Double, false, 2.0);
}

static
void
TestLayerOffsetsTimeCode()
{
    _DoLayerOffsetTest(
        "test_LayerOffsets_TimeCode", SdfValueTypeNames->TimeCode, true, 2.0);
}

static
void
TestInvalidType()
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory();
    const UsdPrim prim = stage->DefinePrim(SdfPath("/MyPrim"));
    UsdAttribute attr = prim.CreateAttribute(
        TfToken("myAttr"), SdfValueTypeNames->String);
    const TsSpline spline = _GetTestSpline();

    TfErrorMark m;
    TF_AXIOM(!attr.HasSpline());
    attr.SetSpline(spline);

    // A coding error should have been posted as String value splines are not
    // allowed. Only double, float or GfHalf!
    TF_AXIOM(!m.IsClean());
    TF_AXIOM(!attr.HasSpline());
    TF_AXIOM(!attr.ValueMightBeTimeVarying());
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
    return 0;
}
