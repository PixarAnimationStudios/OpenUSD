//
// Copyright 2020 Pixar
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

#include "pxr/base/tf/errorMark.h"

#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/extComputationContext.h"
#include "pxr/imaging/hd/extComputationUtils.h"
#include "pxr/imaging/hd/renderDelegate.h"
#include "pxr/imaging/hd/unitTestDelegate.h"

#include <iostream>

PXR_NAMESPACE_USING_DIRECTIVE

static const SdfPath pathA("/path/to/A");
static const SdfPath compA("/path/to/A/computation");
static const TfToken input1("input1");
static const TfToken input2("input2");
static const TfToken primvarName("outputPV");
static const TfToken compOutputName("compOutput");

// Delegate that implements a simple computation (adding together two inputs).
class ExtComputationTestDelegate : public HdUnitTestDelegate {
public:
    ExtComputationTestDelegate(HdRenderIndex *parentIndex)
      : HdUnitTestDelegate(parentIndex, SdfPath::AbsoluteRootPath())
    {
    }

    HdExtComputationPrimvarDescriptorVector
    virtual GetExtComputationPrimvarDescriptors(
        SdfPath const &id, HdInterpolation interpolationMode) override {

        if (id == pathA && interpolationMode == HdInterpolationConstant) {
            HdTupleType valueType;
            valueType.type = HdTypeFloat;
            valueType.count = 1;

            HdExtComputationPrimvarDescriptorVector primvars;
            primvars.emplace_back(primvarName, HdInterpolationConstant,
                    HdPrimvarRoleTokens->none, compA, compOutputName,
                    valueType);
            return primvars;
        }

        return {};
    }

    virtual TfTokenVector
    GetExtComputationSceneInputNames(SdfPath const& computationId) override {
        if (computationId == compA) {
            return {input1, input2};
        }

        return {};
    }

    virtual HdExtComputationOutputDescriptorVector
    GetExtComputationOutputDescriptors(SdfPath const& computationId) override {
        HdExtComputationOutputDescriptorVector outputs;
        if (computationId == compA) {
            HdTupleType valueType;
            valueType.type = HdTypeFloat;
            valueType.count = 1;
            outputs.emplace_back(compOutputName, valueType);
        }

        return outputs;
    }

    virtual size_t SampleExtComputationInput(SdfPath const& computationId,
                                             TfToken const& input,
                                             size_t maxSampleCount,
                                             float *sampleTimes,
                                             VtValue *sampleValues) override {
        if (computationId != compA) {
            return 0;
        }

        const size_t numSamples = std::min(size_t(4), maxSampleCount);

        // The two inputs have different sample times (0,1,2,3 and 0,2,4,6).
        if (input == input1) {
            for (size_t i = 0; i < numSamples; ++i) {
                sampleTimes[i] = i;
                sampleValues[i] = VtValue(double(i));
            }
            return numSamples;
        }
        else if (input == input2) {
            for (size_t i = 0; i < numSamples; ++i) {
                sampleTimes[i] = i * 2;
                sampleValues[i] = VtValue(double(i));
            }
            return numSamples;
        }

        return 0;
    }

    virtual void InvokeExtComputation(SdfPath const& computationId,
                                      HdExtComputationContext *context) override {
        if (computationId != compA) {
            return;
        }

        VtValue val1 = context->GetInputValue(input1);
        VtValue val2 = context->GetInputValue(input2);
        context->SetOutputValue(
            compOutputName, VtValue(val1.Get<double>() + val2.Get<double>()));
    }
};

// Mock render delegate for testing - just handles the ExtComputation sprims.
class ExtCompTestRenderDelegate : public HdRenderDelegate {
private:
    static const TfTokenVector _emptyTypes;
    static const TfTokenVector _sprimTypes;

public:
    virtual HdResourceRegistrySharedPtr GetResourceRegistry() const override {
        return nullptr;
    }

    virtual HdRenderPassSharedPtr CreateRenderPass(
                                HdRenderIndex *index,
                                HdRprimCollection const& collection) override {
        return nullptr;
    }

    virtual HdInstancer *CreateInstancer(HdSceneDelegate *delegate,
                                         SdfPath const& id) override {
        return nullptr;
    }
    virtual void DestroyInstancer(HdInstancer *instancer) override {
    }

    virtual HdRprim *CreateRprim(TfToken const& typeId,
                                 SdfPath const& rprimId) override {
        return nullptr;
    }
    virtual void DestroyRprim(HdRprim *rPrim) override {
    }

    virtual HdSprim *CreateSprim(TfToken const& typeId,
                                 SdfPath const& sprimId) override {
        if (typeId == HdPrimTypeTokens->extComputation) {
            return new HdExtComputation(sprimId);
        } else {
            TF_CODING_ERROR("Unknown Sprim Type %s", typeId.GetText());
        }

        return nullptr;
    }
    virtual HdSprim *CreateFallbackSprim(TfToken const& typeId) override {
        return nullptr;
    }
    virtual void DestroySprim(HdSprim *sprim) override {
        delete sprim;
    }

    virtual HdBprim *CreateBprim(TfToken const& typeId,
                                 SdfPath const& bprimId) override {
        return nullptr;
    }
    virtual HdBprim *CreateFallbackBprim(TfToken const& typeId) override {
        return nullptr;
    }
    virtual void DestroyBprim(HdBprim *bPrim) override {
    }

    virtual void CommitResources(HdChangeTracker *tracker) override {
    }

    virtual const TfTokenVector &GetSupportedRprimTypes() const override {
        return _emptyTypes;
    }
    virtual const TfTokenVector &GetSupportedSprimTypes() const override {
        return _sprimTypes;
    }
    virtual const TfTokenVector &GetSupportedBprimTypes() const override {
        return _emptyTypes;
    }
};

const TfTokenVector ExtCompTestRenderDelegate::_emptyTypes;
const TfTokenVector ExtCompTestRenderDelegate::_sprimTypes = {
    HdPrimTypeTokens->extComputation
};

void RunTest()
{
    ExtCompTestRenderDelegate renderDelegate;
    std::unique_ptr<HdRenderIndex> index(
        HdRenderIndex::New(&renderDelegate, {}));
    ExtComputationTestDelegate delegate(index.get());

    // Create an sprim for the computation.
    index->InsertSprim(HdPrimTypeTokens->extComputation, &delegate, compA);
    auto sprim = index->GetSprim(HdPrimTypeTokens->extComputation, compA);
    HdDirtyBits dirty = HdExtComputation::DirtyBits::AllDirty;
    sprim->Sync(&delegate, nullptr, &dirty);

    auto compPrimvars = delegate.GetExtComputationPrimvarDescriptors(
        pathA, HdInterpolationConstant);

    // Evaluate the computation, and verify the output sample times and values.
    HdExtComputationUtils::SampledValueStore<4> valueStore;
    const size_t maxSamples = 5;
    HdExtComputationUtils::SampleComputedPrimvarValues(
        compPrimvars, &delegate, maxSamples, &valueStore);

    if (valueStore.size() != 1) {
        TF_RUNTIME_ERROR("Incorrect number of computed primvars %d",
                         static_cast<int>(valueStore.size()));
        return;
    }

    auto &&pvSamples = valueStore.at(primvarName);
    if (pvSamples.count != maxSamples) {
        TF_RUNTIME_ERROR("Unexpected number of samples %d",
                         static_cast<int>(pvSamples.count));
        return;
    }

#define CHECK_SAMPLE(index, time, val)                                \
    if (pvSamples.times[index] != time) {                             \
        TF_RUNTIME_ERROR("Unexpected sample time %f vs %f",           \
                         pvSamples.times[index], time);               \
        return;                                                       \
    }                                                                 \
    if (pvSamples.values[index].Get<double>() != val) {               \
        TF_RUNTIME_ERROR("Unexpected sample value %f vs %f",          \
                         pvSamples.values[index].Get<double>(), val); \
        return;                                                       \
    }

    CHECK_SAMPLE(0, 0.f, 0.0);
    CHECK_SAMPLE(1, 1.f, 1.5);
    CHECK_SAMPLE(2, 2.f, 3.0);
    CHECK_SAMPLE(3, 3.f, 4.5);
    CHECK_SAMPLE(4, 4.f, 5.0);
}

int main(int argc, char *argv[])
{
    TfErrorMark mark;

    RunTest();

    // If no error messages were logged, return success.
    if (mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cerr << "FAILED" << std::endl;
        TfReportActiveErrorMarks();
        return EXIT_FAILURE;
    }
}
