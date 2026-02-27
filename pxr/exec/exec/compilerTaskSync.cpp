//
// Copyright 2025 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/exec/exec/compilerTaskSync.h"

#include "pxr/exec/exec/compilerTaskSyncBase.h"
#include "pxr/exec/exec/outputKey.h"

#include <utility>

PXR_NAMESPACE_OPEN_SCOPE

template <class KeyType>
Exec_CompilerTaskSync<KeyType>::Exec_CompilerTaskSync(
    WorkDispatcher &dispatcher)
    : Exec_CompilerTaskSyncBase(dispatcher)
{}

template <class KeyType>
Exec_CompilerTaskSync<KeyType>::~Exec_CompilerTaskSync() = default;

template <class KeyType>
Exec_CompilerTaskSyncBase::ClaimResult
Exec_CompilerTaskSync<KeyType>::Claim(
    const KeyType &key,
    Exec_CompilationTask *task)
{
    // Add the key to the map. If another task got to claiming it first, it's
    // expected and safe for the key to already have an waitlist.
    const auto &[iterator, inserted] = _waitlists.emplace(
        std::piecewise_construct, 
            std::forward_as_tuple(key),
            std::forward_as_tuple());
    _Waitlist *const waitlist = &iterator->second;

    return _Claim(waitlist, task);
}

template <class KeyType>
Exec_CompilerTaskSyncBase::WaitResult
Exec_CompilerTaskSync<KeyType>::WaitOn(
    const KeyType &key,
    Exec_CompilationTask *task)
{
    // Add the key to the map. If another task got to claiming it first, it's
    // expected and safe for the key to already have an waitlist.
    const auto &[iterator, inserted] = _waitlists.emplace(
        std::piecewise_construct, 
            std::forward_as_tuple(key),
            std::forward_as_tuple());
    _Waitlist *const waitlist = &iterator->second;

    return _WaitOn(waitlist, task);
}

template <class KeyType>
void
Exec_CompilerTaskSync<KeyType>::MarkDone(const KeyType &key)
{
    // Get the waitlist for the key. If previously claimed or waited on, the
    // waitlist will exist. If not, we create a new waitlist in case a future
    // caller attempts to wait on this key.
    const auto &[iterator, inserted] = _waitlists.emplace(
        std::piecewise_construct, 
            std::forward_as_tuple(key),
            std::forward_as_tuple());
    _Waitlist *const waitlist = &iterator->second;
 
    // We expect the waitlist to not already be marked done.
    const bool wasNotDone = _MarkDone(waitlist);
    TF_VERIFY(wasNotDone);
}

template <class KeyType>
void
Exec_CompilerTaskSync<KeyType>::MarkAllDone()
{
    // TODO: We may later add a separate atomic flag that "closes" the task
    // sync, so that all keys added in the future are automatically marked
    // done. Currently, there is no need for this flag, because we only
    // interrupt the compilation process (and therefore only call this method)
    // when all tasks are blocked in a cycle, meaning no task should attempt to
    // Claim or WaitOn any key after this method is called.

    for (auto &[key, waitlist] : _waitlists) {
        _MarkDone(&waitlist);
    }
}

// Explicit template instantiations.
template class Exec_CompilerTaskSync<Exec_OutputKey::Identity>;
template class Exec_CompilerTaskSync<const VdfInput *>;
template class Exec_CompilerTaskSync<const VdfNode *>;

PXR_NAMESPACE_CLOSE_SCOPE
