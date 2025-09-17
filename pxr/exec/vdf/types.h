//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_TYPES_H
#define PXR_EXEC_VDF_TYPES_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/defaultInitAllocator.h"
#include "pxr/exec/vdf/mask.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/hashset.h"
#include "pxr/base/tf/smallVector.h"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class VdfConnection;
class VdfInput;
class VdfNode;
class VdfOutput;

/// Returns \c true if single-frame parallel evaluation is enabled.
///
VDF_API
bool VdfIsParallelEvaluationEnabled();

/// Type for an ordered collection of connections.
typedef TfSmallVector<VdfConnection *, 1> VdfConnectionVector;

/// Type for an ordered collection of connections.
typedef TfSmallVector<const VdfConnection *, 1> VdfConnectionConstVector;

/// Type for an unordered set of connections.
typedef TfHashSet<VdfConnection *, TfHash> VdfConnectionSet;

/// Type for an unordered set of connections.
typedef TfHashSet<const VdfConnection *, TfHash> VdfConnectionConstSet;

/// Type for an unordered set of nodes pointers.
typedef TfHashSet<const VdfNode *, TfHash> VdfNodePtrSet;

/// Type for an unordered set of output pointers.
typedef TfHashSet<const VdfOutput *, TfHash> VdfOutputPtrSet;

/// Type for an unordered set of input pointers.
typedef TfHashSet<const VdfInput *, TfHash> VdfInputPtrSet;

/// Type for an ordered collection of inputs
typedef std::vector<const VdfInput *> VdfInputPtrVector;

/// Type for an ordered collection of outputs
typedef std::vector<const VdfOutput *> VdfOutputPtrVector;

/// Type of callback used when processing nodes.
typedef std::function<void (const VdfNode &)> VdfNodeCallback;

/// Type of callback for building a node debug name
typedef std::function<std::string()> VdfNodeDebugNameCallback;

/// Type of the timestamp that identifies the most recent round of invalidation.
typedef unsigned int VdfInvalidationTimestamp;

/// A pair of connection pointer and mask for sparse input dependency computation.
typedef std::pair<VdfConnection *, VdfMask> VdfConnectionAndMask;

/// A vector of VdfConnectionAndMasks.
typedef std::vector<VdfConnectionAndMask> VdfConnectionAndMaskVector;

/// Function type to be used with ForEachScheduledOutput().
///
typedef
    std::function<void (const VdfOutput *, const VdfMask &)>
    VdfScheduledOutputCallback;

/// A map from node pointer to VdfOutputPtrSet.
typedef
    TfHashMap<const VdfNode *, VdfOutputPtrSet, TfHash>
    VdfNodeToOutputPtrSetMap;

/// A map from node pointer to VdfInputPtrVector
typedef TfHashMap<const VdfNode *, VdfInputPtrVector, TfHash> 
    VdfNodeToInputPtrVectorMap;

/// A map from node pointer to VdfInputPtrVector
typedef TfHashMap<const VdfNode *, VdfOutputPtrVector, TfHash> 
    VdfNodeToOutputPtrVectorMap;

/// A map from output pointer to mask
typedef
    std::unordered_map<const VdfOutput *, VdfMask, TfHash>
    VdfOutputToMaskMap;

/// The unique identifier type for Vdf objects.
typedef uint64_t VdfId;

/// The index type for Vdf objects.
typedef uint32_t VdfIndex;

/// The version type for Vdf objects.
typedef uint32_t VdfVersion;

/// A vector of ids.
typedef std::vector<VdfId> VdfIdVector;

/// A std::vector which on resize performs default initialization instead of
/// value initialization. We use this on arrays that are first resized and
/// then immediatelly filled with elements.
template <typename T>
using Vdf_DefaultInitVector = std::vector<T, Vdf_DefaultInitAllocator<T>>;

PXR_NAMESPACE_CLOSE_SCOPE

#endif
