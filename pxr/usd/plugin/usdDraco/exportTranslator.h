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

#ifndef PXR_USD_PLUGIN_USD_DRACO_EXPORT_TRANSLATOR_H
#define PXR_USD_PLUGIN_USD_DRACO_EXPORT_TRANSLATOR_H

#include "attributeDescriptor.h"
#include "exportAttribute.h"
#include "flag.h"

#include "pxr/pxr.h"
#include "pxr/usd/usdGeom/mesh.h"

#include <draco/mesh/mesh.h>


PXR_NAMESPACE_OPEN_SCOPE


/// \class UsdDracoExportTranslator
///
/// Translates USD mesh to Draco mesh.
///
class UsdDracoExportTranslator {
public:
    /// Translates USD mesh to Draco mesh and returns true on success.
    static bool Translate(const UsdGeomMesh &usdMesh,
                          draco::Mesh *dracoMesh,
                          UsdDracoFlag<bool> preservePolygons,
                          UsdDracoFlag<bool> preservePositionOrder,
                          UsdDracoFlag<bool> preserveHoles);

    /// Creates and returns an export attribute from USD primvar or a nullptr if
    /// primvar cannot be exported to Draco. This method is called by translator
    /// as well as by Python script to check whether a primvar should be kept or
    /// deleted from USD mesh.
    static std::unique_ptr<UsdDracoExportAttributeInterface>
    CreateAttributeFrom(const UsdGeomPrimvar &primvar);

private:
    UsdDracoExportTranslator(const UsdGeomMesh &usdMesh,
                             draco::Mesh *dracoMesh);
    bool _Translate(UsdDracoFlag<bool> preservePolygons,
                    UsdDracoFlag<bool> preservePositionOrder,
                    UsdDracoFlag<bool> preserveHoles);
    bool _CheckDescriptors() const;
    void _GetAttributesFromMesh();
    void _GetConnectivityFromMesh();
    void _CheckUnsupportedPrimvar(const UsdGeomPrimvar &primvar);
    bool _CheckData() const;
    bool _CheckPrimvarData(
        const UsdDracoExportAttributeInterface &attribute) const;
    void _ConfigureHelperAttributes(UsdDracoFlag<bool> preservePolygons,
                                    UsdDracoFlag<bool> preservePositionOrder,
                                    UsdDracoFlag<bool> preserveHoles);
    void _SetNumPointsToMesh() const;
    void _SetAttributesToMesh();
    void _SetPointMapsToMesh();
    void _Deduplicate() const;
    bool _HasTrianglesOnly() const;
    bool _SubdivisionRefersToPositions() const;
    bool _SubdivisionRefersToFaces() const;
    static size_t _Triangulate(size_t triIndex, size_t triCorner);
    static bool _IsNewEdge(size_t triCount, size_t triIndex, size_t triCorner);

private:
    const UsdGeomMesh &_usdMesh;
    draco::Mesh *_dracoMesh;

    // Named attributes.
    UsdDracoExportAttribute<GfVec3f> _positions;
    UsdDracoExportAttribute<GfVec2f> _texCoords;
    UsdDracoExportAttribute<GfVec3f> _normals;

    // Helper attributes that allow Draco to support USD geometries that are
    // not supported out of the box, such as quads and hole indices.
    UsdDracoExportAttribute<uint8_t> _holeFaces;
    UsdDracoExportAttribute<uint8_t> _addedEdges;
    UsdDracoExportAttribute<int> _posOrder;

    // Generic attributes.
    std::vector<std::unique_ptr<UsdDracoExportAttributeInterface>>
        _genericAttributes;

    VtIntArray _faceVertexCounts;
    VtIntArray _faceVertexIndices;
    VtIntArray _holeIndices;

    // Flag that indicates that there are unsupported primvars in the mesh
    // that require position order to be preserved.
    bool _unsupportedPrimvarsReferToPositions;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif  // PXR_USD_PLUGIN_USD_DRACO_EXPORT_TRANSLATOR_H
