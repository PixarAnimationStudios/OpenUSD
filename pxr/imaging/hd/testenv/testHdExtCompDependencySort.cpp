//
// Copyright 2021 Pixar
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
#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/extComputationUtils.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/errorMark.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

using HdExtComputationSharedPtr = std::shared_ptr<HdExtComputation>;

void PrintComputations(HdExtComputationConstPtrVector const& comps,
                       std::string const& prefix)
{
    std::cout << std::endl << prefix << " Computation Order: ";
    for (auto const& comp : comps) {
        std::cout << comp->GetId() << ", ";
    }
    std::cout << std::endl;
}

bool OccursBefore(HdExtComputationConstPtrVector const& comps,
                  HdExtComputationConstPtr comp1,
                  HdExtComputationConstPtr comp2)
{
    auto it1 = std::find(comps.begin(), comps.end(), comp1);
    auto it2 = std::find(comps.begin(), comps.end(), comp2);

    return (it1 < it2);
}

bool TestLinearChainDependency()
{
    // Simple linear chain of computations:
    // A <-- B <-- C
    // Read as A depends on B, B depends on C, C does not depend on anything.
    // i.e., A takes as input one or more output(s) of B
    //       B takes as input one or more output(s) of C
    HdExtComputationSharedPtr compA(new HdExtComputation(SdfPath("A")));
    HdExtComputationSharedPtr compB(new HdExtComputation(SdfPath("B")));
    HdExtComputationSharedPtr compC(new HdExtComputation(SdfPath("C")));
    HdExtComputationUtils::ComputationDependencyMap cdm;
    cdm[compA.get()] = {compB.get()};
    cdm[compB.get()] = {compC.get()};
    cdm[compC.get()] = {};

    HdExtComputationUtils::PrintDependencyMap(cdm);

    HdExtComputationConstPtrVector expectedOrder
        = {compC.get(), compB.get(), compA.get()};
    PrintComputations(expectedOrder, "Expected");

    HdExtComputationConstPtrVector sortedComps;
    bool success = HdExtComputationUtils::DependencySort(cdm, &sortedComps);
    
    PrintComputations(sortedComps, "Sorted");

    return (success && (sortedComps == expectedOrder));
}

bool TestTreeChainDependency()
{
    // Tree chain of computations:
    // A <-- B <-- C
    // ^     ^ 
    // |     '-- D <-- E
    // '-- F
    // Read as A depends on B and F,
    //         B depends on C and D,
    //         D depends on E
    //         C, E and F do not depend on anything.
    HdExtComputationSharedPtr compA(new HdExtComputation(SdfPath("A")));
    HdExtComputationSharedPtr compB(new HdExtComputation(SdfPath("B")));
    HdExtComputationSharedPtr compC(new HdExtComputation(SdfPath("C")));
    HdExtComputationSharedPtr compD(new HdExtComputation(SdfPath("D")));
    HdExtComputationSharedPtr compE(new HdExtComputation(SdfPath("E")));
    HdExtComputationSharedPtr compF(new HdExtComputation(SdfPath("F")));
    HdExtComputationUtils::ComputationDependencyMap cdm;
    cdm[compA.get()] = {compB.get(), compF.get()};
    cdm[compB.get()] = {compC.get(), compD.get()};
    cdm[compD.get()] = {compE.get()};
    cdm[compC.get()] = {};
    cdm[compE.get()] = {};
    cdm[compF.get()] = {};
    
    HdExtComputationUtils::PrintDependencyMap(cdm);

    HdExtComputationConstPtrVector sortedComps;
    bool success = HdExtComputationUtils::DependencySort(cdm, &sortedComps);
    PrintComputations(sortedComps, "Sorted");

    // We can't compare with an "expected ordering" since it isn't a simple
    // linear chain. Just ensure depdendencies are handled.
    return success
           && OccursBefore(sortedComps, compF.get(), compA.get())
           && OccursBefore(sortedComps, compC.get(), compB.get())
           && OccursBefore(sortedComps, compE.get(), compB.get())
           && OccursBefore(sortedComps, compC.get(), compB.get());
}

bool TestCycleDependency()
{
    // Chain of computations with a cycle:
    // A <-- B  -->  C
    // ^     ^       |     
    // |     '       v
    //       '------ D  <-- E
    // '-- F
    // Read as A depends on B and F,
    //         B depends on D,
    //         C depends on B,
    //         D depends on C and E
    //         E and F do not depend on anything.
    HdExtComputationSharedPtr compA(new HdExtComputation(SdfPath("A")));
    HdExtComputationSharedPtr compB(new HdExtComputation(SdfPath("B")));
    HdExtComputationSharedPtr compC(new HdExtComputation(SdfPath("C")));
    HdExtComputationSharedPtr compD(new HdExtComputation(SdfPath("D")));
    HdExtComputationSharedPtr compE(new HdExtComputation(SdfPath("E")));
    HdExtComputationSharedPtr compF(new HdExtComputation(SdfPath("F")));
    HdExtComputationUtils::ComputationDependencyMap cdm;
    cdm[compA.get()] = {compB.get(), compF.get()};
    cdm[compB.get()] = {compD.get()};
    cdm[compC.get()] = {compB.get()};
    cdm[compD.get()] = {compC.get(), compE.get()};
    cdm[compE.get()] = {};
    cdm[compF.get()] = {};
    
    HdExtComputationUtils::PrintDependencyMap(cdm);

    HdExtComputationConstPtrVector sortedComps;
    bool success = HdExtComputationUtils::DependencySort(cdm, &sortedComps);

    // We expect the dependency sort to fail.
    return !success;
}


int main()
{
    TfErrorMark mark;

    bool success  = TestLinearChainDependency()
                    && TestTreeChainDependency()
                    && TestCycleDependency();

    TF_VERIFY(mark.IsClean());

    if (success && mark.IsClean()) {
        std::cout << "OK" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cout << "FAILED" << std::endl;
        return EXIT_FAILURE;
    }
}
