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
#include "pxr/pxr.h"
#include "translator.h"
#include "stream.h"

#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/sdf/path.h"
#include "pxr/usd/usd/stage.h"
#include "pxr/usd/usdGeom/mesh.h"
#include "pxr/base/vt/array.h"

#include "pxr/base/gf/range3f.h"

PXR_NAMESPACE_OPEN_SCOPE


SdfLayerRefPtr
UsdObjTranslateObjToUsd(const UsdObjStream &objStream)
{
    // To create an SdfLayer holding Usd data representing \p objStream, we
    // would like to use the Usd and UsdGeom APIs.  To do so, we first create an
    // anonymous in-memory layer, then create a UsdStage with that layer as its
    // root layer.  Then we use the Usd/UsdGeom API to create UsdGeomMeshes on
    // that stage, populating them with the OBJ mesh data.  Finally we return
    // the generated layer to the caller, discarding the UsdStage we created for
    // authoring purposes.

    // Create the layer to populate.
    SdfLayerRefPtr layer = SdfLayer::CreateAnonymous(".usda");

    // Create a UsdStage with that root layer.
    UsdStageRefPtr stage = UsdStage::Open(layer);

    // Now we'll populate the stage with content from the objStream.
    const std::vector<GfVec3f> &objVerts = objStream.GetVerts();
    if (objVerts.empty())
        return layer;

    // Copy objVerts into VtVec3fArray for Usd.
    VtVec3fArray usdPoints;
    usdPoints.assign(objVerts.begin(), objVerts.end());

    // Get the obj Points.
    const std::vector<UsdObjStream::Point> &objPoints = objStream.GetPoints();

    // Usd currently requires an extent, somewhat unfortunately.
    GfRange3f extent;
    for (const auto& pt : usdPoints) {
        extent.UnionWith(pt);
    }
    VtVec3fArray extentArray(2);
    extentArray[0] = extent.GetMin();
    extentArray[1] = extent.GetMax();

    // Make a poly mesh for each group in the obj.
    for (const auto& group : objStream.GetGroups()) {
        if (!TfIsValidIdentifier(group.name)) {
            TF_WARN("Omitting OBJ group with invalid name '%s'",
                    group.name.c_str());
            continue;
        }

        if (group.faces.empty()) {
            TF_WARN("Omitting OBJ group with no faces '%s'",
                    group.name.c_str());
            continue;
        }

        // Create a mesh for the group.
        UsdGeomMesh mesh =
            UsdGeomMesh::Define(stage, SdfPath("/" + group.name));

        // Populate the mesh data from the obj data.  This is not a very smart
        // importer.  We throw all the verts onto everything for simplicity.  If
        // this was for real, you would want to reindex verts per-group.
        mesh.GetPointsAttr().Set(usdPoints);

        VtArray<int> faceVertexCounts, faceVertexIndices;
        for (const auto& face : group.faces) {
            faceVertexCounts.push_back(face.size());
            for (int p = face.pointsBegin; p != face.pointsEnd; ++p) {
                faceVertexIndices.push_back(objPoints[p].vertIndex);
            }
        }

        // Now set the attributes.
        mesh.GetFaceVertexCountsAttr().Set(faceVertexCounts);
        mesh.GetFaceVertexIndicesAttr().Set(faceVertexIndices);

        // Set extent.
        mesh.GetExtentAttr().Set(extentArray);
    }

    return layer;
}




PXR_NAMESPACE_CLOSE_SCOPE

