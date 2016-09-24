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
#ifndef HD_MESH_H
#define HD_MESH_H

#include "pxr/imaging/hd/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/drawingCoord.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/rprim.h"
#include "pxr/imaging/hd/topology.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/array.h"

#include <boost/shared_ptr.hpp>

class HdSceneDelegate;

typedef boost::shared_ptr<class HdMeshTopology> HdMeshTopologySharedPtr;
typedef boost::shared_ptr<class Hd_VertexAdjacency> Hd_VertexAdjacencySharedPtr;

/// \class HdMeshReprDesc
///
/// descriptor to configure a drawItem for a repr
///
struct HdMeshReprDesc {
    HdMeshReprDesc(HdMeshGeomStyle geomStyle = HdMeshGeomStyleInvalid,
                   HdCullStyle cullStyle = HdCullStyleDontCare,
                   bool lit = false,
                   bool smoothNormals = false)
        : geomStyle(geomStyle)
        , cullStyle(cullStyle)
        , lit(lit)
        , smoothNormals(smoothNormals)
        {}

    HdMeshGeomStyle geomStyle:3;
    HdCullStyle     cullStyle:3;
    bool            lit:1;
    bool            smoothNormals:1;
};

/// A subdivision surface or poly-mesh object.
///
class HdMesh : public HdRprim {
public:

    HD_MALLOC_TAG_NEW("new HdMesh");

    /// Constructor. instancerId, if specified, is the instancer which uses
    /// this mesh as a prototype.
	HDLIB_API
    HdMesh(HdSceneDelegate* delegate, SdfPath const& id,
           SdfPath const& surfaceShaderId,
           SdfPath const& instancerId = SdfPath());

    /// Returns whether computation of smooth normals is enabled on GPU.
    static bool IsEnabledSmoothNormalsGPU();

    /// Returns whether quadrangulation is enabled.
    /// This is used temporarily for testing.
    static bool IsEnabledQuadrangulationCPU();

    /// Returns whether quadrangulation is enabled.
    /// This is used temporarily for testing.
    static bool IsEnabledQuadrangulationGPU();

    static bool IsEnabledQuadrangulation() {
        return IsEnabledQuadrangulationCPU() or IsEnabledQuadrangulationGPU();
    }

    /// Returns whether GPU refinement is enabled or not.
    static bool IsEnabledRefineGPU();

    /// Returns whether packed (10_10_10 bits) normals to be used
    static bool IsEnabledPackedNormals();

    /// Configure geometric style of drawItems for \p reprName
    /// HdMesh can have up to 2 descriptors for some complex styling
    /// (FeyRay, Outline)
    static void ConfigureRepr(TfToken const &reprName,
                              HdMeshReprDesc desc1,
                              HdMeshReprDesc desc2=HdMeshReprDesc());

    /// Return the dirtyBits mask to be tracked for \p reprName
    static int GetDirtyBitsMask(TfToken const &reprName);

protected:
    virtual HdReprSharedPtr const & _GetRepr(
        TfToken const &reprName, HdChangeTracker::DirtyBits *dirtyBitsState);

    HdChangeTracker::DirtyBits _PropagateDirtyBits(
        HdChangeTracker::DirtyBits dirtyBits);

    bool _UsePtexIndices() const;

    void _UpdateDrawItem(HdDrawItem *drawItem,
                         HdChangeTracker::DirtyBits *dirtyBits,
                         bool isNew,
                         HdMeshReprDesc desc);

    void _UpdateDrawItemGeometricShader(HdDrawItem *drawItem,
                                        HdMeshReprDesc desc);

    void _SetGeometricShaders();

    void _ResetGeometricShaders();

    void _PopulateTopology(HdDrawItem *drawItem,
                           HdChangeTracker::DirtyBits *dirtyBits,
                           HdMeshReprDesc desc);

    void _PopulateAdjacency();

    void _PopulateVertexPrimVars(HdDrawItem *drawItem,
                                 HdChangeTracker::DirtyBits *dirtyBits,
                                 bool isNew,
                                 HdMeshReprDesc desc);

    void _PopulateFaceVaryingPrimVars(HdDrawItem *drawItem,
                                      HdChangeTracker::DirtyBits *dirtyBits,
                                      HdMeshReprDesc desc);

    void _PopulateElementPrimVars(HdDrawItem *drawItem,
                                  HdChangeTracker::DirtyBits *dirtyBits,
                                  TfTokenVector const &primVarNames);

    int _GetRefineLevelForDesc(HdMeshReprDesc desc);

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

    HdMeshTopologySharedPtr _topology;
    Hd_VertexAdjacencySharedPtr _vertexAdjacency;

    HdTopology::ID _topologyId;
    int _customDirtyBitsInUse;
    bool _doubleSided;
    bool _packedNormals;
    HdCullStyle _cullStyle;

    typedef _ReprDescConfigs<HdMeshReprDesc, /*max drawitems=*/2> _MeshReprConfig;
    static _MeshReprConfig _reprDescConfig;
};

#endif //HD_MESH_H
