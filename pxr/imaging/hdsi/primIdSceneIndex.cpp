//
// Copyright 2026 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdsi/primIdSceneIndex.h"

#include "pxr/imaging/hd/primIdSchema.h"
#include "pxr/imaging/hd/sceneGlobalsSchema.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/imaging/hd/dataSource.h"
#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/overlayContainerDataSource.h"
#include "pxr/imaging/hd/retainedDataSource.h"
#include "pxr/imaging/hd/sceneIndexPrimView.h"
#include "pxr/imaging/hd/tokens.h"

#include "pxr/usd/sdf/pathTable.h"

#include "pxr/base/tf/stl.h"
#include "pxr/base/tf/token.h"
#include "pxr/base/trace/trace.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <variant>
#include <vector>

PXR_NAMESPACE_OPEN_SCOPE

namespace HdsiPrimIdSceneIndex_Impl
{

//
// Compact version of std::optional<uint32_t>.
//
// Assumes that that stored value is less than 2^31.
//
class _OptionalPrimId
{
public:
    _OptionalPrimId()
      : _value(-1) { }
    _OptionalPrimId(const uint32_t value)
      : _value(static_cast<int32_t>(value)) {}

    explicit operator bool() const { return _value >= 0; }

    uint32_t operator*() const { return static_cast<uint32_t>(_value); }

private:
    // Negative value indicates it is an empty optional.
    // Non-negative value stores the value.
    int32_t _value;
};

// Scene index state.
//
// Mapping between imageable prim paths and their prim ids.
//
class _PrimIdInfo
{
public:
    // Assign an id to the given path if it does not have one yet.
    // Returns true if a new id was assigned.
    bool AssignIdToPath(const SdfPath &path)
    {
        _OptionalPrimId &id = _pathToId[path];
        if (id) {
            // Already has a prim Id.
            return false;
        }
        // Assign the new prim Id.
        id = _AllocateId(path);
        return true;
    }

    // Free the id of the given path but not the descendants.
    // Returns true if the prim had an Id (and it was freed).
    bool RemoveAssignedIdForPath(const SdfPath &path)
    {
        const auto it = _pathToId.find(path);
        if (it == _pathToId.end() || !it->second) {
            return false;
        }
        _FreeId(*it->second);
        if (it.HasChild()) {
            // We cannot delete it from the SdfPathTable because it
            // has (imageable) descendants.
            it->second = {};
        } else {
            // Safe to delete since it has no descendants.
            _pathToId.erase(it);
        }
        return true;
    }

    // Free the ids of the given path and all its descendants.
    // Returns true if any id was freed.
    bool RemoveSubtree(const SdfPath &path)
    {
        bool freed = false;
        const auto [begin, end] = _pathToId.FindSubtreeRange(path);
        for (auto it = begin; it != end; ++it) {
            if (it->second) {
                _FreeId(*it->second);
                freed = true;
            }
        }
        // _pathToId is an SdfPathTable, so all descendants get removed
        // automatically.
        _pathToId.erase(path);
        return freed;
    }

    void Clear()
    {
        TfReset(_pathToId);
        TfReset(_idToPathOrNextFreeId);
        _firstFreeId = {};
    }

    // The id assigned to path or the empty optional.
    _OptionalPrimId GetId(const SdfPath &path) const
    {
        const auto it = _pathToId.find(path);
        if (it == _pathToId.end()) {
            return {};
        }
        return it->second;
    }

    // The total number of assigned and free ids. It is an upper bound
    // of any prim id.
    size_t GetNumIds() const
    {
        return _idToPathOrNextFreeId.size();
    }

    // The path for id, or an empty path if id is out of range or free.
    SdfPath GetPathFromId(const size_t id) const
    {
        if (id >= _idToPathOrNextFreeId.size()) {
            return {};
        }
        const SdfPath * const path =
            std::get_if<SdfPath>(&_idToPathOrNextFreeId[id]);
        if (!path) {
            // A free id.
            return {};
        }
        return *path;
    }

private:
    // Allocate an id for the given path.
    uint32_t _AllocateId(const SdfPath &path)
    {
        if (_firstFreeId) {
            // Re-use a free Id if possible.
            const uint32_t id = *_firstFreeId;
            // The slot holds the next free id in the list.
            try {
                _firstFreeId =
                    std::get<_OptionalPrimId>(_idToPathOrNextFreeId[id]);
                _idToPathOrNextFreeId[id] = path;
            }
            catch(const std::bad_variant_access&)
            {
                TF_VERIFY(
                    false,
                    "Expected _OptionalPrimId for slot pointed to by "
                    "_firstFreeId.");
            }
            return id;
        }
        // Use a new Id.
        const uint32_t id = static_cast<uint32_t>(_idToPathOrNextFreeId.size());
        _idToPathOrNextFreeId.emplace_back(path);
        return id;
    }

    // Free the given id.
    void _FreeId(const uint32_t id)
    {
        // Push it onto the linked list.
        _idToPathOrNextFreeId[id] = _firstFreeId;
        _firstFreeId = id;
    }

    // Prim path to optional prim Id.
    //
    // We do not add an entry for non-imageable prims.
    // However, the SdfPathTable adds ancestors of imageable prims
    // if necessary (which will be empty _OptionalPrimId's).
    //
    SdfPathTable<_OptionalPrimId> _pathToId;

    // Inverse of the above path table.
    //
    // If the element at index i is an SdfPath, then i is the Id assigned to
    // the prim at that path. All other elements form a linked list of
    // unassigned Ids. The tail of the linked list is indicated by
    // an empty _OptionalPrimId.
    //
    // That is, if the element at index i is holding an _OptionalPrimId,
    // then i itself is an unassigned Id. If the _OptionalPrimId is non-empty,
    // then it contains the next unassigned Id.
    std::vector<std::variant<SdfPath, _OptionalPrimId>> _idToPathOrNextFreeId;

    // Head of the linked list of free ids (not necessarily the smallest
    // free Id).
    _OptionalPrimId _firstFreeId;
};

// Prim-level container data source for an imagable prim overlaying the
// the given input data source with the prim Id schema.
class _PrimDataSource : public HdContainerDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimDataSource);

    TfTokenVector GetNames() override
    {
        TfTokenVector names =
            _inputDataSource ? _inputDataSource->GetNames() : TfTokenVector();
        if (std::find(names.begin(), names.end(),
                      HdPrimIdSchema::GetSchemaToken()) == names.end()) {
            names.push_back(HdPrimIdSchema::GetSchemaToken());
        }
        return names;
    }

    HdDataSourceBaseHandle Get(const TfToken &name) override
    {
        if (name == HdPrimIdSchema::GetSchemaToken()) {
            return _ComputePrimIdDataSource();
        }
        if (_inputDataSource) {
            return _inputDataSource->Get(name);
        }
        return nullptr;
    }

private:
    _PrimDataSource(
        HdContainerDataSourceHandle const &inputDataSource,
        _PrimIdInfoSharedPtr const &primIdInfo,
        const SdfPath &primPath)
      : _inputDataSource(inputDataSource)
      , _primIdInfo(primIdInfo)
      , _primPath(primPath)
    {
    }

    // Looks up primId in table and puts it into HdPrimIdSchema.
    HdContainerDataSourceHandle _ComputePrimIdDataSource() const
    {
        TRACE_FUNCTION();

        const _OptionalPrimId id = _primIdInfo->GetId(_primPath);
        if (!id) {
            return nullptr;
        }
        return HdPrimIdSchema::Builder()
            .SetPrimId(
                HdRetainedTypedSampledDataSource<uint32_t>::New(*id))
            .Build();
    }

    HdContainerDataSourceHandle const _inputDataSource;
    _PrimIdInfoSharedPtr const _primIdInfo;
    const SdfPath _primPath;
};

// Reverse prim id to path table as data source.
class _PrimIdTableDataSource : public HdVectorDataSource
{
public:
    HD_DECLARE_DATASOURCE(_PrimIdTableDataSource);

    size_t GetNumElements() override
    {
        return _primIdInfo->GetNumIds();
    }

    HdDataSourceBaseHandle GetElement(const size_t element) override
    {
        const SdfPath path = _primIdInfo->GetPathFromId(element);
        if (path.IsEmpty()) {
            return nullptr;
        }
        return HdRetainedTypedSampledDataSource<SdfPath>::New(path);
    }

private:
    _PrimIdTableDataSource(_PrimIdInfoSharedPtr const &primIdInfo)
      : _primIdInfo(primIdInfo)
    {
    }

    _PrimIdInfoSharedPtr const _primIdInfo;
};

} // namespace HdsiPrimIdSceneIndex_Impl

using namespace HdsiPrimIdSceneIndex_Impl;

HdsiPrimIdSceneIndex::HdsiPrimIdSceneIndex(
        HdSceneIndexBaseRefPtr const &inputScene)
  : HdSingleInputFilteringSceneIndexBase(inputScene)
  , _primIdInfo(std::make_shared<_PrimIdInfo>())
  , _rootPrimOverlayDataSource(
      HdRetainedContainerDataSource::New(
          HdSceneGlobalsSchema::GetSchemaToken(),
          HdSceneGlobalsSchema::Builder()
              .SetPrimIdToPath(_PrimIdTableDataSource::New(_primIdInfo))
              .Build()))
{
    TRACE_FUNCTION();

    const HdSceneIndexBaseRefPtr &input = _GetInputSceneIndex();

    for (const SdfPath &primPath : HdSceneIndexPrimView(input)) {
        if (HdPrimTypeIsGprim(input->GetPrim(primPath).primType)) {
            _primIdInfo->AssignIdToPath(primPath);
        }
    }
}

HdsiPrimIdSceneIndex::~HdsiPrimIdSceneIndex() = default;

HdSceneIndexPrim
HdsiPrimIdSceneIndex::GetPrim(const SdfPath &primPath) const
{
    TRACE_FUNCTION();

    HdSceneIndexPrim prim = _GetInputSceneIndex()->GetPrim(primPath);

    if (primPath.IsAbsoluteRootPath()) {
        // Serve the primIdToPath field of the HdSceneGlobalsSchema
        // for the root prim.
        prim.dataSource =
            HdCreateOverlayContainerDataSource(
                _rootPrimOverlayDataSource, prim.dataSource);
        return prim;
    }

    if (HdPrimTypeIsGprim(prim.primType)) {
        // Wrap prim to populate the primId schema (lazily).
        prim.dataSource = _PrimDataSource::New(
            prim.dataSource, _primIdInfo, primPath);
    }

    return prim;
}

SdfPathVector
HdsiPrimIdSceneIndex::GetChildPrimPaths(const SdfPath &primPath) const
{
    return _GetInputSceneIndex()->GetChildPrimPaths(primPath);
}

void
HdsiPrimIdSceneIndex::_PrimsAdded(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    bool primIdsChanged = false;

    {
        TRACE_SCOPE("Updating prim id maps");

        for (const HdSceneIndexObserver::AddedPrimEntry &entry : entries) {
            if (HdPrimTypeIsGprim(entry.primType)) {
                // Assign a new primId unless the prim already has one.
                if (_primIdInfo->AssignIdToPath(entry.primPath)) {
                    primIdsChanged = true;
                }
            } else {
                // Free the primId if the prim had one.
                if (_primIdInfo->RemoveAssignedIdForPath(entry.primPath)) {
                    primIdsChanged = true;
                }
            }
        }
    }

    if (!_IsObserved()) {
        return;
    }

    _SendPrimsAdded(entries);

    if (primIdsChanged) {
        _SendPrimsDirtied(
            { { SdfPath::AbsoluteRootPath(),
                HdSceneGlobalsSchema::GetPrimIdToPathLocator() } });
    }
}

void
HdsiPrimIdSceneIndex::_PrimsRemoved(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::RemovedPrimEntries &entries)
{
    TRACE_FUNCTION();

    bool primIdsChanged = false;

    for (const HdSceneIndexObserver::RemovedPrimEntry &entry : entries) {
        const SdfPath &primPath = entry.primPath;

        if (primPath.IsAbsoluteRootPath()) {
            _primIdInfo->Clear();
            primIdsChanged = true;
            break;
        }

        // Free the ids of prim path and all its descendants.
        if (_primIdInfo->RemoveSubtree(primPath)) {
            primIdsChanged = true;
        }
    }

    if (!_IsObserved()) {
        return;
    }

    _SendPrimsRemoved(entries);

    if (primIdsChanged) {
        _SendPrimsDirtied(
            { { SdfPath::AbsoluteRootPath(),
                        HdSceneGlobalsSchema::GetPrimIdToPathLocator() } });
    }
}

void
HdsiPrimIdSceneIndex::_PrimsDirtied(
        const HdSceneIndexBase &sender,
        const HdSceneIndexObserver::DirtiedPrimEntries &entries)
{
    _SendPrimsDirtied(entries);
}

PXR_NAMESPACE_CLOSE_SCOPE
