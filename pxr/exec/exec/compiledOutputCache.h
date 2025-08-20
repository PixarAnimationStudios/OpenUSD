//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_COMPILED_OUTPUT_CACHE_H
#define PXR_EXEC_EXEC_COMPILED_OUTPUT_CACHE_H

#include "pxr/pxr.h"

#include "pxr/exec/exec/outputKey.h"

#include "pxr/base/tf/hash.h"
#include "pxr/exec/vdf/maskedOutput.h"
#include "pxr/exec/vdf/types.h"

#include <tbb/concurrent_unordered_map.h>
#include <tbb/concurrent_vector.h>

#include <tuple>

PXR_NAMESPACE_OPEN_SCOPE

/// The output cache stores a compiled masked output for a given output key.
class Exec_CompiledOutputCache
{
public:
    Exec_CompiledOutputCache() = default;
    
    Exec_CompiledOutputCache(const Exec_CompiledOutputCache &) = delete;
    Exec_CompiledOutputCache &operator=(const Exec_CompiledOutputCache &) =
        delete;

    /// Insert a masked output corresponding to the output key.
    ///
    /// \return true if a new mapping was inserted for \p key, or false if a
    /// mapping already existed for \p key, in which case the existing mapping
    /// is not modified.
    ///
    bool Insert(
        const Exec_OutputKey::Identity &key,
        const VdfMaskedOutput &maskedOutput);

    /// Find a masked output in the compiled output cache. Returns a tuple with
    /// the masked output for the provided \p key, and a boolean denoting
    /// whether an entry was found for the \p key. An invalid masked output 
    /// will be returned if no entry was found.
    ///
    std::tuple<const VdfMaskedOutput &, bool> Find(
        const Exec_OutputKey::Identity &key) const;

    /// Erases all entries whose VdfMaskedOutput%s are owned by the node with
    /// id \p nodeId.
    ///
    /// \note
    /// This method is not thread-safe.
    ///
    void EraseByNodeId(VdfId nodeId);

private:
    const VdfMaskedOutput _invalidMaskedOutput;

    // Maps output keys to masked outputs.
    using _OutputMap =
        tbb::concurrent_unordered_map<
            Exec_OutputKey::Identity, VdfMaskedOutput, TfHash>;
    _OutputMap _outputMap;

    // Maps nodes to output keys. This map is used for "reverse" lookups into
    // the outputMap, so we can quickly identify which masked outputs need to
    // be purged in response to uncompilation.
    using _ReverseMap =
        tbb::concurrent_unordered_map<
            VdfId, tbb::concurrent_vector<Exec_OutputKey::Identity>>;
    _ReverseMap _reverseMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif