//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hgiPresent/debugCodes.h"

#include "pxr/base/tf/debug.h"
#include "pxr/base/tf/registryManager.h"

PXR_NAMESPACE_OPEN_SCOPE


TF_REGISTRY_FUNCTION(TfDebug)
{
     TF_DEBUG_ENVIRONMENT_SYMBOL(HGIPRESENT_DUMP_CANDIDATE_SURFACE_FORMATS,
        "Dump candidate formats in order of match. "
        "Values are API specific.");
}


PXR_NAMESPACE_CLOSE_SCOPE
