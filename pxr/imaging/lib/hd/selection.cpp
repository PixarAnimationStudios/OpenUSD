//
// Copyright 2018 Pixar
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
#include "pxr/imaging/hd/selection.h"

#include "pxr/base/tf/diagnostic.h"
#include "pxr/imaging/hd/debugCodes.h"

#include <iterator> // std::distance

PXR_NAMESPACE_OPEN_SCOPE

void 
HdSelection::AddRprim(HdSelection::HighlightMode const& mode,
                      SdfPath const& path)
{
    TF_VERIFY(mode < HdSelection::HighlightModeCount);
    _selMap[mode][path].fullySelected = true;
    TF_DEBUG(HD_SELECTION_UPDATE).Msg(
            "Adding Rprim %s to HdSelection (mode %d)", path.GetText(), mode);
}

void 
HdSelection::AddInstance(HdSelection::HighlightMode const& mode,
                         SdfPath const& path,
                         VtIntArray const &instanceIndices/*=VtIntArray()*/)
{
    TF_VERIFY(mode < HdSelection::HighlightModeCount);

    if (instanceIndices.empty()) {
        // For instances, an empty instanceIndices array implies that all
        // instances are selected. Since instances are tied to an rprim (i.e.,
        // they share the same primId), this effectively means that all
        // instances of the rprim are selected.
        _selMap[mode][path].fullySelected = true;
    }
    _selMap[mode][path].instanceIndices.push_back(instanceIndices);
    TF_DEBUG(HD_SELECTION_UPDATE).Msg(
            "Adding instances of Rprim %s to HdSelection (mode %d)",
            path.GetText(), mode);
}

void 
HdSelection::AddElements(HdSelection::HighlightMode const& mode,
                         SdfPath const& path,
                         VtIntArray const &elementIndices)
{
    TF_VERIFY(mode < HdSelection::HighlightModeCount);

    if (elementIndices.empty()) {
        // For element (faces) subprims alone, we use an empty indices array to
        // succintly encode that all elements are selected.
        _selMap[mode][path].fullySelected = true;
        TF_DEBUG(HD_SELECTION_UPDATE).Msg(
            "Adding Rprim (via AddElements) %s to HdSelection (mode %d)",
            path.GetText(), mode);
    } else {
        _selMap[mode][path].elementIndices.push_back(elementIndices);
        TF_DEBUG(HD_SELECTION_UPDATE).Msg(
            "Adding elements of Rprim %s to HdSelection (mode %d)",
            path.GetText(), mode);
    }
}

void 
HdSelection::AddEdges(HdSelection::HighlightMode const& mode,
                      SdfPath const& path,
                      VtIntArray const &edgeIndices)
{
    TF_VERIFY(mode < HdSelection::HighlightModeCount);
    // For edges & points, we skip empty indices arrays
    if (!edgeIndices.empty()) {
        _selMap[mode][path].edgeIndices.push_back(edgeIndices);
        TF_DEBUG(HD_SELECTION_UPDATE).Msg(
            "Adding edges of Rprim %s to HdSelection (mode %d)",
            path.GetText(), mode);
    }
}

void 
HdSelection::AddPoints(HdSelection::HighlightMode const& mode,
                       SdfPath const& path,
                       VtIntArray const &pointIndices)
{
    TF_VERIFY(mode < HdSelection::HighlightModeCount);

    // When points are added without a color, we use -1 to encode this.
    _AddPoints(mode, path, pointIndices, /*pointColorIndex=*/-1);
}

void 
HdSelection::AddPoints(HdSelection::HighlightMode const& mode,
                       SdfPath const& path,
                       VtIntArray const &pointIndices,
                       GfVec4f const& pointColor)
{
    TF_VERIFY(mode < HdSelection::HighlightModeCount);

    // When points are added with a color, add it to the tracked colors if
    // needed, and use the resulting index 
    auto const & pointColorIt = std::find(_selectedPointColors.begin(),
                                          _selectedPointColors.end(),
                                          pointColor);
    size_t pointColorId = 0;
    if (pointColorIt == _selectedPointColors.end()) {
        pointColorId = _selectedPointColors.size();
        _selectedPointColors.push_back(pointColor);
    } else {
        pointColorId =
            std::distance(_selectedPointColors.begin(), pointColorIt);
    }
    _AddPoints(mode, path, pointIndices, (int) pointColorId);
}

SdfPathVector
HdSelection::GetSelectedPrimPaths(HdSelection::HighlightMode const& mode) const
{
    TF_VERIFY(mode < HdSelection::HighlightModeCount);
    // Aggregate rprim paths in the selection map.
    SdfPathVector selectedPrims;
    for (auto const &pair : _selMap[mode]) {
        selectedPrims.push_back(pair.first);
    }

    return selectedPrims;
}

HdSelection::PrimSelectionState const*
HdSelection::GetPrimSelectionState(HdSelection::HighlightMode const& mode,
                                   SdfPath const &path) const
{
    TF_VERIFY(mode < HdSelection::HighlightModeCount);
    auto const &it = _selMap[mode].find(path);
    if (it != _selMap[mode].end()) {
        return &(it->second);
    } else {
        return nullptr;
    }
}

std::vector<GfVec4f> const&
HdSelection::GetSelectedPointColors() const
{
    return _selectedPointColors;
}

void
HdSelection::_AddPoints(HdSelection::HighlightMode const& mode,
                       SdfPath const& path,
                       VtIntArray const &pointIndices,
                       int pointColorIndex)
{
    // For edges & points, we skip empty indices arrays
    if (!pointIndices.empty()) {
        _selMap[mode][path].pointIndices.push_back(pointIndices);
        _selMap[mode][path].pointColorIndices.push_back(pointColorIndex);
        TF_DEBUG(HD_SELECTION_UPDATE).Msg(
            "Adding points of Rprim %s to HdSelection (mode %d) with point"
            " color index %d", path.GetText(), mode, pointColorIndex);
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
