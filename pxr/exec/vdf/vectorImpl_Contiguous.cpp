//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/vectorImpl_Contiguous.h"

#include "pxr/exec/vdf/vectorImpl_Single.h"
#include "pxr/exec/vdf/vectorImpl_Empty.h"

PXR_NAMESPACE_OPEN_SCOPE

#define VDF_INSTANTIATE_VECTOR_IMPL_CONTIGUOUS(type)    \
    template class Vdf_VectorImplContiguous<type>;
VDF_FOR_EACH_COMMON_TYPE(VDF_INSTANTIATE_VECTOR_IMPL_CONTIGUOUS)
#undef VDF_INSTANTIATE_VECTOR_IMPL_CONTIGUOUS

PXR_NAMESPACE_CLOSE_SCOPE
