//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/connection.h"

PXR_NAMESPACE_OPEN_SCOPE

std::string
VdfConnection::GetDebugName() const
{
    std::string res;

    res += GetSourceOutput().GetDebugName();
    res += " -> ";
    res += GetTargetInput().GetDebugName();

    return res;
}

PXR_NAMESPACE_CLOSE_SCOPE
