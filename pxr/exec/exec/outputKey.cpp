//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/outputKey.h"

#include "pxr/base/tf/token.h"
#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

std::string
Exec_OutputKey::Identity::GetDebugName() const 
{
    std::string debugName = _providerPath.GetAsString();
    debugName += " [";
    debugName += _computationDefinition->GetComputationName().GetString();
    debugName += ']';
    return debugName;
}

PXR_NAMESPACE_CLOSE_SCOPE
