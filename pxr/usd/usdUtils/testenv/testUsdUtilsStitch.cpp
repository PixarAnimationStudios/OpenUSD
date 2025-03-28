//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/stitch.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/attributeSpec.h"

#include "pxr/base/vt/dictionary.h"
#include "pxr/base/vt/value.h"

PXR_NAMESPACE_USING_DIRECTIVE

static void
TestCallback()
{
    SdfLayerRefPtr layer1 = SdfLayer::CreateAnonymous(".usda");
    SdfPrimSpecHandle strongPrim = SdfCreatePrimInLayer(
        layer1, SdfPath("/Root"));
    SdfAttributeSpecHandle strongAttr = SdfAttributeSpec::New(
        strongPrim, "attr", SdfValueTypeNames->Double);
    layer1->SetTimeSample(strongAttr->GetPath(), 1.0, 1.0);

    SdfLayerRefPtr layer2 = SdfLayer::CreateAnonymous(".usda");
    SdfPrimSpecHandle weakPrim = SdfCreatePrimInLayer(
        layer2, SdfPath("/Root"));
    SdfAttributeSpecHandle weakAttr = SdfAttributeSpec::New(
        weakPrim, "attr", SdfValueTypeNames->Double);
    weakAttr->GetCustomData()["copy_samples"] = VtValue(false);
    layer2->SetTimeSample(weakAttr->GetPath(), 2.0, 2.0);

    UsdUtilsStitchValueFn maybeMergeTimeSamples =     
        [layer1, layer2](
            const TfToken& field, const SdfPath& path,
            const SdfLayerHandle& strongLayer, bool fieldInStrongLayer,
            const SdfLayerHandle& weakLayer, bool fieldInWeakLayer,
            VtValue* stitchedValue) {

        TF_AXIOM(strongLayer == layer1);
        TF_AXIOM(weakLayer == layer2);
        
        if (field == SdfFieldKeys->TimeSamples) {
            TF_AXIOM(path == SdfPath("/Root.attr"));
            
            // Both layers have time samples in them.
            TF_AXIOM(fieldInStrongLayer);
            TF_AXIOM(fieldInWeakLayer);

            SdfAttributeSpecHandle attrSpec = 
                weakLayer->GetAttributeAtPath(path);
            VtValue shouldCopy = attrSpec->GetCustomData()["copy_samples"];
            if (!shouldCopy.Get<bool>()) {
                return UsdUtilsStitchValueStatus::NoStitchedValue;
            }
        }
        else if (field == SdfFieldKeys->CustomData) {
            TF_AXIOM(path == SdfPath("/Root.attr"));
            TF_AXIOM(path == SdfPath("/Root.attr"));

            VtDictionary customData = 
                strongLayer->GetAttributeAtPath(path)->GetCustomData();
            int numStitched = VtDictionaryGet<int>(
                customData, "num_stitched", VtDefault = 0);

            customData["num_stitched"] = VtValue(++numStitched);
            *stitchedValue = VtValue::Take(customData);
            return UsdUtilsStitchValueStatus::UseSuppliedValue;
        }

        return UsdUtilsStitchValueStatus::UseDefaultValue;
    };

    // Stitch layer1 and layer2 together. Time samples for the attribute
    // should not be merged together.
    UsdUtilsStitchLayers(layer1, layer2, maybeMergeTimeSamples);
    TF_AXIOM(!layer1->QueryTimeSample(strongAttr->GetPath(), 2.0));
    TF_AXIOM((strongAttr->GetCustomData() == 
                VtDictionary{{"num_stitched", VtValue(1)}}));

    // Set custom data to allow merging time samples and stitch again.
    weakAttr->GetCustomData()["copy_samples"] = VtValue(true);

    UsdUtilsStitchLayers(layer1, layer2, maybeMergeTimeSamples);
    TF_AXIOM(layer1->QueryTimeSample(strongAttr->GetPath(), 2.0));
    TF_AXIOM((strongAttr->GetCustomData() == 
                VtDictionary{{"num_stitched", VtValue(2)}}));
}

int main(int argc, char** argv)
{
    TestCallback();
    return 0;
}
