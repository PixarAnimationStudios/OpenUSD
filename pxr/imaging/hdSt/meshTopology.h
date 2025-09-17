//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_IMAGING_HD_ST_MESH_TOPOLOGY_H
#define PXR_IMAGING_HD_ST_MESH_TOPOLOGY_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/version.h"
#include "pxr/imaging/hd/meshTopology.h"
#include "pxr/imaging/hd/types.h"

#include <memory>

PXR_NAMESPACE_OPEN_SCOPE


class HdStResourceRegistry;
class HdSt_Subdivision;
struct HdQuadInfo;
class SdfPath;


using HdBufferSourceWeakPtr = 
    std::weak_ptr<class HdBufferSource>;
using HdBufferSourceSharedPtr = 
    std::shared_ptr<class HdBufferSource>;

using HdBufferArrayRangeSharedPtr =
    std::shared_ptr<class HdBufferArrayRange>;

using HdStComputationSharedPtr = std::shared_ptr<class HdStComputation>;

using HdSt_AdjacencyBuilderComputationPtr = 
    std::weak_ptr<class HdSt_AdjacencyBuilderComputation>;

using HdSt_QuadInfoBuilderComputationPtr = 
    std::weak_ptr<class HdSt_QuadInfoBuilderComputation>;
using HdSt_QuadInfoBuilderComputationSharedPtr =
    std::shared_ptr<class HdSt_QuadInfoBuilderComputation>;

using HdSt_MeshTopologySharedPtr = std::shared_ptr<class HdSt_MeshTopology>;



/// \class HdSt_MeshTopology
///
/// Storm implementation for mesh topology.
///
class HdSt_MeshTopology final : public HdMeshTopology {
public:
    /// Specifies how subdivision mesh topology is refined.
    enum RefineMode {
        RefineModeUniform = 0,
        RefineModePatches
    };

    /// Specifies whether quads are triangulated or untriangulated.
    enum QuadsMode {
        QuadsTriangulated = 0,
        QuadsUntriangulated
    };

    /// Specifies type of interpolation to use in refinement
    enum Interpolation {
        INTERPOLATE_VERTEX,
        INTERPOLATE_VARYING,
        INTERPOLATE_FACEVARYING,
    };

    HDST_API
    static HdSt_MeshTopologySharedPtr New(
        const HdMeshTopology &src,
        int refineLevel,
        RefineMode refineMode = RefineModeUniform,
        QuadsMode quadsMode = QuadsUntriangulated);

    HDST_API
    virtual ~HdSt_MeshTopology();

    /// Equality check between two mesh topologies.
    HDST_API
    bool operator==(HdSt_MeshTopology const &other) const;

    /// \name Triangulation
    /// @{

    /// Returns the triangle indices (for drawing) buffer source computation.
    HDST_API
    HdBufferSourceSharedPtr GetTriangleIndexBuilderComputation(
        SdfPath const &id);

    /// Returns the CPU face-varying triangulate computation
    HDST_API
    HdBufferSourceSharedPtr GetTriangulateFaceVaryingComputation(
        HdBufferSourceSharedPtr const &source,
        SdfPath const &id);

    /// @}

    ///
    /// \name Quadrangulation
    /// @{

    /// Returns the quads mode (triangulated or untriangulated).
    QuadsMode GetQuadsMode() const {
        return _quadsMode;
    }

    /// Helper function returns whether quads are triangulated.
    bool TriangulateQuads() const {
        return _quadsMode == QuadsTriangulated;
    }

    /// Returns the quadinfo computation for the use of primvar
    /// quadrangulation.
    /// If gpu is true, the quadrangulate table will be transferred to GPU
    /// via the resource registry.
    HDST_API
    HdSt_QuadInfoBuilderComputationSharedPtr GetQuadInfoBuilderComputation(
        bool gpu, SdfPath const &id,
        HdStResourceRegistry *resourceRegistry = nullptr);

    /// Returns the quad indices (for drawing) buffer source computation.
    HDST_API
    HdBufferSourceSharedPtr GetQuadIndexBuilderComputation(SdfPath const &id);

    /// Returns the CPU quadrangulated buffer source.
    HDST_API
    HdBufferSourceSharedPtr GetQuadrangulateComputation(
        HdBufferSourceSharedPtr const &source, SdfPath const &id);

    /// Returns the GPU quadrangulate computation.
    HDST_API
    HdStComputationSharedPtr GetQuadrangulateComputationGPU(
        TfToken const &name, HdType dataType, SdfPath const &id);

    /// Returns the CPU face-varying quadrangulate computation
    HDST_API
    HdBufferSourceSharedPtr GetQuadrangulateFaceVaryingComputation(
        HdBufferSourceSharedPtr const &source, SdfPath const &id);

    /// Returns the quadrangulation table range on GPU
    HdBufferArrayRangeSharedPtr const &GetQuadrangulateTableRange() const {
        return _quadrangulateTableRange;
    }

    /// Clears the quadrangulation table range
    void ClearQuadrangulateTableRange() {
        _quadrangulateTableRange.reset();
    }

    /// Sets the quadrangulation struct. HdMeshTopology takes an
    /// ownership of quadInfo (caller shouldn't free)
    HDST_API
    void SetQuadInfo(HdQuadInfo const *quadInfo);

    /// Returns the quadrangulation struct.
    HdQuadInfo const *GetQuadInfo() const {
        return _quadInfo.get();
    }

    /// @}

    ///
    /// \name Points
    /// @{

    /// Returns the point indices buffer source computation.
    HDST_API
    HdBufferSourceSharedPtr GetPointsIndexBuilderComputation();

    /// @}

    ///
    /// \name Subdivision
    /// @{


    /// Returns the subdivision struct.
    HdSt_Subdivision const *GetSubdivision() const {
        return _subdivision.get();
    }

    /// Returns the subdivision struct (non-const).
    HdSt_Subdivision *GetSubdivision() {
        return _subdivision.get();
    }

    /// Returns true if the subdivision on this mesh produces
    /// triangles (otherwise quads)
    HDST_API
    bool RefinesToTriangles() const;

    /// Returns true if the subdivision of this mesh produces bspline patches
    HDST_API
    bool RefinesToBSplinePatches() const;

    /// Returns true if the subdivision of this mesh produces box spline
    /// triangle patches
    HDST_API
    bool RefinesToBoxSplineTrianglePatches() const;

    /// Returns the subdivision topology computation. It computes
    /// far mesh and produces refined quad-indices buffer.
    HDST_API
    HdBufferSourceSharedPtr GetOsdTopologyComputation(SdfPath const &debugId);

    /// Returns the refined indices builder computation.
    /// This just returns index and primitive buffer, and should be preceded by
    /// topology computation.
    HDST_API
    HdBufferSourceSharedPtr GetOsdIndexBuilderComputation();

    /// Returns the refined face-varying indices builder computation.
    /// This just returns the index and patch param buffer for a face-varying
    /// channel, and should be preceded by topology computation.
    HDST_API
    HdBufferSourceSharedPtr GetOsdFvarIndexBuilderComputation(int channel);

    /// Returns the subdivision primvar refine computation on CPU.
    HDST_API
    HdBufferSourceSharedPtr GetOsdRefineComputation(
        HdBufferSourceSharedPtr const &source, 
        Interpolation interpolation,
        int fvarChannel = 0);

    /// Returns the subdivision primvar refine computation on GPU.
    HDST_API
    HdStComputationSharedPtr GetOsdRefineComputationGPU(
        TfToken const &name,
        HdType dataType,
        HdStResourceRegistry *resourceRegistry,
        Interpolation interpolation,
        int fvarChannel = 0);

    /// Returns the mapping from base face to refined face indices.
    HDST_API
    HdBufferSourceSharedPtr GetOsdBaseFaceToRefinedFacesMapComputation(
        HdStResourceRegistry *resourceRegistry);

    /// @}

    ///
    /// \name Geom Subsets
    /// @{
    
    /// Processes geom subsets to remove those with empty indices or empty 
    /// material id. Will initialize _nonSubsetFaces if there are geom subsets.
    HDST_API
    void SanitizeGeomSubsets();

    /// Returns the indices subset computation for unrefined indices.
    HDST_API
    HdBufferSourceSharedPtr GetIndexSubsetComputation(
        HdBufferSourceSharedPtr indexBuilderSource, 
        HdBufferSourceSharedPtr faceIndicesSource);

    /// Returns the indices subset computation for refined indices.
    HDST_API
    HdBufferSourceSharedPtr GetRefinedIndexSubsetComputation(
        HdBufferSourceSharedPtr indexBuilderSource, 
        HdBufferSourceSharedPtr faceIndicesSource);
    
    /// Returns the triangulated/quadrangulated face indices computation.
    HDST_API
    HdBufferSourceSharedPtr GetGeomSubsetFaceIndexBuilderComputation(
        HdBufferSourceSharedPtr geomSubsetFaceIndexHelperSource, 
        VtIntArray const &faceIndices);

    /// Returns computation creating buffer sources used in mapping authored 
    /// face indices to triangulated/quadrangulated face indices.
    HDST_API
    HdBufferSourceSharedPtr GetGeomSubsetFaceIndexHelperComputation(
        bool refined, 
        bool quadrangulated);

    /// @}

    ///
    /// \name Face-varying Topologies
    /// @{
    /// Returns the face indices of faces not used in any geom subsets.
    std::vector<int> const *GetNonSubsetFaces() const {
        return _nonSubsetFaces.get();
    }

    /// Sets the face-varying topologies.
    void SetFvarTopologies(std::vector<VtIntArray> const &fvarTopologies) {
        _fvarTopologies = fvarTopologies;
    }

    /// Returns the face-varying topologies.
    std::vector<VtIntArray> const & GetFvarTopologies()  {
        return _fvarTopologies;
    }

    /// @}

private:
    QuadsMode _quadsMode;

    // quadrangulation info on CPU
    std::unique_ptr<HdQuadInfo const> _quadInfo;

    // quadrangulation info on GPU
    HdBufferArrayRangeSharedPtr _quadrangulateTableRange;

    HdSt_QuadInfoBuilderComputationPtr _quadInfoBuilder;

    // OpenSubdiv
    RefineMode _refineMode;
    std::unique_ptr<HdSt_Subdivision> _subdivision;
    HdBufferSourceWeakPtr _osdTopologyBuilder;
    HdBufferSourceWeakPtr _osdBaseFaceToRefinedFacesMap;

    std::vector<VtIntArray> _fvarTopologies;

    // When using geom subsets, the indices of faces that are not contained
    // within the geom subsets. Populated by SanitizeGeomSubsets().
    std::unique_ptr<std::vector<int>> _nonSubsetFaces;

    // Must be created through factory
    explicit HdSt_MeshTopology(
        const HdMeshTopology &src,
        int refineLevel,
        RefineMode refineMode,
        QuadsMode quadsMode);

    // No default construction or copying.
    HdSt_MeshTopology()                                      = delete;
    HdSt_MeshTopology(const HdSt_MeshTopology &)             = delete;
    HdSt_MeshTopology &operator =(const HdSt_MeshTopology &) = delete;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_MESH_TOPOLOGY_H
