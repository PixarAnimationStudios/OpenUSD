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
#ifndef PXR_IMAGING_HD_ST_MESH_H
#define PXR_IMAGING_HD_ST_MESH_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/drawingCoord.h"
#include "pxr/imaging/hd/mesh.h"
#include "pxr/imaging/hd/perfLog.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/vt/array.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


class HdStDrawItem;
class HdSceneDelegate;

using Hd_VertexAdjacencySharedPtr = std::shared_ptr<class Hd_VertexAdjacency>;
using HdBufferSourceSharedPtr = std::shared_ptr<class HdBufferSource>;
using HdSt_MeshTopologySharedPtr = std::shared_ptr<class HdSt_MeshTopology>;

using HdStResourceRegistrySharedPtr =
    std::shared_ptr<class HdStResourceRegistry>;

/// A subdivision surface or poly-mesh object.
///
class HdStMesh final : public HdMesh
{
public:
    HF_MALLOC_TAG_NEW("new HdStMesh");

    HDST_API
    HdStMesh(SdfPath const& id);

    HDST_API
    ~HdStMesh() override;

    HDST_API
    void Sync(HdSceneDelegate *delegate,
              HdRenderParam   *renderParam,
              HdDirtyBits     *dirtyBits,
              TfToken const   &reprToken) override;

    HDST_API
    void Finalize(HdRenderParam   *renderParam) override;

    HDST_API
    HdDirtyBits GetInitialDirtyBitsMask() const override;

    /// Topology (member) getter
    HDST_API
    HdMeshTopologySharedPtr GetTopology() const override;

    /// Returns whether packed (10_10_10 bits) normals to be used
    HDST_API
    static bool IsEnabledPackedNormals();

protected:
    HDST_API
    void _InitRepr(TfToken const &reprToken, HdDirtyBits *dirtyBits) override;

    HDST_API
    HdDirtyBits _PropagateDirtyBits(HdDirtyBits bits) const override;

    void _UpdateRepr(HdSceneDelegate *sceneDelegate,
                     HdRenderParam *renderParam,
                     TfToken const &reprToken,
                     HdDirtyBits *dirtyBitsState);

    HdBufferArrayRangeSharedPtr
    _GetSharedPrimvarRange(uint64_t primvarId,
                HdBufferSpecVector const &updatedOrAddedSpecs,
                HdBufferSpecVector const &removedSpecs,
                HdBufferArrayRangeSharedPtr const &curRange,
                bool * isFirstInstance,
                HdStResourceRegistrySharedPtr const &resourceRegistry) const;

    bool _UseQuadIndices(const HdRenderIndex &renderIndex,
                         HdSt_MeshTopologySharedPtr const & topology) const;

    bool _UseLimitRefinement(const HdRenderIndex &renderIndex) const;

    bool _UseSmoothNormals(HdSt_MeshTopologySharedPtr const& topology) const;

    bool _UseFlatNormals(const HdMeshReprDesc &desc) const;

    void _UpdateDrawItem(HdSceneDelegate *sceneDelegate,
                         HdRenderParam *renderParam,
                         HdStDrawItem *drawItem,
                         HdDirtyBits *dirtyBits,
                         const HdMeshReprDesc &desc,
                         bool requireSmoothNormals,
                         bool requireFlatNormals);

    void _UpdateDrawItemGeometricShader(HdSceneDelegate *sceneDelegate,
                                        HdRenderParam *renderParam,
                                        HdStDrawItem *drawItem,
                                        const HdMeshReprDesc &desc);

    void _UpdateShadersForAllReprs(HdSceneDelegate *sceneDelegate,
                                   HdRenderParam *renderParam,
                                   bool updateMaterialShader,
                                   bool updateGeometricShader);

    void _PopulateTopology(HdSceneDelegate *sceneDelegate,
                           HdRenderParam *renderParam,
                           HdStDrawItem *drawItem,
                           HdDirtyBits *dirtyBits,
                           const HdMeshReprDesc &desc);

    void _PopulateAdjacency(
        HdStResourceRegistrySharedPtr const &resourceRegistry);

    void _GatherFaceVaryingTopologies(HdSceneDelegate *sceneDelegate,
                                      HdStDrawItem *drawItem,
                                      HdDirtyBits *dirtyBits,
                                      const SdfPath &id,
                                      HdSt_MeshTopologySharedPtr topology);

    void _PopulateVertexPrimvars(HdSceneDelegate *sceneDelegate,
                                 HdRenderParam *renderParam,
                                 HdStDrawItem *drawItem,
                                 HdDirtyBits *dirtyBits,
                                 bool requireSmoothNormals);

    void _PopulateFaceVaryingPrimvars(HdSceneDelegate *sceneDelegate,
                                      HdRenderParam *renderParam,
                                      HdStDrawItem *drawItem,
                                      HdDirtyBits *dirtyBits,
                                      const HdMeshReprDesc &desc);

    void _PopulateElementPrimvars(HdSceneDelegate *sceneDelegate,
                                  HdRenderParam *renderParam,
                                  HdStDrawItem *drawItem,
                                  HdDirtyBits *dirtyBits,
                                  bool requireFlatNormals);

    int _GetRefineLevelForDesc(const HdMeshReprDesc &desc) const;

    // Helper class for meshes to keep track of the topologies of their
    // face-varying primvars. The face-varying topologies are later passed to 
    // the OSD refiner in an order that will correspond to their face-varying 
    // channel number. We keep a vector of only the topologies in use, paired
    // with their associated primvar names.
    class _FvarTopologyTracker 
    {
    public:
        const TopologyToPrimvarVector & GetTopologyToPrimvarVector() const {
            return _topologies;
        } 

        // Add a primvar and its corresponding toplogy to the tracker
        void AddOrUpdateTopology(const TfToken &primvar, 
                                 const VtIntArray &topology) {
            for (size_t i = 0; i < _topologies.size(); ++i) {
                // Found existing topology
                if (_topologies[i].first == topology) {

                    if (std::find(_topologies[i].second.begin(),
                        _topologies[i].second.end(),
                        primvar) == _topologies[i].second.end()) {
                        // Topology does not have that primvar assigned
                        RemovePrimvar(primvar);
                        _topologies[i].second.push_back(primvar);
                    } 
                    return;
                } 
            }

            // Found new topology
            RemovePrimvar(primvar);
            _topologies.push_back(
                std::pair<VtIntArray, std::vector<TfToken>>(
                    topology, {primvar}));
        }

        // Remove a primvar from the tracker.
        void RemovePrimvar(const TfToken &primvar) {
            for (size_t i = 0; i < _topologies.size(); ++i) {
                _topologies[i].second.erase(std::find(
                    _topologies[i].second.begin(),
                    _topologies[i].second.end(),
                    primvar), _topologies[i].second.end());

                }
        }

        // Remove unused topologies (topologies with no associated primvars), as
        // we do not want to build stencil tables for them.
        void RemoveUnusedTopologies() {
            _topologies.erase(std::remove_if(
                _topologies.begin(), _topologies.end(), NoPrimvars), 
                _topologies.end());
        }

        // Get the face-varying channel given a primvar name. If the primvar is 
        // not in the tracker, returns -1.
        int GetChannelFromPrimvar(const TfToken &primvar) const {
            for (size_t i = 0; i < _topologies.size(); ++i) {
                if (std::find(_topologies[i].second.begin(),
                              _topologies[i].second.end(),
                               primvar) != 
                    _topologies[i].second.end()) {
                    return i;
                }
            }
            return -1;
        }

        // Return a vector of all the face-varying topologies.
        std::vector<VtIntArray> GetFvarTopologies() const {
            std::vector<VtIntArray> fvarTopologies;
            for (const auto& it : _topologies) {
                fvarTopologies.push_back(it.first);
            }
            return fvarTopologies;
        }

        size_t GetNumTopologies() const {
            return _topologies.size();
        }

    private:
        // Helper function that returns true if a <topology, primvar vector> has
        // no primvars.
        static bool NoPrimvars(const std::pair<VtIntArray, std::vector<TfToken>>
                               &topology) {
            return topology.second.empty();
        }

        TopologyToPrimvarVector _topologies;
    };

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

    HdType _pointsDataType;
    HdInterpolation _sceneNormalsInterpolation;
    HdCullStyle _cullStyle;
    bool _hasMirroredTransform : 1;
    bool _doubleSided : 1;
    bool _flatShadingEnabled : 1;
    bool _displacementEnabled : 1;
    bool _limitNormals : 1;
    bool _sceneNormals : 1;
    bool _hasVaryingTopology : 1;  // The prim's topology has changed since
                                   // the prim was created
    bool _displayOpacity : 1;
    bool _occludedSelectionShowsThrough : 1;

    std::unique_ptr<_FvarTopologyTracker> _fvarTopologyTracker;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_MESH_H
