//
// Copyright 2019 Pixar
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

#include "pxr/imaging/hd/extComputationUtils.h"

#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/extComputationContextInternal.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/base/trace/trace.h"

#include <deque>
#include <iostream>

PXR_NAMESPACE_OPEN_SCOPE

namespace {

static HdExtComputationUtils::ComputationDependencyMap
_GenerateDependencyMap(
    HdExtComputationPrimvarDescriptorVector const& compPrimvars,
    HdSceneDelegate* sceneDelegate)
{
    HD_TRACE_FUNCTION();

    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();

    using CompQueue = std::deque<HdExtComputation const *>;
    CompQueue computations;

    // First discover the computation tied to the computation primvars.
    for (auto const& pv : compPrimvars) {
        HdExtComputation const * sourceComp =
            static_cast<HdExtComputation const *>(
                renderIndex.GetSprim(HdPrimTypeTokens->extComputation,
                                     pv.sourceComputationId));

        // We can have different computation primvars using the same computation
        // (since it can produce multiple outputs). It may thus be added more
        // than once, but that's okay since we guard against it during the
        // traversal (below).
        computations.emplace_back(sourceComp);
    }

    // Traverse the computation graph starting from the above computation(s)
    // At each step, we pop a node, exit early if it has already been processed,
    // discover its dependent comps and add them to the queue to be visited.
    HdExtComputationUtils::ComputationDependencyMap cdm;
    while (!computations.empty()) {
        // Pop head entry and skip if already processed.
        HdExtComputation const * curComp = computations.back();
        computations.pop_front();
        if (cdm.find(curComp) != cdm.end()) {
            continue;
        }

        // Insert (empty) dependency map entry for the current computation
        auto& entry = cdm[curComp];

        // Find dependent computations.
        HdExtComputationInputDescriptorVector const& compInputs =
                curComp->GetComputationInputs();
        // Get unique dependent computation paths (to reduce the number of
        // render index lookups)
        std::unordered_set<SdfPath, SdfPath::Hash> dependentCompPaths;
        for (auto const& input : compInputs) {
            dependentCompPaths.insert(input.sourceComputationId);
        }
        // Update dependency map entry, and add the dep. comp to the queue.
        for (auto const& depCompPath : dependentCompPaths) {
            HdExtComputation const * depComp =
                static_cast<HdExtComputation const *>(
                    renderIndex.GetSprim(HdPrimTypeTokens->extComputation,
                                         depCompPath));
            
            entry.emplace_back(depComp);
            computations.emplace_back(depComp);
        }
    }

    return cdm;
}

static HdExtComputationUtils::ValueStore
_ExecuteComputations(HdExtComputationConstPtrVector computations,
                     HdSceneDelegate* sceneDelegate)
{
    HD_TRACE_FUNCTION();

    HdExtComputationUtils::ValueStore valueStore;
    for (auto const& comp : computations) {
        SdfPath const& compId = comp->GetId();

        // Add all the scene inputs to the value store
        for (TfToken const& input : comp->GetSceneInputNames()) {
            valueStore[input] = sceneDelegate->GetExtComputationInput(
                                    compId, input);
        }

        if (comp->IsInputAggregation()) {
            // An aggregator computation produces no output, and thus
            // doesn't need to be executed.
            continue;
        }

        // Populate the context with all the inputs (scene, computed) from
        // the value store.
        Hd_ExtComputationContextInternal context;
        for (auto const& sceneInput : comp->GetSceneInputNames()) {
            context.SetInputValue(sceneInput, valueStore.at(sceneInput));
        }

        for (auto const& computedInput : comp->GetComputationInputs()) {
            context.SetInputValue(computedInput.name,
                valueStore.at(computedInput.sourceComputationOutputName));
        }

        sceneDelegate->InvokeExtComputation(compId, &context);
        
        if (context.HasComputationError()) {
            // We could bail here, or choose to execute other computations.
            // Choose the latter.
            TF_WARN("Error invoking computation %s.\n", compId.GetText());
        } else {
            // Add outputs to the value store (subsequent computations may need
            // them as computation inputs)
            TfTokenVector const& outputNames = comp->GetOutputNames();
            for (auto const& name : outputNames) {
                VtValue value;
                if (!context.GetOutputValue(name, &value)) {
                    TF_WARN("Error getting out %s for computation %s.\n",
                            name.GetText(), compId.GetText());
                } else {
                    valueStore[name] = value;
                }
            }
        }
    } // for each computation

    return valueStore;
}

};

/*static*/HdExtComputationUtils::ValueStore
HdExtComputationUtils::GetComputedPrimvarValues(
    HdExtComputationPrimvarDescriptorVector const& compPrimvars,
    HdSceneDelegate* sceneDelegate
)
{
    HD_TRACE_FUNCTION();

    // Directed graph representation of the participating computations
    HdExtComputationUtils::ComputationDependencyMap cdm
        = _GenerateDependencyMap(compPrimvars, sceneDelegate);

    // Topological ordering of the computations
    HdExtComputationConstPtrVector sortedComputations;
    bool success = DependencySort(cdm, &sortedComputations);

    if (!success) {
        return HdExtComputationUtils::ValueStore();
    }

    // Execution
    HdExtComputationUtils::ValueStore valueStore =
        _ExecuteComputations(sortedComputations, sceneDelegate);

    // Output extraction
    HdExtComputationUtils::ValueStore computedPrimvarValueStore;
    for (auto const& pv : compPrimvars) {
        TfToken const& compOutputName = pv.sourceComputationOutputName;

        computedPrimvarValueStore[pv.name] = valueStore[compOutputName];
    }

    return computedPrimvarValueStore;
}

/*static*/bool
HdExtComputationUtils::DependencySort(
    HdExtComputationUtils::ComputationDependencyMap cdm,
    HdExtComputationConstPtrVector* sortedComps)
{
    HD_TRACE_FUNCTION();
    if (!sortedComps) {
        TF_CODING_ERROR("Received nullptr for sortedComps.");
        return false;
    }
    
    // Kahn's topological sorting algorithm
    using CompQueue = std::deque<HdExtComputation const *>;
    CompQueue independentComps;
    for (auto const& pair : cdm) {
        if (pair.second.empty()) {
            independentComps.emplace_back(pair.first);
            cdm.erase(pair.first);
        }
    }

    while (!independentComps.empty()) {
        HdExtComputation const * indComp = independentComps.back();
        sortedComps->emplace_back(indComp);
        independentComps.pop_back();

        // Remove dependency edge from computations that depend on comp.
        for (auto& pair : cdm) {
            auto const& depComp = pair.first;
            auto& dependencies = pair.second;
            auto it = std::find(dependencies.begin(), dependencies.end(),
                                indComp);
            if (it != dependencies.end()) {
                dependencies.erase(it);
            }

            if (dependencies.empty()) {
                // Add the computation to the indepdendent list, since its
                // dependencies have been resolved.
                independentComps.emplace_front(depComp);

                // Remove it from the graph, so we don't revisit it again.
                cdm.erase(depComp);
            }
        }
    }

    if (!cdm.empty()) {
        TF_WARN("Cycle detected in ExtComputation dependency graph. "
                "Unresolved dependencies:\n");
        if (TfDebug::IsEnabled(HD_EXT_COMPUTATION_EXECUTION)) {
            PrintDependencyMap(cdm);
        }
        sortedComps->clear();
        return false;
    }

    return true;
}

/*static*/void
HdExtComputationUtils::PrintDependencyMap(
    HdExtComputationUtils::ComputationDependencyMap const& cdm)
{
    std::cout << "Computations dep map" << std::endl;
    for (auto const& pair : cdm) {
        std::cout << pair.first->GetId() << " -> [ ";
        for (auto const& depComp : pair.second) {
            std::cout << depComp->GetId() << ", ";
        }
        std::cout << " ]" << std::endl;
    }
    std::cout << std::endl;
}

PXR_NAMESPACE_CLOSE_SCOPE
