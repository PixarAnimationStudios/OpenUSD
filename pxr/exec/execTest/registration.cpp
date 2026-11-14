//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/execTest/publicClass.h"

#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/type.h"

PXR_NAMESPACE_OPEN_SCOPE

// To ensure test-only libraries are built correctly in static builds, we
// register the public class in a separate translation unit, which would be
// omitted if not linking as a whole-archive.

TF_REGISTRY_FUNCTION(TfType)
{
    TfType::Define<ExecTestPublicClass>();
}

PXR_NAMESPACE_CLOSE_SCOPE
