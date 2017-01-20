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
#ifndef HDST_MESH_H
#define HDST_MESH_H

#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/drawingCoord.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/array.h"

#include <boost/shared_ptr.hpp>

class HdSceneDelegate;

typedef boost::shared_ptr<class Hd_VertexAdjacency> Hd_VertexAdjacencySharedPtr;
typedef boost::shared_ptr<class HdSt_MeshTopology> HdSt_MeshTopologySharedPtr;

/// \class HdStMeshReprDesc
///
/// descriptor to configure a drawItem for a repr
///
struct HdStMeshReprDesc {
    HdStMeshReprDesc(HdMeshGeomStyle geomStyle = HdMeshGeomStyleInvalid,
                   HdCullStyle cullStyle = HdCullStyleDontCare,
                   bool lit = false,
                   bool smoothNormals = false,
                   bool blendWireframeColor = true)
        : geomStyle(geomStyle)
        , cullStyle(cullStyle)
        , lit(lit)
        , smoothNormals(smoothNormals)
        , blendWireframeColor(blendWireframeColor)
        {}

    HdMeshGeomStyle geomStyle:3;
    HdCullStyle     cullStyle:3;
    bool            lit:1;
    bool            smoothNormals:1;
    bool            blendWireframeColor:1;
};

/// A subdivision surface or poly-mesh object.
///
class HdStMesh final : public HdMesh {
public:
    HF_MALLOC_TAG_NEW("new HdStMesh");

    /// Constructor. instancerId, if specified, is the instancer which uses
    /// this mesh as a prototype.
    HdStMesh(HdSceneDelegate* delegate, SdfPath const& id,
             SdfPath const& instancerId = SdfPath());

    virtual ~HdStMesh();

    /// Returns whether computation of smooth normals is enabled on GPU.
    static bool IsEnabledSmoothNormalsGPU();

    /// Returns whether quadrangulation is enabled.
    /// This is used temporarily for testing.
    static bool IsEnabledQuadrangulationCPU();

    /// Returns whether quadrangulation is enabled.
    /// This is used temporarily for testing.
    static bool IsEnabledQuadrangulationGPU();

    static bool IsEnabledQuadrangulation() {
        return IsEnabledQuadrangulationCPU() || IsEnabledQuadrangulationGPU();
    }

    /// Returns whether GPU refinement is enabled or not.
    static bool IsEnabledRefineGPU();

    /// Returns whether packed (10_10_10 bits) normals to be used
    static bool IsEnabledPackedNormals();

    /// Configure geometric style of drawItems for \p reprName
    /// HdMesh can have up to 2 descriptors for some complex styling
    /// (FeyRay, Outline)
    static void ConfigureRepr(TfToken const &reprName,
                              HdStMeshReprDesc desc1,
                              HdStMeshReprDesc desc2=HdStMeshReprDesc());

protected:
    virtual HdReprSharedPtr const & _GetRepr(
        TfToken const &reprName, HdChangeTracker::DirtyBits *dirtyBitsState);

    HdChangeTracker::DirtyBits _PropagateDirtyBits(
        HdChangeTracker::DirtyBits dirtyBits);

    bool _UsePtexIndices() const;

    void _UpdateDrawItem(HdDrawItem *drawItem,
                         HdChangeTracker::DirtyBits *dirtyBits,
                         bool isNew,
                         HdStMeshReprDesc desc,
                         bool requireSmoothNormals);

    void _UpdateDrawItemGeometricShader(HdDrawItem *drawItem,
                                        HdStMeshReprDesc desc);

    void _SetGeometricShaders();

    void _ResetGeometricShaders();

    void _PopulateTopology(HdDrawItem *drawItem,
                           HdChangeTracker::DirtyBits *dirtyBits,
                           HdStMeshReprDesc desc);

    void _PopulateAdjacency();

    void _PopulateVertexPrimVars(HdDrawItem *drawItem,
                                 HdChangeTracker::DirtyBits *dirtyBits,
                                 bool isNew,
                                 HdStMeshReprDesc desc,
                                 bool requireSmoothNormals);

    void _PopulateFaceVaryingPrimVars(HdDrawItem *drawItem,
                                      HdChangeTracker::DirtyBits *dirtyBits,
                                      HdStMeshReprDesc desc);

    void _PopulateElementPrimVars(HdDrawItem *drawItem,
                                  HdChangeTracker::DirtyBits *dirtyBits,
                                  TfTokenVector const &primVarNames);

    int _GetRefineLevelForDesc(HdStMeshReprDesc desc);

    virtual HdChangeTracker::DirtyBits _GetInitialDirtyBits() const final override;

private:
    enum DrawingCoord {
        HullTopology = HdDrawingCoord::CustomSlotsBegin,
        PointsTopology,
        InstancePrimVar // has to be at the very end
    };

    enum DirtyBits {
        DirtySmoothNormals  = HdChangeTracker::CustomBitsBegin,
        DirtyIndices        = (DirtySmoothNormals << 1),
        DirtyHullIndices    = (DirtyIndices       << 1),
        DirtyPointsIndices  = (DirtyHullIndices   << 1)
    };

    HdSt_MeshTopologySharedPtr _topology;
    Hd_VertexAdjacencySharedPtr _vertexAdjacency;

    HdTopology::ID _topologyId;
    int _customDirtyBitsInUse;
    bool _doubleSided;
    bool _packedNormals;
    HdCullStyle _cullStyle;

    typedef _ReprDescConfigs<HdStMeshReprDesc, /*max drawitems=*/2> _MeshReprConfig;
    static _MeshReprConfig _reprDescConfig;
};

#endif // HDST_MESH_H
