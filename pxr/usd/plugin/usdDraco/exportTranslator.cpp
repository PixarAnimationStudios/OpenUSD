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

#include "attributeDescriptor.h"
#include "exportAttribute.h"
#include "exportTranslator.h"

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/mesh.h"

#include <draco/mesh/mesh.h>


PXR_NAMESPACE_OPEN_SCOPE


bool UsdDracoExportTranslator::Translate(
        const UsdGeomMesh &usdMesh,
        draco::Mesh *dracoMesh,
        UsdDracoFlag<bool> preservePolygons,
        UsdDracoFlag<bool> preservePositionOrder,
        UsdDracoFlag<bool> preserveHoles) {
    UsdDracoExportTranslator translator(usdMesh, dracoMesh);
    return translator._Translate(
        preservePolygons, preservePositionOrder, preserveHoles);
}

UsdDracoExportTranslator::UsdDracoExportTranslator(
        const UsdGeomMesh &usdMesh,
        draco::Mesh *dracoMesh) :
            _usdMesh(usdMesh),
            _dracoMesh(dracoMesh),
            _positions(attributedescriptor::ForPositions()),
            _texCoords(attributedescriptor::ForTexCoords()),
            _normals(attributedescriptor::ForNormals()),
            _holeFaces(attributedescriptor::ForHoleFaces()),
            _addedEdges(attributedescriptor::ForAddedEdges()),
            _posOrder(attributedescriptor::ForPosOrder()) {
}

bool UsdDracoExportTranslator::_Translate(
        UsdDracoFlag<bool> preservePolygons,
        UsdDracoFlag<bool> preservePositionOrder,
        UsdDracoFlag<bool> preserveHoles) {
    // Get data from USD mesh.
    _GetAttributesFromMesh();
    _GetConnectivityFromMesh();

    // Check validity of attributes and connectivity.
    if (!_CheckData())
        return false;

    // Conditionally enable/disable generic attributes.
    _ConfigureGenericAttributes(
        preservePolygons, preservePositionOrder, preserveHoles);

    // Set data to Draco mesh.
    _SetNumPointsToMesh();
    _SetAttributesToMesh();
    _SetPointMapsToMesh();
    _Deduplicate();
    return true;
}

void UsdDracoExportTranslator::_GetAttributesFromMesh() {
    _positions.GetFromMesh(_usdMesh, 0);
    const size_t np = _positions.GetNumValues();
    _texCoords.GetFromMesh(_usdMesh, np);
    _normals.GetFromMesh(_usdMesh, np);
    _holeFaces.GetFromRange(2);
    _addedEdges.GetFromRange(2);
    _posOrder.GetFromRange(np);
}

void UsdDracoExportTranslator::_GetConnectivityFromMesh() {
  _usdMesh.GetFaceVertexIndicesAttr().Get(&_faceVertexIndices);
  _usdMesh.GetFaceVertexCountsAttr().Get(&_faceVertexCounts);
  _usdMesh.GetHoleIndicesAttr().Get(&_holeIndices);
}

bool UsdDracoExportTranslator::_CheckData() const {
    if (_faceVertexCounts.empty()) {
        TF_RUNTIME_ERROR("Mesh has no face vertex counts.");
        return false;
    }
    if (_faceVertexIndices.empty()) {
        TF_RUNTIME_ERROR("Mesh has no face vertex indices.");
        return false;
    }
    if (_positions.GetNumValues() == 0) {
        TF_RUNTIME_ERROR("Mesh has no points.");
        return false;
    }
    if (!_CheckPrimvarData(_texCoords)) {
        TF_RUNTIME_ERROR("Mesh texture coordinates index is inconsistent.");
        return false;
    }
    if (!_CheckPrimvarData(_normals)) {
        TF_RUNTIME_ERROR("Mesh normal index is inconsistent.");
        return false;
    }
    return true;
}

template <class T>
bool UsdDracoExportTranslator::_CheckPrimvarData(
    const UsdDracoExportAttribute<T> &attribute) const {
    if (attribute.GetNumValues() == 0)
        return true;
    if (attribute.UsesPositionIndex())
        return attribute.GetNumIndices() == _positions.GetNumValues();
    return attribute.GetNumIndices() == _faceVertexIndices.size();
}

void UsdDracoExportTranslator::_ConfigureGenericAttributes(
        UsdDracoFlag<bool> preservePolygons,
        UsdDracoFlag<bool> preservePositionOrder,
        UsdDracoFlag<bool> preserveHoles) {

    // Conditionally clear position order preservation attribute.
    if (preservePositionOrder.HasValue()) {
        if (preservePositionOrder.GetValue() == false) {
            _posOrder.Clear();
        }
    } else {
        if (!_SubdivisionRefersToFaces()) {
            _posOrder.Clear();
        }
    }

    // Conditionally clear hole faces attribute.
    if (preserveHoles.HasValue()) {
        if (preserveHoles.GetValue() == false) {
            _holeFaces.Clear();
        }
    } else {
        if (!_usdMesh.GetHoleIndicesAttr().HasAuthoredValueOpinion()) {
            _holeFaces.Clear();
        }
    }

    // Conditionally clear polygon preservation attribute.
    if (preservePolygons.HasValue()) {
        if (preservePolygons.GetValue() == false) {
            _addedEdges.Clear();
        }
    }
    if (_HasTrianglesOnly())
        _addedEdges.Clear();
}

bool UsdDracoExportTranslator::_HasTrianglesOnly() const {
    for (size_t i = 0; i < _faceVertexCounts.size(); i++)
        if (_faceVertexCounts[i] > 3)
            return false;
    return true;
}

bool UsdDracoExportTranslator::_SubdivisionRefersToPositions() const {
    if (_usdMesh.GetCreaseSharpnessesAttr().HasAuthoredValueOpinion())
        return true;
    if (_usdMesh.GetCornerSharpnessesAttr().HasAuthoredValueOpinion())
        return true;
    return false;
}

bool UsdDracoExportTranslator::_SubdivisionRefersToFaces() const {
    if (_usdMesh.GetHoleIndicesAttr().HasAuthoredValueOpinion())
        return true;
    return false;
}

void UsdDracoExportTranslator::_SetNumPointsToMesh() const {
    size_t numPoints = 0;
    for (size_t i = 0; i < _faceVertexCounts.size(); i++) {
        const size_t numFaceVertices = _faceVertexCounts[i];
        const size_t numTriangles = numFaceVertices - 2;
        numPoints += 3 * numTriangles;
    }
    _dracoMesh->set_num_points(numPoints);
}

void UsdDracoExportTranslator::_SetAttributesToMesh() {
    _positions.SetToMesh(_dracoMesh);
    _texCoords.SetToMesh(_dracoMesh);
    _normals.SetToMesh(_dracoMesh);
    _holeFaces.SetToMesh(_dracoMesh);
    _addedEdges.SetToMesh(_dracoMesh);
    _posOrder.SetToMesh(_dracoMesh);
}

void UsdDracoExportTranslator::_SetPointMapsToMesh() {
    draco::Mesh::Face face;
    size_t firstVertexIdx = 0;
    draco::PointIndex pointIdx(0);
    const size_t numFaces = _faceVertexCounts.size();
    std::vector<bool> isHoleFace(numFaces, false);
    for (int i = 0; i < _holeIndices.size(); i++)
        isHoleFace[_holeIndices[i]] = true;
    for (size_t i = 0; i < numFaces; i++) {
        const size_t numFaceVertices = _faceVertexCounts[i];
        // Split quads and other n-gons into n - 2 triangles.
        const size_t nt = numFaceVertices - 2;
        for (size_t t = 0; t < nt; t++) {
            for (size_t c = 0; c < 3; c++) {
                face[c] = pointIdx;
                const size_t cornerIdx = firstVertexIdx + _Triangulate(t, c);
                const size_t positionIdx = _faceVertexIndices[cornerIdx];
                _positions.SetPointMapEntry(pointIdx, positionIdx);
                _texCoords.SetPointMapEntry(pointIdx, positionIdx, cornerIdx);
                _normals.SetPointMapEntry(pointIdx, positionIdx, cornerIdx);
                // TODO: It would suffice to mark one corner and reduce entropy
                // but reader would have to check all corners.
                _holeFaces.SetPointMapEntry(pointIdx, isHoleFace[i]);
                _addedEdges.SetPointMapEntry(pointIdx, _IsNewEdge(nt, t, c));
                _posOrder.SetPointMapEntry(pointIdx, positionIdx);
                pointIdx++;
            }
            _dracoMesh->AddFace(face);
        }
        firstVertexIdx += numFaceVertices;
    }
}

void UsdDracoExportTranslator::_Deduplicate() const {
    if (!_posOrder.HasPointAttribute())
        _dracoMesh->DeduplicateAttributeValues();
    _dracoMesh->DeduplicatePointIds();
}

// Polygon reconstruction attribute is associated with every triangle corner and
// has values zero or one. Zero indicates that an edge opposite to the corner is
// present in the original mesh (dashed lines), and one indicates that the
// opposite edge has been added during polygon triangulation (dotted lines).
//
// Polygon triangulation is illustrated below. Pentagon ABCDE is split into
// three triangles ABC, ACD, ADE. It is sufficient to set polygon reconstruction
// attribute at corners ABC and ACD. The attribute at the second corner of all
// triangles except for the last is set to one.
//
//          C           D
//          * --------- *
//         /. 1     0  .|
//        / .         . |
//       /  .        .  |
//      / 0 .       . 0 |
//     /    .      .    |
//  B * 1   .     .     |
//     \    .    .      |
//      \ 0 . 0 .       |
//       \  .  .        |
//        \ . .         |
//         \..  0     0 |
//          *-----------*
//          A           E
//
inline size_t UsdDracoExportTranslator::_Triangulate(
        size_t triIndex, size_t triCorner) {
    return triCorner == 0 ? 0 : triIndex + triCorner;
}

inline bool UsdDracoExportTranslator::_IsNewEdge(
        size_t triCount, size_t triIndex, size_t triCorner) {
    // All but the last triangle of the triangulated polygon have an added edge
    // opposite of corner 1.
    return triIndex != triCount - 1 && triCorner == 1;
}


PXR_NAMESPACE_CLOSE_SCOPE
