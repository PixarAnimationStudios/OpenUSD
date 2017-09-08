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
#include "pxr/usd/sdf/types.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/propertySpec.h"

#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usd/attribute.h"
#include "pxr/usd/usd/attributeQuery.h"
#include "pxr/usd/usd/references.h"

#include <cstdlib>
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
    TF_AXIOM(attr.HasAuthoredValueOpinion());
}

template <typename T>
void
_CheckDefaultBlocked(UsdAttribute& attr)
{
    T value;
    VtValue untypedValue;
    UsdAttributeQuery query(attr);

    TF_AXIOM(!attr.Get<T>(&value));
    TF_AXIOM(!query.Get<T>(&value));
    TF_AXIOM(!attr.Get(&untypedValue));
    TF_AXIOM(!query.Get(&untypedValue));
    TF_AXIOM(!attr.HasValue());
    TF_AXIOM(attr.HasAuthoredValueOpinion());
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
        std::cout << "+------------------------------------------+" << std::endl;
    }

    printf("\n\n>>> Test SUCCEEDED\n");
}
