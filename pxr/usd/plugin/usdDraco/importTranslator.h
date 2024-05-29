//
// Copyright 2019 Google LLC
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#ifndef PXR_USD_PLUGIN_USD_DRACO_IMPORT_TRANSLATOR_H
#define PXR_USD_PLUGIN_USD_DRACO_IMPORT_TRANSLATOR_H

#include "attributeDescriptor.h"
#include "importAttribute.h"

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/mesh.h"

#include <draco/mesh/corner_table.h>
#include <draco/mesh/mesh.h>


PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdDracoImportTranslator
///
/// Translates Draco mesh to USD mesh.
///
class UsdDracoImportTranslator {
public:
    /// Translates Draco mesh to USD mesh and returns USD layer with mesh.
    static SdfLayerRefPtr Translate(const draco::Mesh &dracoMesh);

private:
    typedef draco::AttributeValueIndex PositionIndex;
    typedef std::map<PositionIndex, draco::PointIndex> PolygonEdges;

    UsdDracoImportTranslator(const draco::Mesh &dracoMesh);
    SdfLayerRefPtr _Translate();
    VtVec3fArray _ComputeExtent() const;
    bool _CheckDescriptors() const;
    bool _CheckData() const;
    void _PopulateValuesFromMesh();
    void _PopulateIndicesFromMesh();
    void _SetIndices(size_t vertexIndex, draco::PointIndex pointIndex);
    void _SetAttributesToMesh(UsdGeomMesh *usdMesh) const;
    bool _HasTrianglesOnly() const;
    bool _SubdivisionRefersToPositions(const UsdGeomMesh &usdMesh) const;
    bool _SubdivisionRefersToFaces(const UsdGeomMesh &usdMesh) const;
    void _FindOriginalFaceEdges(draco::FaceIndex faceIndex,
                                const draco::CornerTable *cornerTable,
                                std::vector<bool> &triangleVisited,
                                PolygonEdges &polygonEdges);
    std::unique_ptr<UsdDracoImportAttributeInterface>
    CreateAttributeFrom(const draco::PointAttribute &attribute,
                        const draco::AttributeMetadata &metadata);

private:
    const draco::Mesh &_dracoMesh;

    // Named attributes.
    UsdDracoImportAttribute<GfVec3f> _positions;
    UsdDracoImportAttribute<GfVec2f> _texCoords;
    UsdDracoImportAttribute<GfVec3f> _normals;
    UsdDracoImportAttribute<uint8_t> _holeFaces;
    UsdDracoImportAttribute<uint8_t> _addedEdges;
    UsdDracoImportAttribute<int> _posOrder;

    // Generic attributes.
    std::vector<std::unique_ptr<UsdDracoImportAttributeInterface>>
        _genericAttributes;

    VtIntArray _faceVertexCounts;
    VtIntArray _faceVertexIndices;
    VtIntArray _holeIndices;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_USD_PLUGIN_USD_DRACO_IMPORT_TRANSLATOR_H
