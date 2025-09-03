//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_EXEC_UNCOMPILATION_TABLE_H
#define PXR_EXEC_EXEC_UNCOMPILATION_TABLE_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/exec/api.h"
#include "pxr/exec/exec/uncompilationRuleSet.h"

#include "pxr/exec/esf/journal.h"
#include "pxr/exec/vdf/types.h"
#include "pxr/usd/sdf/path.h"

#ifdef TBB_PREVIEW_CONCURRENT_ORDERED_CONTAINERS
#include <tbb/concurrent_map.h>
#else
#define TBB_PREVIEW_CONCURRENT_ORDERED_CONTAINERS 1
#include <tbb/concurrent_map.h>
#undef TBB_PREVIEW_CONCURRENT_ORDERED_CONTAINERS
#endif

#include <memory>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

/// Maps scene objects to uncompilation rules.
///
/// The table maps scene object paths to Exec_UncompilationRuleSet%s for that
/// path. The rule set then contains all Vdf objects to be uncompiled, and the
/// appropriate EsfEditReason for each.
///
class Exec_UncompilationTable
{
public:
    /// \name Compilation API
    ///
    /// These methods are invoked during compilation to record uncompilation
    /// rules for newly-compiled network objects.
    ///
    /// \note
    /// All methods in this group can be called concurrently with each other.
    ///
    /// @{

    /// Inserts uncompilation rules for a VdfNode.
    ///
    /// The node with id \p nodeId should be uncompiled for any scene change
    /// that matches an entry in the \p journal.
    ///
    /// \note
    /// This method may only be called concurrently with itself and
    /// AddRulesForInput.
    ///
    EXEC_API void AddRulesForNode(
        VdfId nodeId,
        const EsfJournal &journal);

    /// Inserts uncompilation rules for a VdfInput.
    ///
    /// The input \p inputName on node \p nodeId should be uncompiled for any
    /// scene change that matches an entry in the \p journal.
    ///
    /// \note
    /// This method may only be called concurrently with itself and
    /// AddRulesForNode.
    ///
    EXEC_API void AddRulesForInput(
        VdfId nodeId,
        const TfToken &inputName,
        const EsfJournal &journal);

    /// @}

    /// \name Uncompilation API
    ///
    /// These APIs are used during scene change processing to identify which
    /// parts of the network need to be uncompiled.
    ///
    /// @{

    /// Describes the result of a lookup into the uncompilation table.
    ///
    struct Entry
    {
        /// The rule set corresponds to the scene object at this path.
        SdfPath path;

        /// Pointer to a rule set. Ownership of the rule set is shared by this
        /// object and the table that created it.
        ///
        std::shared_ptr<Exec_UncompilationRuleSet> ruleSet;

        Entry() = default;

        Entry(
            const SdfPath &path_,
            const std::shared_ptr<Exec_UncompilationRuleSet> &ruleSet_)
            : path(path_)
            , ruleSet(ruleSet_)
        {}

        Entry(
            const SdfPath &path_,
            std::shared_ptr<Exec_UncompilationRuleSet> &&ruleSet_)
            : path(path_)
            , ruleSet(std::move(ruleSet_))
        {}

        /// The entry evaluates true iff it contains a non-null rule set.
        operator bool() const {
            return ruleSet != nullptr;
        }
    };

    /// Locates the rule set for the given \p path.
    ///
    /// If not found, the returned entry's rule set is nullptr.
    ///
    EXEC_API Entry Find(const SdfPath &path);

    /// Locates and removes all rule sets prefixed with the given \p path.
    ///
    /// A recursive resync effectively deletes objects from the scene, and the
    /// uncompilation table responds by removing rule sets for those objects.
    ///
    /// Each matching entry (path and rule set) is moved into the result vector.
    ///
    /// This method is not thread-safe.
    ///
    EXEC_API std::vector<Entry> UpdateForRecursiveResync(const SdfPath &path);

    /// @}

private:
    // Locates an existing rule set for \p path, or inserts a new empty rule set
    // if no such rule set exists.
    //
    // Returns a reference to the new-or-existing rule set.
    //
    // \note
    // This method may concurrently be called with itself.
    //
    Exec_UncompilationRuleSet &_FindOrInsert(const SdfPath &path);

    using _ConcurrentMap = tbb::concurrent_map<
        SdfPath,
        std::shared_ptr<Exec_UncompilationRuleSet>>;
    _ConcurrentMap _ruleSets;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
