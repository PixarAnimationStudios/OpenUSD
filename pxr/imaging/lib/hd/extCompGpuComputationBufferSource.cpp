//
// Copyright 2017 Pixar
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
#include "pxr/imaging/glf/glew.h"

#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/imaging/hd/binding.h"
#include "pxr/imaging/hd/bufferArrayRangeGL.h"
#include "pxr/imaging/hd/bufferResourceGL.h"
#include "pxr/imaging/hd/codeGen.h"
#include "pxr/imaging/hd/computeShader.h"
#include "pxr/imaging/hd/extCompGpuComputation.h"
#include "pxr/imaging/hd/extCompGpuComputationBufferSource.h"
#include "pxr/imaging/hd/glslProgram.h"
#include "pxr/imaging/hd/glUtils.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/resourceBinder.h"
#include "pxr/imaging/hd/shaderCode.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/glf/diagnostic.h"

#include <limits>

PXR_NAMESPACE_OPEN_SCOPE

HdExtCompGpuComputationBufferSource::HdExtCompGpuComputationBufferSource(
        SdfPath const &id,
        TfToken const &primvarName,
        HdBufferSourceVector const &inputs,
        int numElements,
        HdExtCompGpuComputationResourceSharedPtr const &resource)
 : HdNullBufferSource()
 , _id(id)
 , _primvarName(primvarName)
 , _inputs(inputs)
 , _numElements(numElements)
 , _resource(resource)
{
}

bool
HdExtCompGpuComputationBufferSource::Resolve()
{
    bool allResolved = true;
    for (size_t i = 0; i < _inputs.size(); ++i) {
        HdBufferSourceSharedPtr const &source = _inputs[i];
        if (!source->IsResolved()) {
            allResolved &= source->Resolve();
        }
    }

    if (!allResolved) {
        return false;
    }

    if (!_TryLock()) {
        return false;
    }
    
    // Resolve the code gen source code
    _resource->Resolve();
    
    _SetResolved();
    
    return true;
}

bool
HdExtCompGpuComputationBufferSource::_CheckValid() const
{
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
