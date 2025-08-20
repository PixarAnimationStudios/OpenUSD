//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/vectorImpl_Dispatch.h"

#include "pxr/base/gf/vec3d.h"

PXR_NAMESPACE_OPEN_SCOPE

// Sanity check: GfVec3d should be memcpy-able. This is important because we
// want to optimize for fast pool output cache copies.
static_assert(
    Vdf_VectorImplDispatch< GfVec3d >::Memcopyable::value,
    "Expected GfVec3d to be memcpy-able");

// Sanity check: std::vector<bool> should not be memcopy-able, due to its
// not being a POD.
static_assert(
    !Vdf_VectorImplDispatch< std::vector<bool> >::Memcopyable::value,
    "Expected std::vector to not be memcpy-able");

PXR_NAMESPACE_CLOSE_SCOPE
