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
#ifndef USDUTILS_COALESCING_DIAGNOSTIC_DELEGATE_H
#define USDUTILS_COALESCING_DIAGNOSTIC_DELEGATE_H

/// \file usdUtils/coalescingDiagnosticDelegate.h
///
/// A class which provides aggregation of warnings and statuses
/// emitted from Tf's diagnostic management system. These diagnostic
/// notifications can be coalesced by invocation point,
/// currently defined as the source file, line number and function name,
/// to recieve a more concise output.

#include "pxr/pxr.h"
#include "pxr/usd/usdUtils/api.h"
#include "pxr/base/tf/diagnosticMgr.h"

#include <ostream>
#include <memory>
#include <string>
#include <vector>

#include <tbb/concurrent_queue.h>

PXR_NAMESPACE_OPEN_SCOPE

/// The shared component in a coalesced result
/// This type can be thought of as the key by which we
/// coalesce our diagnostics.
struct UsdUtilsCoalescingDiagnosticDelegateSharedItem {
    size_t sourceLineNumber;
    std::string sourceFunction;
    std::string sourceFileName;
};

/// The unshared component in a coalesced result 
struct UsdUtilsCoalescingDiagnosticDelegateUnsharedItem {
    TfCallContext context;
    std::string commentary;
};

/// An item used in coalesced results, containing a shared component:
/// the file/function/line number, and a set of unshared components: the
/// call context and commentary.
struct UsdUtilsCoalescingDiagnosticDelegateItem {
    UsdUtilsCoalescingDiagnosticDelegateSharedItem sharedItem;
    std::vector<UsdUtilsCoalescingDiagnosticDelegateUnsharedItem> unsharedItems;
};

/// A vector of coalesced results, each containing a shared component,
/// the file/function/line number, and a set of unshared components, the
/// call context and commentary.
typedef std::vector<UsdUtilsCoalescingDiagnosticDelegateItem>
        UsdUtilsCoalescingDiagnosticDelegateVector;

/// A class which collects warnings and statuses from the 
/// Tf diagnostic manager system in a thread safe manner.
///
/// This class allows clients to get both the unfiltered 
/// results, as well as a compressed view which deduplicates
/// diagnostic events by their source line number, function and file 
/// from which they occurred.
class UsdUtilsCoalescingDiagnosticDelegate : public TfDiagnosticMgr::Delegate {
public:
    USDUTILS_API
    UsdUtilsCoalescingDiagnosticDelegate();

    USDUTILS_API
    virtual ~UsdUtilsCoalescingDiagnosticDelegate();

    /// Methods that implement the interface provided in TfDiagnosticMgr::Delegate
    
    USDUTILS_API
    virtual void IssueError(const TfError&) override; 

    USDUTILS_API
    virtual void IssueStatus(const TfStatus&) override; 

    USDUTILS_API
    virtual void IssueWarning(const TfWarning&) override; 

    USDUTILS_API
    virtual void IssueFatalError(const TfCallContext&, const std::string &) override; 

    // Methods that provide collection of diagnostics as well as sending
    // them to a stream(stderr, stdout, a file, etc).

    /// Print all pending diagnostics in a coalesced form to \p ostr
    /// \note This method clears the pending diagnostics.
    USDUTILS_API
    void DumpCoalescedDiagnostics(std::ostream& ostr);

    /// Print all pending diagnostics without any coalescing to \p ostr
    /// \note This method clears the pending diagnostics.
    USDUTILS_API
    void DumpUncoalescedDiagnostics(std::ostream& ostr);

    /// Get all pending diagnostics in a coalesced form.
    /// \note This method clears the pending diagnostics.
    USDUTILS_API
    UsdUtilsCoalescingDiagnosticDelegateVector TakeCoalescedDiagnostics();

    /// Get all pending diagnostics without any coalescing.
    /// \note This method clears the pending diagnostics.
    USDUTILS_API
    std::vector<std::unique_ptr<TfDiagnosticBase>> TakeUncoalescedDiagnostics(); 

private:
    tbb::concurrent_queue<TfDiagnosticBase*> _diagnostics; 
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // USDUTILS_COALESCING_DIAGNOSTIC_DELEGATE_H
