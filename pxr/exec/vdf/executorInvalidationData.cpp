//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/executorInvalidationData.h"

#include "pxr/base/arch/align.h"

PXR_NAMESPACE_OPEN_SCOPE

static_assert(
    sizeof(VdfExecutorInvalidationData) <= ARCH_CACHE_LINE_SIZE,
    "VdfExecutorInvalidationData is larger than one cache line.");

void
VdfExecutorInvalidationData::Reset()
{
    _maskState = _MaskState::AllOnes;
}

void
VdfExecutorInvalidationData::Clone(VdfExecutorInvalidationData *dest) const
{
    // Duplicate the invalidation specific state
    dest->_mask = _mask;
    dest->_maskState = _maskState;
}

PXR_NAMESPACE_CLOSE_SCOPE
