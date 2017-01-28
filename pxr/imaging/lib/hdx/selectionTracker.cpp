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
#include "pxr/imaging/hdx/selectionTracker.h"

#include "pxr/imaging/hdx/debugCodes.h"

#include "pxr/imaging/hd/renderIndex.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/base/tracelite/trace.h"
#include "pxr/base/work/loops.h"

#include <limits>
#include <iomanip>

PXR_NAMESPACE_OPEN_SCOPE


HdxSelectionTracker::HdxSelectionTracker()
    : _version(0)
{
}

/*virtual*/
void
HdxSelectionTracker::Sync(HdRenderIndex* index)
{
}

int
HdxSelectionTracker::GetVersion() const
{
    return _version;
}

void
HdxSelectionTracker::_IncrementVersion()
{
    ++_version;
}

namespace {
    template <class T>
    void _DebugPrintArray(std::string const& name,
                    T const& array,
                    bool withIndex=true)
    {
        if (ARCH_UNLIKELY(TfDebug::IsEnabled(HDX_SELECTION_SETUP))) {
            std::stringstream out;
            out << name << ": [ ";
            for (auto const& i : array) {
                out << std::setfill(' ') << std::setw(3) << i << " ";
            }
            out << "] (offsets)" << std::endl;

            if (withIndex) {
                // Print the indices
                out << name << ": [ ";
                for (int i = 0; i < array.size(); i++) {
                    out << std::setfill(' ') << std::setw(3) << i << " ";
                }
                out << "] (indices)" << std::endl;
                out << std::endl;
                std::cout << out.str();
            }
        }
    }
}

/*virtual*/
bool
HdxSelectionTracker::GetBuffers(HdRenderIndex const* index,
                         VtIntArray* offsets) const
{
    TRACE_FUNCTION();
    TfAutoMallocTag2 tag("Hdx", "HdxSelection::GetBuffers");

    // XXX: Set minimum size for UBO/SSBO requirements. Seems like this should
    // be handled by Hydra. Update all uses of minSize below when resolved.
    const int minSize = 8;
    size_t numPrims = _selection ? _selection->selectedPrims.size() : 0;
    if (numPrims == 0) {
        TF_DEBUG(HDX_SELECTION_SETUP).Msg("No selected prims\n");
        offsets->resize(minSize);
        return false;
    }

    // Note that numeric_limits<float>::min for is surprising, so using lowest()
    // here instead. Doing this for <int> here to avoid copy and paste bugs.
    int min = std::numeric_limits<int>::max(),
        max = std::numeric_limits<int>::lowest();

    std::vector<int> ids;
    ids.resize(numPrims);

    size_t const N = 1000;
    int const INVALID = -1;
    WorkParallelForN(numPrims/N + 1,
       [&ids, &index, this](size_t begin, size_t end) mutable {
        end = std::min(end*N, ids.size());
        begin = begin*N;
        for (size_t i = begin; i < end; i++) {
            if (auto const& rprim = index->GetRprim(_selection->selectedPrims[i])) {
                ids[i] = rprim->GetPrimId();
            } else {
                // silently ignore non-existing prim
                ids[i] = INVALID;
            }
        }
    });

    for (int id : ids) {
        if (id == INVALID) continue;
        min = std::min(id, min);
        max = std::max(id, max);
    }

    if (max < min) {
        offsets->resize(minSize);
        return false;
    }

    // ---------------------------------------------------------------------- //
    // Buffer Layout
    // ---------------------------------------------------------------------- //
    // In the folowing code, we want to build up a buffer that is capable of
    // driving selection highlighting. To do this, we leverage the fact that all
    // shaders have access to the drawing coord, namely the ObjectID,
    // InstanceID, FaceID, VertexID, etc. The idea is to take one such ID and
    // compare it against a range of known selected values within a given range.
    //
    // For example, imaging the ObjectID is 6, then we can know this object is
    // selected if ID 6 is in the range of selected objects. We then
    // hierarchically re-apply this scheme for instances and faces. The buffer
    // layout is as follows:
    //
    // Object: [ start index | end index | (offset to next level per object) ]
    //
    // So to test if a given object ID is selected, we check if the ID is in the
    // range [start,end), if so, the object's offset in the buffer is ID-start.
    // 
    // The value for an object is one of three cases:
    //
    //  0 - indicates the object is not selected
    //  1 - indicates the object is fully selected
    //  N - an offset to the next level of the hierarchy
    //
    // The structure described above for objects is also applied for each level
    // of instancing as well as for faces. All data is aggregated into a single
    // buffer with the following layout:
    //
    // [ object | element | instance level-N | ... | level 0 ]
    //  
    //  Each section above is prefixed with [start,end) ranges and the values of
    //  each range follow the three cases outlined.
    //
    //  To see these values built incrementally, enable the TF_DEBUG flag
    //  HDX_SELECTION_SETUP.
    // ---------------------------------------------------------------------- //

    // Start with individual arrays. Splice arrays once finished.
    int const SELECT_ALL = 1;
    int const SELECT_NONE = 0;

    _DebugPrintArray("ids", ids);

    std::vector<int> output;
    output.insert(output.end(), 2+1+max-min, SELECT_NONE);
    output[0] = min;
    output[1] = max+1;

    // XXX: currently, _selectedPrims may have duplicated entries
    // (e.g. to instances and to faces) for an objPath.
    // this would cause unreferenced offset buffer allocated in the
    // result buffer.

    _DebugPrintArray("objects", output);

    for (size_t primIndex = 0; primIndex < ids.size(); primIndex++) {
        // TODO: store ID and path in "ids" vector
        SdfPath const& objPath = _selection->selectedPrims[primIndex];
        int id = ids[primIndex];
        if (id == INVALID) continue;

        TF_DEBUG(HDX_SELECTION_SETUP).Msg("Processing: %d - %s\n",
                id, objPath.GetText());

        // ------------------------------------------------------------------ //
        // Elements
        // ------------------------------------------------------------------ //
        // Find element sizes, for this object.
        int elemOffset = output.size();
        if (VtIntArray const *faceIndices
            = TfMapLookupPtr(_selection->selectedFaces, objPath)) {
            if (faceIndices->size()) {
                int minElem = std::numeric_limits<int>::max();
                int maxElem = std::numeric_limits<int>::lowest();

                for (int const& elemId : *faceIndices) {
                    minElem = std::min(minElem, elemId);
                    maxElem = std::max(maxElem, elemId);
                }

                // Grow the element array to hold elements for this object.
                output.insert(output.end(), maxElem-minElem+1+2, SELECT_NONE);
                output[elemOffset+0] = minElem;
                output[elemOffset+1] = maxElem+1;

                for (int elemId : *faceIndices) {
                    // TODO: Add support for edge and point selection.
                    output[2+elemOffset+ (elemId-minElem)] = SELECT_ALL;
                }
                _DebugPrintArray("elements", output);
            } else {
                // Entire object/instance is selected
                elemOffset = SELECT_ALL;
            }
        } else {
            // Entire object/instance is selected
            elemOffset = SELECT_ALL;
        }

        // ------------------------------------------------------------------ //
        // Instances
        // ------------------------------------------------------------------ //
        // Initialize prevLevel to elemOffset which removes a special case in
        // the loops below.
        int prevLevelOffset = elemOffset;
        if (std::vector<VtIntArray> const * a =
            TfMapLookupPtr(_selection->selectedInstances, objPath)) {
            // Different instances can have different number of levels.
            int numLevels = std::numeric_limits<int>::max();
            size_t numInst= a->size();
            if (numInst == 0) {
                numLevels = 0;
            } else {
                for (size_t instNum = 0; instNum < numInst; ++instNum) {
                    size_t levelsForInst = a->at(instNum).size();
                    numLevels = std::min(numLevels,
                                         static_cast<int>(levelsForInst));
                }
            }

            TF_DEBUG(HDX_SELECTION_SETUP).Msg("NumLevels: %d\n", numLevels);
            if (numLevels == 0) {
                output[id-min+2] = elemOffset;
            }
            for (int level = 0; level < numLevels; ++level) {
                // Find the required size of the instance vectors.
                int levelMin = std::numeric_limits<int>::max();
                int levelMax = std::numeric_limits<int>::lowest();
                for (VtIntArray const &instVec : *a) {
                    _DebugPrintArray("\tinstVec", instVec, false);
                    int instId = instVec[level];
                    levelMin = std::min(levelMin, instId);
                    levelMax = std::max(levelMax, instId);
                }

                TF_DEBUG(HDX_SELECTION_SETUP).Msg(
                    "level-%d: min(%d) max(%d)\n",
                    level, levelMin, levelMax);

                int objLevelSize = levelMax - levelMin +2+1;
                int levelOffset = output.size();
                output.insert(output.end(), objLevelSize, SELECT_NONE);
                output[levelOffset + 0] = levelMin;
                output[levelOffset + 1] = levelMax + 1;
                for (VtIntArray const& instVec : *a) {
                    int instId = instVec[level] - levelMin+2;
                    output[levelOffset+instId] = prevLevelOffset; 
                }

                if (level == numLevels-1) {
                    output[id-min+2] = levelOffset;
                }

                if (ARCH_UNLIKELY(TfDebug::IsEnabled(HDX_SELECTION_SETUP))){
                    std::stringstream name;
                    name << "level[" << level << "]";
                    _DebugPrintArray(name.str(), output);
                }
                prevLevelOffset = levelOffset;
            }
        } else {
            output[id-min+2] = elemOffset;
        }
    }
    _DebugPrintArray("final output", output);
    offsets->resize(output.size());
    for (size_t i = 0; i < output.size(); i++) {
        (*offsets)[i] = output[i];
    }
    _DebugPrintArray("final output", *offsets);

    return true;
}


PXR_NAMESPACE_CLOSE_SCOPE

