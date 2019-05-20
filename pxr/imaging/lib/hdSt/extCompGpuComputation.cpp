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
        HdExtComputationPrimvarDescriptorVector const &compPrimvars,
        int dispatchCount,
        int elementCount)
 : HdComputation()
 , _id(id)
 , _resource(resource)
 , _compPrimvars(compPrimvars)
 , _dispatchCount(dispatchCount)
 , _elementCount(elementCount)
{
    
}

static std::string
_GetDebugPrimvarNames(
        HdExtComputationPrimvarDescriptorVector const & compPrimvars)
{
    std::string result;
    for (HdExtComputationPrimvarDescriptor const & compPrimvar: compPrimvars) {
        result += " '";
        result += compPrimvar.name;
        result += "'";
    }
    return result;
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
            "GPU computation '%s' executed for primvars: %s\n",
            _id.GetText(), _GetDebugPrimvarNames(_compPrimvars).c_str());

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

    // Prepare uniform buffer for GPU computation
    // XXX: We'd really prefer to delegate this to the resource binder.
    std::vector<int32_t> _uniforms;
    _uniforms.push_back(outputBar->GetOffset());

    // Bind buffers as SSBOs to the indices matching the layout in the shader
    for (HdExtComputationPrimvarDescriptor const &compPrimvar: _compPrimvars) {
        TfToken const & name = compPrimvar.sourceComputationOutputName;
        HdStBufferResourceGLSharedPtr const & buffer =
                outputBar->GetResource(compPrimvar.name);

        HdBinding const &binding = binder.GetBinding(name);
        // These should all be valid as they are required outputs
        if (TF_VERIFY(binding.IsValid()) && TF_VERIFY(buffer->GetId())) {
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
                _uniforms.push_back(
                 (inputBar->GetOffset() + buffer->GetOffset()) / componentSize);
                // If allocated with a VBO allocator use the line below instead.
                //_uniforms.push_back(
                //    buffer->GetStride() / buffer->GetComponentSize());
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
HdStExtCompGpuComputation::GetBufferSpecs(HdBufferSpecVector *specs) const
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
    HdExtComputationPrimvarDescriptorVector const &compPrimvars)
{
    TF_DEBUG(HD_EXT_COMPUTATION_UPDATED).Msg(
            "GPU computation '%s' created for primvars: %s\n",
            sourceComp->GetId().GetText(),
            _GetDebugPrimvarNames(compPrimvars).c_str());

    // Downcast the resource registry
    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
    HdStResourceRegistrySharedPtr const& resourceRegistry = 
        boost::dynamic_pointer_cast<HdStResourceRegistry>(
                              renderIndex.GetResourceRegistry());

    HdStComputeShaderSharedPtr shader(new HdStComputeShader());
    shader->SetComputeSource(sourceComp->GetGpuKernelSource());

    // Map the computation outputs onto the destination primvar types
    HdBufferSpecVector outputBufferSpecs;
    outputBufferSpecs.reserve(compPrimvars.size());
    for (HdExtComputationPrimvarDescriptor const &compPrimvar: compPrimvars) {
        outputBufferSpecs.emplace_back(compPrimvar.sourceComputationOutputName,
                                       compPrimvar.valueType);
    }

    HdStExtComputation const *deviceSourceComp =
        static_cast<HdStExtComputation const *>(sourceComp);
    if (!TF_VERIFY(deviceSourceComp)) {
        return nullptr;
    }
    HdBufferArrayRangeSharedPtrVector inputs;
    inputs.push_back(deviceSourceComp->GetInputRange());

    for (HdExtComputationInputDescriptor const &desc:
         sourceComp->GetComputationInputs()) {
        HdStExtComputation const * deviceInputComp =
            static_cast<HdStExtComputation const *>(
                renderIndex.GetSprim(
                    HdPrimTypeTokens->extComputation,
                    desc.sourceComputationId));
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
                        sourceComp->GetId(),
                        resource,
                        compPrimvars,
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

    HdExtComputationPrimvarDescriptorVector allCompPrimvars =
            sceneDelegate->GetExtComputationPrimvarDescriptors(
                        id, interpolationMode);

    // Group computation primvars by source computation
    typedef std::map<SdfPath, HdExtComputationPrimvarDescriptorVector>
                                                    CompPrimvarsByComputation;
    CompPrimvarsByComputation byComputation;
    for (HdExtComputationPrimvarDescriptor const & compPrimvar:
                                                        allCompPrimvars) {
        byComputation[compPrimvar.sourceComputationId].push_back(compPrimvar);
    }

    // Create computation primvar buffer sources by source computation
    for (CompPrimvarsByComputation::value_type it: byComputation) { 
        SdfPath const &computationId = it.first;
        HdExtComputationPrimvarDescriptorVector const &compPrimvars = it.second;

        HdExtComputation const * sourceComp =
            static_cast<HdExtComputation const *>(
                renderIndex.GetSprim(HdPrimTypeTokens->extComputation,
                                     computationId));

        if (!(sourceComp && sourceComp->GetElementCount() > 0)) {
            continue;
        }

        if (HdStGLUtils::IsGpuComputeEnabled() &&
            !sourceComp->GetGpuKernelSource().empty()) {

            HdStExtCompGpuComputationSharedPtr gpuComputation;
            for (HdExtComputationPrimvarDescriptor const & compPrimvar:
                                                                compPrimvars) {

                if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id,
                                                    compPrimvar.name)) {

                    if (!gpuComputation) {
                       // Create the computation for the first dirty primvar
                        gpuComputation =
                            HdStExtCompGpuComputation::CreateGpuComputation(
                                sceneDelegate,
                                sourceComp,
                                compPrimvars);

                        HdBufferSourceSharedPtr gpuComputationSource(
                                new HdStExtCompGpuComputationBufferSource(
                                    HdBufferSourceVector(),
                                    gpuComputation->GetResource()));

                        separateComputationSources->push_back(
                                                        gpuComputationSource);
                        computations->push_back(gpuComputation);
                    }

                    // Create a primvar buffer source for the computation
                    HdBufferSourceSharedPtr primvarBufferSource(
                            new HdStExtCompGpuPrimvarBufferSource(
                                compPrimvar.name,
                                compPrimvar.valueType,
                                sourceComp->GetElementCount()));

                    // Gpu primvar sources only need to reserve space
                    reserveOnlySources->push_back(primvarBufferSource);
                }
            }

        } else {

            HdExtCompCpuComputationSharedPtr cpuComputation;
            for (HdExtComputationPrimvarDescriptor const & compPrimvar:
                                                                compPrimvars) {

                if (HdChangeTracker::IsPrimvarDirty(dirtyBits, id,
                                                    compPrimvar.name)) {

                    if (!cpuComputation) {
                       // Create the computation for the first dirty primvar
                        cpuComputation =
                            HdExtCompCpuComputation::CreateComputation(
                                sceneDelegate,
                                *sourceComp,
                                separateComputationSources);

                    }

                    // Create a primvar buffer source for the computation
                    HdBufferSourceSharedPtr primvarBufferSource(
                            new HdExtCompPrimvarBufferSource(
                                compPrimvar.name,
                                cpuComputation,
                                compPrimvar.sourceComputationOutputName,
                                compPrimvar.valueType));

                    // Cpu primvar sources need to allocate and commit data
                    sources->push_back(primvarBufferSource);
                }
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
