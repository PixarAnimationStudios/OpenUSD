//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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
    TF_AXIOM(coalesced.size() == 4);
    
    EmitWarnings();
    EmitStatuses();
    coalesced = delegate.TakeCoalescedDiagnostics();
    TF_AXIOM(coalesced.size() == 8);

    // ensure that the line numbers are unique
    std::set<size_t> sourceLineNumbers;
    for (const auto& p : coalesced) {
        sourceLineNumbers.insert(p.sharedItem.sourceLineNumber); 
    }
    TF_AXIOM(sourceLineNumbers.size() == 8); 

    EmitWarnings();
    EmitWarnings();
    auto unfiltered = delegate.TakeUncoalescedDiagnostics();
    TF_AXIOM(unfiltered.size() == 12);

    // ensure that the line numbers are not unique
    sourceLineNumbers.clear();
    for (const auto& i : unfiltered) {
        sourceLineNumbers.insert(i->GetSourceLineNumber());
    }
    TF_AXIOM(sourceLineNumbers.size() == 4); 

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
