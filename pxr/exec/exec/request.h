//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_REQUEST_H
#define PXR_EXEC_EXEC_REQUEST_H

#include "pxr/pxr.h"

#include "pxr/base/tf/pxrTslRobinMap/robin_set.h"

#include <functional>

PXR_NAMESPACE_OPEN_SCOPE

/// Index into the array of exec value keys used to construct an exec request.
using ExecRequestIndexSet = pxr_tsl::robin_set<int>;

/// Invalidation callback used by exec requests to notify clients of invalid
/// computed values.
/// 
/// The index set contains the indices of value keys with invalid computed
/// values, along with a time interval that specifies the time range over which
/// these computed values are invalid.
/// 
using ExecRequestComputedValueInvalidationCallback =
    std::function<void (
        const ExecRequestIndexSet &,
        const class EfTimeInterval &)>;

/// Invalidation callback used by exec requests to notify clients of invalid
/// computed values as a consequence of time changing.
/// 
/// The index set contains the indices of value keys which are time dependent,
/// and for which input values to the execution system are changing between the
/// old time and new time.
///
using ExecRequestTimeChangeInvalidationCallback =
    std::function<void (const ExecRequestIndexSet &)>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
