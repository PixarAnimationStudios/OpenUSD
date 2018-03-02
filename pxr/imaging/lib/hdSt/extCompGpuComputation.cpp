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

#include "pxr/imaging/hdSt/bufferArrayRangeGL.h"
#include "pxr/imaging/hdSt/bufferResourceGL.h"
#include "pxr/imaging/hdSt/extCompGpuComputationBufferSource.h"
#include "pxr/imaging/hdSt/extCompGpuPrimvarBufferSource.h"
#include "pxr/imaging/hdSt/extCompGpuComputation.h"
#include "pxr/imaging/hdSt/glslProgram.h"
#include "pxr/imaging/hdSt/glUtils.h"
#include "pxr/imaging/hdSt/renderContextCaps.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/extCompPrimvarBufferSource.h"
#include "pxr/imaging/hd/extCompCpuComputation.h"
#include "pxr/imaging/hd/sceneExtCompInputSource.h"
#include "pxr/imaging/hd/compExtCompInputSource.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/glf/diagnostic.h"

#include <limits>

PXR_NAMESPACE_OPEN_SCOPE


HdStExtCompGpuComputation::HdStExtCompGpuComputation(
        SdfPath const &id,
        HdStExtCompGpuComputationResourceSharedPtr const &resource,
        TfToken const &primvarName,
        TfToken const &computationOutputName,
        int numElements)
 : HdComputation()
 , _id(id)
 , _resource(resource)
 , _primvarName(primvarName)
 , _computationOutputName(computationOutputName)
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
            _id.GetText(), _primvarName.GetText());

    if (!glDispatchCompute) {
        TF_WARN("glDispatchCompute not available");
        return;
    }

    HdStBufferArrayRangeGLSharedPtr range =
        boost::static_pointer_cast<HdStBufferArrayRangeGL>(range_);

    TF_VERIFY(range);
    // XXX Currently these computations are always meant to be 1:1 to the
    // output range. If that changes in the future we'll need to design some
    // form of expansion or windowed computation extension to this.

    // Chained computations can expand, but we don't support contraction yet.
    TF_VERIFY(range->GetNumElements() >= GetNumOutputElements());
    HdStBufferResourceGLNamedList const &resources = range->GetResources();

    // Non-in-place sources should have been registered as resource registry
    // sources already and Resolved. They go to an internal buffer range that
    // was allocated in AllocateInternalBuffers
    HdStBufferArrayRangeGLSharedPtr inputRange;
    HdStBufferResourceGLNamedList inputResources;
    if (_resource->GetInternalRange()) {
        inputRange = boost::static_pointer_cast<HdStBufferArrayRangeGL>(
                _resource->GetInternalRange());
        inputResources = inputRange->GetResources();
    }

    HdStGLSLProgramSharedPtr const &computeProgram = _resource->GetProgram();
    HdSt_ResourceBinder const &binder = _resource->GetResourceBinder();

    if (!TF_VERIFY(computeProgram)) {
        return;
    }

    GLuint kernel = computeProgram->GetProgram().GetId();
    glUseProgram(kernel);

    HdStBufferResourceGLSharedPtr outBuffer = range->GetResource(
        _primvarName);
    TF_VERIFY(outBuffer);
    TF_VERIFY(outBuffer->GetId());

    // Prepare uniform buffer for GPU computation
    _uniforms.clear();
    _uniforms.push_back(range->GetOffset());
    // Bind buffers as SSBOs to the indices matching the layout in the shader
    TF_FOR_ALL(it, resources) {
        TfToken name = (*it).first;

        // Map the output onto the destination primvar
        if (name == _primvarName) {
            name = _computationOutputName;
        }
        HdBinding const &binding = binder.GetBinding(name);
        // XXX we need a better way than this to pick
        // which buffers to bind on the output.
        // No guarantee that we are hiding buffers that
        // shouldn't be written to for example.
        if (binding.IsValid()) {
            HdStBufferResourceGLSharedPtr const &buffer = (*it).second;
            size_t componentSize = HdDataSizeOfType(
                HdGetComponentType(buffer->GetTupleType().type));
            _uniforms.push_back(buffer->GetOffset() / componentSize);
            // Assumes non-SSBO allocator for the stride
            _uniforms.push_back(buffer->GetStride() / componentSize);
            binder.BindBuffer(name, buffer);
        } 
    }
    TF_FOR_ALL(it, inputResources) {
        TfToken const &name = (*it).first;
        HdBinding const &binding = binder.GetBinding(name);
        // These should all be valid as they are required inputs
        if (TF_VERIFY(binding.IsValid())) {
            HdStBufferResourceGLSharedPtr const &buffer = (*it).second;
            HdTupleType tupleType = buffer->GetTupleType();
            size_t componentSize =
                HdDataSizeOfType(HdGetComponentType(tupleType.type));
            _uniforms.push_back((inputRange->GetOffset() + buffer->GetOffset()) / componentSize);
            // If allocated with a VBO allocator use the line below instead.
            //_uniforms.push_back(buffer->GetStride() / buffer->GetComponentSize());
            // This is correct for the SSBO allocator only
            _uniforms.push_back(HdGetComponentCount(tupleType.type));
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
            HdStBufferResourceGLSharedPtr const &buffer = (*it).second;
            binder.UnbindBuffer(name, buffer);
        } 
    }
    TF_FOR_ALL(it, inputResources) {
        TfToken const &name = (*it).first;
        HdBinding const &binding = binder.GetBinding(name);
        // These should all be valid as they are required inputs
        if (TF_VERIFY(binding.IsValid())) {
            HdStBufferResourceGLSharedPtr const &buffer = (*it).second;
            binder.UnbindBuffer(name, buffer);
        }
    }

    glUseProgram(0);
}

void
HdStExtCompGpuComputation::AddBufferSpecs(HdBufferSpecVector *specs) const
{
    // nothing
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

/* static */
HdStExtCompGpuComputationSharedPtr
HdStExtCompGpuComputation::CreateGpuComputation(
    HdSceneDelegate *sceneDelegate,
    HdExtComputation const *sourceComp,
    TfToken const &computationOutputName,
    HdBufferSourceSharedPtr const &primvar)
{
    TF_DEBUG(HD_EXT_COMPUTATION_UPDATED).Msg(
            "GPU computation '%s' created for primvar '%s'\n",
            sourceComp->GetId().GetText(), primvar->GetName().GetText());

    // Downcast the resource registry
    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        boost::dynamic_pointer_cast<HdStResourceRegistry>(
                              renderIndex.GetResourceRegistry());

    HdStComputeShaderSharedPtr shader(new HdStComputeShader());
    shader->SetComputeSource(sourceComp->GetKernel());

    // Map the output onto the destination primvar type
    HdBufferSpecVector outputBufferSpecs = {
        { computationOutputName, primvar->GetTupleType() }
    };

    // There is a companion resource that requires allocation
    // and resolution.
    HdStExtCompGpuComputationResourceSharedPtr resource(
            new HdStExtCompGpuComputationResource(
                outputBufferSpecs,
                shader,
                resourceRegistry));

    return HdStExtCompGpuComputationSharedPtr(
                new HdStExtCompGpuComputation(
                        sourceComp->GetId(),
                        resource,
                        primvar->GetName(),
                        computationOutputName,
                        sourceComp->GetElementCount()));
}

static
HdStExtCompGpuComputationBufferSourceSharedPtr
_CreateGpuComputationBufferSource(
    HdSceneDelegate *sceneDelegate,
    HdExtComputation const *sourceComp,
    HdStExtCompGpuComputationResourceSharedPtr const &resource)
{
    // Downcast the resource registry
    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        boost::dynamic_pointer_cast<HdStResourceRegistry>(
                              renderIndex.GetResourceRegistry());

    HdBufferSourceVector inputs;
    for (const TfToken &inputName: sourceComp->GetSceneInputs()) {
        VtValue inputValue = sceneDelegate->Get(sourceComp->GetId(), inputName);
        size_t arraySize =
            inputValue.IsArrayValued() ? inputValue.GetArraySize() : 1;
        HdBufferSourceSharedPtr inputSource = HdBufferSourceSharedPtr(
                    new HdVtBufferSource(inputName, inputValue, arraySize));
        inputs.push_back(inputSource);
    }

    // This allocates a range suitable for the computation
    // if one is needed. If one is not needed the
    // internalSources will be empty.
    HdBufferSourceVector internalSources;
    resource->AllocateInternalRange(
            inputs,
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

    return HdStExtCompGpuComputationBufferSourceSharedPtr(
                new HdStExtCompGpuComputationBufferSource(
                        inputs, resource));
}

void
HdSt_GetExtComputationPrimVarsComputations(
    SdfPath const &id,
    HdSceneDelegate *sceneDelegate,
    HdInterpolation interpolationMode,
    HdDirtyBits dirtyBits,
    HdBufferSourceVector *sources,
    HdBufferSourceVector *reserveOnlySources,
    HdBufferSourceVector *separateComputationSources,
    HdComputationVector *computations)
{
    TF_VERIFY(sources);
    TF_VERIFY(reserveOnlySources);
    TF_VERIFY(separateComputationSources);
    TF_VERIFY(computations);

    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
    TfTokenVector compPrimVars =
        sceneDelegate->GetExtComputationPrimVarNames(id, interpolationMode);

    TF_FOR_ALL(compPrimVarIt, compPrimVars) {
        TfToken const &compPrimVarName =  *compPrimVarIt;

        if (HdChangeTracker::IsPrimVarDirty(dirtyBits, id, compPrimVarName)) {
            HdExtComputationPrimVarDesc primVarDesc =
                sceneDelegate->GetExtComputationPrimVarDesc(id, compPrimVarName);

            HdExtComputation *sourceComp;
            HdSceneDelegate *sourceCompSceneDelegate;

            renderIndex.GetExtComputationInfo(primVarDesc.computationId,
                                              &sourceComp,
                                              &sourceCompSceneDelegate);

            if (sourceComp != nullptr) {
                
                if (HdStRenderContextCaps::GetInstance().gpuComputeEnabled &&
                    !sourceComp->GetKernel().empty()) {

                    HdBufferSourceSharedPtr primVarBufferSource(
                            new HdStExtCompGpuPrimvarBufferSource(
                                compPrimVarName,
                                primVarDesc.defaultValue,
                                sourceComp->GetElementCount()));

                    HdStExtCompGpuComputationSharedPtr gpuComputation = 
                        HdStExtCompGpuComputation::CreateGpuComputation(
                            sourceCompSceneDelegate,
                            sourceComp,
                            primVarDesc.computationOutputName,
                            primVarBufferSource);

                    HdBufferSourceSharedPtr gpuComputationSource = 
                        _CreateGpuComputationBufferSource(
                            sourceCompSceneDelegate,
                            sourceComp,
                            gpuComputation->GetResource());

                    reserveOnlySources->push_back(primVarBufferSource);
                    separateComputationSources->push_back(gpuComputationSource);
                    computations->push_back(gpuComputation);

                } else {

                    HdExtCompCpuComputationSharedPtr cpuComputation =
                        HdExtCompCpuComputation::CreateComputation(
                            sourceCompSceneDelegate,
                            *sourceComp,
                            separateComputationSources);

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
