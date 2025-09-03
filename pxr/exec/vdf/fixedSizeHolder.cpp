//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/fixedSizeHolder.h"

PXR_NAMESPACE_OPEN_SCOPE

// Check that the fixed size holder's size is actually fixed.
static_assert(sizeof(Vdf_FixedSizeHolder<char, 16>) == 16, "");
static_assert(sizeof(Vdf_FixedSizeHolder<char[16], 16>) == 16, "");
static_assert(sizeof(Vdf_FixedSizeHolder<void *, 16>) == 16, "");
static_assert(sizeof(Vdf_FixedSizeHolder<long long[8], 16>) == 16, "");

PXR_NAMESPACE_CLOSE_SCOPE
