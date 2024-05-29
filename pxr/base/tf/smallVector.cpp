//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/smallVector.h"

PXR_NAMESPACE_OPEN_SCOPE

static_assert(
    sizeof(TfSmallVector<int, 1>) == 16, 
    "Expecting sizeof(TfSmallVector<int, N = 1>) to be 16 bytes.");

static_assert(
    sizeof(TfSmallVector<int, 2>) == 16, 
    "Expecting sizeof(TfSmallVector<int, N = 2>) to be 16 bytes.");

static_assert(
    sizeof(TfSmallVector<double, 1>) == 16, 
    "Expecting sizeof(TfSmallVector<double, N = 1>) to be 16 bytes.");

static_assert(
    sizeof(TfSmallVector<double, 2>) == 24, 
    "Expecting sizeof(TfSmallVector<double, N = 2>) to be 24 bytes.");

static_assert(
    TfSmallVectorBase::ComputeSerendipitousLocalCapacity<char>() == 8,
    "Expecting 8 bytes of local capacity.");

PXR_NAMESPACE_CLOSE_SCOPE
