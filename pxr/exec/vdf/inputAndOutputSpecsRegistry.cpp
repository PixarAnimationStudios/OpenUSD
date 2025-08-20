//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/vdf/inputAndOutputSpecsRegistry.h"

PXR_NAMESPACE_OPEN_SCOPE

const VdfInputAndOutputSpecs *
Vdf_InputAndOutputSpecsRegistry::AcquireSharedSpecs(
    const VdfInputSpecs &inputSpecs,
    const VdfOutputSpecs &outputSpecs)
{
    TfAutoMallocTag2 tag("Vdf", 
        "Vdf_InputAndOutputSpecsRegistry::AcquireSharedSpecs");

    // Look up the specs or create a new one if there isn't already one there.
    // Note we start with a ref count of 1 to avoid potential hazards from
    // concurrently inserting and removing. If the new entry is not inserted,
    // i.e., it's already been in the map, make sure to increment the ref count.
    // Non-exclusive access is okay here, because the ref count is an atomic
    // integer, and we only need to protect against concurrent deletion, which
    // will request exclusive access below.
    _SpecsTable::const_accessor accessor;
    if (!_specsTable.emplace(accessor, 
            VdfInputAndOutputSpecs(inputSpecs, outputSpecs), _RefCount(1))) {
        ++accessor->second.refCount;
    }

    return &accessor->first;
}

void 
Vdf_InputAndOutputSpecsRegistry::ReleaseSharedSpecs(
    const VdfInputAndOutputSpecs *specs)
{
    // We allow passing in NULL specs for client convenience.
    if (!specs) {
        return;
    }

    // Let's start by grabbing non-exclusive access and decrementing the atomic
    // reference count. If that reaches zero, we need to try to erase the entry
    // from the table below.
    bool tryErase = false;
    {
        _SpecsTable::const_accessor accessor;
        // We better find the entry in our table!  Otherwise it's not a spec
        // that was tracked by us and should never have been passed down to this
        // function.
        if (TF_VERIFY(_specsTable.find(accessor, *specs))) {
            // We better make sure that the guy we're freeing is exactly the one
            // that was in our table, not just one that compares equal to
            // *specs.
            if (TF_VERIFY(specs == &accessor->first)) {
                tryErase |= (--accessor->second.refCount == 0);
            }
        }
    }

    // If the reference count just reached zero, let's try to erase the entry
    // from the table. To do so, let's first grab exclusive access to the entry.
    // If another thread was able to revive the entry since its ref count
    // reached zero, and before we were able to grab the exclusive accessor,
    // we don't need to do any additional work.
    if (tryErase) {
        _SpecsTable::accessor accessor;
        // At the very least, the entry should still be in the table.
        if (TF_VERIFY(_specsTable.find(accessor, *specs))) {
            // If the ref count is still zero, we can go ahead with erasure. We
            // have exclusive access at this point, so concurrent revival is
            // no longer possible.
            if (accessor->second.refCount.load() == 0) {
                TF_VERIFY(_specsTable.erase(accessor));
            }
        }
    }
}

PXR_NAMESPACE_CLOSE_SCOPE
