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

#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/extComputationContext.h"
#include "pxr/imaging/hd/compExtCompInputSource.h"
#include "pxr/imaging/hd/extCompCpuComputation.h"
#include "pxr/imaging/hd/extCompGpuComputationBufferSource.h"
#include "pxr/imaging/hd/extCompGpuComputation.h"
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sceneExtCompInputSource.h"
#include "pxr/imaging/hd/vtBufferSource.h"
#include "extCompGpuComputationBufferSource.h"

PXR_NAMESPACE_OPEN_SCOPE

HdExtComputation::HdExtComputation(SdfPath const &id)
 : _id(id)
 , _elementCount(0)
 , _sceneInputs()
 , _computationInputs()
 , _computationSourceDescs()
 , _outputs()
{
}

void
HdExtComputation::Sync(HdSceneDelegate *sceneDelegate,
                       HdDirtyBits     *dirtyBits)
{
    HD_TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();
    TF_DEBUG(HD_EXT_COMPUTATION_UPDATED).Msg("HdExtComputation::Sync\n");

    if (!TF_VERIFY(sceneDelegate != nullptr)) {
        return;
    }

    HdDirtyBits bits = *dirtyBits;

    if (bits & DirtyInputDesc) {
        TF_DEBUG(HD_EXT_COMPUTATION_UPDATED).Msg("    dirty inputs\n");
        
        _sceneInputs       = sceneDelegate->GetExtComputationInputNames(_id,
                                                HdExtComputationInputTypeScene);
        _computationInputs = sceneDelegate->GetExtComputationInputNames(_id,
                                          HdExtComputationInputTypeComputation);


        size_t numComputationInputs = _computationInputs.size();
        _computationSourceDescs.reserve(numComputationInputs);
        for (size_t inputNum = 0; inputNum < numComputationInputs; ++inputNum) {
            HdExtComputationInputParams params =
                    sceneDelegate->GetExtComputationInputParams(_id,
                                                  _computationInputs[inputNum]);

            if ((!params.sourceComputationId.IsEmpty()) &&
                (!params.computationOutputName.IsEmpty())) {
                _computationSourceDescs.emplace_back();
                SourceComputationDesc &source = _computationSourceDescs.back();
                source.computationId     = params.sourceComputationId;
                source.computationOutput = params.computationOutputName;
            }
        }

    }

    if (bits & DirtyOutputDesc) {
        _outputs = sceneDelegate->GetExtComputationOutputNames(_id);
    }

    if (bits & DirtyElementCount) {
        VtValue vtElementCount =
                                sceneDelegate->Get(_id, HdTokens->elementCount);
        _elementCount = vtElementCount.Get<size_t>();
    }

    if (bits & DirtyKernel) {
        _kernel = sceneDelegate->GetExtComputationKernel(_id);
        TF_DEBUG(HD_EXT_COMPUTATION_UPDATED).Msg("    _kernel = '%s'\n",
                _kernel.c_str());
        // XXX we should update any created GPU computations as well
        // with the new kernel if we want to provide a good editing flow.
    }

    *dirtyBits = Clean;
}


HdExtCompCpuComputationSharedPtr
HdExtComputation::GetComputation(
    HdSceneDelegate *sceneDelegate,
    HdBufferSourceVector *computationSources) const
{
    // XXX: To do: De-duplication
    return _CreateCpuComputation(sceneDelegate, computationSources);
}

std::pair<HdExtCompGpuComputationSharedPtr,
          HdExtCompGpuComputationBufferSourceSharedPtr>
HdExtComputation::GetGpuComputation(
        HdSceneDelegate *sceneDelegate,
        HdBufferSourceVector *computationSources,
        TfToken const &primvarName,
        HdBufferSpecVector const &outputSpecs,
        HdBufferSpecVector const &primInputSpecs) const
{
    // Only return a GPU computation if there is a kernel bound.
    if (_kernel.empty()) {
        return std::make_pair(HdExtCompGpuComputationSharedPtr(),
                              HdExtCompGpuComputationBufferSourceSharedPtr());
    }
    // XXX: To do: De-duplication
    return _CreateGpuComputation(
            sceneDelegate,
            computationSources,
            primvarName,
            outputSpecs,
            primInputSpecs);
}

HdDirtyBits
HdExtComputation::GetInitialDirtyBits() const
{
    return AllDirty;
}

HdExtCompCpuComputationSharedPtr
HdExtComputation::_CreateCpuComputation(
        HdSceneDelegate *sceneDelegate,
        HdBufferSourceVector *computationSources) const
{
    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();

    Hd_ExtCompInputSourceSharedPtrVector inputs;

    size_t numSceneInputs = _sceneInputs.size();
    for (size_t inputNum = 0; inputNum < numSceneInputs; ++inputNum) {
        const TfToken &inputName = _sceneInputs[inputNum];

        VtValue inputValue = sceneDelegate->Get(_id, inputName);

        Hd_ExtCompInputSourceSharedPtr inputSource(
                         new Hd_SceneExtCompInputSource(inputName, inputValue));

        computationSources->push_back(inputSource);
        inputs.push_back(inputSource);
    }

    size_t numCompInputs = _computationInputs.size();
    for (size_t inputNum = 0; inputNum < numCompInputs; ++inputNum) {
        const TfToken &inputName = _computationInputs[inputNum];
        const SourceComputationDesc &sourceDesc =
                                              _computationSourceDescs[inputNum];

        HdExtComputation *sourceComp;
        HdSceneDelegate *sourceCompSceneDelegate;

        renderIndex.GetExtComputationInfo(sourceDesc.computationId,
                                          &sourceComp,
                                          &sourceCompSceneDelegate);

        if (sourceComp != nullptr) {

            HdExtCompCpuComputationSharedPtr sourceCpuComputation =
                    sourceComp->GetComputation(
                            sourceCompSceneDelegate,
                            computationSources);

            Hd_ExtCompInputSourceSharedPtr inputSource(
                                            new Hd_CompExtCompInputSource(
                                                 inputName,
                                                 sourceCpuComputation,
                                                 sourceDesc.computationOutput));

            computationSources->push_back(inputSource);
            inputs.push_back(inputSource);
        }
    }

    HdExtCompCpuComputationSharedPtr computation(
            new HdExtCompCpuComputation(_id,
                                        inputs,
                                        _outputs,
                                        _elementCount,
                                        sceneDelegate));

    computationSources->push_back(computation);

    return computation;
}

std::pair<HdExtCompGpuComputationSharedPtr,
          HdExtCompGpuComputationBufferSourceSharedPtr>
HdExtComputation::_CreateGpuComputation(
        HdSceneDelegate *sceneDelegate,
        HdBufferSourceVector *computationSources,
        TfToken const &primvarName,
        HdBufferSpecVector const &outputBufferSpecs,
        HdBufferSpecVector const &primInputSpecs) const
{
    TF_DEBUG(HD_EXT_COMPUTATION_UPDATED).Msg(
            "HdExtComputation::_CreateGpuComputation\n");
    
    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
    HdBufferSourceVector inputs;

    size_t numSceneInputs = _sceneInputs.size();
    for (size_t inputNum = 0; inputNum < numSceneInputs; ++inputNum) {
        const TfToken &inputName = _sceneInputs[inputNum];

        VtValue inputValue = sceneDelegate->Get(_id, inputName);

        HdBufferSourceSharedPtr inputSource = HdBufferSourceSharedPtr(
                    new HdVtBufferSource(inputName, inputValue));

        inputs.push_back(inputSource);
    }

    HdComputeShaderSharedPtr shader = HdComputeShaderSharedPtr(
            new HdComputeShader());
    shader->SetComputeSource(_kernel);
    
    HdExtCompGpuComputationResourceSharedPtr resource(
            new HdExtCompGpuComputationResource(
                outputBufferSpecs,
                shader,
                renderIndex.GetResourceRegistry()));
            
    HdExtCompGpuComputationBufferSourceSharedPtr bufferSource(
            new HdExtCompGpuComputationBufferSource(
                _id,
                primvarName,
                inputs,
                _elementCount,
                resource));

    HdExtCompGpuComputationSharedPtr computation(
            new HdExtCompGpuComputation(_id,
                                        resource,
                                        primvarName,
                                        outputBufferSpecs,
                                        _elementCount));

    return std::make_pair(computation, bufferSource);
}

PXR_NAMESPACE_CLOSE_SCOPE
