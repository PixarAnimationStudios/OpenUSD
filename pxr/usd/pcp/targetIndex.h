//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_USD_PCP_TARGET_INDEX_H
#define PXR_USD_PCP_TARGET_INDEX_H

#include "pxr/pxr.h"
#include "pxr/usd/pcp/api.h"
#include "pxr/usd/pcp/errors.h"
#include "pxr/usd/sdf/declareHandles.h"
#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfSpec);
class PcpCache;
class PcpLayerStackIdentifier;
class PcpPrimIndex;
class PcpPropertyIndex;

/// \class PcpTargetIndex
///
/// A PcpTargetIndex represents the results of indexing the target
/// paths of a relationship or attribute.  Note that this is just
/// the result; it does not retain all of the input arguments used
/// in computing the index, such as the owning property.
///
struct PcpTargetIndex {
    SdfPathVector paths;
    PcpErrorVector localErrors;
    bool hasTargetOpinions = false;
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
PCP_API
void
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
/// \p deletedPaths, if not \c NULL, will be populated with target
/// paths whose deletion contributed to the computed value of
/// \c targetIndex->paths.
///
/// \p allErrors will contain any errors encountered while
/// performing this operation.
PCP_API
void
PcpBuildFilteredTargetIndex(
    const PcpSite& propSite,
    const PcpPropertyIndex& propIndex,
    const SdfSpecType relOrAttrType,
    const bool localOnly,
    const SdfSpecHandle &stopProperty,
    const bool includeStopProperty,
    PcpCache *cacheForValidation,
    PcpTargetIndex *targetIndex,
    SdfPathVector *deletedPaths,
    PcpErrorVector *allErrors);

/// Compose the target paths of the relationship or attribute at \p propPath
/// directly from \p primIndex, without building an intermediate
/// PcpPropertyIndex.
///
/// This is equivalent to PcpBuildPrimPropertyIndex followed by
/// PcpBuildTargetIndex, but reads layer fields directly instead of creating an
/// SdfPropertySpecHandle per contributing spec.  Those handles require an entry
/// in each layer's identity registry, which is guarded by a per-layer lock.
/// This function avoids those costs which matters for callers who compose
/// targets concurrently.
///
/// \p primIndex must be a USD-mode prim index; see PcpPrimIndex::IsUsd().
/// Non-USD composition enforces permissions and attribute type consistency,
/// neither of which this function implements.  Passing a non-USD prim index
/// is a coding error.
///
/// \p relOrAttrType indicates whether the property is a relationship or an
/// attribute.
///
/// \p paths is filled with the composed target paths.
///
/// \p hasTargetOpinions is set to true if any contributing spec expressed an
/// opinion about the target paths, even if the composed result is empty.  It is
/// left alone otherwise.
///
/// \p allErrors will contain any errors encountered while performing this
/// operation.
PCP_API
void
PcpComposeTargetPaths(
    const PcpLayerStackIdentifier &layerStackId,
    const SdfPath &propPath,
    const PcpPrimIndex &primIndex,
    SdfSpecType relOrAttrType,
    SdfPathVector *paths,
    bool *hasTargetOpinions,
    PcpErrorVector *allErrors);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_USD_PCP_TARGET_INDEX_H
