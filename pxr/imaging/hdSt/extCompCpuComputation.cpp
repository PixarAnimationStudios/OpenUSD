//
// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/imaging/hdSt/extCompCpuComputation.h"
#include "pxr/imaging/hdSt/extCompComputedInputSource.h"
#include "pxr/imaging/hdSt/extCompSceneInputSource.h"

#include "pxr/imaging/hd/extComputation.h"
#include "pxr/imaging/hd/extComputationContextInternal.h"
#include "pxr/imaging/hd/sceneDelegate.h"

#include <limits>

PXR_NAMESPACE_OPEN_SCOPE


const size_t HdStExtCompCpuComputation::INVALID_OUTPUT_INDEX =
                                        std::numeric_limits<size_t>::max();

HdStExtCompCpuComputation::HdStExtCompCpuComputation(
    const SdfPath &id,
    const HdSt_ExtCompInputSourceSharedPtrVector &inputs,
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

HdStExtCompCpuComputation::~HdStExtCompCpuComputation() = default;

HdStExtCompCpuComputationSharedPtr
HdStExtCompCpuComputation::CreateComputation(
    HdSceneDelegate *sceneDelegate,
    const HdExtComputation &computation,
    HdBufferSourceSharedPtrVector *computationSources)
{
    HdRenderIndex &renderIndex = sceneDelegate->GetRenderIndex();

    const SdfPath &id = computation.GetId();

    HdSt_ExtCompInputSourceSharedPtrVector inputs;
    for (const TfToken &inputName: computation.GetSceneInputNames()) {
        VtValue inputValue = sceneDelegate->GetExtComputationInput(
                                                id, inputName);
        HdSt_ExtCompInputSourceSharedPtr inputSource =
            std::make_shared<HdSt_ExtCompSceneInputSource>(
                                                inputName, inputValue);
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
                    sceneDelegate->GetExtComputationInput(
                            compInput.sourceComputationId,
                            compInput.name);
                HdSt_ExtCompInputSourceSharedPtr inputSource =
                    std::make_shared<HdSt_ExtCompSceneInputSource>(
                                                compInput.name, inputValue);
                computationSources->push_back(inputSource);
                inputs.push_back(inputSource);
                continue;
            }

            HdStExtCompCpuComputationSharedPtr sourceComputation =
                CreateComputation(sceneDelegate,
                                  *sourceComp,
                                  computationSources);

            HdSt_ExtCompInputSourceSharedPtr inputSource =
                std::make_shared<HdSt_ExtCompComputedInputSource>(
                        compInput.name,
                        sourceComputation,
                        compInput.sourceComputationOutputName);

            computationSources->push_back(inputSource);
            inputs.push_back(inputSource);
        }
    }

    HdStExtCompCpuComputationSharedPtr result =
        std::make_shared<HdStExtCompCpuComputation>(id,
                                        inputs,
                                        computation.GetOutputNames(),
                                        computation.GetElementCount(),
                                        sceneDelegate);

    computationSources->push_back(result);

    return result;
}

TfToken const &
HdStExtCompCpuComputation::GetName() const
{
    return _id.GetToken();
}

bool
HdStExtCompCpuComputation::Resolve()
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


    HdExtComputationContextInternal context;

    for (size_t inputNum = 0; inputNum < numInputs; ++inputNum) {
        const HdSt_ExtCompInputSourceSharedPtr &input = _inputs[inputNum];
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

size_t
HdStExtCompCpuComputation::GetNumElements() const
{
    return _numElements;
}

size_t
HdStExtCompCpuComputation::GetOutputIndex(const TfToken &outputName) const
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
HdStExtCompCpuComputation::GetOutputByIndex(size_t index) const
{
    return _outputValues[index];
}

bool
HdStExtCompCpuComputation::_CheckValid() const
{
    return _sceneDelegate != nullptr;
}


PXR_NAMESPACE_CLOSE_SCOPE

