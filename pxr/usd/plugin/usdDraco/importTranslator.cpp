//
// Copyright 2019 Google LLC
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


#include "importAttribute.h"
#include "importTranslator.h"

#include "pxr/pxr.h"
#include "pxr/base/vt/array.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/base/gf/range3f.h"

#include <draco/mesh/mesh_misc_functions.h>


PXR_NAMESPACE_OPEN_SCOPE


UsdDracoImportTranslator::UsdDracoImportTranslator(
        const draco::Mesh &dracoMesh) :
    _dracoMesh(dracoMesh),
    _positions(attributedescriptor::ForPositions(), dracoMesh),
    _texCoords(attributedescriptor::ForTexCoords(), dracoMesh),
    _normals(attributedescriptor::ForNormals(), dracoMesh),
    _holeFaces(attributedescriptor::ForHoleFaces(), dracoMesh),
    _addedEdges(attributedescriptor::ForAddedEdges(), dracoMesh),
    _posOrder(attributedescriptor::ForPosOrder(), dracoMesh) {
}

SdfLayerRefPtr UsdDracoImportTranslator::Translate(
        const draco::Mesh &dracoMesh) {
    UsdDracoImportTranslator translator(dracoMesh);
    return translator._Translate();
}

void UsdDracoImportTranslator::_FindOriginalFaceEdges(
        draco::FaceIndex faceIndex,
        const draco::CornerTable *cornerTable,
        std::vector<bool> &triangleVisited,
        PolygonEdges &polygonEdges) {
    // Do not add any edges if this triangular face has already been visited.
    if (triangleVisited[faceIndex.value()])
      return;
    triangleVisited[faceIndex.value()] = true;
    const draco::Mesh::Face &face = _dracoMesh.face(faceIndex);
    for (size_t c = 0; c < 3; c++) {
        // Check for added edge using this corner.
        const draco::PointIndex pi = face[c];
        bool isNewEdge = _addedEdges.GetMappedValue(pi);
        const draco::CornerIndex ci = cornerTable->FirstCorner(faceIndex) + c;
        const draco::CornerIndex co = cornerTable->Opposite(ci);

        // Check for the new edge using the opposite corner.
        if (!isNewEdge && co != draco::kInvalidCornerIndex) {
            const draco::PointIndex pi = _dracoMesh.CornerToPointId(co);
            isNewEdge = _addedEdges.GetMappedValue(pi);
        }
        if (isNewEdge) {
            // Visit triangle across the new edge.
            const draco::FaceIndex oppositeFaceIndex = cornerTable->Face(co);
            _FindOriginalFaceEdges(oppositeFaceIndex, cornerTable,
                                   triangleVisited, polygonEdges);
        } else {
            // Insert the original edge to the map.
            const draco::PointIndex pointFrom = face[(c + 1) % 3];
            draco::PointIndex pointTo = face[(c + 2) % 3];
            polygonEdges.insert(
                {PositionIndex(_positions.GetMappedIndex(pointFrom)),
                 pointTo});
        }
    }
}

SdfLayerRefPtr UsdDracoImportTranslator::_Translate()
{
    // Create USD layer and stage.
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");
    UsdStageRefPtr stage = UsdStage::Open(layer);

    // Get data from Draco mesh.
    _PopulateValuesFromMesh();

    // Check validity of attributes.
    if (!_CheckData())
        return layer;

    // Populate indices.
    _PopulateIndicesFromMesh();

    // Create USD mesh and set attributes.
    UsdGeomMesh usdMesh = UsdGeomMesh::Define(stage, SdfPath("/DracoMesh"));
    _SetAttributesToMesh(&usdMesh);

    // Set the mesh as the default prim of the stage.
    stage->SetDefaultPrim(usdMesh.GetPrim());
    return layer;
}

void UsdDracoImportTranslator::_PopulateValuesFromMesh() {
    if (_posOrder.HasPointAttribute()) {
        const size_t numFaces = _dracoMesh.num_faces();
        _positions.PopulateValuesWithOrder(_posOrder, numFaces, _dracoMesh);
    } else {
        _positions.PopulateValues();
    }
    _texCoords.PopulateValues();
    _normals.PopulateValues();
}

void UsdDracoImportTranslator::_PopulateIndicesFromMesh() {
    // Allocate index arrays as if all faces were triangles. The arrays will be
    // downsized if the mesh turns out to have quads and other polygons.
    const size_t numFaces = _dracoMesh.num_faces();
    const size_t numCorners = 3 * numFaces;
    _faceVertexCounts.resize(numFaces);
    _faceVertexIndices.resize(numCorners);
    _texCoords.ResizeIndices(numCorners);
    _normals.ResizeIndices(numCorners);

    // Create corner table.
    std::unique_ptr<draco::CornerTable> cornerTable =
        draco::CreateCornerTableFromPositionAttribute(&_dracoMesh);

    // Reconstruct polygons here.
    size_t vertexIndex = 0;
    size_t faceIndex = 0;
    std::vector<bool> triangleVisited(numFaces, false);

    // Populate index arrays.
    PolygonEdges polygonEdges;
    for (size_t i = 0; i < numFaces; i++) {
        const draco::Mesh::Face &face = _dracoMesh.face(draco::FaceIndex(i));
        if (_addedEdges.HasPointAttribute()) {
            draco::FaceIndex fi(i);
            polygonEdges.clear();
            _FindOriginalFaceEdges(
                fi, cornerTable.get(), triangleVisited, polygonEdges);

            // Polygon edges could be empty if this triangle has been visited
            // as part of a polygon discovery that started from an earler face.
            if (polygonEdges.empty()) {
                continue;
            }
            _faceVertexCounts[faceIndex] = polygonEdges.size();

            // Traverse a polygon by following its edges. The starting point is
            // not guaranteed to be the same as in the original polygon.
            // It is deterministic, however, and defined by std::map behavior.
            const draco::AttributeValueIndex firstPositionIndex =
                polygonEdges.begin()->first;
            draco::AttributeValueIndex positionIndex = firstPositionIndex;
            do {
                // Get the next polygon point index by following polygon edge.
                const draco::PointIndex pi = polygonEdges[positionIndex];
                _SetIndices(vertexIndex++, pi);
                positionIndex = _positions.GetMappedIndex(pi);
            } while (positionIndex != firstPositionIndex);
        } else {
            _faceVertexCounts[i] = 3;
            for (size_t c = 0; c < 3; c++) {
                _SetIndices(vertexIndex++, face[c]);
            }
        }
        if (_holeFaces.HasPointAttribute())
            if (_holeFaces.GetMappedValue(face[0]))
                _holeIndices.push_back(faceIndex);
        faceIndex++;
    }

    // Downsize index arrays if there are quads and higher polygons.
    size_t numPoints = 0;
    for (size_t i = 0; i < _faceVertexCounts.size(); i++) {
        const int faceVertexCount = _faceVertexCounts[i];
        if (faceVertexCount == 0) {
            _faceVertexCounts.resize(i);
            break;
        }
        numPoints += faceVertexCount;
    }
    _faceVertexIndices.resize(numPoints);
    _texCoords.ResizeIndices(numPoints);
    _normals.ResizeIndices(numPoints);
}

inline void UsdDracoImportTranslator::_SetIndices(
        size_t vertexIndex, draco::PointIndex pointIndex) {
    _faceVertexIndices[vertexIndex] =
    _posOrder.HasPointAttribute()
        ? _posOrder.GetMappedValue(pointIndex)
        : _positions.GetMappedIndex(pointIndex);
    if (_texCoords.HasPointAttribute())
        _texCoords.SetIndex(vertexIndex, _texCoords.GetMappedIndex(pointIndex));
    if (_normals.HasPointAttribute())
        _normals.SetIndex(vertexIndex, _normals.GetMappedIndex(pointIndex));
}

void UsdDracoImportTranslator::_SetAttributesToMesh(UsdGeomMesh *usdMesh) const {
    _positions.SetToMesh(usdMesh);
    _texCoords.SetToMesh(usdMesh);
    _normals.SetToMesh(usdMesh);
    usdMesh->GetExtentAttr().Set(_ComputeExtent());
    usdMesh->GetFaceVertexCountsAttr().Set(_faceVertexCounts);
    usdMesh->GetFaceVertexIndicesAttr().Set(_faceVertexIndices);
    if (_holeFaces.HasPointAttribute())
        usdMesh->GetHoleIndicesAttr().Set(_holeIndices);
}

VtVec3fArray UsdDracoImportTranslator::_ComputeExtent() const {
    GfRange3f range;
    for (const GfVec3f &position : _positions.GetValues())
        range.UnionWith(position);
    VtVec3fArray extent(2);
    extent[0] = range.GetMin();
    extent[1] = range.GetMax();
    return extent;
}

bool UsdDracoImportTranslator::_CheckData() const {
    if (!_positions.HasPointAttribute()) {
        TF_RUNTIME_ERROR("Mesh has no points.");
        return false;
    }
    return true;
}

bool UsdDracoImportTranslator::_HasTrianglesOnly() const {
    for (size_t i = 0; i < _faceVertexCounts.size(); i++)
        if (_faceVertexCounts[i] > 3)
            return false;
    return true;
}

bool UsdDracoImportTranslator::_SubdivisionRefersToPositions(
        const UsdGeomMesh &usdMesh) const {
    if (usdMesh.GetCreaseSharpnessesAttr().HasAuthoredValueOpinion())
        return true;
    if (usdMesh.GetCornerSharpnessesAttr().HasAuthoredValueOpinion())
        return true;
    return false;
}

bool UsdDracoImportTranslator::_SubdivisionRefersToFaces(
        const UsdGeomMesh &usdMesh) const {
    if (usdMesh.GetHoleIndicesAttr().HasAuthoredValueOpinion())
        return true;
    return false;
}


PXR_NAMESPACE_CLOSE_SCOPE
