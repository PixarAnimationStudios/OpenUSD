//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_DISCONNECTED_INPUTS_INVALIDATION_RESULT_H
#define PXR_EXEC_EXEC_DISCONNECTED_INPUTS_INVALIDATION_RESULT_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/maskedOutputVector.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class VdfNode;

/// Communicates the results of invalidation from uncompilation.
class Exec_DisconnectedInputsInvalidationResult
{
public:
    /// The invalidation request for executor invalidation.
    VdfMaskedOutputVector invalidationRequest;

    /// The leaf nodes reachable from the disconnected inputs. Does not contain
    /// disconnected leaf nodes.
    const std::vector<const VdfNode *> &invalidLeafNodes;

    /// Additional leaf nodes that are now disconnected as a result of the
    /// invalidation.
    std::vector<const VdfNode *> disconnectedLeafNodes;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
