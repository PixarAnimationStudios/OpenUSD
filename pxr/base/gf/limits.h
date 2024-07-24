//
// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_BASE_GF_LIMITS_H
#define PXR_BASE_GF_LIMITS_H

/// \file gf/limits.h
/// \ingroup group_gf_BasicMath
/// Defines useful mathematical limits.

/// This constant is used to determine whether the length of a vector is too
/// small to handle accurately.
/// \ingroup group_gf_BasicMath
#define GF_MIN_VECTOR_LENGTH  1e-10

/// This constant is used to determine when a set of basis vectors is close to
/// orthogonal.
/// \ingroup group_gf_LinearAlgebra
#define GF_MIN_ORTHO_TOLERANCE 1e-6

#endif
