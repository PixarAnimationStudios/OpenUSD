//
// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_WORK_REDUCE_H
#define PXR_BASE_WORK_REDUCE_H

/// \file work/reduce.h
#include "pxr/pxr.h"
#include "pxr/base/work/api.h"
#include "pxr/base/work/impl.h"
#include "pxr/base/work/threadLimits.h"

PXR_NAMESPACE_OPEN_SCOPE


///////////////////////////////////////////////////////////////////////////////
///     
/// Recursively splits the range [0, \p n) into subranges, which are then
/// reduced by invoking \p loopCallback in parallel. Each invocation of
/// \p loopCallback returns a single value that is the result of joining the
/// elements in the respective subrange. These values are then further joined
/// using the binary operator \p reductionCallback, until only a single value
/// remains. This single value is then the result of joining all elements over
/// the entire range [0, \p n).
/// 
/// The \p loopCallback must be of the form:
/// 
///     V LoopCallback(size_t begin, size_t end, const V &identity);
///     
/// The \p reductionCallback must be of the form:
/// 
///     V ReductionCallback(const V &lhs, const V &rhs);
/// 
/// For example, the following code reduces an array of mesh points into a
/// single bounding box:
/// 
/// ```{.cpp}
/// 
/// // Get the mesh points from which we are going to generate the bounding box.
/// const std::vector<Vector3> &points = GetMeshPoints();
/// 
/// // Generate the bounding box by parallel reducing the points.
/// BoundingBox bbox = WorkParallelReduceN(
///     BoundingBox(),
///     points.size(),
///     [&points](size_t b, size_t e, const BoundingBox &identity){
///         BoundingBox bbox(identity);
///         
///         // Insert each point in this subrange into the local bounding box.
///         for (size_t i = b; i != e; ++i) {
///             bbox.InsertPoint(points[i]);
///         }
///         
///         // Return the local bounding box, which now encapsulates all the
///         // points in this subrange.
///         return bbox;
///     },
///     [](const BoundingBox &lhs, const BoundingBox &rhs){
///         // Join two bounding boxes into a single bounding box. The
///         // algorithm will apply this reduction step recursively until there
///         // is only a single bounding box left.
///         BoundingBox bbox(lhs);
///         bbox.UnionWith(rhs);
///         return bbox;
///     }
/// );
/// 
/// ```
/// 
/// \p grainSize specifies a minimum amount of work to be done per-thread.
/// There is overhead to launching a task and a typical guideline is that
/// you want to have at least 10,000 instructions to count for the overhead of
/// launching that task.
///
template <typename Fn, typename Rn, typename V>
V
WorkParallelReduceN(
    const V &identity,
    size_t n,
    Fn &&loopCallback,
    Rn &&reductionCallback,
    size_t grainSize)
{
    if (n == 0)
        return identity;

    // Don't bother with parallel_reduce, if concurrency is limited to 1.
    if (WorkHasConcurrency()) {
        PXR_WORK_IMPL_NAMESPACE_USING_DIRECTIVE;
        return WorkImpl_ParallelReduceN(
            identity,
            n,
            std::forward<Fn>(loopCallback),
            std::forward<Rn>(reductionCallback),
            grainSize);
    }
        
    // If concurrency is limited to 1, execute serially.
    return std::forward<Fn>(loopCallback)(0, n, identity);
}

///////////////////////////////////////////////////////////////////////////////
///
/// \overload
/// 
/// This overload does not accept a grain size parameter and instead attempts
/// to automatically deduce a grain size that is optimal for the current
/// resource utilization and provided workload.
///
template <typename Fn, typename Rn, typename V>
V
WorkParallelReduceN(
    const V &identity,
    size_t n,
    Fn &&loopCallback,
    Rn &&reductionCallback)
{
    return WorkParallelReduceN(identity, n, loopCallback, reductionCallback, 1);
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_BASE_WORK_REDUCE_H
