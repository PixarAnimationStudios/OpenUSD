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
#include "pxr/imaging/hd/subdivision.h"
#include "pxr/imaging/pxOsd/tokens.h"

/*virtual*/
Hd_Subdivision::~Hd_Subdivision()
{
}

bool
Hd_Subdivision::RefinesToTriangles(TfToken const &scheme)
{
    // XXX: Ideally we'd like to delegate this to the concrete class.
    if (scheme == PxOsdOpenSubdivTokens->loop) {
        return true;
    }
    return false;
}

bool
Hd_Subdivision::RefinesToBSplinePatches(TfToken const &scheme)
{
    if (scheme == PxOsdOpenSubdivTokens->catmark ||
        scheme == PxOsdOpenSubdivTokens->catmullClark) {
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
Hd_OsdTopologyComputation::Hd_OsdTopologyComputation(
    HdMeshTopology *topology, int level, SdfPath const &id)
    : _topology(topology), _level(level), _id(id)
{
}

/*virtual*/
void
Hd_OsdTopologyComputation::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    // nothing
}

// ---------------------------------------------------------------------------
Hd_OsdIndexComputation::Hd_OsdIndexComputation(
    HdMeshTopology *topology, HdBufferSourceSharedPtr const &osdTopology)
    : _topology(topology)
    , _osdTopology(osdTopology)
{
}

/*virtual*/
void
Hd_OsdIndexComputation::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    if (Hd_Subdivision::RefinesToTriangles(_topology->GetScheme())) {
        // triangles (loop)
        specs->push_back(HdBufferSpec(HdTokens->indices, GL_INT, 3));
        specs->push_back(HdBufferSpec(HdTokens->primitiveParam, GL_INT, 3));
    } else if (_topology->RefinesToBSplinePatches()) {
        // bi-cubic bspline patches
        specs->push_back(HdBufferSpec(HdTokens->indices, GL_INT, 16));
        // 3+1 (includes sharpness)
        specs->push_back(HdBufferSpec(HdTokens->primitiveParam, GL_INT, 4));
    } else {
        // quads (catmark, bilinear)
        specs->push_back(HdBufferSpec(HdTokens->indices, GL_INT, 4));
        specs->push_back(HdBufferSpec(HdTokens->primitiveParam, GL_INT, 3));
    }
}

/*virtual*/
bool
Hd_OsdIndexComputation::HasChainedBuffer() const
{
    return true;
}

/*virtual*/
HdBufferSourceSharedPtr
Hd_OsdIndexComputation::GetChainedBuffer() const
{
    return _primitiveBuffer;
}

/*virtual*/
bool
Hd_OsdIndexComputation::_CheckValid() const
{
    return true;
}


// ---------------------------------------------------------------------------
/// OpenSubdiv GPU Refinement
///
///
Hd_OsdRefineComputationGPU::Hd_OsdRefineComputationGPU(HdMeshTopology *topology,
                                                       TfToken const &name,
                                                       GLenum dataType,
                                                       int numComponents)
    : _topology(topology), _name(name),
      _dataType(dataType), _numComponents(numComponents)
{
}

void
Hd_OsdRefineComputationGPU::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    // nothing
    //
    // GPU subdivision requires the source data on GPU in prior to
    // execution, so no need to populate bufferspec on registration.
}

void
Hd_OsdRefineComputationGPU::Execute(HdBufferArrayRangeSharedPtr const &range)
{
    HD_TRACE_FUNCTION();
    HD_MALLOC_TAG_FUNCTION();

    Hd_Subdivision *subdivision = _topology->GetSubdivision();
    if (!TF_VERIFY(subdivision)) return;

    subdivision->RefineGPU(range, _name);

    HD_PERF_COUNTER_INCR(HdPerfTokens->subdivisionRefineGPU);
}

int
Hd_OsdRefineComputationGPU::GetNumOutputElements() const
{
    // returns the total number of vertices, including coarse and refined ones.
    Hd_Subdivision const *subdivision = _topology->GetSubdivision();
    if (!TF_VERIFY(subdivision)) return 0;
    return subdivision->GetNumVertices();
}
