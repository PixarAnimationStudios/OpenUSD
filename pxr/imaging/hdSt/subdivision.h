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
#ifndef PXR_IMAGING_HD_ST_SUBDIVISION_H
#define PXR_IMAGING_HD_ST_SUBDIVISION_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/computation.h"
#include "pxr/imaging/hdSt/meshTopology.h"

#include "pxr/imaging/hd/bufferSource.h"

#include "pxr/usd/sdf/path.h"
#include "pxr/base/tf/token.h"

#include <opensubdiv/far/patchTable.h>
#include <opensubdiv/far/stencilTable.h>

#include <memory>
#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE


using HdSt_GpuStencilTableSharedPtr =
        std::shared_ptr<class HdSt_GpuStencilTable>;

/// \class Hd_Subdivision
///
/// Subdivision struct holding subdivision tables and patch tables.
///
/// This single struct can be used for cpu and gpu subdivision at the same time.
///
class HdSt_Subdivision final
{
public:
    using StencilTable = OpenSubdiv::Far::StencilTable;
    using PatchTable = OpenSubdiv::Far::PatchTable;

    HdSt_Subdivision(bool adaptive, int refineLevel);
    ~HdSt_Subdivision();

    bool IsAdaptive() const {
        return _adaptive;
    }

    int GetRefineLevel() const {
        return _refineLevel;
    }

    int GetNumVertices() const;
    int GetNumVarying() const;
    int GetNumFaceVarying(int channel) const;
    int GetMaxNumFaceVarying() const;

    VtIntArray GetRefinedFvarIndices(int channel) const;

    void RefineCPU(HdBufferSourceSharedPtr const &source,
                   std::vector<float> *primvarBuffer,
                   HdSt_MeshTopology::Interpolation interpolation,
                   int fvarChannel = 0);
    void RefineGPU(HdBufferArrayRangeSharedPtr const &primvarBuffer,
                   TfToken const &primvarName,
                   HdSt_GpuStencilTableSharedPtr const &gpuStencilTable,
                   HdStResourceRegistry *resourceRegistry);

    // computation factory methods
    HdBufferSourceSharedPtr CreateTopologyComputation(
        HdSt_MeshTopology *topology,
        SdfPath const &id);

    HdBufferSourceSharedPtr CreateIndexComputation(
        HdSt_MeshTopology *topology,
        HdBufferSourceSharedPtr const &osdTopology);

    HdBufferSourceSharedPtr CreateFvarIndexComputation(
        HdSt_MeshTopology *topology,
        HdBufferSourceSharedPtr const &osdTopology,
        int channel);

    HdBufferSourceSharedPtr CreateRefineComputationCPU(
        HdSt_MeshTopology *topology,
        HdBufferSourceSharedPtr const &source,
        HdBufferSourceSharedPtr const &osdTopology,
        HdSt_MeshTopology::Interpolation interpolation,
        int fvarChannel = 0);

    HdStComputationSharedPtr CreateRefineComputationGPU(
        HdSt_MeshTopology *topology,
        HdBufferSourceSharedPtr const &osdTopology,
        TfToken const &name,
        HdType type,
        HdStResourceRegistry *resourceRegistry,
        HdSt_MeshTopology::Interpolation interpolation,
        int fvarChannel = 0);

    HdBufferSourceSharedPtr CreateBaseFaceToRefinedFacesMapComputation(
        HdBufferSourceSharedPtr const &osdTopology);

    /// Returns true if the subdivision for \a scheme generates triangles,
    /// instead of quads.
    static bool RefinesToTriangles(TfToken const &scheme);

    /// Returns true if the subdivision for \a scheme generates bspline patches.
    static bool RefinesToBSplinePatches(TfToken const &scheme);

    /// Returns true if the subdivision for \a scheme generates box spline
    /// triangle patches.
    static bool RefinesToBoxSplineTrianglePatches(TfToken const &scheme);

    /// Takes ownership of stencil tables and patch table
    void SetRefinementTables(
        std::unique_ptr<StencilTable const> && vertexStencils,
        std::unique_ptr<StencilTable const> && varyingStencils,
        std::vector<std::unique_ptr<StencilTable const>> && faceVaryingStencils,
        std::unique_ptr<PatchTable const> && patchTable);

    StencilTable const *
    GetStencilTable(HdSt_MeshTopology::Interpolation interpolation,
                    int fvarChannel) const;

    PatchTable const *GetPatchTable() const {
        return _patchTable.get();
    }

private:
    HdSt_GpuStencilTableSharedPtr
    _GetGpuStencilTable(
        HdSt_MeshTopology *topology,
        HdBufferSourceSharedPtr const & osdTopology,
        HdStResourceRegistry * registry,
        HdSt_MeshTopology::Interpolation interpolation,
        int fvarChannel = 0);

    HdSt_GpuStencilTableSharedPtr
    _CreateGpuStencilTable(
        HdBufferSourceSharedPtr const & osdTopology,
        HdStResourceRegistry * registry,
        HdSt_MeshTopology::Interpolation interpolation,
        int fvarChannel = 0) const;

    std::unique_ptr<StencilTable const> _vertexStencils;
    std::unique_ptr<StencilTable const> _varyingStencils;
    std::vector<std::unique_ptr<StencilTable const>> _faceVaryingStencils;
    std::unique_ptr<PatchTable const> _patchTable;

    bool const _adaptive;
    int const _refineLevel;
    int _maxNumFaceVarying; // calculated during SetRefinementTables()

    std::mutex _gpuStencilMutex;
    HdSt_GpuStencilTableSharedPtr _gpuVertexStencils;
    HdSt_GpuStencilTableSharedPtr  _gpuVaryingStencils;
    std::vector<HdSt_GpuStencilTableSharedPtr> _gpuFaceVaryingStencils;
};

// ---------------------------------------------------------------------------
/// \class Hd_OsdRefineComputation
///
/// OpenSubdiv CPU Refinement.
/// This class isn't inherited from HdComputedBufferSource.
/// GetData() returns the internal buffer to skip unecessary copy.
///
class HdSt_OsdRefineComputationCPU final : public HdBufferSource
{
public:
    HdSt_OsdRefineComputationCPU(HdSt_MeshTopology *topology,
                            HdBufferSourceSharedPtr const &source,
                            HdBufferSourceSharedPtr const &osdTopology,
                            HdSt_MeshTopology::Interpolation interpolation,
                            int fvarChannel = 0);
    ~HdSt_OsdRefineComputationCPU() override;

    TfToken const &GetName() const override;
    size_t ComputeHash() const override;
    void const* GetData() const override;
    HdTupleType GetTupleType() const override;
    size_t GetNumElements() const override;
    void GetBufferSpecs(HdBufferSpecVector *specs) const override;
    bool Resolve() override;
    bool HasPreChainedBuffer() const override;
    HdBufferSourceSharedPtr GetPreChainedBuffer() const override;
    HdSt_MeshTopology::Interpolation GetInterpolation() const;

protected:
    bool _CheckValid() const override;

private:
    HdSt_MeshTopology *_topology;
    HdBufferSourceSharedPtr _source;
    HdBufferSourceSharedPtr _osdTopology;
    std::vector<float> _primvarBuffer;
    HdSt_MeshTopology::Interpolation _interpolation;
    int _fvarChannel;
};

// ---------------------------------------------------------------------------
/// \class HdSt_OsdRefineComputationGPU
///
/// OpenSubdiv GPU Refinement.
///
class HdSt_OsdRefineComputationGPU final : public HdStComputation
{
public:
    HdSt_OsdRefineComputationGPU(
        HdSt_MeshTopology *topology,
        TfToken const &primvarName,
        HdType type,
        HdSt_GpuStencilTableSharedPtr const & gpuStencilTable,
        HdSt_MeshTopology::Interpolation interpolation);
    ~HdSt_OsdRefineComputationGPU() override;

    void Execute(HdBufferArrayRangeSharedPtr const &range,
                         HdResourceRegistry *resourceRegistry) override;
    void GetBufferSpecs(HdBufferSpecVector *specs) const override;
    int GetNumOutputElements() const override;
    HdSt_MeshTopology::Interpolation GetInterpolation() const;

private:
    HdSt_MeshTopology *_topology;
    TfToken _primvarName;
    HdSt_GpuStencilTableSharedPtr _gpuStencilTable;
    HdSt_MeshTopology::Interpolation _interpolation;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_SUBDIVISION_H
