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

//------------------------------------------------------------------------------
//                              HdxSelection
//------------------------------------------------------------------------------
void 
HdxSelection::AddRprim(HdxSelectionHighlightMode const& mode,
         SdfPath const& path)
{
    TF_VERIFY(mode < HdxSelectionHighlightModeCount);
    _selEntities[mode].prims.push_back(path);
}

void 
HdxSelection::AddInstance(HdxSelectionHighlightMode const& mode,
            SdfPath const& path,
            VtIntArray const &instanceIndex)
{
    TF_VERIFY(mode < HdxSelectionHighlightModeCount);
    _selEntities[mode].prims.push_back(path);
    _selEntities[mode].instances[path].push_back(instanceIndex);
}

void 
HdxSelection::AddElements(HdxSelectionHighlightMode const& mode,
            SdfPath const& path,
            VtIntArray const &elementIndices)
{
    TF_VERIFY(mode < HdxSelectionHighlightModeCount);
    _selEntities[mode].prims.push_back(path);
    _selEntities[mode].elements[path] = elementIndices;
}

void 
HdxSelection::AddEdges(HdxSelectionHighlightMode const& mode,
            SdfPath const& path,
            VtIntArray const &edgeIndices)
{
    TF_VERIFY(mode < HdxSelectionHighlightModeCount);
    _selEntities[mode].prims.push_back(path);
    _selEntities[mode].edges[path] = edgeIndices;
}

SdfPathVector const&
HdxSelection::GetSelectedPrims(HdxSelectionHighlightMode const& mode) const
{
    TF_VERIFY(mode < HdxSelectionHighlightModeCount);
    return _selEntities[mode].prims;
}

HdxSelection::InstanceMap const&
HdxSelection::GetSelectedInstances(HdxSelectionHighlightMode const& mode) const
{
    TF_VERIFY(mode < HdxSelectionHighlightModeCount);
    return _selEntities[mode].instances;
}

HdxSelection::ElementIndicesMap const&
HdxSelection::GetSelectedElements(HdxSelectionHighlightMode const& mode) const
{
    TF_VERIFY(mode < HdxSelectionHighlightModeCount);
    return _selEntities[mode].elements;
}

HdxSelection::EdgeIndicesMap const&
HdxSelection::GetSelectedEdges(HdxSelectionHighlightMode const& mode) const
{
    TF_VERIFY(mode < HdxSelectionHighlightModeCount);
    return _selEntities[mode].edges;
}

//------------------------------------------------------------------------------
//                           HdxSelectionTracker
//------------------------------------------------------------------------------
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
HdxSelectionTracker::GetSelectionOffsetBuffer(HdRenderIndex const* index,
                                              VtIntArray* offsets) const
{
    TRACE_FUNCTION();
    TfAutoMallocTag2 tag("Hdx", "GetSelectionOffsetBuffer");

     // XXX: Set minimum size for UBO/SSBO requirements. Seems like this should
    // be handled by Hydra. Update all uses of minSize below when resolved.
    const int minSize = 8;
    offsets->resize(minSize);

    // We expect the collection of selected objects to be created externally and
    // set via SetSelection. Exit early if the tracker doesn't have one set.
    if (!_selection) {
        return false;
    }

    // -------------------------------------------------------------------------
    // Hydra supports selection highlighting of:
    // (a) a set of prims, wherein each prim is entirely highlighted
    // (b) a set of instances of (a), wherein each instance is highlighted
    // (c) a set of subprimitives of (a) or (b), wherein each subprim is
    //     highlighted.
    // Subprimitives support is limited to elements (faces or curves), with
    // support for edges and points coming soon.
    //
    // The current selection implementation is, in a sense, global in nature. 
    // If there are no selected objects, we do not bind any selection-related 
    // resources, nor does the shader execute any selection-related operations.
    // 
    // If there are one or more selected objects, we *don't* choose to have them
    // in a separate 'selection' collection.
    // Instead, we stick by AZDO principles and avoid command buffer changes as
    // a result of selection updates (which would involve removal of draw items
    // corresponding to the selected objects, and adding them to a separate 
    // "selection pass")
    // 
    // Given a set of selected objects, we encode it into a integer buffer that 
    // allows us to perform a minimal set of lookups to quickly tell us if a
    // fragment needs to be highlighted.
    // 
    // XXX: We currently update the selection buffer in its *entirety* on 
    // each update, resulting in poor performance when a large number of 
    // objects are already selected.
    // -------------------------------------------------------------------------

    // Populate a selection offset buffer that holds offset data per selection
    // highlight mode. See 'Buffer Layout' for the per-selection mode layout.
    // 
    // The full layout is:
    // [# modes] [per-mode offsets] [seloffsets mode0] ... [seloffsets modeM]
    // [--------  header  --------]
    // 
    // Example:
    // [   3   ] [4,     0,     10] [seloffsets mode0] [seloffsets mode2]
    //            |              |  ^                  ^
    //            |              |__|__________________|
    //            |_________________|
    //  
    //  Above:
    //  Index 0 holds the number of selection highlight modes (3)
    //  Indices [1-3] hold the start index for each mode's data.
    //  If a mode doesn't have any highlighted objects, we use 0 to encode this.
    //  
    //  Indices [4-9] hold the selection offsets for mode 0.
    //  Indices [10-x] hold the selection offsets for mode 2.
    //  Mode 1 can be skipped since its mode offset is 0.
    //  See hdx/shaders/renderPass.glslfx (ApplySelectionColor) for the
    //  shader readback of this buffer

    bool hasSelection = false;
    const size_t numHighlightModes = 
        static_cast<size_t>(HdxSelectionHighlightModeCount);
    const size_t headerSize = numHighlightModes /*per mode offsets*/
                              + 1               /*num modes*/;

    if (ARCH_UNLIKELY(numHighlightModes >= minSize)) {
        // allocate enough to hold the header
        offsets->resize(headerSize);
    }

    (*offsets)[0] = numHighlightModes;

    const int SELECT_NONE = 0;
    size_t copyOffset = headerSize;

    for (int mode = HdxSelectionHighlightModeSelect;
             mode < HdxSelectionHighlightModeCount;
             mode++) {
       
        std::vector<int> output;
        bool modeHasSelection = _GetSelectionOffsets(
                                static_cast<HdxSelectionHighlightMode>(mode), 
                                index, copyOffset, &output);
        hasSelection = hasSelection || modeHasSelection;

        (*offsets)[mode + 1] = modeHasSelection? copyOffset : SELECT_NONE;

        if (modeHasSelection) {
            // append the offset buffer for the highlight mode
            offsets->resize(output.size() + copyOffset);

            for (size_t i = 0; i < output.size(); i++) {
                (*offsets)[i + copyOffset] = output[i];
            }

            copyOffset += output.size();

            TF_DEBUG(HDX_SELECTION_SETUP).Msg("Highlight mode %d has %lu "
                "entries\n", mode, output.size());
        }
    }

    if (!hasSelection) {
        return false;
    }

    _DebugPrintArray("final output", *offsets);

    return true;
}

// helper methods to fill the selection buffer
static
int _EncodeSubprimSel(size_t netSubprimOffset, int flag)
{
    return int(netSubprimOffset << 1) | flag;
}

static
void _EncodeSubprimTypeAndRange(std::vector<int>* output, size_t offset,
                                int type, int min, int max)
{
    (*output)[offset    ] = type;
    (*output)[offset + 1] = min;
    (*output)[offset + 2] = max;
}

/*virtual*/
bool
HdxSelectionTracker::_GetSelectionOffsets(HdxSelectionHighlightMode const& mode,
                                          HdRenderIndex const *index,
                                          size_t modeOffset,
                                          std::vector<int> *output) const
{

    SdfPathVector const& selectedPrims =  _selection->GetSelectedPrims(mode);
    size_t numPrims = _selection ? selectedPrims.size() : 0;
    if (numPrims == 0) {
        TF_DEBUG(HDX_SELECTION_SETUP).Msg(
            "No selected prims for mode %d\n", mode);
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
       [&ids, &index, INVALID, &N, &selectedPrims, this](size_t begin, size_t end) mutable {
        end = std::min(end*N, ids.size());
        begin = begin*N;
        for (size_t i = begin; i < end; i++) {
            if (auto const& rprim = index->GetRprim(selectedPrims[i])) {
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
        return false;
    }

    // ---------------------------------------------------------------------- //
    // Buffer Layout
    // ---------------------------------------------------------------------- //
    // In the folowing code, we want to build up a buffer that is capable of
    // driving selection highlighting. To do this, we leverage the fact that all
    // shaders have access to the drawing coord, namely the ObjectID,
    // InstanceID, ElementID, VertexID, etc. The idea is to take one such ID and
    // compare it against a range of known selected values within a given range.
    //
    // For example, imagine the ObjectID is 6, then we can know this object is
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

    enum SubPrimType {
        ELEMENT = 0,
        EDGE    = 1,
        POINT   = 2
    };

    _DebugPrintArray("ids", ids);

    output->insert(output->end(), 2+1+max-min, SELECT_NONE);
    (*output)[0] = min;
    (*output)[1] = max+1;

    // XXX: currently, _selectedPrims may have duplicated entries
    // (e.g. to instances and to faces) for an objPath.
    // this would cause unreferenced offset buffer allocated in the
    // result buffer.

    _DebugPrintArray("objects", *output);

    for (size_t primIndex = 0; primIndex < ids.size(); primIndex++) {
        // TODO: store ID and path in "ids" vector
        SdfPath const& objPath = selectedPrims[primIndex];
        int id = ids[primIndex];
        if (id == INVALID) continue;

        TF_DEBUG(HDX_SELECTION_SETUP).Msg("Processing: %d - %s\n",
                id, objPath.GetText());

        bool hasSelectedSubprimitives = false;
        size_t netSubprimOffset = 0;
         //------------------------------------------------------------------- //
        // Subprimitives: Points
        // ------------------------------------------------------------------ //
        
            // XXX: todo
        
        //------------------------------------------------------------------- //
        // Subprimitives: Edges
        // ------------------------------------------------------------------ //
        size_t edgeOffset = output->size();
        if (VtIntArray const *edgeIndices
            = TfMapLookupPtr(_selection->GetSelectedEdges(mode), objPath)) {
            if (edgeIndices->size()) {
                int minEdge = std::numeric_limits<int>::max();
                int maxEdge = std::numeric_limits<int>::lowest();

                for (int const& edgeId : *edgeIndices) {
                    minEdge = std::min(minEdge, edgeId);
                    maxEdge = std::max(maxEdge, edgeId);
                }

                // Grow the edge array to hold edges for this object.
                output->insert(output->end(),maxEdge-minEdge+1+3,
                               _EncodeSubprimSel(netSubprimOffset, SELECT_NONE));

                _EncodeSubprimTypeAndRange(output, edgeOffset,
                                           EDGE, minEdge, maxEdge+1);

                for (int edgeId : *edgeIndices) {
                    (*output)[3+edgeOffset+ (edgeId-minEdge)] =
                        _EncodeSubprimSel(netSubprimOffset, SELECT_ALL);
                }

                hasSelectedSubprimitives = true;
                netSubprimOffset = edgeOffset + modeOffset;

                _DebugPrintArray("edges", *output);
            } else {
                // empty edge indices buffer. do nothing.
            }
        } else {
            // prim doesn't have any selected edges. do nothing.
        }


        // ------------------------------------------------------------------ //
        // Subprimitives: Elements (coarse/authored face(s) for meshes,
        //                individual curve(s) for basis curves)
        // ------------------------------------------------------------------ //
        // Find element sizes, for this object.
        size_t elemOffset = output->size();
        if (VtIntArray const *elementIndices
            = TfMapLookupPtr(_selection->GetSelectedElements(mode), objPath)) {
            if (elementIndices->size()) {
                int minElem = std::numeric_limits<int>::max();
                int maxElem = std::numeric_limits<int>::lowest();

                for (int const& elemId : *elementIndices) {
                    minElem = std::min(minElem, elemId);
                    maxElem = std::max(maxElem, elemId);
                }

                // Grow the element array to hold elements for this object.
                // Use the net offset as the splatted value if this prim
                // has any selected edges. This lets us highlight edges for
                // fragments that aren't part of highlighted elements.
                output->insert(output->end(), maxElem-minElem+1+3,
                           _EncodeSubprimSel(netSubprimOffset, SELECT_NONE));

                _EncodeSubprimTypeAndRange(output, elemOffset,
                                           ELEMENT, minElem, maxElem+1);

                for (int elemId : *elementIndices) {
                    (*output)[3+elemOffset+ (elemId-minElem)] =
                        _EncodeSubprimSel(netSubprimOffset, SELECT_ALL);
                }

                hasSelectedSubprimitives = true;
                netSubprimOffset = elemOffset + modeOffset;

                _DebugPrintArray("elements", *output);
            } else {
                // prim has an empty selected elements buffer.
                // we use this to encode "highlight all faces", which overrides
                // any subprimitives (edges, points) that may be selected
                hasSelectedSubprimitives = false;
            }
        } else {
            // prim doesn't have selected elements, but may have other
            // subprimitives selected. do nothing.
        }

        // ------------------------------------------------------------------ //
        // Instances
        // ------------------------------------------------------------------ //
        // Override netSubprimOffset to SELECT_ALL (which implies that the
        // entire prim or instance is selected) if we don't have any
        // subprim selected. This removes a special case in the code below.
        if (!hasSelectedSubprimitives) {
            netSubprimOffset = SELECT_ALL;
        }
        int prevLevelOffset = (int)netSubprimOffset;

        if (std::vector<VtIntArray> const * a =
            TfMapLookupPtr(_selection->GetSelectedInstances(mode), objPath)) {
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
                (*output)[id-min+2] = netSubprimOffset;
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
                int levelOffset = output->size();
                output->insert(output->end(), objLevelSize, SELECT_NONE);
                (*output)[levelOffset + 0] = levelMin;
                (*output)[levelOffset + 1] = levelMax + 1;
                for (VtIntArray const& instVec : *a) {
                    int instId = instVec[level] - levelMin+2;
                    (*output)[levelOffset+instId] = prevLevelOffset;
                }

                if (level == numLevels-1) {
                    // The offset is the net index within the selection buffer, and
                    // thus, needs to include the mode offset.
                    (*output)[id-min+2] = levelOffset + modeOffset;
                }

                if (ARCH_UNLIKELY(TfDebug::IsEnabled(HDX_SELECTION_SETUP))){
                    std::stringstream name;
                    name << "level[" << level << "]";
                    _DebugPrintArray(name.str(), *output);
                }
                // The offset is the net index within the selection buffer, and
                // thus, needs to include the mode offset.
                prevLevelOffset = levelOffset + modeOffset;
            }
        } else {
            (*output)[id-min+2] = netSubprimOffset;
        }
    }

    _DebugPrintArray("final output", *output);

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE