//
// Copyright 2019 Pixar
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
#ifndef PXR_IMAGING_HD_ST_DOME_LIGHT_COMPUTATIONS_H
#define PXR_IMAGING_HD_ST_DOME_LIGHT_COMPUTATIONS_H

#include "pxr/pxr.h"
#include "pxr/imaging/hdSt/api.h"
#include "pxr/imaging/hd/computation.h"

#include "pxr/base/tf/token.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Dome Light texture computations GPU
///
///
typedef boost::shared_ptr<class HdSt_DomeLightComputationGPU> 
                            HdSt_DomeLightComputationGPUSharedPtr;
                            
class HdSt_DomeLightComputationGPU : public HdComputation {
public:
    /// Constructor
    HDST_API
    HdSt_DomeLightComputationGPU(TfToken token, 
                                 unsigned int sourceId, 
                                 unsigned int destId, 
                                 int width, int height, 
                                 unsigned int numLevels, 
                                 unsigned int level, 
                                 float roughness=-1.0);

    HDST_API
    virtual void GetBufferSpecs(HdBufferSpecVector *specs) const override {}
   
    HDST_API
    virtual void Execute(HdBufferArrayRangeSharedPtr const &range,
                         HdResourceRegistry *resourceRegistry) override;

    /// This computation doesn't generate buffer source (i.e. 2nd phase)
    /// This is a gpu computation, but no need to resize the destination
    /// since it belongs the same range as src buffer.
    virtual int GetNumOutputElements() const override { return 0; }

private:
    TfToken _shaderToken;

    unsigned int _sourceTextureId;
    unsigned int _destTextureId;
    int _textureWidth;
    int _textureHeight;

    unsigned int _numLevels;
    unsigned int _level;
    bool _layered;
    unsigned int _layer;

    float _roughness;
};


PXR_NAMESPACE_CLOSE_SCOPE

#endif // PXR_IMAGING_HD_ST_DOME_LIGHT_COMPUTATIONS_H
