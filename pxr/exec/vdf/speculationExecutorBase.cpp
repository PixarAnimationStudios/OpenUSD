//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/speculationExecutorBase.h"

PXR_NAMESPACE_OPEN_SCOPE

VdfSpeculationExecutorBase::VdfSpeculationExecutorBase(
    const VdfExecutorInterface *parentExecutor) :
    VdfExecutorInterface(parentExecutor),
    _speculationNode(nullptr),
    _parentSpeculationExecutor(
        dynamic_cast<const VdfSpeculationExecutorBase *>(parentExecutor))
{   
    // A parent executor is required.
    TF_VERIFY(parentExecutor);

    // If there is a parent speculation executor, get the speculation root
    // executor from it. Otherwise, the specified parent executor is the
    // speculation root executor, because it is not a speculation executor
    // itself.
    _parentNonSpeculationExecutor = _parentSpeculationExecutor
        ? _parentSpeculationExecutor->GetNonSpeculationParentExecutor()
        : parentExecutor;

    // A non-speculation parent executor is required.
    TF_VERIFY(_parentNonSpeculationExecutor);
}

VdfSpeculationExecutorBase::~VdfSpeculationExecutorBase()
{

}

PXR_NAMESPACE_CLOSE_SCOPE
