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
#include "pxr/pxr.h"
#include "pxr/imaging/hd/primGather.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/base/tf/diagnostic.h"


PXR_NAMESPACE_OPEN_SCOPE

void HdPrimGather::Filter(const SdfPathVector &paths,
                          const SdfPathVector &includePaths,
                          const SdfPathVector &excludePaths,
                          SdfPathVector       *results)
{
    HD_TRACE_FUNCTION();

    _SetupFilter(includePaths, excludePaths);
    _GatherPaths(paths);

    _WriteResults(paths, _gatheredRanges, results);
}

void
HdPrimGather::PredicatedFilter(const SdfPathVector &paths,
                      const SdfPathVector &includePaths,
                      const SdfPathVector &excludePaths,
                      FilterPredicateFn    predicateFn,
                      void                *predicateParam,
                      SdfPathVector       *results)
{
    HD_TRACE_FUNCTION();

    _SetupFilter(includePaths, excludePaths);
    _GatherPaths(paths);

    {
        // Looked into parallelizing this loop
        // However, the measured performance wasn't large enough to
        // justify the parallel overhead.
        // The scope here, is so it can be tracked and decided to
        // go parallel later if warranted
        HD_TRACE_SCOPE("HdPrimGather::Predicate Test");

        size_t numRanges = _gatheredRanges.size();
        for (size_t rangeNum = 0; rangeNum < numRanges; ++rangeNum) {
            const _Range &range = _gatheredRanges[rangeNum];

            _DoPredicateTest(paths,
                             range._start,
                             range._end,
                             predicateFn,
                             predicateParam);
        }
    }

    _WriteResults(paths, _resultRanges, results);
}

void
HdPrimGather::Subtree(const SdfPathVector &paths,
                       const SdfPath       &rootPath,
                       SdfPathVector       *results)
{
    results->clear();

    if (paths.empty()) {
        return;
    }

    // Setup simple filter
    _filterList.clear();
    _filterList.reserve(1);
    _filterList.emplace_back(rootPath, true);

    // Enter filter function
    // End of range is the inclusive.
    // We start with everything excluded from the results
    _FilterRange(paths, 0, paths.size() - 1, false);

    _WriteResults(paths, _gatheredRanges, results);
}


size_t
HdPrimGather::_FindLowerBound(const SdfPathVector &paths,
                               size_t start,
                               size_t end,
                               const SdfPath &path) const
{
    size_t rangeSize = end - start;
    while (rangeSize > 0) {
        size_t step = rangeSize / 2;
        size_t mid = start + step;

        const SdfPath &testPath = paths[mid];

        if ((testPath <  path)) {
            start = mid + 1;
            rangeSize -= step + 1;
        } else {
            rangeSize = step;
        }
    }

    return start;
}

size_t
HdPrimGather::_FindUpperBound(const SdfPathVector &paths,
                               size_t start,
                               size_t end,
                               const SdfPath &path) const
{
    // This code looks for the first index that doesn't have
    // the prefix, so special case if all paths have the prefix
    if (paths[end].HasPrefix(path)) {
        return end;
    }

    size_t rangeSize = end - start;
    while (rangeSize > 0) {
        size_t step = rangeSize / 2;
        size_t mid = start + step;

        const SdfPath &testPath = paths[mid];

        if (testPath.HasPrefix(path)) {
            start = mid + 1;
            rangeSize -= step + 1;
        } else {
            rangeSize = step;
        }
    }

    // start represents the first path that doens't have the prefix
    // but we want the inclusive range, so -1
    return start - 1;
}

// Apply the the top item on the filter stack to the range
// of elements between (start, end) (inclusive).
// isIncludeRange is the current state of the specified range.
void
HdPrimGather::_FilterRange(const SdfPathVector &paths,
                            size_t start,
                            size_t end,
                            bool isIncludeRange)
{
    // If filter list is empty, we are done processing.
    if (_filterList.empty()) {
        if (isIncludeRange) {
            _gatheredRanges.emplace_back(start, end);
        }
        return;
    }

    // Take copy as we are going to pop_back before we use the filter.
    const _PathFilter currentFilter = _filterList.back();

    // Check to see if the top of the filter stack is beyond the
    // end of the range.  If it is, we are done processing this range.
    if (currentFilter._path > paths[end]) {
        if (isIncludeRange) {
            _gatheredRanges.emplace_back(start, end);
        }
        return;
    }

    // Need to process the filter, so remove it.
    _filterList.pop_back();

    // If the type of filter matches the range, it's a no-op.
    // Filter the same range again with the next filter.

    bool skipFilter = currentFilter._includePath == isIncludeRange;

    // Is filter before the start of the range?
    skipFilter |= (paths[start] > currentFilter._path) &&
                  (!paths[start].HasPrefix(currentFilter._path));

    if (skipFilter) {
        _FilterRange(paths, start, end, isIncludeRange);
    } else {
        // We need to split the range.

        size_t lowerBound = _FindLowerBound(paths,
                                            start,
                                            end,
                                            currentFilter._path);

        size_t upperBound = _FindUpperBound(paths,
                                            lowerBound,
                                            end,
                                            currentFilter._path);

        // And filter the new ranges.
        if (start < lowerBound) {
            _FilterRange(paths, start, lowerBound - 1, isIncludeRange);
        }

        // Note: The type of range is inverted, because this is the
        // area that hit the filter.
        _FilterRange(paths, lowerBound, upperBound, !isIncludeRange);

        if (upperBound < end) {
            _FilterRange(paths, upperBound + 1, end, isIncludeRange);
        }
    }
}


void
HdPrimGather::_SetupFilter(const SdfPathVector &includePaths,
                           const SdfPathVector &excludePaths)
{
    // Combine include and exclude paths in to the filter stack.
    _filterList.clear();
    _filterList.reserve(includePaths.size() + excludePaths.size());
    for (SdfPathVector::const_iterator incIt  = includePaths.begin();
                                       incIt != includePaths.end();
                                     ++incIt) {
        _filterList.emplace_back(*incIt, true);
    }

    for (SdfPathVector::const_iterator excIt  = excludePaths.begin();
                                       excIt != excludePaths.end();
                                     ++excIt) {
        _filterList.emplace_back(*excIt, false);
    }

    // Note: Inverted sort, so can pop back.
    std::sort(_filterList.begin(),
              _filterList.end(),
              std::greater<_PathFilter>());
}

void
HdPrimGather::_GatherPaths(const SdfPathVector &paths)
{
    // There is an expectation that paths is pre-sorted, but it is an
    // expensive check so only do it if safe mode is enabled.
    if (TfDebug::IsEnabled(HD_SAFE_MODE)) {
        TF_VERIFY(std::is_sorted(paths.begin(), paths.end()));
        // The side effect of not sorting is incorrect results of the
        // gather.  That should not lead to a crash, so just continue -
        // thus producing the same results as when safe mode is off.
    }

    _gatheredRanges.empty();

    if (paths.empty()) {
        return;
    }

    // Optimize the common case of including everything
    if ((_filterList.size() == 1) &&
        (_filterList[0]._includePath == true) &&
        (_filterList[0]._path == SdfPath::AbsoluteRootPath())) {
            // End of range is the inclusive.
            _gatheredRanges.emplace_back(0, paths.size() - 1);
            return;
    }

    // Enter recursive function
    // End of range is the inclusive.
    // We start with everything excluded from the results
    _FilterRange(paths, 0, paths.size() - 1, false);
}

void
HdPrimGather::_DoPredicateTest(const SdfPathVector &paths,
                                size_t               begin,
                                size_t               end,
                                FilterPredicateFn    predicateFn,
                                void                *predicateParam)
{
    size_t currentStart = begin;
    for (size_t pathIdx = begin; pathIdx <= end; ++pathIdx) {
        // Test to see if path at index needs to split
        if (!predicateFn(paths[pathIdx], predicateParam)) {

            // Add all paths up to the path before this one
            if (currentStart < pathIdx) {
                _resultRanges.emplace_back(currentStart, pathIdx - 1);
            }

            currentStart = pathIdx + 1;
        }
    }

    // Add final range
    if (currentStart <= end) {
        _resultRanges.emplace_back(currentStart, end);
    }
}


void
HdPrimGather::_WriteResults(const SdfPathVector &paths,
                            const _RangeArray &ranges,
                            SdfPathVector *results) const
{
    results->clear();

    size_t numPaths = 0;
    for (_RangeArray::const_iterator it  = ranges.begin();
                                     it != ranges.end();
                                   ++it) {
        numPaths += (it->_end - it->_start) + 1; // +1 for inclusive range.
    }

    results->reserve(numPaths);

    for (_RangeArray::const_iterator it  = ranges.begin();
                                     it != ranges.end();
                                   ++it) {

        SdfPathVector::const_iterator rangeStartIt = paths.begin();
        SdfPathVector::const_iterator rangeEndIt   = paths.begin();

        std::advance(rangeStartIt, it->_start);
        std::advance(rangeEndIt,   it->_end + 1); // End is exclusive, so +1

        results->insert(results->end(), rangeStartIt, rangeEndIt);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
