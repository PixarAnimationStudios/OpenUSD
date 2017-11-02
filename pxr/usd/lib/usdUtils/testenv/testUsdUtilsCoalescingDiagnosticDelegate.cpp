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
#include "pxr/base/tf/diagnostic.h"
#include "pxr/usd/usdUtils/coalescingDiagnosticDelegate.h"

#include <iostream>
#include <cassert>
#include <set>

PXR_NAMESPACE_USING_DIRECTIVE

// Here we emit some warnings on the same line, so they 
// will get coalesced, and others on different lines, so they wont
// We expect to get 4 results when coalesced, 6 uncoalesced
static void 
EmitWarnings() {
    TF_WARN("aaaaaaaaaaaaaa"); TF_WARN("bbbbbbbbbbbbbb"); 
    TF_WARN("cccccccccccccc");               
    TF_WARN("dddddddddddddd");               
    TF_WARN("eeeeeeeeeeeeee"); TF_WARN("ffffffffffffff");
}

// The same helper, but for statuses
static void
EmitStatuses() {
    TF_STATUS("."); TF_STATUS("."); 
    TF_STATUS(".");               
    TF_STATUS(".");               
    TF_STATUS("."); TF_STATUS(".");
}

int main() {
    UsdUtilsCoalescingDiagnosticDelegate delegate;

    EmitWarnings();
    auto coalesced = delegate.TakeCoalescedDiagnostics();
    assert(coalesced.size() == 4);
    
    EmitWarnings();
    EmitStatuses();
    coalesced = delegate.TakeCoalescedDiagnostics();
    assert(coalesced.size() == 8);

    // ensure that the line numbers are unique
    std::set<size_t> sourceLineNumbers;
    for (const auto& p : coalesced) {
        sourceLineNumbers.insert(p.sharedItem.sourceLineNumber); 
    }
    assert(sourceLineNumbers.size() == 8); 

    EmitWarnings();
    EmitWarnings();
    auto unfiltered = delegate.TakeUncoalescedDiagnostics();
    assert(unfiltered.size() == 12);

    // ensure that the line numbers are not unique
    sourceLineNumbers.clear();
    for (const auto& i : unfiltered) {
        sourceLineNumbers.insert(i->GetSourceLineNumber());
    }
    assert(sourceLineNumbers.size() == 4); 

    std::cout << "-------------------------------------------\n";
    EmitWarnings();
    EmitWarnings();
    EmitStatuses();
    EmitStatuses();
    EmitWarnings();
    delegate.DumpCoalescedDiagnostics(std::cout);

    std::cout << "-------------------------------------------\n";

    EmitWarnings();
    EmitWarnings();
    EmitStatuses();
    EmitStatuses();
    EmitWarnings();
    delegate.DumpUncoalescedDiagnostics(std::cout);

    std::cout << "-------------------------------------------\n";
}
