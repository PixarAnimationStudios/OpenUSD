//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/execIr/computations.h"

PXR_NAMESPACE_OPEN_SCOPE

ExecIrComputationsType::ExecIrComputationsType()
    : computeDesiredValue("computeDesiredValue", TfToken::Immortal)
    , explicitDesiredValue("explicitDesiredValue", TfToken::Immortal)
{}

TfStaticData<ExecIrComputationsType> ExecIrComputations;

PXR_NAMESPACE_CLOSE_SCOPE
