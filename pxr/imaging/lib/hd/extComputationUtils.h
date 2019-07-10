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
#ifndef HD_EXT_COMPUTATION_UTILS_H
#define HD_EXT_COMPUTATION_UTILS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/base/tf/token.h"
#include "pxr/base/vt/value.h"

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
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_EXT_COMPUTATION_UTILS_H
