//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_EXT_COMPUTATION_UTILS_H
#define PXR_IMAGING_HD_EXT_COMPUTATION_UTILS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/base/tf/span.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

#include <optional>
#include <unordered_map>

PXR_NAMESPACE_OPEN_SCOPE

using HdExtComputationConstPtr = HdExtComputation const *;
using HdExtComputationConstPtrVector = std::vector<HdExtComputationConstPtr>;

// This class contains utility methods to allow any Hydra backend to execute
// CPU computations via the Hydra ExtComputation framework.
// 
// Note:
// The computation execution happens during Rprim sync. This precludes the
// use of computations shared by multiple Rprims, since the chain of
// computations for a computation primvar is executed for each Rprim.
class HdExtComputationUtils {   
public:
    using ValueStore =
        std::unordered_map<TfToken, VtValue, TfToken::HashFunctor>;

    // Returns a map containing the (token, value) pairs for each "computation
    // primvar".
    // The participating computations are ordered based on their dependency
    // and then, the CPU kernel is executed for each computation.
    HD_API
    static ValueStore
    GetComputedPrimvarValues(
        HdExtComputationPrimvarDescriptorVector const& compPrimvars,
        HdSceneDelegate* sceneDelegate);

    template <unsigned int CAPACITY>
    using SampledValueStore =
        std::unordered_map<TfToken, HdTimeSampleArray<VtValue, CAPACITY>,
                           TfToken::HashFunctor>;

    /// Returns a map containing the (token, samples) pairs for each
    /// computation primvar, with up to \a maxSampleCount samples.
    /// The participating computations are ordered based on their dependency
    /// and then, the CPU kernel is executed for each computation.
    template <unsigned int CAPACITY>
    static void
    SampleComputedPrimvarValues(
        HdExtComputationPrimvarDescriptorVector const& compPrimvars,
        HdSceneDelegate* sceneDelegate,
        size_t maxSampleCount,
        SampledValueStore<CAPACITY> *computedPrimvarValueStore);

    /// Overload taking startTime and endTime explicitly.
    template <unsigned int CAPACITY>
    static void
    SampleComputedPrimvarValues(
        HdExtComputationPrimvarDescriptorVector const& compPrimvars,
        HdSceneDelegate* sceneDelegate,
        float startTime, float endTime,
        size_t maxSampleCount,
        SampledValueStore<CAPACITY> *computedPrimvarValueStore);

    // Helper methods (these are public for testing purposes)
    using ComputationDependencyMap =
        std::unordered_map<HdExtComputation const *,
                           HdExtComputationConstPtrVector>;
    // Returns true if an ordering of the computations wherein any dependencies
    // of a given computation come before it is possible, and fills
    // sortedComps with the ordering.
    // Returns false otherwise.
    // The directed graph of a computation (vertex) and its dependencies (edges)
    // is represented via the ComputationDependencyMap. 
    HD_API
    static bool
    DependencySort(ComputationDependencyMap cdm,
                   HdExtComputationConstPtrVector* sortedComps);

    HD_API
    static void
    PrintDependencyMap(ComputationDependencyMap const& cdm);

private:
    template <unsigned int CAPACITY>
    static void
    _SampleComputedPrimvarValues(
        HdExtComputationPrimvarDescriptorVector const& compPrimvars,
        HdSceneDelegate* sceneDelegate,
        std::optional<std::pair<float, float>> startAndEndTime,
        size_t maxSampleCount,
        SampledValueStore<CAPACITY> *computedPrimvarValueStore);

    HD_API
    static ComputationDependencyMap
    _GenerateDependencyMap(
        HdExtComputationPrimvarDescriptorVector const& compPrimvars,
        HdSceneDelegate* sceneDelegate);

    template <unsigned int CAPACITY>
    static void
    _ExecuteSampledComputations(
        HdExtComputationConstPtrVector computations,
        HdSceneDelegate* sceneDelegate,
        std::optional<std::pair<float, float>> startAndEndTime,
        size_t maxSampleCount,
        SampledValueStore<CAPACITY>* valueStore);

    // Limits the list of the computation input time samples to the specified
    // maximum number of (unique) samples.
    HD_API
    static void
    _LimitTimeSamples(size_t maxSampleCount, std::vector<double>* times);

    // Internal method to invoke the computation with the specified input
    // values, storing the output values in the provided buffer. The value
    // arrays correspond to GetSceneInputNames(), GetComputationInputs(), and
    // GetComputationOutputs() from the HdExtComputation, respectively, and are
    // required to have the same lengths.
    HD_API
    static bool
    _InvokeComputation(
        HdSceneDelegate& sceneDelegate,
        HdExtComputation const& computation,
        TfSpan<const VtValue> sceneInputValues,
        TfSpan<const VtValue> compInputValues,
        TfSpan<VtValue> compOutputValues);
};

template <unsigned int CAPACITY>
/*static*/ void
HdExtComputationUtils::SampleComputedPrimvarValues(
    HdExtComputationPrimvarDescriptorVector const& compPrimvars,
    HdSceneDelegate* sceneDelegate,
    size_t maxSampleCount,
    SampledValueStore<CAPACITY> *computedPrimvarValueStore
)
{
    _SampleComputedPrimvarValues<CAPACITY>(
        compPrimvars,
        sceneDelegate,
        /* startAndEndTime = */ std::nullopt,
        maxSampleCount,
        computedPrimvarValueStore);
}

template <unsigned int CAPACITY>
/*static*/ void
HdExtComputationUtils::SampleComputedPrimvarValues(
    HdExtComputationPrimvarDescriptorVector const& compPrimvars,
    HdSceneDelegate* sceneDelegate,
    float startTime, float endTime,
    size_t maxSampleCount,
    SampledValueStore<CAPACITY> *computedPrimvarValueStore
)
{
    _SampleComputedPrimvarValues<CAPACITY>(
        compPrimvars,
        sceneDelegate,
        { {startTime, endTime }},
        maxSampleCount,
        computedPrimvarValueStore);
}

template <unsigned int CAPACITY>
/*static*/ void
HdExtComputationUtils::_SampleComputedPrimvarValues(
    HdExtComputationPrimvarDescriptorVector const& compPrimvars,
    HdSceneDelegate* sceneDelegate,
    std::optional<std::pair<float, float>> startAndEndTime,
    size_t maxSampleCount,
    SampledValueStore<CAPACITY> *computedPrimvarValueStore
)
{
    HD_TRACE_FUNCTION();

    // Directed graph representation of the participating computations
    ComputationDependencyMap cdm =
        _GenerateDependencyMap(compPrimvars, sceneDelegate);

    // Topological ordering of the computations
    HdExtComputationConstPtrVector sortedComputations;
    bool success = DependencySort(cdm, &sortedComputations);
    if (!success) {
        return;
    }

    // Execution
    SampledValueStore<CAPACITY> valueStore;
    _ExecuteSampledComputations<CAPACITY>(sortedComputations, sceneDelegate,
                                          startAndEndTime,
                                          maxSampleCount,
                                          &valueStore);

    // Output extraction
    for (auto const& pv : compPrimvars) {
        TfToken const& compOutputName = pv.sourceComputationOutputName;
        (*computedPrimvarValueStore)[pv.name] = valueStore[compOutputName];
    }
}

template <unsigned int CAPACITY>
/*static*/ void
HdExtComputationUtils::_ExecuteSampledComputations(
    HdExtComputationConstPtrVector computations,
    HdSceneDelegate* sceneDelegate,
    std::optional<std::pair<float, float>> startAndEndTime,
    size_t maxSampleCount,
    SampledValueStore<CAPACITY> *valueStore
)
{
    HD_TRACE_FUNCTION();

    for (auto const& comp : computations) {
        SdfPath const& compId = comp->GetId();

        TfTokenVector const& sceneInputNames = comp->GetSceneInputNames();
        HdExtComputationInputDescriptorVector const& compInputs =
            comp->GetComputationInputs();
        HdExtComputationOutputDescriptorVector const& compOutputs =
            comp->GetComputationOutputs();

        // Add all the scene inputs to the value store
        std::vector<double> times;
        for (TfToken const& input : sceneInputNames) {
            auto &samples = (*valueStore)[input];
            if (startAndEndTime) {
                sceneDelegate->SampleExtComputationInput(
                    compId, input,
                    startAndEndTime->first, startAndEndTime->second,
                    &samples);
            } else {
                sceneDelegate->SampleExtComputationInput(
                    compId, input,
                    &samples);
            }

            for (size_t i = 0; i < samples.count; ++i)
                times.push_back(samples.times[i]);
        }

        if (comp->IsInputAggregation()) {
            // An aggregator computation produces no output, and thus
            // doesn't need to be executed.
            continue;
        }

        // Also find all the time samples from the computed inputs.
        for (auto const& computedInput : compInputs) {
            auto const& samples =
                valueStore->at(computedInput.sourceComputationOutputName);
            for (size_t i = 0; i < samples.count; ++i) {
                times.push_back(samples.times[i]);
            }
        }

        // Determine the time samples to evaluate the computation at.
        _LimitTimeSamples(maxSampleCount, &times);

        // Allocate enough space for the evaluated outputs.
        for (const TfToken &name : comp->GetOutputNames())
        {
            auto &output_samples = (*valueStore)[name];
            output_samples.Resize(times.size());
            output_samples.count = 0;
        }

        TfSmallVector<VtValue, CAPACITY> sceneInputValues;
        sceneInputValues.reserve(sceneInputNames.size());

        TfSmallVector<VtValue, CAPACITY> compInputValues;
        compInputValues.reserve(compInputs.size());

        TfSmallVector<VtValue, CAPACITY> compOutputValues;

        // Evaluate the computation for each time sample.
        for (double t : times) {

            // Retrieve all the inputs (scene, computed) from the value store,
            // resampled to the required time.
            sceneInputValues.clear();
            for (auto const& sceneInput : comp->GetSceneInputNames()) {
                auto const& samples = valueStore->at(sceneInput);
                sceneInputValues.push_back(samples.Resample(t));
            }

            compInputValues.clear();
            for (auto const& computedInput : compInputs) {
                auto const& samples =
                    valueStore->at(computedInput.sourceComputationOutputName);
                compInputValues.push_back(samples.Resample(t));
            }

            compOutputValues.resize(compOutputs.size());
            if (!_InvokeComputation(*sceneDelegate, *comp,
                                    TfMakeSpan(sceneInputValues),
                                    TfMakeSpan(compInputValues),
                                    TfMakeSpan(compOutputValues))) {
                // We could bail here, or choose to execute other computations.
                // Choose the latter.
                continue;
            }

            // Add outputs to the value store (subsequent computations may need
            // them as computation inputs)
            for (size_t i = 0; i < compOutputValues.size(); ++i) {
                auto &output_samples = (*valueStore)[compOutputs[i].name];

                output_samples.times[output_samples.count] = t;
                output_samples.values[output_samples.count] =
                    std::move(compOutputValues[i]);
                ++output_samples.count;
            }
        }

    } // for each computation
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_EXT_COMPUTATION_UTILS_H
