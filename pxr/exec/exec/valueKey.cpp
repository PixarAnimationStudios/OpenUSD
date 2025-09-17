//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/valueKey.h"

PXR_NAMESPACE_OPEN_SCOPE

std::string
ExecValueKey::GetDebugName() const
{
    // Diagnostics should not be entangled with uncompilation dependencies.
    EsfJournal *journal = nullptr;
    std::string debugName = _provider->GetPath(journal).GetAsString();
    debugName += " [";
    debugName += _computationName.GetString();
    debugName += ']';
    return debugName;
}

PXR_NAMESPACE_CLOSE_SCOPE
