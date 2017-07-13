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
#include "pxr/imaging/hd/perfLog.h"
#include "pxr/imaging/hd/resourceRegistry.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sceneExtCompInputSource.h"

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

    if (!TF_VERIFY(sceneDelegate != nullptr)) {
        return;
    }

    HdDirtyBits bits = *dirtyBits;

    if (bits & DirtyInputDesc) {
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


    *dirtyBits = Clean;
}


HdExtCompCpuComputationSharedPtr
HdExtComputation::GetComputation(HdSceneDelegate *sceneDelegate) const
{
    // XXX: To do: De-duplication
    return _CreateCpuComputation(sceneDelegate);
}

HdDirtyBits
HdExtComputation::GetInitialDirtyBits() const
{
    return AllDirty;
}

HdExtCompCpuComputationSharedPtr
HdExtComputation::_CreateCpuComputation(HdSceneDelegate *sceneDelegate) const
{
    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();
    HdResourceRegistrySharedPtr const &resourceRegistry =
                                              renderIndex.GetResourceRegistry();


    Hd_ExtCompInputSourceSharedPtrVector inputs;

    size_t numSceneInputs = _sceneInputs.size();
    for (size_t inputNum = 0; inputNum < numSceneInputs; ++inputNum) {
        const TfToken &inputName = _sceneInputs[inputNum];

        VtValue inputValue = sceneDelegate->Get(_id, inputName);

        Hd_ExtCompInputSourceSharedPtr inputSource(
                         new Hd_SceneExtCompInputSource(inputName, inputValue));

        resourceRegistry->AddSource(inputSource);
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
                            sourceComp->GetComputation(sourceCompSceneDelegate);

            Hd_ExtCompInputSourceSharedPtr inputSource(
                                            new Hd_CompExtCompInputSource(
                                                 inputName,
                                                 sourceCpuComputation,
                                                 sourceDesc.computationOutput));

            resourceRegistry->AddSource(inputSource);
            inputs.push_back(inputSource);
        }
    }

    HdExtCompCpuComputationSharedPtr computation(
            new HdExtCompCpuComputation(_id,
                                        inputs,
                                        _outputs,
                                        _elementCount,
                                        sceneDelegate));

    resourceRegistry->AddSource(computation);

    return computation;
}


PXR_NAMESPACE_CLOSE_SCOPE
