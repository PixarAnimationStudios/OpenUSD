//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/connectorSpecs.h"
    
PXR_NAMESPACE_OPEN_SCOPE

bool
VdfInputSpecs::operator==(const VdfInputSpecs &rhs) const
{
    // Early bail out.
    if (this == &rhs) 
        return true;

    // Must not be equal if different number of connectors
    if (GetSize() != rhs.GetSize())
        return false;

    for(size_t i = 0; i < GetSize(); i++) {
        if (*GetInputSpec(i) != *rhs.GetInputSpec(i))
            return false;
    }

    return true;
}

bool
VdfOutputSpecs::operator==(const VdfOutputSpecs &rhs) const
{
    // Early bail out.
    if (this == &rhs) 
        return true;

    // Must not be equal if different number of connectors
    if (GetSize() != rhs.GetSize())
        return false;

    for(size_t i = 0; i < GetSize(); i++) {
        if (*GetOutputSpec(i) != *rhs.GetOutputSpec(i))
            return false;
    }

    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE
