//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/types.h"

#include "pxr/base/tf/envSetting.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_ENV_SETTING(VDF_ENABLE_PARALLEL_EVALUATION_ENGINE, true,
    "Enables parallel evaluation at the level of a single round of exec "
    "evaluation. This is distinct from other forms of evaluation parallelism "
    "where results for different times may be computed in parallel.")

bool
VdfIsParallelEvaluationEnabled() 
{
    return TfGetEnvSetting(VDF_ENABLE_PARALLEL_EVALUATION_ENGINE);
}

PXR_NAMESPACE_CLOSE_SCOPE
