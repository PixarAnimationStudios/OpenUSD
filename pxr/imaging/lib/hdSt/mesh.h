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
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/drawingCoord.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/array.h"

#include <boost/shared_ptr.hpp>

PXR_NAMESPACE_OPEN_SCOPE


class HdStDrawItem;
class HdSceneDelegate;

typedef boost::shared_ptr<class Hd_VertexAdjacency> Hd_VertexAdjacencySharedPtr;
typedef boost::shared_ptr<class HdSt_MeshTopology> HdSt_MeshTopologySharedPtr;
typedef boost::shared_ptr<class HdBufferSource> HdBufferSourceSharedPtr;
typedef boost::shared_ptr<class HdStResourceRegistry>
    HdStResourceRegistrySharedPtr;
typedef boost::shared_ptr<class HdStShaderCode> HdStShaderCodeSharedPtr;

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
    virtual void Sync(HdSceneDelegate   *delegate,
                      HdRenderParam     *renderParam,
                      HdDirtyBits       *dirtyBits,
                      HdReprSelector const &reprToken,
                      bool               forcedRepr) override;

    virtual HdDirtyBits GetInitialDirtyBitsMask() const override;

    /// Topology (member) getter
    HDST_API
    virtual HdMeshTopologySharedPtr GetTopology() const override;

    /// Returns whether packed (10_10_10 bits) normals to be used
    HDST_API
    static bool IsEnabledPackedNormals();

protected:
    virtual void _UpdateRepr(HdSceneDelegate *sceneDelegate,
                             HdReprSelector const &reprToken,
                             HdDirtyBits *dirtyBitsState) override;

    HdBufferArrayRangeSharedPtr
    _GetSharedPrimvarRange(uint64_t primvarId,
                HdBufferSpecVector const &bufferSpecs,
                HdBufferArrayRangeSharedPtr const &existing,
                bool * isFirstInstance,
                HdStResourceRegistrySharedPtr const &resourceRegistry) const;

    bool _UseQuadIndices(const HdRenderIndex &renderIndex,
                         HdSt_MeshTopologySharedPtr const & topology) const;

    bool _UseLimitRefinement(const HdRenderIndex &renderIndex) const;

    bool _UseSmoothNormals(HdSt_MeshTopologySharedPtr const& topology) const;

    bool _UseFlatNormals(const HdMeshReprDesc &desc) const;

    void _UpdateDrawItem(HdSceneDelegate *sceneDelegate,
                         HdStDrawItem *drawItem,
                         HdDirtyBits *dirtyBits,
                         const HdMeshReprDesc &desc,
                         bool requireSmoothNormals,
                         bool requireFlatNormals);

    void _UpdateDrawItemGeometricShader(HdSceneDelegate *sceneDelegate,
                                        HdStDrawItem *drawItem,
                                        const HdMeshReprDesc &desc,
                                        size_t drawItemIdForDesc);

    void _PopulateTopology(HdSceneDelegate *sceneDelegate,
                           HdStDrawItem *drawItem,
                           HdDirtyBits *dirtyBits,
                           const HdMeshReprDesc &desc);

    void _PopulateTopologyVisibility(
            HdStDrawItem *drawItem,
            HdStResourceRegistrySharedPtr const &resourceRegistry,
            HdChangeTracker *changeTracker,
            HdMeshTopology const& meshTopology);

    void _PopulateAdjacency(HdStResourceRegistrySharedPtr const &resourceRegistry);

    void _PopulateVertexPrimvars(HdSceneDelegate *sceneDelegate,
                                 HdStDrawItem *drawItem,
                                 HdDirtyBits *dirtyBits,
                                 bool requireSmoothNormals,
                                 HdBufferSourceSharedPtr *outPoints);

    void _PopulateFaceVaryingPrimvars(HdSceneDelegate *sceneDelegate,
                                      HdStDrawItem *drawItem,
                                      HdDirtyBits *dirtyBits,
                                      const HdMeshReprDesc &desc);

    void _PopulateElementPrimvars(HdSceneDelegate *sceneDelegate,
                                  HdStDrawItem *drawItem,
                                  HdDirtyBits *dirtyBits,
                                  bool requireFlatNormals,
                                  HdBufferSourceSharedPtr const &points);

    int _GetRefineLevelForDesc(const HdMeshReprDesc &desc) const;

    virtual HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

    virtual void _InitRepr(HdReprSelector const &reprToken,
                           HdDirtyBits *dirtyBits) override;

    HdType _GetPointsDataTypeFromBar(HdStDrawItem *drawItem) const;

private:
    enum DrawingCoord {
        HullTopology = HdDrawingCoord::CustomSlotsBegin,
        PointsTopology,
        InstancePrimvar // has to be at the very end
    };

    enum DirtyBits : HdDirtyBits {
        DirtySmoothNormals  = HdChangeTracker::CustomBitsBegin,
        DirtyFlatNormals    = (DirtySmoothNormals << 1),
        DirtyIndices        = (DirtyFlatNormals   << 1),
        DirtyHullIndices    = (DirtyIndices       << 1),
        DirtyPointsIndices  = (DirtyHullIndices   << 1)
    };

    HdSt_MeshTopologySharedPtr _topology;
    Hd_VertexAdjacencySharedPtr _vertexAdjacency;

    HdTopology::ID _topologyId;
    HdTopology::ID _vertexPrimvarId;
    HdDirtyBits _customDirtyBitsInUse;

    HdInterpolation _sceneNormalsInterpolation;
    HdCullStyle _cullStyle;

    bool _doubleSided : 1;
    bool _flatShadingEnabled : 1;
    bool _displacementEnabled : 1;
    bool _smoothNormals : 1;
    bool _packedSmoothNormals : 1;
    bool _limitNormals : 1;
    bool _sceneNormals : 1;
    bool _flatNormals : 1;
    bool _pointsVisibilityAuthored : 1;
    bool _hasVaryingTopology : 1;  // The prim's topology has changed since
                                   // the prim was created
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // HDST_MESH_H
