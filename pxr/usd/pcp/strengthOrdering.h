//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PCP_STRENGTH_ORDERING_H
#define PXR_USD_PCP_STRENGTH_ORDERING_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"

PXR_NAMESPACE_OPEN_SCOPE

class PcpNodeRef;

/// Compares the strength of nodes \p a and \p b. These nodes must be siblings; 
/// it is a coding error if \p a and \p b do not have the same parent node.
///
/// Returns -1 if a is stronger than b,
///          0 if a is equivalent to b,
///          1 if a is weaker than b
PCP_API
int
PcpCompareSiblingNodeStrength(const PcpNodeRef& a, const PcpNodeRef& b);

/// Compares the strength of nodes \p a and \p b. These nodes must be part
/// of the same graph; it is a coding error if \p a and \p b do not have the
/// same root node.
///
/// Returns -1 if a is stronger than b,
///          0 if a is equivalent to b,
///          1 if a is weaker than b
PCP_API
int
PcpCompareNodeStrength(const PcpNodeRef& a, const PcpNodeRef& b);

/// Compares the strength of a payload node with arcSiblingNum \p payloadArcNum
/// to a sibling node \p siblingNode. These nodes must be siblings:  
/// it is a coding error if \p siblingNode 's parent node is not \p payloadParent.
///
/// Returns -1 if the payload node is stronger than siblingNode,
///          0 if the payload node is equivalent to siblingNode,
///          1 if the payload node is weaker than siblingNode
PCP_API
int
PcpCompareSiblingPayloadNodeStrength(const PcpNodeRef& payloadParent, 
    int payloadArcNum, const PcpNodeRef& siblingNode);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_STRENGTH_ORDERING_H
