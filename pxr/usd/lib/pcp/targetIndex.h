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
#ifndef PCP_TARGET_INDEX_H
#define PCP_TARGET_INDEX_H

#include "pxr/usd/pcp/api.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"

SDF_DECLARE_HANDLES(SdfSpec);
class PcpCache;
class PcpPropertyIndex;

/// \class PcpTargetIndex
///
/// A PcpTargetIndex represents the results of indexing the target
/// paths of a relationship or attribute.  Note that this is just
/// the result; it does not retain all of the input arguments used
/// in computing the index, such as the owning property.
struct PcpTargetIndex {
    SdfPathVector paths;
    PcpErrorVector localErrors;
};

/// Build a PcpTargetIndex representing the target paths of the given
/// property.
///
/// \p propIndex is a PcpPropertyIndex of the relationship or attribute.
/// \p relOrAttrType indicates if the property is a relationship
/// or attribute.
/// \p allErrors will contain any errors encountered while
/// performing this operation.
///
/// Note that this function will skip the validation checks performed
/// by PcpBuildFilteredTargetIndex. See documentation below for details.
PCP_API void
PcpBuildTargetIndex(
    const PcpSite& propSite,
    const PcpPropertyIndex& propIndex,
    const SdfSpecType relOrAttrType,
    PcpTargetIndex *targetIndex,
    PcpErrorVector *allErrors);

/// Like PcpBuildTargetIndex, but optionally filters the result by
/// enforcing permissions restrictions and a stopProperty.
///
/// If \p localOnly is \c true then this will compose relationship
/// targets from local nodes only. If \p stopProperty is not \c
/// NULL then this will stop composing relationship targets at \p
/// stopProperty, including \p stopProperty iff \p includeStopProperty
/// is \c true.
///
/// \p cacheForValidation is a PcpCache that will be used to compute
/// additional prim indexes as needed for validation. NULL may be
/// passed in, but doing so will disable validation that relies on
/// this cache, which includes permissions checks.
///
/// \p allErrors will contain any errors encountered while
/// performing this operation.
PCP_API void
PcpBuildFilteredTargetIndex(
    const PcpSite& propSite,
    const PcpPropertyIndex& propIndex,
    const SdfSpecType relOrAttrType,
    const bool localOnly,
    const SdfSpecHandle &stopProperty,
    const bool includeStopProperty,
    PcpCache *cacheForValidation,
    PcpTargetIndex *targetIndex,
    PcpErrorVector *allErrors);

#endif // PCP_TARGET_INDEX_H
