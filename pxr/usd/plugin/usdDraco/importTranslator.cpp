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

#include "importTranslator.h"

#include "attributeFactory.h"
#include "importAttribute.h"

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
    _positions(UsdDracoAttributeDescriptor::ForPositions(dracoMesh), dracoMesh),
    _texCoords(UsdDracoAttributeDescriptor::ForTexCoords(dracoMesh), dracoMesh),
    _normals(UsdDracoAttributeDescriptor::ForNormals(dracoMesh), dracoMesh),
    _holeFaces(UsdDracoAttributeDescriptor::ForHoleFaces(), dracoMesh),
    _addedEdges(UsdDracoAttributeDescriptor::ForAddedEdges(), dracoMesh),
    _posOrder(UsdDracoAttributeDescriptor::ForPosOrder(), dracoMesh) {
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

    // Check validity of descriptors obtained from Draco mesh in constructor.
    if (!_CheckDescriptors())
        return layer;

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

bool UsdDracoImportTranslator::_CheckDescriptors() const {
    // Valid positions must be present in the mesh.
    if (_positions.GetDescriptor().GetStatus() !=
        UsdDracoAttributeDescriptor::VALID) {
        TF_RUNTIME_ERROR("Draco mesh has no valid positions.");
        return false;
    }

    // Texture coordinates are optional and may be absent from USD mesh.
    if (_texCoords.GetDescriptor().GetStatus() ==
        UsdDracoAttributeDescriptor::INVALID) {
        TF_RUNTIME_ERROR("Draco mesh has invalid texture coordinates.");
        return false;
    }

    // Normals are optional and may be absent from USD mesh.
    if (_normals.GetDescriptor().GetStatus() ==
        UsdDracoAttributeDescriptor::INVALID) {
        TF_RUNTIME_ERROR("Draco mesh has invalid normals.");
        return false;
    }
    return true;
}

void UsdDracoImportTranslator::_PopulateValuesFromMesh() {
    // Get named attribute data from mesh.
    if (_posOrder.HasPointAttribute()) {
        const size_t numFaces = _dracoMesh.num_faces();
        _positions.PopulateValuesWithOrder(_posOrder, numFaces, _dracoMesh);
    } else {
        _positions.PopulateValues();
    }
    _texCoords.PopulateValues();
    _normals.PopulateValues();

    // Get generic attributes from Draco mesh.
    const std::vector<std::unique_ptr<draco::AttributeMetadata>> &metadatas =
        _dracoMesh.GetMetadata()->attribute_metadatas();
    for (size_t i = 0; i < metadatas.size(); i++) {
        const draco::AttributeMetadata &metadata = *metadatas[i];
        const draco::PointAttribute &attribute =
            *_dracoMesh.attribute(metadata.att_unique_id());
        auto importAttribute = CreateAttributeFrom(attribute, metadata);
        if (!importAttribute) {
            TF_RUNTIME_ERROR("Draco mesh has invalid attribute.");
            return;
        }

        // Collect only generic attributes.
        if (importAttribute->GetDescriptor().IsGeneric())
            _genericAttributes.push_back(std::move(importAttribute));
    }

    // Get generic attribute data from mesh.
    for (size_t i = 0; i < _genericAttributes.size(); i++) {
        _genericAttributes[i]->PopulateValues();
    }
}

class ImportAttributeCreator {
    public:
       ImportAttributeCreator(const draco::Mesh &mesh) : _dracoMesh(mesh) {}
       template <class ValueT>
       std::unique_ptr<UsdDracoImportAttributeInterface>
       CreateAttribute(const UsdDracoAttributeDescriptor &descriptor) const {
           return std::unique_ptr<UsdDracoImportAttributeInterface>(
               new UsdDracoImportAttribute<ValueT>(descriptor, _dracoMesh));
       }
    private:
      const draco::Mesh &_dracoMesh;
};

std::unique_ptr<UsdDracoImportAttributeInterface>
UsdDracoImportTranslator::CreateAttributeFrom(
    const draco::PointAttribute &attribute,
    const draco::AttributeMetadata &metadata) {
    // Get attribute descriptor from Draco attribute and metadata.
    const bool isPrimvar = true;
    const UsdDracoAttributeDescriptor descriptor =
        UsdDracoAttributeDescriptor::FromDracoAttribute(
            attribute, metadata, isPrimvar);

    // Check if attribute is valid.
    if (descriptor.GetStatus() != UsdDracoAttributeDescriptor::VALID)
        return nullptr;

    // Create import attribute from attribute descriptor.
    const ImportAttributeCreator creator(_dracoMesh);
    return UsdDracoAttributeFactory::CreateAttribute<
        UsdDracoImportAttributeInterface, ImportAttributeCreator>(
            descriptor, creator);
}

void UsdDracoImportTranslator::_PopulateIndicesFromMesh() {
    // Allocate index arrays as if all faces were triangles. The arrays will be
    // downsized if it turns out that the mesh has quads and other polygons.
    const size_t numFaces = _dracoMesh.num_faces();
    const size_t numCorners = 3 * numFaces;
    _faceVertexCounts.resize(numFaces);
    _faceVertexIndices.resize(numCorners);
    _texCoords.ResizeIndices(numCorners);
    _normals.ResizeIndices(numCorners);
    for (size_t i = 0; i < _genericAttributes.size(); i++) {
        _genericAttributes[i]->ResizeIndices(numCorners);
    }

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
        if (_holeFaces.GetMappedValue(face[0]) != 0)
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
    for (size_t i = 0; i < _genericAttributes.size(); i++) {
        _genericAttributes[i]->ResizeIndices(numPoints);
    }
}

inline void UsdDracoImportTranslator::_SetIndices(
        size_t vertexIndex, draco::PointIndex pointIndex) {
    _faceVertexIndices[vertexIndex] =
    _posOrder.HasPointAttribute()
        ? _posOrder.GetMappedValue(pointIndex)
        : _positions.GetMappedIndex(pointIndex);
    _texCoords.SetIndex(vertexIndex, _texCoords.GetMappedIndex(pointIndex));
    _normals.SetIndex(vertexIndex, _normals.GetMappedIndex(pointIndex));
    for (size_t i = 0; i < _genericAttributes.size(); i++) {
        _genericAttributes[i]->SetIndex(
            vertexIndex, _genericAttributes[i]->GetMappedIndex(pointIndex));
    }
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
    for (size_t i = 0; i < _genericAttributes.size(); i++) {
        _genericAttributes[i]->SetToMesh(usdMesh);
    }
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
        TF_RUNTIME_ERROR("Draco mesh has no points.");
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
