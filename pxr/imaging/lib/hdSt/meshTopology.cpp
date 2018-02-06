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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hdSt/meshTopology.h"
#include "pxr/imaging/hdSt/quadrangulate.h"
#include "pxr/imaging/hdSt/subdivision.h"
#include "pxr/imaging/hdSt/subdivision3.h"
#include "pxr/imaging/hdSt/triangulate.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"

#include "pxr/imaging/hd/bufferArrayRange.h"
#include "pxr/imaging/hd/bufferSource.h"
#include "pxr/imaging/hd/meshUtil.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"

#include "pxr/base/gf/vec3d.h"
#include "pxr/base/gf/vec3f.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE



// static
HdSt_MeshTopologySharedPtr
HdSt_MeshTopology::New(const HdMeshTopology &src, int refineLevel)
{
    return HdSt_MeshTopologySharedPtr(new HdSt_MeshTopology(src, refineLevel));
}

// explicit
HdSt_MeshTopology::HdSt_MeshTopology(const HdMeshTopology& src, int refineLevel)
 : HdMeshTopology(src, refineLevel)
 , _quadInfo(nullptr)
 , _quadrangulateTableRange()
 , _quadInfoBuilder()
 , _subdivision(nullptr)
 , _osdTopologyBuilder()
{
}

HdSt_MeshTopology::~HdSt_MeshTopology()
{
    delete _quadInfo;
    delete _subdivision;
}


bool
HdSt_MeshTopology::operator==(HdSt_MeshTopology const &other) const {

    TRACE_FUNCTION();

    // no need to compare _adajency and _quadInfo
    return HdMeshTopology::operator==(other);
}

void
HdSt_MeshTopology::SetQuadInfo(HdQuadInfo const *quadInfo)
{
    delete _quadInfo;
    _quadInfo = quadInfo;
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetPointsIndexBuilderComputation()
{
    // this is simple enough to return the result right away.
    int numPoints = GetNumPoints();
    VtIntArray indices(numPoints);
    for (int i = 0; i < numPoints; ++i) indices[i] = i;

    return HdBufferSourceSharedPtr(
        new HdVtBufferSource(HdTokens->indices, VtValue(indices)));
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetTriangleIndexBuilderComputation(SdfPath const &id)
{
    return HdBufferSourceSharedPtr(
        new HdSt_TriangleIndexBuilderComputation(this, id));
}

HdSt_QuadInfoBuilderComputationSharedPtr
HdSt_MeshTopology::GetQuadInfoBuilderComputation(
    bool gpu, SdfPath const &id, HdStResourceRegistry *resourceRegistry)
{
    HdSt_QuadInfoBuilderComputationSharedPtr builder(
        new HdSt_QuadInfoBuilderComputation(this, id));

    // store as a weak ptr.
    _quadInfoBuilder = builder;

    if (gpu) {
        if (!TF_VERIFY(resourceRegistry)) {
            TF_CODING_ERROR("resource registry must be non-null "
                            "if gpu quadinfo is requested.");
            return builder;
        }

        HdBufferSourceSharedPtr quadrangulateTable(
            new HdSt_QuadrangulateTableComputation(this, builder));

        // allocate quadrangulation table on GPU
        HdBufferSpecVector bufferSpecs;
        quadrangulateTable->AddBufferSpecs(&bufferSpecs);

        _quadrangulateTableRange =
            resourceRegistry->AllocateNonUniformBufferArrayRange(
                HdTokens->topology, bufferSpecs);

        resourceRegistry->AddSource(_quadrangulateTableRange, quadrangulateTable);
    }
    return builder;
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetQuadIndexBuilderComputation(SdfPath const &id)
{
    return HdBufferSourceSharedPtr(
        new HdSt_QuadIndexBuilderComputation(this, _quadInfoBuilder.lock(), id));
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetQuadrangulateComputation(
    HdBufferSourceSharedPtr const &source, SdfPath const &id)
{
    // check if the quad table is already computed as all-quads.
    if (_quadInfo && _quadInfo->IsAllQuads()) {
        // no need of quadrangulation.
        return HdBufferSourceSharedPtr();
    }

    // Make a dependency to quad info, in case if the topology
    // is chaging and the quad info hasn't been populated.
    //
    // It can be null for the second or later primvar animation.
    // Don't call GetQuadInfoBuilderComputation instead. It may result
    // unregisterd computation.
    HdBufferSourceSharedPtr quadInfo = _quadInfoBuilder.lock();

    return HdBufferSourceSharedPtr(
        new HdSt_QuadrangulateComputation(this, source, quadInfo, id));
}

HdComputationSharedPtr
HdSt_MeshTopology::GetQuadrangulateComputationGPU(
    TfToken const &name, HdType dataType, SdfPath const &id)
{
    // check if the quad table is already computed as all-quads.
    if (_quadInfo && _quadInfo->IsAllQuads()) {
        // no need of quadrangulation.
        return HdComputationSharedPtr();
    }
    return HdComputationSharedPtr(
        new HdSt_QuadrangulateComputationGPU(this, name, dataType, id));
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetQuadrangulateFaceVaryingComputation(
    HdBufferSourceSharedPtr const &source, SdfPath const &id)
{
    return HdBufferSourceSharedPtr(
        new HdSt_QuadrangulateFaceVaryingComputation(this, source, id));
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetTriangulateFaceVaryingComputation(
    HdBufferSourceSharedPtr const &source, SdfPath const &id)
{
    return HdBufferSourceSharedPtr(
        new HdSt_TriangulateFaceVaryingComputation(this, source, id));
}

bool
HdSt_MeshTopology::RefinesToTriangles() const
{
    return HdSt_Subdivision::RefinesToTriangles(_topology.GetScheme());
}

bool
HdSt_MeshTopology::RefinesToBSplinePatches() const
{
    return (IsEnabledAdaptive() &&
            HdSt_Subdivision::RefinesToBSplinePatches(_topology.GetScheme()));
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetOsdTopologyComputation(SdfPath const &id)
{
    if (HdBufferSourceSharedPtr builder = _osdTopologyBuilder.lock()) {
        return builder;
    }

    // this has to be the first instance.
    if (!TF_VERIFY(!_subdivision)) return HdBufferSourceSharedPtr();

    // create HdSt_Subdivision
    _subdivision = HdSt_Osd3Factory::CreateSubdivision();

    if (!TF_VERIFY(_subdivision)) return HdBufferSourceSharedPtr();

    bool adaptive = RefinesToBSplinePatches();

    // create a topology computation for HdSt_Subdivision
    HdBufferSourceSharedPtr builder =
        _subdivision->CreateTopologyComputation(this, adaptive,
                                                _refineLevel, id);
    _osdTopologyBuilder = builder; // retain weak ptr
    return builder;
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetOsdIndexBuilderComputation()
{
    HdBufferSourceSharedPtr topologyBuilder = _osdTopologyBuilder.lock();
    return _subdivision->CreateIndexComputation(this, topologyBuilder);
}

HdBufferSourceSharedPtr
HdSt_MeshTopology::GetOsdRefineComputation(HdBufferSourceSharedPtr const &source,
                                        bool varying)
{
    // Make a dependency to far mesh.
    // (see comment on GetQuadrangulateComputation)
    //
    // It can be null for the second or later primvar animation.
    // Don't call GetOsdTopologyComputation instead. It may result
    // unregisterd computation.

    // for empty topology, we don't need to refine anything.
    // source will be scheduled at the caller
    if (_topology.GetFaceVertexCounts().size() == 0) return source;

    if (!TF_VERIFY(_subdivision)) {
        TF_CODING_ERROR("GetOsdTopologyComputation should be called before "
                        "GetOsdRefineComputation.");
        return source;
    }

    HdBufferSourceSharedPtr topologyBuilder = _osdTopologyBuilder.lock();

    return _subdivision->CreateRefineComputation(this, source, varying,
                                                 topologyBuilder);
}

HdComputationSharedPtr
HdSt_MeshTopology::GetOsdRefineComputationGPU(TfToken const &name,
                                              HdType dataType)
{
    // for empty topology, we don't need to refine anything.
    if (_topology.GetFaceVertexCounts().size() == 0) {
        return HdComputationSharedPtr();
    }

    if (!TF_VERIFY(_subdivision)) return HdComputationSharedPtr();

    return _subdivision->CreateRefineComputationGPU(this, name, dataType);
}

PXR_NAMESPACE_CLOSE_SCOPE

