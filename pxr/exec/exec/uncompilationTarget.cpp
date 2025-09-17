//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/uncompilationTarget.h"

#include "pxr/base/tf/stringUtils.h"

PXR_NAMESPACE_OPEN_SCOPE

std::string
Exec_NodeUncompilationTarget::GetDescription() const
{
    return TfStringPrintf("Node(%zu)", _nodeId);
}

std::string
Exec_InputUncompilationTarget::GetDescription() const
{
    if (_identity) {
        return TfStringPrintf(
            "Input(%zu, %s)",
            _identity->nodeId,
            _identity->inputName.GetText());
    }
    
    return "Input(null)";
}

PXR_NAMESPACE_CLOSE_SCOPE
