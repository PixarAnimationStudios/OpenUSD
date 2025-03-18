//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdsi/debuggingSceneIndex.h"

#include "pxr/base/tf/debug.h"

#include <optional>
#include <variant>

PXR_NAMESPACE_OPEN_SCOPE

namespace HdsiDebuggingSceneIndex_Impl
{

// Some policy decisions.

// Do we allow prims at property paths?
const bool allowPropertyPaths = true;

// If we get a AddedPrimEntry for /foo/bar and there was no prim at /foo,
// we mark /foo as existing in namespace. Do we also assume that this
// implicitly added prim has empty prim type?
//
// The HdFlatteningSceneIndex, for example, implements this behavior.
// It does produce a non-trivial data source for /foo.
const bool implicitlyAddedAncestorsHaveEmptyType = true;

// Per prim-info.
//
// We always store a prim info for all ancestors as well.
// In particular, we always store a prim info for the absolute root /.
//
struct _PrimInfo
{
    // Does a prim exist?
    //
    // Note that the HdSceneIndexBase does not specify whether a prim "exists".
    //
    // There are two notions of existence:
    // - The strong form is that GetPrim("/foo") returns a non-empty prim type
    //   or non-null data source handle.
    // - The weak form (existence in namespace) is that a prim exists at a path
    //   if (without a subsequent PrimRemovedEntry):
    //   * We have received a PrimAddedEntry for the path or a descendant path
    //   * GetPrim for path or a descendant path returned a non-empty prim type
    //     or non-null data source
    //   * GetChildPrimPaths for path or a descendant path was non-empty
    //   * path is in GetChildPrimPaths(parentPath)
    //
    // Here we assume the weaker form.
    //
    // If a prim exists (in namespace), there will be a prim-info for all its
    // ancestors which are also assumed to exist.
    //
    // Note that the debugging scene index (lazily) only queries GetPrim or
    // GetChildPrimPaths itself when the client calls that method.
    //
    // If we receive a PrimRemovedEntry, we set _PrimInfo::exists = false for
    // the corresponding prim info. Such a _PrimInfo has no descendants.
    //
    // Note that the implementation could be changed to just use a bool if
    // we were to use _PrimInfo::insert instead of _PrimInfo[].
    //
    std::optional<bool> existsInNamespace;

    // Do we know all children of this prim?
    //
    // True if GetChildPrimPaths(path) was called or we received
    // PrimRemovedEntry(path).
    //
    bool allChildrenKnown = false;

    // primType if known.
    std::optional<TfToken> primType;

    // Does this prim have a non-null ptr data source?
    //
    // Future work might store more information about the data source and wrap
    // it so that we can track which values were returned to a client.
    //
    std::optional<bool> hasDataSource;
};

void _EmitMessage(const std::string &message)
{
    // Future work might throw an error/show a stack trace/...
    TfDebug::Helper().Msg(
        "[HdsiDebuggingSceneIndex] %s\n", message.c_str());
}

void _EmitError(const std::string &message)
{
    _EmitMessage(
        TfStringPrintf(
            "ERROR: %s", message.c_str()));
}

const char * const
_DataSourceString(const bool hasDataSource)
{
    if (hasDataSource) {
        return "non-null data source";
    } else {
        return "null data source";
    }
}

// Like SdfPathAncestorsRange returned by SdfPath::GetAncestorsRange()
// but includes "/".
class _Ancestors
{
public:
    _Ancestors(const SdfPath &path) : _path(path) {}

    struct iterator
    {
        iterator(const SdfPath &path) : _path(path) {}
        iterator() = default;

        iterator& operator++()
        {
            if (!_path.IsEmpty()) {
                _path = _path.GetParentPath();
            }
            return *this;
        }

        const SdfPath& operator*() const { return _path; }

        bool operator!=(const iterator &o) const { return _path != o._path; }

    private:
        SdfPath _path;
    };

    iterator begin() const { return iterator(_path); }
    iterator end() const { return iterator(); };

private:
    SdfPath _path;
};

// Update prims as follows:
// - Mark prim at primPath as existing in namespace.
// - Optionally, set the primType and hasDataSource for the prim at primPath.
// - Also mark all ancestors as existing in namespace.
// - Check whether there are contradictions with what previously stored in
//   prims.
//
// callsite is either "GetPrim" or "GetChildPrimPaths" and used when printing
// messages about inconsistencies.
//
void
_MarkPrimAsExistingInNamespace(
    _PrimMap * const prims,
    const char * const callsite,
    const SdfPath &primPath,
    const std::optional<TfToken> &primType = std::nullopt,
    const std::optional<HdContainerDataSourceHandle> &dataSource = std::nullopt)
{
    size_t level = 0;
    std::optional<bool> existedInNamespace;
    std::optional<bool> childExistedInNamespace;

    for (const SdfPath &ancestor : _Ancestors(primPath)) {
        _PrimInfo &primInfo = (*prims)[ancestor];

        existedInNamespace = std::exchange(
            primInfo.existsInNamespace, std::optional<bool>(true));
        if (existedInNamespace == std::optional<bool>(false)) {
            _EmitError(
                TfStringPrintf(
                    "%s(%s) returned non-trivial result even though the prim "
                    "at %s was established to not exist in namespace.",
                    callsite, primPath.GetText(), ancestor.GetText()));
        }

        if (level == 0) {
            if (primType) {
                if (primInfo.primType) {
                    if (*primInfo.primType != *primType) {
                        _EmitError(
                            TfStringPrintf(
                                "%s(%s) returned prim type %s even though "
                                "the prim was established to be of type %s.",
                                callsite,
                                primPath.GetText(),
                                primType->GetText(),
                                primInfo.primType->GetText()));
                    }
                }
                primInfo.primType = primType;
            }

            if (dataSource) {
                const bool hasDataSource(*dataSource);
                if (primInfo.hasDataSource) {
                    if (*primInfo.hasDataSource != hasDataSource) {
                        _EmitError(
                            TfStringPrintf(
                                "%s(%s) returned %s even though the prim "
                                "was established to have a %s.",
                                callsite,
                                primPath.GetText(),
                                _DataSourceString(hasDataSource),
                                _DataSourceString(*primInfo.hasDataSource)));
                    }
                }
                primInfo.hasDataSource = hasDataSource;
            }
        } else {
            if (primInfo.allChildrenKnown &&
                childExistedInNamespace != std::optional<bool>(true)) {
                _EmitError(
                    TfStringPrintf(
                        "%s(%s) returned a non-trivial result even though "
                        "prim %s does not have a corresponding child.",
                        callsite,
                        primPath.GetText(),
                        ancestor.GetText()));
            }
        }
        childExistedInNamespace = existedInNamespace;
        ++level;
    }
}

// Update prims as follows:
// - Mark prim at primPath as not existing in namespace.
// - Mark prim at primPath to know all its children.
// - Delete all descendants.
void
_MarkPrimAsNonExistingInNamespace(
    _PrimMap * const prims,
    const SdfPath &primPath)
{
    auto it = prims->insert({primPath, {}}).first;
    it->second.existsInNamespace = std::optional<bool>(false);
    it->second.allChildrenKnown = true;
    it->second.primType = std::nullopt;
    it->second.hasDataSource = std::nullopt;

    // Delete all descendants.
    ++it; // But not the prim itself.

    while (it != prims->end() && it->first.HasPrefix(primPath)) {
        it = prims->erase(it);
    }
}

bool
_IsValidPrimPath(const SdfPath &primPath)
{
    if (primPath.IsAbsoluteRootPath()) {
        return true;
    }
    if (primPath.IsPrimPath()) {
        return true;
    }
    if (allowPropertyPaths && primPath.IsPropertyPath()) {
        return true;
    }
    return false;
}

}

using namespace HdsiDebuggingSceneIndex_Impl;

HdsiDebuggingSceneIndexRefPtr
HdsiDebuggingSceneIndex::New(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    HdContainerDataSourceHandle const &inputArgs)
{
    return TfCreateRefPtr(
        new HdsiDebuggingSceneIndex(inputSceneIndex, inputArgs));
}

HdsiDebuggingSceneIndex::HdsiDebuggingSceneIndex(
    HdSceneIndexBaseRefPtr const &inputSceneIndex,
    HdContainerDataSourceHandle const &inputArgs)
  : HdSingleInputFilteringSceneIndexBase(inputSceneIndex)
  , _prims{{SdfPath::AbsoluteRootPath(), _PrimInfo{/*exists = */ true}}}
{
    _EmitMessage(
        TfStringPrintf(
            "Instantiated for %s.",
            _GetInputSceneIndex()->GetDisplayName().c_str()));
}

HdsiDebuggingSceneIndex::~HdsiDebuggingSceneIndex() = default;

HdSceneIndexPrim
HdsiDebuggingSceneIndex::GetPrim(const SdfPath &primPath) const
{
    const HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (!primPath.IsAbsolutePath()) {
        _EmitError(
            TfStringPrintf(
                "GetPrim(%s) was called with relative path.",
                primPath.GetText()));
        return prim;
    }
    if (!_IsValidPrimPath(primPath)) {
        _EmitError(
            TfStringPrintf(
                "GetPrim(%s) was called with non-prim/property path.",
                primPath.GetText()));
        return prim;
    }

    const bool exists = !prim.primType.IsEmpty() || prim.dataSource;

    {
        std::lock_guard<std::mutex> guard(_primsMutex);

        if (exists) {
            _MarkPrimAsExistingInNamespace(
                &_prims,
                "GetPrim",
                primPath,
                prim.primType,
                prim.dataSource);
        } else {
            const auto it = _prims.find(primPath);
            if (it != _prims.end()) {
                if (it->second.primType && !it->second.primType->IsEmpty()) {
                    _EmitError(
                        TfStringPrintf(
                            "GetPrim(%s) returned a trivial result even "
                            "though the prim was previously established of "
                            "type %s.",
                            primPath.GetText(),
                            it->second.primType->GetText()));
                }
                if (it->second.hasDataSource && *it->second.hasDataSource) {
                    _EmitError(
                        TfStringPrintf(
                            "GetPrim(%s) returned a trivial result even "
                            "though the prim was previously established to "
                            "have a non-null data source.",
                            primPath.GetText()));
                }
            }
        }
    }

    return prim;
}

SdfPathVector
HdsiDebuggingSceneIndex::GetChildPrimPaths(const SdfPath &primPath) const
{
    const SdfPathVector childPrimPaths =
        _GetInputSceneIndex()->GetChildPrimPaths(primPath);

    if (!primPath.IsAbsolutePath()) {
        _EmitError(
            TfStringPrintf(
                "GetChildPrimPaths(%s) was called with relative path.",
                primPath.GetText()));
        return childPrimPaths;
    }
    if (!_IsValidPrimPath(primPath)) {
        _EmitError(
            TfStringPrintf(
                "GetChildPrimPaths(%s) was called with non-prim/property path.",
                primPath.GetText()));
        return childPrimPaths;
    }

    for (const SdfPath &childPrimPath : childPrimPaths) {
        if (!childPrimPath.IsAbsolutePath()) {
            _EmitError(
                TfStringPrintf(
                    "GetChildPrimPaths(%s) returned non-absolute path %s.",
                    primPath.GetText(), childPrimPath.GetText()));
        }
        if (!_IsValidPrimPath(childPrimPath)) {
            _EmitError(
                TfStringPrintf(
                    "GetChildPrimPaths(%s) returned non-prim/property path %s.",
                    primPath.GetText(), childPrimPath.GetText()));
        }

        if (childPrimPath.GetParentPath() != primPath) {
            _EmitError(
                TfStringPrintf(
                    "GetChildPrimPaths(%s) returned non-child path %s.",
                    primPath.GetText(), childPrimPath.GetText()));
        }
    }

    const bool existsInNamespace = !childPrimPaths.empty();

    {
        std::lock_guard<std::mutex> guard(_primsMutex);


        // All children reported by GetChildPrimPaths.
        const SdfPathSet childPrimPathSet(
            childPrimPaths.begin(), childPrimPaths.end());

        {
            // We need to check that every child of primPath in _prims is
            // also in GetChildPrimPaths.

            // Go through prim and all its descendants.
            for (auto it = _prims.find(primPath);
                 it != _prims.end() && it->first.HasPrefix(primPath);
                 ++it) {
                if (it->first.GetPathElementCount()
                        != primPath.GetPathElementCount() + 1) {
                    // Not an immediate child.
                    continue;
                }
                if (it->second.existsInNamespace == std::optional<bool>(true)) {
                    if (childPrimPathSet.count(it->first) == 0) {
                        _EmitError(
                            TfStringPrintf(
                                "GetChildPrimPaths(%s) does not include %s "
                                "even though it was established to exist.",
                                primPath.GetText(), it->first.GetText()));
                    }
                }
            }
        }

        // Set allChildrenKnown. Remember previous value.
        const bool allChildrenKnown = std::exchange(
            _prims[primPath].allChildrenKnown, true);

        // We also need to do the check the other way around. That is
        // do a look-up in _prims for path in GetChildPrimPaths.
        //
        for (const SdfPath &childPrimPath : childPrimPaths) {
            // We set the prim to exist for each such path.
            //
            // We remember the previous value.
            const std::optional<bool> childExistsInNamespace =
                std::exchange(
                    _prims[childPrimPath].existsInNamespace,
                    std::optional<bool>(true));

            if (childExistsInNamespace == std::optional<bool>(false)) {
                _EmitError(
                    TfStringPrintf(
                        "GetChildPrimPaths(%s) includes %s even though the "
                        "prim was established to not exist.",
                        primPath.GetText(), childPrimPath.GetText()));
            } else {
                if ( allChildrenKnown &&
                     childExistsInNamespace != std::optional<bool>(true)) {
                    _EmitError(
                        TfStringPrintf(
                            "GetChildPrimPaths(%s) includes %s even though "
                            "the prim was not included in a previous call to "
                            "GetChildPrimPaths or its parent was deleted "
                            "without it being re-added.",
                            primPath.GetText(), childPrimPath.GetText()));
                }
            }
        }

        if (existsInNamespace) {
            _MarkPrimAsExistingInNamespace(
                &_prims, "GetChildPrimPaths", primPath);
        }
    }

    return childPrimPaths;
}

void
HdsiDebuggingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    {
        std::lock_guard<std::mutex> guard(_primsMutex);

        for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
            if (!entry.primPath.IsAbsolutePath()) {
                _EmitError(
                    TfStringPrintf(
                        "AddedPrimsEntry with relative path %s.",
                        entry.primPath.GetText()));
                continue;
            }
            if (!_IsValidPrimPath(entry.primPath)) {
                _EmitError(
                    TfStringPrintf(
                        "AddedPrimsEntry with non-prim/property path %s.",
                        entry.primPath.GetText()));
                continue;
            }

            size_t level = 0;

            for (const SdfPath &ancestor : _Ancestors(entry.primPath)) {
                _PrimInfo &primInfo = _prims[ancestor];

                const std::optional<bool> existedInNamespace = std::exchange(
                    primInfo.existsInNamespace, std::optional<bool>(true));

                if (level == 0) {
                    primInfo.primType = entry.primType;
                } else {
                    if (implicitlyAddedAncestorsHaveEmptyType) {
                        if (existedInNamespace == std::optional<bool>(false)) {
                            primInfo.primType = TfToken();
                        }
                    }
                }

                ++level;
            }
        }
    }

    _SendPrimsAdded(entries);
}

void
HdsiDebuggingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    {
        std::lock_guard<std::mutex> guard(_primsMutex);

        for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
            if (!entry.primPath.IsAbsolutePath()) {
                _EmitError(
                    TfStringPrintf(
                        "RemovedPrimsEntry with relative path %s.",
                        entry.primPath.GetText()));
                continue;
            }
            if (!_IsValidPrimPath(entry.primPath)) {
                _EmitError(
                    TfStringPrintf(
                        "RemovedPrimsEntry with non-prim/property path %s.",
                        entry.primPath.GetText()));
                continue;
            }

            _MarkPrimAsNonExistingInNamespace(&_prims, entry.primPath);
        }
    }

    _SendPrimsRemoved(entries);
}

void
HdsiDebuggingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    {
        std::lock_guard<std::mutex> guard(_primsMutex);

        for (const HdSceneIndexObserver::DirtiedPrimEntry &entry : entries) {
            if (!entry.primPath.IsAbsolutePath()) {
                _EmitError(
                    TfStringPrintf(
                        "DirtiedPrimsEntry with relative path %s.",
                        entry.primPath.GetText()));
                continue;
            }
            if (!_IsValidPrimPath(entry.primPath)) {
                _EmitError(
                    TfStringPrintf(
                        "DirtiedPrimsEntry with non-prim/property path %s.",
                        entry.primPath.GetText()));
                continue;
            }
        }
    }

    _SendPrimsDirtied(entries);
}

void
HdsiDebuggingSceneIndex::_PrimsRenamed(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RenamedPrimEntries &entries)
{
    {
        std::lock_guard<std::mutex> guard(_primsMutex);

        for (const HdSceneIndexObserver::RenamedPrimEntry &entry : entries) {
            if (!entry.oldPrimPath.IsAbsolutePath()) {
                _EmitError(
                    TfStringPrintf(
                        "RenamedPrimsEntry with relative old path %s.",
                        entry.oldPrimPath.GetText()));
                continue;
            }
            if (!_IsValidPrimPath(entry.oldPrimPath)) {
                _EmitError(
                    TfStringPrintf(
                        "RenamedPrimsEntry with non-prim/property old path %s.",
                        entry.oldPrimPath.GetText()));
                continue;
            }
            if (!entry.newPrimPath.IsAbsolutePath()) {
                _EmitError(
                    TfStringPrintf(
                        "RenamedPrimsEntry with relative new path %s.",
                        entry.newPrimPath.GetText()));
                continue;
            }
            if (!_IsValidPrimPath(entry.newPrimPath)) {
                _EmitError(
                    TfStringPrintf(
                        "RenamedPrimsEntry with non-prim/property new path %s.",
                        entry.newPrimPath.GetText()));
                continue;
            }
        }
    }

    if (!entries.empty()) {
        _EmitMessage(
            "Received RenamedPrimEntries but HdsiDebuggingSceneIndex does not "
            "support it (yet).");
    }

    _SendPrimsRenamed(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
