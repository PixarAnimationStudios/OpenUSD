//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_ALLOCATE_BOXED_VALUE_H
#define PXR_EXEC_VDF_ALLOCATE_BOXED_VALUE_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/api.h"
#include "pxr/exec/vdf/boxedContainer.h"
#include "pxr/exec/vdf/vector.h"

#include "pxr/base/arch/functionLite.h"
#include "pxr/base/tf/diagnostic.h"
#include "pxr/base/tf/mallocTag.h"

PXR_NAMESPACE_OPEN_SCOPE

class TfToken;
class VdfContext;

// Allocate a new VdfVector as backing store for a boxed value.
//
VDF_API
VdfVector *Vdf_AllocateBoxedValueVector(const VdfContext &, const TfToken &);

// Allocate a new VdfVector as backing store for a boxed value, then construct
// a boxed value inside that newly allocated VdfVector.
//
template < typename T, typename... Args>
bool
Vdf_AllocateBoxedValue(
    const VdfContext &context,
    const TfToken &name,
    Args... args)
{
    // Allocate a new VdfVector for storing a boxed value. This function will
    // return nullptr if the output is not requested, or if an error occurred.
    // The function will emit a coding error in the latter case.
    VdfVector *v = Vdf_AllocateBoxedValueVector(context, name);
    if (!v) {
        return false;
    }

    // Keep track of how much memory we are allocating.
    TfAutoMallocTag tag(
        "Vdf", TfMallocTag::IsInitialized() ? TF_FUNC_NAME() : "");
    TfAutoMallocTag typedTag("Vdf", __ARCH_PRETTY_FUNCTION__);

    // Store a new Vdf_BoxedContainer at the output.
    v->Set(Vdf_BoxedContainer<T>(std::forward<Args>(args)...));

    // Success!
    return true;
}

PXR_NAMESPACE_CLOSE_SCOPE

#endif
