//
// Copyright 2018 Pixar
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
#ifndef PXRUSDMAYA_DIAGNOSTICDELEGATE_H
#define PXRUSDMAYA_DIAGNOSTICDELEGATE_H

/// \file usdMaya/diagnosticDelegate.h

#include "pxr/pxr.h"
#include "usdMaya/api.h"

#include "pxr/base/tf/diagnosticMgr.h"
#include "pxr/usd/usdUtils/coalescingDiagnosticDelegate.h"

#include <boost/noncopyable.hpp>
#include <maya/MGlobal.h>

#include <atomic>
#include <memory>

PXR_NAMESPACE_OPEN_SCOPE

class UsdMayaDiagnosticBatchContext;

/// Converts Tf diagnostics into native Maya infos, warnings, and errors.
///
/// Provides an optional batching mechanism for diagnostics; see
/// UsdMayaDiagnosticBatchContext for more information. Note that errors
/// are never batched.
///
/// The IssueError(), IssueStatus(), etc. functions are thread-safe, since Tf
/// may issue diagnostics from secondary threads. Note that, when not batching,
/// secondary threads' diagnostic messages are posted to stderr instead of to
/// the Maya script window. When batching, secondary threads' diagnostic
/// messages will be posted by the main thread to the Maya script window when
/// batching ends.
///
/// Installing and removing this diagnostic delegate is not thread-safe, and
/// must be done only on the main thread.
class UsdMayaDiagnosticDelegate : TfDiagnosticMgr::Delegate {
public:
    PXRUSDMAYA_API
    ~UsdMayaDiagnosticDelegate() override;

    PXRUSDMAYA_API
    void IssueError(const TfError& err) override;
    PXRUSDMAYA_API
    void IssueStatus(const TfStatus& status) override;
    PXRUSDMAYA_API
    void IssueWarning(const TfWarning& warning) override;
    PXRUSDMAYA_API
    void IssueFatalError(
        const TfCallContext& context,
        const std::string& msg) override;

    /// Installs a shared delegate globally.
    /// If this is invoked on a secondary thread, issues a fatal coding error.
    PXRUSDMAYA_API
    static void InstallDelegate();
    /// Removes the global shared delegate, if it exists.
    /// If this is invoked on a secondary thread, issues a fatal coding error.
    PXRUSDMAYA_API
    static void RemoveDelegate();
    /// Returns the number of active batch contexts associated with the global
    /// delegate. 0 means no batching; 1 or more means diagnostics are batched.
    /// If there is no delegate installed, issues a runtime error and returns 0.
    PXRUSDMAYA_API
    static int GetBatchCount();

private:
    friend class UsdMayaDiagnosticBatchContext;

    std::atomic_int _batchCount;
    std::unique_ptr<UsdUtilsCoalescingDiagnosticDelegate> _batchedStatuses;
    std::unique_ptr<UsdUtilsCoalescingDiagnosticDelegate> _batchedWarnings;

    UsdMayaDiagnosticDelegate();

    void _StartBatch();
    void _EndBatch();
    void _FlushBatch();
};

/// As long as a batch context remains alive (process-wide), the
/// UsdMayaDiagnosticDelegate will save diagnostic messages, only emitting
/// them when the last batch context is destructed. Note that errors are never
/// batched.
///
/// Batch contexts must only exist on the main thread (though they will apply
/// to any diagnostics issued on secondary threads while they're alive). If
/// they're constructed on secondary threads, they will issue a fatal coding
/// error.
///
/// Batch contexts can be constructed and destructed out of "scope" order, e.g.,
/// this is allowed:
///   1. Context A constructed
///   2. Context B constructed
///   3. Context A destructed
///   4. Context B destructed
class UsdMayaDiagnosticBatchContext : boost::noncopyable
{
public:
    /// Constructs a batch context, causing all subsequent diagnostic messages
    /// to be batched on all threads.
    /// If this is invoked on a secondary thread, issues a fatal coding error.
    PXRUSDMAYA_API
    UsdMayaDiagnosticBatchContext();
    PXRUSDMAYA_API
    ~UsdMayaDiagnosticBatchContext();

private:
    /// This pointer is used to "bind" this context to a specific delegate in
    /// case the global delegate is removed (and possibly re-installed) while
    /// this batch context is alive.
    std::weak_ptr<UsdMayaDiagnosticDelegate> _delegate;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
