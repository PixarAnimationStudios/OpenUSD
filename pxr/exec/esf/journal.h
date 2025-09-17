//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#ifndef PXR_EXEC_ESF_JOURNAL_H
#define PXR_EXEC_ESF_JOURNAL_H

/// \file

#include "pxr/pxr.h"

#include "pxr/exec/esf/editReason.h"

#include "pxr/base/tf/denseHashMap.h"
#include "pxr/base/tf/hash.h"
#include "pxr/usd/sdf/path.h"

PXR_NAMESPACE_OPEN_SCOPE

/// Stores a collection of edit reasons associated with scene objects.
///
/// Exec compilation uses an EsfJournal to log all scene queries performed
/// while compiling a node or forming connections in the exec network.
/// An instance of EsfJournal is passed to public methods of scene adapter
/// interfaces (e.g. EsfPrimInterface::GetParent), and those methods add entries
/// to the journal.
///
/// Given the scene adapter method calls made to produce a node in the network,
/// the resulting journal contains all scene changes that would trigger
/// uncompilation of that node. Likewise, when the exec compiler uses the scene
/// adapter to identify the connections flowing into a node in the exec network,
/// the resulting journal contains all scene changes that would trigger
/// uncompilation of those connections.
///
class EsfJournal
{
    using _HashMap = TfDenseHashMap<SdfPath, EsfEditReason, TfHash>;

public:
    /// Adds or updates a new entry in the journal.
    ///
    /// If a journal entry already exists for \p path, then its edit reasons
    /// are extended by \p editReason.
    ///
    /// Returns a reference to the current journal, so multiple Add calls can be
    /// chained together.
    ///
    EsfJournal &Add(const SdfPath &path, EsfEditReason editReason) {
        if (TF_VERIFY(path.IsAbsolutePath()) && TF_VERIFY(!path.IsEmpty())) {
            _hashMap[path] |= editReason;
        }
        return *this;
    }

    /// Merges the entries from the \p other EsfJournal into this one.
    ///
    /// If this journal and \p other have entries for the same path, then
    /// the merged entry contains the union of both reasons.
    ///
    void Merge(const EsfJournal &other) {
        for (const value_type &entry : other) {
            _hashMap[entry.first] |= entry.second;
        }
    }

    bool operator==(const EsfJournal &other) const {
        return _hashMap == other._hashMap;
    }

    bool operator!=(const EsfJournal &other) const {
        return _hashMap != other._hashMap;
    }

    /// \name Iteration API
    ///
    /// These types and methods allow EsfJournal to be used in range-based
    /// for-loops.
    ///
    /// @{
    /// 
    using value_type = _HashMap::value_type;
    using iterator = _HashMap::iterator;
    using const_iterator = _HashMap::const_iterator;

    iterator begin() & {
        return _hashMap.begin();
    }

    iterator end() & {
        return _hashMap.end();
    }

    const_iterator begin() const & {
        return _hashMap.begin();
    }

    const_iterator end() const & {
        return _hashMap.end();
    }

    const_iterator cbegin() const & {
        return _hashMap.begin();
    }

    const_iterator cend() const & {
        return _hashMap.end();
    }
    /// @}

private:
    _HashMap _hashMap;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif
