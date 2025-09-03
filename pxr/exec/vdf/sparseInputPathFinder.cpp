//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/sparseInputPathFinder.h"

#include "pxr/exec/vdf/connection.h"
#include "pxr/exec/vdf/debugCodes.h"

#include "pxr/base/trace/trace.h"

#include "pxr/base/tf/hash.h"
#include "pxr/base/tf/stl.h"

PXR_NAMESPACE_OPEN_SCOPE

void
VdfSparseInputPathFinder::Traverse(
    const VdfMaskedOutput &start,
    const VdfMaskedOutput &target,
    InputCallback inputCallback,
    std::vector<VdfConnectionConstVector> *paths)
{
    VdfSparseInputPathFinder(start, target, inputCallback, paths);
}

void
VdfSparseInputPathFinder::FindAllCyclePaths(
    const VdfMaskedOutput &start,
    InputCallback inputCallback,
    std::vector<VdfConnectionConstVector> *paths)
{
    VdfSparseInputPathFinder(start, start, inputCallback, paths);
}

VdfSparseInputPathFinder::VdfSparseInputPathFinder(
    const VdfMaskedOutput &start,
    const VdfMaskedOutput &target,
    InputCallback inputCallback,
    std::vector<VdfConnectionConstVector> *paths)
:   _targetOutput(target.GetOutput()),
    _inputCallback(inputCallback)
{
    TRACE_FUNCTION();

    TfAutoMallocTag2 tag(
        "Vdf", "VdfSparseInputPathFinder::VdfSparseInputPathFinder");

    TF_VERIFY(_targetOutput && paths && inputCallback);

    TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
        Msg("\n[VdfSparseInputPathFinder] Starting sparse input path find "
            "traversal at \"%s 0b%s\" looking for \"%s\".\n",
            start.GetOutput()->GetDebugName().c_str(),
            start.GetMask().GetRLEString().c_str(),
            _targetOutput->GetDebugName().c_str());

    // Setup initial path...
    _paths.push_back(VdfConnectionConstVector());
    _stack.push_back(_StackFrame(start, _PathSegment(0, 0)));

    // Loop while we've got work to do.
    bool isStartFrame = true;

    while (!_stack.empty()) {

        // Get the next frame to process.
        //
        // Since we're popping the frame off the stack, we have to be
        // careful to copy it by value, and not just get a reference.
        //
        // Also, by popping back at the end we implement a DFS.

        _StackFrame frame = _stack.back();
        _stack.pop_back();

        // Process the output.
        _TraverseFrame(frame, isStartFrame);
        isStartFrame = false;
    }

    // Full copy of directly found paths.
    for(const _PathToResultMap::value_type &i : _pathToResultMap)
        paths->push_back(i.second);

    // Finalize any pending results.
    _FinalizePendingResults(paths);

    TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
        Msg("\n[VdfSparseInputPathFinder] Stats:\n"
            " # of paths:                          %zu\n"
            " # of visited connections:            %zu\n"
            " # of path to parent segments:        %zu\n"
            " # of path to children paths vectors: %zu\n"
            " # of directly found results:         %zu\n"
            " # of potential results found:        %zu\n"
            " # of total results found:            %zu\n\n",
            _paths.size(),
            _visitedConnectionsInfoMap.size(),
            _pathToParentSegmentMap.size(),
            _pathToPathChildrenMap.size(),
            _pathToResultMap.size(),
            _potentialResults.size(),
            paths->size());
}

void
VdfSparseInputPathFinder::_AppendChildPathsToWorkingSet(
    std::set<unsigned int> *pathToLookup,
    unsigned int pathId,
    const _PathSegment &encounteredSegment,
    unsigned int endingId) const
{
    // Insert children into working set...
    const _PathToPathChildrenMap::const_iterator iter =
        _pathToPathChildrenMap.find(pathId);

    if (iter == _pathToPathChildrenMap.end())
        return;

    for(const unsigned int childPathId : iter->second) {

        // Note that childPathId is a child of path.  However, and that is 
        // important, we need to see if it is a child that begins after 
        // encounteredSegment. 

        _PathToParentSegmentMap::const_iterator j =
            _pathToParentSegmentMap.find(childPathId);

        // Must be able to find parent if we just found childPathId being a 
        // child of path. 

        TF_VERIFY(
            j != _pathToParentSegmentMap.end() && j->second.id == pathId);

        const unsigned int childStartInParentPath = j->second.len;

        // we don't add sibling paths anymore
        TF_VERIFY(childStartInParentPath);

        // Looking at root?
        if (pathId == encounteredSegment.id) {

            if (childStartInParentPath <= encounteredSegment.len) {

                TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
                    Msg("[VdfSparseInputPathFinder] ...... ignoring child path "
                        "%u starting at %u because it does not include "
                        "connection %s.\n",
                        childPathId, childStartInParentPath,
                        encounteredSegment.GetAsString().c_str());

                continue;
            }
        }

        // Ignore cycles.
        if (childPathId == endingId) {

            TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
                Msg("[VdfSparseInputPathFinder] ...... ignoring child path %u "
                    "because it is a cycle.\n", childPathId);
        
            continue;
        }

        TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
            Msg("[VdfSparseInputPathFinder] ...... path %u has child-path "
                "%u starting at %u, queuing lookup.\n",
                pathId, childPathId, childStartInParentPath);
    
        pathToLookup->insert(childPathId);
    }
}

void
VdfSparseInputPathFinder::_FinalizePendingResults(
    std::vector<VdfConnectionConstVector> *paths) const
{
    TRACE_FUNCTION();

    TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
        Msg("\n[VdfSparseInputPathFinder] Finalizing Pending Results:\n");

    for(const _PotentialResult &potentialResult : _potentialResults) {

        const _PathSegment &ending = potentialResult.ending;
        const _PathSegment &encountered = potentialResult.encountered;

        TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
            Msg("[VdfSparseInputPathFinder] ... segment %s (final len= %zu) "
                "leads into %s (which is the first index to be incl. into "
                "result path).\n",
                ending.GetAsString().c_str(), _paths[ending.id].size(),
                encountered.GetAsString().c_str());

        // Be lazy about the common prefix path.
        bool prefixPathBuilt = false;
        VdfConnectionConstVector prefix;

        // Find all children of encountered.id. 
        std::set<unsigned int> pathToLookup;
        pathToLookup.insert(encountered.id);

        while (!pathToLookup.empty()) {

            const unsigned int pathId = *pathToLookup.begin();
            pathToLookup.erase(pathId);

            _AppendChildPathsToWorkingSet(
                &pathToLookup, pathId, encountered, ending.id);

            // If path doesn't lead to the target ignore it.
            if (!TfMapLookupPtr(_pathToResultMap, pathId)) {

                TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
                    Msg("[VdfSparseInputPathFinder] ... path %u doesn't reach "
                        "target, ignoring.\n", pathId);

                continue;
            }

            TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
                Msg("[VdfSparseInputPathFinder] ... path %u reaches target.\n",
                    pathId);

            // Build common prefix path.
            if (!prefixPathBuilt) {

                TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
                    Msg("[VdfSparseInputPathFinder] ... building prefix path "
                        "%s.\n", ending.GetAsString().c_str());

                // Prefix is the segment of the path that was encountering the
                // previously traversed merge point.
                prefix = _BuildFullPath(ending, nullptr /* start */);
                prefixPathBuilt = true;
            }

            // Note that encountered marks the first connection we need to 
            // include. 

            TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
                Msg("[VdfSparseInputPathFinder] ... building postfix path %u, "
                    "starting at %s.\n",
                    pathId, encountered.GetAsString().c_str());

            const VdfConnectionConstVector postfix =
                _BuildFullPath(
                    _PathSegment(pathId, _paths[pathId].size()), &encountered);

            // Create merged, final result path.
            VdfConnectionConstVector result(prefix);
            result.insert(result.end(), postfix.begin(), postfix.end());

            // Add the result path directly into the result set (ie. don't
            // store in in _pathToResultMap since we won't have a path id
            // to associate it with.  This is because we never assigned a
            // pathId to it (and thus the path isn't available via _paths[]).

            paths->push_back(result);
    
            if (TfDebug::IsEnabled(VDF_SPARSE_INPUT_PATH_FINDER)) {
                TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
                    Msg("[VdfSparseInputPathFinder] ... added result no. %zu "
                        "as path of len %zu:\n", paths->size(), result.size());
    
                for(const VdfConnection *c : result) {
                    TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
                        Msg("[VdfSparseInputPathFinder] ...... %s\n",
                            c->GetDebugName().c_str());
                }
            }
        }
    }
}

VdfConnectionConstVector
VdfSparseInputPathFinder::_BuildFullPath(
    const _PathSegment &end, const _PathSegment *start) const
{
    VdfConnectionConstVector result;

    TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
        Msg("[VdfSparseInputPathFinder] BuildFullPath() for end %s, "
            "start %s#%u at %u:\n",
            end.GetAsString().c_str(),
            start ? "(explicit) " : "",
            start ? start->id  : end.id,
            start ? start->len : 0);

    // Gather all path indices for pathId and its parents.
    std::vector<_PathSegment> segments;

    _PathSegment segment = end;

    while (true) {

        TF_VERIFY(
            segment.id < _paths.size() &&
            segment.len <= _paths[segment.id].size(),
            "segment: id %u, len %u, sz %zu",
            segment.id, segment.len, _paths[segment.id].size());

        segments.push_back(segment);

        TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
            Msg("[VdfSparseInputPathFinder] ... discovered %s\n",
                segment.GetAsString().c_str());

        _PathToParentSegmentMap::const_iterator iter =
            _pathToParentSegmentMap.find(segment.id);

        if (iter == _pathToParentSegmentMap.end())
            break;

        segment = iter->second;
    }

    // Now the end of the path is at pathIndices[0] followed by earlier parts
    // of the path.

    bool includeSegment = !start;

    TF_REVERSE_FOR_ALL(i, segments) {

        const VdfConnectionConstVector &path = _paths[i->id];

        unsigned int startIndex = 0;

        // If we have a start segment specified make sure to ignore all
        // elements and segments before the actual start point.

        if (start && i->id == start->id) {
            includeSegment = true;
            startIndex = start->len;
        }

        TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
            Msg("[VdfSparseInputPathFinder] ... segment %s: includeSegment %d, "
                "startIndex %u\n",
                i->GetAsString().c_str(), includeSegment, startIndex);

        if (!includeSegment)
            continue;

        result.insert(
            result.end(), path.begin() + startIndex, path.begin() + i->len);
    }

    if (TfDebug::IsEnabled(VDF_SPARSE_INPUT_PATH_FINDER)) {
    
        for(const VdfConnection *c : result) {
            TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
                Msg("[VdfSparseInputPathFinder] ...... %s\n",
                    c->GetDebugName().c_str());
        }
    }

    return result;
}

void
VdfSparseInputPathFinder::_TraverseSeenConnection(
    const _PathSegment &ending, const _PathSegment &encountered)
{
    // Note that encountered holds the length of the path including the
    // connection itself, thus its length must be always > 0.

    TF_VERIFY(encountered.len > 0);

    TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
        Msg("[VdfSparseInputPathFinder] ... encountered %s while traversing "
            "with ending %s.\n",
            encountered.GetAsString().c_str(),
            ending.GetAsString().c_str());

    // Here two paths merge.  If they are both relevant and they have different 
    // relevance ids we need to track the result.  Note that we don't track the 
    // case when a non relevant paths meets a relevant one. 

    const VdfInput *relEnding = nullptr;
    TfMapLookup(_pathToRelevanceMap, ending.id, &relEnding);

    if (relEnding) {

        const VdfInput *relEncountered = nullptr;
        TfMapLookup(_pathToRelevanceMap, encountered.id, &relEncountered);

        // Note that a potential result will be queued if the current frame has
        // relevance that either differs from encountered's relevance or 
        // encountered has no relevance at all. 

        if (relEnding != relEncountered) {

            // Note: len-1: because the found connection is already included in 
            //       encountered and the _PotentialResult holds the last index
            //       to be included in the potential result path.
    
            TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
                Msg("[VdfSparseInputPathFinder] ... queued potential result "
                    "for ending %s (relevance %p) leading into encountered %s "
                    "(relevance %p).\n",
                    ending.GetAsString().c_str(), relEnding,
                    encountered.GetAsString().c_str(), relEncountered);
    
            _potentialResults.insert(
                _PotentialResult(
                    ending, _PathSegment(encountered.id, encountered.len-1)));
        }
    }
}

void
VdfSparseInputPathFinder::_TraverseFrame(
    const _StackFrame &frame, bool isStartFrame)
{
    const VdfMaskedOutput &maskedOutput = frame.maskedOutput;
    const VdfOutput *output = maskedOutput.GetOutput();

    TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
        Msg("[VdfSparseInputPathFinder] Visiting via %s: \"%s\" '%s':\n",
            frame.GetAsString().c_str(),
            output->GetDebugName().c_str(),
            maskedOutput.GetMask().GetRLEString().c_str());

    const VdfNode &node = output->GetNode();

    // We can only have reached the target if there is overlap between the
    // affects mask (if there is one) and the mask for the output in the current
    // frame.
    const bool isAffective = [&] {
        if (const VdfMask *const affectsMask = output->GetAffectsMask()) {
            return affectsMask->Overlaps(maskedOutput.GetMask());
        }
        return true;
    }();

    // Check to see if we've reached our target.
    if (!isStartFrame && isAffective && output == _targetOutput) {

        TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
            Msg("[VdfSparseInputPathFinder] ... ! found target output via "
                "pathId= %u.\n", frame.id);

        // Assemble final path by looking at all parents and concatenate. We
        // can pass frame because it is the correct length at this point.
        const VdfConnectionConstVector result =
            _BuildFullPath(frame, nullptr /* start */);
        TF_VERIFY(!result.empty());

        // Add result path to path-to-result map.
        _pathToResultMap[frame.id] = result;

        if (TfDebug::IsEnabled(VDF_SPARSE_INPUT_PATH_FINDER)) {
            TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
                Msg("[VdfSparseInputPathFinder] ... added result path %u, len "
                    "%zu:\n", frame.id, result.size());
    
            for(const VdfConnection *c : result) {
                TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
                    Msg("[VdfSparseInputPathFinder] ...... %s\n",
                        c->GetDebugName().c_str());
            }
        }

        return;
    }

    // Loop over all inputs and extend or spawn new paths as needed.  Also
    // check if we run into an earlier path and add pending results in that
    // case.  Need to add pending results because we don't know if the path
    // we ran into is finialized yet.

    bool continueFramePath = true;

    // Is this stack frame re-traversing?  If so, we can't continue the existing
    // segment (aka. we need to create new paths for each new connection (since 
    // a new connection means that we haven't seen it the first time around)). 
    // This needs to be done so that we don't break existing paths.
    // 
    // We do this by setting dependentConnectionsVisisted to 1, which will 
    // ensure that we always start a new path segment (and don't extend).

    if (_paths[frame.id].size() != frame.len)
        continueFramePath = false;

    // Iterate over all inputs and input connections coming into this node.
    for(const std::pair<TfToken, VdfInput *> &i : node.GetInputsIterator()) {

        const VdfInput &input = *i.second;

        const bool isRelevantInput =
            _inputCallback ? _inputCallback(input) : false;

        for(const VdfConnection *c : input.GetConnections()) {

            // Ask the node what mask to use when traversing this input
            // connection.

            const VdfMask::Bits dependencyMaskBits =
                node.ComputeInputDependencyMask(maskedOutput, *c);

            // If there are no bits set in the mask, there's nothing to do.
            if (dependencyMaskBits.AreAllUnset()) {
                continue;
            }

            VdfMask dependencyMask(dependencyMaskBits);

            TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
                Msg("[VdfSparseInputPathFinder] ... traversing \"%s\" '%s':\n",
                    c->GetDebugName().c_str(),
                    dependencyMask.GetRLEString().c_str());

            // See if we have already visited this connection...
            if (const _VisitedDependencyToSegmentMap *d = TfMapLookupPtr(
                _visitedConnectionsInfoMap, c)) {

                // ...and see if we have already visited it with the same
                // dependencyMask.

                if (const _PathSegment *encountered =
                    TfMapLookupPtr(*d, dependencyMask)) {
                
                    _TraverseSeenConnection(frame, *encountered);

                    // Skip this connection as we've already traversed it with 
                    // the same mask.
                    continue;
                }
            }

            // Once we are here, we are sure we didn't see this path segment
            // with a matching relevance.

            // Now continue or spawn new (parent or sibling) path.
            _PathSegment visitedSegment;

            const VdfInput *relevance = nullptr;
        
            // The first dependent connection will extend the current path 
            // segment. All others will create a new path segment.
            
            if (continueFramePath) {
            
                continueFramePath = false;

                // Continue the current path 'frame.id'.
                visitedSegment = _PathSegment(frame.id, frame.len + 1);

                TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
                    Msg("[VdfSparseInputPathFinder] ...... continuing as segment "
                        "%s.\n", visitedSegment.GetAsString().c_str());

                // Is this input relevant and the path needs to be marked? Note
                // that we only the record the first relevant input.  If we 
                // ever discover another one (without branching) it won't 
                // matter.  If we branch or come back to a previously ignored
                // relevant input we will create a new path segment and thus
                // can mark it then.

                if (isRelevantInput && !TfMapLookupPtr(
                    _pathToRelevanceMap, frame.id)) {

                    relevance = &input;
                }
                
            } else {

                // Alloc new path, mark as child of current path.
                const unsigned int newPathId = _paths.size();
                _paths.push_back(VdfConnectionConstVector());

                // We don't need to record parent/child relationships if the
                // parent is empty.
                if (frame.len) {

                    // Record the current frame as a parent segment. 
                    _pathToParentSegmentMap[newPathId] = frame;
                    _pathToPathChildrenMap[frame.id].push_back(newPathId);

                    TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
                        Msg("[VdfSparseInputPathFinder] ...... branched as new "
                            "child %u at %s.\n",
                            newPathId, frame.GetAsString().c_str());

                } else {

                    // Sibling paths can get created when the starting node
                    // has multiple connections being traversed.

                    TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
                        Msg("[VdfSparseInputPathFinder] ...... branched as new "
                            "sibling %u at %u.\n", newPathId, frame.id);
                }

                visitedSegment = _PathSegment(newPathId, 1);

                // Mark new sibling or child path as relevant if the input is.
                if (isRelevantInput)
                    relevance = &input;
            }

            // Assign any relevance?
            if (relevance) {

                TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
                    Msg("[VdfSparseInputPathFinder] ... is relevant: marking "
                        "path #%u with relevance %p.\n",
                        visitedSegment.id, relevance);

                _pathToRelevanceMap[visitedSegment.id] = relevance;
            }

            // Must have a valid visited segment now.
            TF_VERIFY(visitedSegment.len >= 1);

            // Note that at this point we always need to mark the connection.
            // This is because it is either visited for the first time or 
            // it is expanded.  If it wouldn't be expanded, we would have 
            // continued in the code above.

            TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
                Msg("[VdfSparseInputPathFinder] ... visited via %s: \"%s\" "
                    "'%s'\n",
                    visitedSegment.GetAsString().c_str(),
                    c->GetDebugName().c_str(),
                    dependencyMask.GetRLEString().c_str());

            // Found a connection that we have not seen before at all.
            const _VisitedConnectionsInfoMap::iterator visitedIter =
                _visitedConnectionsInfoMap.insert(
                    {c, _VisitedDependencyToSegmentMap()}).first;

            const bool res = visitedIter->second.insert(
                {dependencyMask, visitedSegment}).second;

            // Consistency checks.
            TF_VERIFY(
                res && visitedSegment.id < _paths.size() &&
                _paths[visitedSegment.id].size() == visitedSegment.len - 1);

            // Append connection to the current path.
            _paths[visitedSegment.id].push_back(c);

            // Setup new stack frame.
            _stack.push_back(_StackFrame(
                VdfMaskedOutput(&c->GetNonConstSourceOutput(), dependencyMask),
                visitedSegment));
        }
    }

    if (continueFramePath) {
        TF_DEBUG(VDF_SPARSE_INPUT_PATH_FINDER).
            Msg("[VdfSparseInputPathFinder] ... - path %u ended here because no "
                "new relevant connections have been found.\n", frame.id);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
