//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"

#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/attributeQuery.h"
#include "pxr/usd/usd/editContext.h"
#include "pxr/usd/usd/prim.h"
#include "pxr/usd/usd/stage.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/errorMark.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/array.h"
#include "pxr/base/vt/arrayEdit.h"
#include "pxr/base/vt/arrayEditBuilder.h"

PXR_NAMESPACE_USING_DIRECTIVE

static void
TestBasics()
{
    auto stage = UsdStage::CreateInMemory();

    const UsdPrim prim = stage->DefinePrim(SdfPath("/TestBasics"));

    const UsdAttribute attr =
        prim.CreateAttribute(TfToken("attr"), SdfValueTypeNames->IntArray);

    VtIntArray iarray { 3, 2, 1 };
    TF_AXIOM(!attr.Get(&iarray));

    TF_AXIOM(attr.Set(iarray));

    iarray.clear();
    TF_AXIOM(attr.Get(&iarray));
    TF_AXIOM((iarray == VtIntArray { 3, 2, 1 }));

    // Check basic info.
    TF_AXIOM(attr.HasAuthoredValueOpinion());
    TF_AXIOM(attr.HasAuthoredValue());
    TF_AXIOM(!attr.HasFallbackValue());
    TF_AXIOM(attr.GetNumTimeSamples() == 0);
    TF_AXIOM(!attr.ValueMightBeTimeVarying());

    UsdAttributeQuery attrQ { attr };
    TF_AXIOM(attrQ.HasAuthoredValueOpinion());
    TF_AXIOM(attrQ.HasAuthoredValue());
    TF_AXIOM(!attrQ.HasFallbackValue());
    TF_AXIOM(attrQ.GetNumTimeSamples() == 0);
    TF_AXIOM(!attrQ.ValueMightBeTimeVarying());

    VtArrayEdit zeroNine = VtIntArrayEditBuilder()
        .Prepend(0)
        .Append(9)
        .FinalizeAndReset();

    // Author the zeroNine edit to the session layer.
    {
        UsdEditContext toSessionLayer(stage, stage->GetSessionLayer());
        attr.Set(zeroNine);
    }

    // Now the value should be the edited value.
    iarray.clear();
    TF_AXIOM(attr.Get(&iarray));
    TF_AXIOM((iarray == VtIntArray { 0, 3, 2, 1, 9 }));

    attrQ = UsdAttributeQuery { attr };
    TF_AXIOM(attrQ.Get(&iarray));
    TF_AXIOM((iarray == VtIntArray { 0, 3, 2, 1, 9 }));

    // Check basic info.
    TF_AXIOM(attr.HasAuthoredValueOpinion());
    TF_AXIOM(attr.HasAuthoredValue());
    TF_AXIOM(!attr.HasFallbackValue());
    TF_AXIOM(attr.GetNumTimeSamples() == 0);
    TF_AXIOM(!attr.ValueMightBeTimeVarying());

    TF_AXIOM(attrQ.HasAuthoredValueOpinion());
    TF_AXIOM(attrQ.HasAuthoredValue());
    TF_AXIOM(!attrQ.HasFallbackValue());
    TF_AXIOM(attrQ.GetNumTimeSamples() == 0);
    TF_AXIOM(!attrQ.ValueMightBeTimeVarying());
    
    // Check ResolveInfo at default time, which should list both sources.
    UsdResolveInfo ri = attr.GetResolveInfo(UsdTimeCode::Default());
    TF_AXIOM(ri.GetSource() == UsdResolveInfoSourceDefault);
    TF_AXIOM(!ri.ValueSourceMightBeTimeVarying());
    TF_AXIOM(ri.HasNextWeakerInfo());
    const UsdResolveInfo &weaker = *ri.GetNextWeakerInfo();
    TF_AXIOM(weaker.GetSource() == UsdResolveInfoSourceDefault);
    TF_AXIOM(!weaker.ValueSourceMightBeTimeVarying());
    TF_AXIOM(!weaker.HasNextWeakerInfo());

    // Author session layer samples, which should hide the session layer default
    // and compose over the weaker default.
    {
        VtArrayEdit threeThree = VtIntArrayEditBuilder()
            .Prepend(3)
            .Append(3)
            .FinalizeAndReset();
        VtArrayEdit sixSeven = VtIntArrayEditBuilder()
            .Prepend(6)
            .Append(7)
            .FinalizeAndReset();

        UsdEditContext toSessionLayer(stage, stage->GetSessionLayer());
        attr.Set(threeThree, 3.0);
        attr.Set(sixSeven, 6.0);
    }

    //          Time: <default>   |       3.0          6.0
    // Session layer:             |   threeThree    sixSeven
    //    Root layer: {3, 2, 1}   |

    // Now the values should be the edited values.  Note that the session-layer
    // samples hide the session-layer default.
    iarray.clear();
    TF_AXIOM(attr.Get(&iarray, 0.0));
    TF_AXIOM((iarray == VtIntArray { 3, 3, 2, 1, 3 }));
    iarray.clear();
    TF_AXIOM(attr.Get(&iarray, 3.0));
    TF_AXIOM((iarray == VtIntArray { 3, 3, 2, 1, 3 }));
    iarray.clear();
    TF_AXIOM(attr.Get(&iarray, 5.0));
    TF_AXIOM((iarray == VtIntArray { 3, 3, 2, 1, 3 }));
    iarray.clear();
    TF_AXIOM(attr.Get(&iarray, 6.0));
    TF_AXIOM((iarray == VtIntArray { 6, 3, 2, 1, 7 }));
    iarray.clear();
    TF_AXIOM(attr.Get(&iarray, 7.0));
    TF_AXIOM((iarray == VtIntArray { 6, 3, 2, 1, 7 }));

    // Get at default should ignore the samples.
    TF_AXIOM(attr.Get(&iarray));
    TF_AXIOM((iarray == VtIntArray { 0, 3, 2, 1, 9 }));

    attrQ = UsdAttributeQuery { attr };
    iarray.clear();
    TF_AXIOM(attrQ.Get(&iarray, 0.0));
    TF_AXIOM((iarray == VtIntArray { 3, 3, 2, 1, 3 }));
    iarray.clear();
    TF_AXIOM(attrQ.Get(&iarray, 3.0));
    TF_AXIOM((iarray == VtIntArray { 3, 3, 2, 1, 3 }));
    iarray.clear();
    TF_AXIOM(attrQ.Get(&iarray, 5.0));
    TF_AXIOM((iarray == VtIntArray { 3, 3, 2, 1, 3 }));
    iarray.clear();
    TF_AXIOM(attrQ.Get(&iarray, 6.0));
    TF_AXIOM((iarray == VtIntArray { 6, 3, 2, 1, 7 }));
    iarray.clear();
    TF_AXIOM(attrQ.Get(&iarray, 7.0));
    TF_AXIOM((iarray == VtIntArray { 6, 3, 2, 1, 7 }));

    // Get at default should ignore the samples.
    TF_AXIOM(attrQ.Get(&iarray));
    TF_AXIOM((iarray == VtIntArray { 0, 3, 2, 1, 9 }));
    

    // Check that bracketing samples works as expected.
    {
        bool has;
        double low, up;
        TF_AXIOM(attr.GetBracketingTimeSamples(0.0, &low, &up, &has));
        TF_AXIOM(low == 3.0);
        TF_AXIOM(up == 3.0);
        TF_AXIOM(has);
        TF_AXIOM(attr.GetBracketingTimeSamples(3.0, &low, &up, &has));
        TF_AXIOM(low == 3.0);
        TF_AXIOM(up == 3.0);
        TF_AXIOM(has);
        TF_AXIOM(attr.GetBracketingTimeSamples(4.0, &low, &up, &has));
        TF_AXIOM(low == 3.0);
        TF_AXIOM(up == 6.0);
        TF_AXIOM(has);
        TF_AXIOM(attr.GetBracketingTimeSamples(6.0, &low, &up, &has));
        TF_AXIOM(low == 6.0);
        TF_AXIOM(up == 6.0);
        TF_AXIOM(has);
        TF_AXIOM(attr.GetBracketingTimeSamples(7.0, &low, &up, &has));
        TF_AXIOM(low == 6.0);
        TF_AXIOM(up == 6.0);

        TF_AXIOM(has);
        TF_AXIOM(attrQ.GetBracketingTimeSamples(0.0, &low, &up, &has));
        TF_AXIOM(low == 3.0);
        TF_AXIOM(up == 3.0);
        TF_AXIOM(has);
        TF_AXIOM(attrQ.GetBracketingTimeSamples(3.0, &low, &up, &has));
        TF_AXIOM(low == 3.0);
        TF_AXIOM(up == 3.0);
        TF_AXIOM(has);
        TF_AXIOM(attrQ.GetBracketingTimeSamples(4.0, &low, &up, &has));
        TF_AXIOM(low == 3.0);
        TF_AXIOM(up == 6.0);
        TF_AXIOM(has);
        TF_AXIOM(attrQ.GetBracketingTimeSamples(6.0, &low, &up, &has));
        TF_AXIOM(low == 6.0);
        TF_AXIOM(up == 6.0);
        TF_AXIOM(has);
        TF_AXIOM(attrQ.GetBracketingTimeSamples(7.0, &low, &up, &has));
        TF_AXIOM(low == 6.0);
        TF_AXIOM(up == 6.0);
        TF_AXIOM(has);
    }
    
    // Author root-layer samples, which should hide the root layer default and
    // compose under the session-layer samples.
    {
        VtArrayEdit minusOneOne = VtIntArrayEditBuilder()
            .Prepend(-1)
            .Append(-1)
            .FinalizeAndReset();
        VtArrayEdit minusFiveFive = VtIntArrayEditBuilder()
            .Prepend(-5)
            .Append(-5)
            .FinalizeAndReset();
        VtArrayEdit minusNineNine = VtIntArrayEditBuilder()
            .Prepend(-9)
            .Append(-9)
            .FinalizeAndReset();

        UsdEditContext toRootLayer(stage, stage->GetRootLayer());
        attr.Set(minusOneOne, 1.0);
        attr.Set(minusFiveFive, 5.0);
        attr.Set(minusNineNine, 9.0);
    }

    //          Time:    1.0      3.0      5.0       6.0      9.0
    // Session layer:         threeThree          sixSeven
    //    Root layer: minusOneOne    minusFiveFive      minusNineNine

    // A single spec with timeSamples hides its own default, so defaults no
    // longer apply (unless we evaluate at default time), and if the result of
    // value resolution is an array edit we compose it over an empty array.

    iarray.clear();
    TF_AXIOM(attr.Get(&iarray, 0.0));
    TF_AXIOM((iarray == VtIntArray { 3, -1, -1, 3 }));
    iarray.clear();
    TF_AXIOM(attr.Get(&iarray, 3.0));
    TF_AXIOM((iarray == VtIntArray { 3, -1, -1, 3 }));
    iarray.clear();
    TF_AXIOM(attr.Get(&iarray, 4.0));
    TF_AXIOM((iarray == VtIntArray { 3, -1, -1, 3 }));
    iarray.clear();
    TF_AXIOM(attr.Get(&iarray, 5.0));
    TF_AXIOM((iarray == VtIntArray { 3, -5, -5, 3 }));
    iarray.clear();
    TF_AXIOM(attr.Get(&iarray, 6.0));
    TF_AXIOM((iarray == VtIntArray { 6, -5, -5, 7 }));
    iarray.clear();
    TF_AXIOM(attr.Get(&iarray, 7.0));
    TF_AXIOM((iarray == VtIntArray { 6, -5, -5, 7 }));
    iarray.clear();
    TF_AXIOM(attr.Get(&iarray, 9.0));
    TF_AXIOM((iarray == VtIntArray { 6, -9, -9, 7 }));
    iarray.clear();
    TF_AXIOM(attr.Get(&iarray, 10.0));
    TF_AXIOM((iarray == VtIntArray { 6, -9, -9, 7 }));
    
    // Get at default should continue to ignore the samples.
    TF_AXIOM(attr.Get(&iarray));
    TF_AXIOM((iarray == VtIntArray { 0, 3, 2, 1, 9 }));

    attrQ = UsdAttributeQuery { attr };
    
    iarray.clear();
    TF_AXIOM(attrQ.Get(&iarray, 0.0));
    TF_AXIOM((iarray == VtIntArray { 3, -1, -1, 3 }));
    iarray.clear();
    TF_AXIOM(attrQ.Get(&iarray, 3.0));
    TF_AXIOM((iarray == VtIntArray { 3, -1, -1, 3 }));
    iarray.clear();
    TF_AXIOM(attrQ.Get(&iarray, 4.0));
    TF_AXIOM((iarray == VtIntArray { 3, -1, -1, 3 }));
    iarray.clear();
    TF_AXIOM(attrQ.Get(&iarray, 5.0));
    TF_AXIOM((iarray == VtIntArray { 3, -5, -5, 3 }));
    iarray.clear();
    TF_AXIOM(attrQ.Get(&iarray, 6.0));
    TF_AXIOM((iarray == VtIntArray { 6, -5, -5, 7 }));
    iarray.clear();
    TF_AXIOM(attrQ.Get(&iarray, 7.0));
    TF_AXIOM((iarray == VtIntArray { 6, -5, -5, 7 }));
    iarray.clear();
    TF_AXIOM(attrQ.Get(&iarray, 9.0));
    TF_AXIOM((iarray == VtIntArray { 6, -9, -9, 7 }));
    iarray.clear();
    TF_AXIOM(attrQ.Get(&iarray, 10.0));
    TF_AXIOM((iarray == VtIntArray { 6, -9, -9, 7 }));
    
    // Get at default should continue to ignore the samples.
    TF_AXIOM(attrQ.Get(&iarray));
    TF_AXIOM((iarray == VtIntArray { 0, 3, 2, 1, 9 }));
}

static void
TestInterpolation()
{
    auto stage = UsdStage::CreateInMemory();

    const UsdPrim prim = stage->DefinePrim(SdfPath("/TestInterpolation"));
    
    const UsdAttribute attr =
        prim.CreateAttribute(TfToken("attr"), SdfValueTypeNames->FloatArray);

    // Array-valued attributes of interpolating types will interpolate between
    // samples if they have equal sizes.
    VtFloatArrayEditBuilder builder;

    VtArrayEdit size4 = VtFloatArrayEditBuilder()
        .SetSize(4)
        .FinalizeAndReset();

    VtArrayEdit eight1 = VtFloatArrayEditBuilder()
        .Write(8.f, 1)
        .FinalizeAndReset();

    VtFloatArray cheer { 2.f, 4.f, 6.f, 8.f };

    // Write two samples in the root layer.
    attr.Set(size4, 1.0);
    attr.Set(cheer, 3.0);

    // Should get linear interpolation from all zeros to 2,4,6,8.
    VtFloatArray fa;
    TF_AXIOM((attr.Get(&fa, 0.0) && fa == VtFloatArray { 0.f, 0.f, 0.f, 0.f }));
    TF_AXIOM((attr.Get(&fa, 1.0) && fa == VtFloatArray { 0.f, 0.f, 0.f, 0.f }));
    TF_AXIOM((attr.Get(&fa, 2.0) && fa == VtFloatArray { 1.f, 2.f, 3.f, 4.f }));
    TF_AXIOM((attr.Get(&fa, 3.0) && fa == VtFloatArray { 2.f, 4.f, 6.f, 8.f }));
    TF_AXIOM((attr.Get(&fa, 4.0) && fa == VtFloatArray { 2.f, 4.f, 6.f, 8.f }));
    
    // Write the eight1 in the session layer.
    {
        UsdEditContext toSessionLayer(stage, stage->GetSessionLayer());
        attr.Set(eight1, 2.0);
    }

    //    time:   0     1     2     3     4
    // session:             eight1
    //    root:       size4       cheer

    // Now we should see 8s at index 1 throughout, and interpolation between
    // time 2 and 3.
    TF_AXIOM((attr.Get(&fa, 0.0) && fa == VtFloatArray { 0.f, 8.f, 0.f, 0.f }));
    TF_AXIOM((attr.Get(&fa, 1.0) && fa == VtFloatArray { 0.f, 8.f, 0.f, 0.f }));
    TF_AXIOM((attr.Get(&fa, 2.0) && fa == VtFloatArray { 0.f, 8.f, 0.f, 0.f }));
    TF_AXIOM((attr.Get(&fa, 2.5) && fa == VtFloatArray { 1.f, 8.f, 3.f, 4.f }));
    TF_AXIOM((attr.Get(&fa, 3.0) && fa == VtFloatArray { 2.f, 8.f, 6.f, 8.f }));
    TF_AXIOM((attr.Get(&fa, 4.0) && fa == VtFloatArray { 2.f, 8.f, 6.f, 8.f }));
}

int main()
{
    TestBasics();
    TestInterpolation();

    printf("SUCCEEDED\n");
    return 0;
}
