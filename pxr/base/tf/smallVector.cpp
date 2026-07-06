//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"
#include "pxr/base/tf/smallVector.h"

PXR_NAMESPACE_OPEN_SCOPE

#ifdef ARCH_BITS_64
static_assert(
    sizeof(TfSmallVector<int, 1>) == 16,
    "Expecting sizeof(TfSmallVector<int, N = 1>) to be 16 bytes.");
#else
static_assert(
    sizeof(TfSmallVector<int, 1>) == 12,
    "Expecting sizeof(TfSmallVector<int, N = 1>) to be 12 bytes.");
#endif

static_assert(
    sizeof(TfSmallVector<int, 2>) == 16,
    "Expecting sizeof(TfSmallVector<int, N = 2>) to be 16 bytes.");

static_assert(
    sizeof(TfSmallVector<double, 1>) == 16,
    "Expecting sizeof(TfSmallVector<double, N = 1>) to be 16 bytes.");

static_assert(
    sizeof(TfSmallVector<double, 2>) == 24,
    "Expecting sizeof(TfSmallVector<double, N = 2>) to be 24 bytes.");

#ifdef ARCH_BITS_64
static_assert(
    TfSmallVectorBase::ComputeSerendipitousLocalCapacity<char>() == 8,
    "Expecting 8 bytes of local capacity.");
#else
static_assert(
    TfSmallVectorBase::ComputeSerendipitousLocalCapacity<char>() == 4,
    "Expecting 4 bytes of local capacity.");
#endif

namespace {

struct Test16b
{
    uint64_t m[2];
};

struct alignas(64) TestOverAlignedChar
{
    char c;
};

} // anon

static_assert(
    TfComputeSmallVectorLocalCapacityForTotalSize<uint64_t, 64>() == 7,
    "Expected local capacity of 7 for 64-byte-sized TfSmallVector<uint64_t>");

static_assert(
    sizeof(TfSmallVector<uint64_t,
           TfComputeSmallVectorLocalCapacityForTotalSize<
           uint64_t, 64>()>) == 64,
    "Expected we can make a sizeof(TfSmallVector<uint64_t, N>) == 64");

// Check that size is largest possible <= requested.
static_assert(
    TfComputeSmallVectorLocalCapacityForTotalSize<Test16b, 64>() == 3,
    "Expected local capacity of 3 for 64-byte-sized TfSmallVector<Test16b>");

static_assert(
    sizeof(TfSmallVector<Test16b,
           TfComputeSmallVectorLocalCapacityForTotalSize<Test16b, 64>()>) == 56,
    "Expected sizeof(TfSmallVector<Test16b, CapForSize=64b>) == 56");

// Check that over-aligned types are handled properly.
// sizeof(TestOverAlignedChar) is 64, and
// sizeof(TfSmallVector<TestOverAlignedChar, 0>) is 16, so one might expect that
// we could fit TfSmallVector<TestOverAlignedChar, 1> in 80 bytes, or 2 in 144
// bytes, but that's not possible because the compiler has to add padding after
// the size & capacity fields so that the TestOverAlignedChar is properly
// aligned to 64-bytes.
static_assert(
    TfComputeSmallVectorLocalCapacityForTotalSize<
    TestOverAlignedChar, 80>() == 0,
    "Over-aligned type not properly handled.");
static_assert(
    TfComputeSmallVectorLocalCapacityForTotalSize<
    TestOverAlignedChar, 128>() == 1,
    "Over-aligned type not properly handled.");
static_assert(
    TfComputeSmallVectorLocalCapacityForTotalSize<
    TestOverAlignedChar, 256>() == 3,
    "Over-aligned type not properly handled.");

PXR_NAMESPACE_CLOSE_SCOPE
