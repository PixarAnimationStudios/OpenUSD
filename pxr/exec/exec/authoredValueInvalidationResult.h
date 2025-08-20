//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_AUTHORED_VALUE_INVALIDATION_RESULT_H
#define PXR_EXEC_EXEC_AUTHORED_VALUE_INVALIDATION_RESULT_H

#include "pxr/pxr.h"

#include "pxr/base/tf/bits.h"
#include "pxr/base/tf/span.h"
#include "pxr/exec/ef/timeInterval.h"
#include "pxr/exec/vdf/maskedOutputVector.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class SdfPath;
class VdfNode;

/// Communicates the results of authored value invalidation.
class Exec_AuthoredValueInvalidationResult
{
public:
    /// The array of invalid properties.
    TfSpan<const SdfPath> invalidProperties;

    /// Bit set with the same size as invalidProperties, denoting which of the
    /// invalid properties are compiled in the exec network.
    TfBits compiledProperties;

    /// The invalidation request for executor invalidation.
    VdfMaskedOutputVector invalidationRequest;

    /// The leaf nodes reachable from the compiled, invalid properties, which
    /// are invalid as a result of the authored value invalidation.
    const std::vector<const VdfNode *> &invalidLeafNodes;

    /// The combined time range over which the compiled leaf nodes are invalid
    /// as a result of the authored value change.
    /// 
    /// Note, this combined interval only spans the invalid time ranges of
    /// compiled properties. The time ranges of properties not compiled can be
    /// accessed through invalidProperties.
    EfTimeInterval invalidInterval;

    /// This is true if the authoring of values resulted in time dependency
    /// of an input (or connected leaf node) changing.
    bool isTimeDependencyChange;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
