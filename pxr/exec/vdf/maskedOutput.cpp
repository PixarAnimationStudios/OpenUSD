//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/maskedOutput.h"

#include "pxr/exec/vdf/output.h"

PXR_NAMESPACE_OPEN_SCOPE

std::string
VdfMaskedOutput::GetDebugName() const
{
    return (_output ? _output->GetDebugName() : "(null)")
        + ' ' + _mask.GetRLEString();
}

PXR_NAMESPACE_CLOSE_SCOPE
