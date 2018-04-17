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
#include "pxr/imaging/hdSt/extComputation.h"
#include "pxr/imaging/hdSt/glslProgram.h"
#include "pxr/imaging/hdSt/glUtils.h"
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
        int dispatchCount,
        int elementCount)
 : HdComputation()
 , _id(id)
 , _resource(resource)
 , _primvarName(primvarName)
 , _computationOutputName(computationOutputName)
 , _dispatchCount(dispatchCount)
 , _elementCount(elementCount)
{
    
}

void
HdStExtCompGpuComputation::Execute(
    HdBufferArrayRangeSharedPtr const &outputRange,
    HdResourceRegistry *resourceRegistry)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TF_VERIFY(outputRange);
    TF_VERIFY(resourceRegistry);

    TF_DEBUG(HD_EXT_COMPUTATION_UPDATED).Msg(
            "GPU computation '%s' executed for primvar '%s'\n",
            _id.GetText(), _primvarName.GetText());

    if (!glDispatchCompute) {
        TF_WARN("glDispatchCompute not available");
        return;
    }

    HdStGLSLProgramSharedPtr const &computeProgram = _resource->GetProgram();
    HdSt_ResourceBinder const &binder = _resource->GetResourceBinder();

    if (!TF_VERIFY(computeProgram)) {
        return;
    }

    GLuint kernel = computeProgram->GetProgram().GetId();
    glUseProgram(kernel);

    HdStBufferArrayRangeGLSharedPtr outputBar =
        boost::static_pointer_cast<HdStBufferArrayRangeGL>(outputRange);
    TF_VERIFY(outputBar);

    HdStBufferResourceGLSharedPtr outBuffer =
        outputBar->GetResource(_primvarName);
    if (!TF_VERIFY(outBuffer) || !TF_VERIFY(outBuffer->GetId())) {
        return;
    };

    // Prepare uniform buffer for GPU computation
    // XXX: We'd really prefer to delegate this to the resource binder.
    std::vector<int32_t> _uniforms;
    _uniforms.push_back(outputBar->GetOffset());

    // Bind buffers as SSBOs to the indices matching the layout in the shader
    for (HdStBufferResourceGLNamedPair const & it: outputBar->GetResources()) {
        TfToken name = it.first;
        HdStBufferResourceGLSharedPtr const &buffer = it.second;

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
            size_t componentSize = HdDataSizeOfType(
                HdGetComponentType(buffer->GetTupleType().type));
            _uniforms.push_back(buffer->GetOffset() / componentSize);
            // Assumes non-SSBO allocator for the stride
            _uniforms.push_back(buffer->GetStride() / componentSize);
            binder.BindBuffer(name, buffer);
        } 
    }

    for (HdBufferArrayRangeSharedPtr const & input: _resource->GetInputs()) {
        HdStBufferArrayRangeGLSharedPtr const & inputBar =
            boost::static_pointer_cast<HdStBufferArrayRangeGL>(input);

        for (HdStBufferResourceGLNamedPair const & it:
                        inputBar->GetResources()) {
            TfToken const &name = it.first;
            HdStBufferResourceGLSharedPtr const &buffer = it.second;

            HdBinding const &binding = binder.GetBinding(name);
            // These should all be valid as they are required inputs
            if (TF_VERIFY(binding.IsValid())) {
                HdTupleType tupleType = buffer->GetTupleType();
                size_t componentSize =
                    HdDataSizeOfType(HdGetComponentType(tupleType.type));
                _uniforms.push_back((inputBar->GetOffset() + buffer->GetOffset()) / componentSize);
                // If allocated with a VBO allocator use the line below instead.
                //_uniforms.push_back(buffer->GetStride() / buffer->GetComponentSize());
                // This is correct for the SSBO allocator only
                _uniforms.push_back(HdGetComponentCount(tupleType.type));
                binder.BindBuffer(name, buffer);
            }
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

    glDispatchCompute((GLuint)GetDispatchCount(), 1, 1);
    GLF_POST_PENDING_GL_ERRORS();

    // For now we make sure the computation finishes right away.
    // Figure out if sync or async is the way to go.
    // Assuming SSBOs for the output
    glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

    // Unbind.
    // XXX this should go away once we use a graphics abstraction
    // as that would take care of cleaning state.
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, 0);
    for (HdStBufferResourceGLNamedPair const & it: outputBar->GetResources()) {
        TfToken const &name = it.first;
        HdStBufferResourceGLSharedPtr const &buffer = it.second;

        HdBinding const &binding = binder.GetBinding(name);
        // XXX we need a better way than this to pick
        // which buffers to bind on the output.
        // No guarantee that we are hiding buffers that
        // shouldn't be written to for example.
        if (binding.IsValid()) {
            binder.UnbindBuffer(name, buffer);
        } 
    }
    for (HdBufferArrayRangeSharedPtr const & input: _resource->GetInputs()) {
        HdStBufferArrayRangeGLSharedPtr const & inputBar =
            boost::static_pointer_cast<HdStBufferArrayRangeGL>(input);

        for (HdStBufferResourceGLNamedPair const & it:
                        inputBar->GetResources()) {
            TfToken const &name = it.first;
            HdStBufferResourceGLSharedPtr const &buffer = it.second;

            HdBinding const &binding = binder.GetBinding(name);
            // These should all be valid as they are required inputs
            if (TF_VERIFY(binding.IsValid())) {
                binder.UnbindBuffer(name, buffer);
            }
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
HdStExtCompGpuComputation::GetDispatchCount() const
{
    return _dispatchCount;
}

int
HdStExtCompGpuComputation::GetNumOutputElements() const
{
    return _elementCount;
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
            sourceComp->GetID().GetText(), primvar->GetName().GetText());

    // Downcast the resource registry
    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        boost::dynamic_pointer_cast<HdStResourceRegistry>(
                              renderIndex.GetResourceRegistry());

    HdStComputeShaderSharedPtr shader(new HdStComputeShader());
    shader->SetComputeSource(sourceComp->GetGpuKernelSource());

    // Map the output onto the destination primvar type
    HdBufferSpecVector outputBufferSpecs = {
        { computationOutputName, primvar->GetTupleType() }
    };

    HdStExtComputation const *deviceSourceComp =
        static_cast<HdStExtComputation const *>(sourceComp);
    if (!TF_VERIFY(deviceSourceComp)) {
        return nullptr;
    }
    HdBufferArrayRangeSharedPtrVector inputs;
    inputs.push_back(deviceSourceComp->GetInputRange());

    for (HdExtComputation::SourceComputationDesc const &desc:
         sourceComp->GetComputationSourceDescs()) {
        HdStExtComputation const * deviceInputComp =
            static_cast<HdStExtComputation const *>(
                renderIndex.GetSprim(
                    HdPrimTypeTokens->extComputation,
                    desc.computationId));
        if (deviceInputComp && deviceInputComp->GetInputRange()) {
            HdBufferArrayRangeSharedPtr input =
                deviceInputComp->GetInputRange();
            // skip duplicate inputs
            if (std::find(inputs.begin(),
                          inputs.end(), input) == inputs.end()) {
                inputs.push_back(deviceInputComp->GetInputRange());
            }
        }
    }

    // There is a companion resource that requires allocation
    // and resolution.
    HdStExtCompGpuComputationResourceSharedPtr resource(
            new HdStExtCompGpuComputationResource(
                outputBufferSpecs,
                shader,
                inputs,
                resourceRegistry));

    return HdStExtCompGpuComputationSharedPtr(
                new HdStExtCompGpuComputation(
                        sourceComp->GetID(),
                        resource,
                        primvar->GetName(),
                        computationOutputName,
                        sourceComp->GetDispatchCount(),
                        sourceComp->GetElementCount()));
}

void
HdSt_GetExtComputationPrimvarsComputations(
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
    TfTokenVector compPrimvars =
        sceneDelegate->GetExtComputationPrimvarNames(id, interpolationMode);

    for (TfToken const & compPrimvarName: compPrimvars) {

        if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id, compPrimvarName)) {
            HdExtComputationPrimvarDesc primVarDesc =
                sceneDelegate->GetExtComputationPrimvarDesc(id,
                                                            compPrimvarName);

            HdExtComputation const * sourceComp =
                static_cast<HdExtComputation const *>(
                    renderIndex.GetSprim(HdPrimTypeTokens->extComputation,
                                         primVarDesc.computationId));

            if (sourceComp && sourceComp->GetElementCount() > 0) {
                
                if (HdStGLUtils::IsGpuComputeEnabled() &&
                    !sourceComp->GetGpuKernelSource().empty()) {

                    HdBufferSourceSharedPtr primVarBufferSource(
                            new HdStExtCompGpuPrimvarBufferSource(
                                compPrimvarName,
                                primVarDesc.defaultValue,
                                sourceComp->GetElementCount()));

                    HdStExtCompGpuComputationSharedPtr gpuComputation = 
                        HdStExtCompGpuComputation::CreateGpuComputation(
                            sceneDelegate,
                            sourceComp,
                            primVarDesc.computationOutputName,
                            primVarBufferSource);

                    HdBufferSourceSharedPtr gpuComputationSource(
                            new HdStExtCompGpuComputationBufferSource(
                                HdBufferSourceVector(),
                                gpuComputation->GetResource()));

                    reserveOnlySources->push_back(primVarBufferSource);
                    separateComputationSources->push_back(gpuComputationSource);
                    computations->push_back(gpuComputation);

                } else {

                    HdExtCompCpuComputationSharedPtr cpuComputation =
                        HdExtCompCpuComputation::CreateComputation(
                            sceneDelegate,
                            *sourceComp,
                            separateComputationSources);

                    HdBufferSourceSharedPtr primVarBufferSource(
                            new HdExtCompPrimvarBufferSource(
                                compPrimvarName,
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
