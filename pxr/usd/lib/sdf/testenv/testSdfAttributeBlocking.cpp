#include <string>
#include <functional>
#include <cstdlib>

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/primSpec.h"
#include "pxr/usd/sdf/attributeSpec.h"

int _TestFailed() {
    printf(">>> Test FAILED\n");
    std::exit(1);
}

int main(int argc, char** argv) {
    // Create a layer with a single root prim
    auto layer = SdfLayer::CreateAnonymous();
    auto prim = SdfPrimSpec::New(layer, "Sphere", SdfSpecifierDef);

    // Create a couple of attributes, one with a default and one
    // with time samples authored.
    auto visToken = TfToken("visibility");
    auto defAttr = SdfAttributeSpec::New(prim, visToken,
                                         SdfValueTypeNames->Token);
    defAttr->SetDefaultValue(VtValue("visible"));

    auto sampleAttr = SdfAttributeSpec::New(prim, "xformOp:transform",
                                            SdfValueTypeNames->Double);
    layer->SetTimeSample(sampleAttr->GetPath(), 101, VtValue(101.0));
    layer->SetTimeSample(sampleAttr->GetPath(), 102, VtValue(102.0));
    layer->SetTimeSample(sampleAttr->GetPath(), 103, VtValue(103.0));
    layer->SetTimeSample(sampleAttr->GetPath(), 104, VtValue(104.0));

    // create a container to hold intermediate results
    VtValue value;
    SdfValueBlock typedValue;
    double illTypedValue;

    // test blocking of time samples
    // ----------------------------------------------------------------------
    for (size_t i = 101; i < 105; ++i) {
        // Test the VtValue Based API
        auto sample = static_cast<double>(i); 
        TF_AXIOM(layer->QueryTimeSample(sampleAttr->GetPath(), sample, &value));
        TF_AXIOM(value.UncheckedGet<double>() == sample);

        layer->SetTimeSample(sampleAttr->GetPath(), sample, SdfValueBlock());
        TF_AXIOM(layer->QueryTimeSample(sampleAttr->GetPath(), sample, &value));
        TF_AXIOM(value.IsHolding<SdfValueBlock>());

        // Reset the value so we can test the templated API
        layer->SetTimeSample(sampleAttr->GetPath(), sample, 1.0);

        // stress the templated API
        layer->SetTimeSample<SdfValueBlock>(sampleAttr->GetPath(), 
                                            sample, SdfValueBlock());
        TF_AXIOM(layer->QueryTimeSample<SdfValueBlock>(sampleAttr->GetPath(),
                                              sample, &typedValue));
        TF_AXIOM(value.IsHolding<SdfValueBlock>());

        // Ensure that improperly calling get(mismatched types)
        // both returns false and does not throw any sort of errors.
        TfErrorMark errors;
        TF_AXIOM(not layer->QueryTimeSample<double>(sampleAttr->GetPath(),
                                                    sample, &illTypedValue));
        TF_AXIOM(errors.IsClean());
    }

    // test blocking of defaults
    // ----------------------------------------------------------------------
    // Test the VtValue based API
    TF_AXIOM(defAttr->GetDefaultValue().UncheckedGet<TfToken>() == "visible");
    defAttr->SetDefaultValue(VtValue(SdfValueBlock()));
    TF_AXIOM(defAttr->GetDefaultValue().IsHolding<SdfValueBlock>());

    // Reset the value
    defAttr->SetDefaultValue(VtValue("visible"));

    // Test the templated layer API
    layer->SetField<SdfValueBlock>(defAttr->GetPath(), visToken,typedValue);
    TF_AXIOM(layer->HasField<SdfValueBlock>(defAttr->GetPath(), visToken,
                                            &typedValue));
    layer->GetFieldAs<SdfValueBlock>(defAttr->GetPath(), visToken);

    printf(">>> Test SUCCEEDED\n");
}
