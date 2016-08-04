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
#include "uniformEvaluator.h"

#include <opensubdiv/far/stencilTable.h>
#include <opensubdiv/far/stencilTableFactory.h>
#include <opensubdiv/far/patchParam.h>
#include <opensubdiv/far/patchTable.h>
#include <opensubdiv/far/patchTableFactory.h>

#include <fstream>
#include <iostream>
#include "pxr/base/tracelite/trace.h"

#include "pxr/base/gf/vec3d.h"

using namespace std;
using namespace OpenSubdiv;


PxOsdUniformEvaluator::PxOsdUniformEvaluator():
    _vertexStencils(NULL)
{
}

PxOsdUniformEvaluator::~PxOsdUniformEvaluator()
{
    if (_vertexStencils) {
        // Allocated by Far::StencilTableFactory::Create
        delete(_vertexStencils);
    }
}


// Inverse of OpenSubdiv::Far::PatchParam::Normalize
static void
_InverseNormalize(OpenSubdiv::Far::PatchParam const &patchParam,
                  float& u, float& v)
{
    float frac = patchParam.GetParamFraction();
    float pu = (float) patchParam.GetU() * frac;
    float pv = (float) patchParam.GetV() * frac;

    u = u * frac + pu;
    v = v * frac + pv;
}

bool
PxOsdUniformEvaluator::Initialize(
    PxOsdTopologyRefinerSharedPtr refiner,
    const PxOsdMeshTopology &topology,
    int level,
    string *errorMessage)    
{    
    TRACE_FUNCTION();

    _topology = topology;

    refiner->RefineUniform(level);
    
    Far::StencilTableFactory::Options options;
    options.generateOffsets = true;
    options.generateIntermediateLevels = false;
    _vertexStencils =
        Far::StencilTableFactory::Create(*refiner, options);

    Far::PatchTable const *patchTable  =
        Far::PatchTableFactory::Create(*refiner);

    if (not (TF_VERIFY(_vertexStencils) and TF_VERIFY(patchTable))) {
        return false;
    }
    
    // populate refined quad indices, note that four indices are packed
    // for each quad.
    int numRefinedQuadIndices = refiner->GetLevel(level).GetNumFaceVertices();
    
    _refinedQuadIndices.resize(numRefinedQuadIndices);
    for (int i=0; i<numRefinedQuadIndices/4; ++i) {
        OpenSubdiv::Far::ConstIndexArray faceVertices =
            refiner->GetLevel(level).GetFaceVertices(i);
        int baseIndex = i*4;
        if (faceVertices.size() != 4) {
            TF_WARN("Non-quad found after subdivision, bailing");
            delete patchTable;            
            return false;
        }
        for (int j=0; j<4; ++j) {
            _refinedQuadIndices[baseIndex+j] = faceVertices[j];
        }
    }
    

    int numRefinedQuads = static_cast<int>(_refinedQuadIndices.size())/4;
    _subfaceUvs.resize(_refinedQuadIndices.size());
    vector<float>::iterator uvIt = _subfaceUvs.begin();

    _ptexIndices.resize(numRefinedQuads);
    vector<int>::iterator idIt = _ptexIndices.begin();

    const Far::PatchParamTable &patchParamTable =
        patchTable->GetPatchParamTable();
    
    for (int refinedIndex = 0; refinedIndex < numRefinedQuads; ++refinedIndex) {
        
        const Far::PatchParam& param =
            patchParamTable[refinedIndex];
        
        float u0 = 0;
        float v0 = 0;
        _InverseNormalize(param, u0, v0);
        
        float u1 = 1;
        float v1 = 1;
        _InverseNormalize(param, u1, v1);
        
        *idIt++ = param.GetFaceId();
        *uvIt++ = u0;
        *uvIt++ = v0;
        *uvIt++ = u1;
        *uvIt++ = v1;
    }
    
    delete patchTable;

    return true;
}


bool
PxOsdUniformEvaluator::EvaluatePoint(
    const std::vector<GfVec3d> &coarsePoints,
    int index,
    GfVec3d *result,
    string *errorMessage
    )   
{

    if (index >= _vertexStencils->GetNumStencils()) {
        if (errorMessage)
            *errorMessage =
                "Indexing error in PxOsdUniformEvaluator::EvaluatePoint";
        return false;
    }

    if (coarsePoints.size() != static_cast<size_t>(_vertexStencils->GetNumControlVertices())) {
        if (errorMessage)
            *errorMessage =
                "Mismatch in expected #control vertices in PxOsdUniformEvaluator::EvaluatePoint";
        return false;        
    }
    
    Far::Stencil stencil = _vertexStencils->GetStencil(index);

    
    int size = stencil.GetSize();
    const int *indices = stencil.GetVertexIndices();
    const float *weights = stencil.GetWeights();

    *result = GfVec3d(0.0, 0.0, 0.0);
    
    // For each element in the array, add the coefs contribution
    
    for (int i=0; i<size; ++i) {
        *result += coarsePoints[indices[i]] * weights[i];
    }

    return true;
}

                                            

