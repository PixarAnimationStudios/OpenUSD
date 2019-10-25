//
// Copyright 2017 Pixar
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
#ifndef HD_PRIM_GATHER_H
#define HD_PRIM_GATHER_H

#include "pxr/pxr.h"
#include "pxr/imaging/hd/api.h"

#include "pxr/usd/sdf/path.h"

#include <vector>
#include <tbb/enumerable_thread_specific.h>
#include <tbb/blocked_range.h>

PXR_NAMESPACE_OPEN_SCOPE

class HdPrimGather final {
public:
    typedef bool (*FilterPredicateFn)(const SdfPath &path, const void *param);

    HdPrimGather()  = default;
    ~HdPrimGather() = default;

    ///
    /// Filter takes a list of paths and returns a list of paths that
    /// match the following criteria:
    ///
    /// - A path is prefixed by at least one include path
    /// - A path is not prefixed by an exclude path that has more elements
    ///   than the include path with the most element that is a prefix
    ///   of the path.
    ///
    /// The list of paths to filter must be pre-sorted with
    /// ordering defined by std::less<SdfPath &>.
    ///
    /// The list of include and exclude paths do not need to be pre-sorted.
    ///
    /// If the same path appears in the list of include and exclude paths
    /// results are undefined.
    ///
    /// The resulting set of paths are stored in results arg, the results
    /// might not be in sorted order.
    ///
    HD_API
    void Filter(const SdfPathVector &paths,
                const SdfPathVector &includePaths,
                const SdfPathVector &excludePaths,
                SdfPathVector       *results);

    ///
    /// PredicatedFilter takes a list of paths and returns a list of paths that
    /// match the following criteria:
    ///
    /// - A path is prefixed by at least one include path
    /// - A path is not prefixed by an exclude path that has more elements
    ///   than the include path with the most element that is a prefix
    ///   of the path.
    /// - The predicate function for the path returns true.
    ///
    /// The list of paths to filter must be pre-sorted with
    /// ordering defined by std::less<SdfPath &>.
    ///
    /// The list of include and exclude paths do not need to be pre-sorted.
    ///
    /// If the same path appears in the list of include and exclude paths
    /// results are undefined.
    ///
    /// The predicate function has the prototype:
    ///    bool Predicate(const SdfPath &path, void *param);
    ///
    /// The function should return true if the path should appear in the results
    /// and false if not.  The Predicate function may be called on worker
    /// threads and as such must be thread-safe.
    ///
    /// The resulting set of paths are stored in results arg, the results
    /// might not be in sorted order.
    ///
    HD_API
    void PredicatedFilter(const SdfPathVector &paths,
                          const SdfPathVector &includePaths,
                          const SdfPathVector &excludePaths,
                          FilterPredicateFn    predicateFn,
                          void                *predicateParam,
                          SdfPathVector       *results);

    ///
    /// Subtree is a simplified form of filter, that gathers
    /// all prims that meet the single rootPath prefix condition.
    ///
    /// The list of paths to filter must be pre-sorted with
    /// ordering defined by std::less<SdfPath &>.
    ///
    /// The returned result maintain the sorted order.
    ///
    HD_API
    void Subtree(const SdfPathVector &paths,
                 const SdfPath       &rootPath,
                 SdfPathVector       *results);

    ///
    /// Subtree is a simplified form of filter, that gathers
    /// all prims that meet the single rootPath prefix condition.
    ///
    /// The list of paths to filter must be pre-sorted with
    /// ordering defined by std::less<SdfPath &>.
    ///
    /// Rather than returning a list the paths, it instead returns
    /// the start and end (inclusive) indexes into the paths
    /// vector of that subtree range.
    ///
    /// If the rootPath wasn't found or an error occurred, that
    /// otherwise produces an invalid range. The method returns false.
    HD_API
    bool SubtreeAsRange(const SdfPathVector &paths,
                        const SdfPath       &rootPath,
                        size_t *start,
                        size_t *end);

private:
    struct _PathFilter {
        SdfPath _path;
        bool    _includePath;  // false = exclude path

        _PathFilter(const SdfPath &path, bool includePath)
         : _path(path)
         , _includePath(includePath)
        {

        }

        bool operator >(const _PathFilter &other) const {
            return other._path < _path;
        }
    };
    typedef std::vector<_PathFilter> _PathFilterArray;

    // While processing, the algorithm stores results as a set of ranges
    // rather than copying all the paths.
    // This to avoid copying the larger set of paths at intermediate
    // processing steps.
    struct _Range {
        size_t _start;
        size_t _end; // Inclusive

        _Range(size_t start, size_t end)
         : _start(start)
         , _end(end)
        {

        }

    };
    typedef std::vector<_Range> _RangeArray;
    typedef tbb::enumerable_thread_specific<_RangeArray> _ConcurrentRangeArray;
    typedef tbb::blocked_range<size_t>                   _ConcurrentRange;



    _PathFilterArray      _filterList;
    _RangeArray           _gatheredRanges;
    _ConcurrentRangeArray _resultRanges;



    size_t _FindLowerBound(const SdfPathVector &paths,
                           size_t start,
                           size_t end,
                           const SdfPath &path) const;
    size_t _FindUpperBound(const SdfPathVector &paths,
                           size_t start,
                           size_t end,
                           const SdfPath &path) const;

    void _FilterRange(const SdfPathVector &paths,
                      size_t start,
                      size_t end,
                      bool include);

    void _SetupFilter(const SdfPathVector &includePaths,
                     const SdfPathVector &excludePaths);

    void _GatherPaths(const SdfPathVector &paths);

    // Outer Loop called for each range in vector
    void _DoPredicateTestOnRange(const SdfPathVector &paths,
                                 const _Range        &range,
                                 FilterPredicateFn    predicateFn,
                                 void                *predicateParam);

    // Inner Loop over each prim in a sub range of _Range.
    void _DoPredicateTestOnPrims(const SdfPathVector &paths,
                                 _ConcurrentRange    &range,
                                 FilterPredicateFn    predicateFn,
                                 void                *predicateParam);

    template <class Iterator>
    static void _WriteResults(const SdfPathVector &paths,
                              const Iterator &rangesBegin,
                              const Iterator &rangesEnd,
                              SdfPathVector *results);

    void _FilterSubTree(const SdfPathVector &paths,
                        const SdfPath       &rootPath);

    // No default copying or assignment
    HdPrimGather(const HdPrimGather &) = delete;
    HdPrimGather &operator =(const HdPrimGather &) = delete;

};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HD_PRIM_GATHER_H
