#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/attribute.h"

#include <cstdlib>
#include <vector>
#include <string>
#include <tuple>
using std::string;
using std::vector;
using std::tuple;

constexpr size_t TIME_SAMPLE_BEGIN = 101.0;
constexpr size_t TIME_SAMPLE_END = 120.0;
constexpr double DEFAULT_VALUE = 4.0;

tuple<UsdStageRefPtr, UsdAttribute, UsdAttribute>
_GenerateStage(const string& fmt) {
    const TfToken defAttrTk = TfToken("size");
    const TfToken sampleAttrTk = TfToken("points");


    auto stage = UsdStage::CreateInMemory("test" + fmt);
    auto prim = stage->DefinePrim(SdfPath("/Sphere"));
    auto defAttr = prim.CreateAttribute(defAttrTk, SdfValueTypeNames->Double);
    defAttr.Set<double>(1.0);

    auto sampleAttr = prim.CreateAttribute(sampleAttrTk, 
                                           SdfValueTypeNames->Double);
    
    for (size_t i = TIME_SAMPLE_BEGIN; i < TIME_SAMPLE_END; ++i) {
        const auto sample = static_cast<double>(i);
        sampleAttr.Set<double>(sample, sample);
    }

    return std::make_tuple(stage, defAttr, sampleAttr);
}

int main(int argc, char** argv) {
    // ensure we excersize across all formats
    vector<string> formats = {".usda", ".usdb", ".usdc"};
    auto block = SdfValueBlock();

    for (const auto& fmt : formats) {
        VtValue dummyUntypedValue;
        double dummyTypedValue;
        UsdStageRefPtr stage;
        UsdAttribute defAttr, sampleAttr;
    
        std::tie(stage, defAttr, sampleAttr) = _GenerateStage(fmt);

        // Test the attribute setting API(templated) for defaults
        // -----------------------------------------------------------
        TF_AXIOM(defAttr.Get<double>(&dummyTypedValue));
        defAttr.Set<SdfValueBlock>(block);
        TF_AXIOM(not defAttr.Get<double>(&dummyTypedValue));
        TF_AXIOM(not defAttr.Get(&dummyUntypedValue));
        TF_AXIOM(not defAttr.HasValue());
        TF_AXIOM(defAttr.HasAuthoredValueOpinion());

        // Reset our value
        defAttr.Set<double>(DEFAULT_VALUE);

        // Test the attribute setting API(untyped) for defaults
        // ------------------------------------------------------------
        TF_AXIOM(defAttr.Get(&dummyUntypedValue));
        TF_AXIOM(dummyUntypedValue.UncheckedGet<double>() == DEFAULT_VALUE);
        defAttr.Set(VtValue(block));
        TF_AXIOM(not defAttr.Get<double>(&dummyTypedValue));
        TF_AXIOM(not defAttr.Get(&dummyUntypedValue));
        TF_AXIOM(not defAttr.HasValue());
        TF_AXIOM(defAttr.HasAuthoredValueOpinion());

        // Reset our value
        defAttr.Set<double>(DEFAULT_VALUE);

        // Test the attribute setting for defaults via our convenience API
        // ------------------------------------------------------------
        TF_AXIOM(defAttr.Get(&dummyUntypedValue));
        TF_AXIOM(dummyUntypedValue.UncheckedGet<double>() == DEFAULT_VALUE);
        defAttr.Block();
        TF_AXIOM(not defAttr.Get(&dummyUntypedValue));

        // Test the attribute setting API(templated) for time samples 
        // -----------------------------------------------------------
        for (size_t i = TIME_SAMPLE_BEGIN; i < TIME_SAMPLE_END; ++i) {
            const auto sample = static_cast<double>(i);
            bool hasSamplesPre, hasSamplePost;
            double upperPre, lowerPre, lowerPost, upperPost;
            sampleAttr.GetBracketingTimeSamples(sample, &lowerPre, &upperPre,
                                                &hasSamplesPre);
            TF_AXIOM(sampleAttr.Get<double>(&dummyTypedValue, sample));
            TF_AXIOM(dummyTypedValue == sample);
            sampleAttr.Set<SdfValueBlock>(block, sample);
            TF_AXIOM(not sampleAttr.Get<double>(&dummyTypedValue, sample));
            sampleAttr.GetBracketingTimeSamples(sample, &lowerPost, &upperPost,
                                                &hasSamplePost);
            // ensure bracketing time samples continues to report all 
            // things properly even in the presence of blocks
            TF_AXIOM(hasSamplesPre == hasSamplePost);
            TF_AXIOM(lowerPre == lowerPost);
            TF_AXIOM(upperPre == upperPost);
        }

        // Reset our value
        for (size_t i = TIME_SAMPLE_BEGIN; i < TIME_SAMPLE_END; ++i) {
            const auto sample = static_cast<double>(i);
            sampleAttr.Set<double>(sample, sample);
        }

        // Test the attribute setting API(untyped) for time samples
        // ------------------------------------------------------------
        TF_AXIOM(not sampleAttr.Get(&dummyUntypedValue));
        for (size_t i = TIME_SAMPLE_BEGIN; i < TIME_SAMPLE_END; ++i) {
            const auto sample = static_cast<double>(i);
            TF_AXIOM(sampleAttr.Get(&dummyUntypedValue, sample));
            TF_AXIOM(dummyUntypedValue.UncheckedGet<double>() == sample);
            sampleAttr.Set(VtValue(block), sample);
            TF_AXIOM(not sampleAttr.Get<double>(&dummyTypedValue, sample));
        }

        // Reset our value
        for (size_t i = TIME_SAMPLE_BEGIN; i < TIME_SAMPLE_END; ++i) {
            const auto sample = static_cast<double>(i);
            sampleAttr.Set<double>(sample, sample);
        }
        
        // Test the attribute setting for time samples via our convenience API
        // ------------------------------------------------------------
        TF_AXIOM(sampleAttr.GetNumTimeSamples() != 0);
        sampleAttr.Block();
        TF_AXIOM(sampleAttr.GetNumTimeSamples() == 0);
        // ensure that both default values and time samples are blown away.
        TF_AXIOM(not sampleAttr.Get(&dummyUntypedValue));
        for (size_t i =  TIME_SAMPLE_BEGIN; i < TIME_SAMPLE_END; ++i) {
            const auto sample = static_cast<double>(i);
            TF_AXIOM(not sampleAttr.Get<double>(&dummyTypedValue, sample));
            TF_AXIOM(not sampleAttr.Get(&dummyUntypedValue, sample));
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
            TF_AXIOM(not sampleAttr.Get(&dummyUntypedValue, sample));
            if (sample+1 < TIME_SAMPLE_END) {
                TF_AXIOM(not sampleAttr.Get(&dummyUntypedValue, sample + 0.5));
                TF_AXIOM(sampleAttr.Get(&dummyUntypedValue, sample + 1.0));
            }
        }
      
    }

    printf(">>> Test SUCCEEDED\n");
}
