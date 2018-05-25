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

#include "pxr/imaging/hd/extCompCpuComputation.h"
#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/extComputationContextInternal.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hd/sceneExtCompInputSource.h"
#include "pxr/imaging/hd/compExtCompInputSource.h"

#include <limits>

PXR_NAMESPACE_OPEN_SCOPE

const size_t HdExtCompCpuComputation::INVALID_OUTPUT_INDEX =
                                             std::numeric_limits<size_t>::max();

HdExtCompCpuComputation::HdExtCompCpuComputation(
                             const SdfPath &id,
                             const Hd_ExtCompInputSourceSharedPtrVector &inputs,
                             const TfTokenVector &outputs,
                             int numElements,
                             HdSceneDelegate *sceneDelegate)
 : HdNullBufferSource()
 , _id(id)
 , _inputs(inputs)
 , _outputs(outputs)
 , _numElements(numElements)
 , _sceneDelegate(sceneDelegate)
 , _outputValues()
{
}

HdExtCompCpuComputationSharedPtr
HdExtCompCpuComputation::CreateComputation(
    HdSceneDelegate *sceneDelegate,
    const HdExtComputation &computation,
    HdBufferSourceVector *computationSources)
{
    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();

    const SdfPath &id = computation.GetId();

    Hd_ExtCompInputSourceSharedPtrVector inputs;
    for (const TfToken &inputName: computation.GetSceneInputNames()) {
        VtValue inputValue = sceneDelegate->Get(id, inputName);
        Hd_ExtCompInputSourceSharedPtr inputSource(
                         new Hd_SceneExtCompInputSource(inputName, inputValue));
        computationSources->push_back(inputSource);
        inputs.push_back(inputSource);
    }

    for (const HdExtComputationInputDescriptor & compInput:
                computation.GetComputationInputs()) {

        HdExtComputation const * sourceComp =
            static_cast<HdExtComputation const *>(
                renderIndex.GetSprim(HdPrimTypeTokens->extComputation,
                                     compInput.sourceComputationId));

        if (sourceComp != nullptr) {

            // Computations acting as input aggregations should schedule
            // input values for commit, but will have no Cpu computation
            // to create.
            if (sourceComp->IsInputAggregation()) {
                VtValue inputValue =
                    sceneDelegate->Get(compInput.sourceComputationId,
                                       compInput.name);
                Hd_ExtCompInputSourceSharedPtr inputSource(
                        new Hd_SceneExtCompInputSource(compInput.name,
                                                       inputValue));
                computationSources->push_back(inputSource);
                inputs.push_back(inputSource);
                continue;
            }

            HdExtCompCpuComputationSharedPtr sourceComputation =
                CreateComputation(sceneDelegate,
                                  *sourceComp,
                                  computationSources);

            Hd_ExtCompInputSourceSharedPtr inputSource(
                new Hd_CompExtCompInputSource(
                        compInput.name,
                        sourceComputation,
                        compInput.sourceComputationOutputName));

            computationSources->push_back(inputSource);
            inputs.push_back(inputSource);
        }
    }

    HdExtCompCpuComputationSharedPtr result(
            new HdExtCompCpuComputation(id,
                                        inputs,
                                        computation.GetOutputNames(),
                                        computation.GetElementCount(),
                                        sceneDelegate));

    computationSources->push_back(result);

    return result;
}

TfToken const &
HdExtCompCpuComputation::GetName() const
{
    return _id.GetToken();
}

bool
HdExtCompCpuComputation::Resolve()
{
    size_t numInputs = _inputs.size();

    bool inputError = false;
    for (size_t inputNum = 0; inputNum < numInputs; ++inputNum) {
        if (_inputs[inputNum]->IsValid()) {
            if (!_inputs[inputNum]->IsResolved()) {
                return false;
            }

            inputError |= _inputs[inputNum]->HasResolveError();
        }
        else
        {
            inputError = true;
        }
    }

    if (!_TryLock()) return false;

    if (inputError) {
         _SetResolveError();
         return true;
     }


    Hd_ExtComputationContextInternal context;

    for (size_t inputNum = 0; inputNum < numInputs; ++inputNum) {
        const Hd_ExtCompInputSourceSharedPtr &input = _inputs[inputNum];
        context.SetInputValue(input->GetName(), input->GetValue());
    }

    _sceneDelegate->InvokeExtComputation(_id, &context);
    if (context.HasComputationError()) {
        _SetResolveError();
        return true;
    }


    size_t numOutputs = _outputs.size();

    _outputValues.resize(numOutputs);
    for (size_t outputNum = 0; outputNum < numOutputs; ++outputNum) {
        const TfToken &outputName = _outputs[outputNum];

        if (!context.GetOutputValue(outputName, &_outputValues[outputNum])) {
            _SetResolveError();
            return true;
        }
    }

    _SetResolved();
    return true;
}


int
HdExtCompCpuComputation::GetNumElements() const
{
    return _numElements;
}

size_t
HdExtCompCpuComputation::GetOutputIndex(const TfToken &outputName) const
{
    size_t numOutputs = _outputs.size();
    for (size_t outputNum = 0; outputNum < numOutputs; ++outputNum) {
        if (outputName == _outputs[outputNum]) {
            return outputNum;
        }
    }

    return INVALID_OUTPUT_INDEX;
}

const VtValue &
HdExtCompCpuComputation::GetOutputByIndex(size_t index) const
{
    return _outputValues[index];
}

bool
HdExtCompCpuComputation::_CheckValid() const
{
    return _sceneDelegate != nullptr;
}


PXR_NAMESPACE_CLOSE_SCOPE
