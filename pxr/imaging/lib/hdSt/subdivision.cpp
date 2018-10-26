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
#include "pxr/imaging/hdSt/subdivision.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/pxOsd/tokens.h"

PXR_NAMESPACE_OPEN_SCOPE


/*virtual*/
HdSt_Subdivision::~HdSt_Subdivision()
{
}

bool
HdSt_Subdivision::RefinesToTriangles(TfToken const &scheme)
{
    // XXX: Ideally we'd like to delegate this to the concrete class.
    if (scheme == PxOsdOpenSubdivTokens->loop) {
        return true;
    }
    return false;
}

bool
HdSt_Subdivision::RefinesToBSplinePatches(TfToken const &scheme)
{
    if (scheme == PxOsdOpenSubdivTokens->catmark ||
        scheme == PxOsdOpenSubdivTokens->catmullClark) {
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
HdSt_OsdTopologyComputation::HdSt_OsdTopologyComputation(
    HdSt_MeshTopology *topology, int level, SdfPath const &id)
    : _topology(topology), _level(level), _id(id)
{
}

/*virtual*/
void
HdSt_OsdTopologyComputation::GetBufferSpecs(HdBufferSpecVector *specs) const
{
    // nothing
}

// ---------------------------------------------------------------------------
HdSt_OsdIndexComputation::HdSt_OsdIndexComputation(
    HdSt_MeshTopology *topology, HdBufferSourceSharedPtr const &osdTopology)
    : _topology(topology)
    , _osdTopology(osdTopology)
{
}

/*virtual*/
void
HdSt_OsdIndexComputation::GetBufferSpecs(HdBufferSpecVector *specs) const
{
    if (HdSt_Subdivision::RefinesToTriangles(_topology->GetScheme())) {
        // triangles (loop)
        specs->emplace_back(HdTokens->indices,
                            HdTupleType {HdTypeInt32Vec3, 1});
        specs->emplace_back(HdTokens->primitiveParam,
                            HdTupleType {HdTypeInt32Vec3, 1});
        // vec3 will suffice, but this unifies it for all the cases
        specs->emplace_back(HdTokens->edgeIndices,
                            HdTupleType {HdTypeInt32Vec4, 1});
    } else if (_topology->RefinesToBSplinePatches()) {
        // bi-cubic bspline patches
        specs->emplace_back(HdTokens->indices,
                            HdTupleType {HdTypeInt32, 16});
        // 3+1 (includes sharpness)
        specs->emplace_back(HdTokens->primitiveParam,
                            HdTupleType {HdTypeInt32Vec4, 1});
        specs->emplace_back(HdTokens->edgeIndices,
                            HdTupleType {HdTypeInt32Vec4, 1});
    } else {
        // quads (catmark, bilinear)
        specs->emplace_back(HdTokens->indices,
                            HdTupleType {HdTypeInt32Vec4, 1});
        specs->emplace_back(HdTokens->primitiveParam,
                            HdTupleType {HdTypeInt32Vec3, 1});
        specs->emplace_back(HdTokens->edgeIndices,
                            HdTupleType {HdTypeInt32Vec4, 1});
    }
}

/*virtual*/
bool
HdSt_OsdIndexComputation::HasChainedBuffer() const
{
    return true;
}

/*virtual*/
HdBufferSourceVector
HdSt_OsdIndexComputation::GetChainedBuffers() const
{
    return { _primitiveBuffer, _edgeIndicesBuffer };
}

/*virtual*/
bool
HdSt_OsdIndexComputation::_CheckValid() const
{
    return true;
}


// ---------------------------------------------------------------------------
/// OpenSubdiv GPU Refinement
///
///
HdSt_OsdRefineComputationGPU::HdSt_OsdRefineComputationGPU(
                                                    HdSt_MeshTopology *topology,
                                                    TfToken const &name,
                                                    HdType type)
    : _topology(topology), _name(name), _type(type)
{
}

void
HdSt_OsdRefineComputationGPU::GetBufferSpecs(HdBufferSpecVector *specs) const
{
    // nothing
    //
    // GPU subdivision requires the source data on GPU in prior to
    // execution, so no need to populate bufferspec on registration.
}

void
HdSt_OsdRefineComputationGPU::Execute(HdBufferArrayRangeSharedPtr const &range,
                                      HdResourceRegistry *resourceRegistry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    HdSt_Subdivision *subdivision = _topology->GetSubdivision();
    if (!TF_VERIFY(subdivision)) return;

    subdivision->RefineGPU(range, _name);

    HD_PERF_COUNTER_INCR(HdPerfTokens->subdivisionRefineGPU);
}

int
HdSt_OsdRefineComputationGPU::GetNumOutputElements() const
{
    // returns the total number of vertices, including coarse and refined ones.
    HdSt_Subdivision const *subdivision = _topology->GetSubdivision();
    if (!TF_VERIFY(subdivision)) return 0;
    return subdivision->GetNumVertices();
}

PXR_NAMESPACE_CLOSE_SCOPE

