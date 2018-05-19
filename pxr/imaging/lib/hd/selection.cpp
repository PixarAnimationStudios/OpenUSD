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

PXR_NAMESPACE_OPEN_SCOPE

void 
HdSelection::AddRprim(HdSelection::HighlightMode const& mode,
         SdfPath const& path)
{
    TF_VERIFY(mode < HighlightModeCount);
    _selEntities[mode].prims.push_back(path);
}

void 
HdSelection::AddInstance(HdSelection::HighlightMode const& mode,
            SdfPath const& path,
            VtIntArray const &instanceIndex)
{
    TF_VERIFY(mode < HighlightModeCount);
    _selEntities[mode].prims.push_back(path);
    _selEntities[mode].instances[path].push_back(instanceIndex);
}

void 
HdSelection::AddElements(HdSelection::HighlightMode const& mode,
            SdfPath const& path,
            VtIntArray const &elementIndices)
{
    TF_VERIFY(mode < HighlightModeCount);
    _selEntities[mode].prims.push_back(path);
    _selEntities[mode].elements[path] = elementIndices;
}

void 
HdSelection::AddEdges(HdSelection::HighlightMode const& mode,
            SdfPath const& path,
            VtIntArray const &edgeIndices)
{
    TF_VERIFY(mode < HighlightModeCount);
    _selEntities[mode].prims.push_back(path);
    _selEntities[mode].edges[path] = edgeIndices;
}

void 
HdSelection::AddPoints(HdSelection::HighlightMode const& mode,
            SdfPath const& path,
            VtIntArray const &pointIndices)
{
    TF_VERIFY(mode < HighlightModeCount);
    _selEntities[mode].prims.push_back(path);
    _selEntities[mode].points[path] = pointIndices;
}

SdfPathVector const&
HdSelection::GetSelectedPrims(HdSelection::HighlightMode const& mode) const
{
    TF_VERIFY(mode < HighlightModeCount);
    return _selEntities[mode].prims;
}

HdSelection::InstanceMap const&
HdSelection::GetSelectedInstances(HdSelection::HighlightMode const& mode) const
{
    TF_VERIFY(mode < HighlightModeCount);
    return _selEntities[mode].instances;
}

HdSelection::ElementIndicesMap const&
HdSelection::GetSelectedElements(HdSelection::HighlightMode const& mode) const
{
    TF_VERIFY(mode < HighlightModeCount);
    return _selEntities[mode].elements;
}

HdSelection::EdgeIndicesMap const&
HdSelection::GetSelectedEdges(HdSelection::HighlightMode const& mode) const
{
    TF_VERIFY(mode < HighlightModeCount);
    return _selEntities[mode].edges;
}

HdSelection::PointIndicesMap const&
HdSelection::GetSelectedPoints(HdSelection::HighlightMode const& mode) const
{
    TF_VERIFY(mode < HighlightModeCount);
    return _selEntities[mode].points;
}

PXR_NAMESPACE_CLOSE_SCOPE
