//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_EXT_GPU_BUFFER_CONSUMER_H
#define PXR_IMAGING_HD_ST_EXT_GPU_BUFFER_CONSUMER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/extGpuBufferSchema.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

class HdSceneDelegate;
class HdStResourceRegistry;

/// \file extGpuBufferConsumer.h
///
/// Storm-internal helpers, shared by HdStMesh / HdStPoints / HdStBasisCurves,
/// that turn a producer-published HdExtGpuBufferSchema on a primvar into a
/// Storm buffer source, and a set of such sources into a zero-copy range.

/// Resolve the prim \p id's container data source from the terminal scene
/// index. Hoist this out of per-primvar loops -- it does the one scene-index
/// traversal -- and feed the result to the overload below, so a prim with N
/// dirty primvars pays for one GetPrim instead of N.
///
/// Returns null when there is no terminal scene index, and -- the case that
/// decides what this feature costs everyone else -- when \p registry's Hgi
/// has no external buffer arena at all. Nothing can be shared before an
/// application asks for an arena, so that one check answers the question for
/// the whole prim without touching the scene index, and the null it returns
/// turns every lookup downstream into a null test. Without it an application
/// that shares nothing still pays a GetPrim per prim per Sync, in perpetuity,
/// for a feature it does not use.
HDST_API
HdContainerDataSourceHandle
HdSt_GetPrimDataSource(
    HdSceneDelegate *sceneDelegate,
    SdfPath const &id,
    HdStResourceRegistry *registry);

/// Fetch the HdExtGpuBufferSchema (if any) for primvar \p name off an already
/// resolved prim \p primDataSource (see HdSt_GetPrimDataSource). Returns an
/// undefined schema when the prim / primvar / extGpuBuffer child is absent (a
/// null \p primDataSource included) -- which the caller treats as "no external
/// buffer, use the CPU path".
HDST_API
HdExtGpuBufferSchema
HdSt_GetExtGpuBufferSchema(
    HdContainerDataSourceHandle const &primDataSource,
    TfToken const &name);

/// Try to build an HdStExtGpuBufferSource (a source with no CPU payload) from
/// \p schema, for a primvar whose elements are bound at \p usage.
///
/// Returns nullptr -- and the caller falls through to its CPU path -- when the
/// schema is incomplete, when the producer has withdrawn the buffer, when the
/// buffer belongs to another Hgi, or when it does not fit the described
/// layout. All of those are ordinary, expected outcomes rather than errors.
///
/// Also records the buffer's arena with \p registry, so the commit that reads
/// it is bracketed by that arena's synchronization.
HDST_API
HdBufferSourceSharedPtr
HdSt_TryCreateExtGpuBufferSource(
    TfToken const &name,
    HdExtGpuBufferSchema const &schema,
    HdStResourceRegistry *registry);

/// If every source in \p sources is a GPU-backed source the producer allows
/// binding directly, return a zero-copy range over them; otherwise nullptr, so
/// the caller aggregates through a memory manager as usual.
///
/// When \p existingBar is already an external-buffer range it is rebound in
/// place and returned (the same pointer), so draw batches are not invalidated.
HDST_API
HdBufferArrayRangeSharedPtr
HdSt_TryCreateExtGpuBufferAliasBAR(
    HdBufferSourceSharedPtrVector const &sources,
    HdStResourceRegistry *registry,
    HdBufferArrayRangeSharedPtr const &existingBar = nullptr);

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_EXT_GPU_BUFFER_CONSUMER_H
