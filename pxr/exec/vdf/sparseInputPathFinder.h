//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_SPARSE_INPUT_PATH_FINDER_H
#define PXR_EXEC_VDF_SPARSE_INPUT_PATH_FINDER_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/request.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/hashmap.h"
#include "pxr/base/tf/hashset.h"
#include "pxr/base/tf/stringUtils.h"

#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

class VdfNode;

////////////////////////////////////////////////////////////////////////////////
///
/// \class VdfSparseInputPathFinder
///
/// \brief A class used for fast sparse traversals of VdfNetworks in the
///        output-to-input direction when the goal is to find all possible
///        paths from an output to a node.
/// 
///        A sparse traversal takes affects masks into account and avoids
///        traversing nodes that don't have an affect on the outputs
///        requested for the traversal.  This is most often useful for
///        dependency traversals.
///
///        Note that the main point here is to find all possible paths whereas
///        the VdfSparseInputTraverser reports only the first path it finds.
///
class VdfSparseInputPathFinder
{
public:

    /// Callback to determine if \p input is a relevant path.
    ///
    typedef bool (*InputCallback)(const VdfInput &input);

    /// Traverses the network in the input direction sparsely, starting from
    /// \p start trying to find all possible paths into \p target.
    ///
    /// Will populate \p paths with the results.
    ///
    /// Uses \p inputCallback in order to determine which paths are relevant
    /// and should be reported.  The default implementation (used when \p
    /// inputCallback is NULL) will treat no path as relevant.  This makes
    /// the VdfSparseInputPathFinder behave the same way as the sparse input
    /// traverser.  Only when \p inputCallback returns true for a specific 
    /// input a path will treated as a seperate path.
    ///
    VDF_API
    static void Traverse(
        const VdfMaskedOutput &start,
        const VdfMaskedOutput &target,
        InputCallback inputCallback,
        std::vector<VdfConnectionConstVector> *paths);

    /// Convenience method for a common usage of Traverse() where start == 
    /// target.  Finds all the paths in a cycle from \p start back to \p start. 
    ///
    /// Note, as in Traverse, the network must be fully connected before
    /// making this call.
    ///
    VDF_API
    static void FindAllCyclePaths(
        const VdfMaskedOutput &start,
        InputCallback inputCallback,
        std::vector<VdfConnectionConstVector> *paths);

// -----------------------------------------------------------------------------

private:

    VdfSparseInputPathFinder(
        const VdfMaskedOutput &start,
        const VdfMaskedOutput &target,
        InputCallback inputCallback,
        std::vector<VdfConnectionConstVector> *paths);

    // A class that represents a segment of a path.
    struct _PathSegment
    {
        _PathSegment()
        :   id(0), len(0) {}

        _PathSegment(unsigned int id, unsigned int len)
        :   id(id), len(len) {}

        // Equality operator.
        bool operator==(const _PathSegment &rhs) const {
            return id == rhs.id && len == rhs.len;
        }
        
        std::string GetAsString() const {
            return TfStringPrintf("{#%u / %u}", id, len);
        }

        unsigned int id, len;    // id and length of the segment
    };

    // A stack frame, aka. a masked output and its path segment pending to
    // be visited.
    struct _StackFrame : public _PathSegment
    {
        _StackFrame(
            const VdfMaskedOutput &maskedOutput,
            const _PathSegment &segment)
        :   _PathSegment(segment),
            maskedOutput(maskedOutput) {}

        VdfMaskedOutput maskedOutput;
    };

    // Objects of type _PotentialResult represent potential result paths.  They 
    // are created when the traversal encounters a previously visited connection
    // at \p encountered.  However, the moment we discover that path we don't
    // know if that path may or may not have results because we don't know if it 
    // has been traversed fully. 
    //
    // Therefore _PotentialResult objects are created and evaluated after the
    // traversal is finished.  Note that the \p ending segment ends at \p 
    // encountered by definition. 
    //
    struct _PotentialResult
    {
        _PotentialResult(
            const _PathSegment &ending, const _PathSegment &encountered)
        :   ending(ending),
            encountered(encountered) {}

        // Equality operator.
        bool operator==(const _PotentialResult &rhs) const {
            return ending == rhs.ending && encountered == rhs.encountered;
        }

        struct HashFunctor
        {
            size_t operator()(const _PotentialResult &p) const {
                return TfHash::Combine(
                    p.ending.id,
                    p.ending.len,
                    p.encountered.id,
                    p.encountered.len);
            }
        };

        // The ending path segment.
        _PathSegment ending;

        // The path and index being merged into.  Note that the len of 
        // encountered segment is the last element that is included in the 
        // path. 
        _PathSegment encountered;
    };

    using _VisitedDependencyToSegmentMap =
        TfHashMap<VdfMask, _PathSegment, VdfMask::HashFunctor>;

    // A map from VdfConnection pointer to _VisitedDependencyToSegmentMap. 
    // Tracks visited connections. 
    using _VisitedConnectionsInfoMap =
        TfHashMap<const VdfConnection *, _VisitedDependencyToSegmentMap, TfHash>;

    // A map from path to parent _PathSegment.  This is used to quickly find
    // parent path segments or a path.  Note that the parent is a path segment
    // because the parent path may continue after a child forked off it.
    using _PathToParentSegmentMap =
        TfHashMap<unsigned int, _PathSegment, TfHash>;

    // A map from path to result connection vector.  This map tracks the 
    // directly found results during traversal.  A directly found result is
    // when the traversal manages to find the result node without finding a
    // previously visited connection.
    using _PathToResultMap =
        TfHashMap<unsigned int, VdfConnectionConstVector, TfHash>;

    // A map from path to all its children paths.
    using _PathToPathChildrenMap =
        TfHashMap<unsigned int, std::vector<unsigned int>>;

    // A set of pending results.
    using _PotentialResults = 
        TfHashSet<_PotentialResult, _PotentialResult::HashFunctor>;

    // A map from path-id to first relevant input discovered.
    using _PathToRelevanceMap = 
        TfHashMap<unsigned int, const VdfInput *, TfHash>;

private:

    // Helper to traverse a frame.
    void _TraverseFrame(const _StackFrame &frame, bool isStartFrame);

    // Helper for _TraverseOutput.
    void _TraverseSeenConnection(
        const _PathSegment &ending, const _PathSegment &encountered);

    // Helper to build the full path from pathId (including all parent paths).
    VdfConnectionConstVector _BuildFullPath(
        const _PathSegment &end, const _PathSegment *start) const;

    // Helper to finalize pending results.
    void _FinalizePendingResults(
        std::vector<VdfConnectionConstVector> *paths) const;

    // Helper to _FinalizePendingResults().
    void _AppendChildPathsToWorkingSet(
        std::set<unsigned int> *pathToLookup,
        unsigned int pathId,
        const _PathSegment &encounteredSegment,
        unsigned int joiningPathId) const;

private:

    // The output that we are searching for.
    const VdfOutput *_targetOutput;

    // The input callback used to determine if a path is relevant.
    InputCallback _inputCallback;

    // The discovered paths index via path index.
    std::vector<VdfConnectionConstVector> _paths;

    // A map from path-id to relevance group.
    _PathToRelevanceMap _pathToRelevanceMap;

    // Map from VdfConnection pointer to _PathSegment.  Tracks visited
    // connections.
    _VisitedConnectionsInfoMap _visitedConnectionsInfoMap;

    // Map from path to parent _PathSegment.  This is used to quickly find
    // parent path segments or a path.  Note that the parent is a path segment
    // because the parent path may continue after a child forked off it.
    _PathToParentSegmentMap _pathToParentSegmentMap;

    // A map from path to all its children paths.
    _PathToPathChildrenMap _pathToPathChildrenMap;

    // A map from path to result connection vector.  This map tracks the 
    // directly found results during traversal.  A directly found result is
    // when the traversal manages to find the result node without finding a
    // previously visited connection.  Note that not all paths have results.
    _PathToResultMap _pathToResultMap;

    // A vector of pending stack frames for traversal.
    std::vector<_StackFrame> _stack;

    // A set of pending results.
    _PotentialResults _potentialResults;
};

#endif

PXR_NAMESPACE_CLOSE_SCOPE
