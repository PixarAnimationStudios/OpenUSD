//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_INVALIDATION_RESULT_H
#define PXR_EXEC_EXEC_INVALIDATION_RESULT_H

#include "pxr/pxr.h"

#include "pxr/base/tf/bits.h"
#include "pxr/base/tf/span.h"
#include "pxr/exec/ef/timeInterval.h"
#include "pxr/exec/vdf/maskedOutputVector.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class SdfPath;
class VdfNode;

/// Communicates the results of invalidation.
struct Exec_InvalidationResult
{
    /// The invalidation request for executor invalidation.
    VdfMaskedOutputVector invalidationRequest;

    /// The leaf nodes reachable from the invalidated nodes.
    const std::vector<const VdfNode *> &invalidLeafNodes;
};

/// Communicates the results of authored value invalidation for metadata.
struct Exec_MetadataInvalidationResult final
    : public Exec_InvalidationResult
{
};

/// Communicates the results of authored value invalidation for attributes.
struct Exec_AttributeValueInvalidationResult final
    : public Exec_InvalidationResult
{
    /// The array of invalid attributes.
    TfSpan<const SdfPath> invalidAttributes;

    /// Bit set with the same size as invalidAttributes, denoting which of the
    /// invalid attributes are compiled in the exec network.
    TfBits compiledAttributes;

    /// The combined time range over which the compiled leaf nodes are invalid
    /// as a result of the authored value change.
    /// 
    /// Note, this combined interval only spans the invalid time ranges of
    /// compiled attributes. The time ranges of attributes not compiled can be
    /// accessed through invalidAttributes.
    EfTimeInterval invalidInterval;

    /// This is true if the authoring of values resulted in time dependency
    /// of an input (or connected leaf node) changing.
    bool isTimeDependencyChange;
};

/// Communicates the results of invalidation from uncompilation.
struct Exec_DisconnectedInputsInvalidationResult final
    : public Exec_InvalidationResult
{
    /// Additional leaf nodes that are now disconnected as a result of the
    /// invalidation.
    std::vector<const VdfNode *> disconnectedLeafNodes;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
