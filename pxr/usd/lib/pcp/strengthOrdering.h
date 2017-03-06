//
// Copyright 2016 Pixar
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
#ifndef PCP_STRENGTH_ORDERING_H
#define PCP_STRENGTH_ORDERING_H

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

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PCP_STRENGTH_ORDERING_H
