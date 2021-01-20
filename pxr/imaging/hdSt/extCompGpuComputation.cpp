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
#include "pxr/imaging/hdSt/bufferArrayRange.h"
#include "pxr/imaging/hdSt/bufferResource.h"
#include "pxr/imaging/hdSt/extCompGpuComputationBufferSource.h"
#include "pxr/imaging/hdSt/extCompGpuPrimvarBufferSource.h"
#include "pxr/imaging/hdSt/extCompGpuComputation.h"
#include "pxr/imaging/hdSt/extComputation.h"
#include "pxr/imaging/hdSt/glslProgram.h"
#include "pxr/imaging/hdSt/resourceRegistry.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/extCompPrimvarBufferSource.h"
#include "pxr/imaging/hd/extCompCpuComputation.h"
#include "pxr/imaging/hd/sceneExtCompInputSource.h"
#include "pxr/imaging/hd/compExtCompInputSource.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "pxr/imaging/hgi/hgi.h"
#include "pxr/imaging/hgi/computeCmds.h"
#include "pxr/imaging/hgi/computePipeline.h"
#include "pxr/imaging/hgi/shaderProgram.h"
#include "pxr/imaging/hgi/tokens.h"
#include "pxr/imaging/glf/diagnostic.h"
#include "pxr/base/tf/hash.h"

#include <limits>

PXR_NAMESPACE_OPEN_SCOPE

static void
_AppendResourceBindings(
    HgiResourceBindingsDesc* resourceDesc,
    HgiBufferHandle const& buffer,
    uint32_t location)
{
    HgiBufferBindDesc bufBind;
    bufBind.bindingIndex = location;
    bufBind.resourceType = HgiBindResourceTypeStorageBuffer;
    bufBind.stageUsage = HgiShaderStageCompute;
    bufBind.offsets.push_back(0);
    bufBind.buffers.push_back(buffer);
    resourceDesc->buffers.push_back(std::move(bufBind));
}

static HgiComputePipelineSharedPtr
_CreatePipeline(
    Hgi* hgi,
    uint32_t constantValuesSize,
    HgiShaderProgramHandle const& program)
{
    HgiComputePipelineDesc desc;
    desc.debugName = "ExtComputation";
    desc.shaderProgram = program;
    desc.shaderConstantsDesc.byteSize = constantValuesSize;
    return std::make_shared<HgiComputePipelineHandle>(
        hgi->CreateComputePipeline(desc));
}

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

    HdStResourceRegistry* hdStResourceRegistry =
        static_cast<HdStResourceRegistry*>(resourceRegistry);
    HdStGLSLProgramSharedPtr const &computeProgram = _resource->GetProgram();
    HdSt_ResourceBinder const &binder = _resource->GetResourceBinder();

    if (!TF_VERIFY(computeProgram)) {
        return;
    }

    HdStBufferArrayRangeSharedPtr outputBar =
        std::static_pointer_cast<HdStBufferArrayRange>(outputRange);
    TF_VERIFY(outputBar);

    // Prepare uniform buffer for GPU computation
    // XXX: We'd really prefer to delegate this to the resource binder.
    std::vector<int32_t> _uniforms;
    _uniforms.push_back(outputBar->GetElementOffset());

    // Generate hash for resource bindings and pipeline.
    // XXX Needs fingerprint hash to avoid collisions
    uint64_t rbHash = 0;

    // Bind buffers as SSBOs to the indices matching the layout in the shader
    for (HdExtComputationPrimvarDescriptor const &compPrimvar: _compPrimvars) {
        TfToken const & name = compPrimvar.sourceComputationOutputName;
        HdStBufferResourceSharedPtr const & buffer =
                outputBar->GetResource(compPrimvar.name);

        HdBinding const &binding = binder.GetBinding(name);
        // These should all be valid as they are required outputs
        if (TF_VERIFY(binding.IsValid()) && TF_VERIFY(buffer->GetId())) {
            size_t componentSize = HdDataSizeOfType(
                HdGetComponentType(buffer->GetTupleType().type));
            _uniforms.push_back(buffer->GetOffset() / componentSize);
            // Assumes non-SSBO allocator for the stride
            _uniforms.push_back(buffer->GetStride() / componentSize);

            rbHash = TfHash::Combine(rbHash, buffer->GetId().Get());
        }
    }

    for (HdBufferArrayRangeSharedPtr const & input: _resource->GetInputs()) {
        HdStBufferArrayRangeSharedPtr const & inputBar =
            std::static_pointer_cast<HdStBufferArrayRange>(input);

        for (HdStBufferResourceNamedPair const & it:
                        inputBar->GetResources()) {
            TfToken const &name = it.first;
            HdStBufferResourceSharedPtr const &buffer = it.second;

            HdBinding const &binding = binder.GetBinding(name);
            // These should all be valid as they are required inputs
            if (TF_VERIFY(binding.IsValid())) {
                HdTupleType tupleType = buffer->GetTupleType();
                size_t componentSize =
                    HdDataSizeOfType(HdGetComponentType(tupleType.type));
                _uniforms.push_back( (inputBar->GetByteOffset(name) + 
                    buffer->GetOffset()) / componentSize);
                // If allocated with a VBO allocator use the line below instead.
                //_uniforms.push_back(
                //    buffer->GetStride() / buffer->GetComponentSize());
                // This is correct for the SSBO allocator only
                _uniforms.push_back(HdGetComponentCount(tupleType.type));

                if (binding.GetType() != HdBinding::SSBO) {
                    TF_RUNTIME_ERROR(
                        "Unsupported binding type %d for ExtComputation",
                        binding.GetType());
                }

                rbHash = TfHash::Combine(rbHash, buffer->GetId().Get());
            }
        }
    }

    Hgi* hgi = hdStResourceRegistry->GetHgi();

    // Prepare uniform buffer for GPU computation
    const size_t uboSize = sizeof(int32_t) * _uniforms.size();
    uint64_t pHash = (uint64_t) TfHash::Combine(
        computeProgram->GetProgram().Get(),
        uboSize);

    // Get or add pipeline in registry.
    HdInstance<HgiComputePipelineSharedPtr> computePipelineInstance =
        hdStResourceRegistry->RegisterComputePipeline(pHash);
    if (computePipelineInstance.IsFirstInstance()) {
        HgiComputePipelineSharedPtr pipe = _CreatePipeline(
            hgi, uboSize, computeProgram->GetProgram());
        computePipelineInstance.SetValue(pipe);
    }

    HgiComputePipelineSharedPtr const& pipelinePtr =
        computePipelineInstance.GetValue();
    HgiComputePipelineHandle pipeline = *pipelinePtr.get();

    // Get or add resource bindings in registry.
    HdInstance<HgiResourceBindingsSharedPtr> resourceBindingsInstance =
        hdStResourceRegistry->RegisterResourceBindings(rbHash);
    if (resourceBindingsInstance.IsFirstInstance()) {
        // Begin the resource set
        HgiResourceBindingsDesc resourceDesc;
        resourceDesc.debugName = "ExtComputation";

        for (HdExtComputationPrimvarDescriptor const& compPvar: _compPrimvars) {
            TfToken const & name = compPvar.sourceComputationOutputName;
            HdStBufferResourceSharedPtr const & buffer =
                    outputBar->GetResource(compPvar.name);

            HdBinding const &binding = binder.GetBinding(name);
            // These should all be valid as they are required outputs
            if (TF_VERIFY(binding.IsValid()) && TF_VERIFY(buffer->GetId())) {
                _AppendResourceBindings(
                    &resourceDesc, buffer->GetId(), binding.GetLocation());
            }
        }

        for (HdBufferArrayRangeSharedPtr const & input: _resource->GetInputs()){
            HdStBufferArrayRangeSharedPtr const & inputBar =
                std::static_pointer_cast<HdStBufferArrayRange>(input);

            for (HdStBufferResourceNamedPair const & it:
                            inputBar->GetResources()) {
                TfToken const &name = it.first;
                HdStBufferResourceSharedPtr const &buffer = it.second;

                HdBinding const &binding = binder.GetBinding(name);
                // These should all be valid as they are required inputs
                if (TF_VERIFY(binding.IsValid())) {
                    _AppendResourceBindings(
                        &resourceDesc, buffer->GetId(), binding.GetLocation());
                }
            }
        }

        HgiResourceBindingsSharedPtr rb =
            std::make_shared<HgiResourceBindingsHandle>(
                hgi->CreateResourceBindings(resourceDesc));
        
        resourceBindingsInstance.SetValue(rb);
    }

    HgiResourceBindingsSharedPtr const& resourceBindindsPtr =
        resourceBindingsInstance.GetValue();
    HgiResourceBindingsHandle resourceBindings = *resourceBindindsPtr.get();

    HgiComputeCmds* computeCmds = hdStResourceRegistry->GetGlobalComputeCmds();

    computeCmds->PushDebugGroup("ExtComputation");
    computeCmds->BindResources(resourceBindings);
    computeCmds->BindPipeline(pipeline);

    // Queue transfer uniform buffer
    computeCmds->SetConstantValues(pipeline, 0, uboSize, &_uniforms[0]);

    // Queue compute work
    computeCmds->Dispatch(GetDispatchCount(), 1);

    computeCmds->PopDebugGroup();
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
        std::dynamic_pointer_cast<HdStResourceRegistry>(
                              renderIndex.GetResourceRegistry());

    HdSt_ExtCompComputeShaderSharedPtr shader = 
        std::make_shared<HdSt_ExtCompComputeShader>(sourceComp);

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
    HdExtComputationPrimvarDescriptorVector const& allCompPrimvars,
    HdDirtyBits dirtyBits,
    HdBufferSourceSharedPtrVector *sources,
    HdBufferSourceSharedPtrVector *reserveOnlySources,
    HdBufferSourceSharedPtrVector *separateComputationSources,
    HdStComputationSharedPtrVector *computations)
{
    TF_VERIFY(sources);
    TF_VERIFY(reserveOnlySources);
    TF_VERIFY(separateComputationSources);
    TF_VERIFY(computations);

    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();

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

        if (!sourceComp->GetGpuKernelSource().empty()) {

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
                                    HdBufferSourceSharedPtrVector(),
                                    gpuComputation->GetResource()));

                        separateComputationSources->push_back(
                                                        gpuComputationSource);
                        // Assume there are no dependencies between ExtComp so
                        // put all of them in queue zero.
                        computations->emplace_back(
                            gpuComputation, HdStComputeQueueZero);
                    }

                    // Create a primvar buffer source for the computation
                    HdBufferSourceSharedPtr primvarBufferSource(
                            new HdStExtCompGpuPrimvarBufferSource(
                                compPrimvar.name,
                                compPrimvar.valueType,
                                sourceComp->GetElementCount(),
                                sourceComp->GetId()));

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
