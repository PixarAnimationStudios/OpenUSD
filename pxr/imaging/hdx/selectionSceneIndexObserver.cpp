//
// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include "pxr/imaging/hdx/selectionSceneIndexObserver.h"

#include "pxr/imaging/hd/dataSourceTypeDefs.h"
#include "pxr/imaging/hd/instanceIndicesSchema.h"
#include "pxr/imaging/hd/instancerTopologySchema.h"
#include "pxr/imaging/hd/sceneIndex.h"
#include "pxr/imaging/hd/selectionSchema.h"
#include "pxr/imaging/hd/selectionsSchema.h"
#include "pxr/base/work/loops.h"

#include "pxr/base/trace/trace.h"

#include <mutex>

PXR_NAMESPACE_OPEN_SCOPE

namespace
{

//
// If we have nested instancers we obtain an instance id for
// each level. For HdSelection::AddInstance, we encode this sequence
// of nested ids indices in a single integer.
//
// For example, assume we selected
//     the instances with id 5 and 6 of 10 in the outer most instancer
//     the instances with id 3 of 12 in the next instancer
//     the instances with id 7 and 8 of 15 in the inner most instancer.
// So in total, we have 2 * 1 * 2 = 4 nested instances selected. The
// corresponding encoded nested instance ids are:
//
//  [ (5 * 12 + 3) * 15 + 7,
//    (5 * 12 + 3) * 15 + 8,
//    (6 * 12 + 3) * 15 + 7,
//    (6 * 12 + 3) * 15 + 8 ].
//
class EncodedNestedInstanceIds
{
public:
    // Given selected instance ids for the next inner level
    // of instancing, compute the new encoded nested instance ids.
    //
    EncodedNestedInstanceIds
    AppendInnerInstanceIds(
        const VtIntArray &instanceIds,
        const int numInstances)
    {
        EncodedNestedInstanceIds result;
        if (_ids) {
            const size_t n = _ids->size();
            const size_t m = instanceIds.size();

            result._ids = VtIntArray(n * m);

            VtIntArray &resultIds = *result._ids;

            for (size_t i = 0; i < n; i++) {
                const int b = numInstances * (*_ids)[i];
                for (size_t j = 0; j < m; j++) {
                    resultIds[i * m + j] = b + instanceIds[j];
                }
            }
        } else {
            result._ids = instanceIds;
        }
        return result;
    }

    // The encoded nested instance ids.
    //
    // Null for a prim that has no instancers.
    //
    const std::optional<VtIntArray> &GetIds() const {
        return _ids;
    }

private:
    std::optional<VtIntArray> _ids;
};

void
_AddPrimToSelection(
    const SdfPath &primPath,
    const EncodedNestedInstanceIds &encodedNestedInstanceIds,
    HdSelectionSharedPtr const &result,
    std::mutex &resultMutex)
{
    if (encodedNestedInstanceIds.GetIds()) {
        // HdSelection::AddInstance expects nested instance ids.
        // See explanation for EncodedNestedInstanceIds.
        //
        // For HdSelection::AddInstance, nothing is everything.
        // That is, passing an empty array selects all instances.
        // This is not what we intend here. An empty array can result from
        // user selecting an instance proxy path to an invis'd instance.
        if (!encodedNestedInstanceIds.GetIds()->empty()) {
            std::unique_lock lock(resultMutex);

            result->AddInstance(
                HdSelection::HighlightModeSelect,
                primPath,
                *encodedNestedInstanceIds.GetIds());
        }
    } else {
        std::unique_lock lock(resultMutex);

        result->AddRprim(
            HdSelection::HighlightModeSelect,
            primPath);
    }
}
    
}

static void
_AddToSelection(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &primPath,
    HdSelectionSharedPtr const &result,
    std::mutex &resultMutex);

HdxSelectionSceneIndexObserver::HdxSelectionSceneIndexObserver()
 : _version(0)
 , _selection(std::make_shared<HdSelection>())
{
}

static
void
_PopulateFromSceneIndex(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &path,
    HdSelectionSharedPtr const &result,
    std::mutex &resultMutex)
{
    TRACE_FUNCTION();

    _AddToSelection(sceneIndex, path, result, resultMutex);
    const SdfPathVector childPaths = sceneIndex->GetChildPrimPaths(path);
    if (!childPaths.empty()) {
        WorkParallelForEach(
            childPaths.begin(), childPaths.end(),
            [&](SdfPath const& childPath) {
                _PopulateFromSceneIndex(
                    sceneIndex, childPath, result, resultMutex);
            });
    }
}

void
HdxSelectionSceneIndexObserver::SetSceneIndex(
    HdSceneIndexBaseRefPtr const &sceneIndex)
{
    TRACE_FUNCTION();

    if (sceneIndex == _sceneIndex) {
        return;
    }

    HdSceneIndexObserverPtr self(this);

    if (_sceneIndex) {
        _sceneIndex->RemoveObserver(self);
    }

    _sceneIndex = sceneIndex;
    _selection = std::make_shared<HdSelection>();
    _dirtiedPrims.clear();

    if (_sceneIndex) {
        sceneIndex->AddObserver(self);
        std::mutex resultMutex;
        _PopulateFromSceneIndex(
            sceneIndex, SdfPath::AbsoluteRootPath(), _selection, resultMutex);
    }
    
    _version++;
}

int
HdxSelectionSceneIndexObserver::GetVersion() const
{
    return _version;
}

HdSelectionSharedPtr
HdxSelectionSceneIndexObserver::GetSelection()
{
    if (!_dirtiedPrims.empty()) {
        _selection = _ComputeSelection();
    }

    return _selection;
}

//
// Wraps HdInstancerTopologySchema::ComputeInstanceIndicesForProto.
//
static
VtIntArray
_GetInstanceIndicesForPrototypeIndex(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &instancerPath,
    const int prototypeIndex)
{
    TRACE_FUNCTION();

    const HdInstancerTopologySchema instancerTopologySchema =
        HdInstancerTopologySchema::GetFromParent(
            sceneIndex->GetPrim(instancerPath).dataSource);

    HdPathArrayDataSourceHandle const prototypesDs =
        instancerTopologySchema.GetPrototypes();
    if (!prototypesDs) {
        return {};
    }

    const VtArray<SdfPath> prototypes = prototypesDs->GetTypedValue(0.0f);
    if (prototypeIndex < 0 ||
        static_cast<size_t>(prototypeIndex) >= prototypes.size()) {
        return {};
    }

    return instancerTopologySchema.ComputeInstanceIndicesForProto(
        prototypes[prototypeIndex]);
}

//
// Calls HdInstancerTopologySchema::ComputeInstanceIndicesForProto
// using the instancer and prototype from the HdInstanceIndicesSchema.
//
static
VtIntArray
_GetInstanceIndicesForPrototype(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const HdInstanceIndicesSchema &instanceIndicesSchema)
{
    TRACE_FUNCTION();

    HdPathDataSourceHandle const instancerPathDs =
        instanceIndicesSchema.GetInstancer();
    if (!instancerPathDs) {
        return { };
    }
    const SdfPath instancerPath = instancerPathDs->GetTypedValue(0.0f);
    if (instancerPath.IsEmpty()) {
        return { };
    }

    HdIntDataSourceHandle const prototypeIndexDs =
        instanceIndicesSchema.GetPrototypeIndex();
    if (!prototypeIndexDs) {
        return { };
    }

    return _GetInstanceIndicesForPrototypeIndex(
        sceneIndex,
        instancerPath,
        prototypeIndexDs->GetTypedValue(0.0f));
}

//
// Instance indices from instance indices schema. 
//
static
VtIntArray
_GetSelectedInstanceIndices(
    const HdInstanceIndicesSchema &selectedInstanceIndicesSchema)
{
    HdIntArrayDataSourceHandle const dataSource =
        selectedInstanceIndicesSchema.GetInstanceIndices();
    if (!dataSource) {
        return {};
    }
    return dataSource->GetTypedValue(0.0f);
}

//
// Note: There are two ways to identify an instance by an integer.
//
// We say instance index if the index is into the arrays that
// are assigned to, for example, UsdGeom's PointInstancer.protoIndices,
// PointInstancer.positions, ... or instance-rate primvars of either
// UsdGeom's PointInstancer or the Hydra instancer.
//
// The HdInstancerTopologySchema::ComputeInstanceIndicesForProto
// returns the indices of the visible instances of a given prototype
// as a VtArray<int>.
//
// If we identify an instance as an index into the result of
// HdInstancerTopologySchema::ComputeInstanceIndicesForProto, we
// call it an instance id.
//
// This function converts from instance indices to instance ids.
// It also gives the number of instance ids, that is of visible
// instance of the given given prototype.
//
static
std::pair<VtIntArray, int>
_GetSelectedInstanceIdsAndNumForPrototype(
    const VtIntArray &instanceIndicesForPrototype,
    const VtIntArray &selectedInstanceIndices)
{
    TRACE_FUNCTION();

    VtIntArray sorted(selectedInstanceIndices);
    std::sort(sorted.begin(), sorted.end());

    VtIntArray selectedInstanceIds;

    const size_t n = instanceIndicesForPrototype.size();
    for (size_t i = 0; i < n; i++) {
        if (std::binary_search(sorted.cbegin(), sorted.cend(),
                               instanceIndicesForPrototype[i])) {
            selectedInstanceIds.push_back(i);
        }
    }

    return { selectedInstanceIds, n };
}

//
// Convert data in schema to instance ids.
//
static
std::pair<VtIntArray, int>
_GetSelectedInstanceIdsAndNumForPrototype(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const HdInstanceIndicesSchema &selectedInstanceIndicesSchema)
{
    TRACE_FUNCTION();

    return
        _GetSelectedInstanceIdsAndNumForPrototype(
            _GetInstanceIndicesForPrototype(
                sceneIndex,
                selectedInstanceIndicesSchema),
            _GetSelectedInstanceIndices(
                selectedInstanceIndicesSchema));
}    

static
EncodedNestedInstanceIds
_GetEncodedSelectedNestedInstanceIds(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const HdInstanceIndicesVectorSchema &selectedNestedInstanceIndices)
{
    TRACE_FUNCTION();

    EncodedNestedInstanceIds encodedNestedInstanceIds;

    const size_t depth = selectedNestedInstanceIndices.GetNumElements();

    // Going from outer most to inner most instancer.

    // Continuing with the above example, the numbers after the
    // first iteration:
    // [5, 6],
    // second iteration:
    // [5 * 12 + 3, 6 * 12 + 3]
    // third iteration:
    // as above.

    for (size_t d = 0; d < depth; d++) {
 
        auto [ selectedInstanceIdsForPrototype, num ] =
            _GetSelectedInstanceIdsAndNumForPrototype(
                sceneIndex,
                selectedNestedInstanceIndices.GetElement(d));

        encodedNestedInstanceIds =
            encodedNestedInstanceIds.AppendInnerInstanceIds(
                selectedInstanceIdsForPrototype, num);
    }

    return encodedNestedInstanceIds;
}

// Given one of the data source under the selections locator
// for the prim at path in the scene index, process it and add
// to result.
static
void
_AddToSelection(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &primPath,
    const HdSelectionSchema &selectionSchema,
    HdSelectionSharedPtr const &result,
    std::mutex &resultMutex)
{
    TRACE_FUNCTION();

    // Only support fully selected for now.
    HdBoolDataSourceHandle const ds = selectionSchema.GetFullySelected();
    if (!ds) {
        return;
    }

    if (!ds->GetTypedValue(0.0f)) {
        return;
    }

    _AddPrimToSelection(
        primPath,
        // Retrieve instancing information.
        _GetEncodedSelectedNestedInstanceIds(
            sceneIndex,
            selectionSchema.GetNestedInstanceIndices()),
        result,
        resultMutex);
}

// Query prim at path in scene index for selections data source
// and add to result.
static
void
_AddToSelection(
    HdSceneIndexBaseRefPtr const &sceneIndex,
    const SdfPath &primPath,
    HdSelectionSharedPtr const &result, 
    std::mutex &resultMutex)
{
    TRACE_FUNCTION();
    
    const HdSelectionsSchema selectionsSchema =
        HdSelectionsSchema::GetFromParent(
            sceneIndex->GetPrim(primPath).dataSource);
    if (!selectionsSchema) {
        return;
    }

    {
        TRACE_FUNCTION_SCOPE("Resolving selection schema");

        const size_t n = selectionsSchema.GetNumElements();
        for (size_t i = 0; i < n; ++i) {
            const HdSelectionSchema selectionSchema =
                selectionsSchema.GetElement(i);
            _AddToSelection(
                sceneIndex,
                primPath,
                selectionSchema,
                result, 
                resultMutex);
        }
    }    
}    

HdSelectionSharedPtr
HdxSelectionSceneIndexObserver::_ComputeSelection()
{
    TRACE_FUNCTION();

    HdSelectionSharedPtr const result = std::make_shared<HdSelection>();

    if (!_sceneIndex) {
        return result;
    }

    const SdfPathVector prims = _selection->GetAllSelectedPrimPaths();

    _dirtiedPrims.insert(prims.begin(), prims.end());

    if (!_dirtiedPrims.empty()) {
        TRACE_FUNCTION_SCOPE("Query prims for selection");
        
        std::mutex resultMutex;

        // On comparison with using WorkParallelForN with both a path set and
        // path vector, WorkParallelForEach seems to perform best even though
        // each worker thread processes just one element at a time, rather than
        // a range.
        //
        WorkParallelForEach(
            _dirtiedPrims.begin(), _dirtiedPrims.end(),
            [&](SdfPath const& path) {
                _AddToSelection(_sceneIndex, path, result, resultMutex);
            });

        _dirtiedPrims.clear();
    }

    return result;
}

void
HdxSelectionSceneIndexObserver::PrimsAdded(
    const HdSceneIndexBase &sender,
    const AddedPrimEntries &entries)
{
    TRACE_FUNCTION();

    if (entries.empty()) {
        return;
    }

    ++_version;

    for (const AddedPrimEntry &entry : entries) {
        _dirtiedPrims.insert(entry.primPath);
    }
}

void
HdxSelectionSceneIndexObserver::PrimsDirtied(
    const HdSceneIndexBase &sender,
    const DirtiedPrimEntries &entries)
{
    TRACE_FUNCTION();

    for (const DirtiedPrimEntry &entry : entries) {
        if (entry.dirtyLocators.Contains(
                HdSelectionsSchema::GetDefaultLocator())) {
            ++_version;
            _dirtiedPrims.insert(entry.primPath);
        }
    }
}

void
HdxSelectionSceneIndexObserver::PrimsRemoved(
    const HdSceneIndexBase &sender,
    const RemovedPrimEntries &entries)
{
    if (entries.empty()) {
        return;
    }

    ++_version;
}

void
HdxSelectionSceneIndexObserver::PrimsRenamed(
        const HdSceneIndexBase &sender,
        const RenamedPrimEntries &entries)
{
    ConvertPrimsRenamedToRemovedAndAdded(sender, entries, this);
}

PXR_NAMESPACE_CLOSE_SCOPE
