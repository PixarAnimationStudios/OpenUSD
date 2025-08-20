//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_TIME_CHANGE_INVALIDATION_RESULT_H
#define PXR_EXEC_EXEC_TIME_CHANGE_INVALIDATION_RESULT_H

#include "pxr/pxr.h"

#include "pxr/exec/ef/time.h"
#include "pxr/exec/vdf/maskedOutputVector.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class VdfNode;

/// Communicates the results of a time change causing invalidation.
class Exec_TimeChangeInvalidationResult
{
public:
    /// The time-dependent outputs to traverse and invalidate as a consequence
    /// of the time changing from oldTime to newTime.
    VdfMaskedOutputVector invalidationRequest;

    /// The leaf nodes which are invalid as a result of the time change. This is
    /// the array of time-dependent leaf nodes filtered by the collection of
    /// leaf nodes reachable from the inputs where there is *actually* a value
    /// difference in the input between oldTime and newTime.
    const std::vector<const VdfNode *> &invalidLeafNodes;

    /// The old time, which was effective prior to the time change.
    EfTime oldTime;

    /// The new time, which has now become effective with this time change.
    EfTime newTime;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
