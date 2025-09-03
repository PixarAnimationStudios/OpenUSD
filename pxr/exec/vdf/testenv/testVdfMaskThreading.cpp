//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/pxr.h"

#include "pxr/exec/vdf/mask.h"

#include "pxr/base/tf/bits.h"
#include "pxr/base/trace/trace.h"
#include "pxr/base/trace/reporter.h"
#include "pxr/base/work/loops.h"
#include "pxr/base/work/threadLimits.h"

#include <cstdint>
#include <iostream>
#include <random>

// Declaration of test-only function in Vdf/Mask.cpp.
PXR_NAMESPACE_OPEN_SCOPE
size_t Vdf_MaskRegistry_GetSize();
PXR_NAMESPACE_CLOSE_SCOPE

PXR_NAMESPACE_USING_DIRECTIVE

namespace
{
    // Writes VdfMasks into entries of targetMasks constructed from the
    // corresponding entries of sourceBits.
    struct _MaskCreator
    {
        _MaskCreator(
            const std::vector<VdfMask::Bits> & sourceBits,
            std::vector<VdfMask> * targetMasks)
            : _sourceBits(sourceBits)
            , _targetMasks(targetMasks)
        {}

        void operator()(size_t begin, size_t end)
        {
            for (size_t i=begin; i!=end; ++i) {
                (*_targetMasks)[i] = VdfMask(_sourceBits[i]);
            }
        }

    private:
        const std::vector<VdfMask::Bits> & _sourceBits;
        std::vector<VdfMask> * _targetMasks;
    };


    // Performs a pseudo-random sequence of create, swap and drop on a vector
    // of masks.
    struct _MaskRandomOp
    {
        _MaskRandomOp(
            const std::vector<VdfMask::Bits> & sourceBits,
            std::vector<VdfMask> * targetMasks)
            : _sourceBits(sourceBits)
            , _targetMasks(targetMasks)
        {}

        void operator()(size_t begin, size_t end)
        {
            std::mt19937 rng(begin);
            std::uniform_int_distribution<size_t> randomOp(0, 2);
            std::uniform_int_distribution<size_t> s(begin, end-1);

            for (size_t i=begin; i!=end; ++i) {
                switch (randomOp(rng)) {
                case 0:
                    _Create(i);
                    break;
                case 1:
                    // Note that swapping with this distribution results in a
                    // biased shuffle, but that's not an important property
                    // for this test.
                    _Swap(i, s(rng));
                    break;
                case 2:
                    _Drop(i);
                    break;
                }
            }
        }

    private:
        void _Create(size_t i)
        {
            (*_targetMasks)[i] = VdfMask(_sourceBits[i]);
        }

        void _Swap(size_t i, size_t j)
        {
            std::vector<VdfMask> & targetMasks = *_targetMasks;
            VdfMask m = targetMasks[j];
            targetMasks[j] = targetMasks[i];
            targetMasks[i] = m;
        }

        void _Drop(size_t i)
        {
            (*_targetMasks)[i] = VdfMask();
        }

    private:
        const std::vector<VdfMask::Bits> & _sourceBits;
        std::vector<VdfMask> * _targetMasks;
    };


    // Writes default-construct VdfMasks into entries of targetMasks.
    struct _MaskDropper
    {
        _MaskDropper(
            std::vector<VdfMask> * targetMasks)
            : _targetMasks(targetMasks)
        {}

        void operator()(size_t begin, size_t end)
        {
            for (size_t i=begin; i!=end; ++i) {
                (*_targetMasks)[i] = VdfMask();
            }
        }

    private:
        std::vector<VdfMask> * _targetMasks;
    };


    // Writes VdfMasks into entries of targetMasks constructed by moving
    // the corresponding entries of sourceBits.
    struct _MaskMoveCreator
    {
        _MaskMoveCreator(
            std::vector<VdfMask::Bits> * sourceBits,
            std::vector<VdfMask> * targetMasks)
            : _sourceBits(sourceBits)
            , _targetMasks(targetMasks)
        {}

        void operator()(size_t begin, size_t end)
        {
            for (size_t i=begin; i!=end; ++i) {
                VdfMask::Bits * bits = &((*_sourceBits)[i]);
                (*_targetMasks)[i] = VdfMask(std::move(*bits));
            }
        }

    private:
        std::vector<VdfMask::Bits> * _sourceBits;
        std::vector<VdfMask> * _targetMasks;
    };


    // Repeatedly creates and destroys a single mask.
    struct _MaskThrasher
    {
        _MaskThrasher()
            : _bits(1)
        {}

        void operator()(size_t begin, size_t end)
        {
            for (size_t i=begin; i!=end; ++i) {
                VdfMask m(_bits);
            }
        }

    private:
        VdfMask::Bits _bits;
    };
}

static std::vector<VdfMask::Bits>
_MakeSourceBits(size_t n)
{
    TRACE_FUNCTION();

    std::vector<VdfMask::Bits> sourceBits;
    sourceBits.reserve(n);

    // Build a few really, really bad mask patterns.
    static const size_t NumSlowMasks = n/32;
    for (size_t i=0; i<NumSlowMasks; ++i) {
        static const size_t SlowMaskSizeStart = 1024;

        size_t slowMaskSize = SlowMaskSizeStart + i;
        TfBits b(slowMaskSize);
        for (size_t j=0; j<slowMaskSize; ++j) {
            b.Assign(j, j%2);
        }
        sourceBits.push_back(VdfMask::Bits(b));
    }
    // Populate the rest of the source bits with more reasonable patterns.
    TfBits b;
    for (size_t i=NumSlowMasks; i<n/2; ++i) {
        if (i%2 == 0) {
            size_t sz = b.GetSize();
            b.ResizeKeepContent(sz+1);
        }
        b.Assign(0, i%2);

        sourceBits.push_back(VdfMask::Bits(b));
    }

    // Append a reversed copy of the first half of bits.
    sourceBits.insert(sourceBits.end(), sourceBits.rbegin(), sourceBits.rend());

    return sourceBits;
}

static void
_AssertRegistrySize(size_t expectedSize)
{
    size_t actualSize = Vdf_MaskRegistry_GetSize();
    if (actualSize != expectedSize) {
        TF_FATAL_ERROR("Expected empty registry with size %zu; got %zu",
                       expectedSize, actualSize);
    }
}

int
main(int, char **)
{
    WorkSetMaximumConcurrencyLimit();

    // While this is a correctness test, we dump profiling information to help
    // investigate other performance regressions.
    TraceCollector::GetInstance().SetEnabled(true);

    // Initially, there should not be anything in the mask registry.
    _AssertRegistrySize(0);

    // Make sure the 1x1 mask is always registered.
    VdfMask oneOne = VdfMask::AllOnes(1);

    static const size_t NumMasks = UINT64_C(1) << 18;
    std::vector<VdfMask::Bits> sourceBits = _MakeSourceBits(NumMasks);

    // Test mask lifecycle (creation, copy, drop)
    {
        std::vector<VdfMask> masks(sourceBits.size());
        _MaskCreator creator(sourceBits, &masks);
        _MaskRandomOp randomOp(sourceBits, &masks);
        _MaskDropper dropper(&masks);

        _AssertRegistrySize(1);

        // Fill the masks vector in parallel.
        {
            TRACE_SCOPE("Create masks");
            WorkParallelForN(sourceBits.size(), creator);
        }

        // Verify that the masks vector was filled correctly.
        _AssertRegistrySize(NumMasks/2);
        for (size_t i=0; i < masks.size(); ++i) {
            const VdfMask & m = masks[i];
            TF_AXIOM(m.GetBits() == sourceBits[i]);
        }

        // Create, copy & drop masks in parallel.
        {
            TRACE_SCOPE("Random mask operation pass 1");
            WorkParallelForN(sourceBits.size(), randomOp);
        }
        {
            TRACE_SCOPE("Random mask operation pass 2");
            WorkParallelForN(sourceBits.size(), randomOp);
        }
        {
            TRACE_SCOPE("Random mask operation pass 3");
            WorkParallelForN(sourceBits.size(), randomOp);
        }

        // Drop all remaining masks in parallel
        {
            TRACE_SCOPE("Drop masks");
            WorkParallelForN(sourceBits.size(), dropper);
        }

        // Verify that all masks were dropped.
        _AssertRegistrySize(16);
        for (const VdfMask &m : masks) {
            TF_AXIOM(m == VdfMask());
        }
    }

    // Profile copy vs move construction
    {
        std::vector<VdfMask::Bits> sourceBitsCopy(sourceBits);
        std::vector<VdfMask> masks(sourceBitsCopy.size());
        _MaskMoveCreator moveCreator(&sourceBitsCopy, &masks);

        {
            TRACE_SCOPE("Move construct masks");
            WorkParallelForN(sourceBitsCopy.size(), moveCreator);
        }
    }
    {
        std::vector<VdfMask::Bits> sourceBitsCopy(sourceBits);
        std::vector<VdfMask> masks(sourceBitsCopy.size());
        _MaskCreator copyCreator(sourceBitsCopy, &masks);

        {
            TRACE_SCOPE("Copy construct masks");
            WorkParallelForN(sourceBitsCopy.size(), copyCreator);
        }
    }

    _AssertRegistrySize(16);

    // Create & destroy a single mask by repeatedly in multiple threads.
    {
        static const size_t CreateDestroyCyclesPerThread = UINT64_C(1) << 22;
        _MaskThrasher thrasher;

        TRACE_SCOPE("Create-destroy thrashing");
        WorkParallelForN(CreateDestroyCyclesPerThread, thrasher);
    }

    _AssertRegistrySize(16);

    TraceReporter::GetGlobalReporter()->Report(std::cout);

    return 0;
}
