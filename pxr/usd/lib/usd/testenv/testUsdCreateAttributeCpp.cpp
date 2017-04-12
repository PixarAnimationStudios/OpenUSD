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
#include "pxr/usd/usd/references.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/prim.h"

#include "pxr/usd/sdf/attributeSpec.h"
#include "pxr/usd/sdf/schema.h"

#include "pxr/base/vt/value.h"
#include "pxr/base/tf/debug.h"
#include "pxr/base/arch/fileSystem.h"

#include <Python.h>


PXR_NAMESPACE_USING_DIRECTIVE

void
TestPrim()
{
    SdfPath primPath("/CppFoo");
    TfToken prop("Something");
    std::string propPath(primPath.GetString() + ".Something");
    std::string value = "Foobar";
    std::string result = "not Foobar";
    VtValue tmp;

    ArchUnlinkFile("foo.usdc");
    UsdStageRefPtr stage = UsdStage::CreateNew("foo.usdc");
    SdfLayerHandle layer = stage->GetRootLayer();

    {
        // Listing fields for a property on a non-existent prim path should not
        // post errors (bug 90170).
        TfErrorMark mark;
        TF_VERIFY(layer->ListFields(SdfPath("I_Do_Not_Exist.attribute")).empty());
        TF_VERIFY(mark.IsClean());
    }

    TF_VERIFY(stage->OverridePrim(primPath),
              "Failed to create prim at %s",
              primPath.GetText());

    UsdPrim prim(stage->GetPrimAtPath(primPath));
    TF_VERIFY(prim,
              "Failed to get Prim from %s",
              primPath.GetText());

    TF_VERIFY(prim.CreateAttribute(prop, SdfValueTypeNames->String),
              "Failed to create property at %s",
              propPath.c_str());

    TF_VERIFY(prim.GetAttribute(prop).Set(VtValue(value), UsdTimeCode(0.0)),
              "Failed to set property at %s",
              propPath.c_str());

    TF_VERIFY(prim.GetAttribute(prop).Get(&tmp, UsdTimeCode(0.0)),
              "Failed to get property at %s",
              propPath.c_str());

    TF_VERIFY(tmp.IsHolding<std::string>(),
              "Invalid type for value of property %s",
              propPath.c_str());

    result = tmp.UncheckedGet<std::string>();
    TF_VERIFY(result == value,
              "Values do not match for %s, %s != %s", 
              propPath.c_str(),
              result.c_str(), 
              value.c_str());
}

void
TestIsDefined()
{
    // This tests the functionality of UsdAttribute::IsDefined.
    //
    // It has the ability to specify a target layer, so here we will create an
    // attribute in a weak layer, reference it into a stronger layer and then
    // assert that it wasn't defined in the strong layer.
    //
    // Next, we set a value in the stronger layer, which implicitly must define
    // the attribute, and then assert that the attribute exists in both layers.

    SdfLayerRefPtr weakLayer = SdfLayer::CreateNew("IsDefined_weak.usd");
    SdfLayerRefPtr strongLayer = SdfLayer::CreateNew("IsDefined_strong.usd");

    //
    // Weak Layer: Create prim and attribute.
    //
    UsdStageRefPtr stage = UsdStage::Open(weakLayer->GetIdentifier());
    UsdPrim p = stage->OverridePrim(SdfPath("/Parent"));

    TfToken attr1("attr1");
    TF_VERIFY(!p.GetAttribute(attr1).IsDefined());
    TF_VERIFY(p.CreateAttribute(attr1, SdfValueTypeNames->String));

    //
    // Strong Layer: Create prim and a reference to weak layer.
    //
    stage = UsdStage::Open(strongLayer->GetIdentifier());
    p = stage->OverridePrim(SdfPath("/Parent"));
    p.GetReferences().AppendReference(
        SdfReference(weakLayer->GetIdentifier(), SdfPath("/Parent")));

    //
    // Now that we've referenced in the weak layer, make sure our definition 
    // assumptions hold. 
    //
    TF_VERIFY(p.GetAttribute(attr1).IsDefined());
    TF_VERIFY(p.GetAttribute(attr1).IsAuthoredAt(weakLayer));
    TF_VERIFY(!p.GetAttribute(attr1).IsAuthoredAt(strongLayer));

    // 
    // Now set a value and verify that the attr is defined everywhere.
    //
    TF_VERIFY(p.GetAttribute(attr1).Set("foo"));
    TF_VERIFY(p.GetAttribute(attr1).IsDefined());
    TF_VERIFY(p.GetAttribute(attr1).IsAuthoredAt(weakLayer));
    TF_VERIFY(p.GetAttribute(attr1).IsAuthoredAt(strongLayer));
}

class ExpectedError {
public:
    ExpectedError() {
        std::cerr << "--- BEGIN EXPECTED ERROR ---\n";
    }

    ~ExpectedError() {
        std::cerr << "--- END EXPECTED ERROR ---\n";
        TF_VERIFY(!_mark.IsClean());
    }
private:
    TfErrorMark _mark;
};

void
VerifyTimeSampleRange(UsdAttribute const & attr, size_t expectedNumSamples, 
        double expectedMin = 0, double expectedMax = 0)
{
    std::vector<double> samples;
    if (!TF_VERIFY(attr.GetTimeSamples(&samples)))
        return;

    TF_VERIFY(expectedNumSamples == samples.size());

    double upper=0., lower=0.;
    bool hasTimeSamples=false;
    attr.GetBracketingTimeSamples((expectedMin + expectedMax) / 2.0,
                                  &lower, &upper, &hasTimeSamples);

    size_t numSamples = attr.GetNumTimeSamples();

    // Break-out verifies for better reporting.
    if (expectedNumSamples == 0) {
        TF_VERIFY(samples.empty());
        TF_VERIFY(!hasTimeSamples);
        TF_VERIFY(numSamples==0);
    } else { 
        TF_VERIFY(!samples.empty());
        TF_VERIFY(hasTimeSamples);
        TF_VERIFY(numSamples == expectedNumSamples);
        TF_VERIFY(expectedMin == samples.front());
        TF_VERIFY(expectedMax == samples.back());
        TF_VERIFY(expectedMin == lower);
        TF_VERIFY(expectedMax == upper);
    }
}

void
TestValueMutation(std::string const & layerTag)
{
    UsdStageRefPtr stage = UsdStage::CreateInMemory(layerTag);
    UsdPrim prim = stage->OverridePrim(SdfPath("/APrim"));
    TfToken attrNameToken("attr1");
    UsdAttribute attr = prim.CreateAttribute(attrNameToken, SdfValueTypeNames->String);

    std::string value;

    // Empty initial state
    TF_VERIFY(!attr.Get(&value));

    // Ensure that attempting to set a value with incorrect type issues an
    // error.
    { ExpectedError err; TF_AXIOM(!attr.Set(1.234)); }

    // Make sure clear doesn't do anything crazy before authoring values
    TF_VERIFY(attr.ClearAtTime(1.0));
    TF_VERIFY(attr.ClearDefault());
    TF_VERIFY(attr.Clear());
    // Has 
    TF_VERIFY(!attr.HasValue());
    TF_VERIFY(!attr.HasAuthoredValueOpinion());
    TF_VERIFY(!attr.HasMetadata(SdfFieldKeys->Default));
    TF_VERIFY(!attr.HasMetadata(SdfFieldKeys->TimeSamples));
    VerifyTimeSampleRange(attr, 0);

    // We should safely handle non-existant attributes as well
    UsdAttribute bogusAttr = prim.GetAttribute(TfToken("Non_Existing_Attribute"));
    TF_VERIFY(bogusAttr.Clear());
    TF_VERIFY(bogusAttr.ClearAtTime(1.0)); 
    TF_VERIFY(bogusAttr.ClearDefault());

    //
    // Test exclusively with UsdTimeCode::Default()
    //

    // Set
    TF_VERIFY(attr.Set("foo bar"));
    // Has
    TF_VERIFY(attr.HasValue());
    TF_VERIFY(attr.HasAuthoredValueOpinion());
    TF_VERIFY(attr.HasMetadata(SdfFieldKeys->Default));
    TF_VERIFY(!attr.HasMetadata(SdfFieldKeys->TimeSamples));
    VerifyTimeSampleRange(attr, 0);
    // Get
    TF_VERIFY(attr.Get(&value));
    TF_VERIFY(value == "foo bar");
    // Get with wrong type.
    { double d; TF_VERIFY(!attr.Get<double>(&d)); }
    { double d; TF_VERIFY(!attr.Get<double>(&d, 1.0)); }
    { double d; TF_VERIFY(!attr.Get<double>(&d, 2.0)); }

    // Clear a time, it should leave the default intact
    TF_VERIFY(attr.ClearAtTime(1.0));
    TF_VERIFY(attr.Get(&value));
    // Has
    TF_VERIFY(attr.HasValue());
    TF_VERIFY(attr.HasAuthoredValueOpinion());
    TF_VERIFY(attr.HasMetadata(SdfFieldKeys->Default));
    TF_VERIFY(!attr.HasMetadata(SdfFieldKeys->TimeSamples));
    VerifyTimeSampleRange(attr, 0);
    // Get with wrong type.
    { double d; TF_VERIFY(!attr.Get<double>(&d)); }
    { double d; TF_VERIFY(!attr.Get<double>(&d, 1.0)); }
    { double d; TF_VERIFY(!attr.Get<double>(&d, 2.0)); }

    // Now clear the default
    TF_VERIFY(attr.ClearDefault());
    TF_VERIFY(!attr.Get(&value));
    // Has
    TF_VERIFY(!attr.HasValue());
    TF_VERIFY(!attr.HasAuthoredValueOpinion());
    TF_VERIFY(!attr.HasMetadata(SdfFieldKeys->Default));
    TF_VERIFY(!attr.HasMetadata(SdfFieldKeys->TimeSamples));
    VerifyTimeSampleRange(attr, 0);

    //
    // With a single time sample
    //

    // Set
    TF_VERIFY(attr.Set("time=1", 1.0));
    // Has
    TF_VERIFY(attr.HasValue());
    TF_VERIFY(attr.HasAuthoredValueOpinion());
    TF_VERIFY(!attr.HasMetadata(SdfFieldKeys->Default));
    TF_VERIFY(attr.HasMetadata(SdfFieldKeys->TimeSamples));
    VerifyTimeSampleRange(attr, 1, 1.0, 1.0);
    // Get
    TF_VERIFY(attr.Get(&value, 1.0));
    TF_VERIFY(value == "time=1");
    // Get with wrong type.
    { double d; TF_VERIFY(!attr.Get<double>(&d, 1.0)); }
    { double d; TF_VERIFY(!attr.Get<double>(&d, 2.0)); }

    // Clear the default, it should leave the time intact
    TF_VERIFY(attr.ClearDefault());
    TF_VERIFY(attr.Get(&value, 1.0));
    // Has
    TF_VERIFY(attr.HasValue());
    TF_VERIFY(attr.HasAuthoredValueOpinion());
    TF_VERIFY(!attr.HasMetadata(SdfFieldKeys->Default));
    TF_VERIFY(attr.HasMetadata(SdfFieldKeys->TimeSamples));
    VerifyTimeSampleRange(attr, 1, 1, 1);
    // Get with wrong type.
    { double d; TF_VERIFY(!attr.Get<double>(&d, 1.0)); }
    { double d; TF_VERIFY(!attr.Get<double>(&d, 2.0)); }

    // Now clear the time value
    TF_VERIFY(attr.ClearAtTime(1.0));
    TF_VERIFY(!attr.Get(&value, 1.0));

    // Has
    TF_VERIFY(!attr.HasValue());
    TF_VERIFY(!attr.HasAuthoredValueOpinion());
    TF_VERIFY(!attr.HasMetadata(SdfFieldKeys->Default));
    TF_VERIFY(!attr.HasMetadata(SdfFieldKeys->TimeSamples));
    VerifyTimeSampleRange(attr, 0);

    //
    // With multiple time samples
    //

    // Set
    TF_VERIFY(attr.Set("time=1", 1.0));
    TF_VERIFY(attr.Set("time=2", 2.0));
    // Has
    TF_VERIFY(attr.HasValue());
    TF_VERIFY(attr.HasAuthoredValueOpinion());
    TF_VERIFY(!attr.HasMetadata(SdfFieldKeys->Default));
    TF_VERIFY(attr.HasMetadata(SdfFieldKeys->TimeSamples));
    VerifyTimeSampleRange(attr, 2, 1, 2);
    // Get
    TF_VERIFY(attr.Get(&value, 1.0));
    TF_VERIFY(value == "time=1");
    TF_VERIFY(attr.Get(&value, 2.0));
    TF_VERIFY(value == "time=2");
    // Get with wrong type.
    { double d; TF_VERIFY(!attr.Get<double>(&d, 1.0)); }
    { double d; TF_VERIFY(!attr.Get<double>(&d, 2.0)); }

    // Clear the default, it should leave the time intact
    TF_VERIFY(attr.ClearDefault());
    TF_VERIFY(attr.Get(&value, 1.0));
    TF_VERIFY(value == "time=1");
    TF_VERIFY(attr.Get(&value, 2.0));
    TF_VERIFY(value == "time=2");
    // Get with wrong type.
    { double d; TF_VERIFY(!attr.Get<double>(&d, 1.0)); }
    { double d; TF_VERIFY(!attr.Get<double>(&d, 2.0)); }

    // Now clear the time=1 value
    TF_VERIFY(attr.ClearAtTime(1.0));
    TF_VERIFY(attr.Get(&value, 1.0));
    TF_VERIFY(value == "time=2");
    // Has
    TF_VERIFY(attr.HasValue());
    TF_VERIFY(attr.HasAuthoredValueOpinion());
    TF_VERIFY(!attr.HasMetadata(SdfFieldKeys->Default));
    TF_VERIFY(attr.HasMetadata(SdfFieldKeys->TimeSamples));
    VerifyTimeSampleRange(attr, 1, 2, 2);
    // Get with wrong type.
    { double d; TF_VERIFY(!attr.Get<double>(&d, 1.0)); }
    { double d; TF_VERIFY(!attr.Get<double>(&d, 2.0)); }

    // Now clear the time=2 value
    TF_VERIFY(attr.ClearAtTime(2.0));
    TF_VERIFY(!attr.Get(&value, 2.0));
    // Has
    TF_VERIFY(!attr.HasValue());
    TF_VERIFY(!attr.HasAuthoredValueOpinion());
    TF_VERIFY(!attr.HasMetadata(SdfFieldKeys->Default));
    TF_VERIFY(!attr.HasMetadata(SdfFieldKeys->TimeSamples));
    VerifyTimeSampleRange(attr, 0);
    // Get with wrong type.
    { double d; TF_VERIFY(!attr.Get<double>(&d, 1.0)); }
    { double d; TF_VERIFY(!attr.Get<double>(&d, 2.0)); }

    //
    // With multiple time samples and a default value
    //

    // Set
    TF_VERIFY(attr.Set("time=default"));
    TF_VERIFY(attr.Set("time=1", 1.0));
    TF_VERIFY(attr.Set("time=2", 2.0));
    // Get
    TF_VERIFY(attr.Get(&value));
    TF_VERIFY(value == "time=default");
    TF_VERIFY(attr.Get(&value, 1.0));
    TF_VERIFY(value == "time=1");
    TF_VERIFY(attr.Get(&value, 2.0));
    TF_VERIFY(value == "time=2");
    // Get with wrong type.
    { double d; TF_VERIFY(!attr.Get<double>(&d)); }
    { double d; TF_VERIFY(!attr.Get<double>(&d, 1.0)); }
    { double d; TF_VERIFY(!attr.Get<double>(&d, 2.0)); }

    // Has
    TF_VERIFY(attr.HasValue());
    TF_VERIFY(attr.HasAuthoredValueOpinion());
    TF_VERIFY(attr.HasMetadata(SdfFieldKeys->Default));
    TF_VERIFY(attr.HasMetadata(SdfFieldKeys->TimeSamples));
    VerifyTimeSampleRange(attr, 2, 1, 2);

    // Clear t=1, it should leave t=2 and t=default 
    TF_VERIFY(attr.ClearAtTime(1.0));
    TF_VERIFY(attr.Get(&value, 1.0));
    // Because of held-value interpolation, and because we cleared the value at 1.0,
    // we expect value at 1.0 to be "time=2"
    TF_VERIFY(value == "time=2");
    TF_VERIFY(attr.Get(&value));
    TF_VERIFY(value == "time=default");
    // Has
    TF_VERIFY(attr.HasValue());
    TF_VERIFY(attr.HasAuthoredValueOpinion());
    TF_VERIFY(attr.HasMetadata(SdfFieldKeys->Default));
    TF_VERIFY(attr.HasMetadata(SdfFieldKeys->TimeSamples));
    VerifyTimeSampleRange(attr, 1, 2, 2);
    // Get with wrong type.
    { double d; TF_VERIFY(!attr.Get<double>(&d)); }
    { double d; TF_VERIFY(!attr.Get<double>(&d, 1.0)); }
    { double d; TF_VERIFY(!attr.Get<double>(&d, 2.0)); }

    // Now clear the time=2 value, should leave t=default
    TF_VERIFY(attr.ClearAtTime(2.0));
    // Has
    TF_VERIFY(attr.HasValue());
    TF_VERIFY(attr.HasAuthoredValueOpinion());
    TF_VERIFY(attr.HasMetadata(SdfFieldKeys->Default));
    TF_VERIFY(!attr.HasMetadata(SdfFieldKeys->TimeSamples));
    VerifyTimeSampleRange(attr, 0);
    // Get
    TF_VERIFY(attr.Get(&value, 1.0));
    TF_VERIFY(value == "time=default");
    // Get with wrong type.
    { double d; TF_VERIFY(!attr.Get<double>(&d)); }
    { double d; TF_VERIFY(!attr.Get<double>(&d, 1.0)); }
    { double d; TF_VERIFY(!attr.Get<double>(&d, 2.0)); }

    // Now clear the default value
    TF_VERIFY(attr.ClearDefault());
    TF_VERIFY(!attr.Get(&value, 2.0));
    // Has
    TF_VERIFY(!attr.HasValue());
    TF_VERIFY(!attr.HasAuthoredValueOpinion());
    TF_VERIFY(!attr.HasMetadata(SdfFieldKeys->Default));
    TF_VERIFY(!attr.HasMetadata(SdfFieldKeys->TimeSamples));
    VerifyTimeSampleRange(attr, 0);
    // Get with wrong type.
    { double d; TF_VERIFY(!attr.Get<double>(&d)); }
    { double d; TF_VERIFY(!attr.Get<double>(&d, 1.0)); }
    { double d; TF_VERIFY(!attr.Get<double>(&d, 2.0)); }

    //
    // Multiple time samples and single call to Clear 
    //

    // Set
    TF_VERIFY(attr.Set("time=default"));
    TF_VERIFY(attr.Set("time=1", 1.0));
    TF_VERIFY(attr.Set("time=2", 2.0));
    // Get
    TF_VERIFY(attr.Get(&value));
    TF_VERIFY(value == "time=default");
    TF_VERIFY(attr.Get(&value, 1.0));
    TF_VERIFY(value == "time=1");
    TF_VERIFY(attr.Get(&value, 2.0));
    TF_VERIFY(value == "time=2");
    // Has
    TF_VERIFY(attr.HasValue());
    TF_VERIFY(attr.HasAuthoredValueOpinion());
    TF_VERIFY(attr.HasMetadata(SdfFieldKeys->Default));
    TF_VERIFY(attr.HasMetadata(SdfFieldKeys->TimeSamples));
    VerifyTimeSampleRange(attr, 2, 1, 2);

    // Clear() should remove all values 
    TF_VERIFY(attr.Clear());
    TF_VERIFY(!attr.Get(&value, 1.0));
    TF_VERIFY(!attr.Get(&value, 2.0));
    TF_VERIFY(!attr.Get(&value));
    // Has
    TF_VERIFY(!attr.HasValue());
    TF_VERIFY(!attr.HasAuthoredValueOpinion());
    TF_VERIFY(!attr.HasMetadata(SdfFieldKeys->Default));
    TF_VERIFY(!attr.HasMetadata(SdfFieldKeys->TimeSamples));
    VerifyTimeSampleRange(attr, 0);
}

void
TestQueryTimeSample()
{
    SdfLayerRefPtr l = SdfLayer::CreateAnonymous("f.usdc");
    SdfPrimSpecHandle p = SdfPrimSpec::New(l, "Foo", SdfSpecifierDef, "Scope");
    SdfAttributeSpecHandle a = SdfAttributeSpec::New(p, "attr", 
                                                     SdfValueTypeNames->String);
    SdfTimeSampleMap tsm;
    tsm[1.0] = "Foo";
    a->SetInfo(TfToken("timeSamples"),VtValue(tsm)); 
    SdfPath path("/Foo");
    TfToken attr("attr");
    l->QueryTimeSample(SdfAbstractDataSpecId(&path, &attr), 1.0);
}

void
TestVariability()
{
    // XXX When bug/100734 is addressed, we should also test here
    // that authoring to a uniform attribute creates no timeSamples
    
    UsdStageRefPtr s = UsdStage::CreateInMemory();
    UsdPrim  foo = s->OverridePrim(SdfPath("/foo"));
    
    UsdAttribute varAttr = foo.CreateAttribute(TfToken("varyingAttr"),
                                               SdfValueTypeNames->TokenArray);
    TF_VERIFY(varAttr.GetVariability() == SdfVariabilityVarying);
    
    UsdAttribute uniformAttr = foo.CreateAttribute(TfToken("uniformAttr"),
                                                   SdfValueTypeNames->Token,
                                                   /* custom = */ true,
                                                   SdfVariabilityUniform);
    TF_VERIFY(uniformAttr.GetVariability() == SdfVariabilityUniform);
    
    
    {
        // Config variability is illegal in Usd
        ExpectedError  err;
        TF_VERIFY(!foo.CreateAttribute(TfToken("configAttr"), 
                                          SdfValueTypeNames->Token,
                                          true, SdfVariabilityConfig));
    }

}

int main()
{
    TestPrim();
    TestIsDefined();
    TestValueMutation("foo.usda");
    TestValueMutation("foo.usdc");
    TestQueryTimeSample();
    TestVariability();

    TF_AXIOM(!Py_IsInitialized());
}
