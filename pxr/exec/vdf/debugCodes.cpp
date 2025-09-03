//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/debugCodes.h"

PXR_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(VDF_MUNG_BUFFER_LOCKING,
        "Enable debugging output for mung buffer locking.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(VDF_SPARSE_INPUT_PATH_FINDER,
        "Enable debugging output for the sparse input path finder.");

    TF_DEBUG_ENVIRONMENT_SYMBOL(VDF_SCHEDULING,
        "Enable debugging output for scheduling and schedule invalidation.");
}

PXR_NAMESPACE_CLOSE_SCOPE
