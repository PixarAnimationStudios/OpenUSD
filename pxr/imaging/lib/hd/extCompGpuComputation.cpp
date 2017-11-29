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

#include "pxr/imaging/hd/extCompGpuComputation.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include "pxr/imaging/hd/bufferArrayRangeGL.h"
#include "pxr/imaging/hd/bufferResourceGL.h"
#include "pxr/imaging/hd/glslProgram.h"
#include "pxr/imaging/hd/glUtils.h"
#include "pxr/imaging/hd/tokens.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "extCompGpuComputation.h"
#include "extCompGpuComputationBufferSource.h"

#include <limits>

PXR_NAMESPACE_OPEN_SCOPE


HdExtCompGpuComputation::HdExtCompGpuComputation(
        SdfPath const &id,
        HdExtCompGpuComputationResourceSharedPtr const &resource,
        TfToken const &dstName,
        // XXX used for mapping kernel name to primvar name if needed
        HdBufferSpecVector const &outputBufferSpecs,
        int numElements)
 : HdComputation()
 , _id(id)
 , _resource(resource)
 , _dstName(dstName)
 , _outputSpecs(outputBufferSpecs)
 , _numElements(numElements)
 , _uniforms()
{
    
}

void
HdExtCompGpuComputation::Execute(
    HdBufferArrayRangeSharedPtr const &range_,
    HdResourceRegistry *resourceRegistry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TF_VERIFY(range_);
    TF_VERIFY(resourceRegistry);

    TF_DEBUG(HD_EXT_COMPUTATION_UPDATED).Msg(
            "GPU computation '%s' executed for primvar '%s'\n",
            _id.GetText(), _dstName.GetText());

    if (!glDispatchCompute) {
        TF_WARN("glDispatchCompute not available");
        return;
    }

    HdBufferArrayRangeGLSharedPtr range =
        boost::static_pointer_cast<HdBufferArrayRangeGL>(range_);

    TF_VERIFY(range);
    // XXX Currently these computations are always meant to be 1:1 to the
    // output range. If that changes in the future we'll need to design some
    // form of expansion or windowed computation extension to this.
    TF_VERIFY(range->GetNumElements() == GetNumOutputElements());
    HdBufferResourceGLNamedList const &resources = range->GetResources();

    // Non-in-place sources should have been registered as resource registry
    // sources already and Resolved. They go to an internal buffer range that
    // was allocated in AllocateInternalBuffers
    HdBufferArrayRangeGLSharedPtr inputRange;
    HdBufferResourceGLNamedList inputResources;
    if (_resource->GetInternalRange()) {
        inputRange = boost::static_pointer_cast<HdBufferArrayRangeGL>(
                _resource->GetInternalRange());
        inputResources = inputRange->GetResources();
    }

    HdGLSLProgramSharedPtr const &computeProgram = _resource->GetProgram();
    Hd_ResourceBinder const &binder = _resource->GetResourceBinder();

    if (!TF_VERIFY(computeProgram)) {
        return;
    }
    
    GLuint kernel = computeProgram->GetProgram().GetId();
    glUseProgram(kernel);

    HdBufferResourceGLSharedPtr outBuffer = range->GetResource(
        _dstName);
    TF_VERIFY(outBuffer);
    TF_VERIFY(outBuffer->GetId());

    // Prepare uniform buffer for GPU computation
    _uniforms.clear();
    _uniforms.push_back(range->GetOffset());
    // Bind buffers as SSBOs to the indices matching the layout in the shader
    TF_FOR_ALL(it, resources) {
        TfToken const &name = (*it).first;
        HdBinding const &binding = binder.GetBinding(name);
        // XXX we need a better way than this to pick
        // which buffers to bind on the output.
        // No guarantee that we are hiding buffers that
        // shouldn't be written to for example.
        if (binding.IsValid()) {
            HdBufferResourceGLSharedPtr const &buffer = (*it).second;
            _uniforms.push_back(buffer->GetOffset() / buffer->GetComponentSize());
            // Assumes non-SSBO allocator for the stride
            _uniforms.push_back(buffer->GetStride() / buffer->GetComponentSize());
            binder.BindBuffer(name, buffer);
        } 
    }
    TF_FOR_ALL(it, inputResources) {
        TfToken const &name = (*it).first;
        HdBinding const &binding = binder.GetBinding(name);
        // These should all be valid as they are required inputs
        if (TF_VERIFY(binding.IsValid())) {
            HdBufferResourceGLSharedPtr const &buffer = (*it).second;
            _uniforms.push_back((inputRange->GetOffset() + buffer->GetOffset()) /
                    buffer->GetComponentSize());
            // If allocated with a VBO allocator use the line below instead.
            //_uniforms.push_back(buffer->GetStride() / buffer->GetComponentSize());
            // This is correct for the SSBO allocator only
            _uniforms.push_back(buffer->GetNumComponents());
            binder.BindBuffer(name, buffer);
        }
    }
    
    // Prepare uniform buffer for GPU computation
    GLuint ubo = computeProgram->GetGlobalUniformBuffer().GetId();
    glBindBuffer(GL_UNIFORM_BUFFER, ubo);
    glBufferData(GL_UNIFORM_BUFFER,
            sizeof(int32_t) * _uniforms.size(),
            &_uniforms[0],
            GL_STATIC_DRAW);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);

    glBindBufferBase(GL_UNIFORM_BUFFER, 0, ubo);

    // The computation dimension is some thing we want to manage for
    // users. Right now it is just the size of the output buffer.
    glDispatchCompute((GLuint)GetNumOutputElements(), 1, 1);
    GLF_POST_PENDING_GL_ERRORS();

    // For now we make sure the computation finishes right away.
    // Figure out if sync or async is the way to go.
    // Assuming SSBOs for the output
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Unbind.
    // XXX this should go away once we use a graphics abstraction
    // as that would take care of cleaning state.
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, 0);
    TF_FOR_ALL(it, resources) {
        TfToken const &name = (*it).first;
        HdBinding const &binding = binder.GetBinding(name);
        // XXX we need a better way than this to pick
        // which buffers to bind on the output.
        // No guarantee that we are hiding buffers that
        // shouldn't be written to for example.
        if (binding.IsValid()) {
            HdBufferResourceGLSharedPtr const &buffer = (*it).second;
            binder.UnbindBuffer(name, buffer);
        } 
    }
    TF_FOR_ALL(it, inputResources) {
        TfToken const &name = (*it).first;
        HdBinding const &binding = binder.GetBinding(name);
        // These should all be valid as they are required inputs
        if (TF_VERIFY(binding.IsValid())) {
            HdBufferResourceGLSharedPtr const &buffer = (*it).second;
            binder.UnbindBuffer(name, buffer);
        }
    }

    glUseProgram(0);
}

void
HdExtCompGpuComputation::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    for (HdBufferSpec const &spec : _outputSpecs) {
        specs->push_back(spec);
    }
}

int
HdExtCompGpuComputation::GetNumOutputElements() const 
{
    return _numElements;
}

HdExtCompGpuComputationResourceSharedPtr const &
HdExtCompGpuComputation::GetResource() const
{
    return _resource;
}


PXR_NAMESPACE_CLOSE_SCOPE
