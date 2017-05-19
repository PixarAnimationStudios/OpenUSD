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

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/enums.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/drawingCoord.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/array.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


class HdSceneDelegate;

typedef boost::shared_ptr<class Hd_VertexAdjacency> Hd_VertexAdjacencySharedPtr;
typedef boost::shared_ptr<class HdSt_MeshTopology> HdSt_MeshTopologySharedPtr;
typedef boost::shared_ptr<class HdBufferSource> HdBufferSourceSharedPtr;

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
    HDST_API
    HdStMesh(SdfPath const& id,
             SdfPath const& instancerId = SdfPath());

    HDST_API
    virtual ~HdStMesh();

    HDST_API
    virtual void Sync(HdSceneDelegate *delegate,
                      HdRenderParam   *renderParam,
                      HdDirtyBits     *dirtyBits,
                      TfToken const   &reprName,
                      bool             forcedRepr) override;

    /// Returns whether packed (10_10_10 bits) normals to be used
    HDST_API
    static bool IsEnabledPackedNormals();

    /// Configure geometric style of drawItems for \p reprName
    /// HdMesh can have up to 2 descriptors for some complex styling
    /// (FeyRay, Outline)
    HDST_API
    static void ConfigureRepr(TfToken const &reprName,
                              HdStMeshReprDesc desc1,
                              HdStMeshReprDesc desc2=HdStMeshReprDesc());

protected:
    virtual HdReprSharedPtr const &
        _GetRepr(HdSceneDelegate *sceneDelegate,
                 TfToken const &reprName,
                 HdDirtyBits *dirtyBitsState) override;

    bool _UsePtexIndices(const HdRenderIndex &renderIndex) const;

    void _UpdateDrawItem(HdSceneDelegate *sceneDelegate,
                         HdDrawItem *drawItem,
                         HdDirtyBits *dirtyBits,
                         HdStMeshReprDesc desc,
                         bool requireSmoothNormals);

    void _UpdateDrawItemGeometricShader(HdRenderIndex &renderIndex,
                                        HdDrawItem *drawItem,
                                        HdStMeshReprDesc desc);

    void _SetGeometricShaders(HdRenderIndex &renderIndex);

    void _ResetGeometricShaders();

    void _PopulateTopology(HdSceneDelegate *sceneDelegate,
                           HdDrawItem *drawItem,
                           HdDirtyBits *dirtyBits,
                           HdStMeshReprDesc desc);

    void _PopulateAdjacency();

    void _PopulateVertexPrimVars(HdSceneDelegate *sceneDelegate,
                                 HdDrawItem *drawItem,
                                 HdDirtyBits *dirtyBits,
                                 bool requireSmoothNormals);

    void _PopulateFaceVaryingPrimVars(HdSceneDelegate *sceneDelegate,
                                      HdDrawItem *drawItem,
                                      HdDirtyBits *dirtyBits,
                                      HdStMeshReprDesc desc);

    void _PopulateElementPrimVars(HdSceneDelegate *sceneDelegate,
                                  HdDrawItem *drawItem,
                                  HdDirtyBits *dirtyBits,
                                  TfTokenVector const &primVarNames);

    int _GetRefineLevelForDesc(HdStMeshReprDesc desc);

    virtual HdDirtyBits _GetInitialDirtyBits() const override;
    HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

    virtual void _InitRepr(TfToken const &reprName,
                           HdDirtyBits *dirtyBits) override;

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
        DirtyPointsIndices  = (DirtyHullIndices   << 1),
        DirtyNewRepr        = (DirtyPointsIndices << 1),
    };

    HdSt_MeshTopologySharedPtr _topology;
    Hd_VertexAdjacencySharedPtr _vertexAdjacency;

    HdTopology::ID _topologyId;
    int _customDirtyBitsInUse;
    bool _doubleSided;
    bool _packedNormals;
    HdCullStyle _cullStyle;

    bool  _pointsValid;
    GLenum _pointsDataType;

    typedef _ReprDescConfigs<HdStMeshReprDesc, /*max drawitems=*/2> _MeshReprConfig;
    static _MeshReprConfig _reprDescConfig;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDST_MESH_H
