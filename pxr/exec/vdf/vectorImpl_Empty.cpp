//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/vectorImpl_Empty.h"

#include "pxr/exec/vdf/vectorImpl_Contiguous.h"
#include "pxr/exec/vdf/vectorImpl_Single.h"

PXR_NAMESPACE_OPEN_SCOPE

#define VDF_INSTANTIATE_VECTOR_IMPL_EMPTY(type) \
    template class Vdf_VectorImplEmpty<type>;
VDF_FOR_EACH_COMMON_TYPE(VDF_INSTANTIATE_VECTOR_IMPL_EMPTY)
#undef VDF_INSTANTIATE_VECTOR_IMPL_EMPTY

PXR_NAMESPACE_CLOSE_SCOPE
