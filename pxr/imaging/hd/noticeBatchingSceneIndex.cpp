//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hd/noticeBatchingSceneIndex.h"

#include "pxr/base/trace/trace.h"

PXR_NAMESPACE_OPEN_SCOPE

HdNoticeBatchingSceneIndex::_BatchEntry::~_BatchEntry() = default;

HdNoticeBatchingSceneIndex::HdNoticeBatchingSceneIndex(
    const HdSceneIndexBaseRefPtr &inputScene)
: HdSingleInputFilteringSceneIndexBase(inputScene)
, _batchingEnabled(false)
{}

HdNoticeBatchingSceneIndex::~HdNoticeBatchingSceneIndex() = default;

HdSceneIndexPrim
HdNoticeBatchingSceneIndex::GetPrim(const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetPrim(primPath);
}

SdfPathVector
HdNoticeBatchingSceneIndex::GetChildPrimPaths(const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdNoticeBatchingSceneIndex::_PrimsAdded(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    if (_batchingEnabled) {
        TRACE_FUNCTION();

        if (!_batches.empty()) {
            if (_PrimsAddedBatchEntry *batchEntry =
                    dynamic_cast<_PrimsAddedBatchEntry*>(
                        _batches.back().get())) {
                batchEntry->entries.insert(
                    batchEntry->entries.end(), entries.begin(), entries.end());
                return;
            }
        }

        _PrimsAddedBatchEntry *batchEntry = new _PrimsAddedBatchEntry;
        _batches.emplace_back(batchEntry);
         batchEntry->entries = entries;

    } else {
        _SendPrimsAdded(entries);
    }
}

void
HdNoticeBatchingSceneIndex::_PrimsRemoved(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    if (_batchingEnabled) {
        TRACE_FUNCTION();

        if (!_batches.empty()) {
            if (_PrimsRemovedBatchEntry *batchEntry =
                    dynamic_cast<_PrimsRemovedBatchEntry*>(
                        _batches.back().get())) {
                batchEntry->entries.insert(
                    batchEntry->entries.end(), entries.begin(), entries.end());
                return;
            }
        }

        _PrimsRemovedBatchEntry *batchEntry = new _PrimsRemovedBatchEntry;
        _batches.emplace_back(batchEntry);
         batchEntry->entries = entries;

    } else {
        _SendPrimsRemoved(entries);
    }
}

void
HdNoticeBatchingSceneIndex::_PrimsDirtied(
    const HdSceneIndexBase &sender,
    const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    if (_batchingEnabled) {
        TRACE_FUNCTION();

        if (!_batches.empty()) {
            if (_PrimsDirtiedBatchEntry *batchEntry =
                    dynamic_cast<_PrimsDirtiedBatchEntry*>(
                        _batches.back().get())) {
                batchEntry->entries.insert(
                    batchEntry->entries.end(), entries.begin(), entries.end());
                return;
            }
        }

        _PrimsDirtiedBatchEntry *batchEntry = new _PrimsDirtiedBatchEntry;
        _batches.emplace_back(batchEntry);
         batchEntry->entries = entries;

    } else {
        _SendPrimsDirtied(entries);
    }
}

bool
HdNoticeBatchingSceneIndex::IsBatchingEnabled() const
{
    return _batchingEnabled;
}

void
HdNoticeBatchingSceneIndex::SetBatchingEnabled(bool enabled)
{
    if (_batchingEnabled != enabled) {
        _batchingEnabled = enabled;
        if (!enabled && !_batches.empty()) {
            Flush();
        }
    }
}

void
HdNoticeBatchingSceneIndex::Flush()
{
    for (const std::unique_ptr<_BatchEntry> &batchEntry : _batches) {
        if (_PrimsAddedBatchEntry *typedEntry =
                dynamic_cast<_PrimsAddedBatchEntry *>(batchEntry.get())) {
            _SendPrimsAdded(typedEntry->entries);
        } else if (_PrimsDirtiedBatchEntry *typedEntry =
                dynamic_cast<_PrimsDirtiedBatchEntry *>(batchEntry.get())) {
            _SendPrimsDirtied(typedEntry->entries);
        } else if (_PrimsRemovedBatchEntry *typedEntry =
                dynamic_cast<_PrimsRemovedBatchEntry *>(batchEntry.get())) {
            _SendPrimsRemoved(typedEntry->entries);
        }
    }

    _batches.clear();
}

PXR_NAMESPACE_CLOSE_SCOPE
