//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_VDF_INPUT_AND_OUTPUT_SPECS_REGISTRY_H
#define PXR_EXEC_VDF_INPUT_AND_OUTPUT_SPECS_REGISTRY_H

#include "pxr/pxr.h"

#include "pxr/exec/vdf/inputAndOutputSpecs.h"

#include "tbb/concurrent_hash_map.h"

#include <atomic>

PXR_NAMESPACE_OPEN_SCOPE

///////////////////////////////////////////////////////////////////////////////
///
/// \class Vdf_InputAndOutputSpecsRegistry
///
/// \brief A registry for managing common VdfInputAndOutputSpecs objects.
///
class Vdf_InputAndOutputSpecsRegistry
{
public:

    // Acquires a VdfInputAndOutputSpecs from the registry.  The reference
    // count is incremented before return.  This function must be followed
    // by a corresponding call to ReleaseSharedSpecs.
    //
    const VdfInputAndOutputSpecs *AcquireSharedSpecs(
        const VdfInputSpecs &inputSpecs,
        const VdfOutputSpecs &outputSpecs);

    // Decrements the reference count, and, if appropriate, frees the object
    // referenced by \p specs.  This function must be called after each call
    // to AcquireSharedSpecs().
    //
    void ReleaseSharedSpecs(const VdfInputAndOutputSpecs *specs);

private:

    // Ref count class
    struct _RefCount {
        _RefCount(size_t v) : refCount(v) {}
        _RefCount(const _RefCount &rhs) : refCount(rhs.refCount.load()) {}
        mutable std::atomic<size_t> refCount;
    };

    // Hash functor for VdfInputAndOutputSpecs
    struct _HashSpecs {
        bool equal(const VdfInputAndOutputSpecs &lhs,
            const VdfInputAndOutputSpecs &rhs) const {
            return lhs == rhs;
        }

        size_t hash(const VdfInputAndOutputSpecs &specs) const {
            return specs.GetHash();
        }
    };

    // The table that holds the specs objects and their reference counts.
    using _SpecsTable = tbb::concurrent_hash_map<
        VdfInputAndOutputSpecs, _RefCount, _HashSpecs>;
    _SpecsTable _specsTable;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
