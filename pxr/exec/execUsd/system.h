//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_USD_SYSTEM_H
#define PXR_EXEC_EXEC_USD_SYSTEM_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/execUsd/api.h"

#include "pxr/base/tf/declarePtrs.h"
#include "pxr/exec/exec/request.h"
#include "pxr/exec/exec/system.h"

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_REF_PTRS(UsdStage);

class ExecUsdCacheView;
class ExecUsdRequest;
class ExecUsdValueKey;
class UsdTimeCode;

/// The implementation of a system to procedurally compute values based on USD
/// scene description and computation definitions.
/// 
/// ExecUsdSystem specializes the base ExecSystem class and owns USD-specific
/// structures and logic necessary to compile, schedule and evaluate requested
/// computation values.
/// 
/// The ExecUsdSystem extends the lifetime of the UsdStage it is constructed
/// with, although it is atypical for an ExecUsdSystem to outlive its stage in
/// practice. As a rule of thumb, the ExecUsdSystem lives right alongside the
/// UsdStage in most use-cases. 
/// 
class ExecUsdSystem : public ExecSystem
{
public:
    EXECUSD_API
    explicit ExecUsdSystem(const UsdStageConstRefPtr &stage);

    // Systems are non-copyable and non-movable to simplify management of
    // back-pointers.
    // 
    ExecUsdSystem(const ExecUsdSystem &) = delete;
    ExecUsdSystem& operator=(const ExecUsdSystem &) = delete;

    EXECUSD_API
    ~ExecUsdSystem();

    /// Changes the \p time at which values are computed.
    /// 
    /// Calling this method re-resolves time-dependent inputs from the scene
    /// graph at the new \p time, and determines which of these inputs are
    /// *actually* changing between the old and new time. Computed values that
    /// are dependent on the changing inputs are then invalidated, and requests
    /// are notified of the time change.
    /// 
    /// \note
    /// When computing multiple requests over multiple times, it is much more
    /// efficient to compute all requests at the same time, before moving on to
    /// the next time. Doing so, allows time-dependent intermediate results to
    /// remain cached and be re-used across the multiple calls to Compute().
    ///
    EXECUSD_API
    void ChangeTime(UsdTimeCode time);

    /// Builds a request for the given \p valueKeys.
    ///
    /// The optionally provided \p valueCallback will be invoked when
    /// previously computed value keys become invalid as a result of authored
    /// value changes or structural invalidation of the scene. If multiple
    /// value keys become invalid at the same time, they may be batched into a
    /// single invocation of the callback.
    /// 
    /// \note
    /// The \p valueCallback is only guaranteed to be invoked at least once per
    /// invalid value key and invalid time interval combination, and only after
    /// Compute() has been called. If clients want to be notified of future
    /// invalidation, they must call Compute() again to renew their interest in
    /// the computed value keys. 
    /// 
    /// The optionally provided \p timeCallback will be invoked when
    /// previously computed value keys become invalid as a result of time
    /// changing. The invalid value keys are the set of time-dependent value
    /// keys in this request, further filtered to only include the value keys
    /// where input dependencies are *actually* changing between the old time
    /// and new time.
    /// 
    /// \note
    /// The client must not call into execution (including, but not limited to
    /// Compute() or value extraction) from within the \p valueCallback, as well
    /// as the \p timeCallback.
    /// 
    EXECUSD_API
    ExecUsdRequest BuildRequest(
        std::vector<ExecUsdValueKey> &&valueKeys,
        ExecRequestComputedValueInvalidationCallback &&valueCallback =
            ExecRequestComputedValueInvalidationCallback(),
        ExecRequestTimeChangeInvalidationCallback &&timeCallback =
            ExecRequestTimeChangeInvalidationCallback());

    /// Prepares a given \p request for execution.
    /// 
    /// This ensures the exec network is compiled and scheduled for the value
    /// keys in the request. Compute() will implicitly prepare the request
    /// if needed, but calling PrepareRequest() separately enables clients to
    /// front-load compilation and scheduling cost.
    ///
    EXECUSD_API
    void PrepareRequest(const ExecUsdRequest &request);

    /// Executes the given \p request and returns a cache view for extracting
    /// the computed values.
    /// 
    /// This implicitly calls PrepareRequest(), though clients may choose to
    /// call PrepareRequest() ahead of time and front-load the associated
    /// compilation and scheduling cost.
    ///
    EXECUSD_API
    ExecUsdCacheView Compute(const ExecUsdRequest &request);

private:
    // This object to subscribes to scene changes on the UsdStage and delivers
    // those changes to the base ExecSystem.
    class _NoticeListener;
    std::unique_ptr<_NoticeListener> _noticeListener;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
