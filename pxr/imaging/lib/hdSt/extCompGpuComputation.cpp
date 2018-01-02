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

#include "pxr/imaging/hdSt/extCompGpuComputation.h"
#include "pxr/imaging/hdSt/extCompGpuComputationBufferSource.h"
#include "pxr/imaging/hdSt/glslProgram.h"
#include "pxr/imaging/hdSt/glUtils.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/bufferArrayRangeGL.h"
#include "pxr/imaging/hd/bufferResourceGL.h"
#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/extCompPrimvarBufferSource.h"
#include "pxr/imaging/hd/extCompCpuComputation.h"
#include "pxr/imaging/hd/renderContextCaps.h"
#include "pxr/imaging/hd/sceneExtCompInputSource.h"
#include "pxr/imaging/hd/compExtCompInputSource.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/hd/vtExtractor.h"
#include "pxr/imaging/glf/diagnostic.h"

#include <limits>

PXR_NAMESPACE_OPEN_SCOPE


HdStExtCompGpuComputation::HdStExtCompGpuComputation(
        SdfPath const &id,
        HdStExtCompGpuComputationResourceSharedPtr const &resource,
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
HdStExtCompGpuComputation::Execute(
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

    HdStGLSLProgramSharedPtr const &computeProgram = _resource->GetProgram();
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
HdStExtCompGpuComputation::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    for (HdBufferSpec const &spec : _outputSpecs) {
        specs->push_back(spec);
    }
}

int
HdStExtCompGpuComputation::GetNumOutputElements() const 
{
    return _numElements;
}

HdStExtCompGpuComputationResourceSharedPtr const &
HdStExtCompGpuComputation::GetResource() const
{
    return _resource;
}

std::pair<
    HdStExtCompGpuComputationSharedPtr,
    HdStExtCompGpuComputationBufferSourceSharedPtr>
HdStExtCompGpuComputation::CreateComputation(
    HdSceneDelegate *sceneDelegate,
    const HdExtComputation &computation,
    HdBufferSourceVector *computationSources,
    TfToken const &primvarName,
    HdBufferSpecVector const &outputBufferSpecs,
    HdBufferSpecVector const &primInputSpecs)
{
    TF_DEBUG(HD_EXT_COMPUTATION_UPDATED).Msg("_CreateGpuComputation\n");

    // Only return a GPU computation if there is a kernel bound.
    if (computation.GetKernel().empty()) {
        return std::make_pair(HdStExtCompGpuComputationSharedPtr(),
                              HdStExtCompGpuComputationBufferSourceSharedPtr());
    }

    const SdfPath &id = computation.GetId();
    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
    HdBufferSourceVector inputs;

    for (const TfToken &inputName: computation.GetSceneInputs()) {
        VtValue inputValue = sceneDelegate->Get(id, inputName);
        HdBufferSourceSharedPtr inputSource = HdBufferSourceSharedPtr(
                    new HdVtBufferSource(inputName, inputValue));
        inputs.push_back(inputSource);
    }

    HdStComputeShaderSharedPtr shader(new HdStComputeShader());
    shader->SetComputeSource(computation.GetKernel());

    // Downcast the resource registry
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        boost::dynamic_pointer_cast<HdStResourceRegistry>(
                              renderIndex.GetResourceRegistry());
    
    HdStExtCompGpuComputationResourceSharedPtr resource(
            new HdStExtCompGpuComputationResource(
                outputBufferSpecs,
                shader,
                resourceRegistry));
            
    HdStExtCompGpuComputationBufferSourceSharedPtr bufferSource(
            new HdStExtCompGpuComputationBufferSource(
                id,
                primvarName,
                inputs,
                computation.GetElementCount(),
                resource));

    HdStExtCompGpuComputationSharedPtr gpuComp(
            new HdStExtCompGpuComputation(id,
                                        resource,
                                        primvarName,
                                        outputBufferSpecs,
                                        computation.GetElementCount()));

    return std::make_pair(gpuComp, bufferSource);
}

void
HdSt_GetExtComputationPrimVarsComputations(
    const SdfPath &id,
    HdSceneDelegate *sceneDelegate,
    HdInterpolation interpolationMode,
    HdDirtyBits dirtyBits,
    HdBufferSourceVector *sources,
    HdComputationVector *computations,
    HdBufferSourceVector *computationSources)
{
    TF_VERIFY(sources);
    TF_VERIFY(computations);

    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
    TfTokenVector compPrimVars =
            sceneDelegate->GetExtComputationPrimVarNames(id, interpolationMode);

    // what are the input primvars on the prim,
    // let the computations know so they can manage creation of internal
    // ranges if needed.
    HdBufferSpecVector primBufferSpecs;
    if (compPrimVars.size() > 0) {
        // get the buffer specs
        TF_FOR_ALL(it, (*sources)) {
            (*it)->AddBufferSpecs(&primBufferSpecs);
        }
        TF_FOR_ALL(it, (*computations)) {
            (*it)->AddBufferSpecs(&primBufferSpecs);
        }
    }
    
    TF_FOR_ALL(compPrimVarIt, compPrimVars) {
        const TfToken &compPrimVarName =  *compPrimVarIt;

        if (HdChangeTracker::IsPrimVarDirty(dirtyBits, id, compPrimVarName)) {
            HdExtComputationPrimVarDesc primVarDesc =
                   sceneDelegate->GetExtComputationPrimVarDesc(id,
                                                               compPrimVarName);


            HdExtComputation *sourceComp;
            HdSceneDelegate *sourceCompSceneDelegate;

            renderIndex.GetExtComputationInfo(primVarDesc.computationId,
                                              &sourceComp,
                                              &sourceCompSceneDelegate);

            if (sourceComp != nullptr) {
                // combine the primvars as an output buffer specs
                HdBufferSpecVector outputBufferSpecs;
                {
                    Hd_VtExtractor extractor;
                    extractor.Extract(primVarDesc.defaultValue);
                    outputBufferSpecs.emplace_back(
                        primVarDesc.computationOutputName,
                        extractor.GetGLCompontentType(),
                        extractor.GetNumComponents(),
                        1);        
                }
                
                HdStExtCompGpuComputationSharedPtr gpuComputation;
                HdStExtCompGpuComputationBufferSourceSharedPtr gpuComputationSource;
                if (HdRenderContextCaps::GetInstance().gpuComputeEnabled) {
                    std::pair<HdStExtCompGpuComputationSharedPtr,
                              HdStExtCompGpuComputationBufferSourceSharedPtr> comp =
                        HdStExtCompGpuComputation::CreateComputation(
                            sourceCompSceneDelegate,
                            *sourceComp,
                            computationSources,
                            compPrimVarName,
                            outputBufferSpecs,
                            primBufferSpecs);
                    gpuComputation = comp.first;
                    gpuComputationSource = comp.second;
                }
                
                if (gpuComputation) {
                    HdComputationSharedPtr comp =
                        boost::static_pointer_cast<HdComputation>(
                            gpuComputation);
                    computations->push_back(comp);
                    // There is a companion resource that requires allocation
                    // and resolution.
                    // Query it for any internal buffer ranges needed.
                    HdStExtCompGpuComputationResourceSharedPtr resource =
                            gpuComputation->GetResource();
                    HdResourceRegistrySharedPtr const &resourceRegistry = 
                        renderIndex.GetResourceRegistry();
                    // This allocates a range suitable for the computation
                    // if one is needed. If one is not needed the
                    // internalSources will be empty.
                    HdBufferSourceVector internalSources;
                    resource->AllocateInternalRange(
                            gpuComputationSource->GetInputs(),
                            &internalSources,
                            resourceRegistry);
                    if (!internalSources.empty()) {
                        // Only add it if it is actually needed.
                        // Shortcut here if we are also primvar sharing
                        // as we may not want to actually add the range
                        // and the sources.
                        resourceRegistry->AddSources(
                            resource->GetInternalRange(),
                            internalSources);
                    }
                    computationSources->push_back(gpuComputationSource);
                    
                } else {
                    HdExtCompCpuComputationSharedPtr cpuComputation =
                        HdExtCompCpuComputation::CreateComputation(
                            sourceCompSceneDelegate,
                            *sourceComp,
                            computationSources);
                    HdBufferSourceSharedPtr primVarBufferSource(
                            new HdExtCompPrimvarBufferSource(
                                compPrimVarName,
                                cpuComputation,
                                primVarDesc.computationOutputName,
                                primVarDesc.defaultValue));

                    sources->push_back(primVarBufferSource);
                }
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
