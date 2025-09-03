//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/vectorDataTyped.h"

#include "pxr/exec/vdf/vectorImpl_Contiguous.h"
#include "pxr/exec/vdf/vectorImpl_Empty.h"
#include "pxr/exec/vdf/vectorImpl_Single.h"

PXR_NAMESPACE_OPEN_SCOPE

#if !defined(ARCH_OS_WINDOWS)
#define VDF_INSTANTIATE_VECTOR_DATA_TYPED(type) \
    template class Vdf_VectorDataTyped<type>;
VDF_FOR_EACH_COMMON_TYPE(VDF_INSTANTIATE_VECTOR_DATA_TYPED)
#undef VDF_INSTANTIATE_VECTOR_DATA_TYPED
#endif // !defined(ARCH_OS_WINDOWS)

PXR_NAMESPACE_CLOSE_SCOPE
