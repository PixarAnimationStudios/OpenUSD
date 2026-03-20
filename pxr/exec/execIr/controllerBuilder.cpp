//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/execIr/controllerBuilder.h"

PXR_NAMESPACE_OPEN_SCOPE

ExecIrControllerBuilder::ExecIrControllerBuilder(
    Exec_ComputationBuilder &self,
    Callback forwardCallback,
    Callback inverseCallback)
    : _self(self)
    , _forwardComputeReg(self.PrimComputation(ExecIrTokens->forwardCompute))
    , _inverseComputeReg(self.PrimComputation(ExecIrTokens->inverseCompute))
{
    _forwardComputeReg.Callback<ExecIrResult>(forwardCallback);
    _inverseComputeReg.Callback<ExecIrResult>(inverseCallback);
}

const TfToken &
ExecIrControllerBuilder::_GetConstantInputName()
{
    static TfToken inputNameToken("execIrControllerBuilder_inputName");
    return inputNameToken;
}

PXR_NAMESPACE_CLOSE_SCOPE
