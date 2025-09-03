//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
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

using HdSt_VertexAdjacencyBuilderSharedPtr =
        std::shared_ptr<class HdSt_VertexAdjacencyBuilder>;
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
    void UpdateRenderTag(HdSceneDelegate *delegate,
                         HdRenderParam *renderParam) override;

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

    bool _MaterialHasPtex(const HdRenderIndex &renderIndex, 
                          const SdfPath &materialId) const;

    bool _UseQuadIndices(const HdRenderIndex &renderIndex,
                         const HdSt_MeshTopologySharedPtr &topology) const;

    bool _MaterialHasLimitSurface(const HdRenderIndex &renderIndex, 
                                  const SdfPath &materialId) const;

    bool _UseLimitRefinement(const HdRenderIndex &renderIndex,
                             const HdMeshTopology &topology) const;

    bool _UseSmoothNormals(HdSt_MeshTopologySharedPtr const& topology) const;

    bool _UseFlatNormals(const HdMeshReprDesc &desc) const;

    void _UpdateDrawItem(HdSceneDelegate *sceneDelegate,
                         HdRenderParam *renderParam,
                         HdStDrawItem *drawItem,
                         HdDirtyBits *dirtyBits,
                         const TfToken &reprToken,
                         const HdReprSharedPtr &repr,
                         const HdMeshReprDesc &desc,
                         bool requireSmoothNormals,
                         bool requireFlatNormals,
                         int geomSubsetDescIndex);

    void _UpdateDrawItemGeometricShader(HdSceneDelegate *sceneDelegate,
                                        HdRenderParam *renderParam,
                                        HdStDrawItem *drawItem,
                                        const HdMeshReprDesc &desc,
                                        const SdfPath &materialId);

    void _UpdateShadersForAllReprs(HdSceneDelegate *sceneDelegate,
                                   HdRenderParam *renderParam,
                                   bool updateMaterialNetworkShader,
                                   bool updateGeometricShader);
    
    void _UpdateMaterialTagsForAllReprs(HdSceneDelegate *sceneDelegate,
                                        HdRenderParam *renderParam);

    void _PopulateTopology(HdSceneDelegate *sceneDelegate,
                           HdRenderParam *renderParam,
                           HdStDrawItem *drawItem,
                           HdDirtyBits *dirtyBits,
                           const TfToken &reprToken,
                           const HdReprSharedPtr &repr,
                           const HdMeshReprDesc &desc,
                           int geomSubsetDescIndex);

    void _UpdateDrawItemsForGeomSubsets(HdSceneDelegate *sceneDelegate,
                                        HdRenderParam *renderParam,
                                        HdStDrawItem *drawItem,
                                        const TfToken &reprToken,
                                        const HdReprSharedPtr &repr,
                                        const HdGeomSubsets &geomSubsets,
                                        size_t oldNumGeomSubsets);
    
    void _CreateTopologyRangeForGeomSubset(
        HdStResourceRegistrySharedPtr resourceRegistry,
        HdChangeTracker &changeTracker, 
        HdRenderParam *renderParam, 
        HdStDrawItem *drawItem, 
        const TfToken &indexToken,
        HdBufferSourceSharedPtr indicesSource, 
        HdBufferSourceSharedPtr fvarIndicesSource, 
        HdBufferSourceSharedPtr geomSubsetFaceIndicesHelperSource,
        const VtIntArray &faceIndices,
        bool refined);

    void _GatherFaceVaryingTopologies(HdSceneDelegate *sceneDelegate,
                                      const HdReprSharedPtr &repr,
                                      const HdMeshReprDesc &desc,
                                      HdStDrawItem *drawItem,
                                      int geomSubsetDescIndex,
                                      HdDirtyBits *dirtyBits,
                                      const SdfPath &id,
                                      HdSt_MeshTopologySharedPtr topology);
    
    void _PopulateAdjacency(
        HdStResourceRegistrySharedPtr const &resourceRegistry);
        
    void _PopulateVertexPrimvars(HdSceneDelegate *sceneDelegate,
                                 HdRenderParam *renderParam,
                                 const HdReprSharedPtr &repr,
                                 const HdMeshReprDesc &desc,
                                 HdStDrawItem *drawItem,
                                 int geomSubsetDescIndex,
                                 HdDirtyBits *dirtyBits,
                                 bool requireSmoothNormals);

    void _PopulateFaceVaryingPrimvars(HdSceneDelegate *sceneDelegate,
                                      HdRenderParam *renderParam,
                                      const HdReprSharedPtr &repr,
                                      const HdMeshReprDesc &desc,
                                      HdStDrawItem *drawItem,
                                      int geomSubsetDescIndex,
                                      HdDirtyBits *dirtyBits);

    void _PopulateElementPrimvars(HdSceneDelegate *sceneDelegate,
                                  HdRenderParam *renderParam,
                                  const HdReprSharedPtr &repr,
                                  const HdMeshReprDesc &desc,
                                  HdStDrawItem *drawItem,
                                  int geomSubsetDescIndex,
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
        FreeSlot // If the mesh topology has geom subsets, we might place
                 // them here as geom subsets are processed before instance 
                 // primvars. The instance primvars will follow after. If there
                 // are no geom subsets, instance primvars start here.
    };

    enum DirtyBits : HdDirtyBits {
        DirtySmoothNormals  = HdChangeTracker::CustomBitsBegin,
        DirtyFlatNormals    = (DirtySmoothNormals << 1),
        DirtyIndices        = (DirtyFlatNormals   << 1),
        DirtyHullIndices    = (DirtyIndices       << 1),
        DirtyPointsIndices  = (DirtyHullIndices   << 1)
    };

    HdSt_MeshTopologySharedPtr _topology;
    HdSt_VertexAdjacencyBuilderSharedPtr _vertexAdjacencyBuilder;

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
    bool _displayInOverlay : 1;
    bool _occludedSelectionShowsThrough : 1;
    bool _pointsShadingEnabled : 1;

    std::unique_ptr<_FvarTopologyTracker> _fvarTopologyTracker;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_MESH_H
