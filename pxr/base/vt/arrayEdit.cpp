//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//

#include "pxr/pxr.h"
#include "pxr/base/vt/arrayEdit.h"
#include "pxr/base/vt/typeHeaders.h"
#include "pxr/base/tf/preprocessorUtilsLite.h"

PXR_NAMESPACE_OPEN_SCOPE

// Instantiate basic ArrayEdit templates.
#define VT_ARRAY_EDIT_EXPLICIT_INST(unused, elem) \
    template class VT_API VtArrayEdit< VT_TYPE(elem) >;
TF_PP_SEQ_FOR_EACH(VT_ARRAY_EDIT_EXPLICIT_INST, ~, VT_SCALAR_VALUE_TYPES)
#undef VT_ARRAY_EDIT_EXPLICIT_INST
    
PXR_NAMESPACE_CLOSE_SCOPE
