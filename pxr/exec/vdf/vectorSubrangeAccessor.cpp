//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/vectorSubrangeAccessor.h"

#include "pxr/base/arch/debugger.h"
#include "pxr/base/arch/demangle.h"
#include "pxr/base/tf/diagnostic.h"

PXR_NAMESPACE_OPEN_SCOPE

void
Vdf_VectorSubrangeAccessorPostFatalError(
    const std::type_info &haveType,
    const std::type_info &wantType)
{
    TF_FATAL_ERROR(
        "Invalid type.  Vector is holding %s, tried to use as %s",
        ArchGetDemangled(haveType).c_str(),
        ArchGetDemangled(wantType).c_str());

    // Currently, Tf fatal diagnostics are not marked as noreturn even though
    // they should not return.  See PRES-66823.
    ArchAbort();
}

PXR_NAMESPACE_CLOSE_SCOPE
