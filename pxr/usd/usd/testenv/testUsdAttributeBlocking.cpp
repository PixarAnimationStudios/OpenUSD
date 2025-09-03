//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/propertySpec.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/attributeQuery.h"
#include "pxr/usd/usd/references.h"

#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <tuple>
using std::string;
using std::vector;
using std::tuple;

PXR_NAMESPACE_USING_DIRECTIVE

constexpr size_t TIME_SAMPLE_BEGIN = 101.0;
constexpr size_t TIME_SAMPLE_END = 120.0;
constexpr double DEFAULT_VALUE = 4.0;

tuple<UsdStageRefPtr, UsdAttribute, UsdAttribute, UsdAttribute>
_GenerateStage(const string& fmt) {
    const TfToken defAttrTk = TfToken("size");
    const TfToken sampleAttrTk = TfToken("points");
    const SdfPath primPath = SdfPath("/Sphere");
    const SdfPath localRefPrimPath = SdfPath("/SphereOver");

    auto stage = UsdStage::CreateInMemory("test" + fmt);
    auto prim = stage->DefinePrim(primPath);

    auto defAttr = prim.CreateAttribute(defAttrTk, SdfValueTypeNames->Double);
    defAttr.Set<double>(1.0);

    auto sampleAttr = prim.CreateAttribute(sampleAttrTk, 
                                           SdfValueTypeNames->Double);
    for (size_t i = TIME_SAMPLE_BEGIN; i < TIME_SAMPLE_END; ++i) {
        const auto sample = static_cast<double>(i);
        sampleAttr.Set<double>(sample, sample);
    }

    auto localRefPrim = stage->OverridePrim(localRefPrimPath);
    localRefPrim.GetReferences().AddInternalReference(primPath);
    auto localRefAttr = 
        localRefPrim.CreateAttribute(defAttrTk, SdfValueTypeNames->Double);
    localRefAttr.Block();

    return std::make_tuple(stage, defAttr, sampleAttr, localRefAttr);
}

UsdStageRefPtr
_GenerateStageForAnimationBlock(const string& fmt) {
    // Weaker Layer
    SdfLayerRefPtr weakerLayer = 
        SdfLayer::CreateAnonymous("animationBlocks_weaker");
    weakerLayer->ImportFromString(R"(#usda 1.0
over "Human"
{
    int c = 1
    double d = 2.0
}
)");

    // Weak middle layer
    SdfLayerRefPtr weakLayer = 
        SdfLayer::CreateAnonymous("animationBlocks_weak");
    weakLayer->ImportFromString(R"(#usda 1.0
over "Human"
{
    int a = AnimationBlock
    int a.timeSamples = {
        1: 5,
        2: 18,
    }

    double b.spline = {
        1: 5; post held,
        2: 18; post held,
    }

    int c.timeSamples = {
        0: 456,
        1: 789
    }

    double d.spline = {
        1: 5; post held,
        2: 18; post held,
    }
}
)");

    // Stronger layer
    SdfLayerRefPtr strongerLayer = 
        SdfLayer::CreateAnonymous("animationBlocks_strong");
    strongerLayer->ImportFromString(R"(#usda 1.0
def Xform "Human"
{
    double b = AnimationBlock
    double b.spline = {
        1: 10; post held,
        2: 20; post held,
    }

    int c = AnimationBlock

    double d = AnimationBlock

    double e = AnimationBlock
}
)");
    SdfLayerRefPtr rootLayer = SdfLayer::CreateAnonymous("test" + fmt);
    rootLayer->SetSubLayerPaths(
        {strongerLayer->GetIdentifier(),
         weakLayer->GetIdentifier(),
         weakerLayer->GetIdentifier()});
    return UsdStage::Open(rootLayer);
}

template <typename T>
void
_CheckDefaultNotBlocked(UsdAttribute& attr, const T expectedValue)
{
    T value;
    VtValue untypedValue;
    UsdAttributeQuery query(attr);

    TF_AXIOM(attr.Get<T>(&value));
    TF_AXIOM(query.Get<T>(&value));
    TF_AXIOM(attr.Get(&untypedValue));
    TF_AXIOM(query.Get(&untypedValue));
    TF_AXIOM(value == expectedValue);
    TF_AXIOM(untypedValue.UncheckedGet<T>() == expectedValue);
    TF_AXIOM(attr.HasValue());
    TF_AXIOM(attr.HasAuthoredValue());
}

template <typename T>
void
_CheckDefaultBlocked(UsdAttribute& attr)
{
    T value;
    VtValue untypedValue;
    UsdAttributeQuery query(attr);
    UsdResolveInfo info = attr.GetResolveInfo();

    TF_AXIOM(!attr.Get<T>(&value));
    TF_AXIOM(!query.Get<T>(&value));
    TF_AXIOM(!attr.Get(&untypedValue));
    TF_AXIOM(!query.Get(&untypedValue));
    TF_AXIOM(!attr.HasValue());
    TF_AXIOM(!attr.HasAuthoredValue());
    TF_AXIOM(info.HasAuthoredValueOpinion());
}

template <typename T>
void
_CheckSampleNotBlocked(UsdAttribute& attr, 
                       const double time, const T expectedValue)
{
    T value;
    VtValue untypedValue;
    UsdAttributeQuery query(attr);

    TF_AXIOM(attr.Get<T>(&value, time));
    TF_AXIOM(query.Get<T>(&value, time));
    TF_AXIOM(attr.Get(&untypedValue, time));
    TF_AXIOM(query.Get(&untypedValue, time));
    TF_AXIOM(value == expectedValue);
    TF_AXIOM(untypedValue.UncheckedGet<T>() == expectedValue);
}

template <typename T>
void
_CheckSampleBlocked(UsdAttribute& attr, const double time)
{
    T value;
    VtValue untypedValue;
    UsdAttributeQuery query(attr);

    TF_AXIOM(!attr.Get<T>(&value, time));
    TF_AXIOM(!query.Get<T>(&value, time));
    TF_AXIOM(!attr.Get(&untypedValue, time));
    TF_AXIOM(!query.Get(&untypedValue, time));
}

void
_CheckAnimationBlock(UsdStageRefPtr stage)
{
    UsdPrim prim = stage->GetPrimAtPath(SdfPath("/Human"));
    // Since attribute "a"'s strongest time samples are not blocked by an
    // animation block, its time samples shine through. Also even though it has
    // a default animation block, but its weaker and hence doesn't affect its
    // stronger time samples.
    // do also note that default Animation block in the same layer, doesn't
    // affect time samples in the same layer, time samples still win.
    // only default is animtion block
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("a"));
        // source is time samples
        TF_AXIOM(attr.GetResolveInfo().GetSource() == 
                    UsdResolveInfoSource::UsdResolveInfoSourceTimeSamples);
        VtValue untypedValue;
        TF_AXIOM(!attr.Get(&untypedValue));
        TF_AXIOM(untypedValue.IsEmpty());
        // time samples shine through
        TF_AXIOM(attr.Get(&untypedValue, 1.0));
        TF_AXIOM(untypedValue.UncheckedGet<int>() == 5);

        int value;
        TF_AXIOM(!attr.Get(&value));
        TF_AXIOM(attr.Get(&value, 1.0));
        TF_AXIOM(value == 5);
    }

    // Since attribute "b"'s strongest spline values are not blocked by an
    // animation block, its spline values shine through. Also even though it has
    // a default animation block, but its weaker and hence doesn't affect its
    // strongest spline values.
    // do also note that default Animation block in the same stronger layer, 
    // doesn't affect spline values in the same layer, splines still win.
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("b"));
        // source is spline
        TF_AXIOM(attr.GetResolveInfo().GetSource() == 
                    UsdResolveInfoSource::UsdResolveInfoSourceSpline);
        VtValue untypedValue;
        // default is animtion block
        TF_AXIOM(!attr.Get(&untypedValue));
        TF_AXIOM(untypedValue.IsEmpty());
        // stronger spline value shine through (and not the weaker spline or
        // animation block)
        TF_AXIOM(attr.Get(&untypedValue, 1.0));
        TF_AXIOM(untypedValue.UncheckedGet<double>() == 10.0);

        double value;
        TF_AXIOM(!attr.Get(&value));
        TF_AXIOM(attr.Get(&value, 1.0));
        TF_AXIOM(value == 10.0);
    }

    // Since attribute "c"'s strongest value is an Animation block, its blocks
    // any time sample, and results in any non-animation block default value to
    // shine through from the weaker layer.
    // default is 1 and not animation block
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("c"));
        // source is default
        TF_AXIOM(attr.GetResolveInfo().GetSource() == 
                    UsdResolveInfoSource::UsdResolveInfoSourceDefault);
        {
            VtValue untypedValue;
            TF_AXIOM(attr.Get(&untypedValue));
            TF_AXIOM(untypedValue.UncheckedGet<int>() == 1);

            int value;
            TF_AXIOM(attr.Get<int>(&value));
            TF_AXIOM(value == 1);
        }
        // time samples is animation blocked and default shines through
        {
            VtValue untypedValue;
            TF_AXIOM(attr.Get(&untypedValue, 1.0));
            TF_AXIOM(untypedValue.UncheckedGet<int>() == 1);

            int value;
            TF_AXIOM(attr.Get(&value, 1.0));
            TF_AXIOM(value == 1);
        }
    }

    // Since attribute "d"'s strongest value is an Animation block, its blocks
    // any spline, and results in any non-animation block default value to
    // shine through from the weaker layer.
    // default is 2.0 and not animation block
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("d"));
        // source is default
        TF_AXIOM(attr.GetResolveInfo().GetSource() == 
                    UsdResolveInfoSource::UsdResolveInfoSourceDefault);
        {
            VtValue untypedValue;
            TF_AXIOM(attr.Get(&untypedValue));
            TF_AXIOM(untypedValue.UncheckedGet<double>() == 2.0);

            double value;
            TF_AXIOM(attr.Get(&value));
            TF_AXIOM(value == 2.0);
        }
        // spline is animation blocked and default shines through
        {
            VtValue untypedValue;
            TF_AXIOM(attr.Get(&untypedValue, 1.0));
            TF_AXIOM(untypedValue.UncheckedGet<double>() == 2.0);

            double value;
            TF_AXIOM(attr.Get(&value, 1.0));
            TF_AXIOM(value == 2.0);
        }
    }
    // Attr with just animation block, we should get an empty default value with
    // resolve info source as None
    {
        UsdAttribute attr = prim.GetAttribute(TfToken("e"));
        // source is none
        TF_AXIOM(attr.GetResolveInfo().GetSource() == 
                    UsdResolveInfoSource::UsdResolveInfoSourceNone);
        {
            VtValue untypedValue;
            TF_AXIOM(!attr.Get(&untypedValue));
            TF_AXIOM(untypedValue.IsEmpty());
        }
    }
}

int main(int argc, char** argv) {
    vector<string> formats = {".usda", ".usdc"};
    auto block = SdfValueBlock();

    for (const auto& fmt : formats) {
        std::cout << "\n+------------------------------------------+" << std::endl;
        std::cout << "Testing format: " << fmt << std::endl;

        UsdStageRefPtr stage;
        UsdAttribute defAttr, sampleAttr, localRefAttr;
        std::tie(stage, defAttr, sampleAttr, localRefAttr) = _GenerateStage(fmt);

        std::cout << "Testing blocks through local references" << std::endl;
        _CheckDefaultBlocked<double>(localRefAttr);
        _CheckDefaultNotBlocked(defAttr, 1.0);

        std::cout << "Testing blocks on default values" << std::endl;
        defAttr.Set<SdfValueBlock>(block);
        _CheckDefaultBlocked<double>(defAttr);

        defAttr.Set<double>(DEFAULT_VALUE);
        _CheckDefaultNotBlocked(defAttr, DEFAULT_VALUE);

        defAttr.Set(VtValue(block));
        _CheckDefaultBlocked<double>(defAttr);

        // Reset our value
        defAttr.Set<double>(DEFAULT_VALUE);
        _CheckDefaultNotBlocked(defAttr, DEFAULT_VALUE);

        defAttr.Block();
        _CheckDefaultBlocked<double>(defAttr);

        std::cout << "Testing typed time sample operations" << std::endl;
        for (size_t i = TIME_SAMPLE_BEGIN; i < TIME_SAMPLE_END; ++i) {
            const auto sample = static_cast<double>(i);
            bool hasSamplesPre, hasSamplePost;
            double upperPre, lowerPre, lowerPost, upperPost;
            sampleAttr.GetBracketingTimeSamples(sample, &lowerPre, &upperPre,
                                                &hasSamplesPre);

            _CheckSampleNotBlocked(sampleAttr, sample, sample);

            sampleAttr.Set<SdfValueBlock>(block, sample);
            _CheckSampleBlocked<double>(sampleAttr, sample);

            // ensure bracketing time samples continues to report all 
            // things properly even in the presence of blocks
            sampleAttr.GetBracketingTimeSamples(sample, &lowerPost, &upperPost,
                                                &hasSamplePost);
            
            TF_AXIOM(hasSamplesPre == hasSamplePost);
            TF_AXIOM(lowerPre == lowerPost);
            TF_AXIOM(upperPre == upperPost);
        }

        // Reset our value
        for (size_t i = TIME_SAMPLE_BEGIN; i < TIME_SAMPLE_END; ++i) {
            const auto sample = static_cast<double>(i);
            sampleAttr.Set<double>(sample, sample);
        }

        std::cout << "Testing untyped time sample operations" << std::endl;
        for (size_t i = TIME_SAMPLE_BEGIN; i < TIME_SAMPLE_END; ++i) {
            const auto sample = static_cast<double>(i);

            _CheckSampleNotBlocked(sampleAttr, sample, sample);

            sampleAttr.Set(VtValue(block), sample);
            _CheckSampleBlocked<double>(sampleAttr, sample);
        }

        // Reset our value
        for (size_t i = TIME_SAMPLE_BEGIN; i < TIME_SAMPLE_END; ++i) {
            const auto sample = static_cast<double>(i);
            sampleAttr.Set<double>(sample, sample);
        }
        
        // ensure that both default values and time samples are blown away.
        sampleAttr.Block();
        _CheckDefaultBlocked<double>(sampleAttr);
        TF_AXIOM(sampleAttr.GetNumTimeSamples() == 0);
        UsdAttributeQuery sampleQuery(sampleAttr);
        TF_AXIOM(sampleQuery.GetNumTimeSamples() == 0);

        for (size_t i =  TIME_SAMPLE_BEGIN; i < TIME_SAMPLE_END; ++i) {
            const auto sample = static_cast<double>(i);
            _CheckSampleBlocked<double>(sampleAttr, sample);
        }

        // Reset our value
        for (size_t i = TIME_SAMPLE_BEGIN; i < TIME_SAMPLE_END; ++i) {
            const auto sample = static_cast<double>(i);
            sampleAttr.Set<double>(sample, sample);
        }
     
        // Test attribute blocking behavior in between blocked/unblocked times
        for (size_t i = TIME_SAMPLE_BEGIN; i < TIME_SAMPLE_END; i+=2) {
            const auto sample = static_cast<double>(i);
            sampleAttr.Set<SdfValueBlock>(block, sample);

            _CheckSampleBlocked<double>(sampleAttr, sample);

            if (sample+1 < TIME_SAMPLE_END) {
                double sampleStepHalf = sample+0.5;
                _CheckSampleBlocked<double>(sampleAttr, sampleStepHalf);
                _CheckSampleNotBlocked(sampleAttr, sample+1.0, sample+1.0);
            }
        }

        std::cout << "Testing animation block" << std::endl;
        _CheckAnimationBlock(_GenerateStageForAnimationBlock(fmt));
        std::cout << "+------------------------------------------+" << std::endl;
    }

    printf("\n\n>>> Test SUCCEEDED\n");
}
