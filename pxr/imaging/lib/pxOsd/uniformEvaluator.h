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
#ifndef PXOSDUTIL_UNIFORM_EVALUATOR_H
#define PXOSDUTIL_UNIFORM_EVALUATOR_H


#include "pxr/imaging/pxOsd/refinerFactory.h"
#include "pxr/imaging/pxOsd/tokens.h"

#include <opensubdiv/far/stencilTable.h>

#include <string>
#include <vector>

// This class takes a mesh that has undergone uniform refinement to
// a fixed subdivision level, and creates required run time OpenSubdiv
// data structures used to sample values on subdivision surfaces.
//
//
class PxOsdUniformEvaluator {
  public:
    PxOsdUniformEvaluator();

    ~PxOsdUniformEvaluator();    

    // Initialize returns false on error.  If errorMessage is non-NULL it'll
    // be populated upon error.
    //
    // If successful quad topology and vertex stencils will have been populated
    // and stored as member variables.
    //
    // We don't hold a reference to the refiner, it is used during the
    // function call and not needed afterword.
    //
    bool Initialize(
        PxOsdTopologyRefinerSharedPtr refiner,
        const PxOsdMeshTopology &topology,
        int level,
        std::string *errorMessage = NULL);
    

    // Fetch the topology of the post-refined mesh. The "quads" vector
    // will be filled with 4 ints per quad which index into a vector
    // of positions.
    //
    const std::vector<int> &GetRefinedQuads() const {
        return _refinedQuadIndices;
    } 

    const OpenSubdiv::Far::StencilTable *GetVertexStencils() const {
        return _vertexStencils;
    }

    // For the refined point with index "index" use stencil table
    // to compute the result of subdivision on coarsePoints. Note
    // that this isn't the limit point, but the result of N subdivision
    // steps passed as "level" to Initialize
    bool EvaluatePoint(
        const std::vector<GfVec3d> &coarsePoints,
        int index,
        GfVec3d *result,
        std::string *errorMessage = NULL);

    // Fetch the U/V coordinates of the refined quads in the U/V space
    // of their parent coarse face
    //
    // Ptex indices and parametric coordinates for each
    // refined quad. _subfaceUvs has four floats per quad,
    // (minU, minV, maxU, maxV). _ptexIndices has one int
    // per quad, the ptex index of the coarse face this refined
    // quad came from. This isn't the same as the index of the base
    // face, as triangles etc are subdivided before determining ptex
    // index.
    const std::vector<float>  &GetRefinedPtexUvs() const { return _subfaceUvs;}
    const std::vector<int>  &GetRefinedPtexIndices() const { return _ptexIndices;}
    
    const PxOsdMeshTopology &GetTopology() const { return _topology;}
        
  private:    

    // Topology of the base mesh
    PxOsdMeshTopology _topology;
    
    // 4 ints for each refined quad, length is 4*#quads
    std::vector<int> _refinedQuadIndices;

    const OpenSubdiv::Far::StencilTable *_vertexStencils;

    std::vector<float> _subfaceUvs;
    std::vector<int> _ptexIndices;    
};

#endif /* PXOSDUTIL_UNIFORM_EVALUATOR_H */
