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
#include "usdMaya/diagnosticDelegate.h"

#include "usdMaya/debugCodes.h"

#include "pxr/base/arch/threads.h"
#include "pxr/base/tf/envSetting.h"
#include "pxr/base/tf/stackTrace.h"

#include <maya/MGlobal.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(PIXMAYA_DIAGNOSTICS_BATCH, true,
        "Whether to batch diagnostics coming from the same call site. "
        "If batching is off, all secondary threads' diagnostics will be "
        "printed to stderr.");

// Globally-shared delegate. Uses shared_ptr so we can have weak ptrs.
static std::shared_ptr<PxrUsdMayaDiagnosticDelegate> _sharedDelegate;

class _StatusOnlyDelegate : public UsdUtilsCoalescingDiagnosticDelegate {
    void IssueWarning(const TfWarning&) override {}
    void IssueFatalError(const TfCallContext&, const std::string&) override {}
};

class _WarningOnlyDelegate : public UsdUtilsCoalescingDiagnosticDelegate {
    void IssueStatus(const TfStatus&) override {}
    void IssueFatalError(const TfCallContext&, const std::string&) override {}
};

static MString
_FormatDiagnostic(const TfDiagnosticBase& d)
{
    const std::string msg = TfStringPrintf(
            "%s -- %s in %s at line %zu of %s",
            d.GetCommentary().c_str(),
            TfDiagnosticMgr::GetCodeName(d.GetDiagnosticCode()).c_str(),
            d.GetContext().GetFunction(),
            d.GetContext().GetLine(),
            d.GetContext().GetFile());
    return msg.c_str();
}

static MString
_FormatCoalescedDiagnostic(const UsdUtilsCoalescingDiagnosticDelegateItem& item)
{
    const size_t numItems = item.unsharedItems.size();
    const std::string suffix = numItems == 1
            ? std::string()
            : TfStringPrintf(" -- and %zu similar", numItems - 1);
    const std::string message = TfStringPrintf("%s%s",
            item.unsharedItems[0].commentary.c_str(),
            suffix.c_str());

    return message.c_str();
}

static bool
_IsDiagnosticBatchingEnabled()
{
    return TfGetEnvSetting(PIXMAYA_DIAGNOSTICS_BATCH);
}

PxrUsdMayaDiagnosticDelegate::PxrUsdMayaDiagnosticDelegate() : _batchCount(0)
{
    TfDiagnosticMgr::GetInstance().AddDelegate(this);
}

PxrUsdMayaDiagnosticDelegate::~PxrUsdMayaDiagnosticDelegate()
{
    // If a batch context was open when the delegate is removed, we need to
    // flush all the batched diagnostics in order to avoid losing any.
    // The batch context should know how to clean itself up when the delegate
    // is gone.
    _FlushBatch();
    TfDiagnosticMgr::GetInstance().RemoveDelegate(this);
}

void
PxrUsdMayaDiagnosticDelegate::IssueError(const TfError& err)
{
    // Errors are never batched. They should be rare, and in those cases, we
    // want to see them separately.
    // In addition, always display the full call site for errors by going
    // through _FormatDiagnostic.
    if (ArchIsMainThread()) {
        MGlobal::displayError(_FormatDiagnostic(err));
    }
    else {
        std::cerr << _FormatDiagnostic(err) << std::endl;
    }
}

void
PxrUsdMayaDiagnosticDelegate::IssueStatus(const TfStatus& status)
{
    if (_batchCount.load() > 0) {
        return; // Batched.
    }

    if (ArchIsMainThread()) {
        MGlobal::displayInfo(status.GetCommentary().c_str());
    }
    else {
        std::cerr << _FormatDiagnostic(status) << std::endl;
    }
}

void
PxrUsdMayaDiagnosticDelegate::IssueWarning(const TfWarning& warning)
{
    if (_batchCount.load() > 0) {
        return; // Batched.
    }

    if (ArchIsMainThread()) {
        MGlobal::displayWarning(warning.GetCommentary().c_str());
    }
    else {
        std::cerr << _FormatDiagnostic(warning) << std::endl;
    }
}

void
PxrUsdMayaDiagnosticDelegate::IssueFatalError(
    const TfCallContext& context,
    const std::string& msg)
{
    TfLogCrash(
            "FATAL ERROR",
            msg,
            /*additionalInfo*/ std::string(),
            context,
            /*logToDb*/ true);
    _UnhandledAbort();
}

/* static */
void
PxrUsdMayaDiagnosticDelegate::InstallDelegate()
{
    if (!ArchIsMainThread()) {
        TF_FATAL_CODING_ERROR("Cannot install delegate from secondary thread");
    }
    _sharedDelegate.reset(new PxrUsdMayaDiagnosticDelegate());
}

/* static */
void
PxrUsdMayaDiagnosticDelegate::RemoveDelegate()
{
    if (!ArchIsMainThread()) {
        TF_FATAL_CODING_ERROR("Cannot remove delegate from secondary thread");
    }
    _sharedDelegate.reset();
}

/* static */
int
PxrUsdMayaDiagnosticDelegate::GetBatchCount()
{
    if (std::shared_ptr<PxrUsdMayaDiagnosticDelegate> ptr = _sharedDelegate) {
        return ptr->_batchCount.load();
    }

    TF_RUNTIME_ERROR("Delegate is not installed");
    return 0;
}

void
PxrUsdMayaDiagnosticDelegate::_StartBatch()
{
    TF_AXIOM(ArchIsMainThread());

    if (_batchCount.fetch_add(1) == 0) {
        // This is the first _StartBatch; add the batching delegates.
        _batchedStatuses.reset(new _StatusOnlyDelegate());
        _batchedWarnings.reset(new _WarningOnlyDelegate());
    }
}

void
PxrUsdMayaDiagnosticDelegate::_EndBatch()
{
    TF_AXIOM(ArchIsMainThread());

    const int prevValue = _batchCount.fetch_sub(1);
    if (prevValue <= 0) {
        TF_FATAL_ERROR("_EndBatch invoked before _StartBatch");
    }
    else if (prevValue == 1) {
        // This is the last _EndBatch; print the diagnostic messages.
        // and remove the batching delegates.
        _FlushBatch();
        _batchedStatuses.reset();
        _batchedWarnings.reset();
    }
}

void
PxrUsdMayaDiagnosticDelegate::_FlushBatch()
{
    TF_AXIOM(ArchIsMainThread());

    const UsdUtilsCoalescingDiagnosticDelegateVector statuses =
            _batchedStatuses
            ? _batchedStatuses->TakeCoalescedDiagnostics()
            : UsdUtilsCoalescingDiagnosticDelegateVector();
    const UsdUtilsCoalescingDiagnosticDelegateVector warnings =
            _batchedWarnings
            ? _batchedWarnings->TakeCoalescedDiagnostics()
            : UsdUtilsCoalescingDiagnosticDelegateVector();

    // Note that we must be in the main thread here, so it's safe to call
    // displayInfo/displayWarning.
    for (const UsdUtilsCoalescingDiagnosticDelegateItem& item : statuses) {
        MGlobal::displayInfo(_FormatCoalescedDiagnostic(item));
    }
    for (const UsdUtilsCoalescingDiagnosticDelegateItem& item : warnings) {
        MGlobal::displayWarning(_FormatCoalescedDiagnostic(item));
    }
}

PxrUsdMayaDiagnosticBatchContext::PxrUsdMayaDiagnosticBatchContext()
    : _delegate(_IsDiagnosticBatchingEnabled() ? _sharedDelegate : nullptr)
{
    TF_DEBUG(PXRUSDMAYA_DIAGNOSTICS).Msg(">> Entering batch context\n");
    if (!ArchIsMainThread()) {
        TF_FATAL_CODING_ERROR("Cannot construct context on secondary thread");
    }
    if (std::shared_ptr<PxrUsdMayaDiagnosticDelegate> ptr = _delegate.lock()) {
        ptr->_StartBatch();
    }
}

PxrUsdMayaDiagnosticBatchContext::~PxrUsdMayaDiagnosticBatchContext()
{
    TF_DEBUG(PXRUSDMAYA_DIAGNOSTICS).Msg("!! Exiting batch context\n");
    if (!ArchIsMainThread()) {
        TF_FATAL_CODING_ERROR("Cannot destruct context on secondary thread");
    }
    if (std::shared_ptr<PxrUsdMayaDiagnosticDelegate> ptr = _delegate.lock()) {
        ptr->_EndBatch();
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
